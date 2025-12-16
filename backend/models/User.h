#pragma once
#include <string>

struct User {
	int id;
	std::string username;
	std::string passwordHash;
	std::string phone;
	std::string createdAt;
};


