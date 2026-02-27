#ifndef C_SMSMODULE_H
#define C_SMSMODULE_H


#include <iostream>
#include <string>
#include <mqueue.h>
#include <cstring>
#include <cstdlib>
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
#include <fstream>
#include <termios.h>
#include "I_Communication.h"

#define SERIAL_PORT "/dev/ttyAMA0" 

class C_SMSModule : public I_Communication{

protected:
    int baudRate = B115200;
    int serialPort;
    int setupSerialPort(const char* port, int baudRate);
    void sendATCommand(int serialPort, const std::string& command);
    std::string readResponse(int serialPort);

public:
    C_SMSModule();
    ~C_SMSModule();

    void ReceiveMessage();
    void SendMessage(std::string message);
};

#endif
