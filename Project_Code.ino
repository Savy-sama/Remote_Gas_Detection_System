#include <MQUnifiedsensor.h>
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "AGS02MA.h"              // These are include directives to include necessary libraries for working with various sensors and devices.

#define placa "Arduino UNO"
#define Voltage_Resolution 5
#define pinMQ135 A0               // configuring Arduino analog Pin A0 for MQ-135
#define RL_MQ135 22               // changed the value of RL from 1(default) to 22(changed)
#define pinMQ7 A1                 // configuring Arduino analog Pin A1 for MQ-7
#define RL_MQ7 1                  // these are preprocessor macro definitions used to assign values 
                                  //to specific names (placa, Voltage_Resolution, pinMQ135, etc.) for easier code readability and maintenance.

#define MQ135_DEFAULTPPM 421.72    // preprocessor macro definitions with specific values for MQ135 sensor calibration and calculation.
#define MQ135_DEFAULTRO 68550
#define MQ135_SCALINGFACTOR 116.6020682
#define MQ135_EXPONENT -2.769034857

#define DHTPIN 2                    // configuring Arduino Digital pin 2 for DHT11 sensor
#define DHTTYPE DHT11               // defining the DHT sensor type

#define PWMPin 5

SoftwareSerial mySerial(9, 10);

AGS02MA AGS(26);
DHT_Unified dht(DHTPIN, DHTTYPE);   // Object instantiation for AGS02MA and DHT_Unified using specific pins and types.

unsigned long delayMS;              // Declaration of an unsigned long variable to hold the delay time in milliseconds.

MQUnifiedsensor MQ135(placa, Voltage_Resolution, 10, pinMQ135, "MQ-135");       // Object instantiation for MQ135 sensors with specific parameters.
MQUnifiedsensor MQ7(placa, Voltage_Resolution, 10, pinMQ7, "MQ-7");             // Object instantiation for MQ7 sensors with specific parameters.

float getCorrectionFactor(float t, float h);          // Function prototype for the getCorrectionFactor function, 
                                                      // which calculates a correction factor based on temperature and humidity

void setup() {                                        // Initialization code executed once at the beginning
  Serial.begin(9600);                                 // Starts serial communication with a baud rate of 9600.
  dht.begin();                                        // Initializes the DHT sensor.
  sensor_t sensor;

  MQ135.setRegressionMethod(1);                        // Setting up MQ135 sensor calibration parameters
  MQ135.setA(110.47);
  MQ135.setB(-2.862);
  MQ135.init();
  MQ135.setRL(RL_MQ135);

  Serial.print("Calibrating MQ-135, please wait...");
  float calc135R0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ135.update();
    calc135R0 += MQ135.calibrate(3.6);
  }
  MQ135.setR0(calc135R0 / 10);
  Serial.println(" done!");                             // print "done" once the calibration is successful.

  MQ7.setRegressionMethod(1);                           // Setting up MQ7 sensor calibration parameters
  MQ7.setA(99.042);
  MQ7.setB(-1.518);
  MQ7.init();
  MQ7.setRL(RL_MQ7);

  Serial.print("Calibrating MQ-7, please wait...");
  float calc7R0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ7.update();
    calc7R0 += MQ7.calibrate(27.5);
  }
  MQ7.setR0(calc7R0 / 10);
  Serial.println(" done!");                              // print "done" once the calibration is successful.

  Wire.begin();                                          // Initializes the I2C communication (needed for AGS02MA).
  AGS.begin();
  AGS.setPPBMode();                                      // Initializes AGS02MA sensor and sets it to PPB (parts per billion) mode.

  Serial.println("Initialization complete.");
  delayMS = 500;                                         // Assigns a delay time of 500 milliseconds to the delayMS variable.
}

void loop() {                                            // Main code that runs in a loop after setup() is executed. 
  delay(delayMS);                                        // Pauses the program for 'delayMS' milliseconds.

  sensors_event_t event;
  dht.temperature().getEvent(&event);                    // Reading temperature and humidity from the DHT sensor.
  float cFactor = 0;
  if (!isnan(event.temperature) && !isnan(event.relative_humidity))
    cFactor = getCorrectionFactor(event.temperature, event.relative_humidity);

  MQ135.update();
  float CO2 = MQ135.readSensor(false, cFactor);           // Reading CO2 concentration from the MQ135 sensor.

  MQ7.update();
  float CO = MQ7.readSensor();                            // Reading CO concentration from the MQ7 sensor.

  float SO2 = (AGS.readPPB())*0.00355;
  float H2S = (AGS.readPPB())*0.00666;
  float NO2 = (AGS.readPPB())*0.00494;
  float NO = (AGS.readPPB())*0.00757;
  float O3 = (AGS.readPPB())*0.00473;
  float tCO2 = CO2 + MQ135_DEFAULTPPM;

  Serial.print("CO2(ppm):  ");                            // Printing the sensor readings to the serial monitor
  Serial.print(tCO2);
  Serial.print("       ");

  Serial.print("CO(ppm):  ");
  Serial.print(CO);
  Serial.print("       ");

  Serial.print("SO2(ppm):  ");
  Serial.print(SO2);
  Serial.print("       ");

  Serial.print("H2S(ppb):  ");
  Serial.print(H2S);
  Serial.print("       ");

  Serial.print("NO2(ppb):  ");
  Serial.print(NO2);
  Serial.print("       ");

  Serial.print("NO(ppb):  ");
  Serial.print(NO);
  Serial.print("       ");

  Serial.print("O3(ppb):  ");
  Serial.print(O3);
  Serial.print("       ");

  Serial.print("Temp.(C): ");
  Serial.print(event.temperature);
  Serial.print("       ");

  Serial.print("Humi.(%): ");
  Serial.print(event.relative_humidity);
  Serial.println();
  
  String Data;
  Data = "CO2(ppm):  " + String(tCO2) + "       " +
         "CO(ppm):  " + String(CO) + "       " +
         "SO2(ppm):  " + String(SO2) + "       " +
         "H2S(ppb):  " + String(H2S) + "       " +
         "NO2(ppb):  " + String(NO2) + "       " +
         "NO(ppb):  " + String(NO) + "       " +
         "O3(ppb):  " + String(O3) + "       " +
         "Temp.(C):  " + String(event.temperature) + "       " +
         "Humi.(%): " + String(event.relative_humidity);

  if (Serial.available()>0)
   switch(Serial.read())
  {
    case 's':
      mySerial.println("AT+CMGF=1");                        //Sets the GSM Module in Text Mode
     delay(1000);                                           // Delay of 1 second
     mySerial.println("AT+CMGS=\"+91xxxxxxxxxx\"\r");       // mobile number to which the data will be send
     delay(1000);
     mySerial.println(Data);                                // The Data to send
     delay(100);
     mySerial.println((char)26);                            // ASCII code of CTRL+Z for saying the end of sms to  the module 
      delay(1000);
      break;
      
    case 'r':
      mySerial.println("AT+CNMI=2,2,0,0,0");                // AT Command to receive a live SMS
      delay(1000);
      break;
  }
  if (mySerial.available()>0)
   Serial.write(mySerial.read());
}

float getCorrectionFactor(float t, float h) {                     // Function to calculate a correction factor based on temperature and humidity.
  const float CORA = 0.00035;
  const float CORB = 0.02718;
  const float CORC = 1.39538;
  const float CORD = 0.0018;
  return CORA * t * t - CORB * t + CORC - (h - 33.0) * CORD;      // Returns the calculated correction factor.
}
