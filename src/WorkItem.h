#ifndef WORKITEM_H
#define WORKITEM_H

#include <string> 
#include <vector>

class Observer;

class WorkItemIterator;
class WorkItemState;
class WorkItemMemento;

class WorkItem{
    protected: 
        std::string id;
        std::string name;
        WorkItemState* state;
        int retryCount;  //runtime data: reworks used so far (per instance)
        int maxRetries;  //configuration: reworks allowed after a rejection
        std::vector<Observer*> observers;   //stores attached observers

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
        virtual WorkItem* clone() const = 0;

        //Iterator
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
        bool retriesRemaining() const;
        void useRetry();

        //Memento (Originator): snapshot this item and its whole subtree.
        //restore() returns false and changes nothing if the tree no longer has
        //the same shape as when the snapshot was taken.
        virtual WorkItemMemento* createMemento() const;
        virtual bool restore(const WorkItemMemento& memento);

        //Decorator Pattern
        virtual int getPriority() const; 

        //Observer Pattern
        virtual void attach(Observer*);
        virtual void detach(Observer*);
        virtual void notify(std::string oldState);
};


#endif //WORKITEM_H