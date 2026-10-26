"""Title screen-layer underlines must feed the same margin bands as BG0."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_ui_compositing import ROOT, function


class TitleMarginTests(unittest.TestCase):
    def test_screen_sprite_underline_reaches_the_margin(self):
        source = Path(os.environ.get('KHCOM_TEST_SOURCE', ROOT / 'port/src/port_ppu.c')).read_text(encoding='utf-8')
        unit = '''#include <stdint.h>
#include <string.h>
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define TRANSPARENT 65535
#define LAYER_OBJ 4
#define IO16(n) hw[(n)/2]
static uint16_t hw[64], PLTT16[512];
static uint32_t sColorLut[32768], sTitleBand[2][160];
static unsigned char sTitleBandObj[2][160];
static uint16_t sBgLine[4][320], sObjLine[320], sScrLine[320];
static uint16_t sTitleSampleLine[320];
static unsigned char sObjSemi[320], sScrSemi[320], sTitleSampleSemi[320];
static int sTitleBands=1, sEnvTitleBandsOff, sTitleBarHd=1, sOverlay=1;
static struct { int width,originX; } gPortScreen={320,40};
static uint16_t Blend(uint16_t a,uint16_t b,int c,int d) { return a; }
static uint16_t Brighten(uint16_t a,int b) { return a; }
static uint16_t Darken(uint16_t a,int b) { return a; }
'''
        unit += function(source, 'BuildTitleBands')
        unit += '''
int main(void) {
    int bg[4]={1,0,0,0};
    memset(sBgLine,255,sizeof(sBgLine));
    memset(sObjLine,255,sizeof(sObjLine));
    memset(sScrLine,255,sizeof(sScrLine));
    memset(sTitleSampleLine,255,sizeof(sTitleSampleLine));
    sColorLut[0]=0xFF000000u; sColorLut[1]=0xFF8C0831u;
    for(int x=272;x<=279;x++) sScrLine[x]=0;
    BuildTitleBands(154,bg);
    if(sTitleBand[1][154]!=0xFF000000u) return 1;
    for(int x=272;x<=279;x++) sScrLine[x]=TRANSPARENT;
    for(int x=272;x<=279;x++) sBgLine[0][x]=1;
    BuildTitleBands(150,bg);
    if(sTitleBand[1][150]!=0xFF8C0831u || sBgLine[0][319]!=1 || sTitleBandObj[1][150]) return 2;
    for(int x=272;x<=279;x++) sTitleSampleLine[x]=0;
    BuildTitleBands(154,bg);
    if(sTitleBand[1][154]!=0xFF000000u || !sTitleBandObj[1][154]) return 3;
    return 0;
}'''
        with tempfile.TemporaryDirectory() as folder:
            c = Path(folder) / 'bands.c'
            exe = Path(folder) / 'bands.exe'
            c.write_text(unit, encoding='utf-8')
            subprocess.run([str(ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'), str(c), '-o', str(exe)], check=True, capture_output=True)
            result = subprocess.run([str(exe)], capture_output=True)
            self.assertEqual(result.returncode, 0, 'screen-layer black underline was not included in title margins')


if __name__ == '__main__':
    unittest.main()
