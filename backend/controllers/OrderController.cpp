#include "OrderController.h"
#include <nlohmann/json.hpp>
#include "../models/Order.h"

using json = nlohmann::json;

namespace {
	std::optional<std::string> extractToken(const httplib::Request& req) {
		const auto it = req.headers.find("Authorization");
		if (it == req.headers.end()) return std::nullopt;
		const auto& value = it->second;
		const std::string prefix = "Bearer ";
		if (value.rfind(prefix, 0) != 0) return std::nullopt;
		return value.substr(prefix.size());
	}

	std::optional<User> requireUser(const httplib::Request& req, httplib::Response& res, AuthService& authService) {
		auto token = extractToken(req);
		if (!token.has_value()) {
			res.status = 401;
			res.set_content(R"({"error":"missing bearer token"})", "application/json");
			return std::nullopt;
		}
		std::string err;
		auto user = authService.authenticateUser(token.value(), err);
		if (!user.has_value()) {
			res.status = 401;
			res.set_content(json({{"error", err.empty() ? "invalid token" : err}}).dump(), "application/json");
			return std::nullopt;
		}
		return user;
	}

	std::optional<Merchant> tryMerchant(const httplib::Request& req, AuthService& authService, std::string& err) {
		auto token = extractToken(req);
		if (!token.has_value()) {
			err = "missing bearer token";
			return std::nullopt;
		}
		return authService.authenticateMerchant(token.value(), err);
	}

	std::vector<OrderItem> parseItems(const json& body) {
		std::vector<OrderItem> items;
		if (!body.contains("items") || !body["items"].is_array()) return items;
		for (const auto& it : body["items"]) {
			if (!it.contains("dishId") || !it.contains("quantity")) continue;
			OrderItem oi{};
			oi.dishId = it["dishId"].get<int>();
			oi.quantity = it["quantity"].get<int>();
			if (oi.quantity > 0) items.push_back(oi);
		}
		return items;
	}

	json serializeOrder(const Order& o) {
		json items = json::array();
		for (const auto& it : o.items) {
			items.push_back({
				{"dishId", it.dishId},
				{"quantity", it.quantity},
				{"unitPrice", it.unitPrice}
			});
		}
		json obj{
			{"id", o.id},
			{"status", o.status},
			{"total", o.total},
			{"items", items},
			{"pickupNotified", o.pickupNotified},
			{"createdAt", o.createdAt},
			{"updatedAt", o.updatedAt}
		};
		if (o.userId.has_value()) {
			obj["userId"] = o.userId.value();
		}
		return obj;
	}
}

void registerOrderRoutes(httplib::Server& server, OrderService& orderService, AuthService& authService) {
	server.Post("/orders", [&](const httplib::Request& req, httplib::Response& res) {
		try {
			auto user = requireUser(req, res, authService);
			if (!user.has_value()) return;

			auto body = json::parse(req.body);
			auto items = parseItems(body);
			if (items.empty()) {
				res.status = 400;
				res.set_content(R"({"error":"items array required"})", "application/json");
				return;
			}
			std::string err;
			auto idOpt = orderService.createOrder(items, user->id, err);
			if (!err.empty()) {
				res.status = 500;
				res.set_content(json({{"error", err}}).dump(), "application/json");
				return;
			}
			if (!idOpt.has_value()) {
				res.status = 400;
				res.set_content(R"({"error":"invalid order"})", "application/json");
				return;
			}
			json resp{{"id", idOpt.value()}};
			res.status = 201;
			res.set_content(resp.dump(), "application/json");
		} catch (const std::exception& e) {
			res.status = 400;
			res.set_content(json({{"error", std::string("invalid json: ") + e.what()}}).dump(), "application/json");
		}
	});

	server.Get("/me/orders", [&](const httplib::Request& req, httplib::Response& res) {
		auto user = requireUser(req, res, authService);
		if (!user.has_value()) return;
		std::string err;
		auto orders = orderService.getOrdersByUser(user->id, err);
		if (!err.empty()) {
			res.status = 500;
			res.set_content(json({{"error", err}}).dump(), "application/json");
			return;
		}
		json arr = json::array();
		for (const auto& o : orders) {
			auto data = serializeOrder(o);
			const bool pickupReady = o.status == "completed" && !o.pickupNotified;
			data["pickupReady"] = pickupReady;
			arr.push_back(data);
		}
		res.set_content(arr.dump(), "application/json");
	});

	server.Get(R"(/orders/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
		int id = std::stoi(req.matches[1]);
		std::string err;
		auto ord = orderService.getOrder(id, err);
		if (!err.empty()) {
			res.status = 500;
			res.set_content(json({{"error", err}}).dump(), "application/json");
			return;
		}
		if (!ord.has_value()) {
			res.status = 404;
			res.set_content(R"({"error":"order not found"})", "application/json");
			return;
		}

		const auto token = extractToken(req);
		if (!token.has_value()) {
			res.status = 401;
			res.set_content(R"({"error":"missing bearer token"})", "application/json");
			return;
		}
		auto user = authService.authenticateUser(token.value(), err);
		if (!user.has_value()) {
			err.clear();
			auto merchant = authService.authenticateMerchant(token.value(), err);
			if (!merchant.has_value()) {
				res.status = 401;
				res.set_content(json({{"error", err.empty() ? "invalid token" : err}}).dump(), "application/json");
				return;
			}
		} else {
			if (ord->userId.has_value() && ord->userId.value() != user->id) {
				res.status = 403;
				res.set_content(R"({"error":"order does not belong to you"})", "application/json");
				return;
			}
		}

		auto payload = serializeOrder(ord.value());
		res.set_content(payload.dump(), "application/json");
	});

	server.Post(R"(/orders/(\d+)/pickup-ack)", [&](const httplib::Request& req, httplib::Response& res) {
		auto user = requireUser(req, res, authService);
		if (!user.has_value()) return;
		const int id = std::stoi(req.matches[1]);
		std::string err;
		auto ord = orderService.getOrder(id, err);
		if (!err.empty()) {
			res.status = 500;
			res.set_content(json({{"error", err}}).dump(), "application/json");
			return;
		}
		if (!ord.has_value()) {
			res.status = 404;
			res.set_content(R"({"error":"order not found"})", "application/json");
			return;
		}
		if (!ord->userId.has_value() || ord->userId.value() != user->id) {
			res.status = 403;
			res.set_content(R"({"error":"order does not belong to you"})", "application/json");
			return;
		}
		if (!orderService.markPickupNotified(id, err)) {
			res.status = 500;
			res.set_content(json({{"error", err}}).dump(), "application/json");
			return;
		}
		res.set_content(R"({"message":"acknowledged"})", "application/json");
	});
}


