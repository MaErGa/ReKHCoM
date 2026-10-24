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
void PortUiOpenOptions(void) { ui=1;opened++; }
int PortUiHandleEvent(const void* p) {
    const SDL_Event* e=p;assert(e->type==SDL_EVENT_KEY_DOWN);
    assert(count<32);received[count++]=e->key.key;return 1;
}
static void finger(SDL_EventType type,int id,float x,float y) {
    SDL_Event e={0};e.type=type;e.tfinger.fingerID=id;e.tfinger.x=x;e.tfinger.y=y;
    PortAndroidTouchEvent(&e);
}
int main(void) {
    finger(SDL_EVENT_FINGER_DOWN,10,.07f,.73f);
    finger(SDL_EVENT_FINGER_DOWN,11,.91f,.62f);
    assert(PortAndroidTouchKeys()==(0x20|1));
    finger(SDL_EVENT_FINGER_MOTION,10,.07f,.63f);
    assert(PortAndroidTouchKeys()==(0x20|0x40|1));
    finger(SDL_EVENT_FINGER_CANCELED,10,0,0);assert(PortAndroidTouchKeys()==1);
    finger(SDL_EVENT_FINGER_UP,999,0,0);assert(PortAndroidTouchKeys()==1);
    finger(SDL_EVENT_FINGER_UP,11,0,0);assert(!PortAndroidTouchKeys());
    /* The dedicated settings button must open the real overlay route. */
    finger(SDL_EVENT_FINGER_DOWN,12,.95f,.17f);
    assert(opened==1 && ui && !PortAndroidTouchKeys());
    finger(SDL_EVENT_FINGER_UP,12,0,0);
    finger(SDL_EVENT_FINGER_DOWN,13,.13f,.83f);assert(count==1 && received[0]==SDLK_DOWN);
    finger(SDL_EVENT_FINGER_MOTION,13,.13f,.84f);assert(count==1);
    finger(SDL_EVENT_FINGER_UP,13,0,0);
    finger(SDL_EVENT_FINGER_DOWN,13,.13f,.83f);assert(count==2 && received[1]==SDLK_DOWN);
    finger(SDL_EVENT_FINGER_UP,13,0,0);
    /* a diagonal in a menu: only its main direction (down here), never down and right together */
    {int before=count;finger(SDL_EVENT_FINGER_DOWN,15,.13f+.05f/aspect,.73f+.09f);
     assert(count==before+1 && received[count-1]==SDLK_DOWN);finger(SDL_EVENT_FINGER_UP,15,0,0);}
    finger(SDL_EVENT_FINGER_DOWN,14,.91f,.62f);assert(received[count-1]==SDLK_RETURN);
    finger(SDL_EVENT_FINGER_UP,14,0,0);
    finger(SDL_EVENT_FINGER_DOWN,14,.83f,.79f);assert(received[count-1]==SDLK_ESCAPE);
    SDL_Event e={0};e.type=SDL_EVENT_WINDOW_FOCUS_LOST;PortAndroidTouchEvent(&e);
    assert(!PortAndroidTouchKeys());
    ui=0;
    finger(SDL_EVENT_FINGER_DOWN,20,.45f,.93f);assert(PortAndroidTouchKeys()==4);
    finger(SDL_EVENT_FINGER_DOWN,21,.55f,.93f);
    assert(opened==2 && !PortAndroidTouchKeys());
    /* Disable held inputs, ignore hidden gameplay buttons, retain settings. */
    ui=0;PortAndroidTouchClear();
    finger(SDL_EVENT_FINGER_DOWN,30,.91f,.62f);assert(PortAndroidTouchKeys()==1);
    gPortConfig.hideTouchControls=1;PortAndroidTouchSettingsChanged();
    assert(!PortAndroidTouchKeys());
    finger(SDL_EVENT_FINGER_DOWN,31,.91f,.62f);assert(!PortAndroidTouchKeys());
    finger(SDL_EVENT_FINGER_DOWN,32,.07f,.73f);assert(!PortAndroidTouchKeys());
    finger(SDL_EVENT_FINGER_DOWN,33,.95f,.17f);assert(ui && opened==3);
    finger(SDL_EVENT_FINGER_UP,33,0,0);
    finger(SDL_EVENT_FINGER_DOWN,34,.13f,.83f);assert(received[count-1]==SDLK_DOWN);
    gPortConfig.hideTouchControls=0;PortAndroidTouchSettingsChanged();ui=0;
    finger(SDL_EVENT_FINGER_DOWN,35,.91f,.62f);assert(PortAndroidTouchKeys()==1);
    puts("Touch: multi-finger, UI, hide/restore, held-key clearing and settings recovery passed");
    return 0;
}
