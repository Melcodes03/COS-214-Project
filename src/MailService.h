#ifndef MailService_H
#define MailService_H

#include <iostream>

class MailService
{
private:
    /* data */
public:
    MailService(/* args */);
    ~MailService();
    bool dispatchMail(std::string to, std::string subject, std::string body, std::string priority);
};

#endif
