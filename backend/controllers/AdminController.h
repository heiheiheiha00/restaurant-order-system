#ifndef ADMIN_CONTROLLER_H
#define ADMIN_CONTROLLER_H

#include <httplib.h>
#include "../services/OrderService.h"
#include "../services/MenuService.h"
#include "../services/AuthService.h"

void registerAdminRoutes(httplib::Server& server, OrderService& orderService, MenuService& menuService, AuthService& authService);

#endif // ADMIN_CONTROLLER_H

