#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string> 

class WorkItem;
class WorkflowDefinition;

/// One running case of a WorkflowDefinition.
/// Prototype: Client. The constructor asks the definition's root for clone(),
/// so each instance owns an independent copy of the whole work tree.
class WorkflowInstance {
    private: 
        std::string instanceId;
        const WorkflowDefinition* definition; 
        WorkItem* root;

    public: 
        WorkflowInstance(const std::string& instanceId, const WorkflowDefinition* def);
        ~WorkflowInstance();

        //owns its cloned tree, so copying is disabled
        WorkflowInstance(const WorkflowInstance&) = delete;
        WorkflowInstance& operator=(const WorkflowInstance&) = delete;

        std::string getId() const;
        const WorkflowDefinition* getDefinition() const;
        WorkItem* getRoot();
};

#endif //WORKFLOWINSTANCE_H