//**************************************************
/*
  Author: Habid Rascon-Ramos
  Project: Arduino Electronic Load
  Date: March 3, 2016
  Version: 2
  Description: Temperature alarm using the Arduino
*/
//**************************************************

// Included libraries
#include <Button.h>
#include <LiquidCrystal.h>

// Pin definitions
#define TEMP_PIN A0
#define BUTTON_PIN 6
#define SECOND_BUTTON 10

// Set integer values
int adc = 0;

// Set double values
double tempF = 0.0;
double tempC = 0.0;
double WARN_TEMP_LIMIT = 78;
double OVER_TEMP_LIMIT = 80;
double overTempC = 0.0;

// Set timer values
unsigned long BASE_TIMEOUT = 1000; // Time set to 1s for system. Time in milliseconds.
unsigned long baseTimedOut = 1000;

//Set boolean values
boolean displayCelsius = false;
boolean buttonFlag = false;
boolean alarmFlag = false;

// Set LCD screen and buttons
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
Button button = Button(BUTTON_PIN, BUTTON_PULLUP);
Button secondButton = Button(SECOND_BUTTON, BUTTON_PULLUP);

enum {
  // States
  NORMAL = 1,
  SETTING,
  WARNING,
  ALARM,
  NO_STATE,
} STATES;

enum {
  // Events
  BUTTON_PRESSED = 0,
  BUTTON2_PRESSED,
  LONG_PRESS,
  NORMAL_TEMP,
  WARN_TEMP,
  OVER_TEMP,
  ENTRY,
  NONE,
} EVENTS;

// Set char values
unsigned char state = NORMAL;
unsigned char prevState = NO_STATE;
unsigned char event = NONE;

//Conversion Functions
double ReadThermistor(int adc) {
  double resistance = ((1024.0 / adc) - 1); //calculate resistance from 10k resistor from voltage divider
  double Temp = log(resistance);

  // calculate the temperature in Kelvin using the Steinhart-Hart equation
  Temp = 1 / (0.003354016 + 0.0002569850 * Temp + 0.000002620131 * Temp * Temp + 0.00000006383091 * Temp * Temp * Temp);
  Temp = Temp - 273.15; // Kelvin to Celsius conversion
  return Temp;
}

// Program startup
void setup() {
  // Display Information to user
  Serial.begin (9600);
  Serial.println("Starting Over Temperature Alarm");

  // Set pin modes
  pinMode(BUTTON_PIN, INPUT);
  pinMode(TEMP_PIN, INPUT);
  pinMode(SECOND_BUTTON, INPUT);

  lcd.begin(16, 2); // Initialize LCD screen
}

// Main loop
void loop() {
  //Timing variables for buttons
  int buttonDelay = 0;
  int holdTime = 1000;
  int setTime = 5000;
  float noPressLength = 0;
  float pressLength = 0;


  // Event Polling
  // poll temperature sensor
  adc = analogRead(TEMP_PIN);
  if (adc > 1 && adc < 1022) {
    tempC = ReadThermistor(adc);
    tempF = (tempC * 9.0) / 5.0 + 32.0; // Convert Celsius to Farenheit
  }
  else {
    Serial.println("Thermistor is open or shorted!");
  }

  // Normal temperature event check
  if (tempF < (WARN_TEMP_LIMIT - 0.5)) {
    event = NORMAL_TEMP;
  }

  // Warning temperature event check
  if (tempF > WARN_TEMP_LIMIT) {
    event = WARN_TEMP;
  }

  // Over temperature event check
  if (tempF > OVER_TEMP_LIMIT) {
    event = OVER_TEMP;
  }

  // Poll buttons and check for button event
  if (button.isPressed()) {
    Serial.println("button pressed!");
    event = BUTTON_PRESSED;
  }

  while (secondButton.isPressed() && !buttonFlag) {
    buttonDelay++;
    delay(100);
    Serial.println("second button pressed!");
    Serial.print("Count = ");
    Serial.println(buttonDelay);
  }
  if (buttonDelay >= 1) {
    buttonFlag = true;
  }
  if (buttonDelay < 10 && buttonFlag) {
    buttonDelay = 0;
    buttonFlag = false;
    event = BUTTON2_PRESSED;
  }
  if  (buttonDelay >= 10 && buttonFlag) {
    buttonDelay = 0;
    buttonFlag = false;
    event = LONG_PRESS;
  }

  // state machine
  switch (state) {

    case NORMAL: // Normal state
      // Entry code
      if (state != prevState) {
        Serial.println("In NORMAL state, displaying 'Normal'");
        lcd.clear();
        lcd.print("Normal");
        event = NONE;
        prevState = state;
      }

      // Event Capture
      if (event == BUTTON_PRESSED) { // Button Press transition
        while (digitalRead(BUTTON_PIN) == LOW) {
          delay(100);
          pressLength = pressLength + 100;
          Serial.print("ms = ");
          Serial.println(pressLength);
        }

        if (pressLength > 1) {
          if (displayCelsius) {
            displayCelsius = false;
          }
          else {
            displayCelsius = true;
          }
        }
        event = NONE;
      }

      if (event == BUTTON2_PRESSED) { // Button press transition
        lcd.clear();
        lcd.print("TEMP LIMIT:");
        lcd.setCursor(0, 1);

        if (!displayCelsius) {
          Serial.println("The Over Temp Limit is: ");
          Serial.print(OVER_TEMP_LIMIT, 1);
          Serial.println("F");
          lcd.print(OVER_TEMP_LIMIT);
          lcd.print("F");
        }
        else {
          overTempC = int (((OVER_TEMP_LIMIT - 32.0) * (5.0 / 9.0)) + 0.5);
          Serial.println("The Over Temp Limit is: ");
          Serial.print(overTempC, 1);
          Serial.println("C");
          lcd.print(overTempC);
          lcd.print("C");
        }
        delay(3000);
        lcd.clear();
        lcd.print("Normal");
        event = NONE;
      }

      if (event == LONG_PRESS) { // Button long press transition
        Serial.println("Going to SETTING state");
        state = SETTING;
        event = NONE;
      }

      if (event == WARN_TEMP) { // Warning temperature transition
        Serial.println("WARN_TEMP event!");
        state = WARNING;
        event = NONE;
      }

      if (event == OVER_TEMP) { // Over temperature transition
        Serial.println("OVER_TEMP event!");
        state = ALARM;
        event = NONE;
      }
      break;

    case SETTING: // Setting state
      // Entry code
      if (state != prevState)
      {
        Serial.println("in Setting state");
        event = NONE;
        prevState = state;
        overTempC = int (((OVER_TEMP_LIMIT - 32.0) * (5.0 / 9.0)) + 0.5);
        lcd.clear();
        lcd.print("SETTING");
        delay(3000);
        lcd.clear();
        lcd.print("TEMP LIMIT:");
        lcd.setCursor(0, 1);
        if (!displayCelsius) {
          lcd.print(OVER_TEMP_LIMIT);
          lcd.print("F");
        }
        else {
          lcd.print(overTempC);
          lcd.print("C");
        }
      }

      // Event capture
      if (event == BUTTON2_PRESSED) { // Button press transition
        lcd.clear();
        lcd.print("TEMP LIMIT:");
        lcd.setCursor(0, 1);
        if (!displayCelsius) {
          OVER_TEMP_LIMIT = OVER_TEMP_LIMIT - 1;
          Serial.print("The Over Temp limit is: ");
          Serial.print(OVER_TEMP_LIMIT, 1);
          Serial.println("F");
          lcd.print(OVER_TEMP_LIMIT);
          lcd.print("F");
        }
        else {
          overTempC = overTempC - 1;
          Serial.print("The Over Temp Limit is: ");
          Serial.print(overTempC, 1);
          Serial.println("C");
          lcd.print(overTempC);
          lcd.print("C");
        }

        event = NONE;
      }

      if (event == BUTTON_PRESSED) { // Button press transition
        while (digitalRead(BUTTON_PIN) == LOW) {
          delay(100);
          pressLength = pressLength + 100;
          Serial.print("ms = ");
          Serial.println(pressLength);
        }

        if (pressLength > holdTime) {
          if (displayCelsius) {
            OVER_TEMP_LIMIT = int (((overTempC * 9.0) / 5.0 + 32.0) + 0.5);
            Serial.print("The Over Temp limit is: ");
            Serial.print(OVER_TEMP_LIMIT, 1);
            Serial.println("F");
            lcd.clear();
            lcd.print("TEMP LIMIT:");
            lcd.setCursor(0, 1);
            lcd.print(OVER_TEMP_LIMIT);
            lcd.print("F");
            displayCelsius = false;
          }
          else {
            overTempC = int (((OVER_TEMP_LIMIT - 32.0) * (5.0 / 9.0)) + 0.5);
            Serial.print("The Over Temp Limit is: ");
            Serial.print(overTempC, 1);
            Serial.println("C");
            lcd.clear();
            lcd.print("TEMP LIMIT:");
            lcd.setCursor(0, 1);
            lcd.print(overTempC);
            lcd.print("C");
            displayCelsius = true;
          }
        }

        if (!displayCelsius && pressLength <= holdTime) {
          OVER_TEMP_LIMIT = OVER_TEMP_LIMIT + 1;
          Serial.print("The Over Temp limit is: ");
          Serial.print(OVER_TEMP_LIMIT, 1);
          Serial.println("F");
          lcd.clear();
          lcd.print("TEMP LIMIT:");
          lcd.setCursor(0, 1);
          lcd.print(OVER_TEMP_LIMIT);
          lcd.print("F");
        }

        if (displayCelsius && pressLength <= holdTime) {
          overTempC = overTempC + 1;
          Serial.print("The Over Temp Limit is: ");
          Serial.print(overTempC, 1);
          Serial.println("C");
          lcd.clear();
          lcd.print("TEMP LIMIT:");
          lcd.setCursor(0, 1);
          lcd.print(overTempC);
          lcd.print("C");
        }

        event = NONE;
      }

      if (event == LONG_PRESS) { // Button long press transition
        Serial.println("Returning to previous state");
        state = NORMAL;
        event = NONE;
        break;
      }

      while (digitalRead(SECOND_BUTTON) == HIGH && digitalRead(BUTTON_PIN) == HIGH) { // Idle transition
        delay(100);
        noPressLength = noPressLength + 100;
        Serial.print("Idle ms = ");
        Serial.println(noPressLength);

        if (noPressLength > setTime) {
          state = NORMAL;
          event = NONE;
          break;
        }
        event = NONE;
      }
      break;

    case WARNING: // Warning state
      // Entry code
      if (state != prevState) {
        Serial.println("in WARNING state, displaying Warning");
        lcd.clear();
        lcd.print("WARNING");
        event = NONE;
        prevState = state;
      }

      // Event capture
      if (event == BUTTON_PRESSED) { // Button press transition
        while (digitalRead(BUTTON_PIN) == LOW) {
          delay(100);
          pressLength = pressLength + 100;
          Serial.print("ms = ");
          Serial.println(pressLength);
        }

        if (pressLength > 1) {
          if (displayCelsius) {
            displayCelsius = false;
          }
          else {
            displayCelsius = true;
          }
        }
        event = NONE;
      }

      if (event == BUTTON2_PRESSED) { // Button press transition
        lcd.clear();
        lcd.print("TEMP LIMIT:");
        lcd.setCursor(0, 1);

        if (!displayCelsius) {
          Serial.println("The Over Temp Limit is: ");
          Serial.print(OVER_TEMP_LIMIT, 1);
          Serial.println("F");
          lcd.print(OVER_TEMP_LIMIT);
          lcd.print("F");
        }
        else {
          overTempC = int (((OVER_TEMP_LIMIT - 32.0) * (5.0 / 9.0)) + 0.5);
          Serial.println("The Over Temp Limit is: ");
          Serial.print(overTempC, 1);
          Serial.println("C");
          lcd.print(overTempC);
          lcd.print("C");
        }
        delay(3000);
        lcd.clear();
        lcd.print("WARNING");
        event = NONE;
      }

      if (event == LONG_PRESS) { // Button long press transition
        Serial.println("Going to SETTING state");
        state = SETTING;
        event = NONE;
      }

      if (event == NORMAL_TEMP) { // Normal temperature transition
        Serial.println("Go to normal");
        state = NORMAL;
        event = NONE;
      }

      if (event == OVER_TEMP) { // Over temperature transition
        Serial.println("Go to alarm");
        state = ALARM;
        event = NONE;
      }
      break;

    case ALARM: // Alarm state
      // Entry code
      if (state != prevState) {
        Serial.println("in ALARM state, display 'Alarm'");
        lcd.clear();
        lcd.print("ALARM");
        event = NONE;
        prevState = state;
      }

      // Event Capture
      if (event == BUTTON_PRESSED) { // button press transition
        while (digitalRead(BUTTON_PIN) == LOW) {
          delay(100);
          pressLength = pressLength + 100;
          Serial.print("ms = ");
          Serial.println(pressLength);
        }

        if (pressLength >= holdTime && !alarmFlag) {
          Serial.println("Ending alarm");
          lcd.clear();
          lcd.print("Ending alarm");
          delay (1000);
          if (tempF > OVER_TEMP_LIMIT) {
            alarmFlag = true;
            Serial.println("Temperature is still too high!");
            lcd.clear();
            lcd.print("TEMP still high!");
          }
          else {
            alarmFlag = false;
            state = NORMAL;
          }
        }

        if (pressLength < 500) {
          if (displayCelsius) {
            displayCelsius = false;
          }

          else {
            displayCelsius = true;
          }
        }
        event = NONE;
      }

      if (tempF < (OVER_TEMP_LIMIT - 0.5) && alarmFlag) {
        Serial.println("Temperature is below limit");
        Serial.println("Returning to NORMAL state");
        alarmFlag = false;
        state = NORMAL;
        event = NONE;
      }
      break;
  } // End state machine

  // Base timer
  if ((millis() - baseTimedOut > BASE_TIMEOUT) && state != SETTING) {
    baseTimedOut = millis();

    // Update temperature display
    lcd.setCursor(0, 1);

    if (displayCelsius) {
      Serial.print("The Temp is: ");
      Serial.print(tempC, 1);
      Serial.println("C");
      lcd.print("Temp is:");
      lcd.print(tempC);
      lcd.print("C");
    }
    else {
      Serial.print("The Temp is: ");
      Serial.print(tempF, 1);
      Serial.println("F");
      lcd.print("Temp is:");
      lcd.print(tempF);
      lcd.print("F");
    }
  }

}
