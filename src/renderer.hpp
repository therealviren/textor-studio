#pragma once
#include <string>
#include <vector>
#include <memory>
#include "terminal.hpp"
#include "buffer.hpp"

class Renderer {
public:
    Renderer(Terminal &t) noexcept;
    ~Renderer() noexcept = default;

    void setBuffer(TextBuffer *b) noexcept;
    void resize() noexcept;
    void render(int cx, int cy, int rowOffset, int colOffset, bool dirty, const std::string &statusMsg) noexcept;
    
    int rowsVisible() const noexcept;
    int colsVisible() const noexcept;
    int gutterWidth() const noexcept;

    struct Viewport {
        int rTop;
        int rBottom;
        int cLeft;
        int cRight;
    };

private:
    Terminal &term;
    TextBuffer *buffer;
    int screenRows;
    int screenCols;
    int commandHeight;
    
    void buildGutter(std::string &frame, int lineNum, int width) const noexcept;
    void buildStatusBar(std::string &frame, bool dirty, int totalLines) const noexcept;
    void buildCommandBar(std::string &frame, const std::string &msg) noexcept;
    void applyClipping(std::string &frame, const std::string &line, int offset, int width) const noexcept;
    int calculateRequiredGutter() const noexcept;
    void syncPhysicalCursor(std::string &frame, int cx, int cy, int rOff, int cOff, int gutter) const noexcept;
};
