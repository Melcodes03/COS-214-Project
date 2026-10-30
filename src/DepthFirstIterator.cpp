#include "DepthFirstIterator.h"
#include "WorkItem.h"

DepthFirstIterator::DepthFirstIterator(WorkItem* root) : root(root) {
    first();
}

void DepthFirstIterator::first(){
    stack.clear();
    if(root != nullptr){
        stack.push_back(root);
    }
}

void DepthFirstIterator::next(){
    if (isDone()){
        return;
    }

    WorkItem* current = stack.back();
    stack.pop_back();

    for (int i = current->getChildCount() - 1; i >= 0; i--){
        stack.push_back(current->getChild(i));
    }
}

bool DepthFirstIterator::isDone() const{
    return stack.empty();
}

WorkItem* DepthFirstIterator::currentItem() const{
    if (isDone()){
        return nullptr;
    }
    return stack.back();
}