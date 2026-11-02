#include "SmsConnector.h"

SmsConnector::SmsConnector(/* args */)
{
}

SmsConnector::~SmsConnector()
{
}

void SmsConnector::send(std::string number, std::string message, std::string){   //SMS has no priority setting
    gateway.transmit(number, message);
}