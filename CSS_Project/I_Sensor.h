#ifndef I_SENSOR_H
#define I_SENSOR_H

class I_Sensor {
public:
    virtual ~I_Sensor() = default;
    
    virtual bool CheckForTrigger(double value_read)= 0;
    virtual double getValue()= 0;
};

#endif