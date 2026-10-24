/* The account's unlocks (public ids only) for the game's Options menu,
   written when the launcher has read them. Credentials never go here. */
static void RememberAchievementProgress() {
    static uint64_t previous;
    std::vector<uint32_t> ids;
    uint32_t known = KhRaAccountProgressAvailable(sRaUi) ? 1u : 0u;
    uint64_t hash = 1469598103934665603ull ^ known;
    for (int i = 0; known && i < KhOfficialCount(); ++i) {
        uint32_t id = KhOfficialAt(i)->id;
        if (KhRaAccountUnlocked(sRaUi, id)) { ids.push_back(id); hash ^= id; hash *= 1099511628211ull; }
    }
    if (hash == previous) return;
    std::error_code error; fs::create_directories(sRoot / "cache", error);
    fs::path path = sRoot / "cache" / "achievement-unlocks.dat", temp = path; temp += ".tmp";
    std::ofstream out(temp, std::ios::binary);
    uint32_t count = (uint32_t)ids.size();
    out.write("KHRAPRG1", 8); out.write((const char*)&known, 4); out.write((const char*)&count, 4);
    if (count) out.write((const char*)ids.data(), count * 4);
    out.close();
    if (out && ReplaceWith(temp, path)) previous = hash;
}
