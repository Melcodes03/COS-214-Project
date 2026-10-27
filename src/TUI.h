#ifndef TUI_H
#define TUI_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "Observer.h"

class IndustryScenario;
class WorkAllocator;
class Participant;
class CommandHistory;

/// A lightweight Observer that prints state changes to an ostream (for inline TUI notifications).
/// Does not own the WorkItem; the TUI attaches/detaches it.
class TUIObserver : public Observer {
    private:
        std::ostream& out;
    public:
        TUIObserver(WorkItem* item, std::ostream& out);
        virtual ~TUIObserver();
        void update(WorkItemState* newState, std::string oldState) override;
};

/// The text user interface for the TIVIDY workflow engine.
///
/// Hybrid interaction model:
///   - Numbered menus for navigation
///   - Shortcut commands (e.g. "start SD1-1", "undo", "history") for power users
///
/// All output goes to an ostream (std::cout for interactive, std::ostringstream for tests).
/// All input comes from an istream (std::cin for interactive, std::istringstream for tests).
class TUI {
    public:
        TUI(std::istream& in, std::ostream& out);
        ~TUI();

        /// Runs the TUI until the user quits. Returns 0 on clean exit.
        int run();

    private:
        std::istream& in;
        std::ostream& out;

        // Current scenario (owned by the TUI after setup)
        IndustryScenario* scenario;
        std::vector<TUIObserver*> observers;   // owned; attached to work items

        // --- Screen functions ---
        void showBanner();
        int  pickIndustry();                    // returns 1, 2, or 3, or 0 to quit
        IndustryScenario* buildCustomWorkflow();
        void mainMenu();
        void handleMainMenuChoice(const std::string& choice);

        void update(WorkItemState* newState, std::string oldState);

        // Work tree
        void showWorkTree();
        void showWorkTreeRecursive(WorkItem* item, int depth);

        // Worklist per participant
        void showWorklists();
        void showParticipantWorklist(Participant* p);

        // Actions
        void allocateTask();
        void startTask();
        void completeTask();
        void declineTask();
        void escalateTask();
        void cancelTask();

        // Reports
        void showProgress();
        void showEscalations();

        // History
        void showHistory();
        void undoAction();
        void redoAction();
        void rollbackAction();

        // Utilities
        std::string prompt(const std::string& msg);
        WorkItem* findTask(const std::string& id);
        Participant* findParticipant(const std::string& name);
        void attachObservers();
        void detachObservers();

        void printSeparator();
        void printHeader(const std::string& title);
};

#endif // TUI_H
