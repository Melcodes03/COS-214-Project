#ifndef MANAGERHANDLER_H
#define MANAGERHANDLER_H

#include "AssignmentHandler.h"

/// Chain of Responsibility: ConcreteHandler. The last link in the chain.
///
/// Gives the task to the manager when they have room. When the manager is full and a
/// ReviewHandler follows, the Reviewer decides instead (so nothing is piled onto a full
/// manager, and nothing silently falls off the end of the chain). With no Reviewer after it,
/// the manager always takes it, as before.
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
