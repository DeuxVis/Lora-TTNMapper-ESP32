## Intro

This is a simple sketch demonstrating the capability of the [TTGO T-Beam](https://www.aliexpress.com/store/product/TTGO-T-Beam-ESP32-433-868-915Mhz-WiFi-wireless-Bluetooth-Module-ESP-32-GPS-NEO-6M/2090076_32875743018.html) as a [TTN Mapper](https://ttnmapper.org/) Node on [The Things Network](https://www.thethingsnetwork.org/) LoraWAN.

Derived from [sbiermann/Lora-TTNMapper-ESP32](https://github.com/sbiermann/Lora-TTNMapper-ESP32) and with some information/inspiration from [cyberman54/ESP32-Paxcounter](https://github.com/cyberman54/ESP32-Paxcounter) and [Edzelf/LoRa](https://github.com/Edzelf/LoRa).

## Software dependencies

Arduino IDE [ESP32 extension](https://github.com/espressif/arduino-esp32)

[TinyGPS++](http://arduiniana.org/libraries/tinygpsplus/)

[LMIC-Arduino](https://github.com/matthijskooijman/arduino-lmic) : Make sure to get the last version - *1.5.0+arduino-2* currently - because the arduino IDE library updater is getting confused by the versioning scheme of that library.
If you are on a 915 Mhz - ie USA - network you must modify the lmic library "config.h" file accordingly !

## Instructions

*Only on the oldest versions of the board* you need to connect the [T-Beam](https://github.com/LilyGO/TTGO-T-Beam) DIO1 pin marked *Lora1* to the *pin 33* - So that the ESP32 can read that output from the Lora module.
Optionally you can also connect the *Lora2* output to *GPIO 32*, but this is not needed here.

You can program the T-Beam using the [Arduino ESP32](https://github.com/espressif/arduino-esp32) board 'T-Beam'.

On The Things Network side, the settings needed are available [here](https://www.thethingsnetwork.org/docs/applications/ttnmapper/).

Configure the Payload decoder for TTNv2 with:
```javascript
function Decoder(bytes, port) {
    var decoded = {};

    decoded.latitude = ((bytes[0]<<16)>>>0) + ((bytes[1]<<8)>>>0) + bytes[2];
    decoded.latitude = (decoded.latitude / 16777215.0 * 180) - 90;
  
    decoded.longitude = ((bytes[3]<<16)>>>0) + ((bytes[4]<<8)>>>0) + bytes[5];
    decoded.longitude = (decoded.longitude / 16777215.0 * 360) - 180;
  
    var altValue = ((bytes[6]<<8)>>>0) + bytes[7];
    var sign = bytes[6] & (1 << 7);
    if(sign)
    {
        decoded.altitude = 0xFFFF0000 | altValue;
    }
    else
    {
        decoded.altitude = altValue;
    }
  
    decoded.hdop = bytes[8] / 10.0;

    return decoded;
}
```

For TTNv3 you can use the following function:
```javascript
function decodeUplink(input) {
    var decoded = {};

    decoded.latitude = ((input.bytes[0]<<16)>>>0) + ((input.bytes[1]<<8)>>>0) + input.bytes[2];
    decoded.latitude = (decoded.latitude / 16777215.0 * 180) - 90;
  
    decoded.longitude = ((input.bytes[3]<<16)>>>0) + ((input.bytes[4]<<8)>>>0) + input.bytes[5];
    decoded.longitude = (decoded.longitude / 16777215.0 * 360) - 180;
  
    var altValue = ((input.bytes[6]<<8)>>>0) + input.bytes[7];
    var sign = input.bytes[6] & (1 << 7);
    if(sign)
    {
        decoded.altitude = 0xFFFF0000 | altValue;
    }
    else
    {
        decoded.altitude = altValue;
    }
  
    decoded.hdop = input.bytes[8] / 10.0;

    return { 
      data: decoded
    }
}


Let me know if more detailed instructions are needed.

## Todolist

* ~~Stop sending data to TTN until the GPS get a fix.~~ <== Done thanks to [@Roeland54](https://github.com/Roeland54)
* Switch to [the maintained version](https://github.com/mcci-catena/arduino-lmic) of the arduino-lmic library.
* Manage and document the different T-Beam revisions/versions.
* ~~Switch to OTAA auth method for TTN and save the 'credentials' for use after deep sleep.~~ <== Done thanks to [@fede2cr](https://github.com/fede2cr)
* Save and reload the frame counter somewhere - GPS RTC data ? SPIFFS ? EEPROM ? - so I can check the "Frame Counter Checks" box as recommended on TTN. <== counter is stored in slow RTC memory so it's kept after deep sleep, thanks to [@christianwicke](https://github.com/christianwicke) - Still investigating the GPS module Battery Backed RAM for saving after power off.
* Also save the GPS 'status' so that on next boot it gets a fix faster.
* ~~Reduce the power needed ! That thing is a power hog currently, we need to make it sleep most of the time as possible.~~
* Adapt the data send frequency based on current velocity : When not moving, an update per hour should be enough.

Let me know if you think anything else would make sense for a TTN mapper node : Open an issue, I will consider it.

