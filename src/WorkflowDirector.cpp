#include "WorkflowDirector.h"
#include "WorkflowBuilder.h"
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace {

/// One task's "run me when" rule inside a choice stage.
struct Condition {
    std::string key;
    std::string op;
    std::string value;
};

//Reads the whole text as a number. False if it is empty or has anything else in it.
bool toNumber(const std::string& text, double& out) {
    if (text.empty()) {
        return false;
    }
    const char* begin = text.c_str();
    char* end = nullptr;
    out = std::strtod(begin, &end);
    return end != begin && *end == '\0';
}

//-1, 0 or 1. Numbers compare as numbers ("10" > "9"), anything else as text.
int compare(const std::string& left, const std::string& right) {
    double a = 0;
    double b = 0;
    if (toNumber(left, a) && toNumber(right, b)) {
        return a < b ? -1 : (a > b ? 1 : 0);
    }
    int c = left.compare(right);
    return c < 0 ? -1 : (c > 0 ? 1 : 0);
}

bool isEqual(int c) { return c == 0; }
bool isNotEqual(int c) { return c != 0; }
bool isLess(int c) { return c < 0; }
bool isLessOrEqual(int c) { return c <= 0; }
bool isGreater(int c) { return c > 0; }
bool isGreaterOrEqual(int c) { return c >= 0; }

//operator text -> test on the comparison result (a table, not an if/else chain)
const std::map<std::string, bool (*)(int)>& operators() {
    static const std::map<std::string, bool (*)(int)> table = {
        {"==", &isEqual}, {"!=", &isNotEqual},
        {"<", &isLess}, {"<=", &isLessOrEqual},
        {">", &isGreater}, {">=", &isGreaterOrEqual}
    };
    return table;
}

//True if the data holds the key and the comparison passes. Unknown operators never match.
bool holds(const Condition& condition, const DataMap& data) {
    DataMap::const_iterator found = data.find(condition.key);
    if (found == data.end()) {
        return false;
    }
    std::map<std::string, bool (*)(int)>::const_iterator test = operators().find(condition.op);
    if (test == operators().end()) {
        return false;
    }
    return test->second(compare(found->second, condition.value));
}

//Builds the rule a choice stage uses to pick its child. Everything it needs is
//copied into it, so it stays valid after the spec is gone.
Stage::Selector makeSelector(const StageSpec& stage) {
    std::vector<Condition> conditions;
    int fallback = -1;   //the default branch: first task with no whenKey
    for (int i = 0; i < (int)stage.tasks.size(); i++) {
        const TaskSpec& task = stage.tasks[i];
        Condition condition;
        condition.key = task.whenKey;
        condition.op = task.whenOp;
        condition.value = task.whenValue;
        conditions.push_back(condition);
        if (task.whenKey.empty() && fallback < 0) {
            fallback = i;
        }
    }
    return [conditions, fallback](const DataMap& data) -> int {
        for (int i = 0; i < (int)conditions.size(); i++) {
            if (!conditions[i].key.empty() && holds(conditions[i], data)) {
                return i;   //first match wins
            }
        }
        return fallback;    //-1 when there is no default: nothing matched
    };
}

}

void WorkflowDirector::applySequential(WorkflowBuilder& builder, const StageSpec&) const {
    builder.inSequence();
}

void WorkflowDirector::applyParallel(WorkflowBuilder& builder, const StageSpec&) const {
    builder.inParallel();
}

void WorkflowDirector::applyChoice(WorkflowBuilder& builder, const StageSpec& stage) const {
    builder.choosingBy(makeSelector(stage));
}

void WorkflowDirector::construct(WorkflowBuilder& builder, const WorkflowSpec& spec) const {
    //indexed by Mode: SEQUENTIAL, PARALLEL, CHOICE
    static const ModeStep steps[] = {
        &WorkflowDirector::applySequential,
        &WorkflowDirector::applyParallel,
        &WorkflowDirector::applyChoice
    };

    for (int i = 0; i < (int)spec.stages.size(); i++) {
        const StageSpec& stage = spec.stages[i];
        std::string stageId = "S" + std::to_string(i + 1);

        builder.beginStage(stageId, stage.name);
        int mode = static_cast<int>(stage.mode);
        (this->*steps[(mode >= 0 && mode < 3) ? mode : 0])(builder, stage);   //a bad value falls back to sequential

        for (int j = 0; j < (int)stage.tasks.size(); j++) {
            const TaskSpec& task = stage.tasks[j];
            builder.addTask(stageId + "-T" + std::to_string(j + 1), task.name, task.role);
            if (task.priority > 0) {
                builder.withPriority(task.priority);
            }
            if (task.audit) {
                builder.withAudit();
            }
        }
        builder.endStage();
    }
}