#ifndef STALLEDWORKVISITOR_H
#define STALLEDWORKVISITOR_H

#include <map>
#include <string>
#include <vector>
#include "WorkItemVisitor.h"

/// @brief Visitor pattern: ConcreteVisitor. Finds work that is stuck.
///
/// Walks the tree (with the Iterator, through visitAll) and collects open tasks whose state
/// has not changed for at least @p limit steps of their case, plus escalated work with nobody
/// holding it. The Reviewer re-offers what it finds; the "Stalled work" report lists it.
class StalledWorkVisitor : public WorkItemVisitor {
    public:
        struct Stalled {
            std::string id;
            std::string name;
            std::string state;
            int idleSteps;
        };
        /// @param changedAt task id -> the case step at which its state last changed
        StalledWorkVisitor(const std::map<std::string, int>& changedAt, int step, int limit);
        void visitTask(Task& task) override;
        void visitStage(Stage& stage) override;
        const std::vector<Stalled>& getStalled() const;
        std::string report() const;
    private:
        const std::map<std::string, int>& changedAt;
        int step;
        int limit;
        std::vector<Stalled> stalled;
};

#endif //STALLEDWORKVISITOR_H
