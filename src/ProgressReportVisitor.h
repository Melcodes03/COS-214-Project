#ifndef PROGRESSREPORTVISITOR_H
#define PROGRESSREPORTVISITOR_H

#include "WorkItemVisitor.h"
#include <map>
#include <string>
#include <vector>

/// Visitor: ConcreteVisitor. Monitoring view of a running workflow.
///
/// Reports how far along the work is, overall and for each stage. A task is
/// counted under the stage that directly holds it. The report can be run again
/// after every action (see reset()), so progress is produced iteratively while
/// the workflow runs, not only once at the end.
class ProgressReportVisitor : public WorkItemVisitor {
    private:
        int tasks;
        int stages;
        int completed;                              //tasks whose state is CompletedState
        std::map<std::string, int> tasksPerState;   //e.g. "Completed" -> 3 (display label only)

        std::map<std::string, std::string> stageOf;     //task id -> name of the stage holding it
        std::vector<std::string> stageOrder;            //stage names in the order they were visited
        std::map<std::string, int> stageTasks;          //stage name -> tasks directly in it
        std::map<std::string, int> stageCompleted;      //stage name -> completed tasks directly in it

    public:
        ProgressReportVisitor();

        void visitTask(Task& task) override;
        void visitStage(Stage& stage) override;

        /// Clears all counts so the same report can be run again on the latest state.
        void reset();

        int getTaskCount() const;
        int getStageCount() const;
        int countInState(const std::string& stateName) const;
        int percentComplete() const;   ///< completed tasks / all tasks, 0 if no tasks
        std::string summary() const;   ///< e.g. "2 stages, 3 tasks, 33% complete"

        /// Stages that directly hold at least one task, in the order visited.
        std::vector<std::string> getStageNames() const;
        int stageTaskCount(const std::string& stageName) const;
        int stageCompletedCount(const std::string& stageName) const;
        int stagePercentComplete(const std::string& stageName) const;   ///< 0 if the stage has no tasks

        /// One line per stage, e.g. "Investigate: 1/2 tasks (50%)".
        std::string stageSummary() const;
};

#endif //PROGRESSREPORTVISITOR_H
