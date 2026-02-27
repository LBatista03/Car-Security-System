#ifndef C_SENSORS_H
#define C_SENSORS_H

#include "I_Sensor.h"

#include <iostream>
#include <vector>    
#include <memory>    


class C_Sensors : public I_Sensor {

private:
    std::vector<std::unique_ptr<I_Sensor>> sensors;

public:
    void addSensor(std::unique_ptr<I_Sensor> sensor) {
        sensors.push_back(std::move(sensor));
    }

    double getValue() override {
        for (size_t i = 0; i < sensors.size(); ++i) {
            float value = sensors[i]->getValue();
            std::cout << "Sensor " << i + 1 << " value: " << value << std::endl;
        }
        return 0.0f;
    }

    bool CheckForTrigger(double value_read) override {
        bool anyTriggered = false;
        for (size_t i = 0; i < sensors.size(); ++i) {
            if (sensors[i]->CheckForTrigger(value_read)) {
                std::cout << "Sensor " << i + 1 << " triggered!" << std::endl;
                anyTriggered = true;
            }
        }
        return anyTriggered;
    }

};


#endif