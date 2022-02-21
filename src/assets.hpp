// Windows clocks in at 260, linux allows 4096
constexpr int MAX_PATH_LENGTH = 256;
std::array<char, MAX_PATH_LENGTH> pathBuffer = {0}; // Helper to avoid dynamic allocation
// This function is required since the file paths are different on the web and locally
std::array<char, MAX_PATH_LENGTH> AssetPath(const char *assetName)
{
    snprintf(pathBuffer.data(), MAX_PATH_LENGTH, "%s/%s", DASSET_ROOT, assetName);
    return pathBuffer;
}