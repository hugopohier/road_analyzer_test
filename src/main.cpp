#include <Arduino.h>
#include <Adafruit_GPS.h>
#include <BMA400.h>

#define GNSS
#define ACCELEROMETER
#undef POWER_CHECKING

/****************** Pinout ******************/
#define COM_PB_PIN 34
#define SENS_SW_PIN 35

#ifdef POWER_CHECKING
    #define PWR_CHECK_PIN 33
#endif

#define RED_LED_PIN 25
#define GREEN_LED_PIN 26
#define BLUE_LED_PIN 27

#ifdef ACCELEROMETER
    #define ACC_SDA 21
    #define ACC_SCL 22
    #define ACC_INT1 4
    #define ACC_INT2 2
#endif

#ifdef GNSS
    #define GPSSerial Serial2
    #define GNSS_EN  14
    #define GNSS_FIX 12
#endif

/****************** Global defines ******************/
//Analog value of the critical battery level
#define BATTERY_CRITICAL_LEVEL 2000

/****************** Global variables ******************/
#ifdef GNSS
    //Connect to the GPS on the hardware port
    Adafruit_GPS GPS(&GPSSerial);
#endif

#ifdef ACCELEROMETER
    //Connect the BMA400 through the I2C pins
    BMA400 ACC(ACC_SDA, ACC_SCL);

    float ACC_X = 0, ACC_Y = 0, ACC_Z = 0;
    uint8_t ACC_SENS = 2;
#endif

uint8_t PB_pushed = 0;
uint8_t SW_changed = 0;

uint8_t RGB_color = 0;

uint32_t timer = millis();

/****************** Push-Button Interrupt ******************/
void IRAM_ATTR COM_PB_handler() {
    PB_pushed++;
    detachInterrupt(COM_PB_PIN);
}

/****************** Switch Interrupt ******************/
void IRAM_ATTR SENS_SW_handler() {
    SW_changed++;
    detachInterrupt(SENS_SW_PIN);
}

/****************** Setup ******************/
void setup() {
    Serial.begin(9600);
    Serial.println("----- Setup -----");

    pinMode(COM_PB_PIN, INPUT);
    pinMode(SENS_SW_PIN, INPUT);
    
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);

    digitalWrite(RED_LED_PIN, 0);
    digitalWrite(GREEN_LED_PIN, 0);
    digitalWrite(BLUE_LED_PIN, 0);

    attachInterrupt(digitalPinToInterrupt(COM_PB_PIN), COM_PB_handler, FALLING);
    attachInterrupt(digitalPinToInterrupt(SENS_SW_PIN), SENS_SW_handler, CHANGE);

    #ifdef GNSS
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
    #endif

    #ifdef ACCELEROMETER

        ACC_SENS = (digitalRead(SENS_SW_PIN)*2) + 2;
        
        //Initialization of the accelerometer BMA400
        while(!ACC.isConnection())
        {
            Serial.println("BMA400 is not connected");
            delay(2000);
        }

        ACC.initialize(NORMAL, RANGE_2G, ODR_12);
        Serial.println("BMA400 is connected");
    #endif

    Serial.println("----- Loop -----");
}

/****************** Loop ******************/
void loop() {
    
    #ifdef GNSS
        // Read data from the GPS TX pin (char by char)
        GPS.read();
        
        //If a sentence is received then print it
        if (GPS.newNMEAreceived()) {
            Serial.println(GPS.lastNMEA());
        }
    #endif

    if(PB_pushed > 0)
    {
        Serial.println(PB_pushed);
        PB_pushed = 0;
        RGB_color++;

        if(1 == RGB_color)
        {
            digitalWrite(RED_LED_PIN, 1);
            digitalWrite(GREEN_LED_PIN, 1);
            digitalWrite(BLUE_LED_PIN, 1);
        }
        else if(2 == RGB_color)
        {
            digitalWrite(RED_LED_PIN, 0);
            digitalWrite(GREEN_LED_PIN, 1);
            digitalWrite(BLUE_LED_PIN, 0);
        }
        else if(3 == RGB_color)
        {
            digitalWrite(RED_LED_PIN, 0);
            digitalWrite(GREEN_LED_PIN, 0);
            digitalWrite(BLUE_LED_PIN, 1);
        }
        else
        {
            RGB_color = 0;
            digitalWrite(RED_LED_PIN, 0);
            digitalWrite(GREEN_LED_PIN, 0);
            digitalWrite(BLUE_LED_PIN, 0);
        }

        attachInterrupt(digitalPinToInterrupt(COM_PB_PIN), COM_PB_handler, FALLING);
    }

    #ifdef ACCELEROMETER
        if(SW_changed > 0)
        {
            SW_changed = 0;


            if(2 == ACC_SENS)
            {
                ACC_SENS = 4;

                ACC.setFullScaleRange(RANGE_4G);

                digitalWrite(RED_LED_PIN, 1);
                digitalWrite(GREEN_LED_PIN, 0);
                digitalWrite(BLUE_LED_PIN, 1);
            }
            else
            {
                ACC_SENS = 2;

                #ifdef ACCELEROMETER
                    ACC.setFullScaleRange(RANGE_2G);
                #endif

                digitalWrite(RED_LED_PIN, 1);
                digitalWrite(GREEN_LED_PIN, 1);
                digitalWrite(BLUE_LED_PIN, 0);
            }

            attachInterrupt(digitalPinToInterrupt(SENS_SW_PIN), SENS_SW_handler, CHANGE);
        }
    #endif

    if (millis() - timer > 1000) {
        // Reset the timer
        timer = millis();

        #ifdef ACCELEROMETER
            ACC.getAcceleration(&ACC_X, &ACC_Y, &ACC_Z);

            Serial.print(ACC_X);
            Serial.print(",");
            Serial.print(ACC_Y);
            Serial.print(",");
            Serial.println(ACC_Z);
            Serial.println();
        #endif

        #ifdef POWER_CHECKING
            //If battery reach the critical level (15%) ==> Change the LED color
            if(BATTERY_CRITICAL_LEVEL > analogRead(PWR_CHECK_PIN))
            {
                digitalWrite(RED_LED_PIN, 1);
                digitalWrite(GREEN_LED_PIN, 1);
                digitalWrite(BLUE_LED_PIN, 1);
            }
            else
            {
                digitalWrite(RED_LED_PIN, 0);
                digitalWrite(GREEN_LED_PIN, 1);
                digitalWrite(BLUE_LED_PIN, 1);
            }
        #endif
    }
}