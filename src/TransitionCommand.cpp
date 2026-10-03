#include "TransitionCommand.h"
#include "WorkItemMemento.h"

TransitionCommand::TransitionCommand(WorkItem* target, Transition transition, const std::string& name)
    : target(target), transition(transition), name(name), before(nullptr){}

TransitionCommand::~TransitionCommand(){
    delete before;
}

bool TransitionCommand::execute(){
    delete before;
    before = target->createMemento();

    bool ok = (target->*transition)();
    if (!ok){
        delete before;     //refused by the State/Decorator guard: nothing changed, nothing to undo
        before = nullptr;
    }
    return ok;
}

void TransitionCommand::undo(){
    if (before == nullptr){
        return;
    }
    target->restore(*before);
    delete before;
    before = nullptr;
}

std::string TransitionCommand::getName() const {
    return name;
}
