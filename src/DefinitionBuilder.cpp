#include "DefinitionBuilder.h"
#include "Stage.h"
#include "Task.h"
#include "WorkflowDefinition.h"
#include "PriorityDecorator.h"
#include "ValidationDecorator.h"
#include "AuditingDecorator.h"

DefinitionBuilder::DefinitionBuilder(const std::string& name, int version)
    : name(name), version(version), root(new Stage("root", name)), pending(nullptr) {
    openStages.push_back(root);
}

DefinitionBuilder::~DefinitionBuilder() {
    delete pending;   //a task that was never placed
    delete root;      //nullptr after getResult(), so nothing is deleted twice
}

void DefinitionBuilder::placePending() {
    if (pending != nullptr && !openStages.empty()) {
        openStages.back()->add(pending);   //the stage now owns it
        pending = nullptr;
    }
}

WorkflowBuilder& DefinitionBuilder::beginStage(const std::string& id, const std::string& stageName) {
    if (root == nullptr) {
        return *this;   //already built
    }
    placePending();
    Stage* stage = new Stage(id, stageName);
    openStages.back()->add(stage);
    openStages.push_back(stage);
    return *this;
}

WorkflowBuilder& DefinitionBuilder::addTask(const std::string& id, const std::string& taskName, const std::string& role) {
    if (root == nullptr) {
        return *this;
    }
    placePending();
    pending = new Task(id, taskName);
    //role: stored on the work item once WorkItem::setRequiredRole() is merged (Rati's issue)
    (void)role;
    return *this;
}

WorkflowBuilder& DefinitionBuilder::withPriority(int priority) {
    if (pending != nullptr) {
        pending = new PriorityDecorator(pending, priority);
    }
    return *this;
}

WorkflowBuilder& DefinitionBuilder::withValidation(ValidationDecorator::Rule rule) {
    if (pending != nullptr) {
        pending = new ValidationDecorator(pending, rule);
    }
    return *this;
}

WorkflowBuilder& DefinitionBuilder::withAudit() {
    if (pending != nullptr) {
        pending = new AuditingDecorator(pending);
    }
    return *this;
}

WorkflowBuilder& DefinitionBuilder::endStage() {
    if (root == nullptr) {
        return *this;
    }
    placePending();
    if (openStages.size() > 1) {   //never close the root
        openStages.pop_back();
    }
    return *this;
}

WorkflowDefinition* DefinitionBuilder::getResult() {
    if (root == nullptr) {
        return nullptr;   //single-use
    }
    placePending();
    WorkflowDefinition* result = new WorkflowDefinition(name, version, root);   //takes ownership
    root = nullptr;
    openStages.clear();
    return result;
}
