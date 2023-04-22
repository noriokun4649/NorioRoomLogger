extern "C"
{
#include "user_interface.h"
}
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <SSCI_BME280.h>
#include <MHZ19_uart.h>
#include <SoftwareSerial.h>
#include <SparkFun_VEML6075_Arduino_Library.h>
#include <SparkFun_SGP40_Arduino_Library.h>

// SGP40 air quality Sensor
SGP40 airQuality;

// VEML6075 UV Sensor
VEML6075 uv;

// BME280 temp and press Sensor
SSCI_BME280 bme280;
uint8_t i2c_addr = 0x76; // I2C Address

// MH-Z19 Co2 Sensor
MHZ19_uart mhz19;
const int rx_pin = 12; // Serial rx pin no
const int tx_pin = 14; // Serial tx pin no

// Wi-Fi Setting
ESP8266WiFiMulti WiFiMulti;
const char *ssid = "";
const char *password = "";
IPAddress ip(192, 168, 1, 50);
IPAddress gateway(192, 168, 1, 1);
IPAddress netmask(255, 255, 255, 0);

// Post API endpoint
const String endpoint = "http://192.168.1.31:8080/post/";

void setup()
{
    // bme280 Settings
    uint8_t osrs_t = 1;     // Temperature oversampling x 1
    uint8_t osrs_p = 1;     // Pressure oversampling x 1
    uint8_t osrs_h = 1;     // Humidity oversampling x 1
    uint8_t bme280mode = 3; // Normal mode
    uint8_t t_sb = 5;       // Tstandby 1000ms
    uint8_t filter = 0;     // Filter off
    uint8_t spi3w_en = 0;   // 3-wire SPI Disable

    Serial.begin(115200);
    Wire.begin();
    bme280.setMode(i2c_addr, osrs_t, osrs_p, osrs_h, bme280mode, t_sb, filter, spi3w_en);
    bme280.readTrim();
    mhz19.begin(rx_pin, tx_pin);
    mhz19.setAutoCalibration(false);
    uv.begin();
    airQuality.begin();
    delay(50);
    wifiWake();
}

void loop()
{
    double temp_act, press_act, hum_act;
    bme280.readData(&temp_act, &press_act, &hum_act);
    int co2ppm = mhz19.getPPM();

    if (co2ppm <= -1 || co2ppm > 5000)
    {
        delay(50);
        co2ppm = mhz19.getPPM();
    }

    Serial.println();
    Serial.print("TEMP : ");
    Serial.print(temp_act);
    Serial.print(" DegC  PRESS : ");
    Serial.print(press_act);
    Serial.print(" hPa  HUM : ");
    Serial.print(hum_act);
    Serial.print(" %  Light:");
    Serial.print(readLight());
    Serial.print(" co2: ");
    Serial.println(co2ppm);
    Serial.println("UV-A : " + String(uv.uva()) + " UV-B : " + String(uv.uvb()) + " UV index : " + String(uv.index()));
    Serial.println("VOC index : " + String(airQuality.getVOCindex()));
    sendMessage(temp_act, press_act, hum_act, readLight(), co2ppm, uv.uva(), uv.uvb(), uv.index(), airQuality.getVOCindex(press_act, temp_act));
    delay( 1000);
}

// Post message
void sendMessage(double temp_act, double press_act, double hum_act, int light, int co2, float uv_a, float uv_b, float ubindex, int voc)
{
    if ((WiFiMulti.run() == WL_CONNECTED))
    {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, endpoint); // HTTP
        http.addHeader("Content-Type", "application/json");
        String body = "{\"temp\":" + String(temp_act) +
                      ",\"press\":" + String(press_act) +
                      ",\"hum\":" + String(hum_act) +
                      ",\"light\":" + String(light) +
                      ",\"co2\":" + String(co2) +
                      ",\"uva\":" + String(uv_a) +
                      ",\"uvb\":" + String(uv_b) +
                      ",\"uvindex\":" + String(ubindex) +
                      ",\"voc\":" + String(voc) + "}";
        Serial.println(body);

        Serial.println("[HTTP] POST Start : " + endpoint);
        int httpCode = http.POST(body);
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] POST... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK)
            {
                const String &payload = http.getString();
                Serial.printf("[HTTP] :%s\n", payload);
            }
        }
        else
        {
            Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
}

unsigned int readLight()
{
    unsigned int light = system_adc_read();
    return 1024 - light;
}

void wifiOff()
{
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
}

void wifiWake()
{
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gateway, netmask);
    WiFiMulti.addAP(ssid, password);
}
