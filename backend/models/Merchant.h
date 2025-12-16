#pragma once
#include <string>

struct Merchant {
	int id;
	std::string username;
	std::string passwordHash;
	std::string storeName;
	std::string createdAt;
};


