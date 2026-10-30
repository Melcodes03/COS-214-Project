#include "MailService.h"

bool MailService::dispatchMail(std::string to, std::string subject, std::string body, std::string priority){
    
    std::cout << "===== Connecting to Gmail... =====\n===== Email sent to "<<to<<" =====\n";
    return true;
}

MailService::MailService()
{

}

MailService::~MailService()
{

}