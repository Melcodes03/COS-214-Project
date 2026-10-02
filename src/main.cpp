#include <iostream>
#include <string> 
#include "Task.h"
#include "Stage.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkItemIterator.h"

int main(){
    //building a small template: process -> (Stage A -> A1, A2), Task B
    Stage* process = new Stage("P", "Process");
    Stage* stageA = new Stage("A", "Stage A");

    stageA->add(new Task("A1", "Task A1"));
    stageA->add(new Task("A2", "Task A2"));
    process->add(stageA);
    process->add(new Task("B", "Task B"));

    WorkflowDefinition definition("Generic Process", 1, process);
    WorkflowInstance first("I1", &definition);
    WorkflowInstance second("I2", &definition);

    //1. iterator traverses the whole tree
    std::string order = "";
    WorkItemIterator* it = first.getRoot()->createIterator();
    for(it->first(); !it->isDone(); it->next()){
        order += it->currentItem()->getId() + " ";
    }
    delete it;


    //2. leaf rejecting children
    Task leaf("X", "Leaf");
    Task* extra = new Task("Y", "Extra");
    bool leafRejects = !leaf.add(extra);
    delete extra;

    //3. instances have their own copies
    first.getRoot()->add(new Task("C", "Task C"));
    bool independent = second.getRoot()->getChildCount() == 2;

    bool passed = order == "P A A1 A2 B " && leafRejects && independent;
    std::cout << (passed ? "All tests passed." : "Tests failed") << std::endl;
    return passed ? 0 : 1;
}