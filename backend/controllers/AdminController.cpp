#include "AdminController.h"
#include <nlohmann/json.hpp>
#include <optional>

using json = nlohmann::json;

namespace {
	std::optional<Merchant> requireMerchant(const httplib::Request& req, httplib::Response& res, AuthService& authService) {
		const auto it = req.headers.find("Authorization");
		if (it == req.headers.end()) {
			res.status = 401;
			res.set_content(R"({"error":"missing bearer token"})", "application/json");
			return std::nullopt;
		}
		const std::string prefix = "Bearer ";
		if (it->second.rfind(prefix, 0) != 0) {
			res.status = 401;
			res.set_content(R"({"error":"invalid authorization header"})", "application/json");
			return std::nullopt;
		}
		std::string err;
		auto merchant = authService.authenticateMerchant(it->second.substr(prefix.size()), err);
		if (!merchant.has_value()) {
			res.status = 401;
			res.set_content(json({{"error", err.empty() ? "invalid token" : err}}).dump(), "application/json");
			return std::nullopt;
		}
		return merchant;
	}

	json serializeOrderBrief(const Order& o) {
		json items = json::array();
		for (const auto& it : o.items) {
			items.push_back({{"dishId", it.dishId}, {"quantity", it.quantity}, {"unitPrice", it.unitPrice}});
		}
		json result = {
			{"id", o.id},
			{"status", o.status},
			{"total", o.total},
			{"items", items},
			{"pickupNotified", o.pickupNotified},
			{"createdAt", o.createdAt},
			{"updatedAt", o.updatedAt}
		};
		if (o.userId.has_value()) {
			result["userId"] = o.userId.value();
		}
		return result;
	}

	json serializeDish(const Dish& d) {
		return json{
			{"id", d.id},
			{"name", d.name},
			{"description", d.description},
			{"category", d.category},
			{"price", d.price},
			{"isAvailable", d.isAvailable}
		};
	}
}

void registerAdminRoutes(httplib::Server& server, OrderService& orderService, MenuService& menuService, AuthService& authService) {
	server.Get("/admin/orders", [&](const httplib::Request& req, httplib::Response& res) {
		if (!requireMerchant(req, res, authService).has_value()) return;
		std::string err;
		auto orders = orderService.getAllOrders(err);
		if (!err.empty()) {
			res.status = 500;
			res.set_content(json({{"error", err}}).dump(), "application/json");
			return;
		}
		json arr = json::array();
		for (const auto& o : orders) {
			arr.push_back(serializeOrderBrief(o));
		}
		res.set_content(arr.dump(), "application/json");
	});

	server.Patch(R"(/admin/orders/(\d+)/status)", [&](const httplib::Request& req, httplib::Response& res) {
		if (!requireMerchant(req, res, authService).has_value()) return;
		try {
			const int id = std::stoi(req.matches[1]);
			const auto body = json::parse(req.body);
			if (!body.contains("status") || !body["status"].is_string()) {
				res.status = 400;
				res.set_content(R"({"error":"status field required"})", "application/json");
				return;
			}
			const std::string status = body["status"].get<std::string>();
			if (status != "pending" && status != "preparing" && status != "ready" && status != "completed") {
				res.status = 400;
				res.set_content(R"({"error":"invalid status"})", "application/json");
				return;
			}
			std::string err;
			if (!orderService.updateOrderStatus(id, status, err)) {
				res.status = 500;
				res.set_content(json({{"error", err}}).dump(), "application/json");
				return;
			}
			auto order = orderService.getOrder(id, err);
			if (!order.has_value()) {
				res.status = 404;
				res.set_content(R"({"error":"order not found"})", "application/json");
				return;
			}
			res.set_content(serializeOrderBrief(order.value()).dump(), "application/json");
		} catch (const std::exception& e) {
			res.status = 400;
			res.set_content(json({{"error", std::string("invalid json: ") + e.what()}}).dump(), "application/json");
		}
	});

	server.Get("/admin/menu", [&](const httplib::Request& req, httplib::Response& res) {
		if (!requireMerchant(req, res, authService).has_value()) return;
		std::string err;
		auto dishes = menuService.getMenu(err);
		if (!err.empty()) {
			res.status = 500;
			res.set_content(json({{"error", err}}).dump(), "application/json");
			return;
		}
		json arr = json::array();
		for (const auto& d : dishes) {
			arr.push_back(serializeDish(d));
		}
		res.set_content(arr.dump(), "application/json");
	});

	server.Post("/admin/menu", [&](const httplib::Request& req, httplib::Response& res) {
		if (!requireMerchant(req, res, authService).has_value()) return;
		try {
			const auto body = json::parse(req.body);
			Dish dish{};
			dish.name = body.value("name", "");
			dish.description = body.value("description", "");
			dish.category = body.value("category", "");
			dish.price = body.value("price", 0.0);
			dish.isAvailable = body.value("isAvailable", true);
			if (dish.name.empty() || dish.price <= 0) {
				res.status = 400;
				res.set_content(R"({"error":"name and positive price required"})", "application/json");
				return;
			}
			std::string err;
			auto createdId = menuService.createDish(dish, err);
			if (!createdId.has_value()) {
				res.status = 500;
				res.set_content(json({{"error", err}}).dump(), "application/json");
				return;
			}
			auto createdDish = menuService.getDish(createdId.value(), err);
			if (!createdDish.has_value()) {
				res.status = 500;
				res.set_content(R"({"error":"failed to load created dish"})", "application/json");
				return;
			}
			res.status = 201;
			res.set_content(serializeDish(createdDish.value()).dump(), "application/json");
		} catch (const std::exception& e) {
			res.status = 400;
			res.set_content(json({{"error", std::string("invalid json: ") + e.what()}}).dump(), "application/json");
		}
	});

	server.Patch(R"(/admin/menu/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
		if (!requireMerchant(req, res, authService).has_value()) return;
		try {
			const int id = std::stoi(req.matches[1]);
			const auto body = json::parse(req.body);
			std::optional<std::string> name;
			std::optional<std::string> description;
			std::optional<std::string> category;
			std::optional<double> price;
			std::optional<bool> isAvailable;

			if (body.contains("name") && body["name"].is_string()) name = body["name"].get<std::string>();
			if (body.contains("description") && body["description"].is_string()) description = body["description"].get<std::string>();
			if (body.contains("category") && body["category"].is_string()) category = body["category"].get<std::string>();
			if (body.contains("price") && body["price"].is_number()) price = body["price"].get<double>();
			if (body.contains("isAvailable") && body["isAvailable"].is_boolean()) isAvailable = body["isAvailable"].get<bool>();

			if (!name && !description && !category && !price && !isAvailable) {
				res.status = 400;
				res.set_content(R"({"error":"no fields to update"})", "application/json");
				return;
			}

			std::string err;
			if (!menuService.updateDish(id, name, description, category, price, isAvailable, err)) {
				res.status = 500;
				res.set_content(json({{"error", err}}).dump(), "application/json");
				return;
			}
			auto dish = menuService.getDish(id, err);
			if (!dish.has_value()) {
				res.status = 404;
				res.set_content(R"({"error":"dish not found"})", "application/json");
				return;
			}
			res.set_content(serializeDish(dish.value()).dump(), "application/json");
		} catch (const std::exception& e) {
			res.status = 400;
			res.set_content(json({{"error", std::string("invalid json: ") + e.what()}}).dump(), "application/json");
		}
	});
}

