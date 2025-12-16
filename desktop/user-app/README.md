# Customer Desktop App

Electron 壳，用于将用户端体验（登录、菜单、下单、个人中心）打包成桌面应用。启动后会：

1. 通过 `frontend/start_with_backend.py` 自动拉起 C++ 后端和 Flask 前端。
2. 等待 `http://127.0.0.1:8080/` 可访问。
3. 在桌面窗口中加载用户界面。

## 开发/运行
```powershell
cd desktop/user-app
npm install
npm start
```

可通过环境变量覆盖默认配置：

| 变量 | 默认值 | 说明 |
| --- | --- | --- |
| `PYTHON` | `py`(Win) / `python3`(Unix) | 指定 Python 解释器 |
| `BACKEND_HOST` | `127.0.0.1` | C++ 后端绑定地址 |
| `BACKEND_PORT` | `8081` | C++ 后端端口 |
| `FRONTEND_HOST` | `127.0.0.1` | Flask 服务地址 |
| `FRONTEND_PORT` | `8080` | Flask 服务端口 |
| `USER_ENTRY_URL` | `http://127.0.0.1:8080/` | Electron 窗口加载入口 |
| `DB_PATH` | `<repo>/restaurant.db` | SQLite 数据文件 |

> 商家端已经迁移到端口 8090，因此可以同时运行两个桌面应用；若需要调整端口，可在启动前覆盖 `FRONTEND_PORT` 环境变量。

## 后续计划
- 用 React/Vite 重写用户端 UI，并打包成静态资源嵌入 Electron，而不是依赖 Flask。
- 集成自动更新、系统通知（取餐提醒）等桌面能力。
- 发布 `.exe` 安装包（使用 `electron-builder` 或 `squirrel`）。


