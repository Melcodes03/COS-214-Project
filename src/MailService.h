#ifndef MailService_H
#define MailService_H

#include <iostream>

/** @brief Service responsible for dispatching email messages. */
class MailService
{
private:
    /* data */
public:
    MailService(/* args */);
    ~MailService();
    /**
     * @brief Dispatches an email message.
     * @param to Recipient email address.
     * @param subject Email subject.
     * @param body Email body.
     * @param priority Delivery priority.
     * @return True when the message is dispatched successfully.
     */
    bool dispatchMail(std::string to, std::string subject, std::string body, std::string priority);
};

#endif
