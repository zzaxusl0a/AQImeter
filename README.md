# AQI Meter

AQI Meter is an indoor air quality display for the ESP32-2432S028R ILI9341 device with a 2.8" touchscreen. These devices are often called the "CYD" (Cheap Yellow Display).

This project began as the excellent **Aura** weather widget by Surrey Homeware and has been modified to display **current air quality conditions from PurpleAir** instead of a weather forecast.

This is just the source code for the project. This project includes a case design and assembly instructions. The complete instructions are available
here: https://makerworld.com/en/models/1382304-aura-smart-weather-forecast-display

The original Aura project is available at:
https://github.com/Surrey-Homeware/Aura

## What's Different from Aura?

The core user interface has been simplified and repurposed for air quality monitoring:

- The weather forecast icons that originally appeared across the top of the display have been removed.
- The top section now displays a large, color-coded **Air Quality Index (AQI)** value.
- AQI data is retrieved from the **PurpleAir API** using a user-provided API Read Key.
- The display uses EPA AQI categories and matching colors:
  - Green — Good (0–50)
  - Yellow — Moderate (51–100)
  - Orange — Unhealthy for Sensitive Groups (101–150)
  - Red — Unhealthy (151–200)
  - Purple — Very Unhealthy (201–300)
  - Maroon — Hazardous (301+)

This makes the device ideal for monitoring wildfire smoke, indoor air filtration effectiveness, and general outdoor air quality.

## Hardware

- ESP32-2432S028R ("Cheap Yellow Display") w/ 2.8" ILI9341 touchscreen
- USB power supply

## How It Works

On initial startup, the device will provide instructions on connecting to your wifi network.  Once configured, it:

1. Connects to Wi-Fi using WiFiManager.
2. Retrieves current AQI data from PurpleAir from a configured site.
3. Displays the AQI value prominently on the screen.
4. Updates automatically at regular intervals.

## PurpleAir Sensor Identification

To change the purpleair sensor, tap on the AQI number or color block to change to the settings panel.
On your desktop or laptop, find the sensor you'd like to monitor on the map (https://map.purpleair.com). Click on the sensor, then click on "Get this widget."
In the HTML code displayed, look for the string:
div id='PurpleAirWidget_256957_module_US_EPA_AQI_conversion_C0_average_10_layer_US_EPA_AQI'

The 5 or 6 digit code is the first number displayed. In this case "256957."

The settings page will also ask for your location in order to set the time zone. Search for the largest major city in your time zone.

## PurpleAir API Key Setup

AQI Meter requires a **PurpleAir API Read Key**.

> **Note:** PurpleAir no longer issues API keys by email. Keys are now created through the PurpleAir Developer Dashboard. You should be able to get $10 worth of credits for free, which will run the tool for about 3 years.

### Create a PurpleAir API Read Key

1. Visit the PurpleAir Developer Dashboard:
   https://develop.purpleair.com/dashboards/keys
2. Sign in with a Google account (any email address can be used with Google sign-in).
3. Create a new project, or use the default project.
4. Click **API Keys**.
5. Click **+ API Key**.
6. Set:
   - **Type:** Read
   - **Status:** Enabled
7. Leave optional restrictions blank unless you specifically need them.
8. Click **Create**.
9. Copy the generated key (it will look similar to `B144C732-11DE-11EE-A445-42098A784009`).

PurpleAir documents this process in their community guide.

### Add Your API Key to the Sketch

Open `aqimeter.ino` and replace the example key:

```cpp
String my_api_read_key = "YOUR_PURPLEAIR_READ_KEY";
```

with your own PurpleAir API Read Key.

## How to Compile

1. Configure the Arduino IDE:
   - Install the ESP32 board package.
   - Select **ESP32 Dev Module**.
   - Set **Tools → Partition Scheme** to **Huge App (3MB No OTA/1MB SPIFFS)**.
2. Install the required libraries listed below.
3. Copy the project folders into your Arduino sketch directory (typically `~/Documents/Arduino/`).
4. Replace the PurpleAir API key in `aqimeter.ino`.
5. Compile and upload to the display.

## Required Libraries

- ArduinoJson 7.4.1
- HttpClient 2.2.0
- TFT_eSPI 2.5.43
- WiFiManager 2.0.17
- XPT2046_Touchscreen 1.4
- lvgl 9.2.2

## License

The original `weather.ino` code from Aura is provided under the GNU GPL 3.0 license.

This modified project remains subject to the same license terms.
See the Weather Icons below for information on their licensing.

## Thanks & Credits

- Original Aura project by Surrey Homeware
- Weather icons from https://github.com/mrdarrengriffin/google-weather-icons/tree/main/v2
- Thanks to https://lvgl.io/ for an excellent graphics library
- Thanks to https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display for hardware reference information
- Thanks to https://randomnerdtutorials.com/ for numerous ESP32 and LVGL tutorials
- PurpleAir for providing accessible air quality data via their API

### Libraries Used

- https://arduinojson.org/
- https://github.com/amcewen/HttpClient
- https://github.com/Bodmer/TFT_eSPI
- https://github.com/tzapu/WiFiManager
- https://github.com/PaulStoffregen/XPT2046_Touchscreen
- https://lvgl.io/
