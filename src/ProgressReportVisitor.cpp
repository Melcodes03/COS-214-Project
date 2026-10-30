#include "ProgressReportVisitor.h"
#include "Task.h"
#include "Stage.h"
#include "CompletedState.h"

ProgressReportVisitor::ProgressReportVisitor() : tasks(0), stages(0), completed(0) {}

void ProgressReportVisitor::reset() {
    tasks = 0;
    stages = 0;
    completed = 0;
    tasksPerState.clear();
    stageOf.clear();
    stageOrder.clear();
    stageTasks.clear();
    stageCompleted.clear();
}

//A stage is visited before its children (depth-first), so it records which
//children are its own; each task can then add itself to the right stage.
void ProgressReportVisitor::visitStage(Stage& stage) {
    stages++;
    for (int i = 0; i < stage.getChildCount(); i++) {
        stageOf[stage.getChild(i)->getId()] = stage.getName();
    }
}

void ProgressReportVisitor::visitTask(Task& task) {
    tasks++;
    tasksPerState[task.getStateName()]++;   //name is only a label for display
    bool done = task.getState() == CompletedState::instance();   //state object, not a string check
    if (done) {
        completed++;
    }

    std::map<std::string, std::string>::const_iterator parent = stageOf.find(task.getId());
    if (parent == stageOf.end()) {
        return;   //a task with no stage still counts overall
    }
    const std::string& stage = parent->second;
    if (stageTasks.find(stage) == stageTasks.end()) {
        stageOrder.push_back(stage);
        stageCompleted[stage] = 0;
    }
    stageTasks[stage]++;
    if (done) {
        stageCompleted[stage]++;
    }
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
    return completed * 100 / tasks;
}

std::string ProgressReportVisitor::summary() const {
    return std::to_string(stages) + " stages, " + std::to_string(tasks) + " tasks, "
         + std::to_string(percentComplete()) + "% complete";
}

std::vector<std::string> ProgressReportVisitor::getStageNames() const { return stageOrder; }

int ProgressReportVisitor::stageTaskCount(const std::string& stageName) const {
    std::map<std::string, int>::const_iterator found = stageTasks.find(stageName);
    return found == stageTasks.end() ? 0 : found->second;
}

int ProgressReportVisitor::stageCompletedCount(const std::string& stageName) const {
    std::map<std::string, int>::const_iterator found = stageCompleted.find(stageName);
    return found == stageCompleted.end() ? 0 : found->second;
}

int ProgressReportVisitor::stagePercentComplete(const std::string& stageName) const {
    int total = stageTaskCount(stageName);
    return total == 0 ? 0 : stageCompletedCount(stageName) * 100 / total;
}

std::string ProgressReportVisitor::stageSummary() const {
    std::string out;
    for (int i = 0; i < (int)stageOrder.size(); i++) {
        const std::string& name = stageOrder[i];
        out += name + ": " + std::to_string(stageCompletedCount(name)) + "/"
             + std::to_string(stageTaskCount(name)) + " tasks ("
             + std::to_string(stagePercentComplete(name)) + "%)\n";
    }
    return out;
}
