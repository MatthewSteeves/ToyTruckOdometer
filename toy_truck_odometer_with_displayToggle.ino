// Low-Power - Version: Latest
#include <LowPower.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/io.h>

const int REV_COUNTER_PIN   = 2;
const int DISPLAY_MODE_PIN  = 3;
const int LED_BACKLIGHT_PIN = 10;
const int WHEEL_CIRCUM      = 17;

volatile long distance_odometer = 0; // (inches) Eventually to be retrieved from/stored in EEPROM
volatile long distance_trip     = 0; // (inches) Eventually to be retrieved from/stored in EEPROM
long distance_current;

volatile unsigned long time_last_active; // ERR ERR ERR Needs interrupt disabling when accessed because can't be read in 1 op
volatile unsigned int rev_count;
unsigned int prev_rev_count;

char* display_mode[] = { "Odometer", "Trip" };
const int ODOMETER = 0;
const int TRIP     = 1;
int cur_display_mode = ODOMETER;
String message;

// See the .PNG file for the wiring diagram
const int RS = 9, EN = 8, D4 = 4, D5 = 5, D6 = 6, D7 = 7;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);


void setup()
{
  Serial.begin(9600);

  pinMode(LED_BACKLIGHT_PIN, OUTPUT);
  pinMode(REV_COUNTER_PIN, INPUT);
//  pinMode(DISPLAY_MODE_PIN, INPUT);

  digitalWrite(LED_BACKLIGHT_PIN, HIGH);

  attachInterrupt(digitalPinToInterrupt(REV_COUNTER_PIN), processRev, FALLING);
//  attachInterrupt(digitalPinToInterrupt(DISPLAY_MODE_PIN), processDisplayModeButton, RISING);
  
  // Set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  update_lcd(String( String(distance_odometer / 12) + " feet"));
  
  // So that we don't go to sleep right away
  time_last_active = millis();
}


void loop()
{
  // Go to sleep if not active within past 5 seconds
  if ((millis() - time_last_active) > 5000) {
    Serial.println("About to go to sleep");
    Serial.println("time_now: " + String(millis()));
    Serial.println("time_last_active: " + String(time_last_active));
    delay(250); // Give serial time to print
    go_to_sleep();
  }

  if (rev_count != prev_rev_count) {
    Serial.println(rev_count);
    prev_rev_count = rev_count;

    if (cur_display_mode == ODOMETER) {
      distance_current = distance_odometer;
    }
    else {
      distance_current = distance_odometer - distance_trip;
    }

    update_lcd(String( String(distance_current / 12) + " feet"));
    
  }
}



// ************************************************
// SUBROUTINES
// ************************************************

void processRev()
{
  // Use time comparison to avoid false positives due to fluctating magnetic field
  // Assume minimum time between sensor triggers is 100 millis (derived from est top speed)
  if ((millis() - time_last_active) > 80) {
    rev_count++;
    distance_odometer += WHEEL_CIRCUM;
  }

  time_last_active = millis();
}


void processDisplayModeButton() {
  delay(400); //software debounce

  if (bit_is_set(_SLEEP_CONTROL_REG, SE)) {
    Serial.println("Display mode button pushed while sleeping");
  }
  else {
    if (cur_display_mode == ODOMETER) {
      cur_display_mode = TRIP;
      distance_current = distance_odometer - distance_trip;
    }
    else {
      cur_display_mode = ODOMETER;
      distance_current = distance_odometer;
    }
  }
 
  update_lcd(String( String(distance_current / 12, 1) + " feet"));

  // Refresh time till sleep since the user requested some info
  time_last_active = millis();
}


void update_lcd(String message) {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(display_mode[cur_display_mode]);
  lcd.setCursor(0, 1);
  lcd.print(message);
}


void go_to_sleep() {
  lcd.noDisplay(); // turn off text display
  digitalWrite(LED_BACKLIGHT_PIN, LOW);

  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

  // Upon waking up, sketch continues from this point AFTER
  // servicing whatever ISR triggered the wakeup
  digitalWrite(LED_BACKLIGHT_PIN, HIGH);
  lcd.display();
}
