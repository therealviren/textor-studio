#include "buffer.hpp"
#include "fileio.hpp"
#include <string>
#include <vector>

TextBuffer::TextBuffer() noexcept: rows(), currPath() {}

bool TextBuffer::load(const std::string &path) noexcept {
    if (!loadFileToBuffer(path, rows)) return false;
    currPath = path;
    return true;
}

bool TextBuffer::save(const std::string &path) noexcept {
    if (!saveBufferToFile(path, rows)) return false;
    currPath = path;
    return true;
}

std::vector<std::string> &TextBuffer::lines() noexcept { return rows; }
const std::vector<std::string> &TextBuffer::lines() const noexcept { return rows; }
std::string TextBuffer::path() const noexcept { return currPath; }

void TextBuffer::insertChar(int row, int col, const std::string& s) noexcept {
    if (row < 0) return;
    if (row >= static_cast<int>(rows.size())) {
        rows.resize(static_cast<size_t>(row + 1));
    }
    std::string &r = rows[static_cast<size_t>(row)];
    if (col < 0) col = 0;
    if (col > static_cast<int>(r.size())) col = static_cast<int>(r.size());
    r.insert(static_cast<size_t>(col), s);
}


void TextBuffer::deleteChar(int row, int col) noexcept {
    if (row < 0 || row >= static_cast<int>(rows.size())) return;
    std::string &r = rows[static_cast<size_t>(row)];
    if (col > 0 && col <= static_cast<int>(r.size())) {
        r.erase(static_cast<size_t>(col - 1), 1);
        return;
    }
    if (col == 0 && row > 0) {
        std::string &prev = rows[static_cast<size_t>(row - 1)];
        prev += r;
        rows.erase(rows.begin() + row);
    }
}

void TextBuffer::insertLine(int row, const std::string &s) noexcept {
    if (row < 0) row = 0;
    if (row >= static_cast<int>(rows.size())) rows.push_back(s);
    else rows.insert(rows.begin() + row, s);
}

void TextBuffer::eraseLine(int row) noexcept {
    if (row < 0 || row >= static_cast<int>(rows.size())) return;
    rows.erase(rows.begin() + row);
    if (rows.empty()) rows.emplace_back();
}

void TextBuffer::setLines(const std::vector<std::string> &r) noexcept { rows = r; }
