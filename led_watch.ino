#include <Arduino.h>
#include <math.h>
#include "ShiftRegister74HC595.h"
#if defined(ESP8266)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif
#include <Wire.h>  // must be incuded here so that Arduino library object file references work
#include <RtcDS1307.h>

RtcDS1307 Rtc;

#define HOUR_COLOR 1
#define MINUTE_COLOR 2
#define EMPTY_COLOR 0
#define BRIGHTNESS 240

// create shift register object (number of shift registers, data pin, clock pin, latch pin)
ShiftRegister74HC595 sr (4, 11, 12, 8);

byte shiftColors[3][12] = {
  {17, 3, 4, 16, 7, 255, 29, 20, 8, 26, 10, 15}, //red
  {18, 2, 5, 31, 0, 255, 28, 21, 9, 25, 11, 14}, //green
  {19, 1, 6, 30, 255, 255, 27, 22, 23, 24, 12, 13} //blue
};

byte pwmColors[3][6] = {
  {255, 255, 255, 255, 255, 9},
  {255, 255, 255, 255, 255, 6},
  {255, 255, 255, 255, 3, 5}
};

void setLed(byte n, byte color, byte state) {
  byte pin = shiftColors[color][n];
  if(pin == 255) {
    pin = pwmColors[color][n];
    if(state) {
      analogWrite(pin, 255 - BRIGHTNESS);
    } else {
      digitalWrite(pin, state);
    }
  } else {
    sr.set(pin, state);
  }
}

void setAllLedOff() {
  sr.setAllLow();
  digitalWrite(9, LOW);
  digitalWrite(6, LOW);
  digitalWrite(5, LOW);
  digitalWrite(3, LOW);
}

byte hour = 255;
byte minute = 255;

byte tHour = 0;
byte tMin = 0;

void setup() {
  Serial.begin(9600);
  sr.setAllLow();

  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(7, OUTPUT);
  
  pinMode(2, INPUT);
  pinMode(4, INPUT);
  

  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!Rtc.IsDateTimeValid()) 
  {
      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
  }

  if (!Rtc.GetIsRunning())
  {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) 
  {
      Serial.println("RTC is older than compile time!  (Updating DateTime)");
      Rtc.SetDateTime(compiled);
  }

  analogWrite(10, BRIGHTNESS);

  hour = now.Hour() % 12;
  minute = (round(((double)now.Minute()) / 5)) % 12;

  if(now.Hour() >= 0 && now.Hour() < 12) {
    for(byte i=0;i<12;i++) {
      setLed(i, EMPTY_COLOR, 1);
    }
  }

  setLed(hour, EMPTY_COLOR, 0);
  setLed(hour, HOUR_COLOR, 1);
  setLed(minute, EMPTY_COLOR, 0);
  setLed(minute, MINUTE_COLOR, 1);

  printDateTime(now);
  Serial.println();
  Serial.println("setup end");
}


bool beep = false;
bool buttonDelay = false;


void loop() {
  
  RtcDateTime now = Rtc.GetDateTime();

  if(digitalRead(2)) {
    //plus hour
    now += 3600;
    Rtc.SetDateTime(now);
    buttonDelay = true;
  }

  if(digitalRead(4)) {
    now += 300;
    Rtc.SetDateTime(now);
    buttonDelay = true;
  }
  
  tHour = now.Hour() % 12;
  tMin = (round(((double)now.Minute()) / 5)) % 12;

  if(tMin != minute) {
    setLed(minute, MINUTE_COLOR, 0);
    if(minute != hour) {
      if(now.Hour() >= 0 && now.Hour() < 12) {
        setLed(minute, EMPTY_COLOR, 1);
      }
    }
    minute = tMin;
    setLed(minute, EMPTY_COLOR, 0);
    setLed(minute, MINUTE_COLOR, 1);
  }

  if(tHour != hour) {
    setLed(hour, HOUR_COLOR, 0);
    if(minute != hour) {
      if(now.Hour() >= 0 && now.Hour() < 12) {
        setLed(hour, EMPTY_COLOR, 1);
      }
    }
    hour = tHour;
    setLed(hour, EMPTY_COLOR, 0);
    setLed(hour, HOUR_COLOR, 1);

    if(now.Hour() == 0) {
      for(byte i=0;i<12;i++) {
        if(i != hour && i != minute)
          setLed(i, EMPTY_COLOR, 1);
      }
    } else if(now.Hour() == 12) {
      for(byte i=0;i<12;i++) {
        setLed(i, EMPTY_COLOR, 0);
      }
    }
  }
  
  if(minute == 0 && !beep && !buttonDelay && now.Hour() <= 23 && now.Hour() >= 9) {

    for(byte i=0;i<hour;i++) {
      digitalWrite(7, HIGH);
      if(i == hour - 1) {
        delay(500);
      } else {
        delay(100);
      }
      digitalWrite(7, LOW);
      delay(2000);
      
    }

    beep = true;
  } else if(minute != 0) {
    beep = false;
  }

  if(buttonDelay) {
    delay(500);
    buttonDelay = false;
    beep = true;
  }

  /*for(byte col=0;col<3;col++) {
    for(byte i=0;i<12;i++) {
      setLed(i, col, 1);
      delay(50);
      setLed(i, col, 0);
      delay(50);
    }
  }

  delay(5000);*/
 
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u.%02u.%04u %02u:%02u:%02u"),
            dt.Day(),
            dt.Month(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

