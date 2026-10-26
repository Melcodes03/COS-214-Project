#include "Participant.h"
#include "WorkItem.h"
#include "WorkerCommand.h"
#include "CommandHistory.h"

Participant::Participant(const std::string& name, const std::string& role, int capacity)
    : name(name), role(role), activeTasks(0), capacity(capacity), available(true){}

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
    bool done = history.execute(new WorkerCommand(this, item, &WorkItem::complete,
                                             name + " completes " + item->getName(), true));
    
    if (done){
        remove(item);
    }
    
    return done;
}

bool Participant::decline(WorkItem* item, CommandHistory& history) {
    if (item == nullptr) return false;
    return history.execute(new WorkerCommand(this, item, &WorkItem::reject,
                                             name + " declines " + item->getName(), true));
}

int Participant::getCapacity(){
    return capacity;
}

bool Participant::isAvailable(){
    available = getCapacity() > activeTasks;     //rechecking
    return available;
}

void Participant::setAvailable(bool n){
    available = n;
}

bool Participant::canTakeWork(){
    return isAvailable();
}

void Participant::receive(WorkItem* item){
    if (canTakeWork()){
        worklist.push_back(item);
    }
}

void Participant::remove(WorkItem* item){
    for (vector<WorkItem*>::iterator current = worklist.begin();
         current != worklist.end(); ++current) {
        if (*current == item) {
            worklist.erase(current);
            return;
        }
    }
}

vector<WorkItem*>& Participant::getWorklist(){
    return worklist;
}