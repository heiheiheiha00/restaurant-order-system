#ifndef ORDER_CONTROLLER_H
#define ORDER_CONTROLLER_H

#include <httplib.h>
#include "../services/OrderService.h"

void registerOrderRoutes(httplib::Server& server, OrderService& orderService);

#endif // ORDER_CONTROLLER_H


