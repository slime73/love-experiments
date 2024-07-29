#pragma once

#include "common/runtime.h"

namespace love::video
{

int luaopen_framesync(lua_State *L);
extern const luaL_Reg w_FrameSync_functions[];

} // love::video
