
/**
 * Global shorthand variable ID for document.getElementById
 * for this JS file is defined in settings.js
 */

// WiFi JS Globals
let wifiConnectInterval = null;

/**
 * Show/hide the WiFi password
 */
function showPassword() {
	let input = ID("password");

	input.type = input.type == "password" ? "text" : "password";
}

/**
 * Starts the WiFi connection status interval
 */
function startWifiConnectStatusInterval() {
	wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
}

/**
 * Stops the WiFi connection status interval
 */
function stopWifiConnectStatusInterval() {
	if (wifiConnectInterval !== null) {
		clearInterval(wifiConnectInterval);
		wifiConnectInterval = null;
	}
}

/**
 * Gets the WiFi connection status
 */
async function getWifiConnectStatus() {
	// 0 => pending, 1 => success, 2 => error
	let flag = 0;

	try {
		const response = await fetch("/wifiConnectStatus", { signal: AbortSignal.timeout(5000) });

		if (!response.ok) {
			console.log(response.status + " " + response.statusText);
			return;
		}

		const data = await response.json();
		const status = data.wifi_connect_status;

		// wifi_connect_status (2 => connecting)
		if (status == 1) {
			flag = 1;
		} else if (status == 3 || status == 4) {
			flag = 2;
		}
	} catch (error) {
		flag = 2;
		console.log(error.message);
	}

	if (flag == 1) {
		stopWifiConnectStatusInterval();
		disableButtons(false);
		ID("password").textContent = "";
		ID("icon_network").src = success_icon;
		ID("msg_box_network").classList.remove("loading");
		ID("msg_box_network").textContent = "Connection Success!";
	} else if (flag == 2) {
		stopWifiConnectStatusInterval();
		disableButtons(false);
		ID("password").textContent = "";
		ID("icon_network").src = error_icon;
		ID("msg_box_network").classList.remove("loading");
		ID("msg_box_network").textContent = "Failed to connect. Please check your network credentials.";
	}
}

/**
 * Attempt to connect with new WiFi credentials
 */
async function connectWifi() {
	let flag = false;
	const ssid = ID("ssid").value;
	const password = ID("password").value;

	disableButtons(true);
	ID("icon_network").src = info_icon;
	ID("msg_box_network").classList.add("loading");
	ID("msg_box_network").textContent = "Connecting to WiFi";

	try {
		const response = await fetch("/wifiConnect", {
			method: "POST",
			signal: AbortSignal.timeout(5000),
			headers: { "ssid": ssid, "password": password }
		});

		if (!response.ok) {
			console.log(response.status + " " + response.statusText);
		} else {
			const data = await response.json();

			if (data.response == 0) {
				flag = true;
			}
		}
	} catch (error) {
		console.log(error.message);
	}

	if (flag) {
		startWifiConnectStatusInterval();
	} else {
		disableButtons(false);
		ID("password").textContent = "";
		ID("icon_network").src = error_icon;
		ID("msg_box_network").classList.remove("loading");
		ID("msg_box_network").textContent = "Failed to connect. Please check your network credentials.";
	}
}

/**
 * Checks credentials user given network and password fields
 */
function checkCredentials() {
	const ssid = ID("ssid").value;
	const password = ID("password").value;

	if (ssid === "" || password === "")	{
		ID("icon_network").src = error_icon;
		ID("msg_box_network").textContent = "Network and/or Password cannot be empty.";
		return;
	}

	if (ssid.length > 31) {
		ID("icon_network").src = error_icon;
		ID("msg_box_network").textContent = "Network cannot be more than 31 characters.";
		return;
	}

	if (password.length < 8) {
		ID("icon_network").src = error_icon;
		ID("msg_box_network").textContent = "Password must be at least 8 characters.";
		return;
	}

	if (password.length > 63) {
		ID("icon_network").src = error_icon;
		ID("msg_box_network").textContent = "Password must be less than 64 characters.";
		return;
	}

	connectWifi();
}
