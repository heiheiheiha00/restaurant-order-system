#include "AuthService.h"
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>
#include "../database/Database.h"

namespace {
	constexpr char kPepper[] = "restaurant-order-system-pepper";
	constexpr int kTokenHours = 24;

	std::string hexFromSizeT(size_t value) {
		std::stringstream ss;
		ss << std::hex << value;
		return ss.str();
	}
}

bool AuthService::registerUser(const std::string& username, const std::string& password, const std::string& phone, std::string& errMsg) {
	if (!isValidUserAccount(username)) {
		errMsg = "用户名需要 8 位且包含数字与字母";
		return false;
	}
	if (password.size() < 6) {
		errMsg = "密码至少 6 位";
		return false;
	}
	auto existing = Database::instance().getUserByUsername(username, errMsg);
	if (!errMsg.empty()) {
		return false;
	}
	if (existing.has_value()) {
		errMsg = "用户已存在";
		return false;
	}
	const auto hash = hashPassword(password);
	return Database::instance().createUser(username, hash, phone, errMsg);
}

bool AuthService::registerMerchant(const std::string& username, const std::string& password, const std::string& storeName, std::string& errMsg) {
	if (username.empty() || password.size() < 6) {
		errMsg = "商家账号和密码不能为空，且密码至少 6 位";
		return false;
	}
	auto existing = Database::instance().getMerchantByUsername(username, errMsg);
	if (!errMsg.empty()) {
		return false;
	}
	if (existing.has_value()) {
		errMsg = "商家账号已存在";
		return false;
	}
	const auto hash = hashPassword(password);
	return Database::instance().createMerchant(username, hash, storeName, errMsg);
}

std::optional<AuthToken> AuthService::loginUser(const std::string& username, const std::string& password, std::string& errMsg) {
	auto user = Database::instance().getUserByUsername(username, errMsg);
	if (!user.has_value()) {
		if (errMsg.empty()) errMsg = "用户不存在";
		return std::nullopt;
	}
	if (!verifyPassword(password, user->passwordHash)) {
		errMsg = "密码错误";
		return std::nullopt;
	}
	const auto token = generateToken();
	const auto expiry = buildExpiryString(kTokenHours);
	if (!Database::instance().createSessionToken(token, user->id, std::nullopt, expiry, errMsg)) {
		return std::nullopt;
	}
	return AuthToken{token, user->id};
}

std::optional<AuthToken> AuthService::loginMerchant(const std::string& username, const std::string& password, std::string& errMsg) {
	auto merchant = Database::instance().getMerchantByUsername(username, errMsg);
	if (!merchant.has_value()) {
		if (errMsg.empty()) errMsg = "商家不存在";
		return std::nullopt;
	}
	if (!verifyPassword(password, merchant->passwordHash)) {
		errMsg = "密码错误";
		return std::nullopt;
	}
	const auto token = generateToken();
	const auto expiry = buildExpiryString(kTokenHours);
	if (!Database::instance().createSessionToken(token, std::nullopt, merchant->id, expiry, errMsg)) {
		return std::nullopt;
	}
	return AuthToken{token, merchant->id};
}

std::optional<User> AuthService::authenticateUser(const std::string& token, std::string& errMsg) {
	auto session = Database::instance().getSessionByToken(token, errMsg);
	if (!session.has_value() || !session->userId.has_value()) {
		if (errMsg.empty()) errMsg = "会话无效";
		return std::nullopt;
	}
	return Database::instance().getUserById(session->userId.value(), errMsg);
}

std::optional<Merchant> AuthService::authenticateMerchant(const std::string& token, std::string& errMsg) {
	auto session = Database::instance().getSessionByToken(token, errMsg);
	if (!session.has_value() || !session->merchantId.has_value()) {
		if (errMsg.empty()) errMsg = "会话无效";
		return std::nullopt;
	}
	return Database::instance().getMerchantById(session->merchantId.value(), errMsg);
}

bool AuthService::isValidUserAccount(const std::string& username) {
	if (username.size() != 8) return false;
	bool hasDigit = false;
	bool hasAlpha = false;
	for (char c : username) {
		if (!std::isalnum(static_cast<unsigned char>(c))) return false;
		if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
		if (std::isalpha(static_cast<unsigned char>(c))) hasAlpha = true;
	}
	return hasDigit && hasAlpha;
}

std::string AuthService::hashPassword(const std::string& value) {
	std::hash<std::string> hasher;
	const auto combined = value + std::string(kPepper);
	return hexFromSizeT(hasher(combined));
}

bool AuthService::verifyPassword(const std::string& password, const std::string& hash) {
	return hashPassword(password) == hash;
}

std::string AuthService::generateToken() {
	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<uint64_t> dist;
	std::stringstream ss;
	for (int i = 0; i < 4; ++i) {
		ss << std::hex << dist(gen);
	}
	return ss.str();
}

std::string AuthService::buildExpiryString(int hoursAhead) {
	using namespace std::chrono;
	const auto now = system_clock::now();
	const auto target = now + hours(hoursAhead);
	std::time_t t = system_clock::to_time_t(target);
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &t);
#else
	localtime_r(&t, &tm);
#endif
	std::stringstream ss;
	ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	return ss.str();
}


