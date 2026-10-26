"""The HD indicators share Press Start's existing light and a continuous pulse."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[2]


class MenuLightsTests(unittest.TestCase):
    def test_existing_cursor_and_press_start_use_the_same_orb(self):
        folder = ROOT / 'port/assets/hd/title/words'
        for frame in range(5):
            cursor = Image.open(folder / f'gTitleMenuCursorFrame{frame}.png').convert('RGBA').crop((24, 0, 112, 88))
            for lang in ('', 'French', 'German', 'Italian', 'Spanish'):
                image = Image.open(folder / f'gTitlePressStart{lang}Frame{frame}.png').convert('RGBA')
                top = image.height - 128  # German's frame includes an extra eight native rows above the orb.
                start = image.crop((24, top, 112, top + 88))
                self.assertIsNone(ImageChops.difference(cursor, start).getbbox(), (frame, lang))

    def test_pulse_is_periodic_and_does_not_jump_at_frame_boundaries(self):
        header = ROOT / 'port/include/port/port_menu_light.h'
        self.assertTrue(header.exists(), 'the title and Options must use one shared pulse')
        unit = r'''#include <assert.h>
#include <math.h>
#include "port/port_menu_light.h"
int main(void) {
    const float value[5]={255,246,222,164,115};
    float previous=255;
    for(unsigned tick=0;tick<512;tick++) {
        int a,b,c,d; float mix,again;
        PortMenuLightFrames(tick,&a,&b,&mix);
        PortMenuLightFrames(tick+64,&c,&d,&again);
        assert(a>=0 && a<5 && b>=0 && b<5 && mix>=0 && mix<=1);
        assert(a==c && b==d && fabsf(mix-again)<0.0001f);
        float colour=value[a]*(1-mix)+value[b]*mix;
        assert(fabsf(colour-previous)<20);
        previous=colour;
    }
    return 0;
}'''
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'pulse.c'
            exe = Path(directory) / 'pulse.exe'
            source.write_text(unit)
            subprocess.run([str(ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'),
                            '-I' + str(ROOT / 'port/include'), str(source), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    unittest.main()
