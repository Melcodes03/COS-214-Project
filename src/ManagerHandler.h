#ifndef MANAGERHANDLER_H
#define MANAGERHANDLER_H

#include "AssignmentHandler.h"

/// Chain of Responsibility: ConcreteHandler. The last link in the chain.
///
/// Always gives the task to the manager, so a task that no level below could
/// take is never left without an owner.
class ManagerHandler : public AssignmentHandler {
    private:
        Participant* manager;   ///< not owned

    public:
        /// @param manager the person who receives anything the levels below could not take
        explicit ManagerHandler(Participant* manager);

        Participant* handle(WorkItem& item, const std::vector<Participant*>& candidates) override;
        bool covers(const Participant& worker) const override;
};

#endif //MANAGERHANDLER_H
