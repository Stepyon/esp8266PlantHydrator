# ESP8266 Automatic Plant Hydrator Controller

## Overview
This project implements an automatic plant watering system using the ESP8266 microcontroller (Wemos D1 mini). The system monitors soil moisture levels and activates a water pump to maintain optimal soil moisture for your plants. Additionally, it supports configuration via a web interface, and can send notifications via Telegram and record sensor data using ThingSpeak service. The device can be powered by a battery since, in operation mode, it spends most of the time in deep sleep. It wakes up every 3 to 3.5 hours to check the soil moisture level and waters the plant if needed, then returns to deep sleep.

## Features
- **Automatic Watering:** The system automatically waters the plants based on the soil moisture levels.
- **WiFi Connectivity:** Connects to a WiFi network to provide remote configuration, monitoring and notifications.
- **Web Interface:** A simple web interface for configuration and real-time monitoring.
- **Telegram Integration:** Sends notifications to a Telegram bot when watering occurs.
- **ThingSpeak Integration:** Optionally logs soil moisture data to ThingSpeak for remote monitoring.
- **Over-The-Air (OTA) Updates:** Supports basic OTA updates for easy firmware upgrades.
- **EEPROM Storage:** Stores configuration and calibration settings in EEPROM to retain them across resets.

## Components
- **Wemos D1 mini or any ESP8266:** The microcontroller that powers the system.
- **Soil Moisture Sensor:** Measures the soil moisture level.
- **Water Pump:** Activates to water the plants.
- **LED Indicator:** Indicates system status.
- **Switch Button:** For entering configuration mode.

## Getting Started

### Prerequisites
- **Arduino IDE:** Install the Arduino IDE from [here](https://www.arduino.cc/en/software).
- **ESP8266 Board Support:** Install the ESP8266 board package in the Arduino IDE.
- **Libraries:** If needed install the following libraries via the Arduino Library Manager:
  - WiFiManager
  - ESP8266WebServer
  - ArduinoOTA
  - ArduinoJson
  - EEPROM

### Hardware Setup
1. **Connect the Soil Moisture Sensor:**
   - VCC to GPIO5 (to be powered directly from GPIO pin the soil sensor should draw less than 10mA, otherwise use MOSFET or transistor)
   - GND to GND
   - Analog output to A0 (Analog pin)
2. **Connect the Water Pump:**
   - Use a transistor, MOSFET or relay module to interface the pump with the ESP8266.
   - Connect the control pin of the transistor/MOSFET/relay to GPIO 15.
3. **Connect the Switch Button:**
   - One side to GPIO 4.
   - Other side to GND.
4. **(Optional) Connect the LED Indicator or use onboard one:**
   - Anode to GPIO 2 with a current-limiting resistor.
   - Cathode to GND.

### Software Setup
1. **Clone the Repository**
2. **Open the Sketch:**
   - Open `ESP8266PlantHydrator.ino` in the Arduino IDE.
3. **Upload the Sketch:**
   - Select the appropriate board and port from the Tools menu.
   - Click on the `Upload` button.
4. **Configure WiFi and Settings:**
   - On first boot, the device will start in Access Point mode to configure WIFI.
   - Connect to the WiFi network named `ESP_PlantHydrator`.
   - Open a web browser and navigate to `http://192.168.4.1` to configure the WiFi credentials.
   - When the device has connected to WIFI you should be able to access the configuration web page using `http://ESP_PlantHydrator` link from the local network. If it is not available you will need to find out the client IP address it received from your router and using that IP open the configuration web page in a browser.

## Usage
- **Normal Operation:** The device will automatically monitor the soil moisture and water the plants as needed. It will send notifications via Telegram and log data to ThingSpeak if enabled.
- **Configuration Mode:** To enter configuration mode, switch on the configuration switch on and reset the device.

## Configuration Parameters
- **Watering Percent:** The soil moisture threshold below which watering will occur.
- **Watering Time:** The duration (in seconds) for which the pump will run.
- **Sensor max/min:** The soil moisture sensor calibration values (dry sensor and in water).
- **ThingSpeak Channel ID and Write Key:** For logging data to ThingSpeak.
- **Telegram Bot Token and Chat ID:** For sending notifications via Telegram.

## License
This project is licensed under the Apache License Version 2.0 - see the LICENSE file for details.

## Contributing
Contributions are welcome! Feel free to open an issue or submit a pull request.
