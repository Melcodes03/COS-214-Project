#include "EscalationReportVisitor.h"
#include "Task.h"
#include "Stage.h"

//A stage is visited before its children (depth-first), so it records which
//children are its own; each task can then look up the stage it belongs to.
void EscalationReportVisitor::visitStage(Stage& stage) {
    for (int i = 0; i < stage.getChildCount(); i++) {
        stageOf[stage.getChild(i)->getId()] = stage.getName();
    }
}

void EscalationReportVisitor::visitTask(Task& task) {
    std::string state = task.getStateName();
    if (state == "Escalated" || state == "Rejected") {
        std::map<std::string, std::string>::const_iterator parent = stageOf.find(task.getId());
        std::string where = parent == stageOf.end() ? "no stage" : parent->second;
        flagged.push_back(task.getId() + " (" + where + "): " + state);
    }
}

const std::vector<std::string>& EscalationReportVisitor::getFlagged() const { return flagged; }
