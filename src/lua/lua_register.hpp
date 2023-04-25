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

    /**
     * @brief Used when calling `Register` to indicate that all parameters
     * including, and after, this parameter are variadic
     *
     * @tparam T
     */
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

    /**
     * @brief Used when calling 'Register' to indicate that this parameter
     * should be ignored; it will instead contain the stack index of this
     * parameter
     *
     */
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
    // Does not actually check for a `T const*`, but for a `const T*`
    template<typename T> bool constexpr is_const_pointer_v = std::is_const_v<std::remove_pointer_t<T>>;

    // https://stackoverflow.com/questions/53945490/how-to-assert-that-a-constexpr-if-else-clause-never-happen
    template<class...>
    constexpr std::false_type always_false{};

    // Because some lua_toX are macros, they cannot be used in the LuaGetFunc
    // specialization. These are wrappers for the macros, but also for functions
    // like `to_toboolean` for consistency
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
    template<> inline constexpr auto LuaSetFunc<float> = lua_pushnumber;
    template<> inline constexpr auto LuaSetFunc<double> = lua_pushnumber;
    template<> inline constexpr auto LuaSetFunc<bool> = lua_pushboolean;
    template<> inline constexpr auto LuaSetFunc<const char*> = lua_pushstring;
    template<> inline constexpr auto LuaSetFunc<Placeholder> = [](auto, auto){};
    // clang-format on

    /**
     * @brief Get the parameter at lua stack index \p stackIndex and treat it as
     * the given type.
     *
     * The type of the return value follows:
     * - Any primitive type is converted to type T.
     * - A <tt>const char*</tt> will return lua_tostring <b>without taking
     *   ownership of the string</b>.
     * - A pointer will return a <tt>std::array<T></tt>. If lua_istable returns
     *   true, then \p lua_len is used to determine how many times \p LuaGetFunc
     *   will be called, otherwise only the first index will be set. The size of
     *   the array is always 4.
     * - A \p Placeholder will return a \p Placeholder without touching the
     *   stack or \p stackIndex. @see Placeholder.
     * - A \p lua_State* will return the \p lua parameter without touching the
     *   stack or \p stackIndex
     * - A \p Variadic will convert parameters from \p stackIndex until
     *   \p lua_gettop is reached and return a \p Variadic<T>.
     *
     * @see LuaGetFunc
     * @see GetDefault
     * @see LuaSetFunc
     *
     * @tparam TargetType The type to treat the stack parameter as. The type
     * must have a LuaGetFunc and GetDefault specialization
     * @param lua
     * @param stackIndex
     * @return auto See full description
     */
    template<typename TargetType>
    auto GetParameter(lua_State* lua, int& stackIndex)
    {
        static_assert(!std::is_reference_v<TargetType>);

        if constexpr(is_variadic<TargetType>::value)
        {
            auto var = Variadic<typename TargetType::value_type>();
            // Eat the rest of the arguments. Nom nom nom
            var.count = std::min(lua_gettop(lua) - stackIndex + 1, MAX_VARIADIC_ARG_COUNT);
            for(int j = 0; stackIndex <= lua_gettop(lua); ++stackIndex, ++j)
                var.arr.at(j) = LuaGetFunc<typename TargetType::value_type>(lua, stackIndex);
            return var;
        }
        else if constexpr(std::is_same_v<TargetType, lua_State*>)
        {
            return lua;
        }
        else if constexpr(std::is_same_v<TargetType, Placeholder>)
        {
            return Placeholder{.stackIndex = stackIndex++};
        }
        else if constexpr(std::is_pointer_v<TargetType> && !std::is_same_v<TargetType, const char*>)
        {
            using NakedTargetType = std::remove_const_t<std::remove_pointer_t<TargetType>>;
            auto arr = std::array<NakedTargetType, 4>{GetDefault<NakedTargetType>};
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
                    arr[j - 1] = LuaGetFunc<NakedTargetType>(lua, -1);
                    lua_pop(lua, 1);
                }
            }
            else
                arr[0] = LuaGetFunc<NakedTargetType>(lua, stackIndex);

            stackIndex++;

            return arr;
        }
        else
        {
            // If there is some error here about LuaGetFunc it is because it isn't
            // specialized for type TargetType
            return stackIndex <= lua_gettop(lua)
                       ? (TargetType)LuaGetFunc<TargetType>(lua, stackIndex++)
                       : GetDefault<TargetType>;
        }
    }

    template<typename ReturnType, typename T>
    void PushOneReturnValue(lua_State* lua, T& val, const int stackIndex, int& returnValueCount)
    {
        // Not a pointer or constant, then the value can't be changed by the
        // function
        if constexpr(
            !std::is_pointer_v<ReturnType> || is_const_pointer_v<ReturnType>
            || std::is_same_v<ReturnType, lua_State*>)
            return;
        else
        {
            using U = std::remove_pointer_t<std::remove_reference_t<ReturnType>>;
            if(lua_istable(lua, stackIndex)) // +1 ?
            {
                int len = luaL_len(lua, stackIndex);
                lua_createtable(lua, len, 0);
                for(int i = 0; i < len; ++i)
                {
                    lua_pushnumber(lua, i + 1);
                    LuaSetFunc<U>(lua, val.at(i));
                    lua_settable(lua, -3);
                }
            }
            else
                LuaSetFunc<U>(lua, val.at(0));

            returnValueCount += 1;
        }
    }

    template<typename... Types, typename... TupleTypes, std::size_t... I>
    void PushReturnValuesImpl(
        lua_State* luaState,
        std::tuple<TupleTypes...>& tuple,
        std::index_sequence<I...>,
        int& returnValueCount)
    {
        (..., PushOneReturnValue<Types>(luaState, std::get<I>(tuple), I + 1, returnValueCount));
    }

    /**
     * @brief Push the values in the given tuple to the lua stack if applicable
     * to FuncTypes
     *
     * @tparam FuncTypes
     * @tparam TupleTypes
     * @param luaState
     * @param tuple
     * @return int
     */
    template<typename... FuncTypes, typename... TupleTypes>
    int PushReturnValues(lua_State* luaState, std::tuple<TupleTypes...>& tuple)
    {
        int returnValueCount = 0;
        PushReturnValuesImpl<FuncTypes...>(
            luaState,
            tuple,
            std::make_index_sequence<std::tuple_size_v<std::tuple<TupleTypes...>>>{},
            returnValueCount);
        return returnValueCount;
    }

    template<typename TargetType, typename T>
    auto ReferenceOneValue(T& val)
    {
        if constexpr(std::is_same_v<TargetType, lua_State*>)
            return val;
        else if constexpr(std::is_pointer_v<TargetType> && !std::is_same_v<const char*, TargetType>)
            return (TargetType)&val;
        else
            return val;
    }

    // std::array is used by GetParameter and should be handled in a type-safe way
    template<typename TargetType, typename T, std::size_t Size>
    auto ReferenceOneValue(std::array<T, Size>& val)
    {
        if constexpr(std::is_pointer_v<TargetType> && !std::is_same_v<const char*, TargetType>)
            return val.data();
        else
            static_assert(always_false<T>);
    }

    template<typename... TargetTypes, typename... TupleTypes, std::size_t... I>
    auto ReferenceValuesImpl(std::tuple<TupleTypes...>& tuple, std::index_sequence<I...>)
    {
        return std::tuple{ReferenceOneValue<TargetTypes>(std::get<I>(tuple))...};
    }

    /**
     * @brief Given a tuple with owned values, returns a new tuple where any
     * pointer from \p TargetTypes points to the equivalent index in \p tuple
     *
     * @tparam TargetTypes
     * @tparam OwnedTypes
     * @param tuple
     * @return auto
     */
    template<typename... TargetTypes, typename... OwnedTypes>
    auto ReferenceValues(std::tuple<OwnedTypes...>& tuple)
    {
        return ReferenceValuesImpl<TargetTypes...>(
            tuple,
            std::make_index_sequence<std::tuple_size_v<std::tuple<OwnedTypes...>>>{});
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
        requires(sizeof...(Args) > 0)
    int LuaWrapper(lua_State* lua)
    {
        // Keeps track of which stack index to fetch the next parameter from,
        // needs to be a local variable since GetParameter takes a reference
        int stackIndex = 1;
        // The tuple must own all its values, so it cannot contain const refs
        std::tuple ownedArguments{GetParameter<std::remove_cvref_t<Args>>(lua, stackIndex)...};
        std::tuple functionArguments = ReferenceValues<Args...>(ownedArguments);

        int retCount = 0;
        // Call supplied function, since the types R and Args are known the cast
        // is safe.
        // Here it is assumed that the function will return something, which is
        // true for most ImGui functions, but not all. This is a WIP though
        auto f = (R(*)(Args...))lua_touserdata(lua, lua_upvalueindex(1));
        if constexpr(!std::is_same_v<R, void>)
        {
            R res = std::apply(f, functionArguments);
            LuaSetFunc<R>(lua, res);
            retCount++;
        }
        else
        {
            std::apply(f, functionArguments);
        }

        retCount += PushReturnValues<Args...>(lua, ownedArguments);

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
        requires(sizeof...(Args) > 0)
    int LuaWrapperMember(lua_State* lua)
    {
        int stackIndex = 1;
        std::tuple ownedArguments{GetParameter<std::remove_cvref_t<Args>>(lua, stackIndex)...};
        std::tuple functionArgumentsWithoutInstance = ReferenceValues<Args...>(ownedArguments);

        int retCount = 0;
        auto f = (R(*)(T, Args...))lua_touserdata(lua, lua_upvalueindex(1));
        T instance = (T)lua_touserdata(lua, lua_upvalueindex(2));
        auto functionArguments =
            std::tuple_cat(std::make_tuple(instance), functionArgumentsWithoutInstance);
        if constexpr(!std::is_same_v<R, void>)
        {
            R res = std::apply(f, functionArguments);
            LuaSetFunc<R>(lua, res);
            retCount++;
        }
        else
        {
            std::apply(f, functionArguments);
        }

        retCount += PushReturnValues<Args...>(lua, ownedArguments);

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
