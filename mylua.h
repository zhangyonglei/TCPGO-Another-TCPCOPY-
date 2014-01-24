/*********************************************
 * mylua.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Jan, 2014
 ********************************************/

#ifndef _MYLUA_H_
#define _MYLUA_H_

#include "lua.hpp"

class mylua;
extern mylua g_mylua;

class mylua
{
public:
	mylua();
	virtual ~mylua();

	void set_lua_state(lua_State* state)
	{
		_lua_state = state;
	}

	lua_State* get_lua_state()
	{
		return _lua_state;
	}

	void register_funcs();

public:
	// return the version string to lua.
	static int version(lua_State* L);

private:
	static lua_State* _lua_state;
};

#endif /* _MYLUA_H_ */
