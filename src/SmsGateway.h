#ifndef SMSGATEWAY_H
#define SMSGATEWAY_H

#include <iostream>

class SmsGateway
{
private:
    /* data */
public:
    SmsGateway(/* args */);
    ~SmsGateway();
    bool transmit(std::string number,std::string text);
};

#endif
