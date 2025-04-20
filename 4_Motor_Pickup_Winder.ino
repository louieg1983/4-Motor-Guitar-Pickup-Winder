#include <FastAccelStepper.h>

//-------------------- Pin Definitions --------------------
// Winder Motor Pins
#define WINDER_DIR_PIN      26    // Direction pin for Winder Motor
#define WINDER_STEP_PIN     27    // Step pulse pin for Winder Motor
#define WINDER_ENABLE_PIN   32    // Enable pin for Winder Motor (LOW enables)

// Traverse Motor Pins
#define TRAVERSE_DIR_PIN    13    // Direction pin for Traverse Motor
#define TRAVERSE_STEP_PIN   12    // Step pulse pin for Traverse Motor
#define TRAVERSE_ENABLE_PIN 33    // Enable pin for Traverse Motor (LOW enables)

// Hall Sensor Pin
#define HALL_SENSOR_PIN     34

// Additional Winder Motor Pins
#define WINDER2_DIR_PIN     25    // Direction pin for 2nd Winder Motor
#define WINDER2_STEP_PIN    14    // Step pulse pin for 2nd Winder Motor
#define WINDER2_ENABLE_PIN  23    // Enable pin for 2nd Winder Motor (LOW enables)

// Additional Traverse Motor Pins
#define TRAVERSE2_DIR_PIN   4     // Direction pin for Traverse Motor 2
#define TRAVERSE2_STEP_PIN  2     // Step pulse pin for Traverse Motor 2
#define TRAVERSE2_ENABLE_PIN 15   // Enable pin for Traverse Motor 2 (LOW enables)

//-------------------- Global Objects --------------------
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *winderStepper = NULL;
FastAccelStepper *winder2Stepper = NULL;
FastAccelStepper *traverseStepper = NULL;
FastAccelStepper *traverse2Stepper = NULL;

//-------------------- Global Variables --------------------
// Control flags
bool motorsRunning = false;  

// Winder Motor Variables
volatile long windCount = 0;      // Incremented via ISR on hall sensor
int desiredWindCount = 1000;      // Target wind count
long winderSpeed = 150000;        // Default winder speed (Hz)
long winderAcceleration = 20000;  // Winder acceleration
const int slowdownWinds = 111;    // Slowdown threshold

// Traverse Motor Variables
const int STEPS_PER_REVOLUTION = 3200;  // 3200 microsteps per revolution
int traverseStepDelay = 500;      // Delay per half-step (µs)
int leftSweepLimit = 0;           // Left sweep limit in steps
int rightSweepLimit = 6400;       // Right sweep limit in steps
bool traverseMovingRight = true;  // Traverse direction flag
int currentTraverseStep = 0;      // Current traverse position in steps
int currentTraverse2Step = 0;     // Current traverse2 position in steps

//-------------------- Status Timer Variables --------------------
unsigned long lastStatusPrintTime = 0;
const unsigned long statusPrintInterval = 1000; // 1 second

//-------------------- Function Prototypes --------------------
void processCommand(String command);
void updateWinderSpeed();
void moveTraverseToPosition(int targetSteps);
void moveTraverse2ToPosition(int targetSteps);
void startMotors();
void stopMotors();
void countWinds();
void pulseTraverseStep(int pin, int delay_us);
void controlBothTraverseMotors(int stepDelay, int leftLimit, int rightLimit);
void disableAllMotors();

//-------------------- Helper Function for Step Pulses --------------------
void pulseTraverseStep(int pin, int delay_us) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(10); // Ensure minimum pulse width
  digitalWrite(pin, LOW);
  delayMicroseconds(delay_us - 10 > 0 ? delay_us - 10 : 1); // Maintain timing
}

//-------------------- Control Both Traverse Motors --------------------
void controlBothTraverseMotors(int stepDelay, int leftLimit, int rightLimit) {
  if (traverseMovingRight) {
    // Both motors moving right (increasing position)
    if (currentTraverseStep >= rightLimit) {
      // Reached right limit, change direction for both motors
      traverseMovingRight = false;
      Serial.println("Traverse: Changing direction to LEFT");
    } else {
      // Continue moving right for both motors
      // Traverse 1
      digitalWrite(TRAVERSE_DIR_PIN, HIGH); // Direction for RIGHT movement
      pulseTraverseStep(TRAVERSE_STEP_PIN, stepDelay);
      currentTraverseStep++; // Update position
      
      // Traverse 2 - SAME direction
      digitalWrite(TRAVERSE2_DIR_PIN, HIGH); // Direction for RIGHT movement
      pulseTraverseStep(TRAVERSE2_STEP_PIN, stepDelay);
      currentTraverse2Step++; // Update position
    }
  } else {
    // Both motors moving left (decreasing position)
    if (currentTraverseStep <= leftLimit) {
      // Reached left limit, change direction for both motors
      traverseMovingRight = true;
      Serial.println("Traverse: Changing direction to RIGHT");
    } else {
      // Continue moving left for both motors
      // Traverse 1
      digitalWrite(TRAVERSE_DIR_PIN, LOW); // Direction for LEFT movement
      pulseTraverseStep(TRAVERSE_STEP_PIN, stepDelay);
      currentTraverseStep--; // Update position
      
      // Traverse 2 - SAME direction
      digitalWrite(TRAVERSE2_DIR_PIN, LOW); // Direction for LEFT movement
      pulseTraverseStep(TRAVERSE2_STEP_PIN, stepDelay);
      currentTraverse2Step--; // Update position
    }
  }
}

//-------------------- Process Serial Commands --------------------
void processCommand(String command) {
  if (command == "S") {
    startMotors();
  }
  else if (command == "T") {
    stopMotors();
  }
  else if (command == "R") {
    windCount = 0;
    Serial.println("Wind count reset to 0.");
    Serial.print("Current wind count: ");
    Serial.println(windCount);
  
    // Monitor the Hall sensor status
    Serial.print("Hall sensor current state: ");
    Serial.println(digitalRead(HALL_SENSOR_PIN) == HIGH ? "HIGH" : "LOW");
  }
  else if (command.startsWith("N")) {
    int newWindCount = command.substring(1).toInt();
    if (newWindCount > 0) {
      desiredWindCount = newWindCount;
      Serial.print("Desired wind count set to ");
      Serial.println(desiredWindCount);
    }
  }
  else if (command.startsWith("w_speed:")) {
    long newSpeed = command.substring(8).toInt();
    if (newSpeed > 0) {
      winderSpeed = newSpeed;
      if (winderStepper && motorsRunning) {
        winderStepper->setSpeedInHz(winderSpeed);
      }
      if (winder2Stepper && motorsRunning) {
        winder2Stepper->setSpeedInHz(winderSpeed);
      }
      Serial.print("Winder speed set to ");
      Serial.print(winderSpeed);
      Serial.println(" Hz");
    }
  }
  else if (command.startsWith("t_speed:")) {
    int newDelay = command.substring(8).toInt();
    if (newDelay > 0) {
      traverseStepDelay = newDelay;
      Serial.print("Traverse step delay set to ");
      Serial.print(traverseStepDelay);
      Serial.println(" microseconds");
    }
  }
  else if (command.startsWith("t_leftlimit:")) {
    int newLimit = command.substring(12).toInt();
    leftSweepLimit = newLimit;
    Serial.print("Left sweep limit set to ");
    Serial.println(leftSweepLimit);
  }
  else if (command.startsWith("t_rightlimit:")) {
    int newLimit = command.substring(13).toInt();
    if (newLimit >= 0 && newLimit <= 12800) {  // Allow up to 4 full revolutions
      rightSweepLimit = newLimit;
      Serial.print("Right sweep limit set to: ");
      Serial.println(rightSweepLimit);
    } else {
      Serial.println("Error: Right limit must be between 0 and 12800");
    }
  }
  else if (command == "t_home") {
    currentTraverseStep = 0;
    Serial.println("Traverse position reset to home (0)");
  }
  else if (command == "disable_all_motors") {
    disableAllMotors();
  }
  else {
    Serial.print("Unknown command: ");
    Serial.println(command);
  }
}

//-------------------- Start Motors Function --------------------
void startMotors() {
  if (!motorsRunning) {
    // Enable traverse motors first
    digitalWrite(TRAVERSE_ENABLE_PIN, LOW);
    digitalWrite(TRAVERSE2_ENABLE_PIN, LOW);
    
    // Initialize traverse direction flags
    traverseMovingRight = true;
    
    // Send a burst of steps to get the traverse motors moving
    Serial.println("Starting traverse motors first...");
    for (int i = 0; i < 50; i++) {
      digitalWrite(TRAVERSE_DIR_PIN, HIGH);
      digitalWrite(TRAVERSE2_DIR_PIN, HIGH);
      
      pulseTraverseStep(TRAVERSE_STEP_PIN, 300); 
      pulseTraverseStep(TRAVERSE2_STEP_PIN, 300);
      delay(5);
    }
    
    // Add a delay before starting winder motors
    delay(1000);
    
    Serial.println("Traverse motors running, now starting winder motors...");
    
    // Initialize the winder motors
    if (winderStepper) {
      winderStepper->stopMove();
      winderStepper->forceStopAndNewPosition(0);
    }
    
    if (winder2Stepper) {
      winder2Stepper->stopMove();
      winder2Stepper->forceStopAndNewPosition(0);
    }
    
    // Enable Winder Motors
    digitalWrite(WINDER_ENABLE_PIN, LOW);
    digitalWrite(WINDER2_ENABLE_PIN, LOW);
    
    // Configure and start the Winder Motors rotation
    if (winderStepper) {
      winderStepper->setSpeedInHz(winderSpeed);
      winderStepper->setAcceleration(winderAcceleration);
      winderStepper->runForward();
    }
    
    if (winder2Stepper) {
      winder2Stepper->setSpeedInHz(winderSpeed);
      winder2Stepper->setAcceleration(winderAcceleration);
      winder2Stepper->runForward();
    }
    
    // Set flag
    motorsRunning = true;
    
    Serial.println("All motors running");
  }
}

//-------------------- Disable All Motors Function --------------------
void disableAllMotors() {
  // First stop any stepper movement
  if (winderStepper) {
    winderStepper->stopMove();
    winderStepper->forceStopAndNewPosition(0);
    winderStepper->disableOutputs();
  }
  if (winder2Stepper) {
    winder2Stepper->stopMove();
    winder2Stepper->forceStopAndNewPosition(0);
    winder2Stepper->disableOutputs();
  }
  
  // Disable all motor outputs
  digitalWrite(WINDER_ENABLE_PIN, HIGH);
  digitalWrite(WINDER2_ENABLE_PIN, HIGH);
  digitalWrite(TRAVERSE_ENABLE_PIN, HIGH);
  digitalWrite(TRAVERSE2_ENABLE_PIN, HIGH);
  
  // Update state flag
  motorsRunning = false;
  
  Serial.println("All motors disabled.");
}

//-------------------- Stop Motors Function --------------------
void stopMotors() {
  if (motorsRunning) {
    // Stop the winder stepper motors
    if (winderStepper) {
      winderStepper->stopMove();
      winderStepper->forceStopAndNewPosition(0);
    }
    if (winder2Stepper) {
      winder2Stepper->stopMove();
      winder2Stepper->forceStopAndNewPosition(0);
    }
    
    // Disable all motors
    digitalWrite(WINDER_ENABLE_PIN, HIGH);
    digitalWrite(WINDER2_ENABLE_PIN, HIGH);
    digitalWrite(TRAVERSE_ENABLE_PIN, HIGH);
    digitalWrite(TRAVERSE2_ENABLE_PIN, HIGH);
    
    // Update motor running flag
    motorsRunning = false;
    
    Serial.println("All motors stopped and disabled.");
  }
}

//-------------------- Update Winder Speed --------------------
void updateWinderSpeed() {
  if (!motorsRunning) return;
  
  int remainingWinds = desiredWindCount - windCount;
  if (remainingWinds <= slowdownWinds) {
    float ratio = remainingWinds / (float)slowdownWinds;
    float slowdownFactor = (ratio > 0.1f) ? ratio : 0.1f;
    long adjustedSpeed = winderSpeed * slowdownFactor;
    if (winderStepper) {
      winderStepper->setSpeedInHz(adjustedSpeed);
    }
    if (winder2Stepper) {
      winder2Stepper->setSpeedInHz(adjustedSpeed);
    }
  }
}

//-------------------- Move Traverse Function (Motor 1) --------------------
void moveTraverseToPosition(int targetSteps) {
  // Force enable the traverse motor
  digitalWrite(TRAVERSE_ENABLE_PIN, LOW);
  
  if (targetSteps > currentTraverseStep) {
    while (currentTraverseStep < targetSteps) {
      digitalWrite(TRAVERSE_DIR_PIN, HIGH);
      pulseTraverseStep(TRAVERSE_STEP_PIN, traverseStepDelay);
      currentTraverseStep++;
    }
  }
  else if (targetSteps < currentTraverseStep) {
    while (currentTraverseStep > targetSteps) {
      digitalWrite(TRAVERSE_DIR_PIN, LOW);
      pulseTraverseStep(TRAVERSE_STEP_PIN, traverseStepDelay);
      currentTraverseStep--;
    }
  }
}

//-------------------- Move Traverse Function (Motor 2) --------------------
void moveTraverse2ToPosition(int targetSteps) {
  // Force enable the traverse motor
  digitalWrite(TRAVERSE2_ENABLE_PIN, LOW);
  
  if (targetSteps > currentTraverse2Step) {
    while (currentTraverse2Step < targetSteps) {
      digitalWrite(TRAVERSE2_DIR_PIN, HIGH);
      pulseTraverseStep(TRAVERSE2_STEP_PIN, traverseStepDelay);
      currentTraverse2Step++;
    }
  }
  else if (targetSteps < currentTraverse2Step) {
    while (currentTraverse2Step > targetSteps) {
      digitalWrite(TRAVERSE2_DIR_PIN, LOW);
      pulseTraverseStep(TRAVERSE2_STEP_PIN, traverseStepDelay);
      currentTraverse2Step--;
    }
  }
}

//-------------------- Wind Counting Function --------------------
void countWinds() {
  static int lastSensorState = HIGH;
  int sensorState = digitalRead(HALL_SENSOR_PIN);
  
  // Detect falling edge (magnet approaching sensor)
  if (lastSensorState == HIGH && sensorState == LOW) {
    windCount++;
    
    // Output wind count periodically for debugging
    if (windCount % 5 == 0 || windCount < 10) {
      Serial.print("Wind Count: ");
      Serial.println(windCount);
    }
  }
  
  lastSensorState = sensorState;
}

//-------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  
  // Initialize the FastAccelStepper engine
  engine.init();

  // Reset the wind count
  windCount = 0;
  
  // Configure Winder Motors
  pinMode(WINDER_DIR_PIN, OUTPUT);
  pinMode(WINDER_STEP_PIN, OUTPUT);
  pinMode(WINDER_ENABLE_PIN, OUTPUT);
  digitalWrite(WINDER_ENABLE_PIN, HIGH);  // Start disabled
  
  pinMode(WINDER2_DIR_PIN, OUTPUT);
  pinMode(WINDER2_STEP_PIN, OUTPUT);
  pinMode(WINDER2_ENABLE_PIN, OUTPUT);
  digitalWrite(WINDER2_ENABLE_PIN, HIGH);  // Start disabled
  
  // Configure Traverse Motors
  pinMode(TRAVERSE_DIR_PIN, OUTPUT);
  pinMode(TRAVERSE_STEP_PIN, OUTPUT);
  pinMode(TRAVERSE_ENABLE_PIN, OUTPUT);
  digitalWrite(TRAVERSE_ENABLE_PIN, HIGH); // Start disabled
  
  pinMode(TRAVERSE2_DIR_PIN, OUTPUT);
  pinMode(TRAVERSE2_STEP_PIN, OUTPUT);
  pinMode(TRAVERSE2_ENABLE_PIN, OUTPUT);
  digitalWrite(TRAVERSE2_ENABLE_PIN, HIGH); // Start disabled
  
  // Configure Hall Sensor
  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
  
  // Setup steppers
  winderStepper = engine.stepperConnectToPin(WINDER_STEP_PIN);
  if (winderStepper) {
    winderStepper->setDirectionPin(WINDER_DIR_PIN);
    winderStepper->setEnablePin(WINDER_ENABLE_PIN);
    winderStepper->setAutoEnable(false);
    winderStepper->setSpeedInHz(winderSpeed);
    winderStepper->setAcceleration(winderAcceleration);
  }
  
  winder2Stepper = engine.stepperConnectToPin(WINDER2_STEP_PIN);
  if (winder2Stepper) {
    winder2Stepper->setDirectionPin(WINDER2_DIR_PIN);
    winder2Stepper->setEnablePin(WINDER2_ENABLE_PIN);
    winder2Stepper->setAutoEnable(false);
    winder2Stepper->setSpeedInHz(winderSpeed);
    winder2Stepper->setAcceleration(winderAcceleration);
  }
  
  traverseStepper = engine.stepperConnectToPin(TRAVERSE_STEP_PIN);
  if (traverseStepper) {
    traverseStepper->setDirectionPin(TRAVERSE_DIR_PIN);
    traverseStepper->setEnablePin(TRAVERSE_ENABLE_PIN);
    traverseStepper->setAutoEnable(false);
    traverseStepper->setSpeedInHz(4000);
  }
  
  traverse2Stepper = engine.stepperConnectToPin(TRAVERSE2_STEP_PIN);
  if (traverse2Stepper) {
    traverse2Stepper->setDirectionPin(TRAVERSE2_DIR_PIN);
    traverse2Stepper->setEnablePin(TRAVERSE2_ENABLE_PIN);
    traverse2Stepper->setAutoEnable(false);
    traverse2Stepper->setSpeedInHz(4000);
  }
  
  delay(1000);
  Serial.println("System initialized");
  Serial.println("Available commands:");
  Serial.println("  S             -> Start motors");
  Serial.println("  T             -> Stop motors");
  Serial.println("  R             -> Reset wind count");
  Serial.println("  N<number>     -> Set desired wind count");
  Serial.println("  w_speed:<Hz>  -> Set winder speed in Hz");
  Serial.println("  t_speed:<us>  -> Set traverse step delay (µs)");
  Serial.println("  t_leftlimit:X -> Set left sweep limit (steps)");
  Serial.println("  t_rightlimit:X -> Set right sweep limit (steps)");
  Serial.println("  t_home        -> Reset traverse position to 0 (home)");
  Serial.println("  disable_all_motors -> Disable all motors");
}

//-------------------- Main Loop --------------------
void loop() {
  // Process incoming serial commands
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }
  
  // This section runs when motors are actively running
  if (motorsRunning) {
    // Update winder motor speed if needed
    updateWinderSpeed();
    
    // Process only a few steps per loop iteration
    for (int i = 0; i < 5; i++) {
      // Enable traverse motors
      digitalWrite(TRAVERSE_ENABLE_PIN, LOW);
      digitalWrite(TRAVERSE2_ENABLE_PIN, LOW);
      
      // Use the synchronized control function for both traverse motors
      controlBothTraverseMotors(traverseStepDelay, leftSweepLimit, rightSweepLimit);
      
      // Short delay to prevent overwhelming the CPU
      delayMicroseconds(100);
    }
    
    // Count winds via hall sensor
    countWinds();
    
    // Check if we've reached the desired wind count
    if (desiredWindCount > 0 && (windCount + windCountOffset) >= desiredWindCount) {
      stopMotors();
      Serial.print("Target wind count reached: ");
      Serial.println(windCount + windCountOffset);
    }
  }
  
  // Print status every second
  if (millis() - lastStatusPrintTime >= statusPrintInterval) {
    Serial.print("Current Wind Count: ");
    Serial.println(windCount);
    lastStatusPrintTime = millis();
  }
}
