bool camera_range(std::uintptr_t address, std::size_t size) {
    return address >= 0x10000 && size && size <= memory::highest_user_address &&
        address <= memory::highest_user_address - size;
}
bool camera_object(std::uintptr_t address) { return camera_range(address, 8) && !(address & 7); }
struct CameraIdentity {
    bool available{}, tls_available{}, helper_available{}, entry_backlink{}, helper_backlink{};
    bool helper_type_matches{}, evaluator_available{}, evaluator_context_matches{}, camera_backlink{}, selected_entry{};
    std::uintptr_t callback{}, entry{}, helper{}, graph{}, entry_context{}, tls_context{};
    std::uintptr_t evaluator{}, evaluator_context{}, camera{};
    std::array<bool, 3> helper_present{};
};
std::uintptr_t camera_entry_identity(std::uintptr_t base, std::uintptr_t callback) noexcept {
    std::uintptr_t vtable{}, entry{}, entry_vtable{};
    if (!camera_object(callback) || !camera_range(callback, 0x28) || !memory::peek(callback, vtable) ||
        vtable != base + camera_addresses::callback_vtable || !memory::peek(callback + 0x10, entry) || !camera_object(entry) ||
        !memory::peek(entry, entry_vtable)) return 0;
    // Two native constructors create entry variants through the same base
    // constructor (+60 callback, +90 client context). Gameplay switches between
    // them; accepting only the first loses tracking when the alternate becomes
    // selected after a camera/editor transition.
    if (entry_vtable != base + camera_addresses::entry_vtable &&
        entry_vtable != base + camera_addresses::alternate_entry_vtable) return 0;
    return entry;
}
CameraIdentity camera_identity(std::uintptr_t base, std::uintptr_t callback, std::uintptr_t evaluator = 0) noexcept {
    CameraIdentity result; result.callback = callback;
    result.entry = camera_entry_identity(base, callback);
    if (!result.entry || !camera_range(result.entry, 0x98) ||
        !memory::peek(callback + 0x18, result.helper) || !memory::peek(callback + 0x20, result.graph) ||
        !memory::peek(result.entry + 0x90, result.entry_context)) return result;
    std::uintptr_t backlink{};
    result.entry_backlink = memory::peek(result.entry + 0x60, backlink) && backlink == callback;
    if (camera_object(result.helper) && camera_range(result.helper, 0x70)) {
        std::uintptr_t vtable{};
        result.helper_type_matches = memory::peek(result.helper, vtable) && vtable == base + camera_addresses::helper_vtable;
        result.helper_backlink = memory::peek(result.helper + 0x50, backlink) && backlink == callback;
        std::array<std::uintptr_t, 3> values{};
        result.helper_available = memory::peek(result.helper + 0x58, values);
        if (result.helper_available) for (std::size_t i = 0; i < values.size(); ++i) result.helper_present[i] = values[i] != 0;
    }
    // The evaluator comes from input+8 only for the verified native caller.
    // Save numeric identities, never the caller's input/output scratch pointers.
    if (camera_object(evaluator) && camera_range(evaluator, 0x140)) {
        result.evaluator = evaluator;
        std::uintptr_t collection{}, begin{}, end{}, selected{}, vtable{};
        if (memory::peek(evaluator, result.evaluator_context) && camera_object(result.evaluator_context) &&
            memory::peek(evaluator + 0x138, result.camera) && camera_object(result.camera) &&
            camera_range(result.camera, 0x188) && memory::peek(result.camera, vtable) &&
            vtable == base + addr::engine::camera_vtable) {
            result.evaluator_available = true;
            result.evaluator_context_matches = result.evaluator_context == result.entry_context;
            result.camera_backlink = memory::peek(result.camera + 0x180, backlink) && backlink == evaluator;
        }
        if (memory::peek(evaluator + 8, collection) && camera_object(collection) && camera_range(collection, 0x18) &&
            memory::peek(collection, backlink) && backlink == evaluator &&
            memory::peek(collection + 8, begin) && memory::peek(collection + 0x10, end) &&
            camera_object(begin) && end > begin && (end - begin) % 0x10 == 0 && (end - begin) / 0x10 <= 32 &&
            camera_range(begin, end - begin) && memory::peek(end - 0x10, selected) && selected == result.entry) {
            // Reject a header or final selection that changed during this copy.
            std::uintptr_t again{};
            result.selected_entry = memory::peek(evaluator + 8, again) && again == collection &&
                memory::peek(collection + 8, again) && again == begin &&
                memory::peek(collection + 0x10, again) && again == end &&
                memory::peek(end - 0x10, again) && again == selected;
        }
    }
    std::uint32_t index{};
    std::uintptr_t block{};
    unsigned char initialized{};
    const auto array = static_cast<std::uintptr_t>(__readgsqword(0x58));
    if (memory::peek(base + addr::engine::tls_index, index) && index <= 4095 && camera_object(array) &&
        camera_range(array, std::size_t{index} * 8 + 8) && memory::peek(array + std::uintptr_t{index} * 8, block) &&
        camera_object(block) && camera_range(block, 0xb1a) && memory::peek(block + 0xb19, initialized) && initialized &&
        memory::peek(block + 0x550, result.tls_context) && camera_object(result.tls_context)) result.tls_available = true;
    result.available = true;
    return result;
}
bool camera_same_identity(const CameraIdentity& before, const CameraIdentity& after) noexcept {
    return before.available && after.available && before.callback == after.callback &&
        before.entry == after.entry && before.helper == after.helper && before.graph == after.graph &&
        before.entry_context == after.entry_context && before.evaluator == after.evaluator &&
        before.camera == after.camera && before.evaluator_context == after.evaluator_context;
}
bool camera_idle_identity(const CameraIdentity& value, std::uintptr_t context) noexcept {
    return value.available && value.tls_available && value.tls_context == context && value.entry_context == context &&
        value.entry_backlink && value.helper_backlink && value.helper_type_matches && value.evaluator_available &&
        value.evaluator_context_matches && value.camera_backlink && value.selected_entry && value.helper_available &&
        value.helper_present == std::array<bool, 3>{false, false, false};
}
