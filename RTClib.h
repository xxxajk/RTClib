// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)

#ifndef RTCLIB_H
#define RTCLIB_H
#ifdef __AVR__
#include <Wire.h>
#include <avr/pgmspace.h>
#endif
#include <Arduino.h>
#include "time.h"
#define WIREWRITE Wire.write
#define WIREREAD Wire.read

/* Set this to a one to use the xmem2 lock. This is needed for multitasking and threading */
#ifndef USE_XMEM_I2C_LOCK
#define USE_XMEM_I2C_LOCK 0
#endif

#if USE_XMEM_I2C_LOCK
#include <xmem.h>
#else
#define XMEM_ACQUIRE_I2C() (void(0))
#define XMEM_RELEASE_I2C() (void(0))
#endif


class DateTime {
public:
#ifdef __AVR__
        DateTime(time_t t = 0);
#endif
        DateTime(int32_t t);
        DateTime(uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
        DateTime(const char* date, const char* time);
        DateTime(uint16_t fdate, uint16_t ftime);

        uint16_t year(void) const {
                return _time.tm_year + 1900;
        }

        uint8_t month(void) const {
                return _time.tm_mon;
        }

        uint8_t day(void) const {
                return _time.tm_mday;
        }

        uint8_t hour(void) const {
                return _time.tm_hour;
        }

        uint8_t minute(void) const {
                return _time.tm_min;
        }

        uint8_t second(void) const {
                return _time.tm_sec;
        }

        uint8_t dayOfWeek(void) const {
                return _time.tm_wday;
        }

        time_t secondstime(void) const;
        time_t unixtime(void) const;
        time_t FatPacked(void) const;

protected:
        struct tm _time;
};

#ifndef __arm__

// RTC based on the DS1307 chip connected via I2C and the Wire library

enum Ds1307SqwPinMode {
        Ds1307SquareWaveOFF = 0x00, Ds1307SquareWaveON = 0x80, Ds1307SquareWave1HZ = 0x10, Ds1307SquareWave4kHz = 0x11, Ds1307SquareWave8kHz = 0x12, Ds1307SquareWave32kHz = 0x13
};

class RTC_DS1307 {
public:
        static uint8_t begin(const DateTime& dt);
        static uint8_t adjust(const DateTime& dt);
        static uint8_t set(int shour, int smin, int ssec, int sday, int smonth, int syear);
        uint8_t isrunning(void);
        static DateTime now();
        static uint8_t readMemory(uint8_t offset, uint8_t* data, uint8_t length);
        static uint8_t writeMemory(uint8_t offset, uint8_t* data, uint8_t length);
        static Ds1307SqwPinMode readSqwPinMode();
        static void writeSqwPinMode(Ds1307SqwPinMode mode);
};

// RTC using the internal millis() clock, has to be initialized before use
// NOTE: this clock won't be correct once the millis() timer rolls over (>49d?)

class RTC_Millis {
public:

        static uint8_t begin(const DateTime& dt) {
                return(adjust(dt));
        }
        static uint8_t adjust(const DateTime& dt);
        static DateTime now();
        uint8_t isrunning(void);

protected:
        static int64_t offset;
};

extern RTC_DS1307 RTC_DS1307_RTC; // To-do: deprecate, and use a features function
#endif // __arm__

extern void RTCset(const DateTime& dt);
extern DateTime RTCnow();
boolean RTChardware(void);

#endif // RTCLIB_H
