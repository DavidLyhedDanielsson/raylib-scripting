#include "assets.hpp"
#include <string>

#include <config.h>

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
        auto model = LoadModel(path.data());
        for(int i = 0; i < model.materialCount; ++i)
        {
            for(int j = 0; j < MAX_MATERIAL_MAPS; ++j)
            {
                if(model.materials[i].maps[j].texture.id != 0)
                {
                    SetTextureFilter(
                        model.materials[i].maps[j].texture,
                        TEXTURE_FILTER_ANISOTROPIC_16X);
                    GenTextureMipmaps(&model.materials[i].maps[j].texture);
                }
            }
        }
        loadedAssets[i] = model;
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