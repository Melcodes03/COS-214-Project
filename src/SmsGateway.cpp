#include "SmsGateway.h"

SmsGateway::SmsGateway(/* args */){

}

SmsGateway::~SmsGateway(){
    
}

bool SmsGateway::transmit(std::string number,std::string text){
    std::cout << "===== Message sent to "<<number<<" =====\n";
    return true;
}