#include "WorkItemMemento.h"

WorkItemMemento::WorkItemMemento(WorkItemState* state, int retryCount)
    : state(state), retryCount(retryCount){}

WorkItemMemento::~WorkItemMemento(){
    for (int i = 0; i < (int)children.size(); i++){
        delete children[i];
    }
}

void WorkItemMemento::addChild(WorkItemMemento* child){
    children.push_back(child);
}
