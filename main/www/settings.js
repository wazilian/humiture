
/**
 * Global shorthand variable ID for document.getElementById
 * Only defined in status.js and settings.js
 */
const ID = document.getElementById.bind(document);

// Settings JS Globals
const error_icon	= "error.ico";
const success_icon	= "success.ico";
const info_icon		= "info.ico";

/**
 * Initialize functions and UI
 */
document.addEventListener("DOMContentLoaded", function() {
	// Clear message icons and boxes on each page load
	ID("icon_settings").src = "";
	ID("icon_network").src = "";
	ID("icon_fw").src = "";
	ID("msg_box_settings").textContent = "";
	ID("msg_box_network").textContent = "";
	ID("msg_box_fw").textContent = "";
	ID("tooltip_bin_file").hidden = true;

	// Enable all onlick/onchange events
	ID("button_update_settings").addEventListener("click", updateSystemSettings);
	ID("show_password").addEventListener("click", showPassword);
	ID("button_network").addEventListener("click", checkCredentials);
	ID("bin_file").addEventListener("change", getBinFileInfo);
	ID("button_bin_file").addEventListener("click", () => { ID("bin_file").click() });
	ID("button_update_firmware").addEventListener("click", updateFirmware);

	getSystemSettings();
});

/**
 * Gets the system settings for settings.html
 */
async function getSystemSettings() {
	try {
		const response = await fetch("/systemSettings", { signal: AbortSignal.timeout(5000) });

		if (!response.ok) {
			console.log(response.status + " " + response.statusText);
			return;
		}

		const data = await response.json();

		document.title = data.system_name + " Status";
		ID("icon_main").src = "";
		ID("msg_box_main").textContent = "";

		ID("system_name").value = data.system_name;
		ID("use_led").checked = data.use_led;
		ID("ssid").value = data.ssid;
	} catch (error) {
		ID("icon_main").src = error_icon;
		ID("msg_box_main").textContent = "Trouble communicating with the device.";
		console.log(error.message);
	}
}

/**
 * Disable all buttons when a button is clicked. Enable all buttons when task is completed
 */
function disableButtons(toggle) {
	document.querySelectorAll("input[type='button']").forEach(button => { button.disabled = toggle; });
}

/**
 * Test if given string is an IP address (only IPv4 for now)
 */
function testIp(ip) {
	const ipv4 = /^(\d{1,3}\.){3}\d{1,3}$/;
	return ipv4.test(ip);
}

/**
 * Updates system settings
 */
async function updateSystemSettings() {
	let flag = false;
	const system_name = ID("system_name").value;
	const use_led = +ID("use_led").checked;	// Unary Plus Operator (true => 1, false => 0)

	if (system_name === "") {
		ID("icon_settings").src = error_icon;
		ID("msg_box_settings").textContent = "Please provide a system name.";
		return;
	}

	if (system_name.length > 31) {
		ID("icon_settings").src = error_icon;
		ID("msg_box_settings").textContent = "System name cannot be more than 31 characters.";
		return;
	}

	disableButtons(true);
	ID("icon_settings").src = info_icon;
	ID("msg_box_settings").classList.add("loading");
	ID("msg_box_settings").textContent = "Updating system settings";

	try {
		const response = await fetch("/updateSystemSettings", {
			method: "POST",
			signal: AbortSignal.timeout(5000),
			headers: {	"system_name": system_name, "use_led": use_led }
		});

		if (!response.ok) {
			console.log(response.status + " " + response.statusText);
			return;
		}

		const data = await response.json();

		if (data.response == 0) {
			flag = true
		}
	} catch (error) {
		flag = 2;
		console.log(error.message);
	}

	if (flag) {
		ID("icon_settings").src = success_icon;
		ID("msg_box_settings").textContent = "System settings updated successfully.";
	} else {
		ID("icon_settings").src = error_icon;
		ID("msg_box_settings").textContent = "System settings update failed.";
	}

	disableButtons(false);
	ID("msg_box_settings").classList.remove("loading");
}
