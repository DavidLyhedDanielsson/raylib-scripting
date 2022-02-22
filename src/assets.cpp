#include "assets.hpp"
#include <string>

struct AssetPair
{
    const char* name;
    const char* path;
};

// Assets come from https://quaternius.com/. Thanks Quaternius!
std::array<AssetPair, 2> assets = {
    AssetPair{"Insurgent", "Insurgent/glTF/Insurgent.gltf"},
    AssetPair{"Bob", "Bob/glTF/Bob.gltf"},
};
std::array<Model, std::size(assets)> loadedAssets = {};

// Windows clocks in at 260, linux allows 4096
std::array<char, MAX_PATH_LENGTH> pathBuffer = {0}; // Helper to avoid dynamic allocation
// This function is required since the file paths are different on the web and
// locally
std::array<char, MAX_PATH_LENGTH> AssetPath(const char* assetName)
{
    snprintf(pathBuffer.data(), MAX_PATH_LENGTH, "%s/%s", DASSET_ROOT, assetName);
    return pathBuffer;
}

void LoadAssets()
{
    for(int i = 0; i < (int)Asset::Last; ++i)
    {
        auto path = AssetPath(assets[i].path);
        loadedAssets[i] = LoadModel(path.data());
    }
}

Model GetLoadedAsset(Asset asset)
{
    return loadedAssets.at((int)asset);
}

const char* GetAssetName(Asset asset)
{
    return assets.at((int)asset).name;
}

const char* GetAssetPath(Asset asset)
{
    return assets.at((int)asset).path;
}