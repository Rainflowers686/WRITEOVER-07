#pragma once

#include "writeover/systemic/systemic.h"

#include <algorithm>
#include <string>
#include <vector>

namespace writeover {

// A bounded projection of already-acquired knowledge, not a journal database.
// IDs are the existing campaign writers' durable KnowledgeAssetIds. The final
// authority note is learned at the existing pre-final checkpoint interaction.
inline std::vector<std::string> PlayerKeyRecords(
    const SystemicWorld& world, EntityId player, bool authority_reviewed) {
    struct Record { uint64_t id; const char* title; const char* summary; };
    static constexpr Record records[] = {
        {9301, "TRANSFER MANIFEST", "The records operator described Transit Control access."},
        {9302, "OBSERVATION LOOP", "The analyst identified a blind spot in the transit camera feed."},
        {9401, "SUBJECT 07", "Release was denied before revival. The review order came from Authority on 36F."},
        {9402, "OPERATIONS ROUTE", "Operations recorded a route upward. Existing camera evidence was not erased."},
        {9403, "UNLISTED FEED", "An omitted observation feed survives outside the official account."},
        {9404, "TRANSFER CLEARANCE", "The upper security transfer was recorded."},
        {9405, "EXECUTIVE ORDER 07", "Your signature is required to close the file. The order transfers review authority to you."},
    };
    std::vector<std::string> rows{"KEY RECORDS"};
    for (const auto& record : records) {
        const auto* asset = world.GetKnowledgeAsset(KnowledgeAssetId::New(record.id));
        if (!asset || std::find(asset->known_by.begin(), asset->known_by.end(), player) == asset->known_by.end()) continue;
        rows.push_back(record.title);
        rows.push_back(record.summary);
    }
    const bool knows_shift = std::any_of(world.Knowledge().begin(), world.Knowledge().end(),
        [&](const KnowledgeAssetRecord& asset) {
            return asset.type == KnowledgeAssetType::ShiftSchedule &&
                std::find(asset.known_by.begin(), asset.known_by.end(), player) != asset.known_by.end();
        });
    if (knows_shift) {
        rows.push_back("STAFF SHIFT NOTE");
        rows.push_back("Maintenance changes shift at 02:10.");
    }
    if (authority_reviewed) {
        rows.push_back("AUTHORITY REVIEW");
        rows.push_back("The decision terminal is ready. Review the available choices before confirming; confirmation changes the record.");
    }
    if (rows.size() == 1) rows.push_back("No key records acquired yet.");
    return rows;
}

} // namespace writeover
