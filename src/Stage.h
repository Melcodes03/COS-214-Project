#ifndef STAGE_H
#define STAGE_H

#include "WorkItem.h"
#include <vector> 
#include <functional>

/**
 * @brief Composite: a group of work items (a stage, or a whole process).
 *
 * A Stage owns its children, which can be Tasks or other Stages. It also decides
 * HOW its children are run when the workflow advances:
 *  - sequential (default): children run one after another
 *  - parallel: all children become available together
 *  - choice: exactly one child runs, picked from the instance data
 * Prototype: ConcretePrototype (clone() copies the stage and calls clone() on every child, so the whole subtree is copied).
 */
class Stage : public WorkItem {
    public:
        ///Picks which child runs in a choice stage. Returns the child's index, or -1 for "none".
        typedef std::function<int(const DataMap&)> Selector;

    private: 
        std::vector<WorkItem*> children;
        //how this stage runs its children: set by setSequential/setParallel/setChoice
        bool (Stage::*schedule)(const DataMap&);
        Selector selector;

        bool advanceSequential(const DataMap& data);
        bool advanceParallel(const DataMap& data);
        bool advanceChoice(const DataMap& data);

    public: 
        Stage(const std::string& id, const std::string& name);
        ~Stage() override; 

        bool add(WorkItem* child) override;
        WorkItem* getChild(int index) const override;
        int getChildCount() const override;

        ///Children run one after another: a child opens once the one before it is Completed.
        void setSequential();
        /// All children become available together and the stage completes when all are done.
        void setParallel();
        ///Routing: only the child picked by @p chooser runs. If it picks no child, the stage is cancelled.
        void setChoice(Selector chooser);

        /**
         * @brief Drives this stage through its own lifecycle and runs its children.
         *
         * A stage is moved by the engine, not by a worker: the first time it is reached it
         * goes Created, Available, Assigned, InProgress. It is completed once its children are done.
         *
         * @param data the data of the workflow instance
         * @return true once the stage is Completed
         */
        bool advance(const DataMap& data) override;

        ///Prototype: copies the whole subtree, including how the stage runs its children.
        WorkItem* clone() const override;
        void accept(WorkItemVisitor& visitor) override;   //Visitor: ConcreteElement
        Stage(const Stage&) = delete;
        Stage& operator=(const Stage&) = delete;
};      

#endif //STAGE_H
