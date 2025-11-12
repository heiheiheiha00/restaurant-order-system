#ifndef ADMIN_CONTROLLER_H
#define ADMIN_CONTROLLER_H

#include <httplib.h>
#include "../services/OrderService.h"

void registerAdminRoutes(httplib::Server& server, OrderService& orderService);

#endif // ADMIN_CONTROLLER_H

