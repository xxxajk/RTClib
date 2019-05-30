// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#ifndef ARDUINO
#define RTCLIB_INCLUDER
#include <RTClib.h>
#endif

#ifdef RTCLIB_INCLUDER
////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

#ifdef __AVR__
// 64bit time_t -> struct tm

DateTime::DateTime(time_t t) {
        gmtime_r((const time_t *)&t, &_time);
}
#endif

uint16_t DateTime::year(void) {

#ifdef __arm__
        // arm uses a different epoch
        return _time.tm_year + 1930;
#else
        return _time.tm_year + 1900;
#endif
}

uint8_t DateTime::month(void) {
        return _time.tm_mon;
}

uint8_t DateTime::day(void) {
        return _time.tm_mday;
}

uint8_t DateTime::hour(void) {
        return _time.tm_hour;
}

uint8_t DateTime::minute(void) {
        return _time.tm_min;
}

uint8_t DateTime::second(void) {
        return _time.tm_sec;
}

uint8_t DateTime::dayOfWeek(void) {
        return _time.tm_wday;
}

// 32bit time_t -> struct tm

DateTime::DateTime(int32_t t) {
        time_t x = t;
        gmtime_r((const time_t *)&x, &_time);
}

// YY/MM/DD/HH/mm/SS -> struct tm

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
        _time.tm_year = year - 1900;
        _time.tm_mon = month;
        _time.tm_mday = day;
        _time.tm_hour = hour;
        _time.tm_min = min;
        _time.tm_sec = sec;
        _time.tm_isdst = 0;
        mktime(&_time);
}

// fat time -> struct tm

DateTime::DateTime(uint16_t fdate, uint16_t ftime) {
        /*
         * Time pre-packed for fat file system
         *
         *       7 bit31:25 year
         *       4 bit24:21 month
         *       5 bit20:16 day
         * ------------------------
         *       5 bit15:11 h
         *       6 bit10:5 m
         *       5 bit4:0 s (2 second resolution)
         */
#if defined(__arm__) && defined(CORE_TEENSY) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)
        // Messed up EPOCH
        _time.tm_year = ((fdate >> 9) & 0x7f) + 50;
#else
        _time.tm_year = (1980 + ((fdate >> 9) & 0x7f)) - 1900;
#endif
        _time.tm_mon = (fdate >> 5) & 0x0f;
        _time.tm_mday = fdate & 0x1f;
        _time.tm_hour = (ftime >> 11) & 0x1f;
        _time.tm_min = (ftime >> 5) & 0x3f;
        _time.tm_sec = (ftime & 0x1f) *2;
        _time.tm_isdst = 0;
        mktime(&_time);
}

static uint8_t conv2d(const char* p) {
        uint8_t v = 0;
        if('0' <= *p && *p <= '9')
                v = *p - '0';
        return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using PSTR would further reduce the RAM footprint
// NOTE: This is a short version of strptime

DateTime::DateTime(const char* date, const char* time) {
        // sample input: date = "Dec 26 2009", time = "12:34:56"
        uint8_t m = 0;
        _time.tm_year = ((conv2d(date + 7)*100) + conv2d(date + 9)) - 1900;
        switch(date[0]) {
                case 'J': m = date[1] == 'a' ? 1: (date[2] == 'n' ? 6: 7);
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
        _time.tm_mon = m;
        _time.tm_mday = conv2d(date + 4);
        _time.tm_hour = conv2d(time);
        _time.tm_min = conv2d(time + 3);
        _time.tm_sec = conv2d(time + 6);
        _time.tm_isdst = 0;
        mktime(&_time);
}

// NOTE! teensy3.0 uses unix time. So we need to reverse a few things, and do extra processing on ARM

/**
 *
 * @return Time pre-packed for fat file system.
 * Does not work after the year 2107.
 */

time_t DateTime::FatPacked(void) {
#if !defined(DS1307_ADDRESS)
        DateTime x = DateTime(this->secondstime());
        return fatfs_time(&x._time);
#else
        return fatfs_time(&_time);
#endif
}

/**
 *
 * @return Time in seconds since 1/1/1970
 */

time_t DateTime::unixtime(void) {
#if !defined(DS1307_ADDRESS)
        return mk_gmtime(&_time);
#else
        return (mk_gmtime(&_time) + UNIX_OFFSET);
#endif
}

/**
 *
 * @return Time in seconds since 1/1/1900
 */
time_t DateTime::secondstime(void) {
#if !defined(DS1307_ADDRESS)
        return (mk_gmtime(&_time) + UNIX_OFFSET);
#else
        return mk_gmtime(&_time);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

#if defined(DS1307_ADDRESS)

static uint8_t bcd2bin(uint8_t val) {
        return val - 6 * (val >> 4);
}

static uint8_t bin2bcd(uint8_t val) {
        return val + 6 * (val / 10);
}

static uint8_t decToBcd(uint8_t val) {
        return ( (val / 10 * 16) + (val % 10));
}


uint8_t RTC_DS1307::begin(const DateTime& dt __attribute__((unused))) {
        XMEM_ACQUIRE_I2C();
        Wire.beginTransmission(DS1307_ADDRESS);
        uint8_t x = Wire.endTransmission();
        XMEM_RELEASE_I2C();
        if(x) return 0;
        return 1;
}

uint8_t RTC_DS1307::isrunning(void) {
        XMEM_ACQUIRE_I2C();
        uint8_t ss = 0;
        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE((uint8_t)0);
        if(!Wire.endTransmission()) {
                Wire.requestFrom(DS1307_ADDRESS, 1);
                ss = WIREREAD();
                ss = !((ss >> 7) &0x01);
        }
        XMEM_RELEASE_I2C();
        return ss;
}

uint8_t RTC_DS1307::adjust(DateTime& dt) {
        XMEM_ACQUIRE_I2C();
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
        uint8_t x = Wire.endTransmission();
        XMEM_RELEASE_I2C();
        return (!x);
}

uint8_t RTC_DS1307::set(int shour, int smin, int ssec, int sday, int smonth, int syear) {
        XMEM_ACQUIRE_I2C();
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
        uint8_t x = Wire.endTransmission();
        XMEM_RELEASE_I2C();
        return (!x);
}

// This is bad, can't report error.

DateTime RTC_DS1307::now() {
        XMEM_ACQUIRE_I2C();
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
        XMEM_RELEASE_I2C();

        return DateTime(y, m, d, hh, mm, ss);
}

uint8_t RTC_DS1307::readMemory(uint8_t offset, uint8_t* data, uint8_t length) {
        XMEM_ACQUIRE_I2C();
        uint8_t bytes_read = 0;

        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE(0x08 + offset);
        if(!Wire.endTransmission()) {

                Wire.requestFrom((uint8_t)DS1307_ADDRESS, (uint8_t)length);
                while(Wire.available() > 0 && bytes_read < length) {
                        data[bytes_read] = WIREREAD();
                        bytes_read++;
                }
        }
        XMEM_RELEASE_I2C();
        return bytes_read;
}

uint8_t RTC_DS1307::writeMemory(uint8_t offset, uint8_t* data, uint8_t length) {
        XMEM_ACQUIRE_I2C();

        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE(0x08 + offset);
        uint8_t bytes_written = WIREWRITE(data, length);
        if(Wire.endTransmission()) bytes_written = -1;
        XMEM_RELEASE_I2C();
        return bytes_written;
}

SqwPinMode RTC_DS1307::readSqwPinMode() {
        XMEM_ACQUIRE_I2C();
        int mode;

        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE(0x07);
        Wire.endTransmission();

        Wire.requestFrom((uint8_t)DS1307_ADDRESS, (uint8_t)1);
        mode = WIREREAD();
        XMEM_RELEASE_I2C();

        mode &= 0x93;
        return static_cast<SqwPinMode>(mode);
}

void RTC_DS1307::writeSqwPinMode(SqwPinMode mode) {
        XMEM_ACQUIRE_I2C();
        Wire.beginTransmission(DS1307_ADDRESS);
        WIREWRITE(0x07);
        WIREWRITE(mode);
        Wire.endTransmission();
        XMEM_RELEASE_I2C();
}

////////////////////////////////////////////////////////////////////////////////
//
// Add more RTC as needed.
//
////////////////////////////////////////////////////////////////////////////////
RTC_DS1307 RTC_DS1307_RTC;
static bool WireStarted = false;
#endif // defined(DS1307_ADDRESS)

////////////////////////////////////////////////////////////////////////////////
// RTC_Millis implementation
static bool RTC_MillisStarted = false;
int64_t RTC_Millis::offset = 0;

uint8_t RTC_Millis::begin(DateTime& dt) {
                return(adjust(dt));
        }

uint8_t RTC_Millis::adjust(DateTime& dt) {
        XMEM_ACQUIRE_I2C(); // Yes, this is not I2C, but we should make this safe
        offset = dt.secondstime() - millis() / 1000;
        XMEM_RELEASE_I2C();
        return 1;
}

DateTime RTC_Millis::now() {
        return DateTime((time_t)(offset + millis() / 1000));
}

uint8_t RTC_Millis::isrunning(void) {
        return offset ? 0 : 1;
}
RTC_Millis RTC_ARDUINO_MILLIS_RTC;

void RTCstart(void) {
        DateTime x = DateTime(__DATE__, __TIME__);
        if(!RTC_MillisStarted) {
                RTC_MillisStarted = true;
                RTC_ARDUINO_MILLIS_RTC.begin(x);
        }
#if defined(DS1307_ADDRESS)
        if(!WireStarted) {
                WireStarted = true;
                Wire.begin();
                if(!RTC_DS1307_RTC.isrunning())
                        RTC_DS1307_RTC.adjust(x);
                // Add more RTC as needed.
        }
#endif
}

void RTCset(DateTime& dt) {
#if defined(__arm__) && defined(CORE_TEENSY) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)
        rtc_set(dt.unixtime());
#else
        RTCstart(); // Automatic.
        RTC_ARDUINO_MILLIS_RTC.adjust(dt); // Always present.
#if defined(DS1307_ADDRESS)
        if(RTC_DS1307_RTC.isrunning()) {
                RTC_DS1307_RTC.adjust(dt);
        }
#endif
        // Add more RTC as needed.
#endif
}

DateTime RTCnow(void) {
#if defined(__arm__) && defined(CORE_TEENSY) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)
        return DateTime(rtc_get());
#else
        RTCstart(); // Automatic.
#if defined(DS1307_ADDRESS)
        if(RTC_DS1307_RTC.isrunning()) return RTC_DS1307_RTC.now();
#endif
        // Add more RTC as needed.
        return RTC_ARDUINO_MILLIS_RTC.now();
#endif
}

bool RTChardware(void) {
#if defined(__arm__) && defined(CORE_TEENSY) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)
        return true;
#else
        RTCstart(); // Automatic.
#if defined(DS1307_ADDRESS)
        if(RTC_DS1307_RTC.isrunning()) return true;
#endif
        // Add more RTC as needed.
        return false;
#endif
}

bool RTChasRAM(void) {
#if defined(__arm__) && defined(CORE_TEENSY) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)
        return false;
#else
        RTCstart(); // Automatic.
#if defined(DS1307_ADDRESS)
        if(RTC_DS1307_RTC.isrunning()) return true;
#endif
        // Add more RTC as needed.
        return false;
#endif

}

uint8_t RTCreadMemory(uint8_t offset, uint8_t* data, uint8_t length) {
#if defined(__arm__) && defined(CORE_TEENSY) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)
        return 0;
#else
#if defined(DS1307_ADDRESS)
        if(RTChardware())
                return RTC_DS1307_RTC.readMemory(offset, data, length);
        else
#endif
#endif
                return 0;
}

uint8_t RTCwriteMemory(uint8_t offset, uint8_t* data, uint8_t length) {
#if defined(__arm__) && defined(CORE_TEENSY) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)
        return 0;
#else
#if defined(DS1307_ADDRESS)
        if(RTChardware())
                return RTC_DS1307_RTC.writeMemory(offset, data, length);
        else
#endif
                return 0;

#endif
}

SqwPinMode RTCreadSqwPinMode() {
#if defined(__arm__) && defined(CORE_TEENSY) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)
        if((*portConfigRegister(31) & 0x00000700UL) == 0x00000700UL) {
                if(SIM_SOPT2 & SIM_SOPT2_RTCCLKOUTSEL) return SquareWave32kHz;
                return SquareWave1HZ;
        } else {
                if(digitalRead(31)) return SquareWaveON;
        }
#else
#if defined(DS1307_ADDRESS)
        if(RTChardware()) return RTC_DS1307_RTC.readSqwPinMode();
        else
#endif
#endif
                return SquareWaveOFF;

}

void RTCwriteSqwPinMode(SqwPinMode mode) {
#if defined(__arm__) && defined(CORE_TEENSY) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)

        switch(mode) {
                case SquareWaveOFF:
                        pinMode(31, OUTPUT);
                        digitalWriteFast(31, LOW);
                        break;
                case SquareWaveON:
                        pinMode(31, OUTPUT);
                        digitalWriteFast(31, HIGH);
                        break;
                case SquareWave1HZ:
                        pinMode(31, OUTPUT); // Set with a known default.
                        // This is an 'advanced' pin mode. Set to Alt 7 (RTC_CLK output)
                        *portConfigRegister(31) = (*portConfigRegister(31) & 0xFFFFF8FFUL) | 0x00000700UL;

                        SIM_SOPT2 = (SIM_SOPT2 & (~(SIM_SOPT2_CLKOUTSEL(7) | SIM_SOPT2_RTCCLKOUTSEL))) | SIM_SOPT2_CLKOUTSEL(5);
                        break;
                case SquareWave32kHz:
                        pinMode(31, OUTPUT); // Set with a known default.
                        // This is an 'advanced' pin mode. Set to Alt 7 (RTC_CLK output)
                        *portConfigRegister(31) = (*portConfigRegister(31) & 0xFFFFF8FFUL) | 0x00000700UL;

                        SIM_SOPT2 = (SIM_SOPT2 & (~SIM_SOPT2_CLKOUTSEL(7))) | SIM_SOPT2_CLKOUTSEL(5) | SIM_SOPT2_RTCCLKOUTSEL;
                        break;
        }
#else
#if defined(DS1307_ADDRESS)
        if(RTChardware()) RTC_DS1307_RTC.writeSqwPinMode(mode);
#endif
#endif
}

// C interface... Yes, really... We just call this before using time()
extern "C" {

        void RTC_systime(void) {
                set_system_time(RTCnow().unixtime());
        }
}
#endif

#ifndef RTC_LOADED
#define RTC_LOADED
#endif
