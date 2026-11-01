// doctest tests for TividyService: the facade behind the REST API.
// They drive the real engine through the same JSON the web page receives.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cstdio>

#include "third_party/json.hpp"
#include "TividyService.h"

typedef nlohmann::json Json;

namespace {

Json parse(const std::string& text) { return Json::parse(text); }

/// Finds a task (by id) in a state snapshot.
Json task(const Json& state, const std::string& id) {
    for (size_t i = 0; i < state["stages"].size(); ++i) {
        const Json& tasks = state["stages"][i]["tasks"];
        for (size_t j = 0; j < tasks.size(); ++j) {
            if (tasks[j]["id"] == id) return tasks[j];
        }
    }
    return Json();
}

bool allows(const Json& t, const std::string& action) {
    for (size_t i = 0; i < t["validActions"].size(); ++i) {
        if (t["validActions"][i] == action) return true;
    }
    return false;
}

Json act(TividyService& s, const std::string& action, const std::string& id, const std::string& who = "") {
    return parse(s.act(action, id, who));
}

/// allocate + start + complete one task with whoever the escalation chain picks
Json finish(TividyService& s, const std::string& id) {
    REQUIRE(act(s, "allocate", id)["ok"] == true);
    REQUIRE(act(s, "start", id)["ok"] == true);
    return act(s, "complete", id);
}

}  // namespace

TEST_SUITE("industries") {

    TEST_CASE("both industries are listed with stages, roles and sample cases") {
        TividyService s;
        Json list = parse(s.industries());
        REQUIRE(list.size() == 2);
        CHECK(list[0]["key"] == "support_desk");
        CHECK(list[1]["key"] == "university_admissions");
        CHECK(list[0]["stages"].size() == 5);
        CHECK(list[0]["decisionFields"][0] == "severity");
        CHECK(list[1]["decisionFields"][0] == "score");
        CHECK(list[0]["roles"][0] == "General Assistant");
        CHECK(list[1]["roles"][3] == "Registrar");
        CHECK(list[1]["cases"].size() == 3);
    }

    TEST_CASE("nothing is loaded at first") {
        TividyService s;
        Json st = parse(s.state());
        CHECK(st["loaded"] == false);
        CHECK(act(s, "allocate", "S1-T1")["ok"] == false);
    }

    TEST_CASE("loading an industry opens the first task") {
        TividyService s;
        Json r = parse(s.loadIndustry(1, 0, ""));
        REQUIRE(r["ok"] == true);
        Json st = r["state"];
        CHECK(st["instanceId"] == "SC-1001");
        CHECK(st["status"] == "Running");
        CHECK(task(st, "S1-T1")["state"] == "Available");
        CHECK(task(st, "S1-T2")["state"] == "Created");
        CHECK(allows(task(st, "S1-T1"), "allocate"));
        CHECK(!allows(task(st, "S1-T1"), "complete"));
    }

    TEST_CASE("an unknown industry or case is refused") {
        TividyService s;
        CHECK(parse(s.loadIndustry(99, 0, ""))["ok"] == false);
        CHECK(parse(s.loadIndustry(1, 7, ""))["ok"] == false);
        CHECK(parse(s.loadIndustry(1, 0, "[1,2]"))["ok"] == false);
    }
}

TEST_SUITE("actions") {

    TEST_CASE("allocate, start and complete move a task through its lifecycle") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        Json a = act(s, "allocate", "S1-T1");
        REQUIRE(a["ok"] == true);
        CHECK(task(a["state"], "S1-T1")["state"] == "Assigned");
        CHECK(task(a["state"], "S1-T1")["assignedTo"].is_string());
        CHECK(a["events"][0] == "S1-T1 (Log support case): Available -> Assigned");

        Json st = act(s, "start", "S1-T1");
        CHECK(task(st["state"], "S1-T1")["state"] == "InProgress");
        Json done = act(s, "complete", "S1-T1");
        CHECK(task(done["state"], "S1-T1")["state"] == "Completed");
    }

    TEST_CASE("completing a task opens the next one") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        Json r = finish(s, "S1-T1");
        CHECK(task(r["state"], "S1-T2")["state"] == "Available");
        CHECK(r["events"].size() >= 2);   // the task completed and the next one opened
    }

    TEST_CASE("the escalation chain gives work to someone with the right role") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        Json st = act(s, "allocate", "S1-T1")["state"];
        std::string who = task(st, "S1-T1")["assignedTo"];
        std::string role;
        for (size_t i = 0; i < st["participants"].size(); ++i) {
            if (st["participants"][i]["name"] == who) role = st["participants"][i]["role"];
        }
        CHECK(role == "General Assistant");
    }

    TEST_CASE("an invalid action is refused and changes nothing") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        Json r = act(s, "complete", "S1-T1");
        CHECK(r["ok"] == false);
        CHECK(r["message"].get<std::string>().find("Nobody holds") != std::string::npos);
        CHECK(task(r["state"], "S1-T1")["state"] == "Available");
        CHECK(r["state"]["history"]["undoCount"] == 0);

        Json again = act(s, "start", "S1-T2");   // still Created
        CHECK(again["ok"] == false);
    }

    TEST_CASE("unknown tasks, actions and participants are reported") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        CHECK(act(s, "allocate", "NOPE")["ok"] == false);
        CHECK(act(s, "dance", "S1-T1")["ok"] == false);
        CHECK(act(s, "allocate", "S1-T1", "Nobody Here")["ok"] == false);
        CHECK(act(s, "allocate", "S1")["ok"] == false);   // a stage, not a task
    }

    TEST_CASE("a task can be given to a chosen participant") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        Json r = act(s, "allocate", "S1-T1", "Ayesha Patel");
        REQUIRE(r["ok"] == true);
        CHECK(task(r["state"], "S1-T1")["assignedTo"] == "Ayesha Patel");
    }

    TEST_CASE("only the holder can act on a task") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "allocate", "S1-T1", "Ayesha Patel");
        CHECK(act(s, "start", "S1-T1", "Thabo Nkosi")["ok"] == false);
        CHECK(act(s, "start", "S1-T1", "Ayesha Patel")["ok"] == true);
    }

    TEST_CASE("declined work goes straight up the escalation chain, and one undo takes it back") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "allocate", "S1-T1", "Thabo Nkosi");
        Json declined = act(s, "decline", "S1-T1");
        REQUIRE(declined["ok"] == true);
        Json t = task(declined["state"], "S1-T1");
        CHECK(t["state"] == "Assigned");
        std::string who = t["assignedTo"];
        CHECK((who == "Lerato Mokoena" || who == "Pieter van Wyk"));   // the next level up
        CHECK(declined["message"].get<std::string>().find("passed on to") != std::string::npos);
        CHECK(declined["state"]["history"]["undoCount"] == 2);         // allocate, decline (one entry)
        Json u = parse(s.undo());
        CHECK(task(u["state"], "S1-T1")["assignedTo"] == "Thabo Nkosi");
        CHECK(task(u["state"], "S1-T1")["state"] == "Assigned");
    }

    TEST_CASE("escalating a task hands it to the next level") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "allocate", "S1-T1", "Thabo Nkosi");
        Json r = act(s, "escalate", "S1-T1");
        REQUIRE(r["ok"] == true);
        CHECK(task(r["state"], "S1-T1")["state"] == "Assigned");
        std::string who = task(r["state"], "S1-T1")["assignedTo"];
        CHECK((who == "Lerato Mokoena" || who == "Pieter van Wyk"));
    }

    TEST_CASE("a rejected task can only be reworked a limited number of times") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        Json st;
        int reworks = 0;
        REQUIRE(act(s, "allocate", "S1-T1")["ok"] == true);
        for (int i = 0; i < 10; ++i) {
            act(s, "start", "S1-T1");
            Json d = act(s, "decline", "S1-T1");   // sent back after starting = one rework
            if (d["ok"] != true) break;
            st = d["state"];
            ++reworks;
        }
        //every level below the manager sent it back once; the manager is the last level
        Json t = task(st, "S1-T1");
        CHECK(reworks == 3);
        CHECK(t["state"] == "Assigned");
        CHECK(t["assignedTo"] == "Naledi Khumalo");
        CHECK(t["retryCount"] == t["maxRetries"]);
        CHECK(!allows(t, "decline"));
        CHECK(allows(t, "start"));
    }

    TEST_CASE("the last level cannot pass work on, so the chain never freezes") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "allocate", "S1-T1");
        for (int i = 0; i < 3; ++i) REQUIRE(act(s, "escalate", "S1-T1")["ok"] == true);
        Json st = parse(s.state());
        Json t = task(st, "S1-T1");
        CHECK(t["assignedTo"] == "Naledi Khumalo");
        CHECK(t["lastLevel"] == true);
        CHECK(!allows(t, "escalate"));
        CHECK(!allows(t, "decline"));
        Json refused = act(s, "escalate", "S1-T1");
        CHECK(refused["ok"] == false);
        CHECK(refused["message"].get<std::string>().find("last level") != std::string::npos);
        CHECK(act(s, "decline", "S1-T1")["ok"] == false);
        CHECK(act(s, "start", "S1-T1")["ok"] == true);    // the manager can still do the work
        CHECK(act(s, "complete", "S1-T1")["ok"] == true);
    }

    TEST_CASE("work that has not opened yet cannot be cancelled") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        Json st = parse(s.state());
        CHECK(!allows(task(st, "S2-T1"), "cancel"));
        Json r = act(s, "cancel", "S2-T1");
        CHECK(r["ok"] == false);
        CHECK(r["state"]["status"] == "Running");
    }

    TEST_CASE("escalations, rework and cancellations send notifications (Observer + Adapter)") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "allocate", "S1-T1");
        Json e = act(s, "escalate", "S1-T1");
        REQUIRE(e["state"]["notifications"].size() == 1);
        CHECK(e["state"]["notifications"][0]["channel"] == "SMS");
        CHECK(e["state"]["notifications"][0]["text"].get<std::string>().find("was escalated") != std::string::npos);
        act(s, "start", "S1-T1");
        Json d = act(s, "decline", "S1-T1");
        REQUIRE(d["state"]["notifications"].size() == 2);
        CHECK(d["state"]["notifications"][1]["channel"] == "Email");
        Json c = act(s, "cancel", "S1-T1");
        CHECK(c["state"]["notifications"].back()["channel"] == "Email");
        CHECK(c["state"]["notifications"].back()["text"].get<std::string>().find("was cancelled") != std::string::npos);
        CHECK(c["state"]["notifications"].back()["to"] == "Naledi Khumalo");
        CHECK(c["state"]["failReason"].get<std::string>().find("S1-T1") != std::string::npos);
    }

    TEST_CASE("cancelling a task fails the case and ends the actions") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        Json r = act(s, "cancel", "S1-T1");
        REQUIRE(r["ok"] == true);
        CHECK(r["state"]["status"] == "Failed");
        bool failedEvent = false;
        for (size_t i = 0; i < r["events"].size(); ++i) {
            failedEvent = failedEvent || r["events"][i].get<std::string>().find("Running -> Failed") != std::string::npos;
        }
        CHECK(failedEvent);
        CHECK(act(s, "allocate", "S1-T2")["ok"] == false);
        CHECK(task(r["state"], "S1-T2")["validActions"].empty());
    }
}

TEST_SUITE("history") {

    TEST_CASE("undo and redo reverse and replay an action") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "allocate", "S1-T1", "Thabo Nkosi");
        Json u = parse(s.undo());
        REQUIRE(u["ok"] == true);
        CHECK(task(u["state"], "S1-T1")["state"] == "Available");
        CHECK(u["state"]["participants"][0]["workload"] == 0);
        CHECK(u["state"]["history"]["canRedo"] == true);
        Json r = parse(s.redo());
        REQUIRE(r["ok"] == true);
        CHECK(task(r["state"], "S1-T1")["state"] == "Assigned");
    }

    TEST_CASE("undoing a completion also closes the work it opened") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        Json r = finish(s, "S1-T1");
        REQUIRE(task(r["state"], "S1-T2")["state"] == "Available");
        Json u = parse(s.undo());
        CHECK(task(u["state"], "S1-T1")["state"] == "InProgress");
        CHECK(task(u["state"], "S1-T2")["state"] == "Created");
    }

    TEST_CASE("undo with nothing done is refused") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        CHECK(parse(s.undo())["ok"] == false);
        CHECK(parse(s.redo())["ok"] == false);
    }

    TEST_CASE("rollback returns to a marker and a bad marker is refused") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "allocate", "S1-T1");
        int marker = parse(s.state())["history"]["marker"].get<int>();
        act(s, "start", "S1-T1");
        act(s, "complete", "S1-T1");
        Json r = parse(s.rollback(marker));
        REQUIRE(r["ok"] == true);
        CHECK(task(r["state"], "S1-T1")["state"] == "Assigned");
        CHECK(task(r["state"], "S1-T2")["state"] == "Created");
        CHECK(parse(s.rollback(99))["ok"] == false);
        CHECK(parse(s.rollback(-1))["ok"] == false);
    }

    TEST_CASE("undoing the cancellation of a task makes the case run again") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "cancel", "S1-T1");
        Json u = parse(s.undo());
        CHECK(u["state"]["status"] == "Running");
        CHECK(act(s, "allocate", "S1-T1")["ok"] == true);
    }
}

TEST_SUITE("the second industry uses the same engine") {

    TEST_CASE("verification tasks run in parallel") {
        TividyService s;
        s.loadIndustry(2, 0, "");
        finish(s, "S1-T1");
        Json r = finish(s, "S1-T2");
        CHECK(task(r["state"], "S2-T1")["state"] == "Available");
        CHECK(task(r["state"], "S2-T2")["state"] == "Available");
    }

    TEST_CASE("the decision branch follows the score") {
        const int scores[3] = {78, 62, 48};
        const char* expected[3] = {"S4-T1", "S4-T2", "S4-T3"};
        for (int k = 0; k < 3; ++k) {
            TividyService s;
            s.loadIndustry(2, k, "");
            const char* order[] = {"S1-T1", "S1-T2", "S2-T1", "S2-T2", "S3-T1", "S3-T2"};
            Json st;
            for (int i = 0; i < 6; ++i) st = finish(s, order[i])["state"];
            CAPTURE(scores[k]);
            for (int b = 1; b <= 3; ++b) {
                std::string id = "S4-T" + std::to_string(b);
                bool open = task(st, id)["state"] == "Available";
                CHECK(open == (id == expected[k]));
            }
        }
    }

    TEST_CASE("a score override changes the route") {
        TividyService s;
        s.loadIndustry(2, 0, "{\"score\": 40}");
        const char* order[] = {"S1-T1", "S1-T2", "S2-T1", "S2-T2", "S3-T1", "S3-T2"};
        Json st;
        for (int i = 0; i < 6; ++i) st = finish(s, order[i])["state"];
        CHECK(task(st, "S4-T3")["state"] == "Available");   // rejection notice
    }

    TEST_CASE("a finished case shows the branches it skipped") {
        TividyService s;
        s.loadIndustry(2, 2, "");   // score 48: rejection
        const char* order[] = {"S1-T1", "S1-T2", "S2-T1", "S2-T2", "S3-T1", "S3-T2", "S4-T3", "S5-T2"};
        Json st;
        for (int i = 0; i < 8; ++i) st = finish(s, order[i])["state"];
        CHECK(st["status"] == "Completed");
        CHECK(task(st, "S4-T1")["skipped"] == true);
        CHECK(task(st, "S5-T1")["skipped"] == true);
        CHECK(task(st, "S4-T3")["skipped"] == false);
    }
}

TEST_SUITE("instances are isolated") {

    TEST_CASE("reloading starts a fresh case and the people are free again") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "allocate", "S1-T1", "Thabo Nkosi");
        Json fresh = parse(s.loadIndustry(1, 1, ""));
        CHECK(fresh["state"]["instanceId"] == "SC-1002");
        CHECK(task(fresh["state"], "S1-T1")["state"] == "Available");
        CHECK(fresh["state"]["participants"][0]["workload"] == 0);
        CHECK(fresh["state"]["history"]["undoCount"] == 0);
    }

    TEST_CASE("a new case becomes current and the old one stays open, untouched") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        act(s, "allocate", "S1-T1", "Thabo Nkosi");
        Json other = parse(s.loadIndustry(2, 0, ""));
        CHECK(other["state"]["industry"] == "University Admissions");
        CHECK(other["state"]["stages"].size() == 5);
        REQUIRE(other["state"]["openCases"].size() == 2);
        CHECK(other["state"]["openCases"][1]["current"] == true);
        finish(s, "S1-T1");   // work on the admissions case only
        Json back = parse(s.switchCase(0));
        REQUIRE(back["ok"] == true);
        CHECK(back["state"]["instanceId"] == "SC-1001");
        CHECK(task(back["state"], "S1-T1")["state"] == "Assigned");     // exactly as we left it
        CHECK(task(back["state"], "S1-T1")["assignedTo"] == "Thabo Nkosi");
        CHECK(back["state"]["history"]["undoCount"] == 1);              // its own history
        CHECK(parse(s.switchCase(5))["ok"] == false);
    }

    TEST_CASE("two cases of the same workflow get their own number and their own data") {
        TividyService s;
        s.loadIndustry(2, 0, "");
        Json second = parse(s.loadIndustry(2, 0, "{\"score\": 40}"));
        CHECK(second["state"]["instanceId"] == "ADM-2001-2");
        CHECK(second["state"]["caseData"]["score"] == "40");
        Json first = parse(s.switchCase(0));
        CHECK(first["state"]["caseData"]["score"] == "78");
    }
}

TEST_SUITE("reports") {

    TEST_CASE("the progress and escalation reports are text from the engine's visitors") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        finish(s, "S1-T1");
        Json p = parse(s.report("progress"));
        CHECK(p["reportType"] == "progress");
        CHECK(p["content"].get<std::string>().find("Progress") != std::string::npos);
        act(s, "allocate", "S1-T2", "Thabo Nkosi");
        act(s, "start", "S1-T2");
        act(s, "escalate", "S1-T2");
        Json e = parse(s.report("escalations"));
        CHECK(e["content"].get<std::string>().find("Escalated and rejected") != std::string::npos);
        CHECK(parse(s.report("nonsense")).contains("error"));
    }

    TEST_CASE("a report with nothing loaded is an error") {
        TividyService s;
        CHECK(parse(s.report("progress")).contains("error"));
    }
}

TEST_SUITE("a workflow built in the app") {

    const char* kHospital = R"({
        "name": "Hospital admission",
        "levels": ["Nurse", "Doctor", "Ward Manager"],
        "people": [
            {"name": "Thandi", "role": "Nurse", "capacity": 3},
            {"name": "Dr Naidoo", "role": "Doctor", "capacity": 2},
            {"name": "Mrs Venter", "role": "Ward Manager", "capacity": 5}
        ],
        "stages": [
            {"name": "Triage", "mode": "sequential", "tasks": [{"name": "Take vitals", "role": "Nurse"}]},
            {"name": "Tests", "mode": "parallel", "tasks": [
                {"name": "Blood test", "role": "Nurse"}, {"name": "X-ray review", "role": "Doctor"}]},
            {"name": "Admit", "mode": "sequential", "tasks": [{"name": "Assign a bed", "role": "Ward Manager"}]}
        ]
    })";

    TEST_CASE("a new industry is created and runs to the end on the same engine") {
        TividyService s;
        Json r = parse(s.createWorkflow(kHospital));
        REQUIRE(r["ok"] == true);
        CHECK(r["industryId"] == 3);
        CHECK(parse(s.industries()).size() == 3);
        CHECK(r["state"]["industry"] == "Hospital admission");
        CHECK(r["state"]["custom"] == true);
        const char* order[] = {"S1-T1", "S2-T1", "S2-T2", "S3-T1"};
        Json st;
        for (int i = 0; i < 4; ++i) st = finish(s, order[i])["state"];
        CHECK(st["status"] == "Completed");
        CHECK(task(st, "S2-T2")["assignedTo"] == "Dr Naidoo");    // a Doctor task starts at the Doctor level
        CHECK(task(st, "S3-T1")["assignedTo"] == "Mrs Venter");
    }

    TEST_CASE("a parallel stage in a new industry opens all its tasks together") {
        TividyService s;
        s.createWorkflow(kHospital);
        Json st = finish(s, "S1-T1")["state"];
        CHECK(task(st, "S2-T1")["state"] == "Available");
        CHECK(task(st, "S2-T2")["state"] == "Available");
    }

    TEST_CASE("a workflow that cannot run is refused with a clear message") {
        TividyService s;
        CHECK(parse(s.createWorkflow("not json"))["ok"] == false);
        CHECK(parse(s.createWorkflow(R"({"name": ""})"))["ok"] == false);
        Json noPerson = parse(s.createWorkflow(R"({"name": "X", "levels": ["Clerk", "Boss"],
            "people": [{"name": "A", "role": "Clerk"}],
            "stages": [{"name": "S", "tasks": [{"name": "T", "role": "Clerk"}]}]})"));
        CHECK(noPerson["ok"] == false);
        CHECK(noPerson["message"].get<std::string>().find("Boss") != std::string::npos);
        Json badRole = parse(s.createWorkflow(R"({"name": "X", "levels": ["Clerk"],
            "people": [{"name": "A", "role": "Clerk"}],
            "stages": [{"name": "S", "tasks": [{"name": "T", "role": "Pilot"}]}]})"));
        CHECK(badRole["ok"] == false);
        CHECK(parse(s.industries()).size() == 2);   // nothing was added
    }
}

TEST_SUITE("sign-offs (Strategy: SingleApproval, MultiLevelApproval)") {

    /// runs the support case up to "Verify fix" (S3-T2) and starts it
    void upToVerify(TividyService& s) {
        const char* order[] = {"S1-T1", "S1-T2", "S2-T1", "S2-T2", "S3-T1"};
        for (int i = 0; i < 5; ++i) REQUIRE(finish(s, order[i])["ok"] == true);
        REQUIRE(act(s, "allocate", "S3-T2")["ok"] == true);
        REQUIRE(act(s, "start", "S3-T2")["ok"] == true);
    }

    TEST_CASE("a sign-off task is finished by Approve, not Complete") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        upToVerify(s);
        Json t = task(parse(s.state()), "S3-T2");
        CHECK(allows(t, "approve"));
        CHECK(allows(t, "reject"));
        CHECK(!allows(t, "complete"));
        CHECK(t["signoffs"]["needed"] == 1);
        CHECK(act(s, "complete", "S3-T2")["ok"] == false);
        std::string who = t["assignedTo"];
        Json a = act(s, "approve", "S3-T2");
        REQUIRE(a["ok"] == true);
        CHECK(task(a["state"], "S3-T2")["state"] == "Completed");
        CHECK(task(a["state"], "S4-T1")["state"] == "Available");   // the next stage opened
        for (size_t i = 0; i < a["state"]["participants"].size(); ++i) {
            if (a["state"]["participants"][i]["name"] == who) CHECK(a["state"]["participants"][i]["workload"] == 0);
        }
    }

    TEST_CASE("rejecting a sign-off sends the work back up the chain, and undo restores it") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        upToVerify(s);
        std::string who = task(parse(s.state()), "S3-T2")["assignedTo"];
        Json r = act(s, "reject", "S3-T2");
        REQUIRE(r["ok"] == true);
        Json t = task(r["state"], "S3-T2");
        CHECK(t["state"] == "Assigned");
        CHECK(t["retryCount"] == 1);
        CHECK(t["assignedTo"] != who);
        Json u = parse(s.undo());
        CHECK(task(u["state"], "S3-T2")["state"] == "InProgress");
        CHECK(task(u["state"], "S3-T2")["assignedTo"] == who);
        CHECK(act(s, "approve", "S3-T2")["ok"] == true);   // a fresh vote after the undo
    }

    TEST_CASE("an offer of admission needs two sign-offs") {
        TividyService s;
        s.loadIndustry(2, 0, "");   // score 78: offer
        const char* order[] = {"S1-T1", "S1-T2", "S2-T1", "S2-T2", "S3-T1", "S3-T2"};
        for (int i = 0; i < 6; ++i) finish(s, order[i]);
        act(s, "allocate", "S4-T1");
        act(s, "start", "S4-T1");
        Json first = act(s, "approve", "S4-T1");
        REQUIRE(first["ok"] == true);
        CHECK(task(first["state"], "S4-T1")["state"] == "InProgress");
        CHECK(task(first["state"], "S4-T1")["signoffs"]["given"] == 1);
        CHECK(first["message"].get<std::string>().find("1 more needed") != std::string::npos);
        Json second = act(s, "approve", "S4-T1");
        CHECK(task(second["state"], "S4-T1")["state"] == "Completed");
    }
}

TEST_SUITE("the support case has a decision branch") {

    Json runSupport(TividyService& s) {
        const char* order[] = {"S1-T1", "S1-T2", "S2-T1", "S2-T2", "S3-T1"};
        for (int i = 0; i < 5; ++i) finish(s, order[i]);
        act(s, "allocate", "S3-T2");
        act(s, "start", "S3-T2");
        act(s, "approve", "S3-T2");
        finish(s, "S4-T1");
        return finish(s, "S4-T2")["state"];
    }

    TEST_CASE("a critical case gets an incident report") {
        TividyService s;
        s.loadIndustry(1, 0, "");   // SC-1001 is critical
        Json st = runSupport(s);
        CHECK(task(st, "S5-T1")["state"] == "Available");
        CHECK(task(st, "S5-T1")["when"] == "severity == critical");
        Json done = finish(s, "S5-T1");
        CHECK(done["state"]["status"] == "Completed");
        CHECK(task(done["state"], "S5-T2")["skipped"] == true);
    }

    TEST_CASE("a normal case gets a satisfaction survey") {
        TividyService s;
        s.loadIndustry(1, 0, "{\"severity\": \"normal\"}");
        Json st = runSupport(s);
        CHECK(task(st, "S5-T1")["state"] == "Created");
        CHECK(task(st, "S5-T2")["state"] == "Available");
    }
}

TEST_SUITE("a workflow built in the app: decisions, sign-offs, made-up staff") {

    const char* kClaim = R"({
        "name": "Insurance claim",
        "levels": ["Claims Clerk", "Assessor", "Claims Manager"],
        "decision": {"field": "amount", "value": 60000},
        "stages": [
            {"name": "Intake", "tasks": [{"name": "Log claim", "role": "Claims Clerk"}]},
            {"name": "Payout", "mode": "choice", "tasks": [
                {"name": "Manager approves payout", "role": "Claims Manager", "signoffs": 1,
                 "when": {"op": ">=", "value": "50000"}},
                {"name": "Pay out", "role": "Claims Clerk"}]}
        ]
    })";

    TEST_CASE("with no people listed, staff are made up for every level") {
        TividyService s;
        Json r = parse(s.createWorkflow(kClaim));
        REQUIRE(r["ok"] == true);
        Json people = r["state"]["participants"];
        REQUIRE(people.size() == 5);   // 2 clerks, 2 assessors, 1 manager
        CHECK(people[0]["name"] == "Claims Clerk 1");
        CHECK(people[4]["name"] == "Claims Manager");
        CHECK(r["state"]["instanceId"] == "IC-3001");
        CHECK(r["state"]["caseData"]["amount"] == "60000");
    }

    TEST_CASE("a big claim goes down the approval branch, a small one is paid straight away") {
        TividyService s;
        s.createWorkflow(kClaim);
        Json st = finish(s, "S1-T1")["state"];
        CHECK(task(st, "S2-T1")["state"] == "Available");
        CHECK(task(st, "S2-T1")["when"] == "amount >= 50000");
        act(s, "allocate", "S2-T1");
        act(s, "start", "S2-T1");
        CHECK(act(s, "approve", "S2-T1")["state"]["status"] == "Completed");

        Json small = parse(s.loadIndustry(3, 0, "{\"amount\": 1200}"));
        REQUIRE(small["ok"] == true);
        st = finish(s, "S1-T1")["state"];
        CHECK(task(st, "S2-T1")["state"] == "Created");
        CHECK(task(st, "S2-T2")["state"] == "Available");
    }

    TEST_CASE("a decision needs a case detail and an otherwise branch") {
        TividyService s;
        Json noField = parse(s.createWorkflow(R"({"name": "X", "levels": ["A", "B"],
            "stages": [{"name": "D", "mode": "choice", "tasks": [{"name": "T", "role": "A"}]}]})"));
        CHECK(noField["ok"] == false);
        CHECK(noField["message"].get<std::string>().find("case detail") != std::string::npos);
        Json noElse = parse(s.createWorkflow(R"({"name": "X", "levels": ["A", "B"],
            "decision": {"field": "amount", "value": 5},
            "stages": [{"name": "D", "mode": "choice", "tasks": [
                {"name": "T", "role": "A", "when": {"op": ">", "value": "1"}}]}]})"));
        CHECK(noElse["ok"] == false);
        CHECK(noElse["message"].get<std::string>().find("otherwise") != std::string::npos);
        CHECK(parse(s.industries()).size() == 2);
    }
}

TEST_SUITE("overview across every case") {

    TEST_CASE("cases are never dropped: twelve stay open") {
        TividyService s;
        for (int i = 0; i < 12; ++i) s.loadIndustry(1 + i % 2, i % 3, "");
        Json o = parse(s.overview());
        CHECK(o["cases"].size() == 12);
        CHECK(o["cases"][0]["number"] == 1);
        CHECK(parse(s.state())["openCases"].size() == 12);
    }

    TEST_CASE("work on another case without leaving the current one") {
        TividyService s;
        s.loadIndustry(1, 0, "");   // SC-1001
        s.loadIndustry(2, 0, "");   // ADM-2001 is current
        Json r = parse(s.act("allocate", "S1-T1", "", "SC-1001"));
        REQUIRE(r["ok"] == true);
        CHECK(r["state"]["instanceId"] == "ADM-2001");              // still looking at admissions
        CHECK(task(r["state"], "S1-T1")["state"] == "Available");    // admissions untouched
        Json o = parse(s.overview());
        bool assigned = false;
        for (size_t i = 0; i < o["tasks"].size(); ++i) {
            const Json& t = o["tasks"][i];
            if (t["caseId"] == "SC-1001" && t["id"] == "S1-T1") assigned = t["state"] == "Assigned";
        }
        CHECK(assigned);
        CHECK(parse(s.act("start", "S1-T1", "", "NOPE"))["ok"] == false);
    }

    TEST_CASE("the overview counts work, sign-offs and staff across cases") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        s.loadIndustry(1, 1, "");
        Json o = parse(s.overview());
        REQUIRE(o["cases"].size() == 2);
        CHECK(o["cases"][0]["awaiting"] == 1);
        CHECK(o["cases"][0]["progress"]["total"] == 9);   // 8 tasks + 1 decision
        CHECK(o["cases"][0]["workspace"] == "Customer Support");
        bool thabo = false;
        for (size_t i = 0; i < o["people"].size(); ++i) {
            if (o["people"][i]["name"] == "Thabo Nkosi") thabo = o["people"][i]["workflow"] == "Software Customer Support";
        }
        CHECK(thabo);
        CHECK(o["workspaces"].size() == 2);
    }

    TEST_CASE("tasks that have not opened say what they are waiting for") {
        TividyService s;
        s.loadIndustry(2, 0, "");
        Json st = parse(s.state());
        CHECK(task(st, "S1-T2")["waitingFor"] == "'Register application' to finish");
        CHECK(task(st, "S2-T1")["waitingFor"] == "the 'Intake' stage to finish");
        CHECK(!task(st, "S1-T1").contains("waitingFor"));
    }

    TEST_CASE("the activity log records actions, opened work and finished cases") {
        TividyService s;
        s.loadIndustry(1, 0, "");
        finish(s, "S1-T1");
        act(s, "cancel", "S1-T2");
        Json o = parse(s.overview());
        std::string kinds;
        for (size_t i = 0; i < o["activity"].size(); ++i) kinds += o["activity"][i]["kind"].get<std::string>() + " ";
        CHECK(kinds.find("case-opened") != std::string::npos);
        CHECK(kinds.find("assigned") != std::string::npos);
        CHECK(kinds.find("ready") != std::string::npos);        // S1-T2 opened
        CHECK(kinds.find("case-failed") != std::string::npos);
        CHECK(kinds.find("email") != std::string::npos);        // cancel email to the manager
        CHECK(o["activity"][0]["caseId"] == "SC-1001");
    }

    TEST_CASE("a workflow built in the app lands in its own organisation and workspace") {
        TividyService s;
        Json r = parse(s.createWorkflow(R"({"name": "Baggage handling", "organisation": "OR Tambo Airport",
            "workspace": "Airport Operations", "levels": ["Handler", "Supervisor"],
            "stages": [{"name": "Check in", "tasks": [{"name": "Tag bag", "role": "Handler"}]}]})"));
        REQUIRE(r["ok"] == true);
        CHECK(r["state"]["workspace"] == "Airport Operations");
        Json o = parse(s.overview());
        CHECK(o["workspaces"].size() == 3);
        CHECK(o["cases"][0]["organisation"] == "OR Tambo Airport");
    }
}

TEST_SUITE("saving across restarts") {

    TEST_CASE("a restarted service rebuilds exactly the same cases from the log") {
        const char* file = "test-restart-log.jsonl";
        std::remove(file);
        std::string before, beforeOther;
        {
            TividyService s;
            CHECK(s.useLog(file) == 0);
            s.loadIndustry(1, 0, "");
            finish(s, "S1-T1");
            act(s, "allocate", "S1-T2");
            act(s, "decline", "S1-T2");                       // passes up the chain
            s.createWorkflow(R"({"name": "Baggage", "organisation": "Airport", "workspace": "Ops",
                "levels": ["Handler", "Supervisor"], "stages": [{"name": "S", "tasks": [{"name": "T", "role": "Handler"}]}]})");
            s.act("allocate", "S1-T1", "", "SC-1001");         // on the other case
            s.undo();                                          // undo on the current case (nothing there yet)
            act(s, "dance", "S1-T1");                          // refused: not saved
            s.switchCase(0);
            before = parse(s.state()).dump();
            beforeOther = parse(s.overview())["cases"].dump();
        }
        TividyService again;
        CHECK(again.useLog(file) > 5);
        CHECK(parse(again.state()).dump() == before);
        CHECK(parse(again.overview())["cases"].dump() == beforeOther);
        CHECK(parse(again.industries()).size() == 3);           // the workflow built in the app is back
        CHECK(parse(again.reset())["ok"] == true);
        CHECK(parse(again.overview())["cases"].empty());
        TividyService third;
        CHECK(third.useLog(file) == 0);                        // reset emptied the log
        std::remove(file);
    }
}
