#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkItem.h"
#include "WorkItemIterator.h"

WorkflowInstance::WorkflowInstance(const std::string& instanceId, const WorkflowDefinition* def)
    : instanceId(instanceId), definition(def), root(def->getRoot()->clone()), status(RUNNING){}

WorkflowInstance::~WorkflowInstance(){
    delete root;
}

std::string WorkflowInstance::getId() const{
    return instanceId;
}

const WorkflowDefinition* WorkflowInstance::getDefinition() const{
    return definition;
}

WorkItem* WorkflowInstance::getRoot(){
    return root;
}

//Walks the whole tree with the Iterator, so it works for any nesting depth
bool WorkflowInstance::hasCancelledItem() {
    bool found = false;
    WorkItemIterator* it = root->createIterator();
    for (it->first(); !it->isDone() && !found; it->next()) {
        found = it->currentItem()->isCancelled();
    }
    delete it;
    return found;
}

WorkflowInstance::Status WorkflowInstance::advance() {
    if (status != RUNNING) {
        return status;
    }
    if (hasCancelledItem()) {
        status = FAILED;
    } else if (root->advance(data)) {
        status = COMPLETED;
    } else if (hasCancelledItem()) {   //advancing can cancel too, for example when no branch matches
        status = FAILED;
    }
    return status;
}

WorkflowInstance::Status WorkflowInstance::getStatus() const {
    return status;
}

std::string WorkflowInstance::getStatusName() const {
    if (status == COMPLETED) return "Completed";
    if (status == FAILED) return "Failed";
    return "Running";
}

void WorkflowInstance::setData(const std::string& key, const std::string& value) {
    data[key] = value;
}

bool WorkflowInstance::hasData(const std::string& key) const {
    return data.find(key) != data.end();
}

std::string WorkflowInstance::getData(const std::string& key, const std::string& fallback) const {
    DataMap::const_iterator found = data.find(key);
    return found == data.end() ? fallback : found->second;
}

std::vector<WorkItem*> WorkflowInstance::getAvailableWork() {
    std::vector<WorkItem*> ready;
    if (status != RUNNING) {
        return ready;
    }
    WorkItemIterator* it = root->createIterator();
    for (it->first(); !it->isDone(); it->next()) {
        if (it->currentItem()->isAvailable()) {
            ready.push_back(it->currentItem());
        }
    }
    delete it;
    return ready;
}
