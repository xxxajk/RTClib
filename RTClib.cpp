// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#include <RTClib.h>

#define DS1307_ADDRESS 0x68
#define SECONDS_PER_DAY 86400L

#define SECONDS_FROM_1970_TO_2000 946684800


////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t daysInMonth [] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //has to be const or compiler compaints

// number of days since 2000/01/01, valid for 2001..2099

static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
        if (y >= 2000)
                y -= 2000;
        uint16_t days = d;
        for (uint8_t i = 1; i < m; ++i)
                days += pgm_read_byte(daysInMonth + i - 1);
        if (m > 2 && y % 4 == 0)
                ++days;
        return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
        return ((days * 24L + h) * 60 + m) * 60 + s;
}


////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime(uint32_t t) {
        t -= SECONDS_FROM_1970_TO_2000; // bring to 2000 timestamp from 1970

        ss = t % 60;
        t /= 60;
        mm = t % 60;
        t /= 60;
        hh = t % 24;
        uint16_t days = t / 24;
        uint8_t leap;
        for (yOff = 0;; ++yOff) {
                leap = yOff % 4 == 0;
                if (days < 365 + leap)
                        break;
                days -= 365 + leap;
        }
        for (m = 1;; ++m) {
                uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
                if (leap && m == 2)
                        ++daysPerMonth;
                if (days < daysPerMonth)
                        break;
                days -= daysPerMonth;
        }
        d = days + 1;
}

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
        if (year >= 2000)
                year -= 2000;
        yOff = year;
        m = month;
        d = day;
        hh = hour;
        mm = min;
        ss = sec;
}

DateTime::DateTime(uint16_t fdate, uint16_t ftime) {
        /*
         * Time pre-packed for fat file system
         *
         *       bit31:25 year
         *       bit24:21 month
         *       bit20:16 day
         *       bit15:11 h
         *       bit10:5 m
         *       bit4:0 s (2 second resolution)
         */

        ss = (ftime & 31) << 1;
        mm = (ftime >> 5) & 63;
        hh = (ftime >> 11) & 31;
        d = fdate & 31;
        m = (fdate >> 5) & 15;
        yOff = (((fdate >> 9) & 127) + 1980) - 2000;
}

static uint8_t conv2d(const char* p) {
        uint8_t v = 0;
        if ('0' <= *p && *p <= '9')
                v = *p - '0';
        return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using PSTR would further reduce the RAM footprint

DateTime::DateTime(const char* date, const char* time) {
        // sample input: date = "Dec 26 2009", time = "12:34:56"
        yOff = conv2d(date + 9);
        // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
        switch (date[0]) {
                case 'J': m = date[1] == 'a' ? 1: m = date[2] == 'n' ? 6: 7;
                        break;
                case 'F': m = 2;
                        break;
                case 'A': m = date[2] == 'r' ? 4: 8;
                        break;
                case 'M': m = date[2] == 'r' ? 3: 5;
                        break;
                case 'S': m = 9;
                        break;
                case 'O': m = 10;
                        break;
                case 'N': m = 11;
                        break;
                case 'D': m = 12;
                        break;
        }
        d = conv2d(date + 4);
        hh = conv2d(time);
        mm = conv2d(time + 3);
        ss = conv2d(time + 6);
}

uint8_t DateTime::dayOfWeek() const {
        uint16_t day = date2days(yOff, m, d);
        return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

/**
 *
 * @return Time pre-packed for fat file system
 */

uint32_t DateTime::FatPacked(void) const {
        uint32_t t;
        /*
         * Time pre-packed for fat file system
         *
         *       bit31:25 year
         *       bit24:21 month
         *       bit20:16 day
         *       bit15:11 h
         *       bit10:5 m
         *       bit4:0 s (2 second resolution)
         */
        t = (ss >> 1) + ((uint32_t)mm << 5) + ((uint32_t)hh << 11) + ((uint32_t)d << 16) + ((uint32_t)m << 21) + ((uint32_t)((2000 + yOff) - 1980) << 25);
        return t;
}

uint32_t DateTime::unixtime(void) const {
        uint32_t t;
        uint16_t days = date2days(yOff, m, d);
        t = time2long(days, hh, mm, ss);
        t += SECONDS_FROM_1970_TO_2000; // seconds from 1970 to 2000

        return t;
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

static uint8_t bcd2bin(uint8_t val) {
        return val - 6 * (val >> 4);
}

static uint8_t bin2bcd(uint8_t val) {
        return val + 6 * (val / 10);
}

static uint8_t decToBcd(uint8_t val) {
        return ( (val / 10 * 16) + (val % 10));
}


// Warning Logic is reversed!

uint8_t RTC_DS1307::begin(const DateTime& dt) {
        Wire.beginTransmission(DS1307_ADDRESS);
        if (Wire.endTransmission()) return 0;
        return 1;
}

uint8_t RTC_DS1307::isrunning(void) {
        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE((uint8_t)0);
        if (Wire.endTransmission()) return 0;
        Wire.requestFrom(DS1307_ADDRESS, 1);
        uint8_t ss = WIREREAD();
        return !(ss >> 7);
}

uint8_t RTC_DS1307::adjust(const DateTime& dt) {
        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE((uint8_t)0);
        WIREWRITE(bin2bcd(dt.second()));
        WIREWRITE(bin2bcd(dt.minute()));
        WIREWRITE(bin2bcd(dt.hour()));
        WIREWRITE(bin2bcd(0));
        WIREWRITE(bin2bcd(dt.day()));
        WIREWRITE(bin2bcd(dt.month()));
        WIREWRITE(bin2bcd(dt.year() - 2000));
        WIREWRITE((uint8_t)0);
        return (!Wire.endTransmission());
}

uint8_t RTC_DS1307::set(int shour, int smin, int ssec, int sday, int smonth, int syear) {
        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE((uint8_t)0);
        WIREWRITE(decToBcd(ssec));
        WIREWRITE(decToBcd(smin));
        WIREWRITE(decToBcd(shour));
        WIREWRITE(decToBcd(0));
        WIREWRITE(decToBcd(sday));
        WIREWRITE(decToBcd(smonth));
        WIREWRITE(decToBcd(syear - 2000));
        WIREWRITE((uint8_t)0);
        return (!Wire.endTransmission());
}

// This is bad, can't report error.

DateTime RTC_DS1307::now() {
        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE((uint8_t)0);
        Wire.endTransmission();

        Wire.requestFrom(DS1307_ADDRESS, 7);
        uint8_t ss = bcd2bin(WIREREAD() & 0x7F);
        uint8_t mm = bcd2bin(WIREREAD());
        uint8_t hh = bcd2bin(WIREREAD());
        WIREREAD();
        uint8_t d = bcd2bin(WIREREAD());
        uint8_t m = bcd2bin(WIREREAD());
        uint16_t y = bcd2bin(WIREREAD()) + 2000;

        return DateTime(y, m, d, hh, mm, ss);
}

uint8_t RTC_DS1307::readMemory(uint8_t offset, uint8_t* data, uint8_t length) {
        uint8_t bytes_read = 0;

        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE(0x08 + offset);
        if (!Wire.endTransmission()) {

                Wire.requestFrom((uint8_t)DS1307_ADDRESS, (uint8_t)length);
                while (Wire.available() > 0 && bytes_read < length) {
                        data[bytes_read] = WIREREAD();
                        bytes_read++;
                }
        }
        return bytes_read;
}

uint8_t RTC_DS1307::writeMemory(uint8_t offset, uint8_t* data, uint8_t length) {
        uint8_t bytes_written;

        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE(0x08 + offset);
        bytes_written = WIREWRITE(data, length);
        if (Wire.endTransmission()) bytes_written = -1;

        return bytes_written;
}

Ds1307SqwPinMode RTC_DS1307::readSqwPinMode() {
        int mode;

        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE(0x07);
        Wire.endTransmission();

        Wire.requestFrom((uint8_t)DS1307_ADDRESS, (uint8_t)1);
        mode = WIREREAD();

        mode &= 0x93;
        return static_cast<Ds1307SqwPinMode>(mode);
}

void RTC_DS1307::writeSqwPinMode(Ds1307SqwPinMode mode) {
        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE(0x07);
        WIREWRITE(mode);
        Wire.endTransmission();
}


////////////////////////////////////////////////////////////////////////////////
// RTC_Millis implementation

long RTC_Millis::offset = 0;

uint8_t RTC_Millis::adjust(const DateTime& dt) {
        offset = dt.unixtime() - millis() / 1000;
        return 1;
}

DateTime RTC_Millis::now() {
        return (uint32_t)(offset + millis() / 1000);
}

uint8_t RTC_Millis::isrunning(void) {
        return offset ? 0 : 1;
}

////////////////////////////////////////////////////////////////////////////////
