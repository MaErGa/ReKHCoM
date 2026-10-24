/* miniz (MIT, port/ps5/third_party/miniz) for the launcher: the HD importer writes the HD sprites as PNG, and the
   roms folder scan reads zips (a dump or the HD mod's folders). */
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "../ps5/third_party/miniz/miniz.c"
#include "../ps5/third_party/miniz/miniz_tdef.c"
#include "../ps5/third_party/miniz/miniz_tinfl.c"
#include "../ps5/third_party/miniz/miniz_zip.c"
