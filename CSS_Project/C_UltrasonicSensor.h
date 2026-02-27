#ifndef C_ULTRASONIC_SENSOR_H
#define C_ULTRASONIC_SENSOR_H

#include <vector>
#include <memory>

#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cstring>

#include "I_Sensor.h"

#define RD_VALUE _IOR('a','a',int32_t*)
#define ULTRASONIC_THRESHOLD 5

class C_UltrasonicSensor : public I_Sensor{
protected:
    int ddriver_fd;
    int init_ddriver_fd();

private:
    int32_t distance;
    int32_t ultrasonic_Threshold;
    bool trigger;

public:
    C_UltrasonicSensor();
    ~C_UltrasonicSensor();

    bool CheckForTrigger(double value_read) override;
    double getValue() override;
};

#endif