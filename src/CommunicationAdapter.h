#ifndef COMMUNICATIONADAPTER_H
#define COMMUNICATIONADAPTER_H

#include <iostream>

// ADAPTER INTERFACE

/** @brief Adapter interface for sending workflow notifications. */
class CommunicationAdapter
{
private:
    /* data */
    
public:
    CommunicationAdapter(/* args */);
    virtual ~CommunicationAdapter() = 0;
    /**
     * @brief Sends a notification through the communication channel.
     * @param recipient Destination address or identifier.
     * @param message Notification text to send.
     * @param p Delivery priority or channel-specific priority string.
     */
    virtual void send(std::string recipient, std::string message, std::string p = "normal") = 0;
};

#endif