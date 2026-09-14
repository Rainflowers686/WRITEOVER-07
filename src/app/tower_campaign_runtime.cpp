#include "src/app/tower_campaign_runtime.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace writeover {

TowerCampaignRuntime::TowerCampaignRuntime(FactReader fact_reader)
    : fact_reader_(std::move(fact_reader)) {
    destinations_ = {
        {1, "Arrival / Public Lobby", "room_1f_arrival_lobby", ""},
        {8, "Records Core", "room_8f_records_core", "fact_elevator_records_unlocked"},
        {12, "Operations Control", "room_12f_operations_control", "fact_elevator_operations_unlocked"},
        {18, "Network Node", "room_18f_network_node", "fact_elevator_network_unlocked"},
        {24, "Security Transfer", "room_24f_security_transfer", "fact_elevator_transfer_unlocked"},
        {30, "Executive Archive", "room_30f_executive_archive", "fact_elevator_executive_unlocked"},
        {36, "Authority Core", "room_36f_authority_core", "fact_elevator_authority_unlocked"},
        {0, "Roof / Exit", "room_roof_exit", "fact_elevator_roof_unlocked"},
    };
}

bool TowerCampaignRuntime::Fact(std::string_view name) const {
    return fact_reader_ != nullptr && fact_reader_(name);
}

bool TowerCampaignRuntime::DestinationUnlocked(const Destination& destination) const {
    return destination.unlock_fact.empty() || Fact(destination.unlock_fact);
}

std::vector<size_t> TowerCampaignRuntime::SelectableDestinations(
    std::string_view current_room) const {
    std::vector<size_t> result;
    for (size_t i = 0; i < destinations_.size(); ++i) {
        const Destination& destination = destinations_[i];
        if (destination.room_id == current_room || DestinationUnlocked(destination)) {
            result.push_back(i);
        }
    }
    return result;
}

std::string TowerCampaignRuntime::DestinationStatus(
    const Destination& destination, std::string_view current_room) const {
    if (destination.room_id == current_room) return "CURRENT";
    if (Fact("fact_campaign_completed") && destination.room_id == "room_roof_exit") {
        return "AVAILABLE";
    }
    if (DestinationUnlocked(destination)) return "AVAILABLE";
    if (destination.room_id == "room_18f_network_node" &&
        !Fact("fact_act3_records_accessed")) {
        return "RESTRICTED";
    }
    if (destination.room_id == "room_36f_authority_core" &&
        !Fact("fact_act4_archive_opened")) {
        return "SEALED";
    }
    return "LOCKED";
}

std::string TowerCampaignRuntime::DirectoryLine(std::string_view current_room,
                                                size_t selected_index) const {
    const std::vector<size_t> selectable = SelectableDestinations(current_room);
    if (selectable.empty()) return "LIFT DIRECTORY / NO VALID STOPS";
    const size_t bounded = std::min(selected_index, selectable.size() - 1);
    const Destination& destination = destinations_[selectable[bounded]];
    std::ostringstream out;
    // The playable destinations are intentionally sparse, but the diegetic
    // directory must still communicate the scale of the facility.  Keep this
    // as presentation text rather than inventing a simulated floor graph.
    out << "TOWER DIRECTORY / 41 LEVELS | B4:SEALED B3:NO-STOP B2:RESTRICTED"
        << " B1:CLINICAL | 01:ARRIVAL 04:NO-STOP 08:RECORDS 12:OPS 18:NET"
        << " 24:SEC 30:EXEC 36:AUTH RF:EXIT | SELECTED: "
        << destination.display_name << " [" << DestinationStatus(destination, current_room)
        << "] | W/S SELECT F CONFIRM ESC CLOSE";
    return out.str();
}

std::string TowerCampaignRuntime::Objective(std::string_view room_id) const {
    if (room_id == "room_1f_arrival_lobby") {
        return "Arrival: inspect the lift directory";
    }
    if (room_id == "room_8f_records_core") {
        return Fact("fact_act3_records_accessed")
                   ? "Records: follow the authority trail"
                   : "Records: query the Subject 07 file";
    }
    if (room_id == "room_12f_operations_control") {
        return Fact("fact_act3_operations_accessed")
                   ? "Operations: open a route above security"
                   : "Operations: reconcile the facility response";
    }
    if (room_id == "room_18f_network_node") {
        return Fact("fact_act3_network_discovered")
                   ? "Network: return to the lift"
                   : "Network: find the unlisted observation route";
    }
    if (room_id == "room_24f_security_transfer") {
        return Fact("fact_act4_transfer_reached")
                   ? "Transfer: reach the executive archive"
                   : "Transfer: cross the upper security gate";
    }
    if (room_id == "room_30f_executive_archive") {
        return Fact("fact_act4_archive_opened")
                   ? "Archive: take the authority route"
                   : "Archive: open the sealed executive record";
    }
    if (room_id == "room_36f_authority_core") {
        return Fact("fact_pre_final_checkpoint")
                   ? "Authority: choose what the record becomes"
                   : "Authority: reach the decision terminal";
    }
    if (room_id == "room_roof_exit") {
        return "Epilogue: read the last status";
    }
    return {};
}

std::string TowerCampaignRuntime::CaseFile(std::string_view current_room,
                                           size_t knowledge_count) const {
    std::ostringstream out;
    out << "CASE FILE  " << Objective(current_room);
    if (!Fact("fact_act3_records_accessed")) {
        out << "  LEAD: Subject 07 record is still incomplete.";
    } else if (!Fact("fact_act3_authority_lead")) {
        out << "  LEAD: the authorization chain ends above the archive.";
    } else if (!Fact("fact_act4_authority_ready")) {
        out << "  LEAD: reach the Authority Core before deciding.";
    } else {
        out << "  LEAD: the record can now be amended.";
    }
    if (Fact("fact_act3_network_discovered")) {
        out << "  DISCOVERY: an unlisted observation feed can disclose the record.";
    }
    if (Fact("fact_act3_operations_cooperated")) {
        out << "  DISCOVERY: operations left a quiet route through transfer.";
    }
    if (Fact("fact_act3_force_route") || Fact("fact_act4_security_alerted")) {
        out << "  DISCOVERY: force remains possible, but it will leave a trace.";
    }
    out << "  EVIDENCE " << knowledge_count << ".";
    return out.str();
}

std::vector<TowerCampaignRuntime::EndingOption>
TowerCampaignRuntime::EligibleEndings() const {
    std::vector<EndingOption> options;
    if (Fact("fact_act4_authority_ready")) {
        options.push_back({Ending::Amend, "AMEND",
                           "Reconcile Subject 07 with the official record."});
    }
    if (Fact("fact_act4_authority_ready") &&
        Fact("fact_act3_network_discovered") &&
        Fact("fact_act3_operations_cooperated")) {
        options.push_back({Ending::Disclose, "DISCLOSE",
                           "Release the evidence beyond the facility."});
    }
    if (Fact("fact_act4_transfer_reached") &&
        (Fact("fact_act3_force_route") || Fact("fact_act4_security_alerted"))) {
        options.push_back({Ending::Breach, "BREACH",
                           "Force an exit and leave the sealed system behind."});
    }
    return options;
}

const char* TowerCampaignRuntime::EndingFact(Ending ending) {
    switch (ending) {
    case Ending::Amend: return "fact_ending_amend";
    case Ending::Disclose: return "fact_ending_disclose";
    case Ending::Breach: return "fact_ending_breach";
    }
    return "fact_campaign_completed";
}

const char* TowerCampaignRuntime::EndingLabel(Ending ending) {
    switch (ending) {
    case Ending::Amend: return "AMEND";
    case Ending::Disclose: return "DISCLOSE";
    case Ending::Breach: return "BREACH";
    }
    return "UNKNOWN";
}

}  // namespace writeover
