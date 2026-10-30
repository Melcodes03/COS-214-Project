#ifndef WORKITEMITERATOR_H
#define WORKITEMITERATOR_H

class WorkItem;

//Iterator
class WorkItemIterator {
    public: 
        virtual ~WorkItemIterator() {}
        virtual void first() = 0;
        virtual void next() = 0;
        virtual bool isDone() const = 0;
        virtual WorkItem* currentItem() const = 0; 
};

#endif //WORKITEMITERATOR_H