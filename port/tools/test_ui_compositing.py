"""Compile the actual UI pixel compositor and exercise transparent HD canvas fades."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    start = source.index('static ', source.rfind('\n', 0, source.index(name + '(')))
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


class UiCompositingTests(unittest.TestCase):
    def test_translucent_veil_stays_translucent_through_fade(self):
        ui = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8')
        cc = Path(os.environ.get('CLANG', ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'))
        c = '''#include <stdint.h>
#include <stdio.h>
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_min(a,b) ((a)<(b)?(a):(b))
static uint32_t pixels[4], *sFb=pixels, *sHdUiFrame;
static int sFbW=2,sFbH=2,sHdUiCount;
static struct {int x,y; uint32_t face,shadow;} sHdUiGlyphs[1];
'''
        c += function(ui, 'Mix') + '\n' + function(ui, 'Shade')
        c += '''
int main(void) {
    pixels[0]=0xE6FFFFFFu; pixels[1]=0x00FFFFFFu;
    pixels[2]=0xFFFFFFFFu; pixels[3]=0xE6FFFFFFu;
    Shade(0,0,2,2,0xFF000000u,128);
    if ((pixels[0]>>24)>=255 || (pixels[0]>>24)<=230) return 1;
    if ((pixels[1]>>24)!=128) return 2;
    if (pixels[2]!=0xFF7F7F7Fu) return 3;
    uint32_t before=pixels[3];
    Shade(1,1,1,1,0xFFFFFFFFu,0);
    if (pixels[3]!=before) return 4;
    Shade(0,0,2,2,0xFF000000u,256);
    for(int i=0;i<4;i++) if(pixels[i]!=0xFF000000u) return 5;
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / 'compositor.c'
            exe = Path(folder) / 'compositor.exe'
            source.write_text(c, encoding='utf-8')
            subprocess.run([str(cc), str(source), '-o', str(exe)], check=True, capture_output=True)
            result = subprocess.run([str(exe)], capture_output=True)
            self.assertEqual(result.returncode, 0, 'HD art veil became opaque or fade changed opaque/zero/full-alpha behavior')


if __name__ == '__main__':
    unittest.main()
