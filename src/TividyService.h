#ifndef TIVIDYSERVICE_H
#define TIVIDYSERVICE_H

#include <ctime>
#include <mutex>
#include <string>
#include <vector>

struct TividyCatalog;    ///< the available industries (data from DemoData)
struct TividyScenario;   ///< one loaded industry: definition, instance, people, allocator, history
class BatchRunner;       ///< "Run all tickets" (Mediator)
class ServiceDesk;       ///< the cases as BatchRunner sees them

/// Facade: the one object the web layer talks to.
///
/// TividyService hides the whole engine (definitions, instances, allocator, escalation
/// chain, command history, reports) behind a handful of calls that match the REST
/// API in docs/web-api.md. Every call returns a JSON text, so the transport (the
/// HTTP server in web_main.cpp, or a unit test) never touches an engine class.
///
/// It keeps several cases open at once (each one an independent WorkflowInstance with its
/// own data, people and history) and one of them is "current". Loading an industry builds the scenario
/// from DemoData through the generic WorkflowDirector, so both industries use exactly
/// the same engine code. All calls are serialised with a mutex because the HTTP
/// server is multi-threaded and the engine is not.
class TividyService {
    public:
        TividyService();
        ~TividyService();

        TividyService(const TividyService&) = delete;
        TividyService& operator=(const TividyService&) = delete;

        /// GET /api/industries
        std::string industries();

        /// POST /api/industry. @p dataOverrides is a JSON object text such as {"score":65}
        /// (or "" for none) whose values replace the sample case's data.
        std::string loadIndustry(int industryId, int caseIndex, const std::string& dataOverrides);

        /// POST /api/workflow. @p specJson describes a new industry (name, escalation levels,
        /// people, stages and tasks; see docs/web-api.md). It is checked, added to the list of
        /// industries and a first case is started on it. The engine code is the same as for the
        /// built-in industries: the description goes through the generic WorkflowDirector.
        std::string createWorkflow(const std::string& specJson);

        /// GET /api/state
        std::string state();

        /// POST /api/case. Makes open case number @p index (0-based, see "openCases" in the
        /// state) the current one. The other cases are untouched: instances are independent.
        std::string switchCase(int index);

        /// POST /api/action. @p action is one of allocate, start, complete, decline,
        /// escalate, cancel. @p participantName is optional (see the API document).
        std::string act(const std::string& action, const std::string& taskId,
                        const std::string& participantName);

        /// The same, on the open case whose id is @p caseId (for example from "My work", which lists
        /// tasks of every case). The current case does not change; the reply's state is the current
        /// case. An empty @p caseId means the current case.
        std::string act(const std::string& action, const std::string& taskId,
                        const std::string& participantName, const std::string& caseId);

        /// GET /api/overview: every open case, every task, the staff, the workspaces and the
        /// activity log across all cases, for the dashboard, "My work", alerts, history and reports.
        std::string overview();

        /// Keeps data across restarts. Replays the log file at @p path (every change made
        /// before: cases, workflows, actions, undo, case switches) through the same calls, so the
        /// engine rebuilds exactly the same state, then appends every new change to it.
        /// @return how many changes were replayed (0 for a new file).
        int useLog(const std::string& path);

        /// POST /api/reset: closes every case, forgets workflows built in the app and empties the log.
        std::string reset();

        /// Opens the default scenarios (3 support cases and 3 admissions applications, untouched)
        /// if no case is open. Called at start-up, like a main().
        std::string loadDefaults();

        /// POST /api/defaults: "Reset to defaults" = reset() then loadDefaults().
        std::string resetToDefaults();

        /// POST /api/review: the Reviewer re-offers stalled or flagged work of the current case.
        std::string review();

        /// POST /api/batch: "Run all tickets". Opens @p count new cases of a workflow; at most
        /// @p slots run at once, the rest wait. @p policy is "happy" or "declines".
        std::string startBatch(int industryId, int count, int slots, const std::string& policy);

        /// POST /api/batch/tick: @p ticks rounds (each running ticket takes one engine step);
        /// 0 means "finish instantly".
        std::string tick(int ticks);

        /// POST /api/batch/stop: stops the run. The cases stay open as they are.
        std::string stopBatch();

        /// POST /api/undo, /api/redo, /api/rollback
        std::string undo();
        std::string redo();
        std::string rollback(int marker);

        /// GET /api/report/progress and /api/report/escalations. @p type is "progress"
        /// or "escalations"; any other value gives an {"error": ...} body.
        std::string report(const std::string& type);

    private:
        friend class ServiceDesk;
        std::mutex lock;
        TividyCatalog* catalog;
        std::vector<TividyScenario*> open;   ///< every open case, oldest first (owned)
        std::string logPath;                 ///< "" = nothing is saved
        bool replaying = false;
        std::time_t replayTime = 0;          ///< while replaying: when the change was first made
        BatchRunner* batch = nullptr;        ///< the current "Run all tickets", or nullptr
        ServiceDesk* desk = nullptr;
        TividyScenario* scenario;            ///< the current case (one of open), or nullptr

        void openCase(TividyScenario* added);
        std::string uniqueCaseId(const std::string& base) const;
        std::string withCases(const std::string& stateText) const;

        std::string reply(bool ok, const std::string& message, const std::vector<std::string>& events);
        std::string snapshotJson();
        std::string actHere(const std::string& action, const std::string& taskId, const std::string& participantName);
        std::string loadHere(int industryId, int caseIndex, const std::string& dataOverrides);
        std::string resetHere();
        std::string defaultsHere();
        int reviewHere(TividyScenario& s);
        std::string batchJson() const;
        TividyScenario* findCase(const std::string& id) const;
        void record(const std::string& kind, const std::string& text, const std::string& who);
        std::time_t now() const;
        std::string logged(const std::string& reply, const std::string& entry);
};

#endif //TIVIDYSERVICE_H
