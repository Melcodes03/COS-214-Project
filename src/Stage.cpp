#include "Stage.h"
#include "WorkItemVisitor.h"

Stage::Stage(const std::string& id, const std::string& name)
    : WorkItem(id, name), schedule(&Stage::advanceSequential){}

Stage::~Stage(){
    for (int i=0; i < (int)children.size(); i++){
        delete children[i];
    }
}

bool Stage::add(WorkItem* child){
    if (child == nullptr){
        return false;
    }
    children.push_back(child);
    return true;
}

WorkItem* Stage::getChild(int index) const {
    if (index < 0 || index >= getChildCount()){
        return nullptr;
    }
    return children[index];
}

int Stage::getChildCount() const {
    return (int)children.size();
}

void Stage::setSequential() { schedule = &Stage::advanceSequential; }
void Stage::setParallel() { schedule = &Stage::advanceParallel; }
void Stage::setChoice(Selector chooser) {
    selector = chooser;
    schedule = &Stage::advanceChoice;
}

//Each child is advanced in order and the next one waits until the one before it is Completed
bool Stage::advanceSequential(const DataMap& data) {
    for (int i = 0; i < (int)children.size(); i++) {
        if (!children[i]->advance(data)) {
            return false;
        }
    }
    return true;
}

//Every child is advanced at once (fork); the stage is done when all of them are (join)
bool Stage::advanceParallel(const DataMap& data) {
    bool allDone = true;
    for (int i = 0; i < (int)children.size(); i++) {
        if (!children[i]->advance(data)) {
            allDone = false;
        }
    }
    return allDone;
}

//Only the chosen child runs. The other children stay Created and are ignored
bool Stage::advanceChoice(const DataMap& data) {
    WorkItem* chosen = getChild(selector ? selector(data) : -1);
    if (chosen == nullptr) {
        cancel();   //no branch matches the data: the workflow cannot continue
        return false;
    }
    return chosen->advance(data);
}

bool Stage::advance(const DataMap& data) {
    if (isCompleted() || isCancelled()) {
        return isCompleted();
    }
    if (makeAvailable()) {   //only true the first time the engine reaches this stage
        assign();  //a stage is driven by the engine, so it assigns and starts itself
        start();
    }
    if ((this->*schedule)(data)) {
        complete();
    }
    return isCompleted();
}

WorkItem* Stage::clone() const{
    Stage* copy = new Stage(id, name);
    copy->requiredRole = requiredRole;
    copy->maxRetries = maxRetries;
    copy->schedule = schedule;
    copy->selector = selector;
    for (int i =0; i < (int)children.size(); i++){
        copy->add(children[i]->clone());
    }
    return copy;
}


//Children are not visited here: the Iterator walks the tree (see WorkItemVisitor::visitAll)
void Stage::accept(WorkItemVisitor& visitor) {
    visitor.visitStage(*this);
}
