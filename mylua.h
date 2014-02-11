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

	void register_funcs();

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

private:
	void open_listening_port();
	void accept_conn(int fd);
	void readin_console_cmd(int fd);
	int run_lua_string(char* str);
	void print_console_prompt(int fd, bool newline);

private:
	static lua_State* _lua_state;

	bool _enable_console;  ///< if horos is loaded as a so from within lua console, it's not necessarily open another lua console.
	uint16_t _console_listening_port;
	int _console_listening_fd;
	int _console_connected_fd;
};

#endif /* _MYLUA_H_ */
