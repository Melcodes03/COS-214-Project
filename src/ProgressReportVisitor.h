#ifndef PROGRESSREPORTVISITOR_H
#define PROGRESSREPORTVISITOR_H

#include "WorkItemVisitor.h"
#include <map>
#include <string>

//Visitor: ConcreteVisitor. Monitoring view of a running workflow:
//how many tasks are in each state and how far along the work is.
class ProgressReportVisitor : public WorkItemVisitor {
    private:
        int tasks;
        int stages;
        std::map<std::string, int> tasksPerState;   //e.g. "Completed" -> 3

    public:
        ProgressReportVisitor();

        void visitTask(Task& task) override;
        void visitStage(Stage& stage) override;

        int getTaskCount() const;
        int getStageCount() const;
        int countInState(const std::string& stateName) const;
        int percentComplete() const;   //completed tasks / all tasks, 0 if no tasks
        std::string summary() const;
};

#endif //PROGRESSREPORTVISITOR_H
