#include "assets.hpp"
#include <cmath>
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

std::optional<Shader> shader;

void LoadAssets()
{
    // Shader with very basic lighting
    if(!shader.has_value())
#if defined(GRAPHICS_API_OPENGL_ES2)
        shader = LoadShader(
            AssetPath("shaders/lit_es2.vs").data(),
            AssetPath("shaders/lit_es2.fs").data());
#elif defined(GRAPHICS_API_OPENGL_21)
        shader = LoadShader(
            AssetPath("shaders/lit_21.vs").data(),
            AssetPath("shaders/lit_21.fs").data());
#elif defined(GRAPHICS_API_OPENGL_33)
        shader = LoadShader(
            AssetPath("shaders/lit_33.vs").data(),
            AssetPath("shaders/lit_33.fs").data());
#else
    #error Aaaaaaaaah
#endif

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
            model.materials[i].shader = shader.value();

            for(int j = 0; j < MAX_MATERIAL_MAPS; ++j)
            {
#define TransformColor(component)                \
    model.materials[i].maps[j].color.component = \
        std::pow(model.materials[i].maps[j].color.component / 255.0f, 1.0f / 2.2f) * 255.0f;

                TransformColor(r);
                TransformColor(g);
                TransformColor(b);
                TransformColor(a);

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
