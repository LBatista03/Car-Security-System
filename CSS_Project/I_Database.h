#ifndef I_DATABASE
#define I_DATABASE

#include <iostream>
#include "C_GPSModule.h"
using namespace std;

class I_Database {
public:
    virtual ~I_Database () = default;

    virtual void storeAlert(const string& tablename, const string& trigger) = 0;
    virtual void storeLocation(const string& tablename, C_GPSModule::coordinates& coord) = 0;
    virtual void clearAlert() = 0;
    virtual void clearLocation() = 0;
    virtual vector<C_GPSModule::coordinates> getLastCoordinates(const string& tablename) = 0;
    virtual void CreateTable(const string& tablename) = 0;
};


#endif