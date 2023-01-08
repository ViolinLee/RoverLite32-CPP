#include <MecanumWheelRobot.h>

void MecanumWheelRobot::attachMotor(Motor *M_FL, Motor *M_FR, Motor *M_BL, Motor *M_BR) {
    Motor_FL = M_FL;
    Motor_FR = M_FR;
    Motor_BL = M_BL;
    Motor_BR = M_BR;
}

void MecanumWheelRobot::standBy() {
    Motor_FL->setPwmDuty(0);
    Motor_FR->setPwmDuty(0);
    Motor_BL->setPwmDuty(0);
    Motor_BR->setPwmDuty(0);
}

void MecanumWheelRobot::parseCommand(int8_t L_X, int8_t L_Y, int8_t R_X) {
    // Joystick range: -100 ~ 100
    float omega_Z = map(R_X, -100, 100, -maxTurnOmega, maxTurnOmega) * 0.95 / 1000.;  
    float vel_X = map(L_X, -100, 100, -vel_Max, vel_Max) * 0.95 / 1000.;  
    float vel_Y = map(L_Y, -100, 100, -vel_Max, vel_Max) * 0.95 / 1000.;
    // Serial.printf("omega_Z: %f vel_X: %f vel_Y: %f\n", omega_Z, vel_X, vel_Y);
    // Serial.printf("R_X: %d maxTurnOmega: %d omega_Z: %f\n", R_X, maxTurnOmega, omega_Z);

    KinematicCal(vel_X, vel_Y, omega_Z);
}

void MecanumWheelRobot::KinematicCal(float vel_X, float vel_Y, float omega_Z) {
    omega1 = (vel_X + vel_Y - omega_Z * (L1 + L2))/R;
    omega2 = (vel_X - vel_Y + omega_Z * (L1 + L2))/R;
    omega3 = (vel_X - vel_Y - omega_Z * (L1 + L2))/R;
    omega4 = (vel_X + vel_Y + omega_Z * (L1 + L2))/R;
    // Serial.printf("omega1: %f omega2: %f omega3: %f omega4: %f\n", omega1, omega2, omega3, omega4);
}

void MecanumWheelRobot::move() {
    float M1_speed=map(omega1, -maxWheelOmega, maxWheelOmega, -1000, 1000) / 1000.;
    float M2_speed=map(omega2, -maxWheelOmega, maxWheelOmega, -1000, 1000) / 1000.;
    float M3_speed=map(omega3, -maxWheelOmega, maxWheelOmega, -1000, 1000) / 1000.;
    float M4_speed=map(omega4, -maxWheelOmega, maxWheelOmega, -1000, 1000) / 1000.;
    // Serial.printf("M1_speed: %f M2_speed: %f M3_speed: %f M4_speed: %f\n", M1_speed, M2_speed, M3_speed, M4_speed);

    Motor_FL->setPwmDuty(M1_speed);
    Motor_FR->setPwmDuty(M2_speed);
    Motor_BL->setPwmDuty(M3_speed);
    Motor_BR->setPwmDuty(M4_speed);
}

MecanumWheelRobot::MecanumWheelRobot(float L1, float L2, float R): L1(L1), L2(L2), R(R)
{
    maxWheelOmega = 25;  // RPM=240 -> maxOmega=25rad/s
    maxTurnOmega = (int)(maxWheelOmega * R/(L1+L2) * 1000);  // -> 10.14 * 1000rad/s
    vel_Max = (int)(maxWheelOmega * R * 1000);  // -> 0.74 * 1000m/s
}

MecanumWheelRobot::~MecanumWheelRobot()
{
}