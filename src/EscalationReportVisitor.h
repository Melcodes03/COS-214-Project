#ifndef ESCALATIONREPORTVISITOR_H
#define ESCALATIONREPORTVISITOR_H

#include "WorkItemVisitor.h"
#include <map>
#include <string>
#include <vector>

//Visitor: ConcreteVisitor. Lists the work that needs a manager's attention:
//tasks that were escalated or rejected, and the stage each one sits in.
class EscalationReportVisitor : public WorkItemVisitor {
    private:
        std::map<std::string, std::string> stageOf;   //child id -> name of the stage holding it
        std::vector<std::string> flagged;   //e.g. "A2 (Stage A): Escalated"

    public:
        void visitTask(Task& task) override;
        void visitStage(Stage& stage) override;

        const std::vector<std::string>& getFlagged() const;
};

#endif //ESCALATIONREPORTVISITOR_H
