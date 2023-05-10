#include <entt/entt.hpp>

struct lua_State;

namespace World
{
    void Register(lua_State* lua);
    void Init(entt::registry*);
    void Update(lua_State* lua);
    void Draw();
    void DrawImgui();
}