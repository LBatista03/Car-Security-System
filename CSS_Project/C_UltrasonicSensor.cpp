#include "C_UltrasonicSensor.h"
#include <iostream>

C_UltrasonicSensor :: C_UltrasonicSensor() : ddriver_fd(-1), ultrasonic_Threshold(5) { 
    ddriver_fd = init_ddriver_fd();
    if (ddriver_fd < 0) {
        std::cerr << "Failed to initialize file for C_UltrasonicSensor" << std::endl;
    }
}

C_UltrasonicSensor :: ~C_UltrasonicSensor() {
    if (ddriver_fd >= 0) {
        close(ddriver_fd);
        ddriver_fd=-1;
    }
}

bool C_UltrasonicSensor:: CheckForTrigger(double value_read){
    trigger=false;
    if(value_read<ULTRASONIC_THRESHOLD){
        trigger=true;
        std::cout << "Ultrasonic Sensor Triggered" <<std::endl;
    }
    return trigger;
}

double C_UltrasonicSensor:: getValue(){
    ioctl(ddriver_fd, RD_VALUE, (int32_t*) &distance);
    return static_cast<double>(distance);
}

int C_UltrasonicSensor :: init_ddriver_fd(){
    if((ddriver_fd = open("/dev/sensor_hc-sr04", O_RDWR))< 0) {
        std::cerr << "Failed to open Device Driver File" << std::endl;
        return -1;
    }
    return ddriver_fd;
}