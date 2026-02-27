#ifndef C_BUZZER_H
#define C_BUZZER_H

#include "I_Actuator.h"
#include <iostream>
#include <gpiod.h>
#include <mutex>
#include <unistd.h>
#include <chrono>
#include <thread>

#define GPIOD_NAME "/dev/gpiochip0"
#define BUZZER_PIN 17 

class C_Buzzer : public I_Actuator{
private:
    gpiod_chip* chip;
    gpiod_line* line;

public:
    C_Buzzer();
    ~C_Buzzer();

    void ActivateActuator(int frequency, int duration_ms);
    void DeactivateActuator();
};

#endif