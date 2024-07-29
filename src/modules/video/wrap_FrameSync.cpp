#include "FrameSync.h"
#include "wrap_FrameSync.h"

namespace love::video
{

inline static FrameSync *luax_checkframesync(lua_State *L, int idx)
{
	return luax_checktype<FrameSync>(L, idx);
}

static int w_FrameSync_tell(lua_State *L)
{
	auto sync = luax_checkframesync(L, 1);
	luax_catchexcept(L, [&](){ lua_pushnumber(L, sync->tell()); });
	return 1;
}

static int w_FrameSync_isPlaying(lua_State *L)
{
	auto sync = luax_checkframesync(L, 1);
	luax_catchexcept(L, [&](){ lua_pushboolean(L, sync->isPlaying()); });
	return 1;
}

const luaL_Reg w_FrameSync_functions[] = {
	{ "tell", w_FrameSync_tell },
	{ "isPlaying", w_FrameSync_isPlaying },
	{nullptr, nullptr},
};

int luaopen_framesync(lua_State *L)
{
	luax_register_type(L, &FrameSync::type, w_FrameSync_functions, nullptr);
	return 0;
}

} // love::video