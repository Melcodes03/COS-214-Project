#ifndef SMSCONNECTOR_H
#define SMSCONNECTOR_H

#include <iostream>
#include "SmsGateway.h"
#include "CommunicationAdapter.h"

/** @brief Communication adapter for sending notifications by SMS. */
class SmsConnector : public CommunicationAdapter
{
private:
    SmsGateway gateway;
public:
    SmsConnector(/* args */);
    ~SmsConnector();
    /**
     * @brief Sends an SMS notification.
     * @param number Destination phone number.
     * @param message SMS text to send.
     */
    void send(std::string number, std::string message, std::string p = "n/a") override;
};

#endif
