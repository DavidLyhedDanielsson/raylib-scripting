#pragma once

#include <array>
#include <external/raylib.hpp>
#include <map>
#include <string>

// Assets come from https://quaternius.com/. Thanks Quaternius!
extern std::map<std::string, Model> loadedAssets;

// Windows clocks in at 260, linux allows 4096
constexpr int MAX_PATH_LENGTH = 256;
// This function is required since the file paths are different on the web and locally
std::array<char, MAX_PATH_LENGTH> AssetPath(const char* assetName);

void LoadAssets();