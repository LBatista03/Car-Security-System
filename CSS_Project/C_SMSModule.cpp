#include "C_SMSModule.h"

int C_SMSModule :: setupSerialPort(const char* port, int baudRate) {
    int serialPort = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (serialPort == -1) {
        std::cerr << "Erro ao abrir a porta serial!" << std::endl;
        return -1;
    }

    struct termios tty;
    if (tcgetattr(serialPort, &tty) != 0) {
        std::cerr << "Erro ao obter os parâmetros da porta serial!" << std::endl;
        return -1;
    }

    cfsetospeed(&tty, baudRate);
    cfsetispeed(&tty, baudRate);

    tty.c_cflag &= ~PARENB;    // Desativa paridade
    tty.c_cflag &= ~CSTOPB;    // 1 bit de parada
    tty.c_cflag &= ~CSIZE;     // Limpa os bits de tamanho de dados
    tty.c_cflag |= CS8;        // 8 bits de dados
    tty.c_cflag &= ~CRTSCTS;   // Desativa controle de fluxo
    tty.c_cflag |= CREAD | CLOCAL;  // Habilita a leitura e desativa controle de linha

    tty.c_lflag &= ~ICANON;    // Modo não-canonical
    tty.c_lflag &= ~ECHO;      // Desativa o echo
    tty.c_lflag &= ~ECHOE;     // Desativa a edição de caracteres
    tty.c_lflag &= ~ISIG;      // Desativa sinais (Ctrl-C, Ctrl-Z)

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    if (tcsetattr(serialPort, TCSANOW, &tty) != 0) {
        std::cerr << "Erro ao aplicar as configurações da porta serial!" << std::endl;
        return -1;
    }

    return serialPort;
}

C_SMSModule :: C_SMSModule(){
    serialPort = setupSerialPort(SERIAL_PORT, baudRate);

    sendATCommand(serialPort, "AT\r\n");
    sleep(1);
    readResponse(serialPort);
        
    sendATCommand(serialPort, "AT+CFUN?\r\n");
    sleep(1);
    readResponse(serialPort);

    sendATCommand(serialPort, "AT+CPIN?\r\n");
    sleep(1);
    readResponse(serialPort);

    sendATCommand(serialPort, "AT+CREG?\r\n");
    sleep(1);
    readResponse(serialPort);
}

C_SMSModule :: ~C_SMSModule(){
    if(serialPort>=0){
        close(serialPort);
    }
}

void C_SMSModule :: ReceiveMessage(){
    sendATCommand(serialPort, "AT+CMGR=?\r\n");
    sleep(10);
    const char* last_loc_sentence="Last location";
    std::string message=readResponse(serialPort);
}

void C_SMSModule :: SendMessage(std::string message){
    sendATCommand(serialPort, "AT+CMGF=1\r\n");
    sleep(1);

    sendATCommand(serialPort, "AT+CMGS=\"+351914149814\"\r\n");
    sleep(1);
    readResponse(serialPort);

    message += "\x1A";
    sendATCommand(serialPort, message);
    sleep(1);
    readResponse(serialPort);
}

void C_SMSModule :: sendATCommand(int serialPort, const std::string& command) {
    if((write(serialPort, command.c_str(), command.length()))==-1){
        std::cerr << "Erro ao enviar comando" << std::endl;
    }
}

std :: string C_SMSModule :: readResponse(int serialPort) {
    char buf[256];
    int n = read(serialPort, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::cout << "Resposta do Módulo SIM900A: " << buf << std::endl;
        return std::string(buf);
    } else {
        std::cerr << "Erro ao ler a resposta da porta serial!" << std::endl;    
        return "";
    }
}