from flask import Flask, session
from routes.admin_routes import admin_bp
from routes.auth_routes import auth_bp
from config import Config


def create_app():
	app = Flask(__name__)
	app.config.from_object(Config)

	app.register_blueprint(admin_bp)
	app.register_blueprint(auth_bp)

	@app.route("/")
	def index():
		from flask import redirect, url_for
		merchant = session.get("merchant")
		if merchant:
			return redirect(url_for("admin.orders_list"))
		return redirect(url_for("auth.merchant_login_view", next=url_for("admin.orders_list")))

	@app.context_processor
	def inject_globals():
		return {
			"current_merchant": session.get("merchant")
		}

	return app


if __name__ == "__main__":
	app = create_app()
	app.run(host=Config.FRONTEND_HOST, port=Config.FRONTEND_PORT, debug=True)

