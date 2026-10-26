#include "SupportData.h"
#include "Participant.h"

namespace SupportData {

std::vector<Participant*> createTeam() {
    std::vector<Participant*> team;
    team.push_back(new Participant("Thabo Nkosi", "General Assistant", 3));
    team.push_back(new Participant("Ayesha Patel", "General Assistant", 3));
    team.push_back(new Participant("Lerato Mokoena", "Technical Assistant", 2));
    team.push_back(new Participant("Pieter van Wyk", "Technical Assistant", 2));
    team.push_back(new Participant("Sipho Dlamini", "Engineer", 2));
    team.push_back(new Participant("Naledi Khumalo", "Support Manager", 5));
    return team;
}

std::vector<SupportCase> sampleCases() {
    std::vector<SupportCase> cases;
    cases.push_back({"SC-1001", "Mamelodi Pharmacy", "StockTrack POS", "Till freezes when scanning barcodes"});
    cases.push_back({"SC-1002", "Hatfield Tutors", "LearnHub", "Students cannot reset their passwords"});
    cases.push_back({"SC-1003", "Menlyn Logistics", "RouteWise", "Delivery report exports an empty file"});
    return cases;
}

}
