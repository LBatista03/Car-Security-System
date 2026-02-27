#ifndef C_VIBRATION_SENSOR_WINDOW_H
#define C_VIBRATION_SENSOR_WINDOW_H

#include <vector>
#include <memory>

#include <iostream>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <cstdint>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cstring>

#include "I_Sensor.h"

#define ADS1115_ADDRESS 0x48
#define CONFIG_REGISTER 0x01
#define CONVERSION_REGISTER 0x00

#define CONFIG_OS_SINGLE (1 << 15)
#define ADC_CHANNEL_0 (4 << 12)  // Channel A0
#define ADC_CHANNEL_1 (5 << 12)  // Channel A1
#define ADC_CHANNEL_2 (6 << 12)  // Channel A2
#define ADC_CHANNEL_3 (7 << 12)  // Channel A3
#define CONFIG_GAIN_4_096V (1 << 9)    // +/-4.096V
#define CONFIG_MODE_SINGLE (1 << 8)    // Single-shot mode
#define CONFIG_DR_128SPS (4 << 5)      // 128 samples per second
#define CONFIG_COMP_QUE_DISABLE (3)

class C_VibrationSensor_Window : public I_Sensor{
protected: 
    int i2c_fd;
    int read_channel(uint16_t mux_config); 
    int init_i2c_fd();

private:
    float value;
    float vibration_window_Threshold;
    bool trigger;

public:

    C_VibrationSensor_Window();
    ~C_VibrationSensor_Window();

    bool CheckForTrigger(double value_read) override;
    double getValue() override;
};

#endif