#ifndef SMSGATEWAY_H
#define SMSGATEWAY_H

#include <iostream>

/** @brief Gateway that transmits SMS text messages. */
class SmsGateway
{
private:
    /* data */
public:
    SmsGateway(/* args */);
    ~SmsGateway();
    /**
     * @brief Transmits an SMS to a phone number.
     * @param number Destination phone number.
     * @param text Message content.
     * @return True when transmission succeeds.
     */
    bool transmit(std::string number,std::string text);
};

#endif
