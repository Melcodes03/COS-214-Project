#include "ReportScreen.h"
#include "WorkItem.h"
#include "WorkItemIterator.h"
#include "WorkAllocator.h"
#include "Participant.h"
#include "ProgressReportVisitor.h"
#include "EscalationReportVisitor.h"

#include <ostream>
#include <vector>

void ReportScreen::showProgress(WorkItem& root, std::ostream& out) {
    ProgressReportVisitor report;
    report.visitAll(root);
    out << "=== Progress: " << root.getName() << " ===\n";
    out << report.summary() << "\n";
    out << report.stageSummary();
}

void ReportScreen::showEscalations(WorkItem& root, std::ostream& out) {
    EscalationReportVisitor report;
    report.visitAll(root);
    const std::vector<std::string>& flagged = report.getFlagged();
    out << "=== Escalated and rejected tasks: " << root.getName() << " ===\n";
    if (flagged.empty()) {
        out << "No escalated or rejected tasks.\n";
        return;
    }
    for (int i = 0; i < (int)flagged.size(); i++) {
        out << "- " << flagged[i] << "\n";
    }
}

WorkItem* ReportScreen::find(WorkItem& root, const std::string& itemId) {
    WorkItem* found = nullptr;
    WorkItemIterator* it = root.createIterator();
    for (it->first(); !it->isDone() && found == nullptr; it->next()) {
        if (it->currentItem()->getId() == itemId) {
            found = it->currentItem();
        }
    }
    delete it;
    return found;
}

bool ReportScreen::escalateTask(WorkItem& root, const std::string& itemId,
                                WorkAllocator& allocator, std::ostream& out) {
    WorkItem* item = find(root, itemId);
    if (item == nullptr) {
        out << "There is no task with id \"" << itemId << "\".\n";
        return false;
    }
    Participant* holder = allocator.getAssignee(itemId);
    std::string oldHolder = holder == nullptr ? "" : holder->getName();
    WorkItemState* stateBefore = item->getState();

    Participant* chosen = allocator.escalate(*item);
    if (chosen == nullptr && item->getState() == stateBefore) {
        out << "Task " << itemId << " cannot be escalated while it is " << item->getStateName() << ".\n";
        return false;
    }
    return reportMove(*item, oldHolder, allocator, chosen != nullptr, out);
}

bool ReportScreen::reassignTask(WorkItem& root, const std::string& itemId,
                                WorkAllocator& allocator, std::ostream& out) {
    WorkItem* item = find(root, itemId);
    if (item == nullptr) {
        out << "There is no task with id \"" << itemId << "\".\n";
        return false;
    }
    Participant* holder = allocator.getAssignee(itemId);
    std::string oldHolder = holder == nullptr ? "" : holder->getName();
    return reportMove(*item, oldHolder, allocator, allocator.reassign(*item) != nullptr, out);
}

bool ReportScreen::reportMove(WorkItem& item, const std::string& oldHolder,
                              WorkAllocator& allocator, bool moved, std::ostream& out) {
    if (!moved) {
        out << "Nobody above " << (oldHolder.empty() ? "the current level" : oldHolder)
            << " could take task " << item.getId() << ". It is now " << item.getStateName() << ".\n";
        return false;
    }
    Participant* now = allocator.getAssignee(item.getId());
    out << "Task " << item.getId() << " (" << item.getName() << ") moved";
    if (!oldHolder.empty()) {
        out << " from " << oldHolder;
    }
    out << " to " << now->getName() << " (" << now->getRole() << ").\n";
    return true;
}
