#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include "WorkItem.h"   //for DataMap
#include <string> 
#include <vector>

class WorkflowDefinition;

/**
 * @brief One running case of a workflow definition.
 *
 * An instance owns its own copy of the definition's tree (made with clone(), the
 * Prototype pattern), its own data and its own status, so a change to one instance
 * never touches another instance or the definition.
 *
 * The engine moves a workflow forward with advance(). Call it after every action
 * a worker takes. It opens the next work, picks branches from the instance data,
 * fails the workflow when a task is cancelled, and completes it at the end.
 * Prototype: Client. The constructor asks the definition's root for clone(), so each instance owns an independent copy of the whole work tree.
 */
class WorkflowInstance {
    public:
        //Where the whole workflow currently is
        enum Status { RUNNING, COMPLETED, FAILED };

    private: 
        std::string instanceId;
        const WorkflowDefinition* definition; 
        WorkItem* root;
        Status status;
        DataMap data;   //this instance's own data: routing rules read it

        bool hasCancelledItem();

    public: 
        ///Creates a RUNNING instance with its own copy of the definition's tree and no data.
        WorkflowInstance(const std::string& instanceId, const WorkflowDefinition* def);
        ~WorkflowInstance();

        //owns its cloned tree, so copying is disabled
        WorkflowInstance(const WorkflowInstance&) = delete;
        WorkflowInstance& operator=(const WorkflowInstance&) = delete;

        std::string getId() const;
        const WorkflowDefinition* getDefinition() const;
        WorkItem* getRoot();

        /**
         * @brief Moves the workflow as far forward as it can without a worker, then stops.
         *
         * Safe to call any number of times. The rules, in order:
         *  1. A finished (COMPLETED or FAILED) workflow does not change.
         *  2. If any item was cancelled, the whole workflow FAILS (the cancel rule).
         *  3. Otherwise the root stage is advanced: finished work opens the next work,
         *     and branches are chosen from the instance data.
         *  4. When the root stage is Completed, the workflow is COMPLETED.
         *
         * @return the status after advancing
         */
        Status advance();

        Status getStatus() const;
        ///"Running", "Completed" or "Failed".
        std::string getStatusName() const;

        ///Stores data for this instance only, for example setData("amount", "85000").
        void setData(const std::string& key, const std::string& value);
        bool hasData(const std::string& key) const;
        /// @return the stored value, or @p fallback if the key is not set
        std::string getData(const std::string& key, const std::string& fallback = "") const;

        /**
         * @brief The work that is ready to be picked up (state Available).
         *
         * Empty once the workflow has completed or failed. These are the items a
         * WorkAllocator can allocate and a participant can work on.
         */
        std::vector<WorkItem*> getAvailableWork();
};

#endif //WORKFLOWINSTANCE_H
