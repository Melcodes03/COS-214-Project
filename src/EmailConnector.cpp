#include "EmailConnector.h"

EmailConnector::EmailConnector(/* args */)
{
}

EmailConnector::~EmailConnector()
{
}

void EmailConnector::send(std::string recipient, std::string message, std::string p){
    mail.dispatchMail(recipient, "WORK NOTIFICATION", message, p);
}