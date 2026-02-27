#ifndef C_DATABASE_H
#define C_DATABASE_H

#include <iostream>
#include <string>
#include <sqlite3.h> 
#include "C_GPSModule.h"
#include "I_Database.h"

using namespace std;

class C_Database : public I_Database{
private:
    sqlite3* db;  
    float sensor_data;

public:
    C_Database(const string& dbname);  
    ~C_Database();
    void storeAlert(const string& tablename, const string& trigger);
    void storeLocation(const string& tablename, C_GPSModule::coordinates& coord);
    void clearAlert();
    void clearLocation();
    vector<C_GPSModule::coordinates> getLastCoordinates(const string& tablename);
    void CreateTable(const string& tablename);
};

#endif

