/*********************************************
 * mylua.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Jan, 2014
 ********************************************/

#ifndef _MYLUA_H_
#define _MYLUA_H_

#include "lua.hpp"
#include "misc.h"

class mylua;
extern mylua g_mylua;

class mylua
{
public:
	mylua();
	virtual ~mylua();

	void set_lua_state(lua_State* state);

	lua_State* get_lua_state();

	void register_funcs();

public:
	// functions exposed to lua state.
	static int version(lua_State* L);
	static int sess_statistics(lua_State* L);
	static int horos_run(lua_State* L);
	static int horos_stop(lua_State* L);

private:
	static lua_State* _lua_state;
};

#endif /* _MYLUA_H_ */
