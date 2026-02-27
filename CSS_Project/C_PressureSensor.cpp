#include "C_PressureSensor.h"
#include <iostream>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <cstdint>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cstring>

C_PressureSensor::C_PressureSensor() : i2c_fd(-1), pressure_Threshold(2.0f) { 
   i2c_fd = init_i2c_fd();
    if (i2c_fd < 0) {
        std::cerr << "Failed to initialize I2C for C_VibrationSensor_Door" << std::endl;
    }
}

C_PressureSensor::~C_PressureSensor() {
    if(i2c_fd >= 0) {
        close(i2c_fd);
        i2c_fd = -1;
    }
}

bool C_PressureSensor:: CheckForTrigger(double value_read){
    trigger=false;
    if(value_read>pressure_Threshold){
        trigger=true;
        std::cout << "Pressure Sensor Triggered" <<std::endl;
    }
        return trigger;
}

double C_PressureSensor:: getValue() {
    int adc_value_a0 = read_channel(ADC_CHANNEL_0);
    double voltage_a0 = (adc_value_a0 / 32768.0) * 4.096;
    return voltage_a0;
}

int C_PressureSensor :: init_i2c_fd(){
    const char *i2c_device = "/dev/i2c-1";

    if ((i2c_fd = open(i2c_device, O_RDWR)) < 0) {
        std::cerr << "Failed to open the I2C bus" << std::endl;
        return -1;
    }

    if (ioctl(i2c_fd, I2C_SLAVE, ADS1115_ADDRESS) < 0) {
        std::cerr << "Failed to set I2C address" << std::endl;
        close(i2c_fd);
        return -1;
    }
    return i2c_fd;
}

int C_PressureSensor :: read_channel(uint16_t mux_config) {
    uint16_t config = CONFIG_OS_SINGLE | mux_config | CONFIG_GAIN_4_096V |
                      CONFIG_MODE_SINGLE | CONFIG_DR_128SPS | CONFIG_COMP_QUE_DISABLE;

    uint8_t config_data[3] = {CONFIG_REGISTER, static_cast<uint8_t>(config >> 8), static_cast<uint8_t>(config & 0xFF)};
    if (write(i2c_fd, config_data, 3) != 3) {
        std::cerr << "Failed to write configuration to ADS1115" << std::endl;
        return -1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(8));

    uint8_t pointer_conversion = CONVERSION_REGISTER;
    if (write(i2c_fd, &pointer_conversion, 1) != 1) {
        std::cerr << "Failed to set pointer to conversion register" << std::endl;
        return -1;
    }
    uint8_t read_data[2];
    if (read(i2c_fd, read_data, 2) != 2) {
        perror("Read failed");
        std::cerr << "Failed to read conversion data" << std::endl;
        return -1;
    }

    int16_t raw_value = (read_data[0] << 8) | read_data[1];

    return raw_value;
}