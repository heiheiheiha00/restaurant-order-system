cmake_minimum_required(VERSION 3.20)
project(restaurant_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable MSVC UTF-8
if (MSVC)
	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>" "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

# vcpkg integration (optional)
if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

find_package(nlohmann_json REQUIRED)
find_package(httplib REQUIRED)
find_package(SQLite3 REQUIRED)

add_executable(restaurant_backend
		main.cpp
		config.cpp
		database/Database.cpp
		services/MenuService.cpp
		services/OrderService.cpp
		services/AuthService.cpp
		controllers/MenuController.cpp
		controllers/OrderController.cpp
		controllers/AdminController.cpp
		controllers/AuthController.cpp
)

target_link_libraries(restaurant_backend PRIVATE
		nlohmann_json::nlohmann_json
		httplib::httplib
		SQLite::SQLite3
)

# Windows: set console subsystem to avoid extra window
if (WIN32)
	set_target_properties(restaurant_backend PROPERTIES
		LINK_FLAGS "/SUBSYSTEM:CONSOLE"
	)
endif()


