#ifndef WORKITEM_H
#define WORKITEM_H

#include <string> 

class WorkItemIterator;

class WorkItem{
    protected: 
        std::string id;
        std::string name;
    
    public: 
        WorkItem(const std::string& id, const std::string& name);
        virtual ~WorkItem();

        std::string getId() const;
        std::string getName() const;
            
        //Composite
        virtual bool add(WorkItem* child);
        virtual WorkItem* getChild(int index) const;
        virtual int getChildCount() const;
        virtual WorkItem* clone() const = 0;

        //Iterator
        WorkItemIterator* createIterator(); 
};


#endif //WORKITEM_H