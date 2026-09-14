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
        if (Fact("fact_campaign_completed")) return "Arrival: revisit a floor or return to Roof";
        if (Fact("fact_elevator_authority_unlocked")) return "Lift: select 36F Authority Core";
        if (Fact("fact_elevator_executive_unlocked")) return "Lift: select 30F Executive Archive";
        if (Fact("fact_elevator_transfer_unlocked")) return "Lift: select 24F Security Transfer";
        if (Fact("fact_act3_operations_accessed")) return "Lift: select 18F Network Node";
        if (Fact("fact_act3_records_accessed")) return "Lift: select 12F Operations Control";
        return Fact("fact_elevator_records_unlocked")
            ? "Lift: select 08F Records Core; find your file"
            : "Arrival: ask the clerk to open Records access";
    }
    if (room_id == "room_8f_records_core") {
        return Fact("fact_act3_records_accessed")
                   ? "Records: return to Arrival; select 12F Operations"
                   : "Records: query the Subject 07 file";
    }
    if (room_id == "room_12f_operations_control") {
        return Fact("fact_act3_operations_accessed")
                   ? "Operations: return to Arrival; check unlocked floors"
                   : "Operations: reconcile the facility response";
    }
    if (room_id == "room_18f_network_node") {
        return Fact("fact_act3_network_discovered")
                   ? "Network: return to the lift"
                   : "Network: find the unlisted observation route";
    }
    if (room_id == "room_24f_security_transfer") {
        return Fact("fact_act4_transfer_reached")
                   ? "Transfer: return to Arrival; select 30F Archive"
                   : "Transfer: cross the upper security gate";
    }
    if (room_id == "room_30f_executive_archive") {
        return Fact("fact_act4_archive_opened")
                   ? "Archive: return to Arrival; select 36F Authority"
                   : "Archive: open the sealed executive record";
    }
    if (room_id == "room_36f_authority_core") {
        if (Fact("fact_campaign_completed")) return "Decision recorded. Return to Arrival or Roof.";
        return Fact("fact_pre_final_checkpoint")
                   ? "Authority: choose what the record becomes"
                   : "Authority: reach the decision terminal";
    }
    if (room_id == "room_roof_exit") {
        return Fact("fact_campaign_completed") ? "Campaign complete. You may stay, return or quit."
                                               : "Epilogue: read the last status";
    }
    return {};
}

std::vector<std::string> TowerCampaignRuntime::DirectoryRows(
    std::string_view current_room, size_t selected_index) const {
    std::vector<std::string> rows{"TOWER DIRECTORY / 41 LEVELS", ""};
    const auto selectable = SelectableDestinations(current_room);
    const size_t selected = selectable.empty() ? destinations_.size()
        : selectable[std::min(selected_index, selectable.size() - 1)];
    if (selected < destinations_.size()) {
        rows[1] = "> SELECTED: " + destinations_[selected].display_name;
    }
    for (size_t i = 0; i < destinations_.size(); ++i) {
        const auto& stop = destinations_[i];
        const std::string floor = stop.display_floor == 0 ? "RF"
            : (stop.display_floor < 10 ? "0" : "") + std::to_string(stop.display_floor);
        rows.push_back(std::string(i == selected ? "> " : "  ") + floor + "  " +
            stop.display_name + "  [" + DestinationStatus(stop, current_room) + "]");
    }
    rows.push_back("");
    rows.push_back("Only available stops can be selected. Access follows your filed route.");
    rows.push_back("Other levels: B4 sealed / B3 no stop / B2 restricted / B1 clinical.");
    rows.push_back("W/S SELECT   F TRAVEL   ESC CLOSE");
    return rows;
}

std::string TowerCampaignRuntime::CaseFile(std::string_view current_room,
                                           size_t knowledge_count) const {
    std::ostringstream out;
    out << "CASE FILE / SUBJECT 07\n" << Objective(current_room) << "\n";
    if (Fact("fact_campaign_completed")) {
        out << "RESOLVED: " << (Fact("fact_ending_disclose") ? "DISCLOSE"
            : Fact("fact_ending_breach") ? "BREACH" : "AMEND") << ". The decision is on record.\n";
    } else if (!Fact("fact_act3_records_accessed")) {
        out << "LEAD: follow the service route to the Subject 07 records.\n";
    } else if (!Fact("fact_act3_operations_accessed")) {
        out << "LEAD: Operations can reconcile the events missing from your file.\n";
    } else if (!Fact("fact_act4_transfer_reached")) {
        out << "LEAD: a filed route or force gets you through upper Security.\n";
    } else if (!Fact("fact_act4_archive_opened")) {
        out << "LEAD: the Executive Archive holds the authority behind your number.\n";
    } else if (!Fact("fact_act4_authority_ready")) {
        out << "LEAD: take that authority to 36F. The record can be changed there.\n";
    } else {
        out << "LEAD: review the available resolutions at the decision terminal.\n";
    }
    if (Fact("fact_act3_network_discovered")) {
        out << "EVIDENCE: the unlisted feed preserves what the official record omits.\n";
    }
    if (Fact("fact_act3_operations_cooperated")) {
        out << "ROUTE: Operations filed your transfer; Security can accept it.\n";
    }
    if (Fact("fact_act3_force_route") || Fact("fact_act4_security_alerted")) {
        out << "TRACE: forced access remains on the facility record.\n";
    }
    out << "FILED EVIDENCE: " << knowledge_count << "\nESC / F CLOSE";
    return out.str();
}

std::vector<TowerCampaignRuntime::EndingOption>
TowerCampaignRuntime::EligibleEndings() const {
    std::vector<EndingOption> options;
    if (Fact("fact_campaign_completed")) return options;
    if (Fact("fact_act4_authority_ready")) {
        options.push_back({Ending::Amend, "AMEND",
                           "File your account as the official record. The facility retains custody."});
    }
    if (Fact("fact_act4_authority_ready") &&
        Fact("fact_act3_network_discovered") &&
        Fact("fact_act3_operations_cooperated")) {
        options.push_back({Ending::Disclose, "DISCLOSE",
                           "Publish the record and the surviving feed beyond the facility's control."});
    }
    if (Fact("fact_act4_transfer_reached") &&
        (Fact("fact_act3_force_route") || Fact("fact_act4_security_alerted"))) {
        options.push_back({Ending::Breach, "BREACH",
                           "Use forced access to leave. Refuse to certify the institution's account."});
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
