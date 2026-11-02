#ifndef EMAILCONNECTOR_H
#define EMAILCONNECTOR_H

#include "MailService.h"
#include "CommunicationAdapter.h"

/** @brief Communication adapter for sending notifications by email. */
class EmailConnector : public CommunicationAdapter
{
private:
    MailService mail;
public:
    EmailConnector(/* args */);
    ~EmailConnector();
    /**
     * @brief Sends an email notification.
     * @param recipient Email address of the recipient.
     * @param message Email message body.
     */
    void send(std::string recipient, std::string message, std::string priority = "normal") override;
};






#endif
