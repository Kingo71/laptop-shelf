#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <RTClib.h> // Ensure Adafruit RTClib is installed

// --- Hardware Configuration ---
#define LED_PIN         6
#define LED_COUNT       180 // Total number of leds
#define LED_BRIGHTNESS  50  // Safety: 0-255. 180 LEDs can draw high current!

// --- Shelf Layout Configuration ---
const int pileLeds = 15;    // Leds per pile
const int pileOffset = 2;   // LED gap between piles
// Calculate shelf offset dynamically to prevent magic numbers
const int shelfOffset = (pileLeds * 4) + (pileOffset * 4) + 4;

// --- Global Objects ---
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
RTC_DS3231 rtc;

// --- State Variables ---
int receiveMonday = 0;
int processMonday = 0;
int currentDayCached = -1; // To track if date changed
unsigned long lastUpdate = 0;
const long updateInterval = 1000; // Update LEDs every 1 second
String inputBuffer = ""; // Buffer for incoming serial data

// --- Function Prototypes ---
void updateLedLogic(DateTime now);
void handleSerialInput();
void processSerialCommand(String command);
int calculateWeekOfMonth(DateTime now);
void setPileColor(int pileIndex, int r, int g, int b);
void printStatus();

void setup() {
  Serial.begin(9600);

  // Initialize LEDs
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.show(); // Initialize all pixels to 'off'

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println(F("Error: Couldn't find RTC"));
    // Flash red to indicate hardware failure
    strip.fill(strip.Color(255, 0, 0));
    strip.show();
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power, setting to compile time!"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  Serial.println(F("System Online."));
  Serial.println(F("To set time, type: YYYY-MM-DD HH:MM:SS"));
  Serial.println(F("To get status, type: STATUS"));
  Serial.println(F("Example: 2023-10-25 14:30:00"));
}

void loop() {
  // 1. Handle Serial Input (True Non-blocking accumulator)
  handleSerialInput();

  // 2. Handle Time-Based Logic (Non-blocking delay)
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;
    
    DateTime now = rtc.now();
    updateLedLogic(now);
  }
}

/**
 * Main Logic to determine LED colors based on time
 */
void updateLedLogic(DateTime now) {
  // Check Working Hours: Mon-Sat (1-6), 07:00 - 18:00
  // Note: Library dayOfTheWeek(): 0=Sun, 1=Mon, ... 6=Sat
  bool isWeekend = (now.dayOfTheWeek() == 0 || now.dayOfTheWeek() == 6); 
  bool isWorkingHours = (now.hour() >= 7 && now.hour() <= 18);

  // Debug Output
  Serial.print(F("Time: "));
  Serial.print(now.year()); Serial.print("-");
  Serial.print(now.month()); Serial.print("-");
  Serial.print(now.day()); Serial.print(" ");
  Serial.print(now.hour()); Serial.println(F(":XX"));

  if (isWeekend || !isWorkingHours) {
    Serial.println(F("Status: Outside working hours (LEDs OFF)"));
    strip.clear();
    strip.show();
    return;
  }

  // Only recalculate week logic if the day has changed to save cycles
  if (currentDayCached != now.day()) {
    currentDayCached = now.day();
    int weekOfMonth = calculateWeekOfMonth(now);

    Serial.print(F("New Day Detected. Week of Month: "));
    Serial.println(weekOfMonth);

    // Week Logic Mapping
    switch (weekOfMonth) {
      case 1: receiveMonday = 1; processMonday = 2; break;
      case 2: receiveMonday = 2; processMonday = 3; break;
      case 3: receiveMonday = 3; processMonday = 4; break;
      case 4: receiveMonday = 4; processMonday = 1; break;
      default: receiveMonday = 4; processMonday = 1; break; // 5th week acts as 4th
    }
  }

  strip.clear();

  // Render Piles (0 to 3)
  for (int i = 0; i < 4; i++) {
    // Default Color: Red (Dimmed)
    int r = 75, g = 0, b = 0;

    // Check Logic
    // Note: i is 0-indexed, logic vars are 1-indexed
    if ((i + 1) == receiveMonday) {
      r = 0; g = 75; b = 0; // Green
    }
    if ((i + 1) == processMonday) {
      r = 0; g = 0; b = 75; // Blue
    }

    setPileColor(i, r, g, b);
  }

  strip.show();
}

/**
 * Helper to paint a specific pile and its corresponding shelf below
 */
void setPileColor(int pileIndex, int r, int g, int b) {
  uint32_t color = strip.Color(r, g, b);

  // Calculate starting index for this pile on top shelf
  int startPixel = (pileOffset * pileIndex) + (pileLeds * pileIndex);

  // Loop through LEDs in this pile
  for (int j = 0; j < pileLeds; j++) {
    // Top Shelf
    strip.setPixelColor(startPixel + j, color);
    
    // Bottom Shelf (Mirrored position)
    // Note: Verify your shelfOffset logic physically matches the strip layout!
    strip.setPixelColor(shelfOffset + startPixel + j, color);
  }
}

/**
 * Calculates which week of the month the current date falls into
 * based on the First Monday logic.
 */
int calculateWeekOfMonth(DateTime now) {
  int currentMonth = now.month();
  int currentYear = now.year();
  int firstMondayDate = 0;

  // Find date of the first Monday
  for (int day = 1; day <= 7; day++) {
    DateTime date(currentYear, currentMonth, day, 0, 0, 0);
    if (date.dayOfTheWeek() == 1) { // 1 = Monday
      firstMondayDate = day;
      break;
    }
  }

  int dom = now.day(); // Day of Month

  // Determine week bracket
  if (dom >= firstMondayDate && dom < (firstMondayDate + 7)) return 1;
  if (dom >= (firstMondayDate + 7) && dom < (firstMondayDate + 14)) return 2;
  if (dom >= (firstMondayDate + 14) && dom < (firstMondayDate + 21)) return 3;
  if (dom >= (firstMondayDate + 21) && dom < (firstMondayDate + 28)) return 4;
  
  return 5;
}

/**
 * Accumulates serial characters into a buffer.
 * Processes command only when Newline is received.
 */
void handleSerialInput() {
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    
    if (inChar == '\n') {
      processSerialCommand(inputBuffer);
      inputBuffer = ""; // Clear buffer
    } else if (inChar != '\r') { 
      // Append if not a carriage return
      inputBuffer += inChar;
    }
  }
}

/**
 * Parses the command string using sscanf for flexibility
 * Expected format: "YYYY-MM-DD HH:MM:SS"
 */
void processSerialCommand(String command) {
  command.trim(); // Remove leading/trailing whitespace

  // Handle STATUS command
  if (command.equalsIgnoreCase("STATUS")) {
    printStatus();
    return;
  }

  int year, month, day, hour, minute, second;

  // sscanf parses the C-string. It ignores extra whitespace automatically.
  // Returns number of successfully matched variables.
  int n = sscanf(command.c_str(), "%d-%d-%d %d:%d:%d", 
                 &year, &month, &day, &hour, &minute, &second);

  if (n == 6) {
    // Basic range validation
    if (year < 2020 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31) {
      Serial.println(F("Error: Invalid date range."));
    } else {
      rtc.adjust(DateTime(year, month, day, hour, minute, second));
      Serial.print(F("Success: RTC updated to "));
      Serial.println(command);
      
      // Force immediate logic update
      lastUpdate = 0; 
      currentDayCached = -1; 
    }
  } else {
    Serial.println(F("Error: parsing failed."));
    Serial.println(F("Format: YYYY-MM-DD HH:MM:SS or STATUS"));
  }
}

/**
 * Prints the current system status as a JSON string
 */
void printStatus() {
  DateTime now = rtc.now();
  
  Serial.print(F("{"));
  
  // Time
  Serial.print(F("\"datetime\":\""));
  Serial.print(now.year()); Serial.print("-");
  if(now.month() < 10) Serial.print("0"); Serial.print(now.month()); Serial.print("-");
  if(now.day() < 10) Serial.print("0"); Serial.print(now.day()); Serial.print(" ");
  if(now.hour() < 10) Serial.print("0"); Serial.print(now.hour()); Serial.print(":");
  if(now.minute() < 10) Serial.print("0"); Serial.print(now.minute()); Serial.print(":");
  if(now.second() < 10) Serial.print("0"); Serial.print(now.second());
  Serial.print(F("\","));

  // Logic
  bool isWeekend = (now.dayOfTheWeek() == 0 || now.dayOfTheWeek() == 6); 
  bool isWorkingHours = (now.hour() >= 7 && now.hour() <= 18);
  bool ledsActive = !isWeekend && isWorkingHours;

  Serial.print(F("\"leds_active\":"));
  Serial.print(ledsActive ? "true" : "false");
  Serial.print(F(","));

  // Pile Status
  Serial.print(F("\"receive_pile\":"));
  Serial.print(receiveMonday);
  Serial.print(F(","));

  Serial.print(F("\"process_pile\":"));
  Serial.print(processMonday);

  Serial.println(F("}"));
}