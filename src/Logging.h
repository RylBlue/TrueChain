#pragma once

#include <string>

//Usage example:
// 
//Log::conditional_log_quit(return_val == NULL, __LINE__, __FILE__, "Error Message");
// 
//Use the macros __LINE__ and __FILE__ which are prebuilt into c++ compilers

namespace Log {
	void set_debug_state(bool); //Default = false;

	void set_info_state(bool); //Default = true;

	void set_console_output(bool); //Default = true;

	void set_base_path(std::string); //Default = __FILE__; (When the code was compiled)

	void set_log_file(std::string path); //Default = ""; (No log file by default, only std::cout)


	void log_info(unsigned int line_number, std::string file, std::string msg);

	void log_warning(unsigned int line_number, std::string file, std::string msg);

	void log_error_quit(unsigned int line_number, std::string file, std::string msg);

	void conditional_log_quit(bool should_quit, unsigned int line_number, std::string file, std::string msg);

	void conditional_log_warn(bool should_warn, unsigned int line_number, std::string file, std::string msg);
}