#include "SmsConnector.h"

SmsConnector::SmsConnector(/* args */)
{
}

SmsConnector::~SmsConnector()
{
}

void SmsConnector::send(std::string number, std::string message, std::string p){
    gateway.transmit(number, message);
}