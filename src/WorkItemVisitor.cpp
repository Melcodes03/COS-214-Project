#include "WorkItemVisitor.h"
#include "WorkItem.h"
#include "WorkItemIterator.h"

void WorkItemVisitor::visitAll(WorkItem& root) {
    WorkItemIterator* it = root.createIterator();
    for (it->first(); !it->isDone(); it->next()) {
        it->currentItem()->accept(*this);
    }
    delete it;
}
