#ifndef MENU_CONTROLLER_H
#define MENU_CONTROLLER_H

#include <httplib.h>
#include "../services/MenuService.h"

void registerMenuRoutes(httplib::Server& server, MenuService& menuService);

#endif // MENU_CONTROLLER_H


