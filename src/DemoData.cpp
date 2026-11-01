#include "DemoData.h"
#include "Participant.h"

namespace DemoData {

namespace {

TaskSpec task(const std::string& name, const std::string& role, int priority, bool audit) {
    TaskSpec t;
    t.name = name;
    t.role = role;
    t.priority = priority;
    t.audit = audit;
    return t;
}

//a task for a choice stage: it runs when data[key] <op> value
TaskSpec branch(const std::string& name, const std::string& role, int priority, bool audit,
                const std::string& key, const std::string& op, const std::string& value) {
    TaskSpec t = task(name, role, priority, audit);
    t.whenKey = key;
    t.whenOp = op;
    t.whenValue = value;
    return t;
}

StageSpec stage(const std::string& name, Mode mode) {
    StageSpec s;
    s.name = name;
    s.mode = mode;
    return s;
}

}

// ---- Industry 1: software customer support ----

WorkflowSpec makeSupportSpec() {
    WorkflowSpec spec;
    spec.name = "Software customer support";

    StageSpec intake = stage("Intake", SEQUENTIAL);
    intake.tasks.push_back(task("Log support case", "General Assistant", 1, true));
    intake.tasks.push_back(task("Classify severity", "General Assistant", 1, false));
    spec.stages.push_back(intake);

    StageSpec investigation = stage("Investigation", SEQUENTIAL);
    investigation.tasks.push_back(task("Reproduce issue", "Technical Assistant", 2, false));
    investigation.tasks.push_back(task("Diagnose root cause", "Engineer", 3, true));
    spec.stages.push_back(investigation);

    StageSpec resolution = stage("Resolution", SEQUENTIAL);
    resolution.tasks.push_back(task("Implement fix", "Engineer", 3, true));
    resolution.tasks.push_back(task("Verify fix", "Technical Assistant", 2, false));
    resolution.tasks.back().signoffs = 2;   //the fix is signed off by two people above the tester (MultiLevelApproval)
    spec.stages.push_back(resolution);

    StageSpec closure = stage("Closure", SEQUENTIAL);
    closure.tasks.push_back(task("Confirm resolution with customer", "General Assistant", 1, false));
    closure.tasks.push_back(task("Close support case", "Support Manager", 4, true));
    spec.stages.push_back(closure);

    //decision branch: a critical case gets an incident report, any other case a survey
    StageSpec followUp = stage("Follow-up", CHOICE);
    followUp.tasks.push_back(branch("Write incident report", "Engineer", 3, true, "severity", "==", "critical"));
    followUp.tasks.push_back(branch("Send satisfaction survey", "General Assistant", 1, false, "", "", ""));
    spec.stages.push_back(followUp);

    return spec;
}

std::vector<Participant*> createSupportTeam() {
    std::vector<Participant*> team;
    team.push_back(new Participant("Thabo Nkosi", "General Assistant", 3));
    team.push_back(new Participant("Ayesha Patel", "General Assistant", 3));
    team.push_back(new Participant("Lerato Mokoena", "Technical Assistant", 2));
    team.push_back(new Participant("Pieter van Wyk", "Technical Assistant", 2));
    team.push_back(new Participant("Sipho Dlamini", "Engineer", 2));
    team.push_back(new Participant("Naledi Khumalo", "Support Manager", 5));
    return team;
}

std::vector<SupportCase> sampleSupportCases() {
    std::vector<SupportCase> cases;
    cases.push_back({"SC-1001", "Mamelodi Pharmacy", "StockTrack POS", "Till freezes when scanning barcodes", "critical"});
    cases.push_back({"SC-1002", "Hatfield Tutors", "LearnHub", "Students cannot reset their passwords", "normal"});
    cases.push_back({"SC-1003", "Menlyn Logistics", "RouteWise", "Delivery report exports an empty file", "normal"});
    return cases;
}

// ---- Industry 2: university admissions ----

WorkflowSpec makeAdmissionsSpec() {
    WorkflowSpec spec;
    spec.name = "University admissions";

    StageSpec intake = stage("Intake", SEQUENTIAL);
    intake.tasks.push_back(task("Register application", "Admissions Clerk", 1, true));
    intake.tasks.push_back(task("Acknowledge receipt to applicant", "Admissions Clerk", 1, false));
    spec.stages.push_back(intake);

    //the two checks are independent, so they run at the same time
    StageSpec verification = stage("Verification", PARALLEL);
    verification.tasks.push_back(task("Verify submitted documents", "Verification Officer", 2, false));
    verification.tasks.push_back(task("Verify applicant identity", "Verification Officer", 2, false));
    spec.stages.push_back(verification);

    StageSpec assessment = stage("Assessment", SEQUENTIAL);
    assessment.tasks.push_back(task("Review academic record", "Faculty Reviewer", 3, true));
    assessment.tasks.push_back(task("Calculate admission score", "Faculty Reviewer", 3, false));
    spec.stages.push_back(assessment);

    //first matching task wins; the task with no condition is the default branch
    StageSpec decision = stage("Decision", CHOICE);
    decision.tasks.push_back(branch("Issue offer of admission", "Faculty Reviewer", 3, true, "score", ">=", "70"));
    decision.tasks.back().signoffs = 1;   //a faculty reviewer prepares the offer; the Registrar signs it off
    decision.tasks.push_back(branch("Place applicant on waiting list", "Admissions Clerk", 1, false, "score", ">=", "55"));
    decision.tasks.push_back(branch("Send rejection notice", "Registrar", 4, true, "", "", ""));
    spec.stages.push_back(decision);

    //only an applicant who got an offer is enrolled; everyone else is archived
    StageSpec enrolment = stage("Enrolment", CHOICE);
    enrolment.tasks.push_back(branch("Confirm acceptance and register student", "Registrar", 4, true, "score", ">=", "70"));
    enrolment.tasks.push_back(branch("Archive application file", "Admissions Clerk", 1, false, "", "", ""));
    spec.stages.push_back(enrolment);

    return spec;
}

std::vector<Participant*> createAdmissionsTeam() {
    std::vector<Participant*> team;
    team.push_back(new Participant("Zanele Mahlangu", "Admissions Clerk", 4));
    team.push_back(new Participant("Johan Pretorius", "Admissions Clerk", 4));
    team.push_back(new Participant("Nomsa Radebe", "Verification Officer", 3));
    team.push_back(new Participant("Tebogo Sithole", "Verification Officer", 3));
    team.push_back(new Participant("Dr Maryam Cassim", "Faculty Reviewer", 2));
    team.push_back(new Participant("Prof Andile Ngcobo", "Faculty Reviewer", 2));
    team.push_back(new Participant("Busisiwe Moloi", "Registrar", 5));
    return team;
}

std::vector<AdmissionsApplication> sampleApplications() {
    std::vector<AdmissionsApplication> applications;
    applications.push_back({"ADM-2001", "Kagiso Molefe", "BSc Computer Science", 78});   //offer
    applications.push_back({"ADM-2002", "Anri Joubert", "BCom Accounting", 62});         //waiting list
    applications.push_back({"ADM-2003", "Sandile Zulu", "BEng Civil Engineering", 48});  //rejection
    return applications;
}

}