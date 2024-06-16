const char cssContent[] PROGMEM = R"=====(
body {
    background-color: #f9e79f;
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: flex-start;
    min-height: 100vh; /* Ensure the body takes at least full viewport height */
}

.container {
    width: 90%;
    margin: 10px;
    max-width: 600px;
    text-align: center;
    padding: 20px;
    box-sizing: border-box;
    background-color: #fff;
    border-radius: 8px;
    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
}

h1, h2, h3, h4 {
    margin: 10px 0;
}

input[type="number"],
input[type="text"] {
    width: 100%;
    max-width: 360px; /* Ensures all input fields have a consistent width */
    padding: 10px;
    margin: 5px 0;
    border: 1px solid #ccc;
    border-radius: 4px;
    box-sizing: border-box;
}

.button {
    background-color: #4CAF50;
    border: none;
    color: white;
    padding: 10px 15px;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 14px;
    margin: 5px 2px;
    transition: background-color 0.3s, transform 0.2s;
    cursor: pointer;
    border-radius: 4px;
}

.button-container {
    display: flex;
    justify-content: space-between;
    flex-wrap: wrap;
}

.button:active {
    background-color: #367c39;
    transform: scale(0.98);
}

.button:disabled {
    background-color: #9E9E9E;
    color: #CCCCCC;
    cursor: not-allowed;
}

.progress-container {
    width: 100%;
    margin: 5px auto;
    text-align: center;
}

.progress-bar {
    position: relative;
    width: 100%;
    background-color: #f2f2f2;
    border-radius: 5px;
    overflow: hidden;
}

.progress-fill {
    height: 30px;
    transition: width 0.3s ease-in-out;
    background-color: red;
}

.progress-value {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    color: #000;
    font-weight: bold;
    z-index: 1;
}

.form-row {
    display: flex;
    justify-content: space-between;
    margin-bottom: 15px;
}

.form-group {
    display: flex;
    align-items: center;
    flex: 1;
    margin-right: 10px;
}

.form-group:last-child {
    margin-right: 0;
}

.form-group label {
    margin-right: 10px;
    min-width: 150px;
    color: #555555;
}

.form-group input {
    flex: 1;
    padding: 8px;
    border: 1px solid #cccccc;
    border-radius: 4px;
}

form {
    margin: 10px 0;
}

.link-group {
    display: flex;
    justify-content: center;
    width: 100%;
}

.link-button {
    color: #007BFF;
    text-decoration: none;
    cursor: pointer;
    font-size: 14px;
    display: block;
    text-align: center;
    margin-top: 10px;
}

.link-button:hover {
    text-decoration: underline;
}

.link-button.disabled {
    color: #9E9E9E;
    pointer-events: none;
}

#snackbar {
  visibility: hidden;
  min-width: 250px;
  margin-left: -125px;
  background-color: #333;
  color: #fff;
  text-align: center;
  border-radius: 2px;
  padding: 16px;
  position: fixed;
  z-index: 1;
  left: 50%;
  bottom: 30px;
}

#snackbar.show {
  visibility: visible;
  -webkit-animation: fadein 0.5s, fadeout 0.5s 2.5s;
  animation: fadein 0.5s, fadeout 0.5s 2.5s;
}

@-webkit-keyframes fadein {
  from {bottom: 0; opacity: 0;}
  to {bottom: 30px; opacity: 1;}
}

@keyframes fadein {
  from {bottom: 0; opacity: 0;}
  to {bottom: 30px; opacity: 1;}
}

@-webkit-keyframes fadeout {
  from {bottom: 30px; opacity: 1;}
  to {bottom: 0; opacity: 0;}
}

@keyframes fadeout {
  from {bottom: 30px; opacity: 1;}
  to {bottom: 0; opacity: 0;}
}

@media (max-width: 600px) {
    body {
        font-size: 18px;
    }

    h1 {
        font-size: 2em;
    }

    h2 {
        font-size: 1.75em;
    }

    h3 {
        font-size: 1.5em;
    }

    h4 {
        font-size: 1.25em;
    }

    input[type="number"],
    input[type="text"] {
        font-size: 1em;
        max-width: 100%; /* Ensure inputs take full width on small screens */
    }

    .button {
        font-size: 1em;
        padding: 12px 20px;
    }

    .progress-fill {
        height: 40px;
    }

    .button-container {
        flex-direction: column;
    }

    .form-row {
        flex-direction: column;
        align-items: flex-start;
    }

    .form-group {
        margin-right: 0;
        width: 100%;
    }

    .form-group label {
        margin-bottom: 5px;
    }

    .link-button {
        width: 100%;
        text-align: center;
    }
}
)=====";


const char htmlContent[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Automatic Plant Watering Station</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
    <h1>Automatic Plant Watering Station</h1>
    <div class="button-container">
        <button class="button" onclick="sendPumpOn()">PUMP ON</button>
        <button class="button" onclick="sendPumpOff()">PUMP OFF</button>
        <button class="button" onclick="sendSleep()">SLEEP 1 MIN</button>
    </div>
    <h3>Soil moisture:</h3>
    <div class="progress-container">
        <div class="progress-bar">
            <div class="progress-fill" id="progressFill" style="width: 0"></div>
            <div class="progress-value" id="progressValue">{sen_value}</div>
        </div>
    </div>
    <span id="adc_val">{sen_info}</span>
    <br><br>
    <form id="settingsForm">
        <h2>Settings:</h2>
        <div class="form-row">
            <div class="form-group">
                <label for="wtime">Watering time (sec):</label>
                <input type="number" id="wtime" min="1" max="300" value="{wtime_value}">
            </div>
            <div class="form-group">
                <label for="wpc">Watering percent:</label>
                <input type="number" id="wpc" min="0" max="100" value="{wpc_value}">
            </div>
        </div>
        <div class="form-row">
            <div class="form-group">
                <label for="smin">Sensor min:</label>
                <input type="number" id="smin" min="0" max="1024" value="{smin_value}">
            </div>
            <div class="form-group">
                <label for="smax">Sensor max:</label>
                <input type="number" id="smax" min="0" max="1024" value="{smax_value}">
            </div>
        </div>
    </form>
    <form id="ts_form">
        <h4>
            <input type="checkbox" id="ts_state" onclick="toggleTsSupport()" title="Record soil moisture readings into ThinkSpeak service" {ts_enabled_value}>
            ThingSpeak Recording
        </h4>
        <div class="form-group">
            <label for="ts_channel_id">Channel ID:</label>
            <input type="text" id="ts_channel_id" placeholder="NNNNNN" value="{ts_channel_id_value}">
        </div>
        <div class="form-group">
            <label for="ts_write_key">Write key:</label>
            <input type="text" id="ts_write_key" placeholder="XXXXXXXXXXXXXXXX" value="{ts_write_key_value}">
        </div>
        <div class="form-group link-group">
            <a href="#" id="ts_send_data" onclick="sendTSData()" class="link-button">Send data now</a>
        </div>
    </form>
	<br>
    <form id="tg_form">
        <h4>
            <input type="checkbox" id="tg_state" onclick="toggleTgSupport()" title="Notifications will be sent via Telegram bot when watering is performed" {tg_enabled_value}>
            Telegram Bot Notifications
        </h4>
        <div class="form-group">
            <label for="tg_chat_id">Chat ID:</label>
            <input type="text" id="tg_chat_id" placeholder="XXXXXXXX" value="{tg_chat_id_value}">
        </div>
        <div class="form-group">
            <label for="tg_bot_token">Bot token:</label>
            <input type="text" id="tg_bot_token" placeholder="XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" value="{tg_bot_token_value}">
        </div>
		<div class="form-group link-group">
            <a href="#" id="tg_test_button" onclick="sendTgTest()" class="link-button">Send test message</a>
        </div>
    </form>
	<br>
    <form id="saveForm">
        <div class="button-container">
            <button class="button" type="button" onclick="resetSettings()">RESET DEFAULT</button>
            <button class="button" type="button" onclick="saveSettings()">SAVE</button>
            <button class="button" type="button" onclick="resetWifi()">RESET WIFI</button>
        </div>
    </form>
</div>
<div id="snackbar"></div>
<script src="script.js"></script>
</body>
</html>
)=====";


const char jsContent[] PROGMEM = R"=====(

function sendSleep() {
    sendRequest("GET", "sleep", null, false, true);
}

function sendPumpOn() {
    sendRequest("GET", "pump_on", null, false, true);
}

function sendPumpOff() {
    sendRequest("GET", "pump_off", null, false, true);
}

function saveSettings() {
    const data = getFormData(['wtime', 'wpc', 'smax', 'smin', 'ts_state', 'ts_channel_id', 'ts_write_key', 'tg_state', 'tg_chat_id', 'tg_bot_token']);
    sendRequest("POST", "save_settings", data, false, true);
}

function resetSettings() {
    if (confirm("Are you sure you want to reset all settings to default values?")) {
        sendRequest("GET", "reset", null, true, true);
    }
}

function resetWifi() {
    if (confirm("The device will reboot to WIFI configuration portal. Are you sure you want to reset WIFI settings?")) {
        sendRequest("GET", "reset_wifi", null, true);
    }
}

function sendTgTest() {
    const data = getFormData(['tg_chat_id', 'tg_bot_token']);
    sendRequest("POST", "test_tg_bot", data, false, true);
}

function sendTSData() {
    const data = getFormData(['ts_channel_id', 'ts_write_key']);
    sendRequest("POST", "send_ts_data", data, false, true);
}

function sendRequest(method, url, data = null, reload = false, showShackBar = false) {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState === 4) {
            if (this.status === 200) {
                if (showShackBar) {
                    showSnackbar(this.responseText);
                }
                if (reload) {
                    if (elementId) {
                      localStorage.setItem("responseText", this.responseText);
                    }
                    location.reload();
                }
            } else if (showShackBar) {
                showSnackbar("Error - Status: " + this.status);
            }
        }
    };
    xhttp.open(method, url, true);
    if (data) {
        xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        xhttp.send(serializeData(data));
    } else {
        xhttp.send();
    }
}

function getFormData(ids) {
    const data = {};
    ids.forEach(id => {
        const element = document.getElementById(id);
        data[id] = element.type === 'checkbox' ? element.checked : element.value;
    });
    return data;
}

function serializeData(data) {
    return Object.keys(data).map(key => `${encodeURIComponent(key)}=${encodeURIComponent(data[key])}`).join('&');
}

function toggleSupport(id, formId, extraElementIds = []) {
    const isChecked = document.getElementById(id).checked;
    const inputs = document.querySelectorAll(`#${formId} input`);
    inputs.forEach(input => {
        if (input.id !== id) input.disabled = !isChecked;
    });
    extraElementIds.forEach(elementId => {
        const element = document.getElementById(elementId);
        if (isChecked) {
            element.classList.remove('disabled');
        } else {
            element.classList.add('disabled');
        }
    });
}

function toggleTsSupport() {
    toggleSupport('ts_state', 'ts_form', ['ts_send_data']);
}

function toggleTgSupport() {
    toggleSupport('tg_state', 'tg_form', ['tg_test_button']);
}

function updateProgressBar(value) {
    const progressBar = document.getElementById("progressFill");
    const progressValue = document.getElementById("progressValue");
    progressBar.style.width = value + "%";
    progressValue.textContent = value + "%";
    const color = value <= 50 ? `rgb(255, ${Math.round((value / 50) * 255)}, 0)` : `rgb(${Math.round(((100 - value) / 50) * 255)}, 255, 0)`;
    progressBar.style.backgroundColor = color;
}

function getData() {
    const xhttp = new XMLHttpRequest();
    const timeoutMilliseconds = 1500;
    const requestTimer = setTimeout(() => {
        xhttp.abort();
        document.getElementById("adc_val").innerHTML = "timeout";
    }, timeoutMilliseconds);
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            clearTimeout(requestTimer);
            if (this.status == 200) {
                const response = JSON.parse(this.responseText);
                updateProgressBar(response.adc_percent);
                document.getElementById("adc_val").innerHTML = response.adc_info;
            } else if (this.status != 304){
                document.getElementById("adc_val").innerHTML = "error - status: " + this.status;
            }
        }
    };
    xhttp.open("GET", "adcread", true);
    xhttp.send();
}

setInterval(getData, 2000);

document.addEventListener('DOMContentLoaded', function() {
    toggleTsSupport();
    toggleTgSupport();

    var p = document.getElementById("progressValue");
    var progress = parseFloat(p.textContent);
    updateProgressBar(progress);

    document.querySelectorAll('form').forEach(form => {
        form.addEventListener('submit', e => e.preventDefault());
    });

    document.querySelectorAll('.link-button').forEach(link => {
        link.addEventListener('click', e => e.preventDefault());
    });

    const savedData = localStorage.getItem("responseText");
    if (savedData) {
        const stateElement = document.getElementById("snackbar");
        if (stateElement) {
            showSnackbar(savedData);
        }
        localStorage.removeItem("responseText");
    }
});

function showSnackbar(message) {
  var x = document.getElementById("snackbar");
  x.textContent = message;
  x.className = "show";
  setTimeout(function(){ x.className = x.className.replace("show", ""); }, 3000);
}

)=====";
