#include <Adafruit_TCS34725.h>

#include "color_engine.h"

Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

class ColorSensorTcs : public IColorSensor
{
    bool readColors(ColorMsmt& clr)
    {
        tcs.getRawData(&clr.r, &clr.g, &clr.b, &clr.c);
        clr.colorTemp = tcs.calculateColorTemperature(clr.r, clr.g, clr.b);
        clr.lux = tcs.calculateLux(clr.r, clr.g, clr.b);
    }
};
ColorSensorTcs colorSensorTcs;

IColorSensor* init_tcs34725()
{
    if (tcs.begin())
    {
        Serial.println("TCS34725 init OK");
        return &colorSensorTcs;
    }
    else
    {
        Serial.println("TCS34725 not found!");
    }
    return NULL;
}