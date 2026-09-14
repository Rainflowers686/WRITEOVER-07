#include "tests/test_harness.h"

#include "src/app/tower_campaign_runtime.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_set>

namespace writeover {
namespace {

bool HasEnding(const std::vector<TowerCampaignRuntime::EndingOption>& options,
               TowerCampaignRuntime::Ending ending) {
    return std::any_of(options.begin(), options.end(),
                       [ending](const auto& option) {
                           return option.ending == ending;
                       });
}

bool TestTowerDestinationPolicy() {
    std::unordered_set<std::string> facts;
    TowerCampaignRuntime campaign([&](std::string_view fact) {
        return facts.count(std::string(fact)) != 0;
    });

    WO_CHECK_EQ(campaign.Destinations().size(), 8);
    WO_CHECK_EQ(campaign.SelectableDestinations(
                    "room_1f_arrival_lobby").size(), 1);
    const std::string initial_directory =
        campaign.DirectoryLine("room_1f_arrival_lobby", 0);
    WO_CHECK(initial_directory.find("41 LEVELS") != std::string::npos);
    WO_CHECK(initial_directory.find("Arrival / Public Lobby") !=
             std::string::npos);

    facts.insert("fact_elevator_records_unlocked");
    facts.insert("fact_elevator_operations_unlocked");
    const auto selectable = campaign.SelectableDestinations(
        "room_1f_arrival_lobby");
    WO_CHECK_EQ(selectable.size(), 3);
    WO_CHECK(campaign.DirectoryLine("room_1f_arrival_lobby", 2).find(
                 "Operations Control") != std::string::npos);
    WO_CHECK(!campaign.DestinationUnlocked(campaign.Destinations().at(3)));

    facts.insert("fact_elevator_network_unlocked");
    WO_CHECK(campaign.DestinationUnlocked(campaign.Destinations().at(3)));
    WO_CHECK(campaign.Objective("room_8f_records_core").find("Subject 07") !=
             std::string::npos);
    return true;
}

bool TestTowerEndingPolicy() {
    std::unordered_set<std::string> facts;
    TowerCampaignRuntime campaign([&](std::string_view fact) {
        return facts.count(std::string(fact)) != 0;
    });

    WO_CHECK(campaign.EligibleEndings().empty());
    facts.insert("fact_act4_authority_ready");
    auto options = campaign.EligibleEndings();
    WO_CHECK_EQ(options.size(), 1);
    WO_CHECK(HasEnding(options, TowerCampaignRuntime::Ending::Amend));
    WO_CHECK(!HasEnding(options, TowerCampaignRuntime::Ending::Disclose));
    WO_CHECK(!HasEnding(options, TowerCampaignRuntime::Ending::Breach));

    facts.insert("fact_act3_network_discovered");
    facts.insert("fact_act3_operations_cooperated");
    options = campaign.EligibleEndings();
    WO_CHECK_EQ(options.size(), 2);
    WO_CHECK(HasEnding(options, TowerCampaignRuntime::Ending::Disclose));

    facts.insert("fact_act4_transfer_reached");
    facts.insert("fact_act4_security_alerted");
    options = campaign.EligibleEndings();
    WO_CHECK_EQ(options.size(), 3);
    WO_CHECK(HasEnding(options, TowerCampaignRuntime::Ending::Breach));
    WO_CHECK(std::string(TowerCampaignRuntime::EndingFact(
                  TowerCampaignRuntime::Ending::Disclose)) ==
             "fact_ending_disclose");
    WO_CHECK(std::string(TowerCampaignRuntime::EndingLabel(
                  TowerCampaignRuntime::Ending::Breach)) == "BREACH");
    return true;
}

}  // namespace

void RegisterCampaignTests(TestHarness& harness) {
    harness.Add("tower destination policy", &TestTowerDestinationPolicy);
    harness.Add("tower ending policy", &TestTowerEndingPolicy);
}

}  // namespace writeover
