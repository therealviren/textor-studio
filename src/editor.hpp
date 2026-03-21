#pragma once
#include <string>
#include <vector>
#include <csignal>
#include "terminal.hpp"
#include "buffer.hpp"
#include "renderer.hpp"

class Editor {
public:
    Editor() noexcept;
    ~Editor() noexcept;
    void open(const std::string &path) noexcept;
    void run();
    void render() noexcept;
    void safeExit() noexcept;

private:
    Terminal term;
    TextBuffer buffer;
    Renderer renderer;
    std::string filename;
    std::string clipboard;
    int cx;
    int cy;
    int rowOffset;
    int colOffset;
    bool dirty;
    std::string statusMessage;
    std::vector<std::vector<std::string>> undoStack;
    std::vector<std::vector<std::string>> redoStack;

    void pushSnapshot() noexcept;
    void undo() noexcept;
    void redo() noexcept;

    void validateCursorBounds() noexcept;
    void ensureCursorVisible() noexcept;

    void handleInput(int c) noexcept;
    void saveFileAction() noexcept;
    void commandBarAction() noexcept;
    void insertCharAction(const std::string& s) noexcept;
    void insertNewlineAction() noexcept;
    void backspaceAction() noexcept;

    std::string prompt(const std::string &msg) noexcept;
    void loadOrCreate(const std::string &path) noexcept;

    void loadConfig() noexcept;
    void saveConfig() noexcept;
    void resetConfig() noexcept;
    void performUpdate() noexcept;

    static volatile sig_atomic_t stopRequested;
    static void onSignal(int signal) noexcept;
};
