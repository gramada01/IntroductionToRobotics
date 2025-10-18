//This program takes 3 analog input values from potentiometers,
//maps them to specific output values and sends those values to an RGB LED
//in order to control the brightness of each led in particular(red, green and blue)

//Creating constant variables for the pins
const int redInputPin = A0;
const int greenInputPin = A1;
const int blueInputPin = A2;
const int redLedPin = 9;
const int greenLedPin = 10;
const int blueLedPin = 11;

//Creating constant variables for maximum and minimum values for analogRead() and analogWrite()
const int analogMaxReadValue = 1023;
const int analogMinReadValue = 0;
const int analogMaxWriteValue = 255;
const int analogMinWriteValue = 0;

void setup() {
  //Setting up the serial monitor with a default baud rate of 9600 bps (bits per second)
  Serial.begin(9600);

  //Setting the pinmodes for the RGB LED on OUTPUT and for the potentiometers on INPUT
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(redInputPin, INPUT);
  pinMode(greenInputPin, INPUT);
  pinMode(blueInputPin, INPUT);
}

//Variables used for reading an writing on the pins
int redVal = 0;
int greenVal = 0;
int blueVal = 0;

void loop() {

  //Reading the value from the potentiometer, mapping it to a coresponding output value
  //and sending it to the red LED
  redVal = analogRead(redInputPin);
  redVal = map(redVal, analogMinReadValue, analogMaxReadValue, analogMinWriteValue, analogMaxWriteValue);
  analogWrite(redLedPin, redVal);

  //Reading the value from the potentiometer, mapping it to a coresponding output value
  //and sending it to the green LED
  greenVal = analogRead(greenInputPin);
  greenVal = map(greenVal, analogMinReadValue, analogMaxReadValue, analogMinWriteValue, analogMaxWriteValue);
  analogWrite(greenLedPin, greenVal);

  //Reading the value from the potentiometer, mapping it to a coresponding output value
  //and sending it to the blue LED
  blueVal = analogRead(blueInputPin);
  blueVal = map(blueVal, analogMinReadValue, analogMaxReadValue, analogMinWriteValue, analogMaxWriteValue);
  analogWrite(blueLedPin, blueVal);

  //Writing on the serial monitor the values that are sent to the LED
  Serial.print("Red = ");
  Serial.print(redVal);
  Serial.print(" Green = ");
  Serial.print(greenVal);
  Serial.print(" Blue = ");
  Serial.println(blueVal);
}
