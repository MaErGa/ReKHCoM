/* The HD mod as one file: <data folder>/ReKHCoM-HD.zip (made by the PC launcher, Mod HD > "Prepare for another
 * platform") is unpacked into <data folder>/hd on start, so a console or phone needs one file copied to one place.
 * An hd folder left there by hand is used as always. Shared by the desktop game and the console/Android hosts
 * (port/progress/HD_MOD.md): plain stdio and miniz, no SDL.
 *
 * Only hd/<music|sprites|subs|movies|scenes>/... entries are taken (a bare music/... is read as hd/music/...);
 * anything else, or a path with "..", is skipped. hd/.pack remembers the zip installed (its size), so the next start
 * does nothing until another zip is copied over it. The zip itself is left where it was (the player may delete it). */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define HdMkdir(p) _mkdir(p)
#else
#define HdMkdir(p) mkdir(p, 0777)
#endif
#if __has_include("miniz.h") /* a flat copy (the consoles' VM builds) */
#include "miniz.h"
#else
#include "../ps5/third_party/miniz/miniz.h"
#endif

typedef void (*HdPackLog)(const char* fmt, ...);

static void MakeParents(char* path) {
    char* p;
    for (p = path + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            HdMkdir(path);
            *p = '/';
        }
}

/* the entry's path under hd/, or NULL when it is not the HD mod's */
static const char* HdRelative(const char* name) {
    static const char* const kinds[] = { "music/", "sprites/", "subs/", "movies/", "scenes/" };
    size_t i;
    if (strncmp(name, "hd/", 3) == 0)
        name += 3;
    if (strstr(name, "..") || name[0] == '/' || name[0] == '\\')
        return NULL;
    for (i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++)
        if (strncmp(name, kinds[i], strlen(kinds[i])) == 0 && name[strlen(kinds[i])])
            return name;
    return NULL;
}

/* 1: a zip was installed now, 0: nothing to do, -1: the zip could not be read (the hd folder is left as it was) */
int HdPackInstall(const char* root, HdPackLog log) {
    char zipPath[1024], markPath[1024], out[1100], mark[64] = "", want[64];
    struct stat st;
    mz_zip_archive zip;
    FILE* f;
    mz_uint i, n, done = 0;

    snprintf(zipPath, sizeof(zipPath), "%s/ReKHCoM-HD.zip", root);
    if (stat(zipPath, &st) != 0)
        return 0;
    snprintf(want, sizeof(want), "%lld", (long long)st.st_size);
    snprintf(markPath, sizeof(markPath), "%s/hd/.pack", root);
    if ((f = fopen(markPath, "r")) != NULL) {
        if (!fgets(mark, sizeof(mark), f))
            mark[0] = 0;
        fclose(f);
        mark[strcspn(mark, "\r\n")] = 0;
        if (strcmp(mark, want) == 0)
            return 0; /* this zip is in already */
    }
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zipPath, 0)) {
        if (log) log("hd pack: %s is not a zip that can be read", zipPath);
        return -1;
    }
    n = mz_zip_reader_get_num_files(&zip);
    for (i = 0; i < n; i++) {
        mz_zip_archive_file_stat fs;
        const char* rel;
        if (!mz_zip_reader_file_stat(&zip, i, &fs) || fs.m_is_directory || !(rel = HdRelative(fs.m_filename)))
            continue;
        snprintf(out, sizeof(out), "%s/hd/%s", root, rel);
        MakeParents(out);
        /* write beside, then rename: a cut power leaves no half track under its real name */
        {
            char tmp[1110];
            snprintf(tmp, sizeof(tmp), "%s.part", out);
            if (!mz_zip_reader_extract_to_file(&zip, i, tmp, 0)) {
                remove(tmp);
                if (log) log("hd pack: %s did not unpack", fs.m_filename);
                continue;
            }
            remove(out);
            if (rename(tmp, out) != 0) {
                remove(tmp);
                continue;
            }
        }
        done++;
    }
    mz_zip_reader_end(&zip);
    if (done) {
        snprintf(out, sizeof(out), "%s/hd/.pack", root);
        MakeParents(out);
        if ((f = fopen(out, "w")) != NULL) {
            fprintf(f, "%s\n", want);
            fclose(f);
        }
    }
    if (log) log("hd pack: %u files from %s into %s/hd (the zip can be deleted now)", (unsigned)done, zipPath, root);
    return done ? 1 : 0;
}
