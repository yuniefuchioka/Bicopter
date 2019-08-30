
#include <avr/wdt.h> //watchdog
#include "Constants.h"
#include "Motors.h"
#include "IMU.h" //requires I2Cdev https://github.com/jrowberg/i2cdevlib
#include "RC.h" //requires FlySkyIBus https://gitlab.com/timwilkinson/FlySkyIBus

Motors motors(LEFT_SERVO_PIN, RIGHT_SERVO_PIN, LEFT_BLDC_PIN, RIGHT_BLDC_PIN, LEFT_ENCODER_PIN, RIGHT_ENCODER_PIN);
IMU imu(SDA_PIN, SCL_PIN, INTERRUPT_PIN);
RC rc;

//Interrupt service routine (ISR) triggered by IMU
volatile bool imuInterrupt = false; // indicates whether MPU interrupt pin has gone high
void imuInterruptISR() {
    imuInterrupt = true;
}

void setup() {
    Serial.begin(115200);
    pinMode(INTERRUPT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), imuInterruptISR, RISING);
    pinMode(LED_PIN, OUTPUT);

    motors.attachMotors();
    rc.begin(Serial);
    if (!imu.initialize()) {
        wdt_enable(WATCHDOG_TIME); //enable the watchdog to force the software to reset in the infinite loop
        wdt_reset();
        while (true) { Serial.println("dmp initialization failure"); }
    }
    
    digitalWrite(LED_PIN, HIGH);
    wdt_enable(WATCHDOG_TIME); //initialize watchdog. This needs to happen at the end since imu initialization takes a while
}

void loop() {
    wdt_reset(); //reset watchdog
    rc.loop();
    if (imuInterrupt) {
        imuInterrupt = false;
        imu.update();
    }

    int yawApplied = 0;
    int pitchApplied = 0;
    int rollApplied = 0;

    if (rc.getSWD()) { //if switch D is activated, apply feedback control
        //target orientation
        static float yawDes = 0;
        yawDes += rc.getReadyState() ? rc.getLeftHor()*0.001 - 0.5  : 0;
        int pitchDes = map(rc.getRightVer(), SPEED_MIN, SPEED_MAX, 15, -15);
        int rollDes =  map(rc.getRightHor(), SPEED_MIN, SPEED_MAX, -15, 15);
        int yawDotDes = 0;
        int pitchDotDes = 0;
        int rollDotDes = 0;

        //IMU readings
        float yaw = imu.read().yaw;
        float pitch = imu.read().pitch;
        float roll = imu.read().roll;
        int yawDot = imu.read().yawDot;
        int pitchDot = imu.read().pitchDot;
        int rollDot = imu.read().rollDot;

        //PD feedback control parameters
        static float kpYaw = 0.85; //0; //12 oclock
        static float kdYaw = 0.2; //2.5 dots
        static float kpPitch = 0.85; //12 oclock
        static float kdPitch = 0.2; //0; //2.5 dots
        static float kpRoll = 1.88;
        static float kdRoll = 1.88;

        if (rc.getSWB()) { //gain setting mode
            if (rc.getSWC() == 0) { //set k yaw values
                kpYaw = rc.getVRA()*0.005;
                kdYaw = rc.getVRB()*0.005;
            }
            else if (rc.getSWC() == 1) { //set k pitch values
                kpPitch = rc.getVRA()*0.005;
                kdPitch = rc.getVRB()*0.005;
            }
            else if (rc.getSWC() == 2) { //set k roll values
                kpRoll = rc.getVRA()*0.005;
                kdRoll = rc.getVRB()*0.005;
            }
        }

        /*
        Serial.print(kpYaw);
        Serial.print(' ');
        Serial.print(kdYaw);
        Serial.print(' ');
        Serial.print(kpPitch);
        Serial.print(' ');
        Serial.print(kdPitch);
        Serial.print(' ');
        Serial.print(kpRoll);
        Serial.print(' ');
        Serial.print(kdRoll);
        Serial.print('\n');
        */

        //PD feedback control
        yawApplied = (int) (kpYaw*(yawDes-yaw) + kdYaw*(yawDotDes-yawDot));
        pitchApplied = (int) (kpPitch*(pitchDes-pitch) + kdPitch*(pitchDotDes-pitchDot));
        rollApplied = (int) (kpRoll*(rollDes-roll) + kdRoll*(rollDotDes-rollDot));
    }
    else { //if switch D is not activated, motors are operated manually
        yawApplied = map(rc.getLeftHor(), SPEED_MIN, SPEED_MAX, -45, 45);
        pitchApplied = map(rc.getRightVer(), SPEED_MIN, SPEED_MAX, 45, -45);
        rollApplied = map(rc.getRightHor(), SPEED_MIN, SPEED_MAX, -300, 300);
    }

    int BLDCSpeed = map(rc.getLeftVer(), SPEED_MIN, SPEED_MAX, SPEED_MIN, SPEED_MAX*0.75);

    int u1 = pitchApplied - yawApplied;
    int u2 = pitchApplied + yawApplied;
    int u3 = rc.getSWA() ? BLDCSpeed + rollApplied : 0; //if switch A is turned off, set BLDC speed to 0
    int u4 = rc.getSWA() ? BLDCSpeed - rollApplied : 0;

    if (rc.getReadyState()) { //if the RC receiver has started receiving data
        motors.writeMotors(u1, u2, u3, u4);
    }
}
