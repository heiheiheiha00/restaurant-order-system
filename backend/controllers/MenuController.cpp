#include "MenuController.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void registerMenuRoutes(httplib::Server& server, MenuService& menuService) {
	server.Get("/menu", [&](const httplib::Request&, httplib::Response& res) {
		std::string err;
		auto menu = menuService.getMenu(err);
		if (!err.empty()) {
			res.status = 500;
			res.set_content(json({{"error", err}}).dump(), "application/json");
			return;
		}
		json arr = json::array();
		for (const auto& d : menu) {
			if (!d.isAvailable) continue;
			arr.push_back({
				{"id", d.id},
				{"name", d.name},
				{"description", d.description},
				{"category", d.category},
				{"price", d.price}
			});
		}
		res.set_content(arr.dump(), "application/json");
	});
}


