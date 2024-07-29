// LOVE
#include "common/Module.h"
#include "audio/Audio.h"
#include "SourceSync.h"
#include "wrap_FrameSync.h"
#include "wrap_SourceSync.h"

namespace love::video
{

static int w_SourceSync_getSource(lua_State *L)
{
	auto audioModule = Module::getInstance<audio::Audio>(Module::M_AUDIO);
	auto sync = luax_checktype<SourceSync>(L, 1);
	luax_catchexcept(L, [&](){ luax_pushtype<audio::Source>(L, sync->getSource()); });
	return 1;
}

const luaL_Reg w_SourceSync_functions[] = {
	{ "getSource", w_SourceSync_getSource },
	{nullptr, nullptr},
};

int luaopen_sourcesync(lua_State *L)
{
	luax_register_type(L, &SourceSync::type, w_FrameSync_functions, w_SourceSync_functions, nullptr);
	return 0;
}

} // love::video
