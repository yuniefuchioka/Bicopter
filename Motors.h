
#ifndef MOTORS_H
#define MOTORS_H

#include "Arduino.h"
#include <Servo.h>

class Motors {
    public:
        Motors(int leftServoPin, int rightServoPin, int leftBLDCPin, int rightBLDCPin, int leftEncoderPin, int rightEncoderPin);
        void attachMotors();
        void writeMotors(int u1, int u2,  int u3, int u4); //inputs u1~u4 correspond to desiredLeftAngle, desiredRightAngle (both in absolute coordinates), desiredLeftSpeed, desiredRightSpeed
        int get_y1(); //returns reading of left encoder (in absolute coordinates)
        int get_y2(); //returns reading of right encoder (in absolute coordinates)

    private:
        int _leftServoPin;
        int _rightServoPin;
        int _leftBLDCPin;
        int _rightBLDCPin;
        int _leftEncoderPin;
        int _rightEncoderPin;
        Servo leftServo;
        Servo rightServo;
        Servo leftBLDC;
        Servo rightBLDC;
};

#endif
