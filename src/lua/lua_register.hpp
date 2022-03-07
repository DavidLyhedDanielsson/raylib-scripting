#pragma once

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <array>
#include <tuple>
#include <type_traits>

namespace LuaRegister
{
    // Maximum number of variadic arguments in a lua function
    constexpr int MAX_VARIADIC_ARG_COUNT = 32;
    // Length of statically allocate dstring buffer used to collect variadic
    // string arguments
    constexpr int STRING_BUFFER_LENGTH = 256;

    // Used when calling `Register` to indicate that all parameters including
    // and after this parameter are variadic
    template<typename T>
    struct Variadic
    {
        using value_type = T;
        std::array<T, MAX_VARIADIC_ARG_COUNT> arr;
        int count;

        // strcpy_s isn't a requirement in the standard and sprintf has at least
        // some overhead. Fingers crossed that the compiler will optimize this
        // to at least sprintf performance
        inline std::array<char, STRING_BUFFER_LENGTH> strcpy()
        {
            std::array<char, STRING_BUFFER_LENGTH> buffer;
            int writtenCharacters = 0;
            for(int i = 0; i < count; ++i)
            {
                for(int j = 0; writtenCharacters < STRING_BUFFER_LENGTH - 1;
                    ++writtenCharacters, ++j)
                {
                    if(arr[i][j] == '\0')
                        break;
                    buffer[writtenCharacters] = arr[i][j];
                }

                if(writtenCharacters == STRING_BUFFER_LENGTH - 1)
                    break;
            }
            buffer[writtenCharacters] = '\0';
            return buffer;
        }
    };

    // Used when calling 'Register' to indicate that this parameter should be
    // ignored. It will instead contain the stack index of this parameter
    struct Placeholder
    {
        lua_Integer stackIndex;
    };

    // Disable formatting so this section can be made short and pretty
    // clang-format off
    template<typename> struct is_variadic: std::false_type {};
    template<typename T> struct is_variadic<Variadic<T>>: std::true_type {};
    template<typename T, typename... Other> struct is_any: std::bool_constant<(std::is_same_v<T, Other> || ...)> {};
    template<typename T, typename... Other> inline constexpr bool is_any_v = is_any<T, Other...>::value;
    // Does not actually check for a T const*, but for a const T*
    template<typename T> bool constexpr is_const_pointer_v = std::is_const_v<std::remove_pointer_t<T>>;

    // https://stackoverflow.com/questions/53945490/how-to-assert-that-a-constexpr-if-else-clause-never-happen
    template<class...>
    constexpr std::false_type always_false{};

    // Because some lua_toX are macros it cannot be used in the LuaGetFunc
    // specialization
    inline lua_Integer luaToInteger(lua_State* lua, int i) { return lua_tointeger(lua, i); }
    inline lua_Number luaToNumber(lua_State* lua, int i) { return lua_tonumber(lua, i); }
    inline int luaToBoolean(lua_State* lua, int i) { return lua_toboolean(lua, i); }
    inline const char* luaToString(lua_State* lua, int i) { return lua_tostring(lua, i); }

    // To be able to pass arguments of type T from lua to C++, this function and
    // GetDefault must be specified for the type T
    template<typename T>
    constexpr auto LuaGetFunc = nullptr;
    template<> inline constexpr auto LuaGetFunc<int> = luaToInteger;
    template<> inline constexpr auto LuaGetFunc<unsigned int> = luaToInteger;
    template<> inline constexpr auto LuaGetFunc<long> = luaToInteger;
    template<> inline constexpr auto LuaGetFunc<unsigned long> = luaToInteger;
    template<> inline constexpr auto LuaGetFunc<long long> = luaToInteger;
    template<> inline constexpr auto LuaGetFunc<unsigned long long> = luaToInteger;
    template<> inline constexpr auto LuaGetFunc<float> = luaToNumber;
    template<> inline constexpr auto LuaGetFunc<double> = luaToNumber;
    template<> inline constexpr auto LuaGetFunc<bool> = luaToBoolean;
    template<> inline constexpr auto LuaGetFunc<const char*> = luaToString;

    template<typename T>
    constexpr auto GetDefault = 0;
    template<> inline constexpr auto GetDefault<int> = 0;
    template<> inline constexpr auto GetDefault<unsigned int> = 0;
    template<> inline constexpr auto GetDefault<long> = 0;
    template<> inline constexpr auto GetDefault<unsigned long> = 0;
    template<> inline constexpr auto GetDefault<long long> = 0;
    template<> inline constexpr auto GetDefault<unsigned long long> = 0;
    template<> inline constexpr auto GetDefault<float> = 0.0f;
    template<> inline constexpr auto GetDefault<double> = 0.0;
    template<> inline constexpr auto GetDefault<bool> = false;
    template<> inline constexpr auto GetDefault<const char*> = nullptr;

    // To be able to return arguments of type T from C++ to lua, this function
    // must be specified for the type T
    template<typename T>
    constexpr auto LuaSetFunc = nullptr;
    template<> inline constexpr auto LuaSetFunc<int> = lua_pushinteger;
    template<> inline constexpr auto LuaSetFunc<unsigned int> = lua_pushinteger;
    template<> inline constexpr auto LuaSetFunc<long> = lua_pushinteger;
    template<> inline constexpr auto LuaSetFunc<unsigned long> = lua_pushinteger;
    template<> inline constexpr auto LuaSetFunc<long long> = lua_pushinteger;
    template<> inline constexpr auto LuaSetFunc<unsigned long long> = lua_pushinteger;
    template<> inline constexpr auto LuaSetFunc<float> = lua_pushnumber;
    template<> inline constexpr auto LuaSetFunc<double> = lua_pushnumber;
    template<> inline constexpr auto LuaSetFunc<bool> = lua_pushboolean;
    template<> inline constexpr auto LuaSetFunc<const char*> = lua_pushstring;
    // clang-format on

    /**
     * @brief "Converts" the object at lua stack index \p stackIndex to a C++
     * type.
     *
     * The type of the return value follows:
     * - Any primitive type is converted to type T.
     * - A <tt>const char*</tt> will return lua_tostring <b>without taking
     *   ownership of the string</b>.
     * - A pointer will return a <tt>std::array<T></tt>. If lua_istable return
     *   true, then \p lua_len is used to determine how many times \p LuaGetFunc
     *   will be called, otherwise only the first index will be set. The size of
     *   the array is always 4 .
     * - A \p Placeholder will return a \p Placeholder and nothing will be
     *   parsed. @see Placeholder.
     * - A \p lua_State* will simply return the \p lua parameter and nothing
     *   will be parsed.
     * - A \p Variadic will convert parameters from \p stackIndex until
     *   \p lua_gettop is reached and return a \p Variadic<T>.
     *
     * @see LuaGetFunc
     * @see GetDefault
     * @see LuaSetFunc
     *
     * @tparam T The type to convert to. The type must have a LuaGetFunc and
     * GetDefault specialization
     * @param lua
     * @param stackIndex
     * @return auto See full description
     */
    template<typename T>
    auto GetParameter(lua_State* lua, int& stackIndex)
    {
        static_assert(!std::is_reference_v<T>);

        if constexpr(is_variadic<T>::value)
        {
            auto var = Variadic<typename T::value_type>();
            // Eat the rest of the arguments. Nom nom nom
            var.count = std::min(lua_gettop(lua) - stackIndex + 1, MAX_VARIADIC_ARG_COUNT);
            for(int j = 0; stackIndex <= lua_gettop(lua); ++stackIndex, ++j)
                var.arr.at(j) = LuaGetFunc<typename T::value_type>(lua, stackIndex);
            return var;
        }
        else if constexpr(std::is_same_v<T, lua_State*>)
        {
            return lua;
        }
        else if constexpr(std::is_same_v<T, Placeholder>)
        {
            return Placeholder{.stackIndex = stackIndex++};
        }
        else if constexpr(std::is_pointer_v<T> && !std::is_same_v<T, const char*>)
        {
            using NakedT = std::remove_const_t<std::remove_pointer_t<T>>;
            auto arr = std::array<NakedT, 4>{GetDefault<NakedT>};
            if(stackIndex > lua_gettop(lua))
                return arr;

            if(lua_istable(lua, stackIndex))
            {
                int count = luaL_len(lua, stackIndex);
                if(count > 4)
                {
                    // Error message at some point
                    return arr;
                }

                for(int j = 1; j <= count; j++)
                {
                    lua_geti(lua, stackIndex, j);
                    arr[j - 1] = LuaGetFunc<NakedT>(lua, -1);
                    lua_pop(lua, 1);
                }
            }
            else
                arr[0] = LuaGetFunc<NakedT>(lua, stackIndex);

            stackIndex++;

            return arr;
        }
        else
        {
            // If there is some error here about LuaGetFunc it is because it isn't
            // specialized for type T
            return stackIndex <= lua_gettop(lua) ? (T)LuaGetFunc<T>(lua, stackIndex++)
                                                 : GetDefault<T>;
        }
    }

    /**
     * @brief Given a lua state with X parameters on the stack, convert
     * parameters to the types of \p Arg and \p Args
     *
     * @tparam Arg
     * @tparam Args
     * @param state
     * @param stackIndex
     * @return std::tuple<Arg, Args...> @see GetParameter for possible types of
     * \p Arg and \p Args
     */
    template<typename Arg, typename... Args>
    auto GetParameters(lua_State* state, int& stackIndex)
    {
        if constexpr(sizeof...(Args) > 0)
        {
            // `current` must be set first to preserve the order of fetching from
            // the stack. Argument order of execution is undefined, so local
            // variables are required
            auto current = std::make_tuple(GetParameter<Arg>(state, stackIndex));
            auto rest = GetParameters<Args...>(state, stackIndex);
            return std::tuple_cat(current, rest);
        }
        else
        {
            return std::make_tuple(GetParameter<Arg>(state, stackIndex));
        }
    }

    template<size_t Index, typename... Types, typename... TupTypes>
    requires(Index == sizeof...(Types)) int ReturnVals(lua_State*, const std::tuple<TupTypes...>&)
    {
        return 0;
    }

    /**
     * @brief Given a lua stack index, a paramter pack of types, and a tuple,
     * returns values from \p vals that should be returned to lua based on the
     * types of \p Types
     *
     * For instance:
     * \code{.cpp}
     * auto vals = std::make_tuple(1, 2, "Hello", Vector2{ 1.0f, 2.0f }, 3);
     * ReturnVals<int, int*, const char*, Vector2, const int*>(lua, vals);
     * \endcode
     *
     * Will push the int \p 2, the string \p Hello and type \p Vector2
     *
     * Note that \p 3 is not pushed since it is a const int*, and should not be
     * returned to lua since it will not be modified by C++
     *
     * @tparam StackIndex Current stack index. Since this is a recursive
     * function it will probably only be called with the value 0
     * @tparam Types
     * @tparam TupTypes
     * @param vals
     */
    template<size_t StackIndex, typename... Types, typename... TupTypes>
    requires(StackIndex < sizeof...(Types)) int ReturnVals(
        lua_State* lua,
        const std::tuple<TupTypes...>& vals)
    {
        using T = std::tuple_element_t<StackIndex, std::tuple<Types...>>;
        if constexpr(!std::is_pointer_v<T> || is_const_pointer_v<T>)
        {
            return 0 + ReturnVals<StackIndex + 1, Types...>(lua, vals);
        }
        else
        {
            using U = std::remove_pointer_t<std::remove_reference_t<T>>;
            int retLength = 0;
            if constexpr(std::is_same_v<U, float>)
            {
                if(lua_istable(lua, StackIndex + 1))
                {
                    int len = luaL_len(lua, StackIndex + 1);
                    lua_createtable(lua, len, 0);
                    for(int i = 0; i < len; ++i)
                    {
                        lua_pushnumber(lua, i + 1);
                        auto val = std::get<StackIndex>(vals).at(i);
                        lua_pushnumber(lua, val);
                        lua_settable(lua, -3);
                    }
                }
                else
                {
                    lua_pushnumber(lua, std::get<StackIndex>(vals).at(0));
                }
                retLength = 1;
            }
            else if constexpr(std::is_same_v<U, bool>)
            {
                if(lua_istable(lua, StackIndex + 1))
                {
                    int len = luaL_len(lua, StackIndex + 1);
                    lua_createtable(lua, len, 0);
                    for(int i = 0; i < len; ++i)
                    {
                        lua_pushnumber(lua, i + 1);
                        auto val = std::get<StackIndex>(vals).at(i);
                        lua_pushboolean(lua, val);
                        lua_settable(lua, -3);
                    }
                }
                else
                {
                    lua_pushboolean(lua, std::get<StackIndex>(vals).at(0));
                }
                retLength = 1;
            }
            else if constexpr(
                is_any_v<U, int, unsigned int, long, unsigned long, long long, unsigned long long>)
            {
                if(lua_istable(lua, StackIndex + 1))
                {
                    int len = luaL_len(lua, StackIndex + 1);
                    lua_createtable(lua, len, 0);
                    for(int i = 0; i < len; ++i)
                    {
                        lua_pushnumber(lua, i + 1);
                        auto val = std::get<StackIndex>(vals).at(i);
                        lua_pushinteger(lua, val);
                        lua_settable(lua, -3);
                    }
                }
                else
                {
                    lua_pushinteger(lua, std::get<StackIndex>(vals).at(0));
                }
                retLength = 1;
            }
            // Do nothing if T is of type lua_State* or Placeholder
            else if constexpr(!is_any_v<T, lua_State*, Placeholder>)
                static_assert(always_false<T>);

            return retLength + ReturnVals<StackIndex + 1, Types...>(lua, vals);
        }
    }

    /**
     * @brief Given a tuple that owns all of its values i.e. containing no
     * pointers or references, returns a tuple which can contain pointers to the
     * elements of \p tup depending on \p TargetTypes
     *
     * For instance:
     * \code {.cpp}
     * auto vals = std::make_tuple(123, "Hello", 1.0f, 3);
     * auto converted = Convert<int*, const char* float, int>(vals);
     * \endcode
     * Will create a tuple where index 0 points to vals[0] and index 1 points to
     * vals[1]. Index 2 and 3 will be copied from vals
     *
     * @tparam StackIndex Current stack index. Since this is a recursive
     * function it will probably only be called with the value 0
     * @tparam TargetTypes
     * @tparam TupTypes
     * @param tup
     * @return auto
     */
    template<size_t StackIndex, typename... TargetTypes, typename... TupTypes>
    auto Convert(const std::tuple<TupTypes...>& tup)
    {
        using T = std::tuple_element_t<StackIndex, std::tuple<TargetTypes...>>;
        if constexpr(std::is_same_v<T, lua_State*>)
        {
            auto lhs = std::make_tuple(std::get<StackIndex>(tup));
            auto rhs = Convert<StackIndex + 1, TargetTypes...>(tup);
            return std::tuple_cat(lhs, rhs);
        }
        if constexpr(std::is_pointer_v<T> && !std::is_same_v<const char*, T>)
        {
            auto lhs = std::make_tuple((T)&std::get<StackIndex>(tup));
            if constexpr(StackIndex + 1 == sizeof...(TargetTypes))
            {
                return lhs;
            }
            else
            {
                auto rhs = Convert<StackIndex + 1, TargetTypes...>(tup);
                return std::tuple_cat(lhs, rhs);
            }
        }
        else
        {
            auto lhs = std::make_tuple(std::get<StackIndex>(tup));
            if constexpr(StackIndex + 1 == sizeof...(TargetTypes))
            {
                return lhs;
            }
            else
            {
                auto rhs = Convert<StackIndex + 1, TargetTypes...>(tup);
                return std::tuple_cat(lhs, rhs);
            }
        }
    }

    /**
     * @brief Specialization of LuaWrapper for a function which takes no
     * parameters
     *
     * @see LuaWrapper for detailed comments
     * @see Register for the function that calls this function
     */
    template<typename R>
    int LuaWrapper(lua_State* lua)
    {
        int retCount = 0;
        auto f = (R(*)(void))lua_touserdata(lua, lua_upvalueindex(1));
        if constexpr(!std::is_same_v<R, void>)
        {
            R res = f();
            LuaSetFunc<R>(lua, res);
            retCount++;
        }
        else
        {
            f();
        }

        return retCount;
    }

    /**
     * @brief A lua function which takes a function pointer R (*)(Args) as an
     * upvalue
     *
     * Using \p R and \p Args it is possible to fetch the correct values from
     * the lua stack and call the function pointer stored in upvalue index 1
     *
     * @see Register for the function that calls this function
     *
     * @tparam R
     * @tparam Args
     */
    template<typename R, typename... Args>
    requires(sizeof...(Args) > 0) int LuaWrapper(lua_State* lua)
    {
        // `arg` needs to be a local variable so it can be passed to GetParameters -
        // which takes a reference
        // Keeps track of which stack index to fetch the next parameter from
        int arg = 1;
        // If a function takes a constant reference one of the types in Args
        // will be a const reference, but the tuple must own the value so the
        // parameter can be passed as a reference
        auto tup = GetParameters<std::remove_cvref_t<Args>...>(lua, arg);
        // std::apply will not automatically convert owned types to pointers, so
        // a function that takes a float* must be called with a
        // `std::tuple<float*>` and not `std::tuple<float>`. Convert performs
        // this conversion. Any pointers in `tup2` (name will be changed) will
        // point to members in `tup`, so `tup` needs to stick around
        auto tup2 = Convert<0, Args...>(tup);

        int retCount = 0;
        // Call supplied function, since the types R and Args are known the cast
        // is safe.
        // Here it is assumed that the function will return something, which is
        // true for most ImGui functions, but not all. This is a WIP though
        auto f = (R(*)(Args...))lua_touserdata(lua, lua_upvalueindex(1));
        if constexpr(!std::is_same_v<R, void>)
        {
            R res = std::apply(f, tup2);
            LuaSetFunc<R>(lua, res);
            retCount++;
        }
        else
        {
            std::apply(f, tup2);
        }

        retCount += ReturnVals<0, Args...>(lua, tup);

        // It is also assumed only one return value is required, which also
        // doesn't really hold up since some functions use pointers to "return"
        // values
        return retCount;
    }

    /**
     * @brief Register any function as a lua function where the lua parameters
     * should be of types <tt>Args</tt>. All parameters will be taken from the
     * lua stack and \p f will be called if all parameters could be successfully
     * fetched
     *
     * C++ lambdas can be registered as such (note the unary plus +):
     * \code {.cpp}
     * Register(lua, "SomeFunc", +[]() { });
     * \endcode
     *
     * C++ functions with overloads need to be cast to their correct overload as
     * such:
     * \code {.cpp}
     * void SomeFunc(int a);
     * int SomeFunc(int a, float b);
     *
     * Register(lua, "SomeFunc", static_cast<void (*)(int)>(SomeFunc));
     * Register(lua, "SomeFuncAdvanced", static_cast<int (*)(int, float)>(SomeFunc));
     * \endcode
     *
     * @tparam R return type of the function to register. Should be able to be
     * auto-deduced
     * @tparam Args parameter types of the function to register. Should be able to be auto-deduced
     * @param lua
     * @param name name to register in lua
     * @param f
     */
    template<typename R, typename... Args>
    void Register(lua_State* lua, const char* name, R (*f)(Args...))
    {
        // R and Args are known, so upvalues are a perfect place to store the
        // function pointer
        lua_pushlightuserdata(lua, (void*)f);
        lua_pushcclosure(lua, LuaWrapper<R, Args...>, 1);
        lua_setglobal(lua, name);
    }

    /**
     * @brief Specialization of LuaWrapperMember for a function which takes no
     * parameters.
     *
     * @see LuaWrapper for detailed comments
     * @see RegisterMember for the function that calls this function
     */
    template<typename T, typename R>
    int LuaWrapperMember(lua_State* lua)
    {
        int retCount = 0;
        auto f = (R(*)(T))lua_touserdata(lua, lua_upvalueindex(1));
        T instance = (T)lua_touserdata(lua, lua_upvalueindex(2));
        if constexpr(!std::is_same_v<R, void>)
        {
            R res = f(instance);
            LuaSetFunc<R>(lua, res);
            retCount++;
        }
        else
        {
            f(instance);
        }

        return retCount;
    }

    /**
     * @brief Same as LuaWrapper but with an additional upvalue of type \p T
     *
     * @see LuaWrapper for detailed comments
     * @see RegisterMember for the function that calls this function
     */
    template<typename T, typename R, typename... Args>
    requires(sizeof...(Args) > 0) int LuaWrapperMember(lua_State* lua)
    {
        int arg = 1;
        auto tup = GetParameters<std::remove_cvref_t<Args>...>(lua, arg);
        auto tup2 = Convert<0, Args...>(tup);

        int retCount = 0;
        auto f = (R(*)(T, Args...))lua_touserdata(lua, lua_upvalueindex(1));
        T instance = (T)lua_touserdata(lua, lua_upvalueindex(2));
        auto tup3 = std::tuple_cat(std::make_tuple(instance), tup2);
        if constexpr(!std::is_same_v<R, void>)
        {
            R res = std::apply(f, tup3);
            LuaSetFunc<R>(lua, res);
            retCount++;
        }
        else
        {
            std::apply(f, tup3);
        }

        retCount += ReturnVals<0, Args...>(lua, tup);
        return retCount;
    }

    /**
     * @brief Same as Register but with an additional upvalue of type T. Can be
     * used to register functions that need some sort of context
     *
     * @see Register for more detailed comments
     *
     * @tparam T
     * @tparam R
     * @tparam Args
     * @param lua
     * @param name
     * @param instance
     * @param f
     */
    template<typename T, typename R, typename... Args>
    void RegisterMember(lua_State* lua, const char* name, T instance, R (*f)(T, Args...))
    {
        lua_pushlightuserdata(lua, (void*)f);
        lua_pushlightuserdata(lua, (void*)instance);
        lua_pushcclosure(lua, LuaWrapperMember<T, R, Args...>, 2);
        lua_setglobal(lua, name);
    }
}
