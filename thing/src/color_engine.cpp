#include "HardwareSerial.h"

#include "color_engine.h"

extern IColorSensor* init_tcs34725();
extern IColorSensor* init_apds9660();

ColorEngine::ColorEngine() :
    mTcs(NULL), mApds(NULL)
{
}

bool ColorEngine::init()
{
    Serial.println("Trying TCS34725 ...");
    mTcs = init_tcs34725();
    if (!mTcs)
    {
        Serial.println("Fallback to APDS9660 ...");
        mApds = init_apds9660();
    }
    return mTcs || mApds;
}

bool ColorEngine::readColors(ColorMsmt& msm)
{
    bool ok = false;
    if (mTcs)
    {
        ok = mTcs->readColors(msm);
    }
    if (!ok && mApds)
    {
        ok = mApds->readColors(msm);
    }
    return ok;
}