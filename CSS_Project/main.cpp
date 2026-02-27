#include <memory>
#include "SystemControl.h"
#include "C_Sensors.h"
#include "C_VibrationSensor_Window.h"
#include "C_PressureSensor.h"
#include "C_UltrasonicSensor.h"
#include "C_Buzzer.h"
#include "C_GPSModule.h"
#include "C_SMSModule.h"
#include "C_Database.h"

int main() {

  auto sensorComposite = std::make_unique<C_Sensors>();
  auto pressureSensor = std::make_unique<C_PressureSensor>();
  auto vibrationSensorWindow = std::make_unique<C_VibrationSensor_Window>();
  auto ultrasonicSensor = std::make_unique<C_UltrasonicSensor>();
  auto buzzer = std::make_unique<C_Buzzer>();

    auto gpsModule = std::make_unique<C_GPSModule>();
    auto smsModule = std::make_unique<C_SMSModule>();

    SystemControl system(
        pressureSensor.get(),       
        vibrationSensorWindow.get(), 
        ultrasonicSensor.get(),    
        buzzer.get(),        
        gpsModule.get(),           
        smsModule.get(),      
        "my_database_name.db"                
    );

     system.StartSystem();

    return 0;
}
