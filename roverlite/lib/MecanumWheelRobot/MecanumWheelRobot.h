#ifndef MECANUMWHEELROBOT_H_
#define MECANUMWHEELROBOT_H_

#include <motor.h>
#include <Arduino.h>

class MecanumWheelRobot {
public:
    MecanumWheelRobot(float L1, float L2, float R);
    ~MecanumWheelRobot();
    void attachMotor(Motor *M_FL, Motor *M_FR, Motor *M_BL, Motor *M_BR);
    void standBy();
    void parseCommand(int8_t L_X, int8_t L_Y, int8_t R_X);
    void KinematicCal(float vel_X, float vel_Y, float omega_Z);
    void move();
public:
    Motor *Motor_FL, *Motor_FR, *Motor_BL, *Motor_BR;
    float omega1, omega2, omega3, omega4;
    float L1, L2, R;
    int vel_Max, maxWheelOmega, maxTurnOmega;
};

#endif