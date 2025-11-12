#include "AdminController.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void registerAdminRoutes(httplib::Server& server, OrderService& orderService) {
	// Get all orders
	server.Get("/admin/orders", [&](const httplib::Request&, httplib::Response& res) {
		std::string err;
		auto orders = orderService.getAllOrders(err);
		if (!err.empty()) {
			res.status = 500;
			res.set_content(json({{"error", err}}).dump(), "application/json");
			return;
		}
		
		json arr = json::array();
		for (const auto& o : orders) {
			json items = json::array();
			for (const auto& it : o.items) {
				items.push_back({{"dishId", it.dishId}, {"quantity", it.quantity}});
			}
			arr.push_back({
				{"id", o.id},
				{"status", o.status},
				{"total", o.total},
				{"items", items}
			});
		}
		res.set_content(arr.dump(), "application/json");
	});

	// Update order status
	server.Patch(R"(/admin/orders/(\d+)/status)", [&](const httplib::Request& req, httplib::Response& res) {
		try {
			int id = std::stoi(req.matches[1]);
			auto body = json::parse(req.body);
			
			if (!body.contains("status") || !body["status"].is_string()) {
				res.status = 400;
				res.set_content(R"({"error":"status field required"})", "application/json");
				return;
			}
			
			std::string status = body["status"].get<std::string>();
			// Validate status
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
			
			// Return updated order
			auto order = orderService.getOrder(id, err);
			if (!order.has_value()) {
				res.status = 404;
				res.set_content(R"({"error":"order not found"})", "application/json");
				return;
			}
			
			const auto& o = order.value();
			json items = json::array();
			for (const auto& it : o.items) {
				items.push_back({{"dishId", it.dishId}, {"quantity", it.quantity}});
			}
			json resp{{"id", o.id}, {"status", o.status}, {"total", o.total}, {"items", items}};
			res.set_content(resp.dump(), "application/json");
		} catch (const std::exception& e) {
			res.status = 400;
			res.set_content(json({{"error", std::string("invalid json: ") + e.what()}}).dump(), "application/json");
		}
	});
}

