/*********************************************
 * mylua.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Jan, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _MYLUA_H_
#define _MYLUA_H_

#include "lua.hpp"
#include "misc.h"
#include "poller.h"

class mylua;
extern mylua g_mylua;

class mylua : public evt_workhorse
{
public:
	mylua();
	virtual ~mylua();

	void set_lua_state(lua_State* state);

	lua_State* get_lua_state();

	// register CAPIs to lua and expose them to the global environment.
	void register_APIs4lua(const luaL_Reg* lua_funcs);

	int get_ready();

	void disable_console();

public:
	// implement the evt_workhorse interface.
	virtual void pollin_handler(int fd);
	virtual void pollout_handler(int fd);

public:
	// functions exposed to lua state.
	static int version(lua_State* L);
	static int sess_statistics(lua_State* L);
	static int horos_run(lua_State* L);
	static int horos_stop(lua_State* L);
	static int turn_on_log(lua_State* L);

public:
	/**
	 * load the lua module from the lua script file specified by path.
	 */
	int load_lua_module(const std::string& module_path);

private:
	// debug console based on lua
	void open_listening_port();
	void accept_conn(int fd);
	void readin_console_cmd(int fd);
	void print_console_prompt(int fd, bool newline);
	int do_lua_console_cmd(const char* str);

private:
	/** run lua scripts and get the returned values.
	 * @param lua_str the lua string in lua jargon.
	 * @param retval_formats i: int, b: bool, d: double, s: std::string
	 * @return 0 on success, non-zero on failure.
	 */
	int do_lua_string(const char* lua_str, const char* retval_formats, ...);

	static int lua_panic(lua_State* L);
	int bind_lua_panic(lua_State* L);

private:
	// load all lua scripts for testing.
	void load_test_scripts();

private:
	static lua_State* _lua_state;

	bool _enable_console;  ///< if horos is loaded as a so from within lua console, it's not necessarily open another lua console.
	uint16_t _console_listening_port;
	int _console_listening_fd;
	int _console_connected_fd;

	std::vector<std::string> _lua_modules;
};

#endif /* _MYLUA_H_ */
