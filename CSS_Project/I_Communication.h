#ifndef I_COMMUNICATION_H
#define I_COMMUNICATION_H

#include <string>

class I_Communication {
public:
    virtual ~I_Communication() = default;
    virtual void ReceiveMessage() = 0;
    virtual void SendMessage(std::string message) = 0;
};

#endif