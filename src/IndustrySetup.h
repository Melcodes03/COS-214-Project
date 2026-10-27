#ifndef INDUSTRYSETUP_H
#define INDUSTRYSETUP_H

#include <string>
#include <vector>

class WorkflowDefinition;
class WorkflowInstance;
class Participant;
class WorkAllocator;
class AssignmentHandler;
class CommandHistory;

/// Holds everything needed to run one industry scenario through the TUI.
/// Owns all the objects it creates; the destructor cleans up.
struct IndustryScenario {
    std::string name;
    WorkflowDefinition* definition;
    WorkflowInstance* instance;
    WorkAllocator* allocator;
    CommandHistory* history;
    std::vector<Participant*> participants;

    IndustryScenario();
    ~IndustryScenario();

    IndustryScenario(const IndustryScenario&);
    IndustryScenario& operator=(const IndustryScenario&);
};

/// Factory functions that build fully wired industry scenarios.
namespace IndustrySetup {

    /// Industry 1: Software customer support.
    /// Stages: Log -> Investigate -> Resolve
    /// Chain:  General Assistant -> Technical Assistant -> Engineer -> Support Manager
    IndustryScenario createSupportDesk();

    /// Industry 2: University application process.
    /// Stages: Submit -> Verify Docs -> Academic Review -> Committee Decision
    /// Chain:  Admin Clerk -> Admissions Officer -> Department Head -> Registrar
    IndustryScenario createUniversityAdmissions();
}

#endif // INDUSTRYSETUP_H
