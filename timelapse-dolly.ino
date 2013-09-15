#include <SoftwareSerial.h>

// Attach the serial display's RX line to digital pin 2
SoftwareSerial mySerial(3,2); // pin 2 = TX, pin 3 = RX (unused)

#define MAX_STEPS 100   // Number MAX motor steps per cycle
#define MIN_STEPS 1     // Number MIN motor steps per cycle
#define MAX_INTERVAL 30 // Maximum Interval time (s)
#define MIN_INTERVAL 0  // Minimum Interval time (s)
#define MAX_EXP_TIME 30 // Maximum Exposure time (s)
#define MIN_EXP_TIME 0  // Mininum Exposure time (s)

#define INITIAL_DELAY    1000
#define STABILIZE_DELAY  100

enum ModeScreen {
    M_STEPS    = 0, // Number of motor steps per cycle
    M_INTERVAL = 1, // Interval between pictures
    M_EXP_TIME = 2, // Set the exposure time
    M_TL_START = 3  // Start/Stop the Time-lapse
};

// Stepper Motor pins
const int dirPin  = 6;
const int stepPin = 7;

// UI pins
const int modeBtnPin  = 8;
const int plusBtnPin  = 9;
const int minusBtnPin = 10;

int modeBtnState  = 0;
int plusBtnState  = 0;
int minusBtnState = 0;

// Initialize the UI to the first Mode state
ModeScreen mScreen = M_STEPS;

// Time-lapse params
int  pSteps    = MIN_STEPS;    // Number of motor steps per cycle
int  pInterval = MIN_INTERVAL; // Interval between pictures
int  pExpTime  = MIN_EXP_TIME; // Exposure Time
bool pTLState  = false;        // Time-lapse state initialized to off

bool firstPicture = true; // If first picture of TL do special things

// -----------------------------------------------------------------------------
// LCD Screen Cycle
// -----------------------------------------------------------------------------
void nextScreen() {
    // Get next UI screen
    switch (mScreen) {
        case M_STEPS:    mScreen = M_INTERVAL; break;
        case M_INTERVAL: mScreen = M_EXP_TIME; break;
        case M_EXP_TIME: mScreen = M_TL_START; break;
        case M_TL_START: mScreen = M_STEPS;    break;
    }
}

// -----------------------------------------------------------------------------
// Change the value of LCD screens
// -----------------------------------------------------------------------------

void manageScreen(int valueChange) {
    switch (mScreen) {
        case M_STEPS:    mStepsChange(valueChange);    break;
        case M_INTERVAL: mIntervalChange(valueChange); break;
        case M_EXP_TIME: mExpTimeChange(valueChange);  break;
        case M_TL_START: mStartChange();    break;
    }
}

void mStepsChange(int valueChange) {
    int temp = pSteps + valueChange;

    if (temp >= MIN_STEPS && temp <= MAX_STEPS) {
        pSteps = temp;
    }
}

void mIntervalChange(int valueChange) {
    int temp = pInterval + valueChange;

    if (temp >= MIN_INTERVAL && temp <= MAX_INTERVAL) {
        pInterval = temp;
    }
}

void mExpTimeChange(int valueChange) {
    int temp = pExpTime + valueChange;

    if (temp >= MIN_EXP_TIME && temp <= MAX_EXP_TIME) {
        pExpTime = temp;
    }
}

void mStartChange() {
    pTLState = !pTLState;

    if (pTLState) {
        firstPicture = true;
    }
}

// -----------------------------------------------------------------------------
// Render screens
// -----------------------------------------------------------------------------

void clearLCD() {
    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("                "); // clear display
    mySerial.write("                ");
}

void render() {
    clearLCD();

    switch (mScreen) {
        case M_STEPS:    renderStepsScreen();    break;
        case M_INTERVAL: renderIntervalScreen(); break;
        case M_EXP_TIME: renderExpTimeScren();   break;
        case M_TL_START: renderStartScren();     break;
    }
}

void renderStepsScreen() {
    char tempstring[10];
    sprintf(tempstring,"%4d",pSteps);

    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("Number of Steps:");

    mySerial.write(254); // move cursor to beginning of second line
    mySerial.write(192);
    mySerial.write(tempstring);
}

void renderIntervalScreen() {
    char tempstring[10];
    sprintf(tempstring,"%4d",pSteps);

    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("Interval:       ");

    mySerial.write(254); // move cursor to beginning of second line
    mySerial.write(192);
    mySerial.write(tempstring);
}

void renderExpTimeScren() {
    char tempstring[10];
    sprintf(tempstring,"%4d",pExpTime);

    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("Exposure Time:  ");

    mySerial.write(254); // move cursor to beginning of second line
    mySerial.write(192);
    mySerial.write(tempstring);
}

void renderStartScren() {
    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("Start/Stop TL:  ");

    mySerial.write(254); // move cursor to beginning of second line
    mySerial.write(192);
    if (pTLState) {
        mySerial.write("On");
    } else {
        mySerial.write("Off");
    }
}

// -----------------------------------------------------------------------------
// Motor Cycle
// -----------------------------------------------------------------------------

void doCycle() {
    // Trigger photo
    Serial.println("SNAP!");

    // exp time
    delay(pExpTime*1000);

    // interval
    delay(pInterval*1000);

    // move
    Serial.println("Start stepper-motor");
    delay(200);
    Serial.println("Stop stepper-motor");

    // Stabilize camera delay
    Serial.println("delay STABILIZE_DELAY");
    delay(STABILIZE_DELAY);
}

// -----------------------------------------------------------------------------
// Setup. Run once on start
// -----------------------------------------------------------------------------
void setup() {
    // Initialize LCD
    mySerial.begin(9600); // set up serial port for 9600 baud
    delay(500); // wait for display to boot up

    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("                "); // clear display
    mySerial.write("                ");

    // Declare Motor pins
    pinMode(dirPin,  OUTPUT);
    pinMode(stepPin, OUTPUT);

    // Declare UI pins
    pinMode(modeBtnPin,  INPUT);
    pinMode(plusBtnPin,  INPUT);
    pinMode(minusBtnPin, INPUT);

    // Start Serial for debugging
    Serial.begin(9600);
}

// -----------------------------------------------------------------------------
// Loop. Run continuosly
// -----------------------------------------------------------------------------
void loop() {
    // read buttons
    int newModeBtnState  = digitalRead(modeBtnPin);
    int newPlusBtnState  = digitalRead(plusBtnPin);
    int newMinusBtnState = digitalRead(minusBtnPin);

    bool modeBtnChange  = newModeBtnState  && !modeBtnState;
    bool plusBtnChange  = newPlusBtnState  && !plusBtnState;
    bool minusBtnChange = newMinusBtnState && !minusBtnState;

    int valueChange = 0;
    if (plusBtnChange) {
        valueChange = 1;
    } else if (minusBtnChange) {
        valueChange = -1;
    }

    if (modeBtnChange) {
        nextScreen();
    } else if (plusBtnChange || minusBtnChange) {
        manageScreen(valueChange);
    }

    modeBtnState  = newModeBtnState;
    plusBtnState  = newPlusBtnState;
    minusBtnState = newMinusBtnState;

    if (modeBtnChange || plusBtnChange || minusBtnChange) {
        render();
    }

    if (pTLState) {
        if (firstPicture) {
            firstPicture = false;
            delay(INITIAL_DELAY);
        }
        doCycle();
    }

    delay(1);
}
