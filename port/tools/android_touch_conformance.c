/* ROM-free touch state / UI routing regression on the desktop SDL runtime. */
#include <assert.h>
#include <stdio.h>
#include "../android/native/touch_android.c"
PortConfig gPortConfig;
/* the look's cartridge parts and font (touch_android.c draws Sora and the letters from them): none here */
void* gSor1ll67Frames[8];
const uint8_t gSor1ll67Tiles[32];
const uint16_t gSoraPalette[16];
int PortUiRenderTextLine(uint32_t* pixels,int width,int height,const char* text,uint32_t color) {
    (void)text;(void)color;if(pixels) SDL_memset(pixels,0,(size_t)width*height*4);return 0;
}
static int ui,opened,count;
static SDL_Keycode received[32];
int PortUiActive(void) { return ui; }
static int editHold,saved;
void PortUiTouchEdit(int on) { editHold=on; }
void PortConfigSave(void) { saved++; }
void PortUiOpenOptions(void) { ui=1;opened++; }
int PortUiHandleEvent(const void* p) {
    const SDL_Event* e=p;assert(e->type==SDL_EVENT_KEY_DOWN);
    assert(count<32);received[count++]=e->key.key;return 1;
}
static void finger(SDL_EventType type,int id,float x,float y) {
    SDL_Event e={0};e.type=type;e.tfinger.fingerID=id;e.tfinger.x=x;e.tfinger.y=y;
    PortAndroidTouchEvent(&e);
}
/* places from the layout's homes, so the test follows them */
#define DX kGroupHome[G_DPAD][0]
#define DY kGroupHome[G_DPAD][1]
#define AX (kGroupHome[G_FACE][0]+.135f/aspect)
#define AY kGroupHome[G_FACE][1]
#define MX kGroupHome[G_MENU][0]
#define MY kGroupHome[G_MENU][1]
int main(void) {
    finger(SDL_EVENT_FINGER_DOWN,10,DX-.06f,DY);
    finger(SDL_EVENT_FINGER_DOWN,11,AX,AY);
    assert(PortAndroidTouchKeys()==(0x20|1));
    finger(SDL_EVENT_FINGER_MOTION,10,DX-.06f,DY-.10f);
    assert(PortAndroidTouchKeys()==(0x20|0x40|1));
    finger(SDL_EVENT_FINGER_CANCELED,10,0,0);assert(PortAndroidTouchKeys()==1);
    finger(SDL_EVENT_FINGER_UP,999,0,0);assert(PortAndroidTouchKeys()==1);
    finger(SDL_EVENT_FINGER_UP,11,0,0);assert(!PortAndroidTouchKeys());
    /* The dedicated settings button must open the real overlay route. */
    finger(SDL_EVENT_FINGER_DOWN,12,MX,MY);
    assert(opened==1 && ui && !PortAndroidTouchKeys());
    finger(SDL_EVENT_FINGER_UP,12,0,0);
    finger(SDL_EVENT_FINGER_DOWN,13,DX,DY+.10f);assert(count==1 && received[0]==SDLK_DOWN);
    finger(SDL_EVENT_FINGER_MOTION,13,DX,DY+.11f);assert(count==1);
    finger(SDL_EVENT_FINGER_UP,13,0,0);
    finger(SDL_EVENT_FINGER_DOWN,13,DX,DY+.10f);assert(count==2 && received[1]==SDLK_DOWN);
    finger(SDL_EVENT_FINGER_UP,13,0,0);
    /* a diagonal in a menu: only its main direction (down here), never down and right together */
    {int before=count;finger(SDL_EVENT_FINGER_DOWN,15,DX+.05f/aspect,DY+.09f);
     assert(count==before+1 && received[count-1]==SDLK_DOWN);finger(SDL_EVENT_FINGER_UP,15,0,0);}
    finger(SDL_EVENT_FINGER_DOWN,14,AX,AY);assert(received[count-1]==SDLK_RETURN);
    finger(SDL_EVENT_FINGER_UP,14,0,0);
    finger(SDL_EVENT_FINGER_DOWN,14,kGroupHome[G_FACE][0],AY+.135f);assert(received[count-1]==SDLK_ESCAPE);
    SDL_Event e={0};e.type=SDL_EVENT_WINDOW_FOCUS_LOST;PortAndroidTouchEvent(&e);
    assert(!PortAndroidTouchKeys());
    ui=0;
    finger(SDL_EVENT_FINGER_DOWN,20,kGroupHome[G_SELECT][0],kGroupHome[G_SELECT][1]);assert(PortAndroidTouchKeys()==4);
    finger(SDL_EVENT_FINGER_DOWN,21,kGroupHome[G_START][0],kGroupHome[G_START][1]);
    assert(opened==2 && !PortAndroidTouchKeys());
    /* Disable held inputs, ignore hidden gameplay buttons, retain settings. */
    ui=0;PortAndroidTouchClear();
    finger(SDL_EVENT_FINGER_DOWN,30,AX,AY);assert(PortAndroidTouchKeys()==1);
    gPortConfig.hideTouchControls=1;PortAndroidTouchSettingsChanged();
    assert(!PortAndroidTouchKeys());
    finger(SDL_EVENT_FINGER_DOWN,31,AX,AY);assert(!PortAndroidTouchKeys());
    finger(SDL_EVENT_FINGER_DOWN,32,DX-.06f,DY);assert(!PortAndroidTouchKeys());
    finger(SDL_EVENT_FINGER_DOWN,33,MX,MY);assert(ui && opened==3);
    finger(SDL_EVENT_FINGER_UP,33,0,0);
    finger(SDL_EVENT_FINGER_DOWN,34,DX,DY+.10f);assert(received[count-1]==SDLK_DOWN);
    gPortConfig.hideTouchControls=0;PortAndroidTouchSettingsChanged();ui=0;
    finger(SDL_EVENT_FINGER_DOWN,35,AX,AY);assert(PortAndroidTouchKeys()==1);
    /* The layout editor: the d-pad and the face buttons dragged elsewhere, saved with the tick; the old places are
       empty, the new ones answer; the round arrow puts them back. */
    finger(SDL_EVENT_FINGER_UP,35,0,0);ui=0;PortAndroidTouchClear();
    editHold=1;PortAndroidTouchEditStart();
    finger(SDL_EVENT_FINGER_DOWN,40,DX+.01f,DY-.04f);assert(!PortAndroidTouchKeys()); /* grabs the d-pad, presses nothing */
    finger(SDL_EVENT_FINGER_MOTION,40,DX+.18f,DY-.24f);finger(SDL_EVENT_FINGER_UP,40,0,0);
    assert(gPortConfig.touchPos[G_DPAD][0]==170 && gPortConfig.touchPos[G_DPAD][1]==-200);
    finger(SDL_EVENT_FINGER_DOWN,41,kGroupHome[G_FACE][0],kGroupHome[G_FACE][1]);finger(SDL_EVENT_FINGER_MOTION,41,kGroupHome[G_FACE][0]-.12f,kGroupHome[G_FACE][1]+.06f);finger(SDL_EVENT_FINGER_UP,41,0,0);
    assert(gPortConfig.touchPos[G_FACE][0]==-120 && gPortConfig.touchPos[G_FACE][1]==60);
    finger(SDL_EVENT_FINGER_DOWN,42,kTool[0][0],kTool[0][1]);finger(SDL_EVENT_FINGER_UP,42,0,0); /* the tick */
    assert(saved==1 && !editHold && !editing);
    finger(SDL_EVENT_FINGER_DOWN,43,AX,AY);assert(!PortAndroidTouchKeys()); /* A's old place */
    finger(SDL_EVENT_FINGER_UP,43,0,0);
    finger(SDL_EVENT_FINGER_DOWN,44,AX-.12f,AY+.06f);assert(PortAndroidTouchKeys()==1); /* A's new place */
    finger(SDL_EVENT_FINGER_UP,44,0,0);
    finger(SDL_EVENT_FINGER_DOWN,45,DX+.17f-.06f/aspect,DY-.20f);assert(PortAndroidTouchKeys()==0x20); /* the d-pad's left */
    finger(SDL_EVENT_FINGER_UP,45,0,0);
    PortAndroidTouchEditStart();
    finger(SDL_EVENT_FINGER_DOWN,46,kTool[1][0],kTool[1][1]);finger(SDL_EVENT_FINGER_UP,46,0,0); /* the round arrow */
    assert(!gPortConfig.touchPos[G_DPAD][0] && !gPortConfig.touchPos[G_FACE][1] && editing);
    finger(SDL_EVENT_FINGER_DOWN,47,kTool[0][0],kTool[0][1]);finger(SDL_EVENT_FINGER_UP,47,0,0);
    finger(SDL_EVENT_FINGER_DOWN,48,AX,AY);assert(PortAndroidTouchKeys()==1);
    puts("Touch: multi-finger, UI, hide/restore, held-key clearing, settings recovery and the layout editor passed");
    return 0;
}
