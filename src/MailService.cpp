#include "MailService.h"

bool MailService::dispatchMail(std::string to, std::string subject, std::string body, std::string priority){
    
    //test stub: prints instead of sending, as the brief allows for external services
    std::cout << "===== Connecting to Gmail... =====\n===== Email sent to "<<to<<" =====\n"
              << "  [" << priority << "] " << subject << ": " << body << "\n";
    return true;
}

MailService::MailService()
{

}

MailService::~MailService()
{

}