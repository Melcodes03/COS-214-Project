#ifndef WORKITEMSTATE_H
#define WORKITEMSTATE_H

#include <string>

class WorkItem;

//State: abstract State. Every operation is invalid by default.
/**
 * @brief State: the abstract state of a work item's lifecycle.
 *
 * Every operation is refused (returns false) unless a concrete state overrides it, so an
 * action that is invalid in a state is rejected without any if/else chain. A state moves
 * the item on by calling WorkItem::setState(). Concrete states hold no data, so each one
 * exists once and is shared by every work item.
 */
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