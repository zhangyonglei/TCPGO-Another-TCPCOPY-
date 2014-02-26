/*********************************************
 * testsuite.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Feb, 2014
 ********************************************/

#include "testsuite.h"
#include "cute_logger.h"
#include "mylua.h"

testsuite g_testsuite;

testsuite::testsuite()
{
}

testsuite::~testsuite()
{
	_tester->join();
}

void testsuite::get_ready()
{
	_tester.reset(
			new boost::thread(boost::bind(&testsuite::run_worker, this))
	);
}

void testsuite::report_sess_traffic(std::string client_src_ip,
						 uint16_t port,
						 const std::list<ip_pkt>& traffic,
						 tcpsession::cause_of_death cause)
{

}

void testsuite::run_worker()
{
	bool success;
	boost::shared_ptr<job_block> job;

	while (!_done)
	{
		success = _jobs.pop(job);
		if (success)
		{
			_count_jobs--;
			//std::cout << "consumed " << job->_client_str_ip << std::endl;
			//std::cout << _count_jobs << " left" << std::endl;

			// let's do the job.
		}
		else
		{
			boost::mutex::scoped_lock lock(_mutex);
			_con_var.timed_wait(lock, boost::posix_time::milliseconds(5000));
			//std::cout << "quit timed_wait" << std::endl;
		}
	}
}

void testsuite::load_lua_scripts()
{
	std::list<std::string> scripts;

	std::string home = g_configuration.get_lua_scripts_home();
	find_files(home, ".*\\.lua$", scripts);

	for (std::list<std::string>::iterator ite = scripts.begin();
		ite != scripts.end();
		++ite)
	{
		g_mylua.load_lua_module(*ite);
	}
}

void testsuite::load_shared_objects()
{
	std::list<std::string> shared_objs;

	std::string home = g_configuration.get_so_home();
	find_files(home, ".*\\.so$", shared_objs);

	for (std::list<std::string>::iterator ite = shared_objs.begin();
		ite != shared_objs.end();
		++ite)
	{

	}
}
