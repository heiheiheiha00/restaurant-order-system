#pragma once
#include <optional>
#include <string>
#include "../models/User.h"
#include "../models/Merchant.h"

struct AuthToken {
	std::string token;
	int subjectId;
};

class AuthService {
public:
	bool registerUser(const std::string& username, const std::string& password, const std::string& phone, std::string& errMsg);
	bool registerMerchant(const std::string& username, const std::string& password, const std::string& storeName, std::string& errMsg);

	std::optional<AuthToken> loginUser(const std::string& username, const std::string& password, std::string& errMsg);
	std::optional<AuthToken> loginMerchant(const std::string& username, const std::string& password, std::string& errMsg);

	std::optional<User> authenticateUser(const std::string& token, std::string& errMsg);
	std::optional<Merchant> authenticateMerchant(const std::string& token, std::string& errMsg);

private:
	static bool isValidUserAccount(const std::string& username);
	static std::string hashPassword(const std::string& value);
	static bool verifyPassword(const std::string& password, const std::string& hash);
	static std::string generateToken();
	static std::string buildExpiryString(int hoursAhead);
};


