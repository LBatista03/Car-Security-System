#ifndef I_ACTUATOR_H
#define I_ACTUATOR_H

class I_Actuator {
public:
    virtual ~I_Actuator() = default;
    virtual void ActivateActuator(int frequency, int duration_ms) = 0;
    virtual void DeactivateActuator() = 0;

};

#endif