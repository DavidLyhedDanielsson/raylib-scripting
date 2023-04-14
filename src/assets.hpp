#pragma once

#include <array>
#include <external/raylib.hpp>

// Assets come from https://quaternius.com/. Thanks Quaternius!
enum class Asset : int
{
    Insurgent = 0,
    Bob,
    Last
};

// Windows clocks in at 260, linux allows 4096
constexpr int MAX_PATH_LENGTH = 256;
// This function is required since the file paths are different on the web and locally
std::array<char, MAX_PATH_LENGTH> AssetPath(const char* assetName);

void LoadAssets();
Model GetLoadedAsset(Asset asset);
const char* GetAssetName(Asset asset);
const char* GetAssetPath(Asset asset);