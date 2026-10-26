"""Render the production HD lamp for a full cycle, including fades and cleanup."""
from pathlib import Path
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'port/src/port_hdsprites.c').read_text(encoding='utf-8')
out = ROOT / 'build/hd-menu-light-test'
out.mkdir(parents=True, exist_ok=True)


def body(signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


unit = r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "port/port_menu_light.h"
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"
static const char* assetRoot;
static struct {int hdTitle;} gPortConfig={1};
static int PortSmoothScaleMode(void) {return SDL_SCALEMODE_LINEAR;}
static void* TitleWordAsset(const char* name,size_t* size) {
    char path[2048];snprintf(path,sizeof(path),"%s/%s.png",assetRoot,name);
    FILE* f=fopen(path,"rb");if(!f)return NULL;
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
    void* p=SDL_malloc(n);assert(p);assert(fread(p,1,n,f)==(size_t)n);fclose(f);*size=n;return p;
}
'''
start = source.index('enum { MENU_LIGHT_SIZE')
unit += source[start:source.index('static void MenuLightRelease', start)]
for signature in ('static void MenuLightRelease(', 'static int MenuLightReady(',
                  'static int MenuLightPaint(', 'int PortHdMenuLightDraw('):
    unit += body(signature) + '\n'
unit += r'''
static unsigned colour(SDL_Renderer* renderer) {
    SDL_Surface* shot=SDL_RenderReadPixels(renderer,NULL);assert(shot);
    SDL_Surface* rgba=SDL_ConvertSurface(shot,SDL_PIXELFORMAT_RGBA32);assert(rgba);
    unsigned char* p=(unsigned char*)rgba->pixels+44*rgba->pitch+44*4;
    unsigned value=p[0]<<16|p[1]<<8|p[2];
    SDL_DestroySurface(rgba);SDL_DestroySurface(shot);return value;
}
static unsigned draw(SDL_Renderer* renderer,unsigned tick,float shade,float white) {
    SDL_SetRenderDrawColor(renderer,255,255,255,255);SDL_RenderClear(renderer);
    assert(PortHdMenuLightDraw(renderer,0,0,88,tick,shade,white));return colour(renderer);
}
int main(int argc,char**argv) {
    assetRoot=argv[1];assert(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window* window=SDL_CreateWindow("HD lamp test",88,88,SDL_WINDOW_HIDDEN);assert(window);
    SDL_Renderer* renderer=SDL_CreateRenderer(window,"software");assert(renderer);
    unsigned initial=draw(renderer,0,1,0),last=initial;
    assert(((initial>>16)&255)>((initial>>8)&255) && ((initial>>8)&255)>(initial&255)); /* warm source, not swapped blue */
    for(unsigned tick=1;tick<=64;tick++) {
        unsigned current=draw(renderer,tick,1,0);
        for(int shift=0;shift<=16;shift+=8)assert(abs((int)((current>>shift)&255)-(int)((last>>shift)&255))<24);
        last=current;
    }
    assert(last==initial && initial!=0 && initial!=0xFFFFFF);
    assert(draw(renderer,0,0,0)==0);
    assert(draw(renderer,0,1,1)==0xFFFFFF);
    gPortConfig.hdTitle=0;assert(!PortHdMenuLightDraw(renderer,0,0,88,0,1,0));
    MenuLightRelease();assert(!sMenuLightSamples&&!sMenuLightPixels&&!sMenuLightImage&&!sMenuLightWhite);
    SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
    puts("PASS shared HD light: 64 smooth frames, loop seam, black/white fades, off guard and freed resources");
}
'''
(out / 'light.c').write_text(unit, encoding='utf-8')
compiler = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang.exe'
sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/i686-w64-mingw32'
exe = out / 'light.exe'
subprocess.run([str(compiler), '-std=gnu11', '-O1', '-I' + str(sdl / 'include'), '-I' + str(ROOT / 'port/include'),
                '-I' + str(ROOT / 'port/decompui'), str(out / 'light.c'), '-L' + str(sdl / 'lib'), '-lSDL3', '-o', str(exe)], check=True)
shutil.copy2(sdl / 'bin/SDL3.dll', out / 'SDL3.dll')
subprocess.run([str(exe), str(ROOT / 'port/assets/hd/title/words')], cwd=out,
                env=dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy'), check=True)
