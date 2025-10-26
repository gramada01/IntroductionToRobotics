/* This code is a management system for 2 traffic lights: 1 for vehicles and one for pedestrians.
The traffic lights stay in a default state (red for pedestrians, green for vehicles) and await the 
press of abutton by a pedestrian. The button triggers a sequence of states (2 of them being green for pedestrians)
and then returns back to the default.
*/

// Constants for the 7 segment display pins
const int pinSegA = 13;
const int pinSegB = 12;
const int pinSegC = 11;
const int pinSegD = 10;
const int pinSegE = 9;
const int pinSegF = 8;
const int pinSegG = 7;

// Constants for the traffic light LED pins
const int pedestrianRedPin = A0;
const int pedestrianGreenPin = A1;
const int vehicleRedPin = A2;
const int vehicleYellowPin = A3;
const int vehicleGreenPin = A4;

// Constants for the button and buzzer pins
const int buttonPin = 2;
const int buzzerPin = 5;

const int segSize = 7; // number of segments used
const int noOfDigits = 11; // number of possible things to show on the display
const int noOfLights = 5; // number of LEDs
const int noOfStates = 4; // number of system states

// array for display segment iteration
int segments[segSize] = {
  pinSegA, pinSegB, pinSegC, pinSegD, pinSegE, pinSegF, pinSegG
};

// array for LEDs iteration
int lights[noOfLights] = {
  pedestrianRedPin, pedestrianGreenPin, vehicleRedPin, vehicleYellowPin, vehicleGreenPin
};

// matrix for possible things to show on display
byte digitMatrix[noOfDigits][segSize] = {
  // a b c d e f g
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
  {0, 1, 1, 0, 0, 1, 1}, // 4
  {1, 0, 1, 1, 0, 1, 1}, // 5
  {1, 0, 1, 1, 1, 1, 1}, // 6
  {1, 1, 1, 0, 0, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 1, 0, 1, 1}, // 9
  {0, 0, 0, 0, 0, 0, 1}  // -
};

// Constants for state durations (in seconds)
const int afterButtonDuration = 8;
const int state2Duration = 3;
const int state3Duration = 8;
const int state4Duration = 4;

// matrix for possible LEDs states
byte lightStates[noOfStates][noOfLights] = {
  // pR, pG, vR, vY, vG (p - pedestrian, v - vehicle, R - red, Y - yellow, g - green)
  {1, 0, 0, 0, 1}, // State 1 (red pedestrians, green vehicles)
  {1, 0, 0, 1, 0}, // State 2 (red pedestrians, yellow vehicles)
  {0, 1, 1, 0, 0}, // State 3 (green pedestrians, red vehicles)
  {0, 1, 1, 0, 0}  // State 4 (green pedestrians, red vehicles)
};

// array for state durations
byte stateDurations[noOfStates] = {
  afterButtonDuration, state2Duration, state3Duration, state4Duration
};

void setup() {
  // Setup of display segment pins
  for(int i = 0; i < segSize; ++i)
    pinMode(segments[i], OUTPUT);

  // Setup for LED pins
  for(int i = 0; i < noOfLights; ++i)
    pinMode(lights[i], OUTPUT);
  
  // Setup for button pin
  pinMode(buttonPin, INPUT_PULLUP);

  // Interrupt that listens to the button pin
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleInterrupt, FALLING);

}

// variables for debounce checking
int lastDebounceTime = 0;
int debounceDelay = 50;

bool possiblePress = false; // boolean that announces a possible button press
bool shouldStartCounter = false; // boolean for debounce counter start
bool canBePressed = true; // boolean for whether to listen to button presses
bool startCountdown = false;  // boolean for countdown (refering to state durations) loop

// Interrupt function that signalizes a posiible press and allows the debounce counter to start
void handleInterrupt(){
  possiblePress = true;
  shouldStartCounter = true;
}

// variable for millis(); utilized in countdown
unsigned long long lastCheckedTime = 0;

// variable for countdown number
byte countdownNumber = 8;

// index for digitMatrix; what to show on the display in state 1 (default)
const byte state1DisplayIndex = 10;

// constants for states
const int defaultState = 1; // state 1
const int transitionState = 2; // state 2
const int pedestrianState = 3; // state 3
const int warningState = 4; // state 4

// constant for one second
const int second = 1000;

int currentState = defaultState;

// function for checking whether the button was pressed
// only works in state 1
bool buttonCheckFunction(){
  bool check = false;

  if(possiblePress && canBePressed){
    //starting the debounce counter
    if(shouldStartCounter){
      lastDebounceTime = millis();
      shouldStartCounter = false;
    }

    if(millis() - lastDebounceTime > debounceDelay){
      int buttonCurrentState = digitalRead(buttonPin);
      if(buttonCurrentState == LOW){
        // if enough time has passed(50 milliseconds) and the button is still pressed
        check = true; // returns true
        canBePressed = false; // makes the button do nothing until the state reverts back to 1
        startCountdown = true; // for starting the countdown
        countdownNumber = stateDurations[currentState - 1]; // countdown for wait time between state 1 and state 2
        lastCheckedTime = millis();
      }
      possiblePress = false;
    }
  }
  return check;
}

// state 1 function
void defaultStateFunction(){
  // traffic lights for state 1
  for(int i = 0; i < noOfLights; ++i)
    digitalWrite(lights[i], lightStates[currentState - 1][i]);

  if(!buttonCheckFunction() && canBePressed) // buuton press check
    displayNumber(state1DisplayIndex); // symbol to show on display while in state 1
  else
    // parameters setup for state 2
    if(countdownNumber == 0){
      currentState++; // switches to next state
      startCountdown = true;
      countdownNumber = stateDurations[currentState - 1];
    }
}

long long int lastBuzzTime = 0; // variable for millis(); for checking the last time the buzzer state was changed
int buzzDelay = 900; // time for the buzzer to be off in milliseconds
int silentDelay = 100; // time for the buzzer to be on in milliseconds
int buzzerTone = 3000;
bool buzzerState = false; // false = off, true = on

// state 2 function
void transitionStateFunction(){
  // traffic lights for state 2
  for(int i = 0; i < noOfLights; ++i)
    digitalWrite(lights[i], lightStates[currentState - 1][i]);

  // parameters setup for state 3
  if(countdownNumber == 0){
      currentState++; // switches to next state
      startCountdown = true;
      lastBuzzTime = millis();
      buzzDelay = 900;
      silentDelay = 100;
      buzzerState = true;
      countdownNumber = stateDurations[currentState - 1];
    }
}

long long int lastBlinkTime = 0; // variable for millis(); for checking the last time the the pedestrian green light blinked

// state 3 function
void pedestrianStateFunction(){
  // traffic lights for state 3
  for(int i = 0; i < noOfLights; ++i)
    digitalWrite(lights[i], lightStates[currentState - 1][i]);
  
  // buzzer management
  // checks if the buzzer is on or off and how much time to be on and off
  // buzzer lasts 100 miliiseconds every 900 milliseconds -> aproximately once a second
  if(buzzerState){
    if(millis() - lastBuzzTime > buzzDelay){
      tone(buzzerPin, buzzerTone);
      lastBuzzTime = millis();
      buzzerState = !buzzerState;
    }
  }
  else
    if(millis() - lastBuzzTime > silentDelay){
      noTone(buzzerPin);
      lastBuzzTime = millis();
      buzzerState = !buzzerState;
    }

  // parameters setup for state 4
  if(countdownNumber == 0){
      currentState++; // switches to next state
      startCountdown = true;
      lastBlinkTime = millis();
      lastBuzzTime = millis();
      buzzDelay = 400;
      silentDelay = 100;
      buzzerState = true;
      noTone(buzzerPin);
      countdownNumber = stateDurations[currentState - 1];
    }
}

int blinkDelay = 250; // wait time for switching the the green LED state (off->on or on->off)

int greenVal = HIGH; // variable green LED state

// state 4 function
void warningStateFunction(){
  // Since the traffic lights are the same as in state 3 there is no need for setting them up again

  // pedestrian green LED blinking
  // blinks 2 times in a second -> switches state every 250 milliseconds
  if(millis() - lastBlinkTime > blinkDelay){
    greenVal = !greenVal;
    digitalWrite(pedestrianGreenPin, greenVal);
    lastBlinkTime = millis();
  }

  // buzzer management
  // checks if the buzzer is on or off and how much time to be on and off
  // buzzer lasts 100 miliiseconds every 400 milliseconds -> aproximately twice a second
  if(buzzerState){
    if(millis() - lastBuzzTime > buzzDelay){
      tone(buzzerPin, buzzerTone);
      lastBuzzTime = millis();
      buzzerState = !buzzerState;
    }
  }
  else
    if(millis() - lastBuzzTime > silentDelay){
      noTone(buzzerPin);
      lastBuzzTime = millis();
      buzzerState = !buzzerState;
    }

  // parameters setup for state 1
  if(countdownNumber == 0){
      currentState = defaultState; // switches to state 1
      canBePressed = true; // lets the button function
      noTone(buzzerPin); // makes sure the buzzer is off
    }
}

void loop() {
  // put your main code here, to run repeatedly:

  if(startCountdown){
    // displays current countdown number
    displayNumber(countdownNumber);
    // if the countdown reaches 0  it stops
    if(countdownNumber == 0){
      startCountdown = false;
      lastCheckedTime = millis();
    }
    else
      // checks if a second has passed
      if(millis() - lastCheckedTime > second){
        countdownNumber--; // decrements the countdown number
        lastCheckedTime = millis();
      }
  }

  // checks the systems's current state and calls its specific function
  switch(currentState){
    case defaultState:
      defaultStateFunction();
      break;

    case transitionState:
      transitionStateFunction();
      break;

    case pedestrianState:
      pedestrianStateFunction();
      break;

    case warningState:
      warningStateFunction();
      break;  
  }
}

// function for showing a symbol or number on the display
void displayNumber(byte digit) {
  for (int i = 0; i < segSize; ++i) {
    digitalWrite(segments[i], digitMatrix[digit][i]);
  }
}
