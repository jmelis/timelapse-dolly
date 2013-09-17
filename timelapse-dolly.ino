#include <SoftwareSerial.h>

// Attach the serial display's RX line to digital pin 2
SoftwareSerial mySerial(3,2); // pin 2 = TX, pin 3 = RX (unused)

#define MAX_STEPS 100   // Number MAX motor steps per cycle
#define MIN_STEPS 1     // Number MIN motor steps per cycle
#define MAX_INTERVAL 30 // Maximum Interval time (s)
#define MIN_INTERVAL 0  // Minimum Interval time (s)
#define MAX_EXP_TIME 30 // Maximum Exposure time (s)
#define MIN_EXP_TIME 0  // Mininum Exposure time (s)

#define WAIT_INITIAL     1000
#define STABILIZE_DELAY  100

enum ModeScreen {
    M_EXP_TIME, // Set the exposure time
    M_INTERVAL, // Interval between pictures
    M_STEPS ,   // Number of motor steps per cycle
    M_TL_START  // Start/Stop the Time-lapse
};

enum CycleState {
    OFF,            // Time-lapse is off
    WAIT_START,     // Before starting wait for WAIT_INITIAL
    TRIGGER_PHOTO,  // Trigger the photo
    WAIT_EXP_TIME,  // Wait for exposure time
    WAIT_INTERVAL,  // Wait for interval time
    MOVE_MOTOR,     // Move the motor
    WAIT_STABILIZE  // Stabilize during STABILIZE_DELAY
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
ModeScreen mScreen = M_EXP_TIME;

// Cycle state
CycleState cycleState = OFF;

// The Parameters data structure holds the Time-lapse parameters
typedef struct {
    int expTime;    // Exposure Time
    int interval;   // Interval between pictures
    int steps;      // Number of motor steps per cycle
} Parameters;

Parameters params  = {MIN_EXP_TIME, MIN_INTERVAL, MIN_STEPS};

// The Timer data structure keeps track of a running timer
typedef struct {
    bool status;
    unsigned long endTime;
} Timer;

Timer timer = { false, 0 };

// if -1 then not moving motor, otherwise the number of remaining steps
int pendingSteps = -1;

// -----------------------------------------------------------------------------
// Timer functions
// -----------------------------------------------------------------------------

void resetTimer() {
    Serial.println("End timer at %lu",millis());
    timer.status  = false;
    timer.endTime = 0;
}

void startTimer(int delay) {
    Serial.println("Starting timer of %i ms at %lu",delay,millis());
    timer.status  = true;
    timer.endTime = millis() + delay;
}

bool checkTimer() {
    return millis() >= timer.endTime;
}

void handleTimer(int delay) {
    if (!timer.status) {
        startTimer(delay);
    } else {
        if (checkTimer()) {
            resetTimer();
            cycleState = state;
        }
    }
}

// -----------------------------------------------------------------------------
// LCD Screen Cycle
// -----------------------------------------------------------------------------

void nextScreen() {
    // Get next UI screen
    switch (mScreen) {
        case M_EXP_TIME: mScreen = M_INTERVAL; break;
        case M_INTERVAL: mScreen = M_STEPS;    break;
        case M_STEPS:    mScreen = M_TL_START; break;
        case M_TL_START: mScreen = M_EXP_TIME; break;
    }
}

// -----------------------------------------------------------------------------
// Change the value of LCD screens
// -----------------------------------------------------------------------------

void manageScreen(int valueChange) {
    switch (mScreen) {
        case M_EXP_TIME: mExpTimeChange(valueChange);  break;
        case M_INTERVAL: mIntervalChange(valueChange); break;
        case M_STEPS:    mStepsChange(valueChange);    break;
        case M_TL_START: mStartChange();               break;
    }
}

void mExpTimeChange(int valueChange) {
    int temp = params.expTime + valueChange;

    if (temp >= MIN_EXP_TIME && temp <= MAX_EXP_TIME) {
        params.expTime = temp;
    }
}

void mIntervalChange(int valueChange) {
    int temp = params.interval + valueChange;

    if (temp >= MIN_INTERVAL && temp <= MAX_INTERVAL) {
        params.interval = temp;
    }
}

void mStepsChange(int valueChange) {
    int temp = params.steps + valueChange;

    if (temp >= MIN_STEPS && temp <= MAX_STEPS) {
        params.steps = temp;
    }
}

void mStartChange() {
    if (cycleState == OFF) {
        cycleState = WAIT_START;
    } else {
        cycleState = OFF;
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
        case M_EXP_TIME: renderExpTimeScren();   break;
        case M_INTERVAL: renderIntervalScreen(); break;
        case M_STEPS:    renderStepsScreen();    break;
        case M_TL_START: renderStartScren();     break;
    }
}

void renderExpTimeScren() {
    char tempstring[16];
    sprintf(tempstring,"%16d",params.expTime);

    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("> Exposure Time");

    mySerial.write(254); // move cursor to beginning of second line
    mySerial.write(192);
    mySerial.write(tempstring);
}

void renderIntervalScreen() {
    char tempstring[16];
    sprintf(tempstring,"%16d",params.interval);

    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("> Interval");

    mySerial.write(254); // move cursor to beginning of second line
    mySerial.write(192);
    mySerial.write(tempstring);
}

void renderStepsScreen() {
    char tempstring[16];
    sprintf(tempstring,"%16d",params.steps);

    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("> Number of Steps");

    mySerial.write(254); // move cursor to beginning of second line
    mySerial.write(192);
    mySerial.write(tempstring);
}

void renderStartScren() {
    mySerial.write(254); // move cursor to beginning of first line
    mySerial.write(128);
    mySerial.write("> Start/Stop TL");

    mySerial.write(254); // move cursor to beginning of second line
    mySerial.write(192);

    if (cycleState != OFF) {
        mySerial.write("On");
    } else {
        mySerial.write("Off");
    }
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

    switch(cycleState) {
        case WAIT_START:
            handleTimer(WAIT_INITIAL, TRIGGER_PHOTO);
            break;

        case TRIGGER_PHOTO:
            Serial.println("SNAP!");
            cycleState = WAIT_EXP_TIME;
            break;

        case WAIT_EXP_TIME:
            handleTimer(params.expTime, WAIT_INTERVAL);
            break;

        case WAIT_INTERVAL:
            handleTimer(params.expTime, MOVE_MOTOR);
            break;

        case MOVE_MOTOR:
            if (pendingSteps == -1) {
                pendingSteps = params.steps;
            }

            if (pendingSteps > 0) {
                Serial.println("Motor step %i", pendingSteps);
                pendingSteps--;
            } else {
                pendingSteps = -1;
                cycleState = WAIT_STABILIZE;
            }
            break;

        case WAIT_STABILIZE;
            handleTimer(WAIT_STABILIZE, TRIGGER_PHOTO);
            break;
    }

    delay(1);
}
