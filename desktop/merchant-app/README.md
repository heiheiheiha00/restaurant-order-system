# Merchant Desktop App

Electron 壳，用于打包商家后台（订单管理 + 菜单维护）：

1. 启动 `frontend_merchant/start_with_backend.py`，自动拉起 C++ 后端与商家版 Flask 前端。
2. 等待 `http://127.0.0.1:8090/admin/orders` 可访问。
3. 以内嵌窗口展示商家后台。

## 开发/运行
```powershell
cd desktop/merchant-app
npm install
npm start
```

环境变量：

| 变量 | 默认值 | 说明 |
| --- | --- | --- |
| `PYTHON` | `py`(Win)/`python3`(Unix) | Python 解释器 |
| `BACKEND_HOST` / `BACKEND_PORT` | `127.0.0.1` / `8081` | C++ 后端监听 |
| `FRONTEND_HOST` / `FRONTEND_PORT` | `127.0.0.1` / `8090` | 商家端 Flask 端口 |
| `MERCHANT_ENTRY_URL` | `http://127.0.0.1:8090/admin/orders` | Electron 加载入口 |
| `DB_PATH` | `<repo>/restaurant.db` | SQLite 文件 |

> 现在用户端（8080）与商家端（8090）已拆分成两个端口，可同时运行。

## 后续计划
- 商家端 UI 独立化（React Admin 或类似方案），减少对 Flask 模板的依赖。
- 增加菜单编辑页面、表单校验、批量导入等高级功能。
- 使用 `electron-builder` 打包成 MSI/EXE，并集成自动更新。


