#pragma once
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <termios.h>
#endif

class Terminal {
public:
    Terminal() noexcept;
    ~Terminal() noexcept;
    void enableRawMode() noexcept;
    void disableRawMode() noexcept;
    int readKey() noexcept;
    void getWindowSize(int &rows, int &cols) noexcept;
    void writeAll(const std::string &s) noexcept;
    void clear() noexcept;
    void hideCursor() noexcept;
    void showCursor() noexcept;
    void moveCursor(int row, int col) noexcept;
    void flush() noexcept;
private:
    bool rawEnabled;
#ifdef _WIN32
    DWORD origInMode;
    DWORD origOutMode;
    bool origModesSaved;
    CONSOLE_CURSOR_INFO origCursorInfo;
    bool origCursorSaved;
#else
    struct termios *origTermiosPtr;
#endif
};
