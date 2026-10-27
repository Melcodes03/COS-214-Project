#ifndef WORKITEM_H
#define WORKITEM_H

#include <string> 
#include <vector>
#include <map>

class Observer;

/// Key/value data that belongs to one workflow instance, for example "amount" -> "85000".
typedef std::map<std::string, std::string> DataMap;

class WorkItemIterator;
class WorkItemState;
class WorkItemMemento;
class WorkItemVisitor;

/**
 * @brief The common interface of all work: a single Task or a whole Stage.
 *
 * WorkItem sits at the centre of the design and plays one role in several patterns:
 *  - Composite: the Component, so a task and a stage are used the same way
 *  - State: the Context, which passes every lifecycle operation to its current state
 *  - Observer: the Subject, which tells attached observers when its state changes
 *  - Memento: the Originator, which can snapshot and restore itself and its children
 *  - Visitor: the Element, which accepts visitors that build reports
 *  - Prototype: the Prototype, which copies itself with clone()
 */
class WorkItem{
    protected: 
        std::string id;
        std::string name;
        WorkItemState* state;
        int retryCount;  //runtime data: reworks used so far (per instance)
        int maxRetries;  //configuration: reworks allowed after a rejection
        std::vector<Observer*> observers;   //stores attached observers
        std::string requiredRole;           //configuration: role needed to work on this item ("" = any)

    private:
        bool fits(const WorkItemMemento& memento) const;

    public: 
        WorkItem(const std::string& id, const std::string& name);
        WorkItem(const WorkItem& other);
        WorkItem& operator=(const WorkItem& other);
        virtual ~WorkItem();

        virtual std::string getId() const;
        virtual std::string getName() const;
            
        //Composite
        virtual bool add(WorkItem* child);
        virtual WorkItem* getChild(int index) const;
        virtual int getChildCount() const;
        /// Prototype: a copy of this item and its subtree. The copy starts in Created with its retry count at 0.
        virtual WorkItem* clone() const = 0;

        //Iterator
        /// Iterator: walks this item and everything inside it. The caller deletes the iterator.
        WorkItemIterator* createIterator(); 

        //State (Context) : Virtual so decorators can intercept them
        virtual bool makeAvailable();
        virtual bool assign();
        virtual bool start();
        virtual bool complete();
        virtual bool reject();
        virtual bool escalate();
        virtual bool cancel();
        virtual std::string getStateName() const;
        virtual WorkItemState* getState() const;   //needed by Memento
        virtual void setState(WorkItemState* newState);
        //Retry limit (used by RejectedState). Virtual so decorators can forward.
        virtual int getRetryCount() const;
        virtual int getMaxRetries() const;
        virtual void setMaxRetries(int max);
        virtual void setRetryCount(int count);     //needed by Memento
        /// @return true while the item may still be sent back for rework (guard on Rejected to Assigned)
        bool retriesRemaining() const;
        /// Counts one rework attempt.
        void useRetry();

        //Memento (Originator): snapshot this item and its whole subtree.
        //restore() returns false and changes nothing if the tree no longer has
        //the same shape as when the snapshot was taken.
        virtual WorkItemMemento* createMemento() const;
        virtual bool restore(const WorkItemMemento& memento);

        //Decorator Pattern
        virtual int getPriority() const; 

        //Routing and engine support
        /// @brief The role a participant must have to work on this item ("" means any role).
        /// Copied by clone(), because it is part of the definition and not of one running case.
        virtual std::string getRequiredRole() const;
        virtual void setRequiredRole(const std::string& role);

        /// @name State queries
        /// The engine asks these instead of comparing state names as text.
        /// @{
        virtual bool isAvailable() const;
        virtual bool isCompleted() const;
        virtual bool isCancelled() const;
        /// @}

        /**
         * @brief Moves this item as far forward as it can without a worker.
         *
         * For a Task (leaf) this makes it available. It then waits for a worker to
         * assign, start and complete it. Stage overrides this to run its children.
         *
         * @param data the data of the workflow instance (used by routing rules)
         * @return true once this item is Completed
         */
        virtual bool advance(const DataMap& data);

        //Visitor (Element): each subclass tells the visitor what kind of item it is
        virtual void accept(WorkItemVisitor& visitor) = 0;
        
        //Observer Pattern
        virtual void attach(Observer*);
        virtual void detach(Observer*);
        virtual void notify(std::string oldState);

};


#endif //WORKITEM_H