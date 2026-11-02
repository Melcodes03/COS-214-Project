#include "StalledWorkVisitor.h"
#include <sstream>
#include "Task.h"

StalledWorkVisitor::StalledWorkVisitor(const std::map<std::string, int>& changedAt, int step, int limit)
    : changedAt(changedAt), step(step), limit(limit) {}

void StalledWorkVisitor::visitTask(Task& task) {
    std::string state = task.getStateName();
    if (state == "Created" || state == "Completed" || state == "Cancelled") return;   //not open
    std::map<std::string, int>::const_iterator found = changedAt.find(task.getId());
    int idle = step - (found == changedAt.end() ? 0 : found->second);
    if (state == "Escalated" || idle >= limit) {
        Stalled s = { task.getId(), task.getName(), state, idle };
        stalled.push_back(s);
    }
}

void StalledWorkVisitor::visitStage(Stage&) {}

const std::vector<StalledWorkVisitor::Stalled>& StalledWorkVisitor::getStalled() const { return stalled; }

std::string StalledWorkVisitor::report() const {
    std::ostringstream out;
    out << "=== Stalled work (no change for " << limit << "+ steps, or escalated) ===\n";
    if (stalled.empty()) out << "Nothing is stalled.\n";
    for (size_t i = 0; i < stalled.size(); ++i) {
        out << stalled[i].id << " " << stalled[i].name << ": " << stalled[i].state
            << ", idle for " << stalled[i].idleSteps << " step(s)\n";
    }
    return out.str();
}
