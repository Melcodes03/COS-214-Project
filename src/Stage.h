#ifndef STAGE_H
#define STAGE_H

#include "WorkItem.h"
#include <vector> 

class Stage : public WorkItem {
    private: 
        std::vector<WorkItem*> children;

    public: 
        Stage(const std::string& id, const std::string& name);
        ~Stage() override; 

        bool add(WorkItem* child) override;
        WorkItem* getChild(int index) const override;
        int getChildCount() const override;

        WorkItem* clone() const override;
        void accept(WorkItemVisitor& visitor) override;   //Visitor: ConcreteElement
        Stage(const Stage&) = delete;
        Stage& operator=(const Stage&) = delete;
};      

#endif //STAGE_H