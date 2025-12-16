#ifndef ORDER_CONTROLLER_H
#define ORDER_CONTROLLER_H

#include <httplib.h>
#include "../services/OrderService.h"
#include "../services/AuthService.h"

void registerOrderRoutes(httplib::Server& server, OrderService& orderService, AuthService& authService);

#endif // ORDER_CONTROLLER_H


