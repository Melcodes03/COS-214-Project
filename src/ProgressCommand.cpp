#include "ProgressCommand.h"
#include "WorkflowInstance.h"
#include "WorkItem.h"
#include "WorkItemMemento.h"

ProgressCommand::ProgressCommand(Command* inner, WorkflowInstance* instance)
    : inner(inner), instance(instance), before(nullptr), done(false){}

ProgressCommand::~ProgressCommand(){
    delete inner;
    delete before;
}

bool ProgressCommand::execute(){
    done = false;
    if (inner == nullptr || instance == nullptr){
        return false;
    }
    delete before;
    before = instance->getRoot()->createMemento();
    if (!inner->execute()){
        delete before;   //refused: nothing changed, nothing to undo
        before = nullptr;
        return false;
    }
    instance->advance();   //the engine opens whatever the action unlocked
    done = true;
    return true;
}

void ProgressCommand::undo(){
    if (!done){
        return;
    }
    inner->undo();                            //participants, allocator and the task itself
    instance->getRoot()->restore(*before);    //and whatever opened or closed after it
    instance->recomputeStatus();
    done = false;
}

std::string ProgressCommand::getName() const {
    return inner->getName();
}
