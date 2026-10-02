#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkItem.h"

WorkflowInstance::WorkflowInstance(const std::string& instanceId, const WorkflowDefinition* def) : instanceId(instanceId), definition(def), root(def->getRoot()->clone()){}

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