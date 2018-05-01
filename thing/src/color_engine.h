#ifndef FLARMER_COLOR_ENGINE_H_
#define FLARMER_COLOR_ENGINE_H_

#include <cstdint>

struct ColorMsmt
{
    uint16_t r,g,b,c;
    uint16_t colorTemp;
    uint16_t lux;
};

class IColorSensor
{
public:
    virtual bool readColors(ColorMsmt& clr) = 0;
};

class ColorEngine
{
public:
    ColorEngine();

    bool init();
    bool readColors(ColorMsmt& clr);

private:
    IColorSensor* mTcs;
    IColorSensor* mApds;
};

#endif // FLARMER_COLOR_ENGINE_H_