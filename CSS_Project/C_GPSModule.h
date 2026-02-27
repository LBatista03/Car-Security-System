#ifndef C_GPSMODULE_H
#define C_GPSMODULE_H

#include "I_Location.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <sys/syslog.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h> 
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>
#include <stdexcept>
using namespace std;

#define ZED_F9R_I2C_ADDRESS 0x42

class C_GPSModule : public I_Location{
public:

    C_GPSModule(); 
    ~C_GPSModule();
    void StartTracking();
    void StopTracking();
    coordinates getLocation();

private:
    int i2cfile;
    const char *i2cdevice = "/dev/i2c-1";

    vector<uint8_t> readGPSData(size_t length);
    void parseData(const vector<uint8_t>& data, coordinates& coord);

};

#endif