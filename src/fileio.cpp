#include "fileio.hpp"
#include <fstream>
#include <string>
#include <vector>

bool loadFileToBuffer(const std::string &path, std::vector<std::string> &rows) noexcept {
    rows.clear();
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    std::string line;
    while (std::getline(ifs, line)) rows.push_back(line);
    if (rows.empty()) rows.emplace_back();
    return !ifs.bad();
}

bool saveBufferToFile(const std::string &path, const std::vector<std::string> &rows) noexcept {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    for (size_t i = 0; i < rows.size(); ++i) {
        ofs << rows[i];
        if (i + 1 < rows.size()) ofs << '\n';
    }
    return ofs.good();
}
