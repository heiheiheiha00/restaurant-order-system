const { app, BrowserWindow, dialog } = require('electron');
const path = require('path');
const { spawn } = require('child_process');
const { URL } = require('url');
const http = require('http');
const https = require('https');

const rootDir = path.resolve(__dirname, '..', '..');
const pythonExec = process.env.PYTHON || (process.platform === 'win32' ? 'py' : 'python3');
const launcherScript = path.join(rootDir, 'frontend_merchant', 'start_with_backend.py');
const frontendHost = process.env.FRONTEND_HOST || '127.0.0.1';
const frontendPort = process.env.FRONTEND_PORT || '8090';
const entryUrl = process.env.MERCHANT_ENTRY_URL || `http://${frontendHost}:${frontendPort}/`;
const backendHost = process.env.BACKEND_HOST || '127.0.0.1';
const backendPort = process.env.BACKEND_PORT || '8081';

let stackProcess = null;
let mainWindow = null;

const waitForUrl = (target, attempts = 40, delay = 1000) => {
	const url = new URL(target);
	const client = url.protocol === 'https:' ? https : http;

	const probe = (remaining, resolve, reject) => {
		const req = client.request(
			{
				hostname: url.hostname,
				port: url.port,
				path: url.pathname,
				method: 'GET',
				timeout: 2000
			},
			(res) => {
				res.resume();
				if (res.statusCode && res.statusCode >= 200 && res.statusCode < 500) {
					resolve();
				} else if (remaining > 0) {
					setTimeout(() => probe(remaining - 1, resolve, reject), delay);
				} else {
					reject(new Error(`Timeout waiting for ${target}`));
				}
			}
		);

		req.on('error', () => {
			if (remaining > 0) {
				setTimeout(() => probe(remaining - 1, resolve, reject), delay);
			} else {
				reject(new Error(`Failed to reach ${target}`));
			}
		});

		req.end();
	};

	return new Promise((resolve, reject) => probe(attempts, resolve, reject));
};

const startStack = () => {
	const env = { ...process.env };
	env.BACKEND_HOST = backendHost;
	env.BACKEND_PORT = backendPort;
	env.BACKEND_BASE_URL = env.BACKEND_BASE_URL || `http://${backendHost}:${backendPort}`;
	env.FRONTEND_HOST = frontendHost;
	env.FRONTEND_PORT = frontendPort;
	env.SECRET_KEY = env.SECRET_KEY || 'desktop-merchant-secret';
	env.DB_PATH = env.DB_PATH || path.join(rootDir, 'restaurant.db');
	env.APP_ROLE = 'merchant';

	stackProcess = spawn(pythonExec, [launcherScript], {
		cwd: path.join(rootDir, 'frontend_merchant'),
		env,
		stdio: 'inherit',
		shell: false
	});

	stackProcess.on('exit', (code) => {
		if (code !== 0) {
			console.error(`[merchant-app] stack process exited with code ${code}`);
		}
	});

	return waitForUrl(entryUrl);
};

const cleanup = () => {
	if (stackProcess && !stackProcess.killed) {
		stackProcess.kill('SIGINT');
	}
	stackProcess = null;
};

const createWindow = async () => {
	await startStack();

	mainWindow = new BrowserWindow({
		width: 1280,
		height: 900,
		webPreferences: {
			contextIsolation: true,
			nodeIntegration: false
		},
		show: false
	});

	mainWindow.once('ready-to-show', () => {
		mainWindow.show();
	});

	mainWindow.on('closed', () => {
		mainWindow = null;
	});

	await mainWindow.loadURL(entryUrl);
};

const bootstrap = async () => {
	try {
		await createWindow();
	} catch (err) {
		console.error('Failed to start merchant desktop app', err);
		dialog.showErrorBox('启动失败', err.message);
		app.quit();
	}
};

const gotLock = app.requestSingleInstanceLock();
if (!gotLock) {
	app.quit();
} else {
	app.on('second-instance', () => {
		if (mainWindow) {
			if (mainWindow.isMinimized()) mainWindow.restore();
			mainWindow.focus();
		}
	});

	app.whenReady().then(bootstrap);
}

app.on('window-all-closed', () => {
	if (process.platform !== 'darwin') {
		cleanup();
		app.quit();
	}
});

app.on('before-quit', () => {
	cleanup();
});

process.on('SIGINT', () => {
	cleanup();
	process.exit(0);
});

process.on('SIGTERM', () => {
	cleanup();
	process.exit(0);
});


