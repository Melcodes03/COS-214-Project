#include "SmsGateway.h"

SmsGateway::SmsGateway(/* args */){

}

SmsGateway::~SmsGateway(){
    
}

bool SmsGateway::transmit(std::string number,std::string text){
    //test stub: prints instead of sending, as the brief allows for external services
    std::cout << "===== Message sent to "<<number<<" =====\n  " << text << "\n";
    return true;
}