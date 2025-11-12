#include "OrderController.h"
#include <nlohmann/json.hpp>
#include "../models/Order.h"

using json = nlohmann::json;

static std::vector<OrderItem> parseItems(const json& body) {
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

void registerOrderRoutes(httplib::Server& server, OrderService& orderService) {
	server.Post("/orders", [&](const httplib::Request& req, httplib::Response& res) {
		try {
			auto body = json::parse(req.body);
			auto items = parseItems(body);
			if (items.empty()) {
				res.status = 400;
				res.set_content(R"({"error":"items array required"})", "application/json");
				return;
			}
			std::string err;
			auto idOpt = orderService.createOrder(items, err);
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
		const auto& o = ord.value();
		json items = json::array();
		for (const auto& it : o.items) {
			items.push_back({{"dishId", it.dishId}, {"quantity", it.quantity}});
		}
		json resp{{"id", o.id}, {"status", o.status}, {"total", o.total}, {"items", items}};
		res.set_content(resp.dump(), "application/json");
	});
}


