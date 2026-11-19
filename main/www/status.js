
/**
 * Global shorthand variable ID for document.getElementById
 * Only defined in status.js and settings.js
 */
const ID = document.getElementById.bind(document);

// Status JS Globals
const error_icon = "error.ico";
const status_interval = 5000;		// Update index.html every 5 seconds
const cc_val_green = -65 - 1;		// Color chart values
const cc_val_orange = -80;
const cc_val_red = -81;
const local_options = Intl.DateTimeFormat().resolvedOptions();
const date_options = { weekday: "short", year: "numeric", month: "short", day: "numeric" };
const time_options = { hour: "numeric", minute: "2-digit", second: "2-digit", timeZoneName: "short" };

/**
 * Initialize functions and UI
 */
document.addEventListener("DOMContentLoaded", function() {
	// Populate text for RSSI color chart (.innerHTML for degree symbols)
	ID("cc_text_green").innerHTML = "&gt; " + cc_val_green + " dB";
	ID("cc_text_orange").innerHTML = cc_val_green + " to " + cc_val_orange + " dB";
	ID("cc_text_red").innerHTML = "&lt; " + cc_val_orange + " dB";

	getSystemStatus();
	startStatusInterval();
});

/**
 * Sets the interval for getting the system status on index.html
 */
function startStatusInterval() {
	setInterval(getSystemStatus, status_interval);
}

/**
 * Gets the system status for index.html
 */
async function getSystemStatus() {
	try {
		const response = await fetch("/systemStatus", { signal: AbortSignal.timeout(5000) });

		if (!response.ok) {
			console.log(response.status + " " + response.statusText);
			return;
		}

		const data = await response.json();
		const iso_8601_boot = new Date(data.iso_8601_boot);
		const iso_8601 = new Date(data.iso_8601);

		// .replace used here to remove comma after weekday
		const boot_date = iso_8601_boot.toLocaleDateString(local_options.locale, date_options).replace(",", "");
		const boot_time = iso_8601_boot.toLocaleTimeString(local_options.locale, time_options);
		const date = iso_8601.toLocaleDateString(local_options.locale, date_options).replace(",", "");
		const time = iso_8601.toLocaleTimeString(local_options.locale, time_options);

		const rssi = data.rssi + " dB";
		const uptime = convertUptime(data.uptime);
		const temp_c = data.temperature;
		const temp_f = temp_c * 1.8 + 32;
		const temperature = temp_f.toFixed(1) + "&deg;F (" + temp_c + "&deg;C)";
		const humidity = data.humidity + "%";

		document.title = data.system_name + " Status";
		ID("icon_main").src = "";
		ID("msg_box_main").textContent = "";

		ID("fw_version").textContent = data.fw_version;
		ID("system_name").textContent = data.system_name;
		ID("ssid").textContent = data.ssid;
		ID("rssi").textContent = rssi;
		ID("ip_address").textContent = data.ip;
		ID("mac_address").textContent = data.mac;

		ID("boot_date").textContent = boot_date;
		ID("boot_time").textContent = boot_time;
		ID("uptime").textContent = uptime;
		ID("date").textContent = date;
		ID("time").textContent = time;
		ID("temperature").innerHTML = temperature;  // .innerHTML for degree symbols
		ID("humidity").textContent = humidity;

		// Change text color for RSSI values for emphasis
		ID("rssi").classList.remove("green", "orange", "red");
		if (data.rssi > cc_val_green) {
			ID("rssi").classList.add("green");
		} else if (data.rssi > cc_val_red) {
			ID("rssi").classList.add("orange");
		} else {
			ID("rssi").classList.add("red");
		}
	} catch (error) {
		ID("icon_main").src = error_icon;
		ID("msg_box_main").textContent = "Trouble communicating with the device.";
		console.log(error.message);
	}
}

/**
 * Convert given timestamp to days/hours/minutes/seconds
 */
function convertUptime(timestamp) {
	const days = Math.floor(timestamp / 86400);
	const hours = Math.floor((timestamp % 86400) / 3600);
	const minutes = Math.floor((timestamp % 3600) / 60);
	const seconds = Math.floor(timestamp % 60);
	let units = [];

	if (days > 0) units.push(days + "d");
	if (hours > 0) units.push(hours + "h");
	if (minutes > 0) units.push(minutes + "m");
	if (seconds > 0 || units.length === 0) units.push(seconds + "s");

	return units.join(" ");
}
