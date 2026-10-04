#include "WorkItem.h"
#include "DepthFirstIterator.h"
#include "CreatedState.h"
#include "WorkItemMemento.h"
#include "Observer.h"

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
bool WorkItem::makeAvailable() { 
    std::string oldState = this->getStateName();
    bool ok = state->makeAvailable(*this);
    if (ok) notify(oldState);
    return ok; 
 }
bool WorkItem::assign() { 
    std::string oldState = this->getStateName();
    bool ok = state->assign(*this);
    if (ok) notify(oldState);
    return ok; 
 }
bool WorkItem::start() { 
    std::string oldState = this->getStateName();
    bool ok = state->start(*this);
    if (ok) notify(oldState);
    return ok; 
 }
bool WorkItem::complete() { 
    std::string oldState = this->getStateName();
    bool ok = state->complete(*this);
    if (ok) notify(oldState);
    return ok; 
}
bool WorkItem::reject() { 
    std::string oldState = this->getStateName();
    bool ok = state->reject(*this);
    if (ok) notify(oldState);
    return ok; 
}
bool WorkItem::escalate() {
    std::string oldState = this->getStateName();
    bool ok = state->escalate(*this);
    if (ok) notify(oldState);
    return ok; 
}
bool WorkItem::cancel() {
    std::string oldState = this->getStateName();
    bool ok = state->cancel(*this);
    if (ok) notify(oldState);
    return ok; 
}

std::string WorkItem::getStateName() const { return state->getName(); }

WorkItemState* WorkItem::getState() const { return state; }

void WorkItem::setState(WorkItemState* newState) { state = newState; }

//Retry limit - guard for state
int WorkItem::getRetryCount() const { return retryCount; }
int WorkItem::getMaxRetries() const { return maxRetries; }
void WorkItem::setMaxRetries(int max) { maxRetries = max; }
void WorkItem::setRetryCount(int count) { retryCount = count; }
bool WorkItem::retriesRemaining() const { return getRetryCount() < getMaxRetries(); }
void WorkItem::useRetry() { retryCount++; }

//Priority
int WorkItem::getPriority() const { return 0; }

//Memento section
WorkItemMemento* WorkItem::createMemento() const {
    WorkItemMemento* memento = new WorkItemMemento(getState(), getRetryCount());
    for (int i = 0; i < getChildCount(); i++) {
        memento->addChild(getChild(i)->createMemento());   //deep: every descendant gets its own snapshot
    }
    return memento;
}

bool WorkItem::fits(const WorkItemMemento& memento) const {
    if (getChildCount() != (int)memento.children.size()) {
        return false;
    }
    for (int i = 0; i < getChildCount(); i++) {
        if (!getChild(i)->fits(*memento.children[i])) {
            return false;
        }
    }
    return true;
}

bool WorkItem::restore(const WorkItemMemento& memento) {
    if (!fits(memento)) {
        return false;   //checked first, so a failed restore never leaves the tree half-restored
    }
    setState(memento.state);
    setRetryCount(memento.retryCount);
    for (int i = 0; i < getChildCount(); i++) {
        getChild(i)->restore(*memento.children[i]);
    }
    return true;
}

//observer related functions
void WorkItem::attach(Observer* observer){  
    if(observer){
        observers.push_back(observer);
    }
}

void WorkItem::detach(Observer* observer){
    auto current = observers.begin();
    while (current != observers.end()) {
        if (*current == observer) {
            current = observers.erase(current);
        } else {
            ++current;
        }
    }
}

void WorkItem::notify(std::string oldState){
    if (observers.size() > 0){
        for (auto observer : observers){
            observer->update(state, oldState);
        }
    }
}
