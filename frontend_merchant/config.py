import os


class Config:
	BACKEND_BASE_URL = os.environ.get("BACKEND_BASE_URL", "http://127.0.0.1:8081")
	FRONTEND_HOST = os.environ.get("FRONTEND_HOST", "127.0.0.1")
	FRONTEND_PORT = int(os.environ.get("FRONTEND_PORT", "8090"))
	SECRET_KEY = os.environ.get("SECRET_KEY", "merchant-secret-key")


