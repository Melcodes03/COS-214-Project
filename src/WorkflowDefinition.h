#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>

class WorkItem;

//Basically the process description

class WorkflowDefinition{
    private:
        std::string name;
        int version; 
        WorkItem* root;

    public: 
        //takes ownership of the root
        WorkflowDefinition(const std::string& name, int version, WorkItem* root);
        ~WorkflowDefinition();

        std::string getName() const;
        int getVersion() const;
        const WorkItem* getRoot() const;
};

#endif // WORKFLOWDEFINITION_H