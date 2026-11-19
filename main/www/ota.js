
/**
 * Global shorthand variable ID for document.getElementById
 * for this JS file is defined in settings.js
 */

// OTA JS Globals
let seconds = null;
let otaTimerVar = null;

/**
 * Gets bin file name and size for display on the web page.
 */
function getBinFileInfo() {
	const x = ID("bin_file");
	const span_bin_file = ID("bin_filename");
	const cell_text = x.files[0].name;
	const hover_text = x.files[0].name;
	let bytes = x.files[0].size;
	let megabytes = bytes / 1024 / 1000;

	megabytes = Math.round(megabytes).toLocaleString() + " MB";
	bytes = bytes.toLocaleString() + " bytes";

	ID("tooltip_bin_file").textContent = hover_text;
	ID("bin_filename").textContent = cell_text;
	ID("bin_file_size").textContent = megabytes + " (" + bytes + ")";

	// If CSS enabled ellipsis for bin_filename then enable tooltip
	if (span_bin_file.offsetWidth < span_bin_file.scrollWidth) {
		ID("tooltip_bin_file").hidden = false;
	} else {
		ID("tooltip_bin_file").hidden = true;
	}
}

/**
 * Update firmware with user provided .bin file OTA.
 */
async function updateFirmware() {
	let flag = false;
	const file_input = ID("bin_file");

	if (file_input.files.length === 0 || !file_input.files[0].name.endsWith(".bin")) {
		ID("icon_fw").src = error_icon;
		ID("msg_box_fw").textContent = "Please select a bin file.";
		return;
	}

	const bin_file = file_input.files[0];
	let form_data = new FormData();
	form_data.set("file", bin_file, bin_file.name);

	disableButtons(true);
	ID("icon_fw").src = info_icon;
	ID("msg_box_fw").classList.add("loading");
	ID("msg_box_fw").textContent = "Firmware update in progress";

	try {
		const response = await fetch("/OTAupdate", {
			method: "POST",
			signal: AbortSignal.timeout(60000),
			body: form_data
		});

		if (!response.ok) {
			console.log(response.status + " " + response.statusText);
		} else {
			const data = await response.json();

			if (data.ota_update_status == 1) {
				flag = true;
			}
		}
	} catch (error) {
		console.log(error.message);
	}

	if (flag) {
		seconds = 10;		// Set the countdown timer time
		otaRebootTimer();	// Start the countdown timer
	} else {
		ID("icon_fw").src = error_icon;
		ID("msg_box_fw").textContent = "Firmware upload error.";
	}

	disableButtons(false);
	ID("msg_box_fw").classList.remove("loading");
}

/**
 * Display the OTA reboot countdown.
 */
function otaRebootTimer() {
	ID("icon_fw").src = success_icon;
	ID("msg_box_fw").classList.remove("loading");
	ID("msg_box_fw").textContent = "Firmware Update Complete. Device will reboot in: " + seconds;
	seconds--;

	if (seconds == -1)	{
		clearTimeout(otaTimerVar);
		window.location.href = "index.html";
	} else {
		otaTimerVar = setTimeout(otaRebootTimer, 1000);
	}
}
