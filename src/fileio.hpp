#pragma once
#include <string>
#include <vector>

bool loadFileToBuffer(const std::string &path, std::vector<std::string> &rows) noexcept;
bool saveBufferToFile(const std::string &path, const std::vector<std::string> &rows) noexcept;