// A memory-based game implemented on Arduino with 4 seven-segment displays. 
// Players must remember and recreate a sequence of characters shown on the displays.

// Shift register control pins
const byte latchPin = 11;
const byte clockPin = 10;
const byte dataPin = 12;

// 7-segment display digit control pins
const byte segD4 = 4;
const byte segD3 = 5;
const byte segD2 = 6;
const byte segD1 = 7;

// Joystick input pins
const int joystickYAxisPin = A5;
const int joystickXAxisPin = A4;
const int joystickButtonPin = 2;

const int pauseButtonPin = 3;
const int buzzerPin = A3;

// Buzzer state constants
const int buzzerIdle = 0;
const int buzzerTick = 1;
const int buzzerClick = 2;
const int buzzerSuccess = 3;
const int buzzerError = 4;

int buzzerState = buzzerIdle;
unsigned long buzzerStartTime = 0;
int buzzerCurrentNote = 0;
bool buzzerNoteActive = false;

// Buzzer sound parameters
const int tickDuration = 30;
const int tickFrequency = 800;
const int clickDuration = 50;
const int clickFrequency = 1000;
const int errorFrequency = 200;
const int errorToneDuration = 300;

// Success jingle notes (frequency, duration in ms)
const int successNotes[] = {523, 659, 784, 1047}; // C5, E5, G5, C6
const int successNoteDurations[] = {150, 150, 150, 300};
const int successNoteCount = 4;

// Display configuration
byte displayDigits[] = {segD1, segD2, segD3, segD4};
const byte displayCount = 4;
const int encodingsNumber = 23;

// 7-segment encodings: bit pattern for each character (A-G, DP)
const byte encodingArray[encodingsNumber] = {
  B11111100, // 0 or O
  B01100000, // 1
  B11011010, // 2
  B11110010, // 3
  B01100110, // 4
  B10110110, // 5 or S
  B10111110, // 6
  B11100000, // 7
  B11111110, // 8
  B11110110, // 9
  B11101110, // A
  B00111110, // b
  B10011100, // C
  B01111010, // d
  B10011110, // E
  B10001110, // F
  B00011100, // L
  B11001110, // P
  B11011110, // R
  B00011110, // t
  B01110110, // y
  B01111100, // U
  B00000000  // blank
};



void setup() {
  // Shift register pins
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  // Input pins
  pinMode(joystickYAxisPin, INPUT);
  pinMode(joystickXAxisPin, INPUT);
  pinMode(joystickButtonPin, INPUT_PULLUP);
  pinMode(pauseButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  // Initialize display digit pins
  for (byte i = 0; i < displayCount; i++) {
    pinMode(displayDigits[i], OUTPUT);
    digitalWrite(displayDigits[i], LOW);
  }

  // Attach interrupt handlers
  attachInterrupt(digitalPinToInterrupt(joystickButtonPin), joystickButtonInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(pauseButtonPin), pauseButtonInterrupt, FALLING);

  randomSeed(analogRead(A0));
  Serial.begin(9600);
}

// Interrupt flags
bool possibleButtonPress = false;
long long int lastButtonPressTime;
bool possiblePauseButtonPress = false;

void joystickButtonInterrupt() {
  possibleButtonPress = true;
  lastButtonPressTime = millis();
}

void pauseButtonInterrupt() {
  possiblePauseButtonPress = true;
}

// Game state
int currentDigit = 0;
bool digitIsSelected = false;

// Joystick calibration
const int joystickIdle = 500;
const int joystickError = 200;

// Joystick state machine
const int joystickIdleState = 0;
const int joystickPlusState = 1;
const int joystickMinusState = -1;
int joystickState = joystickIdleState;
int joystickLastState = joystickIdleState;

// Joystick action codes
const int joystickPlus = 2;
const int joystickMinus = 1;
const int joystickPress = 0;
const int joystickNoAction = -1;

int joystickAction = -1;
int joystickValue;
int buttonValue;
int lastButtonValue = HIGH;

int checkJoystick() {
  // Read appropriate axis based on selection mode
  if(digitIsSelected)
    joystickValue = analogRead(joystickYAxisPin);
  else
    joystickValue = analogRead(joystickXAxisPin);

  // State machine for joystick movement detection
  if(joystickState == joystickIdleState) {
    if(joystickValue > joystickIdle + joystickError) {
      joystickLastState = joystickState;
      joystickState = joystickMinusState;
    }
    if(joystickValue < joystickIdle - joystickError) {
      joystickLastState = joystickState;
      joystickState = joystickPlusState;
    }
  }
  
  if(joystickState == joystickMinusState) {
    if(joystickValue > joystickIdle + joystickError) {
      if(joystickLastState == joystickIdleState) {
        joystickLastState = joystickMinusState;
        return joystickMinus;
      }
    } else {
      joystickState = joystickIdleState;
    }
  }
  
  if(joystickState == joystickPlusState) {
    if(joystickValue < joystickIdle - joystickError) {
      if(joystickLastState == joystickIdleState) {
        joystickLastState = joystickPlusState;
        return joystickPlus;
      }
    } else {
      joystickState = joystickIdleState;
    }
  }
  
  // Check for button press
  if(possibleButtonPress) {
    buttonValue = digitalRead(joystickButtonPin);
    if(buttonValue == LOW && lastButtonValue == HIGH) {
      lastButtonValue = buttonValue;
      return joystickPress;
    }
    possibleButtonPress = false;
  }
  
  lastButtonValue = HIGH;
  return joystickNoAction;
}

// Display multiplexing
int digitIndex = 0;
long long int lastBlink = millis();
const int defaultBlinkDelay = 125;
const int selectedBlinkDelay = 500;
int delayValue;
bool blinkOn = true;

// Game data
int displayValuesForPlayer[displayCount] = {0, 0, 0, 0};
long gameValues[displayCount] = {0, 0, 0, 0};
bool ongoingSession = false;

// Game states
bool menuOn = true;
bool gameOn = false;
bool gameTransitionOn = false;

// Game timing
const int defaultGameTransitionDuration = 5000;
int currentGameTransitioDuration = defaultGameTransitionDuration;
long long int gameTransitionStartTime;
bool checkForLongPress = false;
const int longPressDelay = 4000;

// Error and success states
long long int errorStartTime;
const int errorDuration = 3000;
bool errorOn = false;
bool successOn = false;

// Scoring
int score = 0;
int highScore = 0;
bool showScore = false;
int scoreDisplay[displayCount] = {22, 22, 22, 0};

// Success display animation
const int successDisplayDigits = 8;
int successDisplay[successDisplayDigits] = {5, 21, 12, 12, 14, 5, 5, 22}; // "SUCCESS "
long long int lastScrollTime;
int scrollDelay = 500;
int numberOfScrolls = 0;
int scrollIndex = 0;

void checkResult() {
  gameOn = false;
  
  // Check if player's answer matches
  for(int i = 0; i < displayCount; ++i) {
    if(gameValues[i] != displayValuesForPlayer[i]) {
      // Wrong answer
      errorOn = true;
      if(score > highScore)
        highScore = score;
      score = 0;
      currentGameTransitioDuration = defaultGameTransitionDuration;
      ongoingSession = false;
      errorStartTime = millis();
      playError();
      return;
    }
  }

  // Correct answer
  successOn = true;
  score++;
  
  // Decrease transition time with score (minimum 1 second)
  if(currentGameTransitioDuration > 1000)
    currentGameTransitioDuration = defaultGameTransitionDuration - score * 1000;
  else
    currentGameTransitioDuration = 1000;
  
  numberOfScrolls = 0;
  scrollIndex = 0;
  playSuccess();
  
  // Generate new sequence
  for(int i = 0; i < displayCount; ++i)
    gameValues[i] = random(encodingsNumber - 1);
  
  if(score > highScore)
    highScore = score;
  
  lastScrollTime = millis();
}

void gameLogicFunction() {
  // Toggle digit selection on button press
  if(joystickAction == joystickPress) {
    checkForLongPress = true;
    digitIsSelected = !digitIsSelected;
    playClick();
  }

  // Check for long press to submit answer
  buttonValue = digitalRead(joystickButtonPin);
  if(checkForLongPress) {
    if(buttonValue == LOW) {
      if(millis() - lastButtonPressTime > longPressDelay) {
        checkResult();
      }
    } else {
      checkForLongPress = false;
    }
  }
  
  if(digitIsSelected) {
    // Change selected digit value
    if(joystickAction == joystickPlus) {
      if(displayValuesForPlayer[currentDigit] == encodingsNumber - 2)
        displayValuesForPlayer[currentDigit] = 0;
      else
        displayValuesForPlayer[currentDigit]++;
    }

    if(joystickAction == joystickMinus) {
      if(displayValuesForPlayer[currentDigit] == 0)
        displayValuesForPlayer[currentDigit] = encodingsNumber - 2;
      else
        displayValuesForPlayer[currentDigit]--;
    }
  } else {
    // Move cursor between digits
    if(joystickAction == joystickMinus) {
      if(currentDigit == displayCount - 1)
        currentDigit = 0;
      else
        currentDigit++;
      playTick();
    }

    if(joystickAction == joystickPlus) {
      if(currentDigit == 0)
        currentDigit = displayCount - 1;
      else
        currentDigit--;
      playTick();
    }
  }
}

// Menu system
int menuIndex = 0;
const int numberOfMenuOptions = 3;
const int playIndex = 0;
const int scorIndex = 1;
const int stopIndex = 2;

const int menuDisplayOptions[numberOfMenuOptions][displayCount] = {
  {17, 16, 10, 20}, // "PLAY"
  {5, 12, 0, 18},   // "SCOR"
  {5, 19, 0, 17}    // "STOP"
};

const int errorDisplay[displayCount] = {14, 18, 18, 22}; // "ERR "

void optionToFunctionMap() {
  if(menuIndex == playIndex) {
    // Start game
    menuOn = false;
    gameTransitionOn = true;
    joystickAction = joystickNoAction;
    if(ongoingSession == false) {
      for(int i = 0; i < displayCount; ++i)
        gameValues[i] = random(encodingsNumber - 1);
      ongoingSession = true;
    }
    gameTransitionStartTime = millis();
  }

  if(menuIndex == scorIndex) {
    // Show high score
    showScore = true;
    menuOn = false;
    int scoreCopy = highScore;
    for(int i = displayCount - 1; i >= 0; --i) {
      if(i == displayCount - 1) {
        scoreDisplay[i] = scoreCopy % 10;
        scoreCopy = scoreCopy / 10;
      } else {
        if(scoreCopy == 0) {
          scoreDisplay[i] = 22; // blank
        } else {
          scoreDisplay[i] = scoreCopy % 10;
          scoreCopy = scoreCopy / 10;
        }
      }
    }
    joystickAction = joystickNoAction;
  }
  
  if(menuIndex == stopIndex) {
    // Reset game
    ongoingSession = false;
    currentGameTransitioDuration = defaultGameTransitionDuration;
    score = 0;
    joystickAction = joystickNoAction;
  }
}

void gameTransitionFunction() {
  // Wait for transition duration, then start game
  if(millis() - gameTransitionStartTime > currentGameTransitioDuration) {
    gameTransitionOn = false;
    gameOn = true;
  }
}

void menuLogicFunction() {
  digitIsSelected = false;
  
  // Select menu option
  if(joystickAction == joystickPress) {
    optionToFunctionMap();
  }

  // Navigate menu
  if(joystickAction == joystickMinus) {
    if(menuIndex == numberOfMenuOptions - 1)
      menuIndex = 0;
    else
      menuIndex++;
    playTick();
  }

  if(joystickAction == joystickPlus) {
    if(menuIndex == 0)
      menuIndex = numberOfMenuOptions - 1;
    else
      menuIndex--;
    playTick();
  }
}

int lastPauseButtonValue = HIGH;
int pauseButtonValue = HIGH;

// Buzzer feedback functions
void playTick() {
  if(buzzerState == buzzerIdle) {
    buzzerState = buzzerTick;
    buzzerStartTime = millis();
    tone(buzzerPin, tickFrequency);
  }
}

void playClick() {
  if(buzzerState == buzzerIdle) {
    buzzerState = buzzerClick;
    buzzerStartTime = millis();
    tone(buzzerPin, clickFrequency);
  }
}

void playSuccess() {
  if(buzzerState == buzzerIdle || buzzerState == buzzerSuccess) {
    buzzerState = buzzerSuccess;
    buzzerStartTime = millis();
    buzzerCurrentNote = 0;
    buzzerNoteActive = true;
    tone(buzzerPin, successNotes[0]);
  }
}

void playError() {
  buzzerState = buzzerError;
  buzzerStartTime = millis();
  tone(buzzerPin, errorFrequency);
}

void updateBuzzer() {
  // Non-blocking buzzer state machine
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - buzzerStartTime;

  switch(buzzerState) {
    case buzzerIdle:
      break;

    case buzzerTick:
      if(elapsedTime >= tickDuration) {
        noTone(buzzerPin);
        buzzerState = buzzerIdle;
      }
      break;

    case buzzerClick:
      if(elapsedTime >= clickDuration) {
        noTone(buzzerPin);
        buzzerState = buzzerIdle;
      }
      break;

    case buzzerSuccess:
      // Play jingle sequence
      if(buzzerCurrentNote < successNoteCount) {
        int noteStartTime = 0;
        for(int i = 0; i < buzzerCurrentNote; i++) {
          noteStartTime += successNoteDurations[i];
        }
        int noteEndTime = noteStartTime + successNoteDurations[buzzerCurrentNote];

        if(elapsedTime >= noteEndTime) {
          buzzerCurrentNote++;
          buzzerNoteActive = false;
          if(buzzerCurrentNote < successNoteCount) {
            tone(buzzerPin, successNotes[buzzerCurrentNote]);
            buzzerNoteActive = true;
          } else {
            noTone(buzzerPin);
            buzzerState = buzzerIdle;
            buzzerNoteActive = false;
          }
        }
      }
      break;

    case buzzerError:
      if(elapsedTime >= errorToneDuration) {
        noTone(buzzerPin);
        buzzerState = buzzerIdle;
      }
      break;
  }
}

void pauseButtonCheck() {
  // Handle pause button to return to menu
  if(possiblePauseButtonPress) {
    pauseButtonValue = digitalRead(pauseButtonPin);
    if(pauseButtonValue == LOW && lastPauseButtonValue == HIGH) {
      if(!(menuOn || errorOn || successOn)) {
        lastPauseButtonValue = pauseButtonValue;
        
        // Save remaining transition time if in transition
        if(gameTransitionOn) {
          currentGameTransitioDuration = currentGameTransitioDuration - (millis() - gameTransitionStartTime);
        } else {
          currentGameTransitioDuration = 0;
        }

        // Return to menu
        gameOn = false;
        errorOn = false;
        successOn = false;
        gameTransitionOn = false;
        menuOn = true;
        return;
      }
    }
    possiblePauseButtonPress = false;
  }
  lastPauseButtonValue = HIGH;
}

void loop() {
  // Main game loop
  joystickAction = checkJoystick();
  pauseButtonCheck();
  blinkCheck();
  updateBuzzer();
  
  // State machine
  if(menuOn) {
    menuLogicFunction();
  }

  if(gameTransitionOn) {
    gameTransitionFunction();
  }

  if(gameOn) {
    gameLogicFunction();
  }

  // Error state handling
  if(errorOn) {
    if(millis() - errorStartTime > errorDuration) {
      errorOn = false;
      menuOn = true;
      for(int i = 0; i < displayCount; ++i)
        displayValuesForPlayer[i] = 0;
    }
  }

  // Success state handling with scrolling animation
  if(successOn) {
    if(millis() - lastScrollTime > scrollDelay) {
      scrollIndex++;
      lastScrollTime = millis();
    }

    numberOfScrolls = scrollIndex / successDisplayDigits;

    if(numberOfScrolls == 2) {
      successOn = false;
      gameTransitionOn = true;
      for(int i = 0; i < displayCount; ++i)
        displayValuesForPlayer[i] = 0;
      gameTransitionStartTime = millis();
    }
  }

  // Score display
  if(showScore) {
    if(joystickAction == joystickPress) {
      showScore = false;
      menuOn = true;
    }
  }

  display();
}

void blinkCheck() {
  // Adjust blink rate based on selection state
  if(digitIsSelected)
    delayValue = selectedBlinkDelay;
  else
    delayValue = defaultBlinkDelay;

  if(millis() - lastBlink > delayValue) {
    blinkOn = !blinkOn;
    lastBlink = millis();
  }
}

void display() {
  // Multiplexed 7-segment display update
  // Turn off all digits first
  for(int i = 0; i < displayCount; ++i) {
    digitalWrite(displayDigits[i], HIGH);
  }

  digitalWrite(latchPin, LOW);

  // Display based on current game state
  if(menuOn) {
    shiftOut(dataPin, clockPin, MSBFIRST, encodingArray[menuDisplayOptions[menuIndex][digitIndex]]);
    digitalWrite(latchPin, HIGH);
    digitalWrite(displayDigits[digitIndex], LOW);
  }

  if(gameTransitionOn) {
    shiftOut(dataPin, clockPin, MSBFIRST, encodingArray[gameValues[digitIndex]]);
    digitalWrite(latchPin, HIGH);
    digitalWrite(displayDigits[digitIndex], LOW);
  }

  if(gameOn) {
    shiftOut(dataPin, clockPin, MSBFIRST, encodingArray[displayValuesForPlayer[digitIndex]]);
    digitalWrite(latchPin, HIGH);

    // Blink selected digit
    if(digitIndex == currentDigit) {
      if(blinkOn)
        digitalWrite(displayDigits[digitIndex], LOW);
      else
        digitalWrite(displayDigits[digitIndex], HIGH);
    } else {
      digitalWrite(displayDigits[digitIndex], LOW);
    }
  }

  if(errorOn) {
    shiftOut(dataPin, clockPin, MSBFIRST, encodingArray[errorDisplay[digitIndex]]);
    digitalWrite(latchPin, HIGH);
    digitalWrite(displayDigits[digitIndex], LOW);
  }

  if(successOn) {
    shiftOut(dataPin, clockPin, MSBFIRST, encodingArray[successDisplay[(digitIndex + scrollIndex) % successDisplayDigits]]);
    digitalWrite(latchPin, HIGH);
    digitalWrite(displayDigits[digitIndex], LOW);
  }

  if(showScore) {
    shiftOut(dataPin, clockPin, MSBFIRST, encodingArray[scoreDisplay[digitIndex]]);
    digitalWrite(latchPin, HIGH);
    digitalWrite(displayDigits[digitIndex], LOW);
  }

  // Cycle to next digit
  if(digitIndex == displayCount - 1)
    digitIndex = 0;
  else
    digitIndex++;
}
