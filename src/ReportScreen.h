#ifndef REPORTSCREEN_H
#define REPORTSCREEN_H

#include <iosfwd>
#include <string>

class WorkItem;
class WorkAllocator;

/// The report and escalation screens of the text interface.
///
/// Each function prints to the stream it is given, so the TUI passes std::cout
/// and the tests pass a std::ostringstream. Nothing here knows about an
/// industry: it works on any workflow tree and any chain of levels.
///
/// Invalid requests (an unknown task id, a task that cannot be escalated)
/// print a friendly message and change nothing; they never crash.
class ReportScreen {
    public:
        /// Overall progress plus one line per stage
        /// (ProgressReportVisitor, walked with the Iterator).
        static void showProgress(WorkItem& root, std::ostream& out);

        /// Every escalated or rejected task and the stage it is in
        /// (EscalationReportVisitor).
        static void showEscalations(WorkItem& root, std::ostream& out);

        /// The worker holding task @p itemId did not respond: escalate it to the next level.
        /// @return true if someone new now holds the task.
        static bool escalateTask(WorkItem& root, const std::string& itemId,
                                 WorkAllocator& allocator, std::ostream& out);

        /// The worker declined task @p itemId: give it to the next level.
        /// @return true if someone new now holds the task.
        static bool reassignTask(WorkItem& root, const std::string& itemId,
                                 WorkAllocator& allocator, std::ostream& out);

        /// Finds the item with id @p itemId anywhere in the tree, or nullptr.
        static WorkItem* find(WorkItem& root, const std::string& itemId);

    private:
        /// Prints who has the task now, or why nobody could take it.
        static bool reportMove(WorkItem& item, const std::string& oldHolder,
                               WorkAllocator& allocator, bool moved, std::ostream& out);
};

#endif //REPORTSCREEN_H
