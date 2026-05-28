#pragma once

#include <optional>
#include <type_traits>
#include <variant>

#include "lua.hpp"

namespace Mirael
{

/*
 * A ValueBuffer wraps the value at an Output Pin.
 *
 * The Runner assigns each Output Pin its own ValueBuffer, and that assignment persists
 * between frames for as long as that Pin exists.  Each Node Core is responsible for
 * updating the value of the ValueBuffers for its Output Pins as desired.  It is valid
 * for a Core to avoid updating the value if it wants, or to read the value that persisted
 * from the previous frame.
 */
class ValueBuffer final
{
public:
    explicit ValueBuffer(lua_State *luaState) : L(luaState) {}
    ~ValueBuffer() { clear(); }

    // disable move/copy
    ValueBuffer(const ValueBuffer &)            = delete;
    ValueBuffer &operator=(const ValueBuffer &) = delete;
    ValueBuffer(ValueBuffer &&)                 = delete;
    ValueBuffer &operator=(ValueBuffer &&)      = delete;

    bool isTrivial() const noexcept { return value_.index() <= LastTrivialTypeIndex; }
    bool isLuaRef() const noexcept { return value_.index() > LastTrivialTypeIndex; }

    void clear() { internalSafeSetValue(std::monostate()); }

    void setValue(bool other) { internalSafeSetValue(other); }
    void setValue(double other) { internalSafeSetValue(other); }
    void setValue(int other) { internalSafeSetValue(static_cast<double>(other)); }

    void setValue(std::string_view other)
    {
        lua_pushlstring(L, other.data(), other.size());
        internalSafeSetValue(LuaString{luaL_ref(L, LUA_REGISTRYINDEX)});
    }

    void setValue(const ValueBuffer &other)
    {
        if (other.isTrivial()) {
            internalSafeSetValue(other.value_);
            return;
        }

        // acquire new ref to the other's value
        std::visit(overloaded{[this]<typename T>(T &base)
                                  requires std::derived_from<T, LuaRefBase>
                              { lua_rawgeti(L, LUA_REGISTRYINDEX, base.ref); },

                              [](auto &&) { assert(false); }},

                              other.value_);
        int newRef = luaL_ref(L, LUA_REGISTRYINDEX);

        // prepare replacement variant
        value_t newValue = other.value_; // sets proper type efficiently, no destructor risk, other.value_.ref to be replaced
        std::visit(                      // now set correct ref value
            overloaded{[&]<typename T>(T &base)
                           requires std::derived_from<T, LuaRefBase>
                       { base.ref = newRef; },

                       [](auto &&) { assert(false); }},

                       newValue);

        // set it
        internalSafeSetValue(newValue);
    }

    bool setValueFromLuaStack()
    {
        int t = lua_type(L, -1);

        switch (t) {

        // trivial types require an explicit pop
        case LUA_TNIL:
            lua_pop(L, 1);
            clear(); // nil is the default/cleared value
            return true;
        case LUA_TBOOLEAN: {
            bool b = lua_toboolean(L, -1) != 0;
            lua_pop(L, 1);
            internalSafeSetValue(b);
            return true;
        }
        case LUA_TNUMBER: {
            double d = lua_tonumber(L, -1);
            lua_pop(L, 1);
            internalSafeSetValue(d);
            return true;
        }

        // ref types automatically pop as part of the luaL_ref() call
        case LUA_TSTRING:
            internalSafeSetValue(LuaString{luaL_ref(L, LUA_REGISTRYINDEX)});
            return true;
        case LUA_TTABLE:
            internalSafeSetValue(LuaTable{luaL_ref(L, LUA_REGISTRYINDEX)});
            return true;
        case LUA_TFUNCTION:
            internalSafeSetValue(LuaFunction{luaL_ref(L, LUA_REGISTRYINDEX)});
            return true;
        case LUA_TTHREAD:
            internalSafeSetValue(LuaThread{luaL_ref(L, LUA_REGISTRYINDEX)});
            return true;

        default: // TODO: we should have some sort of non-fatal indicator of this error, until the remaining types are supported
            lua_pop(L, 1);
            clear();
            return false;
        }
    }

    bool toBool() const noexcept
    {
        if (isBool())
            return std::get<bool>(value_);
        else if (isNil())
            return false;
        else
            return true; // lua standard - all but nil and false are considered true
    }

    std::optional<double> toDouble() const
    {
        if (isDouble())
            return std::get<double>(value_);
        else if (isString()) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, std::get<LuaString>(value_).ref);
            std::optional<double> result{};
            if (lua_isnumber(L, -1))
                result = lua_tonumber(L, -1);
            lua_pop(L, 1);
            return result;
        } else
            return std::nullopt;
    }

    std::optional<int> toInt() const
    {
        // use saturation math for actual numbers, but nullopt for non-numbers/NaN
        auto d = toDouble(); // allow strings to convert by reusing toDouble()
        // TODO: can increase efficiency by going straight to int in the string case, but that's low priority for now
        if (!d || std::isnan(*d))
            return std::nullopt;
        else if (*d >= static_cast<double>(INT_MAX))
            return INT_MAX;
        else if (*d <= static_cast<double>(INT_MIN))
            return INT_MIN;
        else
            return static_cast<int>(*d); // in range we truncate toward zero, same as C++ and LuaJIT internally
    }

    std::string toString() const // no optional<T> needed - this will always yield a string
    {
        if (isNil())
            return "nil";
        else if (isBool())
            return std::get<bool>(value_) ? "true" : "false";

        std::visit(overloaded{
            [this](double d) { lua_pushnumber(L, d); },

            [this]<typename T>(T &base)
                requires std::derived_from<T, LuaRefBase>
            { lua_rawgeti(L, LUA_REGISTRYINDEX, base.ref); },

            [](auto &&) { assert(false); } // all other value alternatives should already be handled above

            },
            value_);

        size_t len = 0;
        // NOTE: we're relying on LuaJIT-specific behavior that isn't in the Lua specification, where
        // it returns "type: 0x..." for non-coercible types.
        const char *s = lua_tolstring(L, -1, &len);
        if (!s)
            s = ""; // TODO: should really flag some kind of live warning when things like this happen
        std::string result(s, len);
        lua_pop(L, 1);

        return result;
    }

    void pushValueToLuaStack() const
    {
        std::visit(overloaded{
            [this](std::monostate) { lua_pushnil(L); },
            [this](bool b) { lua_pushboolean(L, b ? 1 : 0); },
            [this](double d) { lua_pushnumber(L, d); },

            [this]<typename T>(T &base)
                requires std::derived_from<T, LuaRefBase>
            { lua_rawgeti(L, LUA_REGISTRYINDEX, base.ref); },

            [this](auto &&) {
                assert(false); // shouldn't occur, but we handle it safely just in case
                lua_pushnil(L);
            },

            },
            value_);
    }

    bool isNil() const noexcept { return std::holds_alternative<std::monostate>(value_); }
    bool isBool() const noexcept { return std::holds_alternative<bool>(value_); }
    bool isDouble() const noexcept { return std::holds_alternative<double>(value_); }
    bool isString() const noexcept { return std::holds_alternative<LuaString>(value_); }
    bool isFunction() const noexcept { return std::holds_alternative<LuaFunction>(value_); }
    bool isThread() const noexcept { return std::holds_alternative<LuaThread>(value_); }
    bool isTable() const noexcept { return std::holds_alternative<LuaTable>(value_); }

private:
    struct LuaRefBase {
        int ref;
        explicit LuaRefBase(int r) : ref(r) {}
    };

    template <int Tag> struct LuaRef : public LuaRefBase {
        using LuaRefBase::LuaRefBase;
    };

    using LuaString   = LuaRef<LUA_TSTRING>;
    using LuaFunction = LuaRef<LUA_TFUNCTION>;
    using LuaThread   = LuaRef<LUA_TTHREAD>;
    using LuaTable    = LuaRef<LUA_TTABLE>;

    // Userdata is intentionally omitted for now.  In the future we will support several different flavors of userdata,
    // each with its own LuaRefBase-derived variant alternative and metatable.  For example, Vulkan Host-Visible Buffers
    // for manipulating textures or setting up compute shader inputs from Lua scripts when desired.

    using value_t = std::variant<std::monostate, // lua nil
                                 bool,           // lua bool
                                 double,         // lua number
                                 LuaString,      //
                                 LuaFunction,    //
                                 LuaThread,      //
                                 LuaTable        //
                                 >;

    static constexpr int LastTrivialTypeIndex = 2;
    static_assert(std::is_same_v<std::variant_alternative_t<LastTrivialTypeIndex, value_t>, double>,
                  "LastTrivialTypeIndex must be in sync with value_t.");
    static_assert(std::is_trivially_copyable_v<value_t>, "ValueBuffer::value_t must be trivially copyable.");

    template <typename... Ts> struct overloaded : Ts... { // helper for std::visit()
        using Ts::operator()...;
    };

    void internalSafeSetValue(value_t newValue)
    {
        if (isLuaRef())
            std::visit(overloaded{[this]<typename T>(T &base)
                                      requires std::derived_from<T, LuaRefBase>
                                  { luaL_unref(L, LUA_REGISTRYINDEX, base.ref); },

                                  [](auto &&) { assert(false); }

                                  },
                                  value_);

        value_ = newValue; // this is the ONLY line of code which should directly modify the value_ member
    }

    lua_State *L = nullptr; // traditional name in all examples, which I'm adopting even at odds to the naming standard for members
    value_t value_{};
};

} // namespace Mirael