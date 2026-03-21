#include "editor.hpp"
#include "fileio.hpp"
#include "version.hpp"
#include "commands.hpp"
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <csignal>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <ctime>
#include <unistd.h>
#include <cctype>

volatile sig_atomic_t Editor::stopRequested = 0;

Editor::Editor() noexcept
    : term(),
      buffer(),
      renderer(term),
      filename(""),
      clipboard(""),
      cx(0),
      cy(0),
      rowOffset(0),
      colOffset(0),
      dirty(false),
      statusMessage("Welcome to Textor Studio")
{
    try {
        loadConfig();
        undoStack.reserve(1000);
        redoStack.reserve(1000);
        renderer.setBuffer(&buffer);
        std::signal(SIGINT, Editor::onSignal);
        std::signal(SIGTERM, Editor::onSignal);
    } catch (...) {}
}

Editor::~Editor() noexcept {
    try {
        safeExit();
    } catch (...) {}
}

void Editor::loadConfig() noexcept {
    const char* h = std::getenv("HOME");
    if (!h) return;
    std::ifstream ifs(std::string(h) + "/.tserc");
    if (!ifs) return;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line == "theme=white") {
            ts::bg = "\x1b[107m";
            ts::fg = "\x1b[30m";
            ts::reset = "\x1b[0m\x1b[30m\x1b[107m";
            ts::gray = "\x1b[90m";
            ts::red = "\x1b[31m";
            ts::green = "\x1b[32m";
            ts::yellow = "\x1b[33m";
            ts::blue = "\x1b[34m";
            ts::magenta = "\x1b[35m";
            ts::cyan = "\x1b[36m";
        }
    }
}

void Editor::saveConfig() noexcept {
    const char* h = std::getenv("HOME");
    if (!h) return;
    std::ofstream ofs(std::string(h) + "/.tserc");
    if (ofs) {
        std::string t = (ts::bg == "\x1b[107m") ? "white" : "black";
        ofs << "theme=" << t << "\n";
    }
}

void Editor::resetConfig() noexcept {
    const char* h = std::getenv("HOME");
    if (h) {
        std::string path = std::string(h) + "/.tserc";
        std::remove(path.c_str());
    }
    ts::bg = "\x1b[48;5;234m";
    ts::fg = "\x1b[37m";
    ts::reset = "\x1b[0m";
    ts::red = "\x1b[91m";
    ts::green = "\x1b[92m";
    ts::yellow = "\x1b[93m";
    ts::blue = "\x1b[94m";
    ts::gray = "\x1b[90m";
    ts::magenta = "\x1b[95m";
    ts::cyan = "\x1b[96m";
    buffer.lines().clear();
    buffer.lines().emplace_back("");
    filename = "";
    dirty = false;
    cx = cy = rowOffset = colOffset = 0;
    undoStack.clear();
    redoStack.clear();
}

void Editor::onSignal(int signal) noexcept {
    (void)signal;
    stopRequested = 1;
}

void Editor::safeExit() noexcept {
    std::cout << "\x1b[0m\x1b[2J\x1b[H";
    std::cout.flush();
    term.showCursor();
    term.disableRawMode();
}

void Editor::pushSnapshot() noexcept {
    if (undoStack.size() >= 1000) {
        undoStack.erase(undoStack.begin());
    }
    undoStack.push_back(buffer.lines());
    redoStack.clear();
}

void Editor::undo() noexcept {
    if (undoStack.empty()) {
        statusMessage = "Error: Nothing to undo";
        return;
    }
    redoStack.push_back(buffer.lines());
    buffer.setLines(undoStack.back());
    undoStack.pop_back();
    dirty = true;
    statusMessage = "Undo successful";
    validateCursorBounds();
}

void Editor::redo() noexcept {
    if (redoStack.empty()) {
        statusMessage = "Error: Nothing to redo";
        return;
    }
    undoStack.push_back(buffer.lines());
    buffer.setLines(redoStack.back());
    redoStack.pop_back();
    dirty = true;
    statusMessage = "Redo successful";
    validateCursorBounds();
}

void Editor::validateCursorBounds() noexcept {
    if (buffer.lines().empty()) {
        buffer.lines().emplace_back("");
    }
    if (cy < 0) cy = 0;
    if (cy >= static_cast<int>(buffer.lines().size())) {
        cy = static_cast<int>(buffer.lines().size()) - 1;
    }
    int lineLength = static_cast<int>(buffer.lines()[static_cast<size_t>(cy)].size());
    if (cx < 0) cx = 0;
    if (cx > lineLength) cx = lineLength;
}

void Editor::run() {
    term.enableRawMode();
    while (!stopRequested) {
        ensureCursorVisible();
        renderer.render(cx, cy, rowOffset, colOffset, dirty, statusMessage);
        
        int key = term.readKey();
        if (key == -1 || key == -2) continue;

        if (key >= 128 && key <= 255) { 
            std::string utf8_char;
            utf8_char += static_cast<char>(key);
            int extra_bytes = 0;
            if ((key & 0xE0) == 0xC0)      extra_bytes = 1;
            else if ((key & 0xF0) == 0xE0) extra_bytes = 2;
            else if ((key & 0xF8) == 0xF0) extra_bytes = 3;

            for (int i = 0; i < extra_bytes; ++i) {
                unsigned char next_b;
                if (read(STDIN_FILENO, &next_b, 1) == 1) {
                    utf8_char += next_b;
                }
            }
            insertCharAction(utf8_char);
        } 
        else {
            handleInput(key);
        }
    }
    safeExit();
}

void Editor::ensureCursorVisible() noexcept {
    validateCursorBounds();
    const int vRows = renderer.rowsVisible();
    const int gutter = renderer.gutterWidth();
    int vCols = renderer.colsVisible() - gutter;
    if (vCols < 1) vCols = 1;

    if (cy < rowOffset) {
        rowOffset = cy;
    } else if (cy >= rowOffset + vRows) {
        rowOffset = cy - vRows + 1;
    }

    if (cx < colOffset) {
        colOffset = cx;
    } else if (cx >= colOffset + vCols) {
        colOffset = cx - vCols + 1;
    }

    if (rowOffset < 0) rowOffset = 0;
    if (colOffset < 0) colOffset = 0;
}

void Editor::handleInput(int c) noexcept {
    if (c == -1 || c == -2) return;

    const int KEY_UP = 1000, KEY_DOWN = 1001, KEY_LEFT = 1002, KEY_RIGHT = 1003;
    const int CTRL_UP = 1004, CTRL_DOWN = 1005, CTRL_RIGHT = 1006, CTRL_LEFT = 1007;
    const int HOME = 1010, DEL = 1011, END = 1012, PAGE_UP = 1013, PAGE_DOWN = 1014;
    const int CTRL_O = 15, CTRL_K = 11, CTRL_X = 24, CTRL_U = 21, CTRL_Z = 26, CTRL_Y = 25;
    const int CTRL_F = 6, CTRL_A = 1, CTRL_E = 5, CTRL_L = 12, CTRL_D = 4, CTRL_T = 20;

    switch (c) {
        case CTRL_X:
            if (dirty) {
                std::string confirm = prompt("Discard changes? (y/n): ");
                if (confirm != "y") break;
            }
            safeExit();
            std::exit(0);
            break;

        case CTRL_O:
            saveFileAction();
            break;

        case CTRL_T:
            commandBarAction();
            break;

        case CTRL_Z:
            undo();
            break;

        case CTRL_Y:
            redo();
            break;

        case CTRL_K:
            if (cy < static_cast<int>(buffer.lines().size())) {
                pushSnapshot();
                clipboard = buffer.lines()[static_cast<size_t>(cy)];
                buffer.eraseLine(cy);
                if (buffer.lines().empty()) buffer.lines().emplace_back("");
                dirty = true;
                statusMessage = "Line cut";
            }
            break;

        case CTRL_U:
            if (!clipboard.empty()) {
                pushSnapshot();
                buffer.insertLine(cy, clipboard);
                cy++;
                dirty = true;
                statusMessage = "Line pasted";
            }
            break;

        case CTRL_F: {
            std::string query = prompt("Search: ");
            if (!query.empty()) {
                bool found = false;
                for (size_t i = 0; i < buffer.lines().size(); ++i) {
                    size_t pos = buffer.lines()[i].find(query);
                    if (pos != std::string::npos) {
                        cy = static_cast<int>(i);
                        cx = static_cast<int>(pos);
                        statusMessage = "Found at line " + std::to_string(i + 1);
                        found = true;
                        break;
                    }
                }
                if (!found) statusMessage = "Not found: " + query;
            }
            break;
        }

        case CTRL_A:
        case HOME:
            cx = 0;
            break;

        case CTRL_E:
        case END:
            cx = static_cast<int>(buffer.lines()[static_cast<size_t>(cy)].size());
            break;

        case CTRL_L:
            statusMessage = "Screen Refreshed";
            break;

        case CTRL_D:
        case DEL:
            if (cx < static_cast<int>(buffer.lines()[cy].size())) {
                pushSnapshot();
                buffer.lines()[cy].erase(static_cast<size_t>(cx), 1);
                dirty = true;
            } else if (cy < static_cast<int>(buffer.lines().size()) - 1) {
                pushSnapshot();
                buffer.lines()[cy] += buffer.lines()[cy + 1];
                buffer.eraseLine(cy + 1);
                dirty = true;
            }
            break;

        case CTRL_LEFT: {
            if (cx > 0) {
                const std::string &line = buffer.lines()[cy];
                int tempCx = cx;
                while (tempCx > 0 && std::isspace(static_cast<unsigned char>(line[tempCx - 1]))) tempCx--;
                while (tempCx > 0 && !std::isspace(static_cast<unsigned char>(line[tempCx - 1]))) tempCx--;
                cx = tempCx;
            } else if (cy > 0) {
                cy--;
                cx = static_cast<int>(buffer.lines()[cy].size());
            }
            break;
        }

        case CTRL_RIGHT: {
            const std::string &line = buffer.lines()[cy];
            int tempCx = cx;
            if (tempCx < static_cast<int>(line.size())) {
                while (tempCx < static_cast<int>(line.size()) && !std::isspace(static_cast<unsigned char>(line[tempCx]))) tempCx++;
                while (tempCx < static_cast<int>(line.size()) && std::isspace(static_cast<unsigned char>(line[tempCx]))) tempCx++;
                cx = tempCx;
            } else if (cy < static_cast<int>(buffer.lines().size()) - 1) {
                cy++;
                cx = 0;
            }
            break;
        }

        case CTRL_UP:
            cy = std::max(0, cy - 5);
            break;

        case CTRL_DOWN:
            cy = std::min(static_cast<int>(buffer.lines().size()) - 1, cy + 5);
            break;

        case KEY_UP:
            if (cy > 0) cy--;
            break;

        case KEY_DOWN:
            if (cy < static_cast<int>(buffer.lines().size()) - 1) cy++;
            break;

        case KEY_LEFT:
            if (cx > 0) {
                size_t idx = static_cast<size_t>(cx);
                std::string &line = buffer.lines()[static_cast<size_t>(cy)];
                int move = 1;
                while (idx - move > 0 && (static_cast<unsigned char>(line[idx - move]) & 0xC0) == 0x80) move++;
                cx -= move;
            } else if (cy > 0) {
                cy--;
                cx = static_cast<int>(buffer.lines()[static_cast<size_t>(cy)].size());
            }
            break;

        case KEY_RIGHT:
            if (cx < static_cast<int>(buffer.lines()[static_cast<size_t>(cy)].size())) {
                size_t idx = static_cast<size_t>(cx);
                std::string &line = buffer.lines()[static_cast<size_t>(cy)];
                int move = 1;
                if ((static_cast<unsigned char>(line[idx]) & 0xC0) != 0x80) {
                    while (idx + move < line.size() && (static_cast<unsigned char>(line[idx + move]) & 0xC0) == 0x80) move++;
                }
                cx += move;
            } else if (cy < static_cast<int>(buffer.lines().size()) - 1) {
                cy++;
                cx = 0;
            }
            break;

        case PAGE_UP:
            cy = std::max(0, cy - renderer.rowsVisible());
            break;

        case PAGE_DOWN:
            cy = std::min(static_cast<int>(buffer.lines().size()) - 1, cy + renderer.rowsVisible());
            break;

        case '\r':
        case '\n':
            insertNewlineAction();
            break;

        case 127:
        case 8:
            backspaceAction();
            break;

        default:
            if (c >= 32 && c <= 126) {
                insertCharAction(std::string(1, static_cast<char>(c)));
            }
            break;
    }
}

void Editor::saveFileAction() noexcept {
    if (filename.empty()) {
        filename = prompt("Enter filename to save: ");
    }
    if (!filename.empty()) {
        if (buffer.save(filename)) {
            dirty = false;
            statusMessage = "Successfully saved to " + filename;
        } else {
            statusMessage = "Error: Could not save file!";
        }
    }
}

void Editor::commandBarAction() noexcept {
    std::string input = prompt(ts::green + ":" + ts::reset);
    if (input.empty()) return;

    size_t spacePos = input.find(' ');
    std::string cmd = input.substr(0, spacePos);
    std::string arg = (spacePos != std::string::npos) ? input.substr(spacePos + 1) : "";

    if (cmd == "exit" || cmd == "q" || cmd == "quit") {
        if (dirty && prompt(ts::yellow + "Discard changes? (y/n): " + ts::reset) != "y") return;
        safeExit();
        std::exit(0);
    }

    ts::CommandContext ctx { 
        buffer.lines(), 
        filename, 
        clipboard, 
        cx, 
        cy, 
        dirty,
        [this](const std::string& f) { loadOrCreate(f); },
        [this]() { saveFileAction(); },
        [this]() { undo(); },
        [this]() { redo(); },
        [this](const std::string& p) { return prompt(p); },
        [this]() { saveConfig(); },
        [this]() { resetConfig(); }
    };

    statusMessage = ts::handle(cmd, arg, ctx);
}

void Editor::insertCharAction(const std::string& s) noexcept {
    pushSnapshot();
    buffer.insertChar(cy, cx, s);
    cx += s.length();
    
    dirty = true;
    statusMessage = "";
}

void Editor::insertNewlineAction() noexcept {
    pushSnapshot();
    std::string &current = buffer.lines()[static_cast<size_t>(cy)];
    std::string remainder = current.substr(static_cast<size_t>(cx));
    current.erase(static_cast<size_t>(cx));
    buffer.insertLine(cy + 1, remainder);
    cy++;
    cx = 0;
    dirty = true;
}

void Editor::backspaceAction() noexcept {
    if (cx > 0) {
        pushSnapshot();
        std::string &line = buffer.lines()[static_cast<size_t>(cy)];
        
        int bytesToDelete = 1;
        while (cx - bytesToDelete > 0 && 
               (static_cast<unsigned char>(line[cx - bytesToDelete]) & 0xC0) == 0x80) {
            bytesToDelete++;
        }

        line.erase(static_cast<size_t>(cx - bytesToDelete), static_cast<size_t>(bytesToDelete));
        cx -= bytesToDelete;
        dirty = true;
    } else if (cy > 0) {
        pushSnapshot();
        size_t prevIdx = static_cast<size_t>(cy - 1);
        cx = static_cast<int>(buffer.lines()[prevIdx].size());
        buffer.lines()[prevIdx] += buffer.lines()[static_cast<size_t>(cy)];
        buffer.eraseLine(cy);
        cy--;
        dirty = true;
    }
}

std::string Editor::prompt(const std::string &msg) noexcept {
    term.disableRawMode();
    std::cout << "\x1b[H\x1b[K" << msg;
    std::string input;
    std::getline(std::cin, input);
    term.enableRawMode();
    return input;
}

void Editor::loadOrCreate(const std::string &path) noexcept {
    if (!buffer.load(path)) {
        buffer.lines().clear();
        buffer.lines().emplace_back("");
    }
    filename = path;
    cx = cy = rowOffset = colOffset = 0;
    dirty = false;
    undoStack.clear();
    redoStack.clear();
}

void Editor::open(const std::string &path) noexcept {
    term.enableRawMode();
    loadOrCreate(path);
}
