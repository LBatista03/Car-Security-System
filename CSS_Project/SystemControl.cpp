#include "SystemControl.h"
#include "C_Database.h"
#include <sched.h>
#include <errno.h>

using namespace std;

pthread_t tReadSensorsID, tCheckSensorsID, tBuzzerID, tSMSCommunicationID, tGPSCommunicationID, tSMSID, tGPSID, tDataBaseID;
pthread_mutex_t mutexSensors = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBuzzer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condBuzzer = PTHREAD_COND_INITIALIZER;
bool SystemControl::terminateThreads = false;
SystemControl* SystemControl::instance = nullptr;

SystemControl::SystemControl(I_Sensor* ps, I_Sensor* vsw, I_Sensor* us, I_Actuator* a, I_Location* loc, I_Communication* comm, const std::string& dbname)

    : pressure_sensor(ps), 
      vibration_sensor_window(vsw), 
      ultrasonic_sensor(us), 
      actuator(a),
      locationModule(loc), 
      communicationModule(comm),
      database(std::make_unique<C_Database>(dbname)), 
      terminate_read(false),
      terminate_check(false),
      buzzer_active(false),
      gps_active(false) {
    
    instance = this;
    pthread_mutex_init(&mutexSensors, nullptr);
    pthread_mutex_init(&mutexBuzzer, nullptr);
    pthread_cond_init(&condBuzzer, nullptr);
}

SystemControl :: ~SystemControl(){
    pthread_mutex_destroy(&mutexSensors);
    pthread_mutex_destroy(&mutexBuzzer);
    pthread_cond_destroy(&condBuzzer);
}

void SystemControl:: SetupMessageQueues(void) {
    mq_unlink("/msqSensorsValue");
    mq_unlink("/msqSMSCommunication");
    mq_unlink("/msqGPSCommunication");
    mq_unlink("/msqSMS");
    mq_unlink("/msqGPS");
    mq_unlink("/msqGPStoSMS");
    mq_unlink("msqStoreLocation");
    mq_unlink("msqStoreAlert");
    mq_unlink("msqRequestLocation");

    mqd_t mq;
    struct mq_attr attr;
    attr.mq_flags = 0;       // Blocking mode
    attr.mq_maxmsg = 200;     // Maximum number of messages in queue
    attr.mq_msgsize = 256;   // Maximum size of one message
    attr.mq_curmsgs = 0;     // Number of messages currently in queue

    mq = mq_open("/msqSensorsValue", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, &attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create /msqSensorsValue");
        exit(EXIT_FAILURE);
    }
    mq_close(mq);

    mq = mq_open("/msqSMSCommunication", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, &attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create /msqSMSCommunication");
        exit(EXIT_FAILURE);
    }
    mq_close(mq);

    mq = mq_open("/msqGPSCommunication", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, &attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create /msqGPSCommunication");
        exit(EXIT_FAILURE);
    }
    mq_close(mq);

        mq = mq_open("/msqSMS", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG,&attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create /msqSMS");
        exit(EXIT_FAILURE);
    }
    mq_close(mq);

        mq = mq_open("/msqGPS", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG,&attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create /msqGPS");
        exit(EXIT_FAILURE);
    }
    mq_close(mq);

        mq = mq_open("/msqGPStoSMS", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, &attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create /msqGPStoSMS");
        exit(EXIT_FAILURE);
    }
 
        mq = mq_open("/msqStoreLocation", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, &attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create /msqStoreLocation");
        exit(EXIT_FAILURE);
    }

        mq = mq_open("/msqStoreAlert", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, &attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create /msqStoreAlert");
        exit(EXIT_FAILURE);
    }  else   {
        struct mq_attr currentAttr;
        if (mq_getattr(mq, &currentAttr) == -1) {
            perror("Failed to get attributes for /msqStoreAlert");
        }
        char *buffer = new char[currentAttr.mq_msgsize];
        unsigned int prio;
        for (int i = 0; i < currentAttr.mq_curmsgs; ++i) {
            if (mq_receive(mq, buffer, currentAttr.mq_msgsize, &prio) == -1) {
                perror("Error while flushing message");
                break;
            }
            std::cout << "Discarding message: " << buffer << std::endl;
        }
        delete[] buffer;
    }

        mq = mq_open("/msqRequestLocation", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, &attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create /msqRequestLocation");
        exit(EXIT_FAILURE);
    }

    mq_close(mq);
}

void SystemControl :: SetupThread(int prio,pthread_attr_t *pthread_attr,struct sched_param *pthread_param)
{
	int rr_min_priority, rr_max_priority;

	pthread_attr_setschedpolicy (pthread_attr, SCHED_RR);
	rr_min_priority = sched_get_priority_min (SCHED_RR);
	rr_max_priority = sched_get_priority_max (SCHED_RR);
	pthread_param->sched_priority = prio;

	pthread_attr_setschedparam (pthread_attr, pthread_param);
}

void SystemControl :: initializeThreads(void){

    SetupMessageQueues();

    int thread_policy;

    pthread_attr_t thread_attr;
	struct sched_param thread_param;

	pthread_attr_init (&thread_attr);
	pthread_attr_getschedpolicy (&thread_attr, &thread_policy);
	pthread_attr_getschedparam (&thread_attr, &thread_param);
	
    SetupThread(15,&thread_attr,&thread_param);
	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
    int rReadSensors = pthread_create(&tReadSensorsID, NULL, tReadSensors, this);

    SetupThread(15,&thread_attr,&thread_param);
	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
    int rCheckSensors = pthread_create(&tCheckSensorsID, NULL, tCheckSensors, this);

    SetupThread(25,&thread_attr,&thread_param);
	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
    int rBuzzer = pthread_create(&tBuzzerID, NULL, tBuzzer, this);

    SetupThread(25,&thread_attr,&thread_param);
	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
    int rSMSCommunication = pthread_create(&tSMSCommunicationID, NULL, tSMSCommunication, this);

    SetupThread(25,&thread_attr,&thread_param);
	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
    int rGPSCommunication = pthread_create(&tGPSCommunicationID, NULL, tGPSCommunication, this);

    SetupThread(40,&thread_attr,&thread_param);
	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
    int rSMS = pthread_create(&tSMSID, NULL, tSMS, this);

    SetupThread(35,&thread_attr,&thread_param);
	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
    int rGPS = pthread_create(&tGPSID, NULL, tGPS, this);

    SetupThread(30,&thread_attr,&thread_param);
	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
    int rDataBase = pthread_create(&tDataBaseID, NULL, tDataBase, this);

    if (rReadSensors || rCheckSensors || rBuzzer || rSMSCommunication || rGPSCommunication || rSMS || rGPS || rDataBase)
    {
        printf("ERROR; return code from pthread_create() is %d,%d,%d,%d,%d,%d,%d,%d\n", 
            rReadSensors, rCheckSensors, rBuzzer, rSMSCommunication, rGPSCommunication, rSMS, rGPS, rDataBase);
        exit(-1);
    }

}

void SystemControl :: SetupSignals() {
    signal(SIGALRM, [](int) { std::cout << "Timer signal received!" << std::endl; });
}

void SystemControl :: StartSystem() {
    terminateThreads = false;
    SetupMessageQueues();
    SetupSignals();
    initializeThreads();
}

void SystemControl :: ShutDownSystem() {

    pthread_join(tReadSensorsID, nullptr);
    pthread_join(tCheckSensorsID, nullptr);
    pthread_join(tBuzzerID, nullptr);
    pthread_join(tSMSCommunicationID, nullptr);
    pthread_join(tGPSCommunicationID, nullptr);

    mq_unlink("/msqSensorsValue");
    mq_unlink("/msqSMSCommunication");
    mq_unlink("/msqGPSCommunication");
    mq_unlink("/msqSMS");
    mq_unlink("/msqGPS");
}


void SystemControl::activate_sms_communication(){
  SystemControl* self = instance;

    mqd_t msqSMSCommunication_id;
    unsigned int msgprio = 1;
    pid_t my_sms_pid = getpid();
    char msg_alert_content[16]; 
    
    if (msgprio == 0) {
        exit(1);
    }
    msqSMSCommunication_id = mq_open("/msqSMSCommunication", O_RDWR);
    if (msqSMSCommunication_id == (mqd_t)-1) {
        perror("In mq_open()");
        exit(1);
    }
    pthread_mutex_lock(&self->mutexBuzzer);
        pthread_cond_signal(&self->condBuzzer);
        self->buzzer_active=true;
        snprintf(msg_alert_content, sizeof(msg_alert_content), "Theft Attempted");
        printf("trigger\n");
    pthread_mutex_unlock(&self->mutexBuzzer);
    snprintf(msg_alert_content, sizeof(msg_alert_content), "Theft Attempted");
    mq_send(msqSMSCommunication_id, msg_alert_content, strlen(msg_alert_content)+1, msgprio);

    self->set_alarm=true;
    mq_close(msqSMSCommunication_id); 
}

void SystemControl::activate_gps_communication(){

    mqd_t msqGPSCommunication_id;
    unsigned int msgprio = 1;
    pid_t my_gps_pid = getpid();
    char msg_gps_content[7]; 

    if (msgprio == 0) {
        exit(1);
    }
    
    msqGPSCommunication_id = mq_open("/msqGPSCommunication", O_RDWR);
    if (msqGPSCommunication_id == (mqd_t)-1) {
        perror("In mq_open()");
        exit(1);
    }
    snprintf(msg_gps_content, sizeof(msg_gps_content), "GPS ON");
    mq_send(msqGPSCommunication_id, msg_gps_content, strlen(msg_gps_content)+1, msgprio);

    terminate_read = true;
    terminate_check = true;
    set_gps_on=true;

    mq_close(msqGPSCommunication_id); 
}

void SystemControl :: readSensorValues(float& pressure, float& vibration_window, float& distance) {
        pthread_mutex_lock(&mutexSensors);
            pressure = pressure_sensor->getValue();
            vibration_window = vibration_sensor_window->getValue();
            distance = ultrasonic_sensor->getValue();
        pthread_mutex_unlock(&mutexSensors);
}

void SystemControl :: checkSensorValues(float& pressure, float& vibration_window, float& distance) {
    if (ultrasonic_sensor->CheckForTrigger(distance)) {
        activate_gps_communication();
    } 
    
    if (pressure_sensor->CheckForTrigger(pressure) ||
    vibration_sensor_window->CheckForTrigger(vibration_window) ||
    ultrasonic_sensor->CheckForTrigger(distance)) {
        activate_sms_communication();
    }
}

void SystemControl::signalrm_condReadSensors(int sig) {
    SystemControl* self = instance;

    if (self->terminate_read) {
        struct itimerval stop_timer = {0};
        setitimer(ITIMER_REAL, &stop_timer, nullptr);  
        return;
    }

    double vibration_window = 0, pressure = 0, distance = 0;

    pthread_mutex_lock(&self->mutexSensors);
        vibration_window = self->vibration_sensor_window->getValue();
        pressure = self->pressure_sensor->getValue();
        distance = self->ultrasonic_sensor->getValue();
    pthread_mutex_unlock(&self->mutexSensors);

    mqd_t msqSensorsValue_id = mq_open("/msqSensorsValue", O_RDWR);
    if (msqSensorsValue_id == (mqd_t)-1) {
        perror("In mq_open()");
        return;
    }

    char msgcontent[500];
    snprintf(msgcontent, sizeof(msgcontent), "distance %lf, vibr wind %lf, press %lf", distance, vibration_window, pressure);
    std::cout << msgcontent << std::endl;

    mq_send(msqSensorsValue_id, msgcontent, strlen(msgcontent) + 1, 1);
}

void* SystemControl::tReadSensors(void* systemControl) {
    SystemControl* self = static_cast<SystemControl*>(systemControl);

    mqd_t msqSensorsValue_id = mq_open("/msqSensorsValue", O_RDWR);
    if (msqSensorsValue_id == (mqd_t)-1) {
        perror("In mq_open()");
        pthread_exit(NULL);
    }

    // Setup signal handler
    signal(SIGALRM, SystemControl::signalrm_condReadSensors);

    struct itimerval timer;
    timer.it_value.tv_sec = 2;  // Initial expiration in 2 seconds
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0; // Repeat every 2 seconds
    timer.it_interval.tv_usec = 500000;
    setitimer(ITIMER_REAL, &timer, nullptr);

    while (!self->terminate_read) {
        pause();  
    }

    mq_close(msqSensorsValue_id);
    std::cout << "Exiting tReadSensors..." << std::endl;
    pthread_exit(NULL);
}

void* SystemControl::tCheckSensors(void *systemControl){
    SystemControl* self = static_cast<SystemControl*>(systemControl);

    static double pressure=0;
    static double vibration_window=0;
    static double distance=0;

    mqd_t msqSensorsValue_id;
    int msgsz;
    unsigned int sender;
    struct mq_attr msgq_attr;
    while(!(self->isTerminateCheck())){
        msqSensorsValue_id = mq_open("/msqSensorsValue", O_RDWR);
        if (msqSensorsValue_id== (mqd_t)-1) {
            perror("In mq_open()");
            exit(1);
        }
        mq_getattr(msqSensorsValue_id, &msgq_attr);
        char *msg_sensors_content = new char[msgq_attr.mq_msgsize];
        // getting a message 
        msgsz = mq_receive(msqSensorsValue_id, msg_sensors_content, msgq_attr.mq_msgsize, &sender);
        if (msgsz == -1) {
            perror("In mq_receive()");
            exit(1);
        }
        printf("%s\n", msg_sensors_content);
        sscanf(msg_sensors_content, "distance %lf, vibr wind %lf, press %lf", &distance, &vibration_window, &pressure);


        delete[] msg_sensors_content;
        mq_close(msqSensorsValue_id);

        if (self->getUltrasonicSensor()->CheckForTrigger(distance)) {
            self->activate_gps_communication();
        }

        if (self->getUltrasonicSensor()->CheckForTrigger(distance) ||
            self->getVibrationSensor()->CheckForTrigger(vibration_window) ||
            self->getPressureSensor()->CheckForTrigger(pressure)) {
            self->activate_sms_communication();
        }

    }
    std::cout << "Exiting tCheckSensors..." << std::endl;
    pthread_exit(NULL);
}

void *SystemControl::tBuzzer(void* systemControl){
    SystemControl* self = static_cast<SystemControl*>(systemControl);

    while(true){
        pthread_mutex_lock(&self->mutexBuzzer);
            printf("Buzzer waiting\n");
            pthread_cond_wait(&self->condBuzzer, &self->mutexBuzzer);
            self->getBuzzer()->ActivateActuator(50,50);
        pthread_mutex_unlock(&self->mutexBuzzer); 
        if(self->gps_active)
            break;
    }
   
    pthread_exit(NULL);
}

void* SystemControl::tSMSCommunication(void* systemControl){
    SystemControl* self = static_cast<SystemControl*>(systemControl);
    mqd_t msqSMSCommunication_id;
    int msgsz;
    unsigned int sender;
    struct mq_attr msgq_attr;

    while(true){
    if(self->set_alarm){
        self->set_alarm=false;

        msqSMSCommunication_id = mq_open("/msqSMSCommunication", O_RDWR);
        if (msqSMSCommunication_id == (mqd_t)-1) {
            perror("In mq_open()");
            exit(1);
        }

        mq_getattr(msqSMSCommunication_id, &msgq_attr);
        char *msg_alert_content = new char[msgq_attr.mq_msgsize];
        msgsz = mq_receive(msqSMSCommunication_id, msg_alert_content, msgq_attr.mq_msgsize, &sender);
        if (msgsz == -1) {
            perror("In mq_receive()");
            exit(1);
        }

        printf("Received message (%d bytes) from %d: %s\n", msgsz, sender, msg_alert_content);

        const char* target_sentence = "Theft Attempted";
        if (strcmp(msg_alert_content, target_sentence) == 0) {              
            
            mqd_t msqSMS_id;
            unsigned int msgprio = 1;
            pid_t my_sms_pid = getpid();
            char msg_sms_content[50];
            
            if (msgprio == 0) {
                exit(1);
            }
            msqSMS_id = mq_open("/msqSMS", O_RDWR);
            if (msqSMS_id == (mqd_t)-1) {
                perror("In mq_open()");
                exit(1);
            }
            printf("activate sms\n");
            snprintf(msg_sms_content, sizeof(msg_sms_content), "Activate SMS");
            mq_send(msqSMS_id, msg_sms_content, strlen(msg_sms_content)+1, msgprio);
            
            mq_close(msqSMS_id); 
        }

        delete[] msg_alert_content;

        mq_close(msqSMSCommunication_id); 
    }
    }
    pthread_exit(NULL);
}

void* SystemControl::tGPSCommunication(void* systemControl){
    SystemControl* self = static_cast<SystemControl*>(systemControl);
     mqd_t msqGPSCommunication_id;
    int msgsz;
    unsigned int sender;
    struct mq_attr msgq_attr;
while(true){
  if(self->set_gps_on){ 
    self->set_gps_on=false;

    msqGPSCommunication_id = mq_open("/msqGPSCommunication", O_RDWR);
    if (msqGPSCommunication_id == (mqd_t)-1) {
        perror("In mq_open()");
        exit(1);
    }

    mq_getattr(msqGPSCommunication_id, &msgq_attr);
    char *msg_gps_content = new char[msgq_attr.mq_msgsize];

    msgsz = mq_receive(msqGPSCommunication_id, msg_gps_content, msgq_attr.mq_msgsize, &sender);
    if (msgsz == -1) {
        perror("In mq_receive()");
        exit(1);
    }

    printf("Received message (%d bytes) from %d: %s\n", msgsz, sender, msg_gps_content);

    const char* target_sentence = "GPS ON";
    if (strcmp(msg_gps_content, target_sentence) == 0) {               
        
        mqd_t msqGPS_id;
        unsigned int msgprio = 1;
        pid_t my_gps_pid = getpid();
        char msg_track_content[50];
        
        if (msgprio == 0) {
            exit(1);
        }
        msqGPS_id = mq_open("/msqGPS", O_RDWR);
        if (msqGPS_id == (mqd_t)-1) {
            perror("In mq_open()");
            exit(1);
        }

        printf("activate gps\n");
        self->locationModule->StartTracking();

        snprintf(msg_track_content, sizeof(msg_track_content), "Activate GPS");
        mq_send(msqGPS_id, msg_track_content, strlen(msg_track_content)+1, msgprio);
        mq_close(msqGPS_id); 
    }

    delete[] msg_gps_content;
    mq_close(msqGPSCommunication_id);  
  }
}
    pthread_exit(NULL);
}


void* SystemControl :: tSMS(void* systemControl){
    SystemControl* self = static_cast<SystemControl*>(systemControl);
    mqd_t msqSMS_id;
    int msgsz1;
    unsigned int sender1;
    struct mq_attr msgq_attr1;
    msqSMS_id = mq_open("/msqSMS", O_RDWR | O_NONBLOCK);
    if (msqSMS_id == (mqd_t)-1) {
        perror("In mq_open()");
        exit(1);
    }

    mqd_t msqGPStoSMS_id;
    int msgsz2;
    unsigned int sender2;
    struct mq_attr msgq_attr2;
    msqGPStoSMS_id = mq_open("/msqGPStoSMS", O_RDWR | O_NONBLOCK);
    if (msqGPStoSMS_id == (mqd_t)-1) {
        perror("In mq_open()");
        exit(1);
    }


    mqd_t msqStoreAlert_id;
    unsigned int msgprio = 1;
    pid_t my_store_pid = getpid();
    char msg_store_content[50];
    
    if (msgprio == 0) {
        exit(1);
    }
    msqStoreAlert_id = mq_open("/msqStoreAlert", O_RDWR | O_NONBLOCK);
    if (msqSMS_id == (mqd_t)-1) {
        perror("In mq_open()");
        exit(1);
    }   

    while (true){
    if(!(self->gps_active)){
        msgsz1=-1;
        
        mq_getattr(msqSMS_id, &msgq_attr1);
        char *msg_sms_content = new char[msgq_attr1.mq_msgsize];
        msgsz1 = mq_receive(msqSMS_id, msg_sms_content, msgq_attr1.mq_msgsize, &sender1);
        if (msgsz1 == -1) {
            if (errno == EAGAIN) {
                    usleep(100000); 
                    continue;
                }
            perror("In mq_receive()");
            exit(1);
        }
        printf("%s\n", msg_sms_content);
        
        const char* target_sentence_sms = "Activate SMS";
        if (strcmp(msg_sms_content, target_sentence_sms) == 0) { 
            snprintf(msg_store_content, sizeof(msg_store_content), "Trigger");
            printf("Trigger\n");
            self->communicationModule->SendMessage("Trigger");
            mq_send(msqStoreAlert_id, msg_store_content, strlen(msg_store_content)+1, msgprio);
            }
        }

    }
        if(self->gps_active){
            mq_getattr(msqGPStoSMS_id, &msgq_attr2);
            char *msg_gps_content = new char[msgq_attr2.mq_msgsize];
            msgsz2 = mq_receive(msqGPStoSMS_id, msg_gps_content, msgq_attr2.mq_msgsize, &sender2);
            if (msgsz2 == -1) {
                if (errno == EAGAIN) {
                    usleep(100000);
                }
                perror("In mq_receive()");
                exit(1);
            }
            printf("Received message (%d bytes) from %d: %s\n", msgsz2, sender2, msg_gps_content);
            self->communicationModule->SendMessage(std::string (msg_gps_content));
        }

        mq_close(msqSMS_id); 
        mq_close(msqGPStoSMS_id);
        mq_close(msqStoreAlert_id);

    pthread_exit(NULL);
}

void* SystemControl :: tGPS(void* systemControl){
    SystemControl* self = static_cast<SystemControl*>(systemControl);
            mqd_t msqGPS_id;
        int msgsz1;
        unsigned int sender1;
        struct mq_attr msgq_attr1;
        msqGPS_id = mq_open("/msqGPS", O_RDWR);
        if (msqGPS_id == (mqd_t)-1) {
            perror("In mq_open()");
            exit(1);
        }


        mqd_t msqGPStoSMS_id;
        unsigned int msgprio1 = 1;
        pid_t my_gpstosms_pid = getpid();
        char msg_track_content[100];
        
        if (msgprio1 == 0) {
            exit(1);
        }
        msqGPStoSMS_id = mq_open("/msqGPStoSMS", O_RDWR);
        if (msqGPStoSMS_id == (mqd_t)-1) {
            perror("In mq_open()");
            exit(1);
        }


        mqd_t msqStoreLocation_id;
        unsigned int msgprio2 = 1;
        pid_t my_storelocation_pid = getpid();
        char msg_store_content[100];
        
        if (msgprio2 == 0) {
            exit(1);
        }
        msqStoreLocation_id = mq_open("/msqStoreLocation", O_RDWR);
        if (msqStoreLocation_id == (mqd_t)-1) {
            perror("In mq_open()");
            exit(1);
        }


    while(true){        
        mq_getattr(msqGPS_id, &msgq_attr1);
        char *msg_gps_content = new char[msgq_attr1.mq_msgsize];
        msgsz1 = mq_receive(msqGPS_id, msg_gps_content, msgq_attr1.mq_msgsize, &sender1);
        if (msgsz1 == -1) {
            perror("In mq_receive()");
            exit(1);
        }
        const char* target_sentence1 = "Activate GPS";
        if (strcmp(msg_gps_content, target_sentence1) == 0) { 
            self->gps_active=true;
        }

        while(self->gps_active){
            C_GPSModule::coordinates coord = self->locationModule->getLocation();

            snprintf(msg_track_content, sizeof(msg_track_content), "Lat: %lf, Lon: %lf", coord.latitude, coord.longitude);
            printf("%s\n", msg_track_content);
            
            if (mq_send(msqGPStoSMS_id, msg_track_content, strlen(msg_track_content) + 1, msgprio1) == -1) {
                perror("mq_send failed");
            } else {
                printf("GPS message sent: %s\n", msg_track_content);
            }
          
            snprintf(msg_store_content, sizeof(msg_store_content), "Lat: %lf, Lon: %lf", coord.latitude, coord.longitude);
            mq_send(msqStoreLocation_id, msg_store_content, strlen(msg_store_content)+1, msgprio2);

            mq_send(msqStoreLocation_id, msg_store_content, strlen(msg_store_content)+1, msgprio2);
            sleep(8);
        }
        
       
    } 

    self->locationModule->StopTracking();
    mq_close(msqGPS_id); 
    mq_close(msqGPStoSMS_id); 
    mq_close(msqStoreLocation_id); 
    pthread_exit(NULL);
    
}

void* SystemControl :: tDataBase(void* systemControl){
    SystemControl* self = static_cast<SystemControl*>(systemControl);
    mqd_t msqStoreLocation_id;
        int msgsz1;
        unsigned int sender1;
        struct mq_attr msgq_attr1;
        
        msqStoreLocation_id = mq_open("/msqStoreLocation", O_RDWR );
        if (msqStoreLocation_id == (mqd_t)-1) {
            perror("In mq_open()");
            exit(1);
        }


        mqd_t msqStoreAlert_id;
        int msgsz2;
        unsigned int sender2;
        struct mq_attr msgq_attr2;
        
        msqStoreAlert_id = mq_open("/msqStoreAlert", O_RDWR );
        if (msqStoreAlert_id == (mqd_t)-1) {
            perror("In mq_open()");
            exit(1);
        }


        mqd_t msqRequestLocation_id;
        int msgsz3;
        unsigned int sender3;
        struct mq_attr msgq_attr3;
        
        msqRequestLocation_id = mq_open("/msqRequestLocation", O_RDWR );
        if (msqRequestLocation_id == (mqd_t)-1) {
            perror("In mq_open()");
            exit(1);
        }  

        while (true){

            mq_getattr(msqStoreLocation_id, &msgq_attr1);
            char *msg_storelocation_content = new char[msgq_attr1.mq_msgsize];
            // getting a message 
            msgsz1 = mq_receive(msqStoreLocation_id, msg_storelocation_content, msgq_attr1.mq_msgsize, &sender1);
            if (msgsz1 == -1) {
                if (errno == EAGAIN) {
                    usleep(100000);
                    continue;
                }
                perror("In mq_receive()");
                exit(1);
            }

            mq_getattr(msqStoreAlert_id, &msgq_attr2);
            char *msg_storealert_content = new char[msgq_attr2.mq_msgsize];
            // getting a message 
            msgsz2 = mq_receive(msqStoreAlert_id, msg_storealert_content, msgq_attr2.mq_msgsize, &sender2);
            if (msgsz2 == -1) {
                if (errno == EAGAIN) {
                    usleep(100000); 
                    continue;
                }
                perror("In mq_receive()");
                exit(1);
            }


            mq_getattr(msqRequestLocation_id, &msgq_attr3);
            char *msg_requestlocation_content = new char[msgq_attr3.mq_msgsize];
            msgsz3 = mq_receive(msqRequestLocation_id, msg_requestlocation_content, msgq_attr3.mq_msgsize, &sender3);
            if (msgsz3 == -1) {
                if (errno == EAGAIN) {
                    usleep(100000); 
                    continue;
                }
                perror("In mq_receive()");
                exit(1);
            }

            C_GPSModule::coordinates coord;
            sscanf(msg_storelocation_content, "Lat: %lf, Lon: %lf", &coord.latitude, &coord.longitude);
            self->database->storeLocation("Coordinates", coord);


            const char* target_sentence1 = "Trigger";
            if (strcmp(msg_storealert_content, target_sentence1) == 0) { 
                self->database->storeAlert("Alerts", msg_storealert_content);
            }

        }

        mq_close(msqStoreLocation_id); 
        mq_close(msqStoreAlert_id);
        mq_close(msqRequestLocation_id);
    pthread_exit(NULL);
}