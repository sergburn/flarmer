#include <limits>
#include <stdint.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//#include <Adafruit_Sensor.h>
#include <DHT.h>

#include <BlynkSimpleEsp8266.h>

#include "credentials.h"
#include "color_engine.h"

static uint8_t macAddr[WL_MAC_ADDR_LENGTH];

volatile static bool isUpdating = false;
static unsigned long lastTime = 0;
static const unsigned long UPLOAD_PERIOD_BLYNK = 1000;

static const unsigned long UPLOAD_PERIOD_FLARMER = 5000;
static unsigned long uploadTimeFlarmer = 0;

#define DHT_PIN 0 // D3, 10k Pull-up, GPIO0

DHT dht(DHT_PIN, DHT22, 0);

ColorEngine colorEngine;

ADC_MODE(ADC_VCC);

void setup()
{
    Serial.begin(115200);
    Serial.println("Booting");

    // DHT
    pinMode(DHT_PIN, INPUT_PULLUP);
    dht.begin();

    // WIFI
    WiFi.mode(WIFI_STA);
    WiFi.begin(WLAN_SSID, WLAN_PASSWD);
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    WiFi.macAddress(macAddr);
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());

    // Color sensors
    if (!colorEngine.init())
    {
        Serial.println("Color measurements not available! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    // BLYNK
    Blynk.config(BLYNK_AUTH);

    // OTA

    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
        isUpdating = true;
    });
    ArduinoOTA.onEnd([]() {
        isUpdating = false;
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("Ready");
}

uint16_t mapTcsToBlynk(uint16_t chn)
{
    return map(chn, 0, 2000, 0, 1023);
}

void uploadToBlynk(const ColorMsmt& msm)
{
    Blynk.run();

    Blynk.virtualWrite(V0, mapTcsToBlynk(msm.r));
    Blynk.virtualWrite(V1, mapTcsToBlynk(msm.g));
    Blynk.virtualWrite(V2, mapTcsToBlynk(msm.b));
    Blynk.virtualWrite(V3, mapTcsToBlynk(msm.c));
}

enum WarmerState
{
    WRMR_STATE_OFF,
    WRMR_STATE_STANDBY,
    WRMR_STATE_WARMING
};

WarmerState convertColorToState(const ColorMsmt& clr)
{
    if (clr.c < 50)
        return WRMR_STATE_OFF;
    else if (clr.g > clr.r)
        return WRMR_STATE_STANDBY;
    else
        return WRMR_STATE_WARMING;
}

struct Measurement
{
    WarmerState wstate;
    float temperature;
    float humidity;
};

size_t writeFloat(WiFiClient& wifi, float value)
{
    uint32_t u4 = htonl(*(uint32_t*)&value);
    return wifi.write((char*) &u4, 4);
}

size_t writeInt32(WiFiClient& wifi, uint32_t value)
{
    uint32_t u4 = htonl(value);
    return wifi.write((char*) &u4, 4);
}

void uploadToFlarmer(const Measurement& msm)
{
    static
    WiFiClient wifi;
    if (!wifi.connect("192.168.0.101", 9268)) {
        Serial.printf("Failed to connect Flarmer, status %d\n", wifi.status());
        return;
    }

    // '!B6cBff'
    wifi.write(1); // version - B
    // MAC-48 - 6c
    wifi.write(macAddr, WL_MAC_ADDR_LENGTH);
    // wstate - B
    wifi.write(msm.wstate);
    // temp - f
    writeFloat(wifi, msm.temperature);
    // humidity - f
    writeFloat(wifi, msm.humidity);
    wifi.stop();
}

void uploadMsm(const Measurement& th, const ColorMsmt& clr)
{
    Serial.printf("{ %4u %4u %4u %4u } -> %5u K, %3u lx\n",
        clr.r, clr.g, clr.b, clr.c, clr.colorTemp, clr.lux);

    uploadToBlynk(clr);

    unsigned long now = millis();
    if (now > uploadTimeFlarmer + UPLOAD_PERIOD_FLARMER)
    {
        Measurement msm;
        msm.humidity = th.humidity;
        msm.temperature = th.temperature;
        msm.wstate = convertColorToState(clr);

        Serial.printf("State: %d, Temp: %.1f, Humidity: %.1f\n",
            msm.wstate, msm.temperature, msm.humidity);

        uploadToFlarmer(msm);
        uploadTimeFlarmer = now;
    }
}

void loop()
{
    ArduinoOTA.handle();

    if (!isUpdating)
    {
        unsigned long now = millis();

        Measurement msm;
        msm.temperature = dht.readTemperature();
        msm.humidity = dht.readHumidity();

        ColorMsmt clr;
        if (colorEngine.readColors(clr))
        {
            uploadMsm(msm, clr);
        }
        else
        {
            if (now > lastTime + UPLOAD_PERIOD_BLYNK)
            {
                Serial.println("Failed to read colors");

                Serial.print("MAC: ");
                Serial.println(WiFi.macAddress());
                Serial.print("IP: ");
                Serial.println(WiFi.localIP());
                Serial.printf("ChipId %x, FlashId %x, FSize %u, FRsize %u, Freq %d, VCC: %hu\n",
                            ESP.getChipId(),
                            ESP.getFlashChipId(),
                            ESP.getFlashChipSize(),
                            ESP.getFlashChipRealSize(),
                            ESP.getCpuFreqMHz(),
                            ESP.getVcc());
                lastTime = now;
            }
        }
    }
}