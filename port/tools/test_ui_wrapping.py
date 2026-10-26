"""Exercise the production wrapper with narrow boxes, long words and UTF-8."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_ui_compositing import ROOT, function


class WrappingTests(unittest.TestCase):
    def test_long_words_cannot_escape_the_box(self):
        ui = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8')
        c = '''#include <stdint.h>
#include <string.h>
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_memcpy memcpy
#define SDL_strlen strlen
#define SDL_strncmp strncmp
static int bad, drawn;
static int TextWidth(const char* s) { int n=0; for(;*s;s++) if((*s & 0xC0)!=0x80) n++; return n; }
static int DrawText(const char* s,int x,int y,uint32_t a,uint32_t b) { drawn++; if(TextWidth(s)>8) bad++; return 0; }
'''
        for name in ('WideChar', 'ClosingMark', 'WrapUnit', 'DrawWrapped'):
            c += function(ui, name) + '\n'
        c += '''int main(void) {
    const char* samples[]={"RestaurarValoresPredeterminados", "Hello supercalifragilistic next", "aaaaaaaaaaaaaaaaaaaaééééé", "日本語のとても長い文章です。"};
    for(int i=0;i<4;i++) {
        drawn=0;
        int count=DrawWrapped(samples[i],0,0,8,0,1);
        if(count!=drawn || !count) return 2;
    }
    return bad ? 1 : 0;
}'''
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / 'wrap.c'
            exe = Path(folder) / 'wrap.exe'
            source.write_text(c, encoding='utf-8')
            subprocess.run([str(ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'), str(source), '-o', str(exe)], check=True, capture_output=True)
            result = subprocess.run([str(exe)], capture_output=True)
            self.assertEqual(result.returncode, 0, 'a long word escaped the dialog text box')


if __name__ == '__main__':
    unittest.main()
