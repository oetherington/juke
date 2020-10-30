#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

bool IsAudioPath(const fs::path &p);
std::vector<std::string> Split(const std::string &str, const std::string &delim);
std::string RemoveExtension(const std::string &s);
