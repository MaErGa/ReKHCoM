/* The one-file HD mod for the consoles' hosts (port/portable/hd_pack.c): <data folder>/ReKHCoM-HD.zip, made by the PC
 * launcher, is unpacked into <data folder>/hd before the game starts. Include once, in the host's single source.
 * In the tree it is found beside port/portable; the VM builds (Switch, Vita) copy hd_pack.c and miniz flat next to
 * this file. HD_PACK_HAVE_MINIZ: the host links miniz already (PS5). */
#if __has_include("hd_pack.c")
#include "hd_pack.c"
#ifndef HD_PACK_HAVE_MINIZ
#include "miniz.c"
#include "miniz_tdef.c"
#include "miniz_tinfl.c"
#include "miniz_zip.c"
#endif
#else
#include "../../portable/hd_pack.c"
#ifndef HD_PACK_HAVE_MINIZ
#include "../third_party/miniz/miniz.c"
#include "../third_party/miniz/miniz_tdef.c"
#include "../third_party/miniz/miniz_tinfl.c"
#include "../third_party/miniz/miniz_zip.c"
#endif
#endif
