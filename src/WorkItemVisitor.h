#ifndef WORKITEMVISITOR_H
#define WORKITEMVISITOR_H

class WorkItem;
class Task;
class Stage;

//Visitor: abstract Visitor. One operation (a report, a check) over the work tree.
//A new report is a new subclass; Task, Stage and WorkItem never change for it.
class WorkItemVisitor {
    public:
        virtual ~WorkItemVisitor() {}

        virtual void visitTask(Task& task) = 0;
        virtual void visitStage(Stage& stage) = 0;

        //Visits every item under root (root included). The Iterator does the
        //walking, so the visitor never needs to know how the tree is stored.
        void visitAll(WorkItem& root);
};

#endif //WORKITEMVISITOR_H
