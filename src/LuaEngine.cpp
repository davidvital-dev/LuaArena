#include "LuaEngine.hpp"

LuaEngine::LuaEngine() noexcept
    : state_(nullptr) {}

LuaEngine::~LuaEngine() noexcept = default;

bool LuaEngine::isInitialized() const noexcept {
    return state_ != nullptr;
}

lua_State* LuaEngine::getState() noexcept {
    return state_;
}

const lua_State* LuaEngine::getState() const noexcept {
    return state_;
}
