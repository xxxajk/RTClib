// Date and time functions using a DS1307 RTC connected via I2C and Wire lib


#include <Wire.h> // Needed for arduino IDE
#include <RTClib.h>

void setup() {
        while(!Serial);
        Serial.begin(115200);
}

void loop() {
        DateTime now = RTCnow();
        Serial.print("                ");
        Serial.print(now.year(), DEC);
        Serial.print('/');
        if(now.month() < 10) Serial.print("0");
        Serial.print(now.month(), DEC);
        Serial.print('/');
        if(now.day() < 10) Serial.print("0");
        Serial.print(now.day(), DEC);
        Serial.print(' ');
        if(now.hour() < 10) Serial.print("0");
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        if(now.minute() < 10) Serial.print("0");
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        if(now.second() < 10) Serial.print("0");
        Serial.print(now.second(), DEC);

        Serial.print(" since midnight 1/1/1970 = ");
        Serial.print(now.unixtime());
        Serial.print("s = ");
        Serial.print(now.unixtime() / 86400L);
        Serial.println("d");

        // calculate a date which is 7 days and 30 seconds into the future
        DateTime future(now.secondstime() + (time_t)((7 * 86400L) + 30));

        Serial.print("now + 7d + 30s: ");
        Serial.print(future.year(), DEC);
        Serial.print('/');
        if(future.month() < 10) Serial.print("0");
        Serial.print(future.month(), DEC);
        Serial.print('/');
        if(future.day() < 10) Serial.print("0");
        Serial.print(future.day(), DEC);
        Serial.print(' ');
        if(future.hour() < 10) Serial.print("0");
        Serial.print(future.hour(), DEC);
        Serial.print(':');
        if(future.minute() < 10) Serial.print("0");
        Serial.print(future.minute(), DEC);
        Serial.print(':');
        if(future.second() < 10) Serial.print("0");
        Serial.print(future.second(), DEC);
        Serial.println();

        Serial.println();
        delay(1000);
}