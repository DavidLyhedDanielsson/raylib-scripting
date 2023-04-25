#pragma once

extern "C" {
#include <lua.h>
}
#include <numeric> // Why isn't this in algorithm??
#include <optional>
#include <string>
#include <vector>

#include <external/raylib.hpp>

namespace LuaValidator
{
    // https://stackoverflow.com/questions/53945490/how-to-assert-that-a-constexpr-if-else-clause-never-happen
    template<class...>
    constexpr std::false_type always_false{};

    struct LuaValidator
    {
      private:
        lua_State* lua;
        std::vector<const char*> stack;

        /**
         * @brief "Smart" object that uses RAII to keep track of stack fields.
         * As nested fields are accessed, `stack` expands.
         */
        struct Field
        {
            lua_State* lua;
            // If the type is LUA_TNIL, the field didn't exist
            int type;
            std::vector<const char*>& stack;
            // If the object has been moved the stack should not be "popped" in
            // the destructor
            bool moved = false;

            Field(lua_State* lua, const char* field, std::vector<const char*>& stack)
                : lua(lua)
                , stack(stack)
            {
                this->type = lua_getfield(lua, lua_gettop(lua), field);
                stack.push_back(field);
            }
            Field(const Field&) = delete;
            Field& operator=(const Field&) = delete;

            Field(Field&& other): lua(other.lua), type(other.type), stack(other.stack)
            {
                other.moved = true;
            }

            ~Field()
            {
                if(!moved)
                {
                    lua_pop(lua, 1);
                    stack.pop_back();
                }
            }
        };

        /**
         * @brief Appends the current "stack trace" to the given error message and pushes it into
         * `errors`
         *
         * @param errorMessage
         */
        void Invalidate(std::string errorMessage)
        {
            std::string stackTrace = std::accumulate(
                this->stack.begin(),
                this->stack.end(),
                std::string(),
                [](std::string left, std::string right) { return left + "." + right; });
            errorMessage += " " + stackTrace;
            errors.push_back(std::move(errorMessage));
        }

        /**
         * @brief Verifies that the given field exists, otherwise calls `Invalidate`
         *
         * @param field
         * @return std::optional<Field> the field that exists, or empty
         */
        std::optional<Field> expectField(const char* field)
        {
            auto hold = Field(lua, field, stack);
            if(hold.type == LUA_TNIL)
            {
                Invalidate(std::string("Field ") + field + " does not exist at");
                return std::nullopt;
            }

            return hold;
        }

      public:
        std::vector<std::string> errors;

        LuaValidator(lua_State* lua): lua(lua) {}

        template<typename T>
        LuaValidator& FieldIs(const char* field)
        {
            // An error here means FieldIs has been called with a T that has no overload yet
            static_assert(always_false<T>);
        }

        template<>
        LuaValidator& FieldIs<double>(const char* field)
        {
            auto hold = expectField(field);
            if(!hold.has_value())
                return *this;

            if(!lua_isnumber(lua, -1))
                Invalidate(std::string("Field ") + field + " is not a number");

            return *this;
        }
        template<>
        LuaValidator& FieldIs<float>(const char* field)
        {
            return FieldIs<double>(field);
        }

        template<>
        LuaValidator& FieldIs<uint64_t>(const char* field)
        {
            auto hold = expectField(field);
            if(!hold.has_value())
                return *this;

            if(!lua_isinteger(lua, -1))
                Invalidate(std::string("Field ") + field + " is not an integer");

            return *this;
        }
        template<>
        LuaValidator& FieldIs<uint32_t>(const char* field)
        {
            return FieldIs<uint64_t>(field);
        }
        template<>
        LuaValidator& FieldIs<uint16_t>(const char* field)
        {
            return FieldIs<uint64_t>(field);
        }
        template<>
        LuaValidator& FieldIs<uint8_t>(const char* field)
        {
            return FieldIs<uint64_t>(field);
        }

        template<>
        LuaValidator& FieldIs<int64_t>(const char* field)
        {
            auto hold = expectField(field);
            if(!hold.has_value())
                return *this;

            if(!lua_isinteger(lua, -1))
                Invalidate(std::string("Field ") + field + " is not an integer");

            return *this;
        }
        template<>
        LuaValidator& FieldIs<int32_t>(const char* field)
        {
            return FieldIs<int64_t>(field);
        }
        template<>
        LuaValidator& FieldIs<int16_t>(const char* field)
        {
            return FieldIs<int64_t>(field);
        }
        template<>
        LuaValidator& FieldIs<int8_t>(const char* field)
        {
            return FieldIs<int64_t>(field);
        }

        template<>
        LuaValidator& FieldIs<const char*>(const char* field)
        {
            auto hold = expectField(field);
            if(!hold.has_value())
                return *this;

            if(!lua_isstring(lua, -1))
                Invalidate(std::string("Field ") + field + " is not a string");

            return *this;
        }

        // Raylib

        template<>
        LuaValidator& FieldIs<Vector3>(const char* field)
        {
            auto hold = expectField(field);
            if(!hold.has_value())
                return *this;

            FieldIs<float>("x");
            FieldIs<float>("y");
            FieldIs<float>("z");
            return *this;
        }

        /**
         * @brief Verifies that the given field is a table and executes the given function if it is.
         *
         * The function should be used to keep verifying the table. Keep calling `FieldIs` and
         * `VerifyTable` on the supplied LuaValidator
         *
         * @tparam Func A function with the signature void(LuaValidator&)
         * @param field
         * @param func
         * @return LuaValidator&
         */
        template<typename Func>
        LuaValidator& VerifyTable(const char* field, const Func& func)
        {
            auto hold = expectField(field);
            if(!hold.has_value())
                return *this;

            if(!lua_isstring(lua, -1))
                Invalidate(std::string("Field ") + field + " is not a table");
            else
                func(*this);

            return *this;
        }

        /**
         * @brief Concatenates all errors to a string and returns it
         *
         * @return std::optional<std::string>
         */
        std::optional<std::string> GetErrorString() const
        {
            if(!this->errors.empty())
            {
                std::string errorMessage = std::accumulate(
                    this->errors.begin(),
                    this->errors.end(),
                    std::string(),
                    [](std::string left, std::string right) { return left + "\n" + right; });
                return errorMessage;
            }
            else
                return std::nullopt;
        }
    };
}