#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <iostream>
#include "C_Database.h"
#include <sqlite3.h>
#include <stdexcept>
#include <string>

C_Database::C_Database(const string& dbname): db(nullptr) {
    if (sqlite3_open(dbname.c_str(), &db)) {
        std::cerr << "Falha ao conectar ao SQLite: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("Conexão com o banco de dados falhou");
    } else {
        std::cout << "Conectado ao SQLite com sucesso!" << std::endl;
    }
    CreateTable("Coordinates");
    CreateTable("Alerts");
}

C_Database::~C_Database() {
    if (db) {
        sqlite3_close(db);
    }
}

void C_Database::CreateTable(const string& tablename) {
    char* errMsg = nullptr;
    string query = "CREATE TABLE IF NOT EXISTS " + tablename + " ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "latitude REAL, "
                "longitude REAL, "
                "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Erro ao criar tabela: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Tabela '" << tablename << "' criada ou já existente." << std::endl;
    }
}

void C_Database::storeLocation(const string& tablename, C_GPSModule::coordinates& coord) {
    char* errMsg = nullptr;
    string query = "INSERT INTO " + tablename + "(latitude, longitude) VALUES (" 
                + std::to_string(coord.latitude) + ", " + std::to_string(coord.longitude) + ");";
    
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Erro ao armazenar coordenadas: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Coordenadas armazenadas com sucesso!" << std::endl;
    }
}

void C_Database::storeAlert(const string& tablename, const string& trigger) {
    char* errMsg = nullptr;
    string query = "INSERT INTO " + tablename + "(trigger) VALUES ('" 
                + trigger + "');";
    
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Erro ao armazenar alerta: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Alerta armazenado com sucesso!" << std::endl;
    }
}

void C_Database::clearAlert() {
    char* errMsg = nullptr;
    string query = "DELETE FROM Alerts WHERE id = (SELECT MAX(id) FROM Alerts);";
    
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Erro ao eliminar o alerta: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Último alerta eliminado com sucesso!" << std::endl;
    }
}

void C_Database::clearLocation() {
    char* errMsg = nullptr;
    string query = "DELETE FROM Coordinates WHERE id = (SELECT MAX(id) FROM Coordinates);";
    
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Erro ao eliminar a coordenada: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Última coordenada eliminada com sucesso!" << std::endl;
    }
}

vector<C_GPSModule::coordinates> C_Database::getLastCoordinates(const string& tablename) {
    vector<C_GPSModule::coordinates> all_coords;
    string query = "SELECT latitude, longitude, timestamp FROM " + tablename + ";";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Erro ao preparar a query: " << sqlite3_errmsg(db) << std::endl;
        return all_coords;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        C_GPSModule::coordinates coord;
        coord.latitude = sqlite3_column_double(stmt, 0); 
        coord.longitude = sqlite3_column_double(stmt, 1);  
        all_coords.push_back(coord);
        
        std::cout << "Timestamp: " << sqlite3_column_text(stmt, 2) << " | Latitude: " 
                << coord.latitude << ", Longitude: " << coord.longitude << std::endl;
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Erro ao recuperar coordenadas: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return all_coords;
}
