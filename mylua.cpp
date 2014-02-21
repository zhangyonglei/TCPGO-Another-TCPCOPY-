/*********************************************
 * mylua.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Jan, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "mylua.h"
#include "version.h"
#include "statistics_bureau.h"
#include "horos.h"

#define HOROS_CONSOLE_PROMPT  "horos>"

lua_State* mylua::_lua_state;
class mylua g_mylua;

/**
 * This function will be invoked while .so is loaded from within lua environment using "require".
 */
extern "C" __attribute__((visibility("default"))) int luaopen_libhoros(lua_State* L)
{
	g_mylua.disable_console();
	g_mylua.set_lua_state(L);
	g_mylua.register_funcs();

	return 1;
}

mylua::mylua()
{
	_console_listening_port = 1994;
	_console_listening_fd = -1;
	_console_connected_fd = -1;
	_lua_state = NULL;
	_enable_console = true;
}

mylua::~mylua()
{
	set_lua_state(NULL);
}

// the following are functions exposed to lua state.
int mylua::version(lua_State* L)
{
	lua_pushstring(L, "Horos "VERSION_NUM);

	return 1;
}

int mylua::sess_statistics(lua_State* L)
{
	std::string s;

	s = g_statistics_bureau.sess_statistics();

	lua_pushstring(L, s.c_str());

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

int mylua::turn_on_log(lua_State* L)
{
	int argc;
	bool log_on;
	std::string ret_val;

	argc = lua_gettop(L);
	log_on = lua_toboolean(L, 1);
	g_configuration.set_log_on(log_on);

	if (g_configuration.get_log_on())
	{
		ret_val = "Log is on.";
	}
	else
	{
		ret_val = "Log is off.";
	}

	lua_pushstring(L, ret_val.c_str());

	return 1;
}

// the above are functions exposed to lua state.

static const struct luaL_Reg lua_funcs[] = {
	{"version",   mylua::version},
	{"sess_stat", mylua::sess_statistics},
	{"run", mylua::horos_run},
	{"stop", mylua::horos_stop},
	{"log_on", mylua::turn_on_log},
	{NULL, NULL}
};

void mylua::set_lua_state(lua_State* state)
{
	if (_lua_state != NULL)
	{
		lua_close(_lua_state);
	}

	_lua_state = state;
}

lua_State* mylua::get_lua_state()
{
	return _lua_state;
}

void mylua::register_funcs()
{
	const char* key;

	// luaL_newlibtable(lua_state, 1);
	// luaL_setfuncs(lua_state, lua_tests, 0);  // use luaL_newlib() to simplify the code.
	luaL_newlib(_lua_state, lua_funcs);

    // table holding all the functions exported to lua is on stack top
    lua_pushnil(_lua_state);  // first key is nil

    while (lua_next(_lua_state, -2) != 0)
    {
    	// 'key' is at index -2 and 'value' is at index -1
    	assert(LUA_TSTRING == lua_type(_lua_state, -2)); // function name is a string
    	key = lua_tolstring(_lua_state, -2, NULL);
    	lua_setglobal(_lua_state, key);
    }
    luaL_dostring(_lua_state, "p = print");  // alias p to print to save typing efforts.
}

void mylua::disable_console()
{
	_enable_console = false;
}

int mylua::get_ready()
{
	lua_State* state;

	// in the case libhoros.so is loaded from lua console whose state has already
	// passed in. There is no need to create another lua state.
	if (NULL == _lua_state)
	{
		state = luaL_newstate();
		assert(NULL != state);

		luaL_openlibs(state);
		set_lua_state(state);
		register_funcs();
	}

	if (_enable_console)
	{
		open_listening_port();
	}
}

void mylua::open_listening_port()
{
	int ret;
	int i, opt;
	socklen_t len;
	struct sockaddr_in addr;

	_console_listening_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_console_listening_fd == -1)
	{
		perror("socket");
		abort();
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_port = ntohs(_console_listening_port);
	addr.sin_family = AF_INET;

	opt = 1;
	ret = setsockopt(_console_listening_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1)
	{
		perror("setsockopt");
		abort();
	}

	if (bind(_console_listening_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("bind");
		abort();
	}

	if (listen(_console_listening_fd, 10) == -1)
	{
		perror("listen");
		abort();
	}

	g_poller.register_evt(_console_listening_fd, poller::POLLIN, this);
}

void mylua::pollin_handler(int fd)
{
	int ret;

	if (fd == _console_listening_fd)
	{
		accept_conn(fd);
	}
	else if (fd == _console_connected_fd)
	{
		readin_console_cmd(fd);
	}
	else
	{
		// not supposed to get here.
		abort();
	}
}

void mylua::accept_conn(int fd)
{
	std::string welcome;
	// A connection is already in use.
	if (_console_connected_fd > 0)
	{
		return;
	}

	_console_connected_fd = accept(fd, NULL, NULL);
	if (_console_connected_fd < 0)
	{
		perror("accept");
		return;
	}

	g_poller.register_evt(_console_connected_fd, poller::POLLIN, this);

	welcome = "Welcome to the horos console v"VERSION_NUM"\n";
	write(_console_connected_fd, welcome.c_str(), welcome.length());
	print_console_prompt(_console_connected_fd, false);
}

void mylua::readin_console_cmd(int fd)
{
	int ret;
	char buff[1024];

	ret = read(fd, buff, sizeof(buff));
	if (ret > 0)
	{
		buff[ret] = 0;
		run_lua_string(buff);
	}
	else
	{
		_console_connected_fd = -1;
		g_poller.deregister_evt(fd);
		close(fd);
	}
}

void mylua::print_console_prompt(int fd, bool newline)
{
	if (newline)
	{
		write(fd, "\n"HOROS_CONSOLE_PROMPT, sizeof(HOROS_CONSOLE_PROMPT) + 1);
	}
	else
	{
		write(fd, HOROS_CONSOLE_PROMPT, sizeof(HOROS_CONSOLE_PROMPT));
	}
}

int mylua::run_lua_string(char* str)
{
	int orig_top, curr_top;
	int new_stack_frame;
	int retcode;
	std::ostringstream ss;

	if (!strstr(str, "return"))
	{
		ss << "return " << str;
	}
	else
	{
		ss << str;
	}

	orig_top = lua_gettop(_lua_state);
	retcode = luaL_dostring(_lua_state, ss.str().c_str());
	curr_top = lua_gettop(_lua_state);

	new_stack_frame = curr_top - orig_top;
	assert(new_stack_frame >= 0);
	// got info on stack.
	if (new_stack_frame > 0)
	{
		for (int i = 1; i <= new_stack_frame; i++)
		{
			const char* retval;
			if (!lua_isnil(_lua_state, -i))
			{
				retval = luaL_checkstring(_lua_state, -i);
				write(_console_connected_fd, retval, strlen(retval));
			}
			else
			{
				retval = "Not recognizable command.";
				write(_console_connected_fd, retval, strlen(retval));
			}
		}
	}

_exit:
	// clear the lua top.
	lua_settop(_lua_state, 0);
	print_console_prompt(_console_connected_fd, true);
	return retcode;
}

void mylua::pollout_handler(int fd)
{
	// not supposed to reach here.
	abort();
}
