#include "AuthController.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
	void respondError(httplib::Response& res, int status, const std::string& message) {
		res.status = status;
		res.set_content(json({{"error", message}}).dump(), "application/json");
	}

	std::string getStringField(const json& body, const std::string& key) {
		if (!body.contains(key) || !body[key].is_string()) {
			return "";
		}
		return body[key].get<std::string>();
	}
}

void registerAuthRoutes(httplib::Server& server, AuthService& authService) {
	server.Post("/auth/user/register", [&](const httplib::Request& req, httplib::Response& res) {
		try {
			const auto body = json::parse(req.body);
			const auto username = getStringField(body, "username");
			const auto password = getStringField(body, "password");
			const auto phone = getStringField(body, "phone");
			if (username.empty() || password.empty()) {
				respondError(res, 400, "username/password required");
				return;
			}
			std::string err;
			if (!authService.registerUser(username, password, phone, err)) {
				respondError(res, 400, err.empty() ? "failed to register" : err);
				return;
			}
			res.status = 201;
			res.set_content(json({{"message", "registered"}}).dump(), "application/json");
		} catch (const std::exception& e) {
			respondError(res, 400, std::string("invalid json: ") + e.what());
		}
	});

	server.Post("/auth/user/login", [&](const httplib::Request& req, httplib::Response& res) {
		try {
			const auto body = json::parse(req.body);
			const auto username = getStringField(body, "username");
			const auto password = getStringField(body, "password");
			if (username.empty() || password.empty()) {
				respondError(res, 400, "username/password required");
				return;
			}
			std::string err;
			auto token = authService.loginUser(username, password, err);
			if (!token.has_value()) {
				respondError(res, 401, err.empty() ? "登录失败" : err);
				return;
			}
			res.set_content(json({
				{"token", token->token},
				{"userId", token->subjectId},
				{"username", username}
			}).dump(), "application/json");
		} catch (const std::exception& e) {
			respondError(res, 400, std::string("invalid json: ") + e.what());
		}
	});

	server.Post("/auth/merchant/register", [&](const httplib::Request& req, httplib::Response& res) {
		try {
			const auto body = json::parse(req.body);
			const auto username = getStringField(body, "username");
			const auto password = getStringField(body, "password");
			const auto storeName = getStringField(body, "storeName");
			if (username.empty() || password.empty()) {
				respondError(res, 400, "username/password required");
				return;
			}
			std::string err;
			if (!authService.registerMerchant(username, password, storeName, err)) {
				respondError(res, 400, err.empty() ? "failed to register merchant" : err);
				return;
			}
			res.status = 201;
			res.set_content(json({{"message", "registered"}}).dump(), "application/json");
		} catch (const std::exception& e) {
			respondError(res, 400, std::string("invalid json: ") + e.what());
		}
	});

	server.Post("/auth/merchant/login", [&](const httplib::Request& req, httplib::Response& res) {
		try {
			const auto body = json::parse(req.body);
			const auto username = getStringField(body, "username");
			const auto password = getStringField(body, "password");
			if (username.empty() || password.empty()) {
				respondError(res, 400, "username/password required");
				return;
			}
			std::string err;
			auto token = authService.loginMerchant(username, password, err);
			if (!token.has_value()) {
				respondError(res, 401, err.empty() ? "登录失败" : err);
				return;
			}
			res.set_content(json({
				{"token", token->token},
				{"merchantId", token->subjectId},
				{"username", username}
			}).dump(), "application/json");
		} catch (const std::exception& e) {
			respondError(res, 400, std::string("invalid json: ") + e.what());
		}
	});
}


