// Libraries
#include <Wire.h>                 // I2C communication
#include <LiquidCrystal_I2C.h>    // Interfacing with I2C-based LCD displays
#include <DallasTemperature.h>    // Interface with Dallas temperature sensors
#include <OneWire.h>              // OneWire communication for temperature sensor
#include <GravityTDS.h>           // Interface with GravityTDS

// Constants
#define TdsSensorPin A1           // The analog pin connected to the salinity sensor
#define pH_pin A0                 // The analog pin connected to the pH sensor
#define ONE_WIRE_BUS 2            // The digital pin connected to the OneWire bus for temperature sensors
#define X 1.2                     // Calibrating pH sensor
#define samplingInterval 20       // Collect the value every 20ms
#define printInterval 1000        // Print every 1000ms
#define ArrayLenth 50             // Put the collect value in the array

// Global Variables
GravityTDS gravityTds;                         // Create an instance of the GravityTDS class
LiquidCrystal_I2C lcd(0x27, 16, 2);            // Initialize the LCD with I2C address 0x27
OneWire oneWire(ONE_WIRE_BUS);                 // Setup a OneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire);           // Pass the OneWire reference to DallasTemperature
int pHArray[ArrayLenth];                       // Array to store pH values
int pHArrayIndex = 0;                          // pH array index

void setup() {
  Serial.begin(9600);                          // Start serial communication at 9600 bps
  lcd.init();                                  // Initialize the LCD
  lcd.backlight();                             // Turn on the backlight
  lcd.begin(16, 2);                            // Initialize the LCD with 16 columns and 2 rows
  sensors.begin();                             // Initialize the Dallas temperature sensor
  gravityTds.setPin(TdsSensorPin);             // Set the pin for TDS sensor
  gravityTds.setAref(5.0);                     // Define the reference voltage to 5V
  gravityTds.setAdcRange(1024);                // Set the ADC range
  gravityTds.begin();                          // Initialize the TDS sensor
}

void loop() {
  float pHVal, voltVal;                        // Declare variables to store pH and voltage values
  mean_average_pH(pHVal, voltVal);             // Calculate mean average pH value
  float temperature = readTemperature();       // Read temperature value
  gravityTds.setTemperature(temperature);      // Set temperature compensation for TDS
  gravityTds.update();                         // Update TDS value
  float tdsValue = gravityTds.getTdsValue();   // Get TDS value in ppm
  float salinity = tdsValue * 0.00064;         // Convert TDS value to salinity in w/w% (approximation)
  displayData(pHVal, temperature, salinity);   // Display data on LCD
  serialData(pHVal, voltVal, temperature, salinity); // Send data to serial monitor
}

void mean_average_pH(float &pHVal, float &voltVal) {
  static unsigned long samplingTime = millis();  // Variable to keep track of sampling time
  static unsigned long printTime = millis();     // Variable to keep track of print time
  float pHValue, voltage;                        // Declare variables to store pH value and voltage

  do {
    if (millis() - samplingTime > samplingInterval) {  // Check if sampling interval has passed
      pHArray[pHArrayIndex++] = analogRead(pH_pin);    // Read pH value and store in array
      if (pHArrayIndex == ArrayLenth) pHArrayIndex = 0; // Reset array index if it reaches array length
      voltage = avergearray(pHArray, ArrayLenth) * 5.0 / 1024;  // Calculate voltage
      pHValue = 3.5 * voltage + X;                     // Calculate pH value
      samplingTime = millis();                         // Update sampling time
    }
    if (millis() - printTime > printInterval) {        // Check if print interval has passed
      pHVal = pHValue;                                 // Update pH value
      voltVal = voltage;                               // Update voltage value
      break;                                           // Exit loop
    }
  } while (true);
}

double avergearray(int* arr, int number) {
  int i;
  int max, min;
  double avg;
  long amount = 0;

  if (number <= 0) {                                  // Check for invalid array length
    Serial.println("Error number for the array to averaging!");
    return 0;
  }
  if (number < 5) {                                   // If array length is less than 5, calculate average directly
    for (i = 0; i < number; i++) amount += arr[i];
    avg = amount / number;
    return avg;
  } else {                                            // If array length is 5 or more, calculate average excluding min and max
    if (arr[0] < arr[1]) {
      min = arr[0];
      max = arr[1];
    } else {
      min = arr[1];
      max = arr[0];
    }
    for (i = 2; i < number; i++) {
      if (arr[i] < min) {
        amount += min;
        min = arr[i];
      } else if (arr[i] > max) {
        amount += max;
        max = arr[i];
      } else {
        amount += arr[i];
      }
    }
    avg = (double)amount / (number - 2);               // Calculate average excluding min and max
  }
  return avg;
}

void displayData(float pHValue, float temperature, float salinity) {
  lcd.setCursor(0, 0);                               // Set cursor to first row, first column
  lcd.print("pH   : ");                              // Print label for pH value
  lcd.print(pHValue, 2);                             // Print pH value with 2 decimal places

  lcd.setCursor(0, 1);                               // Set cursor to second row, first column
  lcd.print("Temp : ");                              // Print label for temperature
  lcd.print(temperature, 2);                         // Print temperature with 2 decimal places
  lcd.print(" C");                                   // Print temperature unit

  lcd.setCursor(8, 1);                               // Set cursor to second row, ninth column
  lcd.print("Sal: ");                                // Print label for salinity
  lcd.print(salinity, 2);                            // Print salinity with 2 decimal places
  lcd.print(" %");                                   // Print salinity unit
}

void serialData(float pHValue, float voltage, float temperature, float salinity) {
  Serial.println("*****************");               // Print separator line
  Serial.print("pH      : ");                        // Print label for pH
  Serial.println(pHValue, 2);                        // Print pH value with 2 decimal places
  Serial.print("Voltage : ");                        // Print label for voltage
  Serial.println(voltage, 2);                        // Print voltage with 2 decimal places
  Serial.print("Temp    : ");                        // Print label for temperature
  Serial.print(temperature, 2);                      // Print temperature with 2 decimal places
  Serial.println(" C");                              // Print temperature unit
  Serial.print("Salinity: ");                        // Print label for salinity
  Serial.print(salinity, 2);                         // Print salinity with 2 decimal places
  Serial.println(" %");                              // Print salinity unit
  Serial.println("*****************\n");             // Print separator line
}

float readTemperature() {
  sensors.requestTemperatures();                     // Request temperature measurement
  return sensors.getTempCByIndex(0);                 // Get temperature in Celsius from the first sensor
}
