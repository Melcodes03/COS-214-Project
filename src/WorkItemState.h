#ifndef WORKITEMSTATE_H
#define WORKITEMSTATE_H

#include <string>

class WorkItem;

//State: abstract State. Every operation is invalid by default.
class WorkItemState {
    public:
        virtual ~WorkItemState() {}

        virtual bool makeAvailable(WorkItem& item);
        virtual bool assign(WorkItem& item);
        virtual bool start(WorkItem& item);
        virtual bool complete(WorkItem& item);
        virtual bool reject(WorkItem& item);
        virtual bool escalate(WorkItem& item);
        virtual bool cancel(WorkItem& item);

        virtual std::string getName() const = 0;
};

#endif //WORKITEMSTATE_H