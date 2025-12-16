#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include <httplib.h>
#include "../services/AuthService.h"

void registerAuthRoutes(httplib::Server& server, AuthService& authService);

#endif // AUTH_CONTROLLER_H


