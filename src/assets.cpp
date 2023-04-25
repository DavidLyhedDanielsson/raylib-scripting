#include "assets.hpp"
#include <filesystem>
#include <iostream>
#include <optional>

#include <config.h>

// Assets come from https://quaternius.com/. Thanks Quaternius!
std::map<std::string, Model> loadedAssets = {};

// Windows clocks in at 260, linux allows 4096
std::array<char, MAX_PATH_LENGTH> pathBuffer = {0}; // Helper to avoid dynamic allocation
// This function is required since the file paths are different on the web and
// locally
std::array<char, MAX_PATH_LENGTH> AssetPath(const char* assetName)
{
    snprintf(pathBuffer.data(), MAX_PATH_LENGTH, "%s/%s", DASSET_ROOT, assetName);
    return pathBuffer;
}

std::optional<Shader> transparentShader;

void LoadAssets()
{
    // There's apparently no way to find out if a mesh has transparency, so
    // just treat all meshes as if they are transparent...
    if(!transparentShader.has_value())
        transparentShader = LoadShader(nullptr, AssetPath("transparent.fs").data());

    for(const auto& entry :
        std::filesystem::directory_iterator(std::filesystem::path(DASSET_ROOT) / "ruins"))
    {
        if(!entry.is_regular_file())
            continue;

        if(entry.path().extension() != ".obj")
            continue;

        auto model = LoadModel(entry.path().string().c_str());
        for(int i = 0; i < model.materialCount; ++i)
        {
            model.materials[i].shader = transparentShader.value();

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

        loadedAssets.insert(std::make_pair(entry.path().stem().string(), model));
    }
}
