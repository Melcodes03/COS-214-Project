#include "EscalationReportVisitor.h"
#include "Task.h"
#include "Stage.h"
#include "EscalatedState.h"
#include "RejectedState.h"

//A stage is visited before its children (depth-first), so it records which
//children are its own; each task can then look up the stage it belongs to.
void EscalationReportVisitor::visitStage(Stage& stage) {
    for (int i = 0; i < stage.getChildCount(); i++) {
        stageOf[stage.getChild(i)->getId()] = stage.getName();
    }
}

//States are shared singletons, so comparing the state object itself tells us
//which state the task is in without comparing state names as text.
void EscalationReportVisitor::visitTask(Task& task) {
    WorkItemState* state = task.getState();
    if (state == EscalatedState::instance() || state == RejectedState::instance()) {
        std::map<std::string, std::string>::const_iterator parent = stageOf.find(task.getId());
        std::string where = parent == stageOf.end() ? "no stage" : parent->second;
        flagged.push_back(task.getId() + " (" + where + "): " + task.getStateName());
    }
}

const std::vector<std::string>& EscalationReportVisitor::getFlagged() const { return flagged; }
