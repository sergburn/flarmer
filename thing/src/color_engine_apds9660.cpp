#include <SparkFun_APDS9960.h>

#include "color_engine.h"

SparkFun_APDS9960 apds;

class ColorSensorApds : public IColorSensor
{
    bool readColors(ColorMsmt& clr)
    {
        clr.colorTemp = 0;
        clr.lux = 0;

        // Read the light levels (ambient, red, green, blue)
        if (!apds.readAmbientLight(clr.c) ||
            !apds.readRedLight(clr.r) ||
            !apds.readGreenLight(clr.g) ||
            !apds.readBlueLight(clr.b))
        {
            Serial.println("Error reading light values");
            return false;
        }
        return true;
    }
};
ColorSensorApds colorSensorApds;

IColorSensor* init_apds9660()
{
  // Initialize APDS-9960 (configure I2C and initial values)
    if (apds.init())
    {
        Serial.println(F("APDS-9960: init OK"));
        // Start running the APDS-9960 light sensor (no interrupts)
        if (apds.enableLightSensor(false))
        {
            Serial.println(F("APDS-9960: light sensor init OK"));
            return &colorSensorApds;
        }
        else
        {
            Serial.println(F("APDS9660: light sensor init failed!"));
        }
    }
    else
    {
        Serial.println(F("APDS9660: init failed!"));
    }
    return NULL;
}
