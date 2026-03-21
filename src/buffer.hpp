#pragma once
#include <string>
#include <vector>

class TextBuffer {
public:
    TextBuffer() noexcept;
    bool load(const std::string &path) noexcept;
    bool save(const std::string &path) noexcept;
    std::vector<std::string> &lines() noexcept;
    const std::vector<std::string> &lines() const noexcept;
    std::string path() const noexcept;
    void insertChar(int row, int col, const std::string& s) noexcept;
    void deleteChar(int row, int col) noexcept;
    void insertLine(int row, const std::string &s) noexcept;
    void eraseLine(int row) noexcept;
    void setLines(const std::vector<std::string> &r) noexcept;
private:
    std::vector<std::string> rows;
    std::string currPath;
};
