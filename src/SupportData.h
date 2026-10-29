#ifndef SUPPORTDATA_H
#define SUPPORTDATA_H

#include <string>
#include <vector>

class Participant;

/// Industry 1 test data: the support desk team and some sample support cases.
/// Plain functions, not a pattern: this is data the TUI and the tests load.
namespace SupportData {

    /// One incoming support case (the input data for a workflow instance).
    struct SupportCase {
        std::string caseId;
        std::string customer;
        std::string product;
        std::string summary;
    };

    /// The support team. The caller owns the returned participants.
    /// Roles match the SupportDirector task roles and the escalation chain.
    std::vector<Participant*> createTeam();

    /// Realistic sample cases to start instances with.
    std::vector<SupportCase> sampleCases();

}

#endif //SUPPORTDATA_H
