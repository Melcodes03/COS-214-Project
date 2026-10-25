#include "WorkerCommand.h"
#include "Participant.h"

WorkerCommand::WorkerCommand(Participant* worker, WorkItem* target,
                             TransitionCommand::Transition transition,
                             const std::string& name, bool releasesWork)
    : worker(worker), inner(new TransitionCommand(target, transition, name)),
      releasesWork(releasesWork), released(false){}

WorkerCommand::~WorkerCommand(){
    delete inner;
}

bool WorkerCommand::execute(){
    released = false;
    if (!inner->execute()){
        return false;   //refused: item unchanged, workload untouched
    }
    if (releasesWork){
        int before = worker->getActiveTasks();
        worker->finishTask();                    //finishTask never goes below zero,
        released = (worker->getActiveTasks() != before);   //so check it really changed
    }
    return true;
}

void WorkerCommand::undo(){
    inner->undo();
    if (released){
        worker->addTask();   //give back exactly what execute() took
        released = false;
    }
}

std::string WorkerCommand::getName() const {
    return inner->getName();
}
