// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)

#ifndef RTCLIB_H
#define RTCLIB_H
#include <Wire.h>
#include <avr/pgmspace.h>
#if (ARDUINO >= 100)
#include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems
#define WIREWRITE Wire.write
#define WIREREAD Wire.read
#else
#include <WProgram.h>
#define WIREWRITE Wire.send
#define WIREREAD Wire.receive
#endif

class DateTime {
public:
        DateTime(uint32_t t = 0);
        DateTime(uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
        DateTime(const char* date, const char* time);
        DateTime(uint16_t fdate, uint16_t ftime);

        uint16_t year() const {
                return 2000 + yOff;
        }

        uint8_t month() const {
                return m;
        }

        uint8_t day() const {
                return d;
        }

        uint8_t hour() const {
                return hh;
        }

        uint8_t minute() const {
                return mm;
        }

        uint8_t second() const {
                return ss;
        }
        uint8_t dayOfWeek() const;

        // 32-bit times as seconds since 1/1/2000
        long secondstime() const;
        // 32-bit times as seconds since 1/1/1970
        uint32_t unixtime(void) const;
        // time packed
        uint32_t FatPacked(void) const;

protected:
        uint8_t yOff, m, d, hh, mm, ss;
};

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
        static long offset;
};
#endif // RTCLIB_H
