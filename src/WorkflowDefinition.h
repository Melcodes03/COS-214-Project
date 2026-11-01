#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>

class WorkItem;

/// The reusable process template. Built by DefinitionBuilder (Builder) and
/// holds the prototype tree that every WorkflowInstance clones (Prototype).

class WorkflowDefinition{
    private:
        std::string name;
        int version; 
        WorkItem* root;

    public: 
        //takes ownership of the root
        WorkflowDefinition(const std::string& name, int version, WorkItem* root);
        ~WorkflowDefinition();

        //owns the root, so copying is disabled (a copy would delete the tree twice)
        WorkflowDefinition(const WorkflowDefinition&) = delete;
        WorkflowDefinition& operator=(const WorkflowDefinition&) = delete;

        std::string getName() const;
        int getVersion() const;
        const WorkItem* getRoot() const;
};

#endif // WORKFLOWDEFINITION_H