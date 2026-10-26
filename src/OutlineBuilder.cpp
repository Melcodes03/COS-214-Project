#include "OutlineBuilder.h"

OutlineBuilder::OutlineBuilder(const std::string& processName)
    : text(processName + "\n"), depth(1), lastWasTask(false) {}

std::string OutlineBuilder::indent() const {
    return std::string(depth * 2, ' ');
}

WorkflowBuilder& OutlineBuilder::beginStage(const std::string& id, const std::string& name) {
    text += indent() + "[" + id + "] " + name + "\n";
    depth++;
    lastWasTask = false;
    return *this;
}

WorkflowBuilder& OutlineBuilder::addTask(const std::string& id, const std::string& name, const std::string& role) {
    text += indent() + "- " + id + " " + name + " (" + role + ")\n";
    lastWasTask = true;
    return *this;
}

//Adds a label to the end of the task line written last (before its newline).
void OutlineBuilder::decorate(const std::string& label) {
    if (lastWasTask) {
        text.insert(text.size() - 1, " [" + label + "]");
    }
}

WorkflowBuilder& OutlineBuilder::withPriority(int priority) {
    decorate("priority " + std::to_string(priority));
    return *this;
}

WorkflowBuilder& OutlineBuilder::withValidation(ValidationDecorator::Rule) {
    decorate("validated");
    return *this;
}

WorkflowBuilder& OutlineBuilder::withAudit() {
    decorate("audited");
    return *this;
}

WorkflowBuilder& OutlineBuilder::endStage() {
    if (depth > 1) {
        depth--;
    }
    lastWasTask = false;
    return *this;
}

std::string OutlineBuilder::getResult() const {
    return text;
}
