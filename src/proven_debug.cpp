#include "native.h"
#include "Extension/Skater/free_flight.h"
namespace dingosdk::client_source::detail {
double source_flight_now() {
    // GetTickCount64 advances by roughly 15.6 ms even at high frame rates.
    // Keep those coarse ticks for deadlines, never for integrating motion.
    static const double frequency = [] {
        LARGE_INTEGER value{};
        return QueryPerformanceFrequency(&value) && value.QuadPart > 0 ? static_cast<double>(value.QuadPart) : 0.0;
    }();
    LARGE_INTEGER counter{};
    source_require(frequency > 0 && QueryPerformanceCounter(&counter) && counter.QuadPart >= 0,
        "The flight timer is unavailable.");
    return static_cast<double>(counter.QuadPart) / frequency;
}
std::array<float, 3> debug_position(SourceReader& reader, std::uintptr_t address) {
    // The fourth SIMD lane is not a coordinate and can contain a native sentinel.
    std::array<float, 4> row{};
    source_require(reader.raw(address, row.data(), sizeof(row)), "Position is unavailable.");
    for (std::size_t i = 0; i < 3; ++i)
        source_require(std::isfinite(row[i]) && std::abs(row[i]) <= 1000000.0f, "Position exceeds diagnostic bounds.");
    return {row[0], row[1], row[2]};
}
std::array<float, 16> debug_skater(std::uintptr_t base, std::uintptr_t client, overlay::DebugModel& model) {
    SourceReader reader;
    source_require(reader.pointer(client) == base + addr::engine::client_vtable, "Client changed.");
    const auto context = reader.pointer(client, 8);
    const auto offset = reader.value<std::uint32_t>(base, addr::engine::context_player_manager_offset);
    source_require(offset <= 0x1000000, "Player manager offset changed.");
    const auto manager = reader.pointer(context, offset);
    source_require(reader.pointer(manager) == base + addr::engine::local_player_manager_vtable, "Player manager changed.");
    const auto begin = reader.pointer(manager, 0x4c8), end = reader.pointer(manager, 0x4d0);
    source_require(reader.count(begin, end, 8) == 1, "No unique local skater.");
    const auto player = reader.pointer(begin);
    source_require(reader.pointer(player) == base + addr::engine::local_player_vtable && reader.pointer(player, 0x78) == context &&
        reader.value<std::uint8_t>(player, 0x45) == 1 && !reader.value<std::uint8_t>(player, 0x44), "Local player changed.");
    const auto entity = reader.pointer(player, 0xb8), handle = reader.pointer(player, 0xb0);
    source_require(reader.pointer(entity) == base + addr::engine::skater_entity_vtable && reader.pointer(entity, 0x20) == context &&
        reader.pointer(entity, 0xf8) == player && reader.pointer(handle) == entity + 8,
        "No verified player-bound skater.");
    const auto collection = reader.pointer(entity, 0x70);
    source_require(reader.pointer(collection) == entity, "Skater transform owner changed.");
    const auto count = reader.value<std::uint8_t>(collection, 8);
    const auto first = reader.value<std::uint8_t>(collection, 9), extra = reader.value<std::uint8_t>(collection, 10);
    source_require(count <= 128 && first <= 128 && extra <= 32, "Skater transform layout changed.");
    const auto offset_matrix = std::uintptr_t{0x10} + (std::uintptr_t{first} + 2 * std::uintptr_t{extra}) * 0x20;
    const auto position = debug_position(reader, collection + offset_matrix + 0x30);
    std::array<float, 16> matrix{};
    source_require(reader.raw(collection + offset_matrix, matrix.data(), sizeof(matrix)) && valid_flight_transform(matrix),
        "Skater transform is invalid.");
    reader.verify();
    model.skater_position = position;
    model.skater_identity = entity;
    model.skater_position_valid = true;
    return matrix;
}
}
