# esp32-ble-advertisement-scanner-xiaomi-sensor-decoder
An Example Arduino Application for BLE Advertisement Scanning and Xiaomi Sensor Decoding

This Arduino application was created to experiment with the ESP32 and BLE support.  Specifically,
I wanted to create a more full featured application to gather environmental data (Temperature
and Humidity) from the inexpensive XIAOMI Mijia Model LYWSD03MMC sensor that I had bought on
Ali Express from here: https://www.aliexpress.com/item/3256803316317463.html

When investigating this support, I came across several tutorials that referred me to two
repos here on Github that had extended and enhanced the firmware for these devices:

https://github.com/atc1441/ATC_MiThermometer which referred me to:
https://github.com/pvvx/ATC_MiThermometer

The latter site is what I've used to upgrade the firmware in my Xiaomi sensors, and seems
to provide some very nice enhancements.

## Installation
### Upgrading the Xiaomi firmware
The first step is to power up your Xiaomi sensors, and then update the firmware.  From
the site above, you can access the Web-based Telink Flasher for Mi Thermostat:

https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html

NOTE: I've only used the Chrome browser to do this, and your browser must support the
      Bluetooth extensions that allow BLE communications.

When you open this page there are a few steps to follow:
1. Open the link above
2. Click the Connect button
3. In the pop-up that appears, wait and watch for your sensor to appear, named "LYWSD03MMC" by default
4. Select the sensor and click Pair
5. Once paired, your sensor information will be populated in the web page
6. At this point you can click the Activate button. (NOTE: I'm not yet sure WHY several videos indicate the next two steps, but I follow them!)
7. Copy down the Mi Token (this might be useful in the future)
8. Copy down the Mi Bind Key (this might be useful in the future)
9. After you had clicked the Activate button, and the page refreshed, there will now be a button: Custom Firmware ver 3.8 (at the time of this writing!)
10. Click this button and a new buton should appear to Flash
11. Click the Flash button and watch at the top of the screen as the flashing of the firmware progresses
12. Once this is done, the device will restart and be disconnected.  You can see this in the log at the bottom of the webpage.
13. Click the Reconnect button at the top of the web page
14. The web page will now look quite different as the new features are now displayed
15. There are two values that can be changed here - one required and the other optional:
      - Required: Advertising Type: this must be changed to PVVX (Custom) and is the packet format expacted
      - Optional: Temperature: F or C option to display in your preferred scale
16. Click the "Send Config" button to save this to the device
17. You can now also click the "Set Time" button which wil sync the device RTC to your browser time
18. Now click the "Disconnect" button.  NOTE: When you are Connected to a device it will NOT advertise!

This is all that is required.  By default, when this new firmware is installed the device will now have a BLE name of ATC_{mac address}

### Arduino Software
You will need to have the ESP32 Board support installed.  Create a new Arduino application and open this application file.
When you run it on your device, it should immediately begin to loop, performing BLE Advertisement Scans, and displaying the 
Xiaomi detected advertisements in the serial monitor.
