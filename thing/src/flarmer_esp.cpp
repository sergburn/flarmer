#include <stdint.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_TCS34725.h>

#include <BlynkSimpleEsp8266.h>

const char* ssid = "Pandora";
const char* password = "kospoozyat";

const char* blynk_auth = "24079142884247ba8242fc208221f75d";

volatile static bool isUpdating = false;
static unsigned long lastTime = 0;
static const unsigned long PERIOD = 1000;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);
bool hasTcs = false;

void setup()
{
    Serial.begin(115200);
    Serial.println("Booting");

    // WIFI
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    // TCS34725
    if (tcs.begin())
    {
        Serial.println("Found TCS34725");
        hasTcs = true;
    }
    else
    {
        Serial.println("No TCS34725 found ... check your connections");
    }

    // BLYNK
    Blynk.config(blynk_auth);

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
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

uint16_t mapTcsToBlynk(uint16_t chn)
{
    return map(chn, 0, 2000, 0, 1023);
}

void loop()
{
    ArduinoOTA.handle();

    if (!isUpdating)
    {
        Blynk.run();

        unsigned long now = millis();
        if (hasTcs)
        {
            uint16_t r, g, b, c, t, lux;        
            tcs.getRawData(&r, &g, &b, &c);
            t = tcs.calculateColorTemperature(r, g, b);
            lux = tcs.calculateLux(r, g, b);

            Serial.printf("{ %4u %4u %4u %4u } -> %5u K, %3u lx\n", r, g, b, c, t, lux);

            Blynk.virtualWrite(V0, mapTcsToBlynk(r));
            Blynk.virtualWrite(V1, mapTcsToBlynk(g));
            Blynk.virtualWrite(V2, mapTcsToBlynk(b));
            Blynk.virtualWrite(V3, mapTcsToBlynk(c));
        }
        else
        {
            if (now > lastTime + PERIOD)
            {
                Serial.printf("ChipId %x, FlashId %x, FSize %u, FRsize %u, Freq %d\n",
                            ESP.getChipId(),
                            ESP.getFlashChipId(),
                            ESP.getFlashChipSize(),
                            ESP.getFlashChipRealSize(),
                            ESP.getCpuFreqMHz());
                lastTime = now;
            }
        }
    }
}