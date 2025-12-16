#!/usr/bin/env python
"""
启动脚本：自动启动后端服务，再启动商家版 Flask 应用（默认端口 8090）
"""
import subprocess
import sys
import os
import time
import signal
import requests
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.parent
BACKEND_DIR = PROJECT_ROOT / "backend"
BACKEND_EXE = BACKEND_DIR / "build" / "Release" / "restaurant_backend.exe"

BACKEND_HOST = os.environ.get("BACKEND_HOST", "127.0.0.1")
BACKEND_PORT = int(os.environ.get("BACKEND_PORT", "8081"))
BACKEND_URL = f"http://{BACKEND_HOST}:{BACKEND_PORT}"

backend_process = None


def check_backend_running():
	try:
		resp = requests.get(f"{BACKEND_URL}/health", timeout=2)
		return resp.status_code == 200
	except Exception:
		return False


def start_backend():
	global backend_process
	if check_backend_running():
		print(f"✅ 后端服务已在运行 ({BACKEND_URL})")
		return None

	if not BACKEND_EXE.exists():
		print(f"❌ 找不到后端可执行文件: {BACKEND_EXE}")
		print("   请先编译后端: cd backend && cmake --build build --config Release")
		input("按 Enter 键继续（前端可能报错）或 Ctrl+C 取消...")
		return None

	print("🚀 启动后端服务...")
	env = os.environ.copy()
	env["BACKEND_HOST"] = BACKEND_HOST
	env["BACKEND_PORT"] = str(BACKEND_PORT)
	env["DB_PATH"] = env.get("DB_PATH", str(PROJECT_ROOT / "restaurant.db"))

	try:
		if sys.platform == "win32":
			backend_process = subprocess.Popen(
				[str(BACKEND_EXE)],
				cwd=str(BACKEND_DIR),
				env=env,
				creationflags=subprocess.CREATE_NEW_CONSOLE
			)
		else:
			backend_process = subprocess.Popen(
				[str(BACKEND_EXE)],
				cwd=str(BACKEND_DIR),
				env=env,
				stdout=subprocess.PIPE,
				stderr=subprocess.PIPE
			)

		for _ in range(20):
			time.sleep(0.5)
			if check_backend_running():
				print(f"✅ 后端服务启动成功 ({BACKEND_URL})")
				return backend_process
			if backend_process.poll() is not None:
				print("❌ 后端服务启动失败")
				input("\n按 Enter 键继续（前端可能报错）或 Ctrl+C 取消...")
				return None
		print("⚠️ 后端启动超时，请检查 8081 端口或数据库配置")
		return backend_process
	except Exception as e:
		print(f"❌ 启动后端失败: {e}")
		input("按 Enter 键继续（前端可能报错）或 Ctrl+C 取消...")
		return None


def stop_backend():
	global backend_process
	if backend_process:
		print("\n🛑 停止后端服务...")
		try:
			if sys.platform == "win32":
				backend_process.terminate()
			else:
				backend_process.send_signal(signal.SIGTERM)
			backend_process.wait(timeout=5)
			print("✅ 后端服务已停止")
		except Exception:
			try:
				backend_process.kill()
			except Exception:
				pass
		backend_process = None


def main():
	global backend_process

	def signal_handler(sig, frame):
		stop_backend()
		sys.exit(0)

	signal.signal(signal.SIGINT, signal_handler)
	signal.signal(signal.SIGTERM, signal_handler)

	backend_process = start_backend()

	print("\n🚀 启动商家前端 Flask 应用...")
	print("=" * 50)
	try:
		from app import create_app
		from config import Config

		app = create_app()
		print(f"前端地址: http://{Config.FRONTEND_HOST}:{Config.FRONTEND_PORT}")
		print(f"后端地址: {Config.BACKEND_BASE_URL}")
		print("=" * 50)
		app.run(host=Config.FRONTEND_HOST, port=Config.FRONTEND_PORT, debug=True)
	except KeyboardInterrupt:
		print("\n\n用户中断")
	except Exception as e:
		print(f"\n❌ 前端启动失败: {e}")
		import traceback
		traceback.print_exc()
	finally:
		stop_backend()


if __name__ == "__main__":
	main()

