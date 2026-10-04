#ifndef notificationService_H
#define notificationService_H

#include "Observer.h"
#include "WorkItemState.h"
#include "WorkItem.h"
#include "CommunicationAdapter.h"

//CONCRETE OBSERVER

class notificationService : public Observer{
    private:
        WorkItemState* observerState;
        CommunicationAdapter* channel;     //sms or email
        std::string recipient;

    public:
        notificationService(WorkItem* subject, CommunicationAdapter* channel, std::string recipient);
        virtual ~notificationService();
        void setSubject(WorkItem*);
        WorkItemState* getObservedState();
        virtual void update(WorkItemState* newState, std::string oldState) override;
};

#endif