#ifndef COMMUNICATIONADAPTER_H
#define COMMUNICATIONADAPTER_H

#include <iostream>

// ADAPTER INTERFACE

class CommunicationAdapter
{
private:
    /* data */
    
public:
    CommunicationAdapter(/* args */);
    virtual ~CommunicationAdapter() = 0;
    virtual void send(std::string recipient, std::string message, std::string p = "normal") = 0;
};

#endif