#include "DeltaSync.h"
#include "wrap_DeltaSync.h"
#include "wrap_FrameSync.h"

namespace love::video
{

inline static DeltaSync *luax_checkdeltasync(lua_State *L, int idx)
{
	return luax_checktype<DeltaSync>(L, idx);
}

static int w_DeltaSync_isAutomatic(lua_State *L)
{
	auto sync = luax_checkdeltasync(L, 1);
	luax_catchexcept(L, [&]() { luax_pushboolean(L, sync->isAutomatic()); });
	return 1;
}

static int w_DeltaSync_update(lua_State *L)
{
	auto sync = luax_checkdeltasync(L, 1);
	double dt = luaL_checknumber(L, 2);
	luax_catchexcept(L, [&]() { sync->updateManual(dt); });
	return 0;
}

const luaL_Reg w_DeltaSync_functions[] = {
	{ "isAutomatic", w_DeltaSync_isAutomatic },
	{ "update", w_DeltaSync_update },
	{nullptr, nullptr},
};

int luaopen_deltasync(lua_State *L)
{
	luax_register_type(L, &DeltaSync::type, w_FrameSync_functions, w_DeltaSync_functions, nullptr);
	return 0;
}

} // love::video
