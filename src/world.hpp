#include <entt/entt.hpp>

struct lua_State;

namespace World
{
    void Register(lua_State* lua);
    void Init(entt::registry*, lua_State*);
    void Update();
    void Draw();
    void DrawImgui();
}