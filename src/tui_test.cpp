/// Scripted TUI test.  Exercises BOTH industries through the running TUI interface.
///
/// What it proves (matches the "done when" list):
///   1. Start an instance
///   2. Inspect available work  (tree, worklists)
///   3. Take actions            (allocate, start, complete, decline, escalate, cancel)
///   4. Observe routing and state changes  (inline observer notifications in output)
///   5. View history            (history, undo, redo, rollback)
///   6. Use both industries     (runs the whole sequence twice, once per industry)
///
/// Build and run with:   make test_tui

#include <iostream>
#include <sstream>
#include <string>
#include <cassert>

#include "TUI.h"

/// Feeds `commands` as input, captures all output, then searches the output
/// for every string in `expected`.  Returns true if all were found.
static bool runSession(const std::string& commands,
                       const std::vector<std::string>& expected,
                       std::string& capturedOutput) {
    std::istringstream in(commands);
    std::ostringstream out;
    TUI tui(in, out);
    tui.run();
    capturedOutput = out.str();

    bool allFound = true;
    for (size_t i = 0; i < expected.size(); ++i) {
        if (capturedOutput.find(expected[i]) == std::string::npos) {
            std::cerr << "MISSING: \"" << expected[i] << "\"\n";
            allFound = false;
        }
    }
    return allFound;
}

// ──────────────────────────────────────────────────────────────────────────
// Test 1: Software Customer Support (Industry 1) – full lifecycle
// ──────────────────────────────────────────────────────────────────────────
static bool testSupportDesk() {
    // Commands fed to the TUI (one per line, as if typed by a user)
    std::string commands =
        "1\n"                   // pick industry 1
        "tree\n"                // 1. inspect the work tree
        "worklists\n"           // 2. view worklists (empty at start)
        "allocate SD1-1\n"      // 3. allocate "Log the case" -> goes to first available GA
        "tree\n"                //    confirm it moved to Assigned
        "start SD1-1\n"         // 4. start it
        "complete SD1-1\n"      // 5. complete it
        "tree\n"                //    confirm Completed
        "allocate SD2-1\n"      // 6. allocate "Check logs"
        "start SD2-1\n"         //    start it
        "decline SD2-1\n"       // 7. decline -> reassign up the chain
        "tree\n"                //    confirm new assignee
        "allocate SD2-2\n"      // 8. allocate "Reproduce bug"
        "escalate SD2-2\n"      // 9. escalate -> next level
        "cancel SD3-1\n"        // 10. cancel "Apply fix"
        "progress\n"            // 11. progress report
        "escalations\n"         // 12. escalation report
        "history\n"             // 13. view command history
        "undo\n"                // 14. undo last action
        "redo\n"                // 15. redo
        "rollback\n"            //     rollback
        "0\n"                   //     marker 0 (beginning)
        "history\n"             //     confirm history is empty after rollback
        "quit\n";               // done

    std::vector<std::string> expected;
    // Instance started
    expected.push_back("Software Customer Support");
    expected.push_back("SD-I1");
    // Tree visible with task ids
    expected.push_back("SD1-1");
    expected.push_back("Log the case");
    expected.push_back("SD2-1");
    expected.push_back("Check logs");
    expected.push_back("SD3-1");
    expected.push_back("Apply fix");
    // Worklists header
    expected.push_back("Worklists");
    // Allocation happened
    expected.push_back("Allocated SD1-1");
    // Observer notification (state change)
    expected.push_back("[Observer]");
    expected.push_back("Assigned");
    // Start
    expected.push_back("started SD1-1");
    // Complete
    expected.push_back("completed SD1-1");
    expected.push_back("Completed");
    // Decline + reassign
    expected.push_back("declined SD2-1");
    expected.push_back("Reassigned to");
    // Escalate
    expected.push_back("Escalated SD2-2");
    // Cancel
    expected.push_back("Cancelled SD3-1");
    // Progress report
    expected.push_back("Progress");
    // Escalation report
    expected.push_back("Escalation");
    // History
    expected.push_back("Command History");
    // Undo
    expected.push_back("Undone");
    // Redo
    expected.push_back("Redone");
    // Rollback
    expected.push_back("Rolled back");

    std::string output;
    bool ok = runSession(commands, expected, output);
    if (!ok) {
        std::cerr << "\n=== FULL OUTPUT (Support Desk) ===\n" << output << "\n";
    }
    return ok;
}

// ──────────────────────────────────────────────────────────────────────────
// Test 2: University Application Process (Industry 2) – full lifecycle
// ──────────────────────────────────────────────────────────────────────────
static bool testUniversityAdmissions() {
    std::string commands =
        "2\n"                   // pick industry 2
        "tree\n"                // inspect tree
        "worklists\n"           // view worklists
        "allocate UA1-1\n"      // allocate "Receive application"
        "start UA1-1\n"         // start it
        "complete UA1-1\n"      // complete it
        "allocate UA2-1\n"      // allocate "Check documents"
        "decline UA2-1\n"       // decline -> reassign
        "allocate UA3-1\n"      // allocate "Assess grades"
        "escalate UA3-1\n"      // escalate
        "progress\n"            // progress report
        "escalations\n"         // escalation report
        "history\n"             // history
        "undo\n"                // undo
        "redo\n"                // redo
        "quit\n";

    std::vector<std::string> expected;
    expected.push_back("University Application Process");
    expected.push_back("UA-I1");
    expected.push_back("UA1-1");
    expected.push_back("Receive application");
    expected.push_back("UA2-1");
    expected.push_back("Check documents");
    expected.push_back("UA3-1");
    expected.push_back("Assess grades");
    expected.push_back("Worklists");
    expected.push_back("Allocated UA1-1");
    expected.push_back("[Observer]");
    expected.push_back("started UA1-1");
    expected.push_back("completed UA1-1");
    expected.push_back("declined UA2-1");
    expected.push_back("Escalated UA3-1");
    expected.push_back("Progress");
    expected.push_back("Escalation");
    expected.push_back("Command History");
    expected.push_back("Undone");
    expected.push_back("Redone");

    std::string output;
    bool ok = runSession(commands, expected, output);
    if (!ok) {
        std::cerr << "\n=== FULL OUTPUT (University Admissions) ===\n" << output << "\n";
    }
    return ok;
}

// ──────────────────────────────────────────────────────────────────────────
// Test 3: Switch between industries in a single session
// ──────────────────────────────────────────────────────────────────────────
static bool testSwitchIndustry() {
    std::string commands =
        "1\n"                   // start with support desk
        "tree\n"
        "switch\n"              // switch to industry 2
        "2\n"
        "tree\n"                // should now show UA tree
        "quit\n";

    std::vector<std::string> expected;
    expected.push_back("Software Customer Support");
    expected.push_back("SD1-1");
    expected.push_back("Switched to University Application Process");
    expected.push_back("UA1-1");

    std::string output;
    bool ok = runSession(commands, expected, output);
    if (!ok) {
        std::cerr << "\n=== FULL OUTPUT (Switch) ===\n" << output << "\n";
    }
    return ok;
}

// ──────────────────────────────────────────────────────────────────────────
// Test 4: Create a custom workflow from scratch (Wizard)
// ──────────────────────────────────────────────────────────────────────────
static bool testCustomWorkflow() {
    std::string commands =
        "3\n"                   // pick custom workflow
        "Custom Order Intake\n" // workflow name
        "COI\n"                 // workflow id prefix
        "2\n"                   // 2 stages
        "COI1\n"                // stage 1 id
        "Validation\n"          // stage 1 name
        "1\n"                   // sequential
        "1\n"                   // 1 task
        "COI1-1\n"              // task id
        "Verify Address\n"      // task name
        "Clerk\n"               // required role
        "COI2\n"                // stage 2 id
        "Processing\n"          // stage 2 name
        "1\n"                   // sequential
        "1\n"                   // 1 task
        "COI2-1\n"              // task id
        "Fulfill Order\n"       // task name
        "Manager\n"             // required role
        "2\n"                   // 2 participants
        "Kagiso\n"              // participant 1
        "Clerk\n"               // role
        "3\n"                   // capacity
        "Nomvula\n"             // participant 2
        "Manager\n"             // role
        "10\n"                  // capacity
        "tree\n"                // inspect tree
        "allocate COI1-1\n"     // allocate
        "start COI1-1\n"        // start
        "complete COI1-1\n"     // complete
        "quit\n";

    std::vector<std::string> expected;
    expected.push_back("Custom Order Intake");
    expected.push_back("COI-I1");
    expected.push_back("COI1-1");
    expected.push_back("Verify Address");
    expected.push_back("Allocated COI1-1");
    expected.push_back("Kagiso");
    expected.push_back("started COI1-1");
    expected.push_back("completed COI1-1");

    std::string output;
    bool ok = runSession(commands, expected, output);
    if (!ok) {
        std::cerr << "\n=== FULL OUTPUT (Custom Workflow) ===\n" << output << "\n";
    }
    return ok;
}

// ──────────────────────────────────────────────────────────────────────────
int main() {
    int failures = 0;

    std::cout << "=== TUI Scripted Test ===" << std::endl;

    std::cout << "Test 1: Software Customer Support ... ";
    if (testSupportDesk()) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        ++failures;
    }

    std::cout << "Test 2: University Application Process ... ";
    if (testUniversityAdmissions()) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        ++failures;
    }

    std::cout << "Test 3: Switch industry in one session ... ";
    if (testSwitchIndustry()) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        ++failures;
    }

    std::cout << "Test 4: Create custom workflow from scratch ... ";
    if (testCustomWorkflow()) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        ++failures;
    }

    std::cout << "\n" << (failures == 0 ? "All TUI tests passed." : "Some tests FAILED.") << std::endl;
    return failures;
}
