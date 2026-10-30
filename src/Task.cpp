#include "Task.h"
#include "WorkItemVisitor.h"

Task::Task(const std::string& id, const std::string& name) : WorkItem(id, name){

}

WorkItem* Task::clone() const {
    return new Task(*this);
}

void Task::accept(WorkItemVisitor& visitor) {
    visitor.visitTask(*this);
}
