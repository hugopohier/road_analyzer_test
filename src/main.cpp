#include <Arduino.h>
#include <Adafruit_GPS.h>
#include <BMA400.h>

/****************** Pinout ******************/
#define COM_PB_PIN 34
#define SENS_SW_PIN 35

#define PWR_CHECK_PIN 33

#define RED_LED_PIN 25
#define GREEN_LED_PIN 26
#define BLUE_LED_PIN 27

#define ACC_SDA 21
#define ACC_SCL 22
#define ACC_INT1 4
#define ACC_INT2 2

#define GNSS_EN  17
#define GNSS_FIX 16

/****************** Global defines ******************/
//Analog value of the critical battery level
#define BATTERY_CRITICAL_LEVEL 2300

//Serial port name
#define GPSSerial Serial2

/****************** Global variables ******************/
//Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

//Connect the BMA400 through the I2C pins
BMA400 ACC(ACC_SDA, ACC_SCL);

uint8_t PB_pushed = 0;
uint8_t SW_changed = 0;

uint8_t RGB_color = 0;
uint8_t ACC_SENS = 2;

float ACC_X = 0, ACC_Y = 0, ACC_Z = 0;

uint32_t timer = millis();

/****************** Push-Button Interrupt ******************/
void IRAM_ATTR COM_PB_handler() {
    PB_pushed++;
}

/****************** Switch Interrupt ******************/
void IRAM_ATTR SENS_SW_handler() {
    SW_changed++;
}

/****************** Setup ******************/
void setup() {
    Serial.begin(115200);
    Serial.println("----- Setup -----");

    pinMode(COM_PB_PIN, INPUT_PULLUP);
    pinMode(SENS_SW_PIN, INPUT_PULLUP);
    
    pinMode(RED_LED_PIN, OUTPUT_OPEN_DRAIN);
    pinMode(GREEN_LED_PIN, OUTPUT_OPEN_DRAIN);
    pinMode(BLUE_LED_PIN, OUTPUT_OPEN_DRAIN);

    attachInterrupt(digitalPinToInterrupt(COM_PB_PIN), COM_PB_handler, FALLING);
    attachInterrupt(digitalPinToInterrupt(SENS_SW_PIN), SENS_SW_handler, CHANGE);

    //Init of the GPS component
    GPS.begin(9600);

    //Configuration on RMC and GGA data
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

    //1 Hz update rate
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); 

    //Request updates on antenna status
    GPS.sendCommand(PGCMD_ANTENNA);

    delay(1000);

    //Ask for firmware version
    GPSSerial.println(PMTK_Q_RELEASE);

    Serial.println("----- Loop -----");
}

/****************** Loop ******************/
void loop() {
    if(PB_pushed > 0)
    {
      PB_pushed = 0;
      RGB_color++;

      if(1 == RGB_color)
      {
          digitalWrite(RED_LED_PIN, 255);
          digitalWrite(GREEN_LED_PIN, 0);
          digitalWrite(BLUE_LED_PIN, 0);
      }
      else if(2 == RGB_color)
      {
          digitalWrite(RED_LED_PIN, 0);
          digitalWrite(GREEN_LED_PIN, 255);
          digitalWrite(BLUE_LED_PIN, 0);
      }
      else if(3 == RGB_color)
      {
          digitalWrite(RED_LED_PIN, 0);
          digitalWrite(GREEN_LED_PIN, 0);
          digitalWrite(BLUE_LED_PIN, 255);
      }
      else
      {
          RGB_color = 0;
          digitalWrite(RED_LED_PIN, 0);
          digitalWrite(GREEN_LED_PIN, 0);
          digitalWrite(BLUE_LED_PIN, 0);
      }
    }

    if(SW_changed > 0)
    {
        SW_changed = 0;

        if(2 == ACC_SENS)
        {
            ACC_SENS = 4;
        }
        else
        {
            ACC_SENS = 2;
        }
    }

    if (millis() - timer > 2000) {
      // Reset the timer
      timer = millis();

      // Read data from the GPS
      GPS.read();

      //If a sentence is received then print it
      if (GPS.newNMEAreceived()) {
        Serial.println(GPS.lastNMEA());
      }

      ACC.getAcceleration(&ACC_X, &ACC_Y, &ACC_Z);

      Serial.println(ACC_X);
      Serial.print(",");
      Serial.print(ACC_Y);
      Serial.print(",");
      Serial.print(ACC_Z);

      //If battery reach the critical level (25%) ==> Change the LED color
      if(BATTERY_CRITICAL_LEVEL > analogRead(PWR_CHECK_PIN))
      {
          digitalWrite(RED_LED_PIN, 255);
          digitalWrite(GREEN_LED_PIN, 255);
          digitalWrite(BLUE_LED_PIN, 255);
      }
    }
}