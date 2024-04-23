#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>

#define LED_PIN 6
#define LED_COUNT 180 // Total number of leds
#define RTC_SDA A4
#define RTC_SCL A5

const int pileLeds = 15; // Leds per pile
const int pileOfset = 2; // Pile offset

int receiveMonday = 0;
int processMonday = 0;
int currentDay = 1;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
RTC_DS3231 rtc;


void setup() {
  
  strip.begin();
  strip.show();

  Serial.begin(9600);


  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  DateTime now = rtc.now();
  strip.clear();
  
  // Get the current month and year
  int currentMonth = now.month();
  int currentYear = now.year();

  // Find the date of the first Monday of the current month
  int firstMonday = 0;
  for (int day = 1; day <= 7; day++) {
    DateTime date(currentYear, currentMonth, day, 0, 0, 0);
    if (date.dayOfTheWeek() == 1) {
      firstMonday = day;
      break;
    }
  }

  // Determine which week of the month today falls into
  int currentDay = now.day();
  int weekOfMonth = 0;
  if (currentDay >= firstMonday && currentDay < (firstMonday + 7))
    weekOfMonth = 1;
  else if (currentDay >= (firstMonday + 7) && currentDay < (firstMonday + 14))
    weekOfMonth = 2;
  else if (currentDay >= (firstMonday + 14) && currentDay < (firstMonday + 21))
    weekOfMonth = 3;
  else if (currentDay >= (firstMonday + 21) && currentDay < (firstMonday + 28))
    weekOfMonth = 4;
  else
    weekOfMonth = 5;

  // Print the week of the month
  Serial.print("Cuurrent Day: " + String(currentDay) + " Today falls into the ");
  switch (weekOfMonth) {
    case 1:
      Serial.println("1st Monday of the month.");
      receiveMonday = 1;
      processMonday = 2;
      break;
    case 2:
      Serial.println("2nd Monday of the month.");
      receiveMonday = 2;
      processMonday = 3;
      break;
    case 3:
      Serial.println("3rd Monday of the month.");
      receiveMonday = 3;
      processMonday = 4;
      break;
    case 4:
      Serial.println("4th/5th Monday of the month.");
      receiveMonday = 4;
      processMonday = 1;
      break;
    case 5:
      Serial.println("4th/5th Monday of the month.");
      receiveMonday = 4;
      processMonday = 1;
      break;
    default:
      Serial.println("Monday of the month.");
      break;
  }
  
  
  for (int i = 0; i < 4; i++) {
    
    int color[0][3] = {};
    
    color[0][0] = 150;
    color[0][1] = 0;
    color[0][2] = 0;  

    if ((i + 1) == receiveMonday ){

        color[0][0] = 0;
        color[0][1] = 150;
        color[0][2] = 0;

    }

    if ((i+ 1) == processMonday){

        color[0][0] = 0;
        color[0][1] = 0;
        color[0][2] = 150;
    }

        for (int j = 0; j < pileLeds; j++) {

            strip.setPixelColor((pileOfset * i) + (pileLeds * i) + j, color[0][0], color[0][1], color[0][2]); // Set the color of the LED
            int nextShelf = (pileOfset * 4) + (pileLeds * 4) + 4;
            strip.setPixelColor(nextShelf + (pileOfset * i) + (pileLeds * i) + j, color[0][0], color[0][1], color[0][2]); // Set the color of the second shelf LED  
    }



  }
  
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    currentDay = input.toInt();

    // Print the current date and time
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  }

  strip.show();
  
  
  delay(1000); // Wait for one second
  
}