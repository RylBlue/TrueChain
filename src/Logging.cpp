#include "Logging.h"

#include <iostream>
#include <fstream>

#include <mutex>

namespace Log{
	bool info = true;

	bool debug = false;

	bool use_console = true;

	std::string trimmer = __FILE__;

	std::mutex log_mutex;

	bool use_log_file = false;

	std::ofstream log_file;


	std::string path_trim(std::string file_path) {
		unsigned int i = 0;
		for (; i < file_path.length() && i < trimmer.length(); ++i) {
			if (trimmer.at(i) != file_path.at(i)) {
				break;
			}
		}
		return file_path.substr(i, file_path.length());
	}
}

void Log::set_info_state(bool b) {
	info = b;
}

void Log::set_debug_state(bool b) {
	debug = b;
}

void Log::set_console_output(bool b) {
	use_console = true;
}

void Log::set_base_path(std::string p) {
	trimmer = p;
}

void Log::set_log_file(std::string p) {
	log_mutex.lock();
	if (use_log_file) {
		log_file.close();
		use_log_file = false;
	}
	log_file = std::ofstream(p);
	log_mutex.unlock();

	Log::conditional_log_quit(!log_file.good(), __LINE__, "Logging.cpp", "Log file could not be opened: " + p);
	use_log_file = true;
}

//INFO Text (bold cyan)
//"\033[1;36m"
//"\033[0m"

//WARN Text (bold yellow)
//"\033[1;33m"
//"\033[0m"

//ERR Text (bold red)
//"\033[1;31m"
//"\033[0m"

void Log::log_info(unsigned int line_number, std::string file, std::string msg) {
	log_mutex.lock();
	if (use_console && info) {
		std::cout << "\033[1;36m[" << path_trim(file) << ": " << line_number << "] (INFO): \033[0m" << msg << std::endl;
	}
	if (use_log_file) {
		log_file << "[" << path_trim(file) << ": " << line_number << "] (INFO): " << msg << std::endl;
	}
	log_mutex.unlock();
}

void Log::log_warning(unsigned int line_number, std::string file, std::string msg) {
	log_mutex.lock();
	if (use_console && debug) {
		std::cout << "\033[1;33m[" << path_trim(file) << ": " << line_number << "] (WARN): \033[0m" << msg << std::endl;
	}
	if (use_log_file) {
		log_file << "[" << path_trim(file) << ": " << line_number << "] (WARN): " << msg << std::endl;
	}
	log_mutex.unlock();
}

void Log::log_error_quit(unsigned int line_number, std::string file, std::string msg) {
	log_mutex.lock();
	if (use_console) {
		std::cout << "\033[1;31m[" << path_trim(file) << ": " << line_number << "] (ERR): \033[0m" << msg << std::endl;
	}
	if (use_log_file) {
		log_file << "[" << path_trim(file) << ": " << line_number << "] (ERR): " << msg << std::endl;
	}
	log_mutex.unlock();
	exit('L');
}

void Log::conditional_log_warn(bool b, unsigned int line_number, std::string file, std::string msg) {
	if (b) {
		log_mutex.lock();
		if (use_console && debug) {
			std::cout << "\033[1;33m[" << path_trim(file) << ": " << line_number << "] (WARN): \033[0m" << msg << std::endl;
		}
		if (use_log_file) {
			log_file << "[" << path_trim(file) << ": " << line_number << "] (WARN): " << msg << std::endl;
		}
		log_mutex.unlock();
	}
}

void Log::conditional_log_quit(bool b, unsigned int line_number, std::string file, std::string msg) {
	if (b) {
		log_mutex.lock();
		if (use_console) {
			std::cout << "\033[1;31m[" << path_trim(file) << ": " << line_number << "] (ERR): \033[0m" << msg << std::endl;
		}
		if (use_log_file) {
			log_file << "[" << path_trim(file) << ": " << line_number << "] (ERR): " << msg << std::endl;
		}
		log_mutex.unlock();
		exit('L');
	}
}