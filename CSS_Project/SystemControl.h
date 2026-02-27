#ifndef SYSTEMCONTROL_H
#define SYSTEMCONTROL_H

#include "I_Sensor.h"
#include "I_Actuator.h"
#include "I_Location.h"
#include "I_Communication.h"
#include "I_Database.h"
#include <string>
#include <pthread.h>
#include <mqueue.h>
#include <iostream>
#include <vector>
#include <memory>
#include <atomic>
#include <math.h>
#include <mqueue.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>

extern pthread_t tReadSensorsID, tCheckSensorsID, tBuzzerID, tSMSCommunicationID, tGPSCommunicationID, tSMSID, tGPSID, tDataBaseID;

class SystemControl {
protected:
    I_Actuator*       actuator;
    I_Location*       locationModule;
    I_Communication*  communicationModule;
   std::unique_ptr<I_Database> database; 
    I_Sensor* pressure_sensor;
    I_Sensor* vibration_sensor_window;
    I_Sensor* ultrasonic_sensor;

    std::atomic<bool> terminate_read;
    std::atomic<bool> terminate_check;
    std::atomic<bool> gps_active;
    std::atomic<bool> buzzer_active;
    bool set_alarm=false;
    bool set_gps_on=false;

private:

    pthread_mutex_t mutexSensors;
    pthread_mutex_t mutexBuzzer;
    pthread_cond_t condBuzzer;
    void SetupMessageQueues();
    void SetupSignals();
    void SetupThread(int prio, pthread_attr_t* pthread_attr, struct sched_param* pthread_param);
    
    static SystemControl* instance;

public:

    SystemControl(I_Sensor* ps, I_Sensor* vsw, I_Sensor* us, I_Actuator* a, I_Location* loc, I_Communication* comm, const std::string& dbname);
    ~SystemControl();

    void initializeThreads();


    bool isTerminateCheck() { return terminate_check.load(); }
    I_Sensor* getPressureSensor() { return pressure_sensor; }
    I_Sensor* getVibrationSensor() { return vibration_sensor_window; }
    I_Sensor* getUltrasonicSensor() { return ultrasonic_sensor; }
    I_Actuator* getBuzzer() { return actuator; }

    void readSensorValues(float& pressure, float& vibration_window, float& distance);
    void checkSensorValues(float& pressure, float& vibration_window, float& distance);
    void StartSystem();
    void ShutDownSystem();
    static bool terminateThreads;

    void activate_sms_communication(void);
    void activate_gps_communication(void);

    static void signalrm_condReadSensors(int sig); 
    static void *tReadSensors(void* systemControl);
    static void *tCheckSensors(void *systemControl);
    static void *tBuzzer(void *systemControl);
    static void *tSMSCommunication(void *systemControl);
    static void *tGPSCommunication(void *systemControl);
    static void* tSMS(void *systemControl);
    static void* tGPS(void *systemControl);
    static void* tDataBase(void *systemControl);
};

#endif