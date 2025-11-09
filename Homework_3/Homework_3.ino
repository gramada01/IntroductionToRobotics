#include <string.h>
#include <stdlib.h>
#include <EEPROM.h>

// Pin definition
const int buzzerPin = 6;
const int redLedPin = 11;
const int greenLedPin = 10;
const int LDRPin = A4;
const int ultrasonicTriggerPin = 5;
const int ultrasonicEchoPin = A5;
const int joystickYAxisPin = A3;
const int joystickButtonPin = 2;

// Menu types enum
enum menuType{
    NORMAL,
    SUBMENU
};

// Menu class
class menu{
private:
    int numberOfLinesBetweenMenuDisplays = 20;

public:
    const char** options;
    int numberOfOptions;
    int optionIndex = 0;
    menuType type;
    int indexOfPreviousMenu; // only for menuType::SUBMENU

    menu(const char* opt[], int num, menuType typ, int indexOfPrevMenu = 0){
        options = opt;
        numberOfOptions = num;
        type = typ;
        indexOfPreviousMenu = indexOfPrevMenu;
    }
    
    // Displays menu options with current selection highlighted
    void displayOptions(){
        Serial.println();
        for(int i = 0; i < numberOfOptions; ++i){
            if(i == optionIndex){
                Serial.print(options[i]);
                Serial.println(" <-");
            }
            else {
                Serial.println(options[i]); 
            }
        }
        Serial.println();
    }
};

bool possibleButtonPress = false;
// Function to handle possible button press
void interruptFunction(){
    possibleButtonPress = true;
}

// Options arrays for each menu
const char* defaultArmedMenuOptions[] = {"Disarm System"};
int numberOfDefaultArmedMenuOptions = 1;

const char* disarmSuboptions[] = {"Back"};
int numberOfDisarmSuboptions = 1;

const char* defaultDisarmedMenuOptions[] = {"Arm System", "Alarm test", "Settings"};
int numberOfDefaultDisarmedMenuOptions = 3;

const char* alarmTestMenuOptions[] = {"Start alarm", "Stop alarm", "Back"};
int numberOfAlarmTestMenuOptions = 3;

const char* settingsMenuOptions[] = {"Set Ultrasonic Sensitivity", "Set LDR Light Threshold", "Set Buzzer Tone or Pattern", "Set System Name", "Change Password", "Back"};
int numberOfSettingsMenuOptions = 6;

const char* ultrasonicSensitivityMenuOptions[] = {"Back"};
int numberOfUltrasonicSensitivityMenuOptions = 1;

const char* LDRMenuOptions[] = {"Back"};
int numberOfLDRMenuOptions = 1;

const char* buzzerMenuOptions[] = {"Back"};
int numberOfBuzzerMenuOptions = 1;

const char* systemNameMenuOptions[] = {"Back"};
int numberOfSystemNameMenuOptions = 1;

const char* passwordMenuOptions[] = {"Back"};
int numberOfPasswordMenuOptions = 1;

String inputString = ""; 

//  Security System class
class SecuritySystem{
private:

    // Joystick threshold values
    const int joystickIdle = 500;
    const int joystickError = 100;
    int joystickValue;
    int lastAxisValue = 500;

    bool buttonWasPressed = false;
    int buttonValue;
    int lastButtonValue = HIGH;

    const int joystickIdleState = 0;
    const int joystickUpState = 1;
    const int joystickDownState = -1;
    int joystickState = joystickIdleState;
    int joystickLastState = joystickIdleState;

public:
    static const int defaultMenuIndex = 0;
    static const int disarmingMenuIndex = 1;
    static const int alarmTestMenuIndex = 1;
    static const int settingsMenuIndex = 2;
    static const int ultrasonicSensitivityMenuIndex = 3;
    static const int LDRMenuIndex = 4;
    static const int buzzerMenuIndex = 5;
    static const int systemNameMenuIndex = 6;
    static const int passwordMenuIndex = 7;
    
    bool systemBootingUp = true;
    bool systemArmed = true;
    bool inputEnabled = false;
    bool passwordCheckEnabled = false;
    bool distanceInputEnabled = false;
    bool LDRInputEnabled = false;
    bool buzzerToneInputEnabled = false;
    bool systemNameInputEnabled = false;
    bool passwordChangeEnabled = false;
    bool passwordOldCheckEnabled = false;

    long long int lastCheckedTime;
    bool delayStart = false;
    int armingDelay = 3000;
    bool isNotAlreadyArming = true;

    int joystickAction;
    const int joystickUp = 2;
    const int joystickDown = 1;
    const int joystickPress = 0;
    const int joystickNoAction = -1;

    String systemName = "Chiscan Alarm System";

    float distanceToObject;
    float distanceThreshold = 15;
    long duration;

    int LDRThreshold = 300;
    int LDRValue;

    long long int lastBlinkTime;
    int blinkDelay = 250;
    int redLedValue = LOW;

    String password = "123";

    bool alarmOn = false;
    long long int lastBuzzTime;
    int buzzDelay = 300;
    int silentDelay = 100;
    bool buzzerState = false;
    int buzzerTone = 3000;

    int currentArmedMenu = defaultMenuIndex;
    int currentDisarmedMenu = defaultMenuIndex;

    menu armedMenus[2];
    menu disarmedMenus[8];

    SecuritySystem() 
    : armedMenus{
        menu(defaultArmedMenuOptions, numberOfDefaultArmedMenuOptions, menuType::NORMAL, defaultMenuIndex),
        menu(disarmSuboptions, numberOfDisarmSuboptions, menuType::SUBMENU, defaultMenuIndex)
      },
      disarmedMenus{
        menu(defaultDisarmedMenuOptions, numberOfDefaultDisarmedMenuOptions, menuType::NORMAL, defaultMenuIndex),
        menu(alarmTestMenuOptions, numberOfAlarmTestMenuOptions, menuType::SUBMENU, defaultMenuIndex),
        menu(settingsMenuOptions, numberOfSettingsMenuOptions, menuType::SUBMENU, defaultMenuIndex),
        menu(ultrasonicSensitivityMenuOptions, numberOfUltrasonicSensitivityMenuOptions, menuType::SUBMENU, settingsMenuIndex),
        menu(LDRMenuOptions, numberOfLDRMenuOptions, menuType::SUBMENU, settingsMenuIndex),
        menu(buzzerMenuOptions, numberOfBuzzerMenuOptions, menuType::SUBMENU, settingsMenuIndex),
        menu(systemNameMenuOptions, numberOfSystemNameMenuOptions, menuType::SUBMENU, settingsMenuIndex),
        menu(passwordMenuOptions, numberOfPasswordMenuOptions, menuType::SUBMENU, settingsMenuIndex)
      }
    {}

    const int EEPROM_START_ADDRESS = 0;
    const int MAGIC_NUMBER = 12345; 

    // Saves current settings to EEPROM
    void saveSettings() {
        EEPROM.put(EEPROM_START_ADDRESS, MAGIC_NUMBER); 
    
        EEPROM.put(2, distanceThreshold);
        EEPROM.put(6, LDRThreshold);
        EEPROM.put(8, buzzerTone);
    
        char tempName[51];
        systemName.toCharArray(tempName, 51);
        EEPROM.put(10, tempName);
    
        char tempPassword[21];
        password.toCharArray(tempPassword, 21);
        EEPROM.put(60, tempPassword);

        Serial.println("Settings saved to EEPROM.");
    }

    // Loads settings from EEPROM on boot
    void loadSettings() {
        int savedMarker;
        EEPROM.get(EEPROM_START_ADDRESS, savedMarker); 

        if (savedMarker == MAGIC_NUMBER) { 
            EEPROM.get(2, distanceThreshold);
            EEPROM.get(6, LDRThreshold);
            EEPROM.get(8, buzzerTone);

            char tempName[51]; 
            EEPROM.get(10, tempName);
            systemName = String(tempName);

            char tempPassword[21]; 
            EEPROM.get(60, tempPassword);
            password = String(tempPassword);
            
            Serial.println("Settings loaded from EEPROM.");
        } 
        else { 
            Serial.println("EEPROM not initialized. Using default settings.");
            saveSettings(); 
        }
    }

    // Processes joystick movements and button presses
    int checkJoystick(){
        joystickValue = analogRead(joystickYAxisPin);

        if(joystickState == joystickIdleState){
            if(joystickValue > joystickIdle + joystickError){
                joystickLastState = joystickState;
                joystickState = joystickDownState;
            }
            if(joystickValue < joystickIdle - joystickError){
                joystickLastState = joystickState;
                joystickState = joystickUpState;
            }
        }
        if(joystickState == joystickDownState){
            if(joystickValue > joystickIdle + joystickError){
                if(joystickLastState == joystickIdleState){
                    joystickLastState = joystickDownState;
                    return joystickDown;
                }
            }
            else joystickState = joystickIdleState;
        }
        if(joystickState == joystickUpState){
            if(joystickValue < joystickIdle - joystickError){
                if(joystickLastState == joystickIdleState){
                    joystickLastState = joystickUpState;
                    return joystickUp;
                }
            }
            else joystickState = joystickIdleState;
        }
        if(possibleButtonPress){
            buttonValue = digitalRead(joystickButtonPin);
            if(buttonValue == LOW && lastButtonValue == HIGH){
                lastButtonValue = buttonValue;
                return joystickPress;
            }
            possibleButtonPress = false;
        }
        lastButtonValue = HIGH;
        return joystickNoAction;
    }

    // Activates password input for disarming
    void enablePasswordCheck(){
        if(systemArmed){
            inputEnabled = true;
            passwordCheckEnabled = true;
            inputString = "";
            Serial.println("Insert Password");
        }
    }

    // Activates distance threshold input mode
    void enableDistanceInput(){
        distanceInputEnabled = true;
        inputEnabled = true;
        inputString = "";
        Serial.println("Enter distance threshold (cm):");
    }

    // Activates LDR threshold input mode
    void enableLDRInput(){
        LDRInputEnabled = true;
        inputEnabled = true;
        inputString = "";
        Serial.println("Enter LDR threshold (0-1023):");
    }

    // Activates buzzer tone input mode
    void enableBuzzerToneInput(){
        buzzerToneInputEnabled = true;
        inputEnabled = true;
        inputString = "";
        Serial.println("Enter buzzer tone (Hz):");
    }

    // Activates system name input mode
    void enableSystemNameInput(){
        systemNameInputEnabled = true;
        inputEnabled = true;
        inputString = "";
        Serial.println("Enter system name:");
    }

    // Activates password change input mode (requires old password first)
    void enablePasswordChange(){
        passwordOldCheckEnabled = true;
        inputEnabled = true;
        inputString = "";
        Serial.println("Enter old password:");
    }

    // Returns to previous menu and clears all input flags
    void backFunction(){
        inputEnabled = false;
        passwordCheckEnabled = false;
        distanceInputEnabled = false;
        LDRInputEnabled = false;
        buzzerToneInputEnabled = false;
        systemNameInputEnabled = false;
        passwordChangeEnabled = false;
        passwordOldCheckEnabled = false;
        inputString = "";

        if(systemArmed){
            currentArmedMenu = armedMenus[currentArmedMenu].indexOfPreviousMenu;    
        }
        else{
            currentDisarmedMenu = disarmedMenus[currentDisarmedMenu].indexOfPreviousMenu;
        }
    }

    // Starts 3-second arming delay
    void armingFunction(){
        isNotAlreadyArming = false;
        delayStart = true;
        Serial.println("Arming system in 3 seconds!");
        currentArmedMenu = defaultMenuIndex;
        lastCheckedTime = millis();
        alarmOn = false;
        noTone(buzzerPin);
    }

    // Maps menu selections to corresponding functions
    void optionToFunctionMap(){
        if(systemArmed){
            switch(currentArmedMenu){
                case defaultMenuIndex:
                    if (strcmp(armedMenus[currentArmedMenu].options[armedMenus[currentArmedMenu].optionIndex], "Disarm System") == 0) {
                        currentArmedMenu = disarmingMenuIndex;
                        enablePasswordCheck();
                    }
                    break;
                case disarmingMenuIndex:
                    if (strcmp(armedMenus[currentArmedMenu].options[armedMenus[currentArmedMenu].optionIndex], "Back") == 0) {
                        backFunction();
                    }
                    break;
            }
        }
        else{
            switch(currentDisarmedMenu){
                case defaultMenuIndex:
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Arm System") == 0)
                        armingFunction();
                    else if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Alarm test") == 0)
                        currentDisarmedMenu = 1;
                    else if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Settings") == 0)
                        currentDisarmedMenu = 2;
                    break;
                case alarmTestMenuIndex:
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Start alarm") == 0)
                        alarmOn = true;
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Stop alarm") == 0){
                        alarmOn = false;
                        noTone(buzzerPin);
                    }
                
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Back") == 0)
                        backFunction();
                    break;
                case settingsMenuIndex:
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Set Ultrasonic Sensitivity") == 0){
                        currentDisarmedMenu = ultrasonicSensitivityMenuIndex;
                        enableDistanceInput();
                    }
                    else if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Set LDR Light Threshold") == 0){
                        currentDisarmedMenu = LDRMenuIndex;
                        enableLDRInput();
                    }
                    else if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Set Buzzer Tone or Pattern") == 0){
                        currentDisarmedMenu = buzzerMenuIndex;
                        enableBuzzerToneInput();
                    }
                    else if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Set System Name") == 0){
                        currentDisarmedMenu = systemNameMenuIndex;
                        enableSystemNameInput();
                    }
                    else if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Change Password") == 0){
                        currentDisarmedMenu = passwordMenuIndex;
                        enablePasswordChange();
                    }
                    else if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Back") == 0)
                        backFunction();
                    break;
                case ultrasonicSensitivityMenuIndex:
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Back") == 0)
                        backFunction();
                    break;
                case LDRMenuIndex:
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Back") == 0)
                        backFunction();
                    break;
                case buzzerMenuIndex:
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Back") == 0)
                        backFunction();
                    break;
                case systemNameMenuIndex:
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Back") == 0)
                        backFunction();
                    break;
                case passwordMenuIndex:
                    if(strcmp(disarmedMenus[currentDisarmedMenu].options[disarmedMenus[currentDisarmedMenu].optionIndex], "Back") == 0)
                        backFunction();
                    break;
            }
        }
    }

    // Controls LEDs based on system state
    void ledFunction(){
        if(systemArmed){
            if(alarmOn){
                if(millis() - lastBlinkTime > blinkDelay){
                    if(redLedValue == HIGH)
                        redLedValue = LOW;
                    else
                        redLedValue = HIGH;
                    digitalWrite(redLedPin, redLedValue);
                    lastBlinkTime = millis();
                }
            }
            else{
                    digitalWrite(redLedPin, LOW);
                    digitalWrite(greenLedPin, LOW);
                }
        }
        else{
            digitalWrite(redLedPin, LOW);
            digitalWrite(greenLedPin, HIGH);
        }
    }

    // Handles alarm buzzer
    void buzzerFunction(){
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
    }

    // Monitors ultrasonic sensor for intruder detection
    void detectIntruders(){
        
        digitalWrite(ultrasonicTriggerPin, LOW);
        delayMicroseconds(2);
        // Sets the ultrasonicTriggerPin on HIGH state for 10 micro seconds
        digitalWrite(ultrasonicTriggerPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(ultrasonicTriggerPin, LOW);
        // Reads the echoPin, returns the sound wave travel time in microseconds
        duration = pulseIn(ultrasonicEchoPin, HIGH);
        // Sound wave reflects from the obstacle, so to calculate the distance we
        // consider half of the distance traveled.
        distanceToObject = duration*0.034/2;

        if(distanceToObject < distanceThreshold && !alarmOn){
            Serial.println("Intruders detected!");
            currentArmedMenu = disarmingMenuIndex;
            enablePasswordCheck();
            alarmOn = true;
            lastBlinkTime = millis();
            lastBuzzTime = millis();
        }
    }

    // Reads current light level from LDR sensor
    void lightSensing(){
        LDRValue = analogRead(LDRPin);
    }

    // Displays current system status
    void showSystemState(){
        Serial.print(systemName);
        Serial.print(" SYSTEM is ");
        if(systemArmed)
            Serial.println("armed!");
        else
            Serial.println("disarmed!");
    }
};

SecuritySystem sys;

void setup() {
    Serial.begin(9600);

    pinMode(buzzerPin, OUTPUT);
    pinMode(redLedPin, OUTPUT);
    pinMode(greenLedPin, OUTPUT);
    pinMode(LDRPin, INPUT);
    pinMode(ultrasonicTriggerPin, OUTPUT);
    pinMode(ultrasonicEchoPin, INPUT);
    pinMode(joystickYAxisPin, INPUT);
    pinMode(joystickButtonPin, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(joystickButtonPin), interruptFunction, FALLING);

}

void loop() {
    if(sys.systemBootingUp){
        Serial.println();
        sys.loadSettings();
        
        Serial.print(sys.systemName);
        Serial.println(" booting up!");

        if(sys.systemArmed){
            sys.showSystemState();
            sys.armedMenus[sys.currentArmedMenu].displayOptions();
        }
        else{
            sys.showSystemState();
            sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
        }

        sys.systemBootingUp = false;
    }

    sys.lightSensing();

    sys.ledFunction();

    if(sys.delayStart){
        if(millis() - sys.lastCheckedTime > sys.armingDelay){
            Serial.println("System is armed!");
            sys.systemArmed = true;
            sys.delayStart = false;
            sys.showSystemState();
            sys.armedMenus[sys.currentArmedMenu].displayOptions();
        }
    }
    
      sys.joystickAction = sys.checkJoystick();

    // Serial input processing
    if(sys.inputEnabled){
        if(sys.passwordCheckEnabled){
            if(Serial.available() > 0){
                inputString = Serial.readStringUntil('\n');
                inputString.trim();
                
                if(inputString == sys.password){
                    sys.alarmOn = false;
                    noTone(buzzerPin);
                    Serial.println("The password is correct. System Disarmed!");
                    sys.systemArmed = false;
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                    sys.inputEnabled = false;
                    sys.passwordCheckEnabled = false;
                    sys.isNotAlreadyArming = true;
                }
                else{
                    Serial.print("Incorrect password! Try again! ");
                    sys.showSystemState();
                    sys.armedMenus[sys.currentArmedMenu].displayOptions();
                }
                inputString = "";
            }
        }
        if(sys.distanceInputEnabled){
            if(Serial.available() > 0){
                inputString = Serial.readStringUntil('\n');
                inputString.trim();
                
                float newDistance = inputString.toFloat();
                if(newDistance > 0 && newDistance < 400){
                    sys.distanceThreshold = newDistance;
                    Serial.print("Distance threshold set to: ");
                    Serial.println(sys.distanceThreshold);
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                    sys.inputEnabled = false;
                    sys.distanceInputEnabled = false;
                    sys.saveSettings();
                }
                else{
                    Serial.println("Invalid distance! Use 0-400 cm.");
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                }
                
                inputString = "";
            }
        }
        if(sys.LDRInputEnabled){
            if(Serial.available() > 0){
                inputString = Serial.readStringUntil('\n');
                inputString.trim();
                
                int newLDR = inputString.toInt();
                if(newLDR >= 0 && newLDR <= 1023){
                    sys.LDRThreshold = newLDR;
                    Serial.print("LDR threshold set to: ");
                    Serial.println(sys.LDRThreshold);
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                    sys.inputEnabled = false;
                    sys.LDRInputEnabled = false;
                    sys.saveSettings();
                }
                else{
                    Serial.println("Invalid threshold! Use 0-1023.");
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                }
                
                inputString = "";
            }
        }
        if(sys.buzzerToneInputEnabled){
            if(Serial.available() > 0){
                inputString = Serial.readStringUntil('\n');
                inputString.trim();
                
                int newTone = inputString.toInt();
                if(newTone > 0 && newTone <= 10000){
                    sys.buzzerTone = newTone;
                    Serial.print("Buzzer tone set to: ");
                    Serial.print(sys.buzzerTone);
                    Serial.println(" Hz");
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                    sys.inputEnabled = false;
                    sys.buzzerToneInputEnabled = false;
                    sys.saveSettings();
                }
                else{
                    Serial.println("Invalid tone! Use 1-10000 Hz.");
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                }
                
                inputString = "";
            }
        }
        if(sys.systemNameInputEnabled){
            if(Serial.available() > 0){
                inputString = Serial.readStringUntil('\n');
                inputString.trim();
                
                int len = inputString.length();
                if(len > 0 && len <= 50){
                    sys.systemName = inputString;
                    Serial.print("System name set to: ");
                    Serial.println(sys.systemName);
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                    sys.inputEnabled = false;
                    sys.systemNameInputEnabled = false;
                    sys.saveSettings();
                }
                else{
                    Serial.println("Invalid name! Use 1-50 characters.");
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                }
                
                inputString = "";
            }
        }
        if(sys.passwordOldCheckEnabled){
            if(Serial.available() > 0){
                inputString = Serial.readStringUntil('\n');
                inputString.trim();
                
                if(inputString == sys.password){
                    Serial.println("Old password correct. Enter new password:");
                    sys.passwordOldCheckEnabled = false;
                    sys.passwordChangeEnabled = true;
                }
                else{
                    Serial.println("Incorrect old password! Try again:");
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                }
                
                inputString = "";
            }
        }
        if(sys.passwordChangeEnabled){
            if(Serial.available() > 0){
                inputString = Serial.readStringUntil('\n');
                inputString.trim();
                
                int len = inputString.length();
                if(len >= 3 && len <= 20){
                    sys.password = inputString;
                    Serial.println("Password changed successfully!");
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                    sys.inputEnabled = false;
                    sys.passwordChangeEnabled = false;
                    sys.saveSettings();
                }
                else{
                    Serial.println("Invalid password! Use 3-20 characters.");
                    sys.showSystemState();
                    sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
                }
                
                inputString = "";
            }
        }
    }

    // Armed state logic
    if(sys.systemArmed){
        if(sys.alarmOn){
            sys.buzzerFunction();
        }
        else
            sys.detectIntruders();

        if(sys.joystickAction == sys.joystickUp)
            if(sys.armedMenus[sys.currentArmedMenu].optionIndex > 0){
                  sys.armedMenus[sys.currentArmedMenu].optionIndex--;
                  sys.showSystemState();
                  sys.armedMenus[sys.currentArmedMenu].displayOptions();
            }
        
        if(sys.joystickAction == sys.joystickDown)
            if(sys.armedMenus[sys.currentArmedMenu].optionIndex < sys.armedMenus[sys.currentArmedMenu].numberOfOptions - 1){
                sys.armedMenus[sys.currentArmedMenu].optionIndex++;
                sys.showSystemState();
                sys.armedMenus[sys.currentArmedMenu].displayOptions();
            }

        if(sys.joystickAction == sys.joystickPress){
            sys.optionToFunctionMap();
            sys.showSystemState();
            sys.armedMenus[sys.currentArmedMenu].displayOptions();
        }
    }
    // Disarmed state logic
    else{
        if(sys.alarmOn){
            sys.buzzerFunction();
        }

        if(sys.LDRValue < sys.LDRThreshold && sys.isNotAlreadyArming)
                sys.armingFunction();

        if(sys.joystickAction == sys.joystickUp)
            if(sys.disarmedMenus[sys.currentDisarmedMenu].optionIndex > 0){
                sys.disarmedMenus[sys.currentDisarmedMenu].optionIndex--;
                sys.showSystemState();
                sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
            }
        
        if(sys.joystickAction == sys.joystickDown)
            if(sys.disarmedMenus[sys.currentDisarmedMenu].optionIndex < sys.disarmedMenus[sys.currentDisarmedMenu].numberOfOptions - 1){
                sys.disarmedMenus[sys.currentDisarmedMenu].optionIndex++;
                sys.showSystemState();
                sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
            }

        if(sys.joystickAction == sys.joystickPress){
            sys.optionToFunctionMap();
            sys.showSystemState();
            sys.disarmedMenus[sys.currentDisarmedMenu].displayOptions();
        }
    }
}