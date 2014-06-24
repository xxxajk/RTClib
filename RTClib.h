// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)

#ifndef RTCLIB_H
#define RTCLIB_H

// TO-DO: Still allow (and prefer) DS chip.
#if !defined(__arm__) || defined(ARDUINO_SAM_DUE)
#define DS1307_ADDRESS 0x68
#endif // ! __arm__


#ifdef	__cplusplus
#if defined(DS1307_ADDRESS)
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
#if defined(__AVR__)
        // Overload needed because we use 64bits, not 32.
        DateTime(time_t t = 0);
#endif
        DateTime(int32_t t);
        DateTime(uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
        DateTime(const char* date, const char* time);
        DateTime(uint16_t fdate, uint16_t ftime);

        uint16_t year(void) const {

#ifdef __arm__
                // arm uses a different epoch
                return _time.tm_year + 1930;
#else
                return _time.tm_year + 1900;
#endif
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
        struct tm _time; // since 1/1/1900
};


#if defined(DS1307_ADDRESS)
// RTC based on the DS1307 chip connected via I2C and the Wire library

enum SqwPinMode {
        SquareWaveOFF   = 0x00,
        SquareWaveON    = 0x80,
        SquareWave1HZ   = 0x10,
        SquareWave4kHz  = 0x11,
        SquareWave8kHz  = 0x12,
        SquareWave32kHz = 0x13
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
        static SqwPinMode readSqwPinMode();
        static void writeSqwPinMode(SqwPinMode mode);
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
#else //  defined(DS1307_ADDRESS)
enum SqwPinMode {
        SquareWaveOFF   = 0,
        SquareWaveON    = 0x80,
        SquareWave1HZ   = 1,
        SquareWave32kHz = 2
};
#endif //  defined(DS1307_ADDRESS)

extern void RTCset(const DateTime& dt);
extern DateTime RTCnow();
boolean RTChardware(void);
uint8_t RTCreadMemory(uint8_t offset, uint8_t* data, uint8_t length);
uint8_t RTCwriteMemory(uint8_t offset, uint8_t* data, uint8_t length);
SqwPinMode RTCreadSqwPinMode();
void RTCwriteSqwPinMode(SqwPinMode mode);


extern "C" {
#endif
        extern void RTC_systime(void);
#ifdef	__cplusplus
}
#endif

#endif // RTCLIB_H
