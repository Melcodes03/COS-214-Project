#ifndef WORKITEM_H
#define WORKITEM_H

#include <string> 

class WorkItemIterator;
class WorkItemState;

class WorkItem{
    protected: 
        std::string id;
        std::string name;
        WorkItemState* state;
        int retryCount;  //runtime data: reworks used so far (per instance)
        int maxRetries;  //configuration: reworks allowed after a rejection

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
        virtual void setState(WorkItemState* newState);
        //Retry limit (used by RejectedState). Virtual so decorators can forward.
        virtual int getRetryCount() const;
        virtual int getMaxRetries() const;
        virtual void setMaxRetries(int max);
        bool retriesRemaining() const;
        void useRetry();

        //Decorator Pattern
        virtual int getPriority() const; 
};


#endif //WORKITEM_H