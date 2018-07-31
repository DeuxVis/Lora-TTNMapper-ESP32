## Intro

This is a simple sketch demonstrating the capability of the [TTGO T-Beam](https://www.aliexpress.com/store/product/TTGO-T-Beam-ESP32-433-868-915Mhz-WiFi-wireless-Bluetooth-Module-ESP-32-GPS-NEO-6M/2090076_32875743018.html) as a [TTN Mapper](https://ttnmapper.org/) Node on [The Things Network](https://www.thethingsnetwork.org/) LoraWAN.

Derived from [sbiermann/Lora-TTNMapper-ESP32](https://github.com/sbiermann/Lora-TTNMapper-ESP32) and with some information/inspiration from [cyberman54/ESP32-Paxcounter](https://github.com/cyberman54/ESP32-Paxcounter) and [Edzelf/LoRa](https://github.com/Edzelf/LoRa).

## Software dependencies

Arduino IDE [ESP32 extension](https://github.com/espressif/arduino-esp32)

[TinyGPS++](http://arduiniana.org/libraries/tinygpsplus/)

[LMIC-Arduino](https://github.com/matthijskooijman/arduino-lmic) : Make sure to get the last version - *1.5.0+arduino-2* currently - because the arduino IDE library updater is getting confused by the versioning scheme of that library.

## Instructions

You need to connect the [T-Beam](https://github.com/LilyGO/TTGO-T-Beam) DIO1 pin marked *Lora1* to the *pin 33* - So that the ESP32 can read that output from the Lora module.
Optionally you can also connect the *Lora2* output to *GPIO 32*, but this is not needed here.

You can program the T-Beam using the [Arduino ESP32](https://github.com/espressif/arduino-esp32) board 'Heltec_WIFI_LoRa_32'.

On The Things Network side, the settings needed are available [here](https://www.thethingsnetwork.org/docs/applications/ttnmapper/).

Let me know if more detailed instructions are needed.

## Todolist

* ~~Stop sending data to TTN until the GPS get a fix.~~
* Save and reload the frame counter somewhere - GPS RTC data ? SPIFFS ? EEPROM ? - so I can check the "Frame Counter Checks" box as recommended on TTN.
* Also save the GPS 'status' so that on next boot it gets a fix faster.
* Switch to OTAA auth method for TTN and save the 'credentials' for reboot use.
* Reduce the power needed ! That thing is a power hog currently, we need to make it sleep most of the time as possible.
* Adapt the data send frequency based on current velocity : When not moving, an update per hour should be enough.

Let me know if you think anything else would make sense for a TTN mapper node : Open an issue, I will consider it.
