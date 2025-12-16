// Minimal HTTP server for restaurant-order-system backend
// Uses cpp-httplib (header-only). For now returns in-memory menu and simple order creation.
#include <string>
#include <nlohmann/json.hpp>
#include "config.h"
#include <httplib.h>
#include "controllers/MenuController.h"
#include "controllers/OrderController.h"
#include "controllers/AdminController.h"
#include "controllers/AuthController.h"
#include "services/MenuService.h"
#include "services/OrderService.h"
#include "services/AuthService.h"
#include "database/Database.h"
#include "models/Dish.h"
#include "models/Order.h"
using json = nlohmann::json;



int main() {
	httplib::Server server;

	// Open database
	const char* dbPathEnv = std::getenv("DB_PATH");
	const std::string dbPath = dbPathEnv ? std::string(dbPathEnv) : std::string("restaurant.db");
	printf("Opening database at: %s\n", dbPath.c_str());
	std::string dbErr;
	if (!Database::instance().open(dbPath, dbErr)) {
		printf("Failed to open DB at %s: %s\n", dbPath.c_str(), dbErr.c_str());
		return 1;
	}
	printf("Database opened and initialized successfully\n");

	server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
		json j;
		j["status"] = "ok";
		j["service"] = "restaurant-backend";
		res.set_content(j.dump(), "application/json");
	});

	// Register routes via controllers/services
	MenuService menuService;
	OrderService orderService;
	AuthService authService;
	registerAuthRoutes(server, authService);
	registerMenuRoutes(server, menuService);
	registerOrderRoutes(server, orderService, authService);
	registerAdminRoutes(server, orderService, menuService, authService);

	// 404 handler
	server.set_error_handler([](const httplib::Request& req, httplib::Response& res) {
		json j;
		j["error"] = "Not Found";
		j["path"] = req.path;
		res.status = 404;
		res.set_content(j.dump(), "application/json");
	});

	// Config
	const auto host = get_server_host();
	const int port = get_server_port();
	printf("Starting backend at http://%s:%d\n", host.c_str(), port);
	server.listen(host.c_str(), port);
	return 0;
}


