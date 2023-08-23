#include <entt/entt.hpp>
#include <navigation.hpp>
#include <optional>

struct lua_State;

namespace World
{
    // Instead of having global variables floating around, placing them in a struct makes it easy to
    // recreate/reinitialise the variables. There is no risk of forgetting a loose global variable
    // if they're all wrapped in some instance.
    struct WorldState
    {
        entt::registry* registry = nullptr;
        lua_State* lua = nullptr;
        int behaviourTable = -1;

        bool drawNavigationTiles = false;
        std::optional<int32_t> drawNavigationField;
        Navigation navigation;
    };
    // This is global because lua needs access to the variables inside it (see lua_world_impl). A
    // global variable can be directly accessed from C functions and lambdas (in other words,
    // exactly what we register with lua). While it is possible to send instances to lua with
    // upvalues and such, this is easier :). Global variables are easier, who would have thought it?
    // This is the only one in the project though. It could be hidden behind some singleton but I
    // see no reason for that.
    extern WorldState state;

    void Init();
    void Update();
    void Draw();
}