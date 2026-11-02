#ifndef DEMODATA_H
#define DEMODATA_H

#include <string>
#include <vector>
#include "WorkflowSpec.h"

class Participant;

/// Test and demo data for the live demo: the sample teams, cases and workflow
/// descriptions for BOTH scenarios (software customer support and university
/// admissions) in one place. Plain functions, not a pattern: this is data the GUI
/// and the tests load. Nothing here is part of the engine. Each make...Spec()
/// returns the WorkflowSpec that the one generic WorkflowDirector turns into a
/// definition.
namespace DemoData {

    /// One incoming support case (the input data for a workflow instance).
    struct SupportCase {
        std::string caseId;
        std::string customer;
        std::string product;
        std::string summary;
        std::string severity;   ///< "critical" or "normal": decides the Follow-up branch
    };

    /// One incoming university application. Start an instance with
    /// setData("score", std::to_string(score)): the Decision and Enrolment stages
    /// choose their branch from it (>= 70 offer, >= 55 waiting list, otherwise reject).
    struct AdmissionsApplication {
        std::string applicationId;
        std::string applicant;
        std::string programme;
        int score;
    };

    // ---- Industry 1: software customer support ----

    /// The support process: Intake, Investigation, Resolution (the fix is verified by an approval),
    /// Closure, then Follow-up (choice on severity: critical gets an incident report, others a survey).
    /// Task priorities match the escalation levels: 1 = General Assistant,
    /// 2 = Technical Assistant, 3 = Engineer, 4 = Support Manager.
    WorkflowSpec makeSupportSpec();

    /// The support team. The caller owns the returned participants.
    /// Roles match makeSupportSpec() and the escalation chain.
    std::vector<Participant*> createSupportTeam();

    /// Realistic sample cases to start instances with.
    std::vector<SupportCase> sampleSupportCases();

    // ---- Industry 2: university admissions ----

    /// The admissions process: Intake, Verification (parallel), Assessment,
    /// Decision (choice on score) and Enrolment (choice on score).
    /// Task priorities follow the same escalation levels: 1 = Admissions Clerk,
    /// 2 = Verification Officer, 3 = Faculty Reviewer, 4 = Registrar.
    WorkflowSpec makeAdmissionsSpec();

    /// The admissions office. The caller owns the returned participants.
    std::vector<Participant*> createAdmissionsTeam();

    /// Sample applications: one offer, one waiting list, one rejection.
    std::vector<AdmissionsApplication> sampleApplications();

}

#endif //DEMODATA_H