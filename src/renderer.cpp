#include "renderer.hpp"
#include "commands.hpp"
#include "version.hpp"
#include <algorithm>
#include <cstdio>
#include <vector>
#include <string>

Renderer::Renderer(Terminal &t) noexcept 
    : term(t), buffer(nullptr), screenRows(24), screenCols(80), commandHeight(1) {}

void Renderer::setBuffer(TextBuffer *b) noexcept {
    buffer = b;
}

int Renderer::calculateRequiredGutter() const noexcept {
    if (!buffer) return 4;
    size_t lines = buffer->lines().size();
    int count = 1;
    while (lines >= 10) { lines /= 10; count++; }
    return count + 2;
}

int Renderer::gutterWidth() const noexcept {
    return calculateRequiredGutter();
}

int Renderer::rowsVisible() const noexcept {
    return screenRows;
}

int Renderer::colsVisible() const noexcept {
    return screenCols;
}

void Renderer::buildGutter(std::string &frame, int lineNum, int width) const noexcept {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s%*d %s", ts::gray.c_str(), width - 1, lineNum, ts::fg.c_str());
    frame += buf;
}

void Renderer::applyClipping(std::string &frame, const std::string &line, int offset, int width) const noexcept {
    std::vector<std::string> tokens;
    std::vector<std::string> raw_tokens;
    
    size_t i = 0;
    while (i < line.size()) {
        size_t len = 1;
        unsigned char c = static_cast<unsigned char>(line[i]);
        if ((c & 0x80) == 0) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        
        if (i + len > line.size()) len = line.size() - i;
        raw_tokens.push_back(line.substr(i, len));
        i += len;
    }

    if (offset >= (int)raw_tokens.size()) return;
    
    int end = std::min((int)raw_tokens.size(), offset + width);
    std::vector<std::string> v;
    for (int j = offset; j < end; ++j) {
        v.push_back(raw_tokens[j]);
    }

    std::vector<std::string> kw = {
        "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue", "return", "goto",
        "try", "catch", "throw", "class", "struct", "enum", "union", "namespace", "using", "public", "private", 
        "protected", "virtual", "override", "final", "static", "const", "inline", "template", "typename", 
        "sizeof", "alignof", "decltype", "noexcept", "explicit", "operator", "new", "delete", "this"
    };
    std::vector<std::string> tp = {
        "int", "char", "bool", "float", "double", "void", "short", "long", "signed", "unsigned", "size_t", 
        "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t",
        "std", "string", "vector", "map", "set", "unique_ptr", "shared_ptr", "auto"
    };

    std::string w;
    bool inS = false;

    for (size_t j = 0; j <= v.size(); ++j) {
        std::string cur = (j < v.size()) ? v[j] : "";
        char c = (cur.size() == 1) ? cur[0] : '\0';

        if (inS) {
            frame += ts::yellow + cur;
            if (c == '"' || j == v.size()) { inS = false; frame += ts::fg; }
            continue;
        }

        if (!cur.empty() && (std::isalnum(static_cast<unsigned char>(cur[0])) || cur[0] == '_' || (static_cast<unsigned char>(cur[0]) > 127))) {
            w += cur;
        } else {
            if (!w.empty()) {
                if (std::find(kw.begin(), kw.end(), w) != kw.end()) frame += ts::blue + w + ts::fg;
                else if (std::find(tp.begin(), tp.end(), w) != tp.end()) frame += ts::cyan + w + ts::fg;
                else if (std::isdigit(static_cast<unsigned char>(w[0]))) frame += ts::yellow + w + ts::fg;
                else frame += ts::fg + w;
                w = "";
            }

            if (!cur.empty()) {
                if (c == '"') { inS = true; frame += ts::yellow + "\""; }
                else if (std::string("+-*/%=&|^!<>?:").find(c) != std::string::npos) frame += ts::green + cur + ts::fg;
                else if (std::string("(){}[]").find(c) != std::string::npos) frame += ts::magenta + cur + ts::fg;
                else if (c == '#' || c == '.' || c == ',' || c == ';') frame += ts::gray + cur + ts::fg;
                else frame += ts::fg + cur;
            }
        }
    }
}

void Renderer::buildStatusBar(std::string &frame, bool dirty, int totalLines) const noexcept {
    frame += "\x1b[7m";
    std::string path = buffer ? buffer->path() : "";
    std::string name = path.empty() ? "[No Name]" : path;
    std::string l = " " + name + (dirty ? " [+]" : " [-]") + " | Lines: " + std::to_string(totalLines);
    std::string r = ts::VERSION + " ";
    frame += l;
    int pad = screenCols - static_cast<int>(l.size()) - static_cast<int>(r.size());
    if (pad > 0) frame.append(static_cast<size_t>(pad), ' ');
    frame += r;
    frame += ts::reset;
    frame += "\r\n";
}

void Renderer::buildCommandBar(std::string &frame, const std::string &msg) noexcept {
    frame += "\x1b[K";
    if (msg.empty()) {
        commandHeight = 1;
        frame += ts::green + "^O" + ts::reset + " Save  " + ts::green + "^T" + ts::reset + " Cmd  " + ts::green + "^X" + ts::reset + " Exit";
    } else {
        int neededRows = (static_cast<int>(msg.size()) / screenCols) + 1;
        if (neededRows > 5) neededRows = 5;
        commandHeight = neededRows;

        for (int i = 0; i < commandHeight; ++i) {
            int start = i * screenCols;
            if (start < static_cast<int>(msg.size())) {
                frame += msg.substr(start, screenCols);
                if (i < commandHeight - 1) frame += "\r\n\x1b[K";
            }
        }
    }
}

void Renderer::syncPhysicalCursor(std::string &frame, int cx, int cy, int rOff, int cOff, int gutter) const noexcept {
    int py = (cy - rOff) + 1;
    
    int vx = 0;
    if (buffer && cy < (int)buffer->lines().size()) {
        const std::string& line = buffer->lines()[cy];
        int byte_idx = 0;
        int char_count = 0;
        while (byte_idx < cx && byte_idx < (int)line.size()) {
            unsigned char c = static_cast<unsigned char>(line[byte_idx]);
            int len = 1;
            if ((c & 0x80) == 0) len = 1;
            else if ((c & 0xE0) == 0xC0) len = 2;
            else if ((c & 0xF0) == 0xE0) len = 3;
            else if ((c & 0xF8) == 0xF0) len = 4;
            byte_idx += len;
            char_count++;
        }
        vx = char_count;
    } else {
        vx = cx;
    }

    int px = (vx - cOff) + gutter + 1;
    
    if (py < 1) py = 1;
    if (py > screenRows) py = screenRows;
    if (px < gutter + 1) px = gutter + 1;
    if (px > screenCols) px = screenCols;
    
    char move[32];
    std::snprintf(move, sizeof(move), "\x1b[%d;%dH", py, px);
    frame += move;
}

void Renderer::resize() noexcept {
    int r, c;
    term.getWindowSize(r, c);
    screenCols = (c > 0) ? c : 80;
    int reserved = 1 + commandHeight;
    if (r > reserved) {
        screenRows = r - reserved;
    } else {
        screenRows = 1;
    }
}

void Renderer::render(int cx, int cy, int rOff, int cOff, bool dirty, const std::string &statusMsg) noexcept {
    int msgLen = static_cast<int>(statusMsg.size());
    commandHeight = (statusMsg.empty()) ? 1 : (msgLen / screenCols) + 1;
    if (commandHeight > 5) commandHeight = 5;

    resize(); 
    
    std::string frame;
    frame.reserve(screenRows * screenCols + 2048);

    frame += "\x1b[?25l";
    frame += "\x1b[H";
    frame += ts::bg + ts::fg;

    int gutter = calculateRequiredGutter();
    int usable = screenCols - gutter;

    for (int y = 0; y < screenRows; ++y) {
        int fileIdx = y + rOff;
        frame += "\x1b[K";
        if (buffer && fileIdx >= 0 && fileIdx < static_cast<int>(buffer->lines().size())) {
            buildGutter(frame, fileIdx + 1, gutter);
            applyClipping(frame, buffer->lines()[fileIdx], cOff, usable);
        } else {
            frame += ts::gray + "~" + ts::fg;
        }
        if (y < screenRows - 1) {
            frame += "\r\n";
        }
    }

    frame += "\r\n";
    buildStatusBar(frame, dirty, buffer ? static_cast<int>(buffer->lines().size()) : 0);
    buildCommandBar(frame, statusMsg);
    
    syncPhysicalCursor(frame, cx, cy, rOff, cOff, gutter);
    frame += "\x1b[?25h";

    term.writeAll(frame);
}
