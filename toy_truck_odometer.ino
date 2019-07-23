// Low-Power - Version: Latest
#include <LowPower.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/io.h>

const int REV_COUNTER_PIN   = 2;
const int REV_HALFWAY_PIN   = 3;
const int LED_BACKLIGHT_PIN = 10;
const int WHEEL_CIRCUM      = 17;
const int EE_ADDRESS        = 0; // EEPROM memory location where odometer storage begins

unsigned long distance_odometer; // (inches) 
volatile unsigned long time_last_active;

volatile bool rev_happened         = false;
volatile bool halfway_point_passed = false;

String message;

// See the .PNG file for the wiring diagram
const int RS = 9, EN = 8, D4 = 4, D5 = 5, D6 = 6, D7 = 7;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);


void setup()
{
  Serial.begin(9600);

  pinMode(LED_BACKLIGHT_PIN, OUTPUT);
  pinMode(REV_COUNTER_PIN, INPUT);
  pinMode(REV_HALFWAY_PIN, INPUT);

  digitalWrite(LED_BACKLIGHT_PIN, HIGH);

  attachInterrupt(digitalPinToInterrupt(REV_COUNTER_PIN), processRev, FALLING);
  attachInterrupt(digitalPinToInterrupt(REV_HALFWAY_PIN), processHalfwayPoint, FALLING);

  // Get the stored distance
  EEPROM.get(EE_ADDRESS, distance_odometer);
  
  // Set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  update_lcd(String( String(distance_odometer / 12) + " feet"));
  
  // So that we don't go to sleep right away
  time_last_active = millis();
}

long time_now;
void loop()
{
  // Go to sleep if not active within past 5 seconds
  if ((millis() - time_last_active) > 5000) {
    /*
    Serial.println("About to go to sleep");
    time_now = millis();
    Serial.print("time_now - time_last_active(ms): ");
    Serial.println(time_now - time_last_active);
    delay(250); // Give serial time to print
    */
    go_to_sleep();
  }

  if (rev_happened) {
    rev_happened = false;   // Reset for next wheel revolution
     
    distance_odometer += WHEEL_CIRCUM;

//    Serial.print("rev_happened: ");
//    Serial.print(distance_odometer);
//    Serial.println(" inches");
    
    update_lcd(String( String(distance_odometer / 12) + " feet"));
    
  }
}



// ************************************************
// SUBROUTINES
// ************************************************

void processRev() {
  if (halfway_point_passed) {
      rev_happened = true;
      halfway_point_passed = false; // prep for next rotation
  }
  
  // Refresh time till sleep since the wheel is moving
  time_last_active = millis();
}


void processHalfwayPoint() {
  halfway_point_passed = true;
  
  // Refresh time till sleep since the wheel is moving
  time_last_active = millis();
}


void update_lcd(String message) {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Odometer");
  lcd.setCursor(0, 1);
  lcd.print(message);
}


void go_to_sleep() {
  lcd.noDisplay(); // turn off text display
  digitalWrite(LED_BACKLIGHT_PIN, LOW);
  EEPROM.put(EE_ADDRESS, distance_odometer); // Write out current distance to memory in case of power loss

  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

  // Upon waking up, sketch continues from this point AFTER
  // servicing whatever ISR triggered the wakeup
  digitalWrite(LED_BACKLIGHT_PIN, HIGH);
  lcd.display();
}
