#include "WorkflowDefinition.h"
#include "WorkItem.h"

WorkflowDefinition::WorkflowDefinition(const std::string& name, int version, WorkItem* root) : name(name), version(version), root(root){}

WorkflowDefinition::~WorkflowDefinition(){
    delete root;
}

std::string WorkflowDefinition::getName() const {
    return name;
}

int WorkflowDefinition::getVersion() const{
    return version;
}

const WorkItem* WorkflowDefinition::getRoot() const {
    return root;
}