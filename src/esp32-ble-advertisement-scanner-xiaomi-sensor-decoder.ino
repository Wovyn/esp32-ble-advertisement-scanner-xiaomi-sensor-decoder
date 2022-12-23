/**************************************************************************************
 * 
 *  ESP32 BLE Advertisement Scanner and Xiaomi Sensor Example
 *  
 *  Designed to scan for BLE advertisements and decode the data from the XIAOMI Mijia 
 *  Model LYWSD03MMC Temperature Humidity Sensors
 *  
 *  NOTE: These Xiaomi sensors are first upgraded with new firmware from: 
 *          https://github.com/pvvx/ATC_MiThermometer
 *        The page to flash the devices is here: 
 *          https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html
 *        This code is currently working with sensors flashed to the firmware version 3.8
 *        The sensors are configured to output the pvvx (Custom) advertisements
 *  
 *  Last updated 2022-10-04
 *  Written by Scott C. Lemon
 *  Based on code from:
 *        http://educ8s.tv/esp32-xiaomi-hack
 *        https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/examples/BLE_Beacon_Scanner/BLE_Beacon_Scanner.ino
 *
 * Version 1.4
 *  - fix BLE decoding to use a signed short for proper negative handling 
 * 
 * Version 1.3
 *  - changed to 10 second scan
 *  - added LED blink on scan
 *  - fixed check for name prefix bug
 *  - added namePrefix variable
 *  
 **************************************************************************************/

#include <sstream>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEBeacon.h>

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

/*
 * The pvvx (Custom) format has the following Service Data structure:
 * 
 *    uint8_t     MAC[6]; // [0] - lo, .. [6] - hi digits
 *    int16_t     temperature;    // x 0.01 degree
 *    uint16_t    humidity;       // x 0.01 %
 *    uint16_t    battery_mv;     // mV
 *    uint8_t     battery_level;  // 0..100 %
 *    uint8_t     counter;        // measurement count
 *    uint8_t     flags;  // GPIO_TRG pin (marking "reset" on circuit board) flags: 
 *                        // bit0: Reed Switch, input
 *                        // bit1: GPIO_TRG pin output value (pull Up/Down)
 *                        // bit2: Output GPIO_TRG pin is controlled according to the set parameters
 *                        // bit3: Temperature trigger event
 *                        // bit4: Humidity trigger event
 * 
 * From this we get the #define lines below:
 */
 
#define TEMP_LSB    6
#define TEMP_MSB    7
#define HUMID_LSB   8
#define HUMID_MSB   9
#define BATTV_LSB   10
#define BATTV_MSB   11
#define BATTP_BYTE  12

/*
 * How long do we want to wait to collect advertisements?
 *  By default the Xiaomi sensors are sending advertisements every 2.5 seconds.
 *  This can be reconfigured in the firmware using the web tool above.
 *  When we scan for 10 seconds we ought to catch all of the sensors, but not for sure!
 *  
 *  NOTE: During the scan our code is halted in the loop!
 *  
 *  TODO: Update the code to be non-blocking
 */
#define SCAN_TIME  10 // seconds

/*
 * Do you want to see all advertisements that are detected during the scan?
 * This flag will enable high-level logging of all of the advertisements seen.
 */
#define SHOW_ALL  false

/*
 * Do you want the temperature value in C or F?
 * Set true for metric system; false for imperial
 */
boolean METRIC = false;

/*
 * The code will blink an LED to indicate when it is scanning
 * 
 * To blink the LED we need to know the LED Pin
 */
 #define LED_PIN  2

/*
 * Create the BLE Scan object
 */
BLEScan *pBLEScan;

void IRAM_ATTR resetModule(){
    ets_printf("reboot\n");
    esp_restart();
}

/*
 * Our variables for storing the readings
 */
float battery_voltage = -100;
float battery_percent = 0;
float humidity = -100;
float temperature = -100;
int xiaomi_device_count = 0;

/*
 * Set this to the name prefix you want to scan for
 *   - default for the pvvx firmware is 'ATC_'
 */
 const char namePrefix[]  = "ATC_";
 
/**************************************************************************************
 * 
 * This is the BLE Adsvertisement callback.  It will be called as each advertisement is 
 * received during a scan.
 * 
 *    It is important to understand that ALL BLE advertisements from ALL advertising BLE 
 *    devices will be "heard" and cause this callback to be fired.  This means that you 
 *    must filter the advertisements for the ones you are interested in.  There are 
 *    several ways to filter on the payloads, and in this case we are filtering for the
 *    ATC_ prefix that is part of the "name" assigned in the firmware on these devices.
 *    
 **************************************************************************************/
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      if (SHOW_ALL)
      {
        // what can we display about this received advertisement?
        if (advertisedDevice.haveName())
        {
          Serial.print("Device name: ");
          Serial.println(advertisedDevice.getName().c_str());
          Serial.println("");
        }
  
        if (advertisedDevice.haveServiceUUID())
        {
          BLEUUID devUUID = advertisedDevice.getServiceUUID();
          Serial.print("Found ServiceUUID: ");
          Serial.println(devUUID.toString().c_str());
          Serial.println("");
        }
        else
        {
          if (advertisedDevice.haveManufacturerData() == true)
          {
            std::string strManufacturerData = advertisedDevice.getManufacturerData();
  
            uint8_t cManufacturerData[100];
            strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);
  
            if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00)
            {
              Serial.println("Found an iBeacon!");
              BLEBeacon oBeacon = BLEBeacon();
              oBeacon.setData(strManufacturerData);
              Serial.printf("iBeacon Frame\n");
              Serial.printf("ID: %04X Major: %d Minor: %d UUID: %s Power: %d\n", oBeacon.getManufacturerId(), ENDIAN_CHANGE_U16(oBeacon.getMajor()), ENDIAN_CHANGE_U16(oBeacon.getMinor()), oBeacon.getProximityUUID().toString().c_str(), oBeacon.getSignalPower());
            }
            else
            {
              Serial.println("Found another manufacturers beacon!");
              Serial.printf("strManufacturerData: %d ", strManufacturerData.length());
              for (int i = 0; i < strManufacturerData.length(); i++)
              {
                Serial.printf("[%X]", cManufacturerData[i]);
              }
              Serial.printf("\n");
            }
          }
        }
      }

      /*
       *  Is this a device we are looking for with the namePrefix?
       */
      if (advertisedDevice.haveName() && advertisedDevice.haveServiceData() && (advertisedDevice.getName().find(namePrefix) != std::string::npos))
      {
          xiaomi_device_count++;
          int serviceDataCount = advertisedDevice.getServiceDataCount();
          std::string strServiceData = advertisedDevice.getServiceData(0);

          uint8_t cServiceData[100];
          char charServiceData[100];

          strServiceData.copy((char *)cServiceData, strServiceData.length(), 0);

          Serial.printf("Advertised Device: %s\n", advertisedDevice.toString().c_str());

          for (int i=0;i<strServiceData.length();i++) {
              sprintf(&charServiceData[i*2], "%02x", cServiceData[i]);
          }

          std::stringstream ss;
          ss << charServiceData;

          Serial.print("    BLE Service Data:");
          Serial.println(ss.str().c_str());

          // this is a 16-bit signed variable to handle negative temperatures, etc.
          signed short value;

          // extract the temperature
          value = (cServiceData[TEMP_MSB] << 8) + cServiceData[TEMP_LSB];
          if(METRIC)
          {
            temperature = (float)value/100;
          } else {
            temperature = CelciusToFahrenheit((float)value/100);
          }

          // extract the humidity
          value = (cServiceData[HUMID_MSB] << 8) + cServiceData[HUMID_LSB];
          humidity = (float)value/100;

          // extract the battery voltage
          value = (cServiceData[BATTV_MSB] << 8) + cServiceData[BATTV_LSB];
          battery_voltage = (float)value/1000;

          // extract the battery percentage
          value = cServiceData[BATTP_BYTE];
          battery_percent = (float)value;

          printf("    Temperature: %.2f  Humidity: %.2f%%  Battery: %.4fmV  ~%.0f%%\n", temperature, humidity, battery_voltage, battery_percent);
      }
    }
};

void setup() {
  
  Serial.begin(115200);
  Serial.println("ESP32 XIAOMI Mijia Model LYWSD03MMC Advertisement Monitor");

  pinMode(LED_PIN, OUTPUT);     // Initialize the LED_PIN as an output
 
  initBluetooth();

}

void loop() {

  Serial.println("============================================================================");
  Serial.printf("Starting BLE scan for %d seconds...\n", SCAN_TIME);

  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster

  digitalWrite(LED_PIN, LOW);   // Turn the LED on (Note that LOW is the voltage level but actually the LED is on)

  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);

  digitalWrite(LED_PIN, HIGH);  // Turn the LED off by making the voltage HIGH
  
  int count = foundDevices.getCount();

  printf("\nTotal found device count : %d\n", count);
  printf("Total Xiaomi device count: %d\n", xiaomi_device_count);

  xiaomi_device_count = 0;

  delay(1000);
}

void initBluetooth()
{
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(0x50);
    pBLEScan->setWindow(0x30);
}

float CelciusToFahrenheit(float Celsius)
{
 float Fahrenheit=0;
 Fahrenheit = Celsius * 9/5 + 32;
 return Fahrenheit;
}
