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
#include "testsuite.h"
#include "cute_logger.h"

#define HOROS_CONSOLE_PROMPT  "horos>"

lua_State* mylua::_lua_state;
class mylua g_mylua;

class mylua_exception :
	public boost::exception,
	public std::exception
{
public:
		mylua_exception(const std::string& hint) : _what(hint)
		{
		}

		// throw() guarantees that no exception will be thrown.
		virtual const char* what() const throw()
		{
			return _what.c_str();
		}

		~mylua_exception()throw(){}
private:
		std::string _what;
};

/////////////////////////////////////////////////////////////////////////////////////
// the following are functions exposed to lua state.
int mylua::version(lua_State* L)
{
	lua_pushstring(L, "Horos "VERSION_NUM);

	return 1;
}

int mylua::sess_statistics(lua_State* L)
{
	int count;
	std::string s;

	count = lua_tointeger(L, 1);
	s = g_statistics_bureau.sess_statistics();
	lua_pushstring(L, s.c_str());

	return 1;
}

int mylua::horos_run(lua_State* L)
{
	// horos_init(); // obsoleted
	return 0;
}

int mylua::horos_stop(lua_State* L)
{
	// horos_uninit();  // obsoleted
	return 0;
}

int mylua::reload_testsuite(lua_State* L)
{
	// To serialize lua environment restart using timer mechanism.
	g_timer.register_one_shot_timer_event(
			boost::bind(&mylua::one_shot_timer_event_handler, &g_mylua), 0);

	lua_pushstring(L, "Testsuite has been reloaded.");

	return 1;
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

int mylua::flush_log(lua_State* L)
{
	g_logger.flush();

	return 0;
}

int mylua::save_traffic(lua_State* L)
{
	int retcode = 0;
	std::string pcap_file_path;

	int argc = lua_gettop(L);
	if (argc < 1)
	{
		lua_pushstring(L, "no parameter specified.\n");
		return 1;
	}

	pcap_file_path = luaL_checkstring(L, -1);
	retcode = g_testsuite.save_traffic(pcap_file_path);
	if (0 == retcode)
	{
		lua_pushstring(L, "Saved Successfully.\n");
	}
	else
	{
		lua_pushstring(L, "Failed to save traffic to pcap file.\n");
	}

	return 1;
}
// the above are functions exposed to lua state.

static const struct luaL_Reg lua_funcs[] = {
	{"version",   mylua::version},
	{"sess_stat", mylua::sess_statistics},
	{"stat", mylua::sess_statistics},
	{"run", mylua::horos_run},
	{"stop", mylua::horos_stop},
	{"reload_testsuite", mylua::reload_testsuite},
	{"log_on", mylua::turn_on_log},
	{"flush_log", mylua::flush_log},
	{"save_traffic", mylua::save_traffic},
	{NULL, NULL}
};
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * This function will be invoked while .so is loaded from within lua environment using "require".
 */
// obsoleted ...
//extern "C" __attribute__((visibility("default"))) int luaopen_libhoros(lua_State* L)
//{
//	g_mylua.disable_console();
//	g_mylua.set_lua_state(L);
//	g_mylua.register_APIs4lua(lua_funcs);
//
//	return 1;
//}

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

void mylua::register_APIs4lua(const luaL_Reg* lua_funcs)
{
	const char* key;

	// luaL_newlibtable(lua_state, 1);
	// luaL_setfuncs(lua_state, lua_tests, 0);  // use luaL_newlib() to simplify the code.
	luaL_newlib(_lua_state, lua_funcs);

    // table holding all the functions exported to lua is on stack top
    lua_pushnil(_lua_state);  // first key is nil

    while (lua_next(_lua_state, -2) != 0) // table is at index -1 at this moment.
    {
    	// after the call of lua_next, 'key' is at index -2 and 'value' is at index -1
    	assert(LUA_TSTRING == lua_type(_lua_state, -2)); // function name is a string
    	key = lua_tolstring(_lua_state, -2, NULL);
    	lua_setglobal(_lua_state, key);  // pops a value from the stack and sets it as the new value of global name.
    }
//  luaL_dostring(_lua_state, "p = print");  // alias p to print to save typing efforts.
}

void mylua::disable_console()
{
	_enable_console = false;
}

void mylua::init_lua_machine()
{
	lua_State* state;

	assert(NULL == _lua_state);

	state = luaL_newstate();
	assert(NULL != state);

	luaL_openlibs(state);
	set_lua_state(state);
	register_APIs4lua(lua_funcs);

	lua_atpanic(_lua_state, &mylua::lua_panic);
	// lua_atpanic(_lua_state, boost::bind(&mylua::bind_lua_panic, this, _1));
}

int mylua::get_ready()
{
	// in the case libhoros.so is loaded from lua console whose state has already
	// passed in. There is no need to create another lua state.
	if (NULL == _lua_state)
	{
		init_lua_machine();
	}

	if (_enable_console)
	{
		open_listening_port();
	}

	_lua_modules.clear();
}

void mylua::restart()
{
	set_lua_state(NULL);
	init_lua_machine();
}

int mylua::load_lua_module(const std::string& module_path)
{
	int orig_top, curr_top;
	int new_stack_frame;
	int retcode;
	bool well_formated;
	std::ostringstream ss;

	// extract the stem from a path and use the stem as the module name
	// which will be registered in the global lua environment.
	boost::filesystem::path the_path(module_path);
	std::string module_name = the_path.stem().generic_string();

	ss << boost::format("%s = dofile('%s')\n") % module_name % module_path
	   << boost::format("return %s.main ~= nil\n") % module_name;

	well_formated = false;
	try
	{
		retcode = do_lua_string(ss.str().c_str(), "b", &well_formated);
	}
	catch (mylua_exception& e)
	{
		g_logger.printf("%s", boost::diagnostic_information(e).c_str());
		std::cerr << "Failed to load lua script " << module_path << std::endl;
		assert(0 != retcode);

		return retcode;
	}

	if (well_formated && 0 == retcode)
	{
		_lua_modules.push_back(module_name);
	}
	else
	{
		if (well_formated)
		{
			g_logger.printf("Something wrong detected in lua script %s.\n", module_path.c_str());
		}
		else
		{
			g_logger.printf("Lua script %s should return a tuple with both request and response defined",
					module_path.c_str());
		}
		abort();
	}

	return retcode;
}

void mylua::run_lua_tests(const std::string& client_str_ip, uint16_t port,
						  const std::vector<char>& request, const std::vector<char>& response)
{
	for (std::vector<std::string>::iterator ite = _lua_modules.begin();
		 ite != _lua_modules.end();
		 ++ite)
	{
		const std::string& module_name(*ite);
		lua_getglobal(_lua_state, module_name.c_str()); // [FRAMES1] push the module "lua table" on the stack
		lua_getfield(_lua_state, -1, "main");        // [FRAMES2] push the module's main function on the stack
		lua_remove(_lua_state, -2);                  // [FRAMES1] remove the module "lua table" on the -2 level of the stack
		lua_pushstring(_lua_state, client_str_ip.c_str());       // [FRAMES2] push the second parameter
		lua_pushinteger(_lua_state, port);
		lua_pushlstring(_lua_state, &request[0], request.size());// [FRAMES3] push the first parameter
		lua_pushlstring(_lua_state, &response[0], response.size());// [FRAMES4] push the third parameter
		lua_pcall(_lua_state, 4, 0, 0); // int lua_pcall(lua_State *L, int nargs, int nresults, int msgh);

		lua_settop(_lua_state, 0);
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

	g_reactor.register_evt(_console_listening_fd, reactor::MYPOLLIN, this);
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
		close_conn_fd();
	}

	_console_connected_fd = accept(fd, NULL, NULL);
	if (_console_connected_fd < 0)
	{
		perror("accept");
		return;
	}

	g_reactor.register_evt(_console_connected_fd, reactor::MYPOLLIN, this);

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
		do_lua_console_cmd(buff);
	}
	else
	{
		assert(fd == _console_connected_fd);
		close_conn_fd();
	}
}

void mylua::close_conn_fd()
{
	if (_console_connected_fd > 0)
	{
		g_reactor.deregister_evt(_console_connected_fd);
		close(_console_connected_fd);
		_console_connected_fd = -1;
	}
}

void mylua::close_listening_fd()
{
	if (_console_listening_fd > 0)
	{
		g_reactor.deregister_evt(_console_listening_fd);
		close(_console_listening_fd);
		_console_listening_fd = -1;
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

int mylua::do_lua_console_cmd(const char* str)
{
	int orig_top, curr_top;
	int new_stack_frame;
	int retcode;
	const char* retval;
	std::ostringstream ss;

	if (!strstr(str, "return"))
	{
		// str will be executed in a new module environment.
		// "return" keyword has to be specified to get returned values from module environment.
		ss << "return " << str;
	}
	else
	{
		ss << str;
	}

	const std::string& the_cmd_line = ss.str();
	for (std::string::const_iterator ite = the_cmd_line.begin(); ite != the_cmd_line.end(); ++ite)
	{
		char ch = *ite;
		if ( !isprint(ch) && ch != '\r' && ch != '\n' ) // non-printable characters will corrupt the lua machine.
		{
			retval = "Non-printable character is not allowed in command string.";
			write(_console_connected_fd, retval, strlen(retval));
			goto _exit;
		}
	}

	orig_top = lua_gettop(_lua_state);
	retcode = luaL_dostring(_lua_state, the_cmd_line.c_str());
	curr_top = lua_gettop(_lua_state);

	new_stack_frame = curr_top - orig_top;
	// assert(new_stack_frame >= 0);
	if (new_stack_frame < 0)  // secure-crt console will give birth this weird phenomenon.
	{
		g_logger.printf("weired command line: [%s] causes a negative lua stack.\n", the_cmd_line.c_str());
		retval = "A error occurred when parsing command line.\n";
		write(_console_connected_fd, retval, strlen(retval));
		goto _exit;
	}
	// got info on stack.
	if (new_stack_frame > 0)
	{
		for (int i = 1; i <= new_stack_frame; i++)
		{
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

int mylua::do_lua_string(const char* lua_str, const char* retval_formats, ...)
{
	int retcode;
	int orig_top, curr_top;
	int new_stack_frame;
	va_list ap;

	va_start(ap, retval_formats);

	assert(NULL != lua_str && NULL != retval_formats);

	orig_top = lua_gettop(_lua_state);
	retcode = luaL_dostring(_lua_state, lua_str);
	curr_top = lua_gettop(_lua_state);

	new_stack_frame = curr_top - orig_top;
	// got info on stack.
	for (int i = 1; i <= new_stack_frame; i++)
	{
		if (!lua_isnil(_lua_state, -i))
		{
			char  retval_type;
			const char* retval_str;
			int*  ptr2i;
			bool* ptr2bool;
			double* ptr2d;
			std::string* ptr2str;
			retval_type = *(retval_formats + i - 1);

			if (0 != retcode)
			{
				retval_str = luaL_checkstring(_lua_state, -i);
				g_logger.printf("%s\n", retval_str);
				BOOST_THROW_EXCEPTION(mylua_exception(retval_str));
			}

			switch (retval_type)
			{
			case 'i':
				ptr2i = va_arg(ap, int*);
				*ptr2i = luaL_checkinteger(_lua_state, -i);
				break;

			case 'b':
				ptr2bool = va_arg(ap, bool*);
				*ptr2bool = lua_toboolean(_lua_state, -i);
				break;

			case 'd':
			    ptr2d = va_arg(ap, double*);
				*ptr2d = luaL_checknumber(_lua_state, -i);
				break;

			case 's':
				ptr2str = va_arg(ap, std::string*);
				*ptr2str = luaL_checkstring(_lua_state, -i);
				break;

			default:
				retcode = -1;
				abort();
			}
		}
		else
		{
			retcode = -1;
			abort();
		}
	}

	va_end(ap);

	lua_settop(_lua_state, 0);

	return retcode;
}

int mylua::lua_panic(lua_State* L)
{
	// catch the lua panic here.
	std::string errinfo = lua_tostring(L, -1);
	g_logger.printf("lua_panic: %s", errinfo.c_str());

	return 0;
}

// obsoleted
int mylua::bind_lua_panic(lua_State* L)
{
	// catch the lua panic here.
	return 0;
}

void mylua::one_shot_timer_event_handler()
{
	g_testsuite.stop();
	restart();
	g_testsuite.ready_go();
}

void mylua::pollout_handler(int fd)
{
	// not supposed to reach here.
	abort();
}
