#!/usr/bin/env python
"""
启动脚本：自动启动后端服务，然后启动前端Flask应用
"""
import subprocess
import sys
import os
import time
import signal
import requests
from pathlib import Path

# 项目根目录
PROJECT_ROOT = Path(__file__).parent.parent
BACKEND_DIR = PROJECT_ROOT / "backend"
BACKEND_EXE = BACKEND_DIR / "build" / "Release" / "restaurant_backend.exe"

# 后端配置
BACKEND_HOST = os.environ.get("BACKEND_HOST", "127.0.0.1")
BACKEND_PORT = int(os.environ.get("BACKEND_PORT", "8081"))
BACKEND_URL = f"http://{BACKEND_HOST}:{BACKEND_PORT}"

# 存储后端进程
backend_process = None


def check_backend_running():
	"""检查后端是否已经在运行"""
	try:
		resp = requests.get(f"{BACKEND_URL}/health", timeout=2)
		return resp.status_code == 200
	except:
		return False


def start_backend():
	"""启动后端服务"""
	global backend_process
	
	# 检查后端是否已经在运行
	if check_backend_running():
		print(f"✅ 后端服务已在运行 ({BACKEND_URL})")
		return None
	
	# 检查后端可执行文件是否存在
	if not BACKEND_EXE.exists():
		print(f"❌ 后端可执行文件不存在: {BACKEND_EXE}")
		print("   请先编译后端:")
		print(f"   cd {BACKEND_DIR}")
		print("   cmake --build build --config Release")
		print("\n⚠️  无法启动后端，前端将无法正常工作！")
		input("按 Enter 键继续（前端会报错）或 Ctrl+C 取消...")
		return None
	
	print(f"🚀 启动后端服务...")
	print(f"   路径: {BACKEND_EXE}")
	print(f"   数据库: {os.environ.get('DB_PATH', str(PROJECT_ROOT / 'restaurant.db'))}")
	
	# 设置环境变量
	env = os.environ.copy()
	env["BACKEND_HOST"] = BACKEND_HOST
	env["BACKEND_PORT"] = str(BACKEND_PORT)
	env["DB_PATH"] = os.environ.get("DB_PATH", str(PROJECT_ROOT / "restaurant.db"))
	
	# 启动后端进程
	try:
		# Windows下使用新控制台窗口，这样可以看到后端日志
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
		
		# 等待后端启动
		print("   等待后端启动（最多10秒）...")
		for i in range(20):  # 增加到20次，每次0.5秒，总共10秒
			time.sleep(0.5)
			if check_backend_running():
				print(f"✅ 后端服务启动成功 ({BACKEND_URL})")
				return backend_process
			if backend_process.poll() is not None:
				# 进程已退出
				print(f"❌ 后端服务启动失败（进程已退出）")
				if sys.platform != "win32":
					stderr = backend_process.stderr.read().decode('utf-8', errors='ignore')
					if stderr:
						print(f"   错误信息: {stderr}")
				print("   请检查:")
				print("   1. 数据库文件是否存在")
				print("   2. 端口8081是否被占用")
				print("   3. 查看后端控制台窗口的错误信息")
				input("\n按 Enter 键继续（前端会报错）或 Ctrl+C 取消...")
				return None
		
		print("⚠️  后端服务启动超时（10秒内未响应）")
		print("   可能原因:")
		print("   1. 后端启动较慢，请稍等")
		print("   2. 后端启动失败，请查看后端控制台窗口")
		print("   3. 端口8081被占用")
		print("\n   继续启动前端，如果后端未启动，前端会报连接错误")
		return backend_process
		
	except Exception as e:
		print(f"❌ 启动后端失败: {e}")
		import traceback
		traceback.print_exc()
		input("\n按 Enter 键继续（前端会报错）或 Ctrl+C 取消...")
		return None


def stop_backend():
	"""停止后端服务"""
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
		except:
			try:
				backend_process.kill()
			except:
				pass
		backend_process = None


def main():
	"""主函数"""
	global backend_process
	
	# 注册退出处理
	def signal_handler(sig, frame):
		stop_backend()
		sys.exit(0)
	
	signal.signal(signal.SIGINT, signal_handler)
	signal.signal(signal.SIGTERM, signal_handler)
	
	# 启动后端
	backend_process = start_backend()
	
	# 再次检查后端是否运行
	if backend_process and not check_backend_running():
		print("\n⚠️  警告: 后端服务可能未成功启动")
		print("   前端可能会报连接错误")
		print("   如果看到连接错误，请:")
		print("   1. 检查后端控制台窗口的错误信息")
		print("   2. 手动启动后端: cd backend && .\\build\\Release\\restaurant_backend.exe")
		print()
	
	# 启动前端
	print("\n🚀 启动前端Flask应用...")
	print("=" * 50)
	
	try:
		# 导入并运行Flask应用
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

