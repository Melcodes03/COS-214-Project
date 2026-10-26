#ifndef LEVELHANDLER_H
#define LEVELHANDLER_H

#include "AssignmentHandler.h"
#include <string>

class AssignmentStrategy;

/// Chain of Responsibility: ConcreteHandler. One level of workers who share a role.
///
/// A level takes a task when the task's priority is no higher than the level's
/// maximum and at least one worker with the level's role is available. It then
/// asks its own AssignmentStrategy (Strategy pattern) which of those workers gets
/// it, so each level can choose workers in its own way. Otherwise it passes the
/// task on to the next level.
///
/// Higher levels have a higher maximum priority, so a task that is escalated or
/// declined further down is always accepted by the levels above it.
class LevelHandler : public AssignmentHandler {
    private:
        std::string role;
        int maxPriority;
        AssignmentStrategy* strategy;   ///< owned

    public:
        /// @param role         the role of the workers in this level
        /// @param maxPriority  the highest task priority this level deals with
        /// @param strategy     how this level picks a worker; takes ownership (must not be null)
        LevelHandler(const std::string& role, int maxPriority, AssignmentStrategy* strategy);
        ~LevelHandler() override;

        Participant* handle(WorkItem& item, const std::vector<Participant*>& candidates) override;
        bool covers(const Participant& worker) const override;

        std::string getRole() const;
        int getMaxPriority() const;

    private:
        /// Is @p worker in this level and able to take more work right now?
        bool isFree(const Participant& worker) const;
};

#endif //LEVELHANDLER_H
