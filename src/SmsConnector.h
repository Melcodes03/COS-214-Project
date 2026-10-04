#ifndef SMSCONNECTOR_H
#define SMSCONNECTOR_H

#include <iostream>
#include "SmsGateway.h"
#include "CommunicationAdapter.h"

class SmsConnector : public CommunicationAdapter
{
private:
    SmsGateway gateway;
public:
    SmsConnector(/* args */);
    ~SmsConnector();
    void send(std::string number, std::string, std::string p = "n/a") override;
};

#endif
