/*********************************************
 * mylua.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Jan, 2014
 ********************************************/

#include "mylua.h"
#include "version.h"

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

int mylua::version(lua_State* L)
{
	lua_pushstring(_lua_state, VERSION_NUM);

	return 1;
}

static const struct luaL_Reg lua_funcs[] = {
	{"version",   mylua::version},
	{NULL, NULL}
};

mylua::mylua()
{
	set_lua_state(NULL);
}

mylua::~mylua()
{
}

void mylua::register_funcs()
{
	// luaL_newlibtable(lua_state, 1);
	// luaL_setfuncs(lua_state, lua_tests, 0);  // use luaL_newlib() to simplify the code.
	luaL_newlib(_lua_state, lua_funcs);
}

