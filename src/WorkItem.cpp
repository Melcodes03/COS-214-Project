#include "WorkItem.h"
#include "DepthFirstIterator.h"
#include "CreatedState.h"

WorkItem::WorkItem(const std::string& id, const std::string& name)
    : id(id), name(name), state(CreatedState::instance()), retryCount(0), maxRetries(3){}

WorkItem::WorkItem(const WorkItem& other)
    : id(other.id), name(other.name), state(CreatedState::instance()),
      retryCount(0), maxRetries(other.maxRetries){}

WorkItem& WorkItem::operator=(const WorkItem& other) {
    if (this != &other) {
        id = other.id;
        name = other.name;
        state = CreatedState::instance();
        retryCount = 0;
        maxRetries = other.maxRetries;
    }
    return *this;
}

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

//State section
bool WorkItem::makeAvailable() { return state->makeAvailable(*this); }
bool WorkItem::assign() { return state->assign(*this); }
bool WorkItem::start() { return state->start(*this); }
bool WorkItem::complete() { return state->complete(*this); }
bool WorkItem::reject() { return state->reject(*this); }
bool WorkItem::escalate() { return state->escalate(*this); }
bool WorkItem::cancel() { return state->cancel(*this); }

std::string WorkItem::getStateName() const { return state->getName(); }

void WorkItem::setState(WorkItemState* newState) { state = newState; }

//Retry limit - guard for state
int WorkItem::getRetryCount() const { return retryCount; }
int WorkItem::getMaxRetries() const { return maxRetries; }
void WorkItem::setMaxRetries(int max) { maxRetries = max; }
bool WorkItem::retriesRemaining() const { return getRetryCount() < getMaxRetries(); }
void WorkItem::useRetry() { retryCount++; }