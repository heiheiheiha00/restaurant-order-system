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

## 前端（Flask）
目录：`frontend/`

安装依赖：
```powershell
cd frontend
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
```

运行前端服务（默认 127.0.0.1:8080）：
```powershell
set BACKEND_BASE_URL=http://127.0.0.1:8081
set FRONTEND_HOST=127.0.0.1
set FRONTEND_PORT=8080
set SECRET_KEY=change-me
python app.py
```

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

2. **启动前端服务**（两种方式）：

   **方式1：自动启动后端（推荐）**：
   ```powershell
   cd frontend
   set BACKEND_BASE_URL=http://127.0.0.1:8081
   set FRONTEND_HOST=127.0.0.1
   set FRONTEND_PORT=8080
   set SECRET_KEY=change-me
   python start_with_backend.py
   ```
   这个脚本会自动启动后端服务，然后启动前端。停止前端时也会自动停止后端。

   **方式2：手动启动后端后，再启动前端**：
   ```powershell
   cd frontend
   set BACKEND_BASE_URL=http://127.0.0.1:8081
   set FRONTEND_HOST=127.0.0.1
   set FRONTEND_PORT=8080
   set SECRET_KEY=change-me
   python app.py
   ```
   看到 `Running on http://127.0.0.1:8080` 表示启动成功。

### 客户端使用（顾客）

访问地址：`http://127.0.0.1:8080`

功能页面：
- **首页** (`/`)：浏览菜单，将菜品加入购物车
- **购物车** (`/cart`)：查看购物车、调整数量、清空或提交订单
- **订单状态** (`/order/<id>`)：查看已提交订单的状态和详情

使用流程：
1. 打开浏览器访问 `http://127.0.0.1:8080`
2. 浏览菜单，点击"加入购物车"
3. 进入购物车页面，确认订单后点击"提交订单"
4. 查看订单状态页面，了解订单处理进度

### 商家端使用（管理员）

**重要：请访问前端地址，不是后端地址！**

访问地址：`http://127.0.0.1:8080/admin/orders` ✅（前端地址，端口 8080）

❌ 错误：`http://127.0.0.1:8081/admin/orders`（这是后端地址，不能直接访问）

功能页面：
- **订单管理** (`/admin/orders`)：查看所有订单，按状态分类显示，更新订单状态

使用流程：
1. **确保后端和前端都已启动**（见上方"完整启动流程"）
2. 打开浏览器访问 `http://127.0.0.1:8080/admin/orders`（注意是 8080 端口）
3. 或从导航栏点击"商家后台"链接
4. 查看订单列表，按状态分类（待处理、制作中、待取餐、已完成）
5. 点击订单卡片上的操作按钮更新订单状态：
   - **待处理** → 点击"开始制作" → 状态变为"制作中"
   - **制作中** → 点击"制作完成" → 状态变为"待取餐"
   - **待取餐** → 点击"完成订单" → 状态变为"已完成"
6. 页面每 10 秒自动刷新，实时显示最新订单

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

**提示**：商家端和客户端使用同一个前端服务，通过不同的 URL 路径访问不同功能。

## API 接口

### 顾客端
- `GET /menu` - 获取菜单
- `POST /orders` - 创建订单
- `GET /orders/{id}` - 查询订单详情

### 商家端
- `GET /admin/orders` - 获取所有订单列表
- `PATCH /admin/orders/{id}/status` - 更新订单状态

订单状态流转：`pending` → `preparing` → `ready` → `completed`

## 开发路线
当前版本已完成：
- 后端：SQLite 数据库接入、控制器/服务层、顾客/商家 REST API
- 前端：菜单、购物车、下单流程、订单状态页面、商家后台订单管理

下一步可选增强方向：
- 订单筛选和搜索功能
- 菜品管理后台（增删改查）
- 实时订单通知（WebSocket）
- 订单统计和报表


