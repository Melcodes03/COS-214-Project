#include "CommunicationAdapter.h"
#include "EmailConnector.h"
#include "MailService.h"
#include "SmsConnector.h"
#include "SmsGateway.h"

int main(){

    EmailConnector gmail;

    gmail.send("john@gmail.com", "yoooooooo! What's up?");

    SmsConnector VODACOM;
    
    VODACOM.send("0625550100", "Test SMS");

    return 0;
}