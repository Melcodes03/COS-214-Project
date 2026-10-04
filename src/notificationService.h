#ifndef notificationService_H
#define notificationService_H

#include "Observer.h"
#include "WorkItemState.h"
#include "WorkItem.h"

//CONCRETE OBSERVER

class notificationService : public Observer{
    private:
        WorkItemState* observerState;

    public:
        notificationService(WorkItem* subject);
        virtual ~notificationService();
        void setSubject(WorkItem*);
        virtual void update(WorkItemState* newState) override;
};

#endif