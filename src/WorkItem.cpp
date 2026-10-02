#include "WorkItem.h"
#include "DepthFirstIterator.h"

WorkItem::WorkItem(const std::string& id, const std::string& name) : id(id), name(name){}

WorkItem::~WorkItem(){}

std::string WorkItem::getId() const {
    return id;
}

std::string WorkItem::getName() const {
    return name;
}

bool WorkItem::add(WorkItem*){
    return false;
}

WorkItem* WorkItem::getChild(int) const{
    return nullptr;
}

int WorkItem::getChildCount() const {
    return 0;
}

WorkItemIterator* WorkItem::createIterator(){
    return new DepthFirstIterator(this);
}