#include "WorkerCommand.h"
#include "Participant.h"
#include <vector>

//position of item in list, or -1
static int indexOf(const std::vector<WorkItem*>& list, const WorkItem* item){
    for (int i = 0; i < (int)list.size(); i++){
        if (list[i] == item) return i;
    }
    return -1;
}

WorkerCommand::WorkerCommand(Participant* worker, WorkItem* target,
                             TransitionCommand::Transition transition,
                             const std::string& name, bool releasesWork)
    : worker(worker), target(target), inner(new TransitionCommand(target, transition, name)),
      releasesWork(releasesWork), released(false), listIndex(-1), removedFromList(false){}

WorkerCommand::~WorkerCommand(){
    delete inner;
}

bool WorkerCommand::execute(){
    released = false;
    removedFromList = false;
    listIndex = indexOf(worker->getWorklist(), target);   //remember where it was, for undo
    if (!inner->execute()){
        return false;   //refused: item unchanged, workload and worklist untouched
    }
    if (releasesWork){
        int before = worker->getActiveTasks();
        worker->finishTask();                    //finishTask never goes below zero,
        released = (worker->getActiveTasks() != before);   //so check it really changed
        worker->remove(target);                  //it is no longer on the worker's list
        removedFromList = (listIndex >= 0);
    }
    return true;
}

void WorkerCommand::undo(){
    inner->undo();
    if (released){
        worker->addTask();   //give back exactly what execute() took
        released = false;
    }
    if (removedFromList){
        std::vector<WorkItem*>& list = worker->getWorklist();
        if (indexOf(list, target) < 0){
            int at = listIndex < (int)list.size() ? listIndex : (int)list.size();
            list.insert(list.begin() + at, target);   //same position as before
        }
        removedFromList = false;
    }
}

std::string WorkerCommand::getName() const {
    return inner->getName();
}
