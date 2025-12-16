from flask import Flask, session
from routes.menu_routes import menu_bp
from routes.order_routes import order_bp
from routes.cart_routes import cart_bp
from routes.auth_routes import auth_bp
from config import Config


def create_app():
	app = Flask(__name__)
	app.config.from_object(Config)

	app.register_blueprint(menu_bp)
	app.register_blueprint(order_bp)
	app.register_blueprint(cart_bp)
	app.register_blueprint(auth_bp)

	@app.context_processor
	def inject_globals():
		cart = session.get("cart", {})
		return {
			"cart_count": sum(cart.values()),
			"current_user": session.get("user")
		}

	return app


if __name__ == "__main__":
	# 如果直接运行 app.py，正常启动
	# 如果想自动启动后端，请运行 start_with_backend.py
	app = create_app()
	app.run(host=Config.FRONTEND_HOST, port=Config.FRONTEND_PORT, debug=True)


