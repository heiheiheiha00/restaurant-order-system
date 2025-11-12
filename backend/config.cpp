#include "config.h"
#include <cstdlib>

std::string get_env_str(const char* key, const char* defVal) {
	const char* v = std::getenv(key);
	return v ? std::string(v) : std::string(defVal);
}

int get_env_int(const char* key, int defVal) {
	const char* v = std::getenv(key);
	if (!v) return defVal;
	try {
		return std::stoi(v);
	} catch (...) {
		return defVal;
	}
}

std::string get_server_host() {
	return get_env_str("BACKEND_HOST", "127.0.0.1");
}

int get_server_port() {
	return get_env_int("BACKEND_PORT", 8081);
}


