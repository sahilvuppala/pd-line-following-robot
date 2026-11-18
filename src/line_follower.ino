#include <QTRSensors.h> // Include Pololu QTR reflectance sensor array library
QTRSensors qtr;         // Create a QTRSensors object

const uint8_t SensorCount = 5;      // Number of IR sensors on the array
uint16_t sensorValues[SensorCount]; // Array to store raw sensor readings

// PID and Motor Speed Constants
#define Kp 2          // Proportional gain (reacts to current line position error)
#define Kd 40         // Derivative gain (reacts to rate of change of error to reduce oscillation)
#define MaxSpeed 225  // Maximum PWM speed limit for the motors (0-255)
#define BaseSpeed 155 // Default cruising PWM speed when centred on the line
#define speedturn 100 // PWM speed used for sharp pivot turns and calibration

// Motor Driver (TB6612FNG / L298N) Pin Definitions
//int STBY = 10; 
//Motor A (Left Motor)
int PWMA = 5;  // PWM pin for Motor A speed control
int AIN1 = 11; // Direction control pin 1 for Motor A
int AIN2 = 12; // Direction control pin 2 for Motor A
//Motor B (Right Motor)
int PWMB = 3;  // PWM pin for Motor B speed control
int BIN1 = 9;  // Direction control pin 1 for Motor B
int BIN2 = 8;  // Direction control pin 2 for Motor B

int lastError=0; // Stores the previous loop's error for calculating the derivative (D) term

void setup()
{   
    // Configure the QTR sensor array as analog inputs on pins A0 through A4
    qtr.setTypeAnalog();
    qtr.setSensorPins((const uint8_t[]){A0, A1, A2, A3, A4}, SensorCount);
    //qtr.setEmitterPin(2);pinMode(STBY, OUTPUT);

    // Set all motor control pins as outputs
    pinMode(PWMA, OUTPUT);pinMode(AIN1, OUTPUT);pinMode(AIN2, OUTPUT);
    pinMode(PWMB, OUTPUT);pinMode(BIN1, OUTPUT);pinMode(BIN2, OUTPUT);

    // Turn on the built-in LED to indicate calibration is starting soon
    delay(500);pinMode(LED_BUILTIN, OUTPUT);digitalWrite(LED_BUILTIN, HIGH);
    delay(3000); // 3-second pause before robot starts spinning to calibrate

    // Auto-calibration routine: sweeps the sensors back and forth over the line
    int i;
    for (int i = 0; i < 100; i++)
    {   
        // Spin in one direction for the first and last quarter, and opposite in the middle
        if ( i  < 25 || i >= 75 ) 
        {   move(0,speedturn, 1); // Left motor forward
            move(1,speedturn, 0); // Right motor backward
        }
        else
        {   move(0,speedturn, 0); // Left motor backward
            move(1,speedturn, 1); // Right motor forward
        }
        qtr.calibrate();delay(20); // Record min/max reflectance values and wait 20ms
    }
    delay(3000); // 3-second pause after calibration before entering the main line-following loop
}  

void loop()
{   
    // Read the black line position (returns a value between 0 and 4000 for 5 sensors)
    uint16_t position = qtr.readLineBlack(sensorValues);

    // Sharp right turn handling: if line is detected far to the right (> 3700), pivot right
    if(position>3700)
    {   move(1, speedturn, 1);move(0, speedturn, 0);return;    
    }
    // Sharp left turn handling: if line is detected far to the left (< 300), pivot left
    if(position<300)
    {   move(1, speedturn, 0);move(0, speedturn, 1);return;
    }

    // Calculate PD (Proportional-Derivative) correction
    int error = position - 2000; // 2000 is the exact centre for a 5-sensor array (0 to 4000)
    int motorSpeed = Kp * error + Kd * (error - lastError); // PD formula
    lastError = error; // Save current error for the next iteration

    // Adjust left and right motor speeds based on the PD correction
    int rightMotorSpeed = BaseSpeed + motorSpeed;
    int leftMotorSpeed = BaseSpeed - motorSpeed;
  
    // Constrain motor speeds within valid PWM limits (0 to MaxSpeed)
    if (rightMotorSpeed > MaxSpeed ) rightMotorSpeed = MaxSpeed; 
    if (leftMotorSpeed > MaxSpeed ) leftMotorSpeed = MaxSpeed;
    if (rightMotorSpeed < 0)rightMotorSpeed = 0;    
    if (leftMotorSpeed < 0)leftMotorSpeed = 0;
    
    // Drive both motors forward at their calculated speeds
    move(1, rightMotorSpeed, 1);
    move(0, leftMotorSpeed, 1);
}

// Helper function to control individual motor speed and direction
// motor: 0 = Motor A (Left), 1 = Motor B (Right)
// speed: PWM value (0-255)
// direction: 1 = Forward, 0 = Reverse
void move(int motor, int speed, int direction)
{   
    boolean inPin1=HIGH,inPin2=LOW;
    // Set H-bridge logic levels based on desired direction
    if(direction == 1){inPin1 = HIGH;inPin2 = LOW;}  
    if(direction == 0){inPin1 = LOW; inPin2 = HIGH;}

    // Apply direction and PWM speed to Motor A
    if(motor == 0)
    {   digitalWrite(AIN1, inPin1);digitalWrite(AIN2, inPin2);analogWrite(PWMA, speed);
    }
    // Apply direction and PWM speed to Motor B
    if(motor == 1)
    {   digitalWrite(BIN1, inPin1);digitalWrite(BIN2, inPin2);analogWrite(PWMB, speed);
    }  
}
