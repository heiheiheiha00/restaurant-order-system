#pragma once
#include <string>
#include <optional>

struct Session {
	int id;
	std::string token;
	std::optional<int> userId;
	std::optional<int> merchantId;
	std::string expiresAt;
	std::string createdAt;
};


