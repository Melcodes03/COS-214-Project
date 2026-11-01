#ifndef WORKFLOWSPEC_H
#define WORKFLOWSPEC_H

#include <string>
#include <vector>

/// Plain data that describes ANY workflow: the filled-in "New scenario" form as
/// C++ data. These structs only hold information. They have no behaviour and are
/// not a pattern. The Director reads them, so a new industry means new data and
/// no new code. A GUI form, a saved JSON file or a unit test can all produce
/// the same WorkflowSpec.
///
/// Ids are not stored here. WorkflowDirector generates them from position:
/// stage 2 is "S2" and its first task is "S2-T1".

/// How a stage runs its children (mirrors Stage::setSequential, setParallel, setChoice).
/// The Director's mode table is indexed by this order, so do not reorder it.
enum Mode {
    SEQUENTIAL,   ///< one task after another (the default)
    PARALLEL,     ///< all tasks open together; the stage waits for all of them
    CHOICE        ///< only one task runs: the first whose condition matches the data
};

/// One task in a stage.
/// whenKey/whenOp/whenValue only matter inside a CHOICE stage. The task runs when
/// the instance data value for whenKey passes the comparison. Operators: ==, !=,
/// <, <=, >, >=. Values compare as numbers when both sides are numbers, otherwise
/// as text. An EMPTY whenKey marks the default (else) branch.
struct TaskSpec {
    std::string name;
    std::string role;
    int priority = 0;       ///< 0 = no priority decorator
    bool audit = false;     ///< true = wrap the task in an auditing decorator
    std::string whenKey;
    std::string whenOp;
    std::string whenValue;
    int signoffs = 0;       ///< 0 = finished with Complete; 1 or more = needs that many approvals (Strategy)
};

/// One stage: a name, how it runs its tasks, and the tasks.
struct StageSpec {
    std::string name;
    Mode mode = SEQUENTIAL;
    std::vector<TaskSpec> tasks;
};

/// A whole workflow description. The name is also the industry name shown to the user.
struct WorkflowSpec {
    std::string name;
    std::vector<StageSpec> stages;
};

#endif //WORKFLOWSPEC_Hs