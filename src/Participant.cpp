#include "Participant.h"
#include "WorkItem.h"
#include "WorkerCommand.h"
#include "CommandHistory.h"

Participant::Participant(const std::string& name, const std::string& role)
    : name(name), role(role), activeTasks(0) {}

std::string Participant::getName() const { return name; }
std::string Participant::getRole() const { return role; }
int Participant::getActiveTasks() const { return activeTasks; }
void Participant::addTask() { activeTasks++; }
void Participant::finishTask() { if (activeTasks > 0) activeTasks--; }

bool Participant::start(WorkItem* item, CommandHistory& history) {
    if (item == nullptr) return false;
    return history.execute(new WorkerCommand(this, item, &WorkItem::start,
                                             name + " starts " + item->getName(), false));
}

bool Participant::complete(WorkItem* item, CommandHistory& history) {
    if (item == nullptr) return false;
    return history.execute(new WorkerCommand(this, item, &WorkItem::complete,
                                             name + " completes " + item->getName(), true));
}

bool Participant::decline(WorkItem* item, CommandHistory& history) {
    if (item == nullptr) return false;
    return history.execute(new WorkerCommand(this, item, &WorkItem::reject,
                                             name + " declines " + item->getName(), true));
}
