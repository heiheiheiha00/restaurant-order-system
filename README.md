# Restaurant Order System

简单的餐厅点餐系统，包含：
- 后端 C++ REST API（cpp-httplib + nlohmann-json + SQLite3）
- 前端 Python Flask（菜单展示、购物车、下单、订单状态）
- SQLite 数据库 schema 与初始化数据脚本

## 环境要求
- Windows 10/11（PowerShell）
- CMake 3.20+，C++17 编译器（MSVC/MinGW）
- Python 3.10+
- vcpkg（用于拉取 `nlohmann-json`、`cpp-httplib`、`sqlite3`）

## 数据库
SQLite 表结构与初始化数据在 `db/`：
- `db/schema.sql`
- `db/init_data.sql`

你可以用任意 SQLite 工具创建数据库文件：
```bash
sqlite3 restaurant.db < db/schema.sql
sqlite3 restaurant.db < db/init_data.sql
```

或使用仓库脚本一键创建（推荐）：
```powershell
# 进入项目根目录后，调用你在 PyCharm 里使用的 Python 解释器
# 例：E:\path\to\your\venv\Scripts\python.exe scripts\create_db.py
python scripts/create_db.py
```

## 后端（C++）
目录：`backend/`

依赖通过 vcpkg 安装：
```powershell
# 安装 vcpkg（若未安装）
git clone https://github.com/microsoft/vcpkg.git $env:USERPROFILE\vcpkg
$env:VCPKG_ROOT="$env:USERPROFILE\vcpkg"
& $env:VCPKG_ROOT\bootstrap-vcpkg.bat

# 安装依赖
& $env:VCPKG_ROOT\vcpkg.exe install nlohmann-json cpp-httplib sqlite3
```

配置与构建：
```powershell
cd backend
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

运行（默认 127.0.0.1:8081）：
```powershell
set BACKEND_HOST=127.0.0.1
set BACKEND_PORT=8081
set DB_PATH=E:\restaurant-order-system\restaurant.db  # 可省略，默认为当前目录 restaurant.db
.\build\Release\restaurant_backend.exe
```

测试接口（PowerShell）：
```powershell
# 注意：PowerShell 中 curl 是 Invoke-WebRequest 的别名，语法不同
# 推荐使用 curl.exe（Windows 10+ 自带）或 Invoke-WebRequest

# 方法1：使用 curl.exe（推荐，语法与 Linux 一致）
curl.exe http://127.0.0.1:8081/health
curl.exe http://127.0.0.1:8081/menu

# 方法2：使用 Invoke-WebRequest（PowerShell 原生）
Invoke-WebRequest -Uri http://127.0.0.1:8081/health | Select-Object -ExpandProperty Content
Invoke-WebRequest -Uri http://127.0.0.1:8081/menu | Select-Object -ExpandProperty Content

# POST 请求（创建订单）
$body = @{items=@(@{dishId=1;quantity=2})} | ConvertTo-Json
Invoke-WebRequest -Uri http://127.0.0.1:8081/orders -Method POST -ContentType "application/json" -Body $body | Select-Object -ExpandProperty Content
```

后端默认使用 `DB_PATH` 指定的 SQLite 文件（缺省为当前目录 `restaurant.db`）。确保提前执行 `schema.sql` 与 `init_data.sql` 初始化数据。

### 新增接口（用户/商家分离）
- `POST /auth/user/register`：用户注册（8 位数字+字母账号），body：`username`、`password`、`phone`。
- `POST /auth/user/login`：用户登录，返回 `token`。
- `POST /auth/merchant/register`、`/auth/merchant/login`：商家注册/登录。
- `POST /orders`：用户下单，需携带 `Authorization: Bearer <token>`。
- `GET /me/orders`：用户个人中心订单列表，返回 `pickupReady` 字段提示待取餐订单。
- `POST /orders/{id}/pickup-ack`：用户确认收到取餐提醒。
- `GET /admin/orders`、`PATCH /admin/orders/{id}/status`：商家后台查看/更新订单，需商家 Token。
- `GET /admin/menu`：加载完整菜单（含上下架信息）。
- `POST /admin/menu`、`PATCH /admin/menu/{id}`：商家新增菜品或更新分类/价格/上下架。

> 所有需要登录的接口均使用 `Authorization: Bearer <token>`；Token 通过 `/auth/.../login` 获取，当前默认 24 小时有效。

完整的接口说明与角色区分见 `docs/desktop-app-plan.md`。

## 前端（Flask）

### 用户端（`frontend/`，默认端口 8080）
```powershell
cd frontend
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt

set BACKEND_BASE_URL=http://127.0.0.1:8081
set FRONTEND_HOST=127.0.0.1
set FRONTEND_PORT=8080
set SECRET_KEY=change-me
python app.py    # 或 python start_with_backend.py
```
主要页面：
- `/` 菜单 / `/cart` 购物车
- `/login` / `/register` 用户登录注册
- `/profile` 个人中心，查看订单、确认取餐
- `/order/<id>` 单个订单详情

### 商家端（`frontend_merchant/`，默认端口 8090）
```powershell
cd frontend_merchant
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt

set BACKEND_BASE_URL=http://127.0.0.1:8081
set FRONTEND_HOST=127.0.0.1
set FRONTEND_PORT=8090
set SECRET_KEY=change-me
python app.py    # 或 python start_with_backend.py
```
主要页面：
- `/merchant/login` / `/merchant/register` 商家认证
- `/admin/orders` 订单看板
- `/admin/menu/manage` 菜单维护（增改、上下架）

> 两个前端共享同一个后端和数据库，可同时运行；只需确保它们绑定不同端口（默认 8080 与 8090）。桌面应用也已分别指向对应端口。

## 启动和使用

### 完整启动流程

1. **启动后端服务**（必须首先启动）：
   ```powershell
   cd backend
   set BACKEND_HOST=127.0.0.1
   set BACKEND_PORT=8081
   set DB_PATH=restaurant.db
   .\build\Release\restaurant_backend.exe
   ```
   看到 `Starting backend at http://127.0.0.1:8081` 表示启动成功。

2. **启动前端服务**：

   - 用户端：
     ```powershell
     cd frontend
     python start_with_backend.py   # 或 python app.py（需先手动启动后端）
     ```
     访问 `http://127.0.0.1:8080`

   - 商家端：
     ```powershell
     cd frontend_merchant
     python start_with_backend.py   # 或 python app.py
     ```
     访问 `http://127.0.0.1:8090/admin/orders`

### 客户端使用（顾客）
1. 访问 `http://127.0.0.1:8080`
2. 注册/登录 → 浏览菜单 → 加入购物车 → 提交订单
3. 打开“个人中心”查看全部订单，完成后可点击“我已收到取餐提醒”

### 商家端使用
1. 访问 `http://127.0.0.1:8090/merchant/login` 注册/登录
2. 打开 `http://127.0.0.1:8090/admin/orders` 管理订单，支持状态流转（pending → preparing → ready → completed）
3. 进入 `http://127.0.0.1:8090/admin/menu/manage` 新增或编辑菜品、上下架

**常见问题排查**：

1. **显示"后端API错误"或"找不到URL"**：
   - ✅ 检查后端服务是否已启动：
     ```powershell
     # 测试后端健康检查
     curl.exe http://127.0.0.1:8081/health
     # 应该返回: {"status":"ok","service":"restaurant-backend"}
     ```
   - ✅ 检查后端是否包含商家API路由：
     ```powershell
     # 测试商家API
     curl.exe http://127.0.0.1:8081/admin/orders
     # 应该返回订单列表（可能是空数组 []）
     ```
   - ✅ 如果返回404，说明后端没有重新编译，需要：
     ```powershell
     cd backend
     cmake --build build --config Release
     # 然后重新启动后端服务
     ```
   - ✅ 检查前端配置：
     ```powershell
     # 确保环境变量设置正确
     echo $env:BACKEND_BASE_URL
     # 应该是: http://127.0.0.1:8081
     ```

2. **显示"无法连接到后端服务"**：
   - 确保后端服务正在运行（检查后端控制台是否有输出）
   - 检查防火墙是否阻止了8081端口
   - 确认BACKEND_BASE_URL环境变量正确

3. **页面显示正常但看不到订单**：
   - 这是正常的，如果没有订单会显示空列表
   - 可以先在客户端下单，然后刷新商家后台查看

**提示**：用户端与商家端已经拆分为两个独立的 Flask 服务（8080 / 8090），互不影响。

## API 接口

### 认证
- `POST /auth/user/register`、`POST /auth/user/login`
- `POST /auth/merchant/register`、`POST /auth/merchant/login`

### 用户端
- `GET /menu`：只返回上架菜品。
- `POST /orders`：创建订单（需用户 Token）。
- `GET /orders/{id}`：查看订单详情（需用户/商家 Token，用户仅能查自己的单）。
- `GET /me/orders`：个人中心订单列表，附带 `pickupReady` 字段。
- `POST /orders/{id}/pickup-ack`：确认已收到取餐提醒。

### 商家端
- `GET /admin/orders`：查看全部订单。
- `PATCH /admin/orders/{id}/status`：更新状态（`pending → preparing → ready → completed`）。
- `GET /admin/menu`：获取完整菜单（含未上架菜品）。
- `POST /admin/menu`：新增菜品（含分类、描述、价格、上架状态）。
- `PATCH /admin/menu/{id}`：更新名称、分类、价格或上下架。



## 开发路线
当前版本已完成：
- 后端：SQLite 数据库接入、控制器/服务层、顾客/商家 REST API
- 前端：菜单、购物车、下单流程、订单状态页面、商家后台订单管理

下一步可选增强方向：
- 订单筛选和搜索功能
- 菜品管理后台（增删改查）
- 实时订单通知（WebSocket）
- 订单统计和报表


