/*********************************************
 * mylua.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Jan, 2014
 ********************************************/

#include "mylua.h"
#include "version.h"
#include "statistics_bureau.h"
#include "horos.h"

lua_State* mylua::_lua_state;
class mylua g_mylua;

/**
 * This function will be invoked while .so is loaded from within lua environment using "require".
 */
extern "C" __attribute__((visibility("default"))) int luaopen_libhoros(lua_State* L)
{
	g_mylua.set_lua_state(L);
	g_mylua.register_funcs();

	return 1;
}

// the following are functions exposed to lua state.
int mylua::version(lua_State* L)
{
	lua_pushstring(_lua_state, VERSION_NUM);

	return 1;
}

int mylua::sess_statistics(lua_State* L)
{
	std::string s;

	s = g_statistics_bureau.sess_statistics();

	lua_pushstring(_lua_state, s.c_str());

	return 1;
}

int mylua::horos_run(lua_State* L)
{
	horos_init();
}

int mylua::horos_stop(lua_State* L)
{
	horos_uninit();
}

// the above are functions exposed to lua state.

static const struct luaL_Reg lua_funcs[] = {
	{"version",   mylua::version},
	{"sess_stat", mylua::sess_statistics},
	{"run", mylua::horos_run},
	{"stop", mylua::horos_stop},
	{NULL, NULL}
};

mylua::mylua()
{
	lua_State* state;

	state = luaL_newstate();
	assert(NULL != state);

	luaL_openlibs(state);
	set_lua_state(state);
}

mylua::~mylua()
{
}

void mylua::set_lua_state(lua_State* state)
{
	_lua_state = state;
}

lua_State* mylua::get_lua_state()
{
	return _lua_state;
}

void mylua::register_funcs()
{
	// luaL_newlibtable(lua_state, 1);
	// luaL_setfuncs(lua_state, lua_tests, 0);  // use luaL_newlib() to simplify the code.
	luaL_newlib(_lua_state, lua_funcs);
}
