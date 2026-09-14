#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace writeover {

// Private application policy for the bounded vertical campaign.  This is
// deliberately not a new campaign/act/region engine: all durable progression
// still lives in the existing fact store, systemic knowledge and save state.
class TowerCampaignRuntime final {
public:
    using FactReader = std::function<bool(std::string_view)>;

    enum class Ending {
        Amend,
        Disclose,
        Breach,
    };

    struct Destination {
        int display_floor = 0;
        std::string display_name;
        std::string room_id;
        std::string unlock_fact;
    };

    struct EndingOption {
        Ending ending = Ending::Amend;
        std::string title;
        std::string summary;
    };

    explicit TowerCampaignRuntime(FactReader fact_reader);

    const std::vector<Destination>& Destinations() const { return destinations_; }
    std::vector<size_t> SelectableDestinations(std::string_view current_room) const;
    bool DestinationUnlocked(const Destination& destination) const;

    std::string Objective(std::string_view room_id) const;
    std::string DirectoryLine(std::string_view current_room,
                              size_t selected_index) const;
    std::vector<std::string> DirectoryRows(std::string_view current_room,
                                           size_t selected_index) const;
    std::string CaseFile(std::string_view current_room,
                         size_t knowledge_count) const;

    std::vector<EndingOption> EligibleEndings() const;
    static const char* EndingFact(Ending ending);
    static const char* EndingLabel(Ending ending);

private:
    bool Fact(std::string_view name) const;
    std::string DestinationStatus(const Destination& destination,
                                  std::string_view current_room) const;

    FactReader fact_reader_;
    std::vector<Destination> destinations_;
};

}  // namespace writeover
