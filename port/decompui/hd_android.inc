/* The HD mod on the Android launcher (LAUNCHER_ANDROID): the phone takes the hd folder the PC launcher builds, copied
 * by the activity through the system's folder picker (launcher_android.inc, ANDROID_PICK_HD); nothing is read from
 * the PC game or re-encoded here. The same names as hd_import.inc's, for the pages that show the mod. */
namespace hd {

struct Job {
    std::atomic<bool> running{ false };
    std::atomic<bool> cancel{ false };
    std::atomic<int> done{ 0 }, total{ 0 };
    std::atomic<int> partTotal[4] = {};
    std::string text; /* the activity's line (AndroidStatusNow, refreshed every frame on the main thread) */
    float Partial() { return 0.0f; }
    float PartProgress(int) { return 0.0f; }
    std::string Status() { return text; }
};
static Job sJob;

struct JournalInfo {
    bool found = false, movies = false, scenes = false, music = false, sprites = false;
    int quality = 1;
    fs::path source;
    std::vector<std::string> files;
};

static fs::path FindInstall() { return fs::path(); }
static bool IsInstall(const fs::path&) { return false; }
static JournalInfo ReadJournal(const fs::path&) { return JournalInfo(); }
static void DiscardImport(const fs::path&) {}
static void Start(const fs::path&, const fs::path&, bool, bool, bool, bool, int, bool = false) {}
static void Run(fs::path, fs::path, bool, bool, bool, bool, int, bool) {}
static void Compress(fs::path, int) {}

/* scenes kept by older versions in hd/movies, moved to hd/scenes (as hd_import.inc does) */
static void MoveScenes(const fs::path& hdDir) {
    std::error_code ec;
    fs::path from = hdDir / "movies", to = hdDir / "scenes";
    if (!fs::is_directory(from, ec))
        return;
    for (const auto& f : fs::directory_iterator(from, ec)) {
        std::string name = f.path().filename().string();
        if (f.path().extension() != ".mp4" || name.rfind("zz_event_", 0) != 0)
            continue;
        fs::create_directories(to, ec);
        if (!fs::exists(to / name, ec))
            fs::rename(f.path(), to / name, ec);
    }
}

static int Count(const fs::path& folder, const char* ext) {
    std::error_code ec;
    int n = 0;
    for (const auto& f : fs::directory_iterator(folder, ec))
        n += f.path().extension() == ext;
    return n;
}

}  // namespace hd
