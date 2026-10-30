#ifndef EMAILCONNECTOR_H
#define EMAILCONNECTOR_H

#include "MailService.h"
#include "CommunicationAdapter.h"

class EmailConnector : public CommunicationAdapter
{
private:
    MailService mail;
public:
    EmailConnector(/* args */);
    ~EmailConnector();
    void send(std::string recipient, std::string, std::string = "normal") override;
};






#endif
