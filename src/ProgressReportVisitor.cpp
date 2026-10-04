#include "ProgressReportVisitor.h"
#include "Task.h"
#include "Stage.h"

ProgressReportVisitor::ProgressReportVisitor() : tasks(0), stages(0) {}

void ProgressReportVisitor::visitTask(Task& task) {
    tasks++;
    tasksPerState[task.getStateName()]++;
}

void ProgressReportVisitor::visitStage(Stage&) {
    stages++;   //stages group work; only tasks count towards progress
}

int ProgressReportVisitor::getTaskCount() const { return tasks; }
int ProgressReportVisitor::getStageCount() const { return stages; }

int ProgressReportVisitor::countInState(const std::string& stateName) const {
    std::map<std::string, int>::const_iterator found = tasksPerState.find(stateName);
    return found == tasksPerState.end() ? 0 : found->second;
}

int ProgressReportVisitor::percentComplete() const {
    if (tasks == 0) {
        return 0;
    }
    return countInState("Completed") * 100 / tasks;
}

std::string ProgressReportVisitor::summary() const {
    return std::to_string(stages) + " stages, " + std::to_string(tasks) + " tasks, "
         + std::to_string(percentComplete()) + "% complete";
}
