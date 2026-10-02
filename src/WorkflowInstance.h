#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string> 

class WorkItem;
class WorkflowDefinition;

class WorkflowInstance {
    private: 
        std::string instanceId;
        const WorkflowDefinition* definition; 
        WorkItem* root;

    public: 
        WorkflowInstance(const std::string& instanceId, const WorkflowDefinition* def);
        ~WorkflowInstance();

        std::string getId() const;
        const WorkflowDefinition* getDefinition() const;
        WorkItem* getRoot();
};

#endif //WORKFLOWINSTANCE_H