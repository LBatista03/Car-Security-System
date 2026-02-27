#include "C_GPSModule.h"

C_GPSModule::C_GPSModule() : i2cfile(-1){

    i2cfile = open(i2cdevice, O_RDWR);
    if (i2cfile < 0) {
        perror("I2C open");
        throw runtime_error("Falha ao abrir o barramento I2C");
    }


    if (ioctl(i2cfile, I2C_SLAVE, ZED_F9R_I2C_ADDRESS) < 0) {
        perror("I2C ioctl");
        close(i2cfile);
        throw runtime_error("Falha ao configurar o endereço do dispositivo I2C");
    }

    cout << "I2C inicializado com sucesso." << std::endl;
}

C_GPSModule ::~C_GPSModule(){

    if(i2cfile >= 0){
        close(i2cfile);
    }
}

vector<uint8_t> C_GPSModule :: readGPSData(size_t length){

    vector<uint8_t> buffer(length);

    ssize_t bytesRead = read(i2cfile, buffer.data(), length);
    if (bytesRead < 0) {
        perror("I2C read");
        throw runtime_error("Failed to read GPS data");
    }
    buffer.resize(bytesRead);
    return buffer;
}

void C_GPSModule::parseData(const vector<uint8_t>& data, coordinates& coord) { 

    if (data.size() < 9) {
        std::cerr << "Incomplete GPS data received." << std::endl;
        return;
    }
    int32_t rawLatitude = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
    int32_t rawLongitude = (data[5] << 24) | (data[6] << 16) | (data[7] << 8) | data[8];

    coord.latitude = rawLatitude / 1e7;
    coord.longitude = rawLongitude / 1e7;

    cout << "Latitude: " << coord.latitude << "°" << std::endl;
    cout << "Longitude: " << coord.longitude << "°" << std::endl;

}


void C_GPSModule::StartTracking() {
    if (i2cfile < 0) {
        cerr << "Failed to start tracking: I2C initialization error!" << std::endl;
        return;
    }
        cout << "I2C already initialized successfully. Tracking started." << std::endl;
}


void C_GPSModule :: StopTracking(){

    if (i2cfile != -1) {
        close(i2cfile);
        cout << "Barramento I2C fechado." << std::endl;
        i2cfile = -1;
    } else {
        cout << "Barramento I2C já estava fechado." << std::endl;
    }
        cout << "Rastreamento finalizado." << std::endl;
}


C_GPSModule:: coordinates C_GPSModule :: getLocation(){

    coordinates coord;
    try{
        vector<uint8_t> gpsData = readGPSData(100);
        parseData(gpsData, coord);
        return {coord.latitude, coord.longitude};
    }catch(const std::exception &e) {
        cerr << "Erro ao buscar dados de GPS: " << e.what() << std::endl;
        return {0.0, 0.0}; 
    }

}