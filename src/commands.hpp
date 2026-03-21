#pragma once
#include "version.hpp"
#include <string>
#include <vector>
#include <functional>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <unistd.h>

namespace ts {
    inline std::string red = "\x1b[91m";
    inline std::string green = "\x1b[92m";
    inline std::string yellow = "\x1b[93m";
    inline std::string blue = "\x1b[94m";
    inline std::string magenta = "\x1b[95m";
    inline std::string cyan = "\x1b[96m";
    inline std::string gray = "\x1b[90m";
    inline std::string reset = "\x1b[0m";
    inline std::string bg = "\x1b[48;5;234m";
    inline std::string fg = "\x1b[37m";

    struct CommandContext {
        std::vector<std::string>& lines;
        std::string& filename;
        std::string& clipboard;
        int& cx;
        int& cy;
        bool& dirty;
        std::function<void(const std::string&)> loadFile;
        std::function<void()> saveFile;
        std::function<void()> undo;
        std::function<void()> redo;
        std::function<std::string(const std::string&)> prompt;
        std::function<void()> saveConfig;
        std::function<void()> resetConfig;
    };

    inline std::string handle(const std::string& cmd, const std::string& arg, CommandContext& ctx) {
        if (cmd == "open") {
            if (arg.empty()) return red + "Usage: open <file>" + reset;
            if (ctx.dirty && ctx.prompt(yellow + "Discard changes? (y/n): " + reset) != "y") return "Open cancelled";
            ctx.loadFile(arg); return green + "Opened: " + reset + arg;
        }
        if (cmd == "save") { ctx.saveFile(); return green + "File saved" + reset; }
        if (cmd == "saveas") {
            if (arg.empty()) return red + "Usage: saveas <file>" + reset;
            ctx.filename = arg; ctx.saveFile(); return green + "Saved as: " + reset + arg;
        }
        if (cmd == "undo") { ctx.undo(); return "Undo performed"; }
        if (cmd == "redo") { ctx.redo(); return "Redo performed"; }
        if (cmd == "clear") { ctx.lines.clear(); ctx.lines.emplace_back(""); ctx.cx = ctx.cy = 0; ctx.dirty = true; return yellow + "Buffer cleared" + reset; }
        if (cmd == "lines") return blue + "Lines: " + reset + std::to_string(ctx.lines.size());
        if (cmd == "words") {
            size_t wc = 0;
            for (const auto& l : ctx.lines) {
                bool in = false;
                for (char c : l) { if (std::isspace(c)) in = false; else if (!in) { in = true; wc++; } }
            }
            return blue + "Words: " + reset + std::to_string(wc);
        }
        if (cmd == "chars") {
            size_t cc = 0;
            for (const auto& l : ctx.lines) cc += l.size();
            return blue + "Chars: " + reset + std::to_string(cc);
        }
        if (cmd == "cut") {
            if (ctx.cy < (int)ctx.lines.size()) {
                ctx.clipboard = ctx.lines[ctx.cy];
                ctx.lines.erase(ctx.lines.begin() + ctx.cy);
                if (ctx.lines.empty()) ctx.lines.emplace_back("");
                ctx.dirty = true; return yellow + "Line cut" + reset;
            }
            return red + "Nothing to cut" + reset;
        }
        if (cmd == "paste") {
            if (!ctx.clipboard.empty()) {
                ctx.lines.insert(ctx.lines.begin() + ctx.cy + 1, ctx.clipboard);
                ctx.cy++; ctx.dirty = true; return green + "Line pasted" + reset;
            }
            return red + "Clipboard empty" + reset;
        }
        if (cmd == "find") {
            if (arg.empty()) return red + "Usage: find <text>" + reset;
            for (size_t i = 0; i < ctx.lines.size(); ++i) {
                size_t p = ctx.lines[i].find(arg);
                if (p != std::string::npos) { ctx.cy = (int)i; ctx.cx = (int)p; return green + "Found at L" + std::to_string(i + 1) + reset; }
            }
            return red + "Not found" + reset;
        }
        if (cmd == "goto") {
            if (arg.empty()) return red + "Usage: goto <line>" + reset;
            try {
                int t = std::stoi(arg) - 1;
                if (t >= 0 && t < (int)ctx.lines.size()) { ctx.cy = t; return "Jumped to " + arg; }
            } catch (...) {}
            return red + "Out of range" + reset;
        }
        if (cmd == "home") { ctx.cx = ctx.cy = 0; return "Home"; }
        if (cmd == "end") {
            ctx.cy = (int)ctx.lines.size() - 1;
            ctx.cx = (int)ctx.lines[ctx.cy].size();
            return "End";
        }
        if (cmd == "upper" && ctx.cy < (int)ctx.lines.size()) {
            for (char &c : ctx.lines[ctx.cy]) c = std::toupper(c);
            ctx.dirty = true; return "To UPPERCASE";
        }
        if (cmd == "lower" && ctx.cy < (int)ctx.lines.size()) {
            for (char &c : ctx.lines[ctx.cy]) c = std::tolower(c);
            ctx.dirty = true; return "To lowercase";
        }
        if (cmd == "delete") {
            ctx.lines.erase(ctx.lines.begin() + ctx.cy);
            if (ctx.lines.empty()) ctx.lines.emplace_back("");
            if (ctx.cy >= (int)ctx.lines.size()) ctx.cy = ctx.lines.size() - 1;
            ctx.dirty = true; return red + "Line deleted" + reset;
        }
        if (cmd == "rename") {
            if (arg.empty()) return red + "Usage: rename <name>" + reset;
            ctx.filename = arg; return "Renamed to " + arg;
        }
        if (cmd == "reload") {
            if (ctx.filename.empty()) return red + "No file to reload" + reset;
            ctx.loadFile(ctx.filename); return "Reloaded";
        }
        if (cmd == "pwd") { char cwd[1024]; getcwd(cwd, sizeof(cwd)); return blue + "Path: " + reset + cwd; }
        if (cmd == "time") { std::time_t t = std::time(nullptr); char b[16]; std::strftime(b, sizeof(b), "%H:%M:%S", std::localtime(&t)); return blue + "Time: " + reset + b; }
        if (cmd == "date") { std::time_t t = std::time(nullptr); char b[16]; std::strftime(b, sizeof(b), "%Y-%m-%d", std::localtime(&t)); return blue + "Date: " + reset + b; }
        if (cmd == "sys") {
            if (arg.empty()) return red + "Usage: sys <cmd>" + reset;
            std::system(arg.c_str()); return "Executed: " + arg;
        }
        if (cmd == "status") return (ctx.dirty ? yellow + "[MOD] " : green + "[OK] ") + reset + ctx.filename;
        if (cmd == "version") return blue + ts::NAME + " v" + ts::VERSION + reset;
        if (cmd == "credits") return blue + "Author: " + reset + "Viren Sahti";
        if (cmd == "edit") return "Mode: Insert (Standard)";
        
        if (cmd == "dup") {
            if (ctx.cy < (int)ctx.lines.size()) {
                ctx.lines.insert(ctx.lines.begin() + ctx.cy, ctx.lines[ctx.cy]);
                ctx.dirty = true; return "Line duplicated";
            }
            return red + "Error" + reset;
        }
        if (cmd == "rev") {
            if (ctx.cy < (int)ctx.lines.size()) {
                std::reverse(ctx.lines[ctx.cy].begin(), ctx.lines[ctx.cy].end());
                ctx.dirty = true; return "Line reversed";
            }
            return red + "Error" + reset;
        }
        if (cmd == "top") { ctx.cy = 0; ctx.cx = 0; return "Jumped to top"; }
        if (cmd == "bottom") { ctx.cy = (int)ctx.lines.size() - 1; ctx.cx = 0; return "Jumped to bottom"; }
        if (cmd == "trim") {
            for (auto& l : ctx.lines) l.erase(l.find_last_not_of(" \t\n\r\f\v") + 1 == std::string::npos ? 0 : l.find_last_not_of(" \t\n\r\f\v") + 1);
            ctx.dirty = true; return "Trailing spaces removed";
        }
        if (cmd == "join") {
            if (ctx.cy < (int)ctx.lines.size() - 1) {
                ctx.lines[ctx.cy] += ctx.lines[ctx.cy+1];
                ctx.lines.erase(ctx.lines.begin() + ctx.cy + 1);
                ctx.dirty = true; return "Lines joined";
            }
            return red + "No next line" + reset;
        }
        if (cmd == "sort") {
            std::sort(ctx.lines.begin(), ctx.lines.end());
            ctx.dirty = true; return "Buffer sorted alphabetically";
        }
        if (cmd == "cap") {
            if (ctx.cy < (int)ctx.lines.size() && !ctx.lines[ctx.cy].empty()) {
                ctx.lines[ctx.cy][0] = std::toupper(ctx.lines[ctx.cy][0]);
                ctx.dirty = true; return "Line capitalized";
            }
            return red + "Error" + reset;
        }
        if (cmd == "empty") {
            int count = 0;
            for (auto it = ctx.lines.begin(); it != ctx.lines.end(); ) {
                if (it->empty()) { it = ctx.lines.erase(it); count++; } else ++it;
            }
            if (ctx.lines.empty()) ctx.lines.emplace_back("");
            ctx.dirty = true; return std::to_string(count) + " empty lines removed";
        }
        if (cmd == "replace") {
            size_t s = arg.find(' ');
            if (s == std::string::npos) return red + "Usage: replace <old> <new>" + reset;
            std::string oldW = arg.substr(0, s), newW = arg.substr(s + 1);
            int count = 0;
            for (auto& l : ctx.lines) {
                size_t p = 0;
                while ((p = l.find(oldW, p)) != std::string::npos) {
                    l.replace(p, oldW.length(), newW);
                    p += newW.length(); count++;
                }
            }
            ctx.dirty = true; return "Replaced " + std::to_string(count) + " occurrences";
        }

        if (cmd == "theme") {
            if (arg == "white") {
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
                ctx.saveConfig();
                return "Theme set to White";
            } else if (arg == "black") {
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
                ctx.saveConfig();
                return "Theme set to Black";
            }
            return red + "Usage: theme <white|black>" + reset;
        }

        if (cmd == "reset") {
            ctx.resetConfig();
            return "Editor has been reset to factory settings";
        }

        if (cmd == "help") return yellow + "Commands: " + reset + "open, save, saveas, clear, lines, words, chars, undo, redo, cut, paste, find, goto, home, end, upper, lower, time, date, version, credits, status, reload, delete, rename, pwd, sys, edit, dup, rev, top, bottom, trim, join, sort, cap, empty, replace, theme, reset, exit";

        return red + "Unknown command: " + reset + cmd;
    }
}
