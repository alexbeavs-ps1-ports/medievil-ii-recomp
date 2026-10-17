/* Host-only evidence, never replay input or part of a simulation identity.
 * All guest reads are passive, bounded and restricted to the USA retail
 * layout. A nearest actor is a spatial candidate, not proof of an enemy/hit.
 * No allocation, formatting, file I/O or guest writes in this sampler. */
#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include "mod_plugins.h"

namespace medievil2_diagnostic {
constexpr unsigned TelemetryColumns = 36;
using TelemetryRow = std::array<int64_t, TelemetryColumns>;
constexpr const char *TelemetryHeader =
    "sequence,host_elapsed_us,host_interval_us,retail_layout,game_vblank,game_updates,game_state,game_flags,"
    "player,player_state,health,grace,player_x,player_y,player_z,player_m00,player_m02,"
    "near_actor,near_kind,near_health,near_grace,near_x,near_y,near_z,distance_squared,actor_scan_capped,"
    "incident,cd_reading,cd_pending,cd_lba,cd_data_read,cd_xa,render_passes,render_aborted,render_verify_checks,render_verify_mismatch\n";
inline bool retail_actor(uint32_t address) {
    return !(address & 3) && address >= 0x80010000u && address <= 0x801FFE00u;
}
inline int16_t signed_half(uint32_t address) {
    return static_cast<int16_t>(psx_mod_read_word(address & ~3u) >> ((address & 2) * 8));
}
inline int8_t actor_state(uint32_t address) {
    return static_cast<int8_t>(psx_mod_read_word(address + 0x38) >> 24);
}
inline int32_t position(uint32_t address, unsigned axis) {
    return static_cast<int32_t>(psx_mod_read_word(address + 0x24 + axis * 4));
}
inline void sample_retail(TelemetryRow& row) {
    if (psx_mod_read_word(0x8004FB9Cu) != 0x8F840584u) return;
    row[3] = 1;
    row[4] = psx_mod_read_word(0x800EF324u);
    row[5] = psx_mod_read_word(0x800EFF2Cu);
    row[6] = psx_mod_read_word(0x800F0604u);
    row[7] = psx_mod_read_word(0x800EFF08u);
    const uint32_t player = psx_mod_read_word(0x800EEFD8u);
    if (!retail_actor(player)) return;
    row[8] = player; row[9] = actor_state(player);
    row[10] = psx_mod_read_word(0x800F152Cu);
    row[11] = signed_half(player + 0x60);
    for (unsigned axis = 0; axis < 3; ++axis) row[12 + axis] = position(player, axis);
    row[15] = signed_half(player + 0x10); row[16] = signed_half(player + 0x14);
    if (row[6] || (row[7] & 0x3FFu) || row[9] < 0) return;
    uint32_t actor = psx_mod_read_word(0x800EFC60u);
    uint64_t closest = UINT64_MAX;
    unsigned count = 0;
    for (; count < 64 && actor != 0x800EFC5Cu && retail_actor(actor); ++count) {
        const int health = signed_half(actor + 0x62);
        if (actor != player && health > 0 && actor_state(actor) >= 0) {
            // Corrupt/extreme coordinates cannot overflow signed arithmetic.
            int64_t distance = 0;
            for (unsigned axis = 0; axis < 3; ++axis) {
                const int64_t delta = int64_t(position(actor, axis)) - row[12 + axis];
                if (delta < -1000000 || delta > 1000000) { distance = INT64_MAX; break; }
                distance += delta * delta;
            }
            if (uint64_t(distance) < closest) {
                closest = uint64_t(distance);
                row[17] = actor; row[18] = psx_mod_read_word(actor + 8);
                row[19] = health; row[20] = signed_half(actor + 0x60);
                for (unsigned axis = 0; axis < 3; ++axis) row[21 + axis] = position(actor, axis);
                row[24] = distance;
            }
        }
        actor = psx_mod_read_word(actor + 4);
    }
    row[25] = count == 64 && actor != 0x800EFC5Cu && retail_actor(actor);
}
}
