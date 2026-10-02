#include "Task.h"

Task::Task(const std::string& id, const std::string& name) : WorkItem(id, name){

}

WorkItem* Task::clone() const {
    return new Task(*this);
}