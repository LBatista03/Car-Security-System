#ifndef I_LOCATION_H
#define I_LOCATION_H

#include <string>

class I_Location {

public:
    virtual ~I_Location() = default;

    struct coordinates{
        double longitude;
        double latitude;
    };

    virtual void StartTracking() = 0;
    virtual void StopTracking() = 0;
    virtual coordinates getLocation() = 0;
};

#endif