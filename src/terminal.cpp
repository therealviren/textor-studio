#include "terminal.hpp"
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif

Terminal::Terminal() noexcept: rawEnabled(false)
#ifdef _WIN32
, origInMode(0), origOutMode(0), origModesSaved(false), origCursorSaved(false)
#else
, origTermiosPtr(nullptr)
#endif
{}

Terminal::~Terminal() noexcept { disableRawMode(); }

void Terminal::enableRawMode() noexcept {
    if (rawEnabled) return;
#ifdef _WIN32
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hin == INVALID_HANDLE_VALUE || hout == INVALID_HANDLE_VALUE) return;
    GetConsoleMode(hin, &origInMode);
    GetConsoleMode(hout, &origOutMode);
    origModesSaved = true;
    DWORD newInMode = origInMode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleMode(hin, newInMode);
    DWORD newOutMode = origOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hout, newOutMode);
#else
    struct termios *orig = new termios;
    if (tcgetattr(STDIN_FILENO, orig) == -1) { delete orig; return; }
    struct termios raw = *orig;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    origTermiosPtr = orig;
#endif
    rawEnabled = true;
}

void Terminal::disableRawMode() noexcept {
    if (!rawEnabled) return;
#ifdef _WIN32
    if (origModesSaved) {
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), origInMode);
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), origOutMode);
    }
#else
    if (origTermiosPtr) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, (struct termios*)origTermiosPtr);
        delete (struct termios*)origTermiosPtr;
        origTermiosPtr = nullptr;
    }
#endif
    rawEnabled = false;
}

int Terminal::readKey() noexcept {
#ifdef _WIN32
    return -1;
#else
    unsigned char c;
    int nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) return -1;
        if (nread == 0) return -2;
    }

    if (c == '\x1b') {
        char seq[6];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == ';') {
                    if (read(STDIN_FILENO, &seq[3], 1) != 1) return '\x1b';
                    if (read(STDIN_FILENO, &seq[4], 1) != 1) return '\x1b';

                    if (seq[3] == '5') {
                        switch (seq[4]) {
                            case 'A': return 1004;
                            case 'B': return 1005;
                            case 'C': return 1006;
                            case 'D': return 1007;
                        }
                    }
                } else if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return 1010;
                        case '3': return 1011;
                        case '4': return 1012;
                        case '5': return 1013;
                        case '6': return 1014;
                        case '7': return 1010;
                        case '8': return 1012;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return 1000;
                    case 'B': return 1001;
                    case 'C': return 1003;
                    case 'D': return 1002;
                    case 'H': return 1010;
                    case 'F': return 1012;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return 1010;
                case 'F': return 1012;
            }
        }
        return '\x1b';
    }
    return (int)c;
#endif
}

void Terminal::getWindowSize(int &rows, int &cols) noexcept {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        cols = 80; rows = 24;
    } else {
        cols = ws.ws_col; rows = ws.ws_row;
    }
#endif
}

void Terminal::writeAll(const std::string &s) noexcept {
    write(STDOUT_FILENO, s.c_str(), s.size());
}

void Terminal::clear() noexcept {
    writeAll("\x1b[2J\x1b[H");
}

void Terminal::hideCursor() noexcept {
    writeAll("\x1b[?25l");
}

void Terminal::showCursor() noexcept {
    writeAll("\x1b[?25h");
}
