# 双端桌面应用重构方案

本方案描述如何将现有餐厅点餐系统拆分为两个桌面应用（用户端、商家端），并沿用后端 REST API。

## 总体架构
- **后端**：沿用 C++ REST 服务，新增用户/商家账号、认证、订单与菜单维护接口（见 `backend/`）。
- **数据层**：SQLite 新增 `users`、`merchants`、`sessions`、扩展 `dishes`/`orders` 结构，支持账号登录和菜单维护。
- **桌面包装**：采用 Tauri/Electron 等跨平台壳，内嵌 Web UI 或前端 SPA；启动时自动拉起本地后端，并在退出时回收进程。

## 用户端（Customer App）
1. **启动流程**：桌面壳启动 → 检测/启动后端 → 打开用户前端 WebView。
2. **功能模块**：
   - 登录/注册页（8 位字母数字账号）。
   - 菜单页：展示分类、加购、下单。
   - 订单提交：调用 `/orders`，附带用户 Token。
   - 个人中心：`/me/orders` 展示所有订单，针对 `completed & !pickupNotified` 的订单触发“取餐提醒”，并调用 `/orders/{id}/pickup-ack` 进行确认。
3. **技术建议**：
   - UI：React/Vue/Svelte（任选）+ Vite，部署为静态资源供 Tauri/Electron 加载。
   - 状态管理：前端本地存储用户 Token，并在 API 请求中写入 `Authorization: Bearer <token>`。
   - 桌面壳：Tauri（体积小）或 Electron（生态成熟），封装启动脚本与自动更新。

## 商家端（Merchant App）
1. **启动流程**：与用户端相同，但 UI 入口直达商家后台。
2. **功能模块**：
   - 登录/注册页（账号+密码+门店名）。
   - 订单管理：调用 `/admin/orders`、`/admin/orders/{id}/status` 进行分类展示和状态更新。
   - 菜单维护：使用 `/admin/menu`（GET/POST/PATCH）增删改菜品、调整价格及上下架状态。
3. **技术建议**：
   - UI 依旧可使用 Web 技术（如 React Admin / Ant Design）。
   - 桌面壳与用户端共用一套基础设施，但发布两个可执行文件，便于分发给不同角色。

## 项目结构建议
```
apps/
  user-desktop/
    frontend/   # React/Vite 等
    src-tauri/  # 或 electron 主进程
  merchant-desktop/
    frontend/
    src-tauri/
```
- `apps/*/README.md` 描述构建/打包流程。
- CI 中分别构建两个产物，生成安装包或绿色版。

## 后续工作清单
1. **后端**（进行中）
   - 完成用户/商家注册、登录、菜单维护接口（本次提交）。
   - 增补自动化测试、更多状态校验。
2. **前端重建**
   - 为用户端、商家端分别创建 SPA（可基于前端现有模板转化）。
   - 接入新 API、实现 Token 管理与页面路由。
3. **桌面壳搭建**
   - 选定 Tauri/Electron，搭建启动/停止后端的脚本。
   - 实现单实例、托盘、自动更新（可选）。
4. **打包分发**
   - Windows：使用 `tauri build` 或 `electron-builder` 输出 `.msi`/`.exe`。
   - 配置环境变量/配置文件，允许用户自定义数据库位置、端口等。

以上各阶段可逐步合入主干，确保每次重构都有可运行的端到端链路。


