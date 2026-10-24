#include <SDL3/SDL.h>
#include <math.h>
#include "port/port.h"
#include "port/port_android.h"
#include "port/port_ui.h"
#include "touch_letters.inc"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
static int touchEnabled;
EMSCRIPTEN_KEEPALIVE void PortWebTouchEnable(int enabled) {
    touchEnabled=enabled;
    gPortConfig.hideTouchControls=!enabled;
    PortAndroidTouchClear();
}
EMSCRIPTEN_KEEPALIVE void PortWebTouchPersist(void) { PortConfigSave(); }
#endif

typedef struct Finger { SDL_FingerID id; float x,y; int active; } Finger;
typedef struct Button { float x,y; uint16_t bits; const char* text; int icon; float ox,oy; } Button;
static Finger fingers[10];
static float aspect=16.0f/9.0f;
#if defined(__EMSCRIPTEN__) || defined(PORT_TOUCH_CENTERED_LAYOUT)
static float layoutX,layoutY,layoutW=1,layoutH=1,outputW=1,outputH=1;
static void CenterLayout(int w,int h) {
    float margin=SDL_max(8.0f,SDL_min(w,h)*.025f);
    outputW=w;outputH=h;
    layoutW=SDL_min(w-2*margin,(h-2*margin)*16.0f/9.0f);
    layoutW=SDL_max(1.0f,layoutW);layoutH=layoutW*9.0f/16.0f;
    layoutX=(w-layoutW)*.5f;layoutY=(h-layoutH)*.5f;
    aspect=16.0f/9.0f;
}
#endif
static const Button buttons[]={
    {.12f,.42f,0x200,"L",0},{.88f,.42f,0x100,"R",0},
    /* the face buttons in a diamond like a pad's: A right, B below, roll left, stock above (user 24/10); ox/oy in
       screen heights from the diamond's centre, so the spacing holds on any screen shape */
    {.86f,.72f,1,"A",0,.135f,0},{.86f,.72f,2,"B",0,0,.135f},
    {.86f,.72f,PORT_KEY_DODGE,"",1,-.135f,0},{.86f,.72f,PORT_KEY_STOCK,"",2,0,-.135f},
    {.44f,.94f,4,"SELECT",0},{.56f,.94f,8,"START",0},
    {.95f,.08f,12,"...",0}
};
static uint16_t uiHeld;
/* ---- where the controls are: each group at its place, moved by the player (gPortConfig.touchPos, the layout editor
   below), in fractions of the 16:9 layout. Home is a current mobile game's layout (user 24/10): each thumb's cluster
   low in its corner, the d-pad and the face diamond mirrored, L and R just above them where the thumb reaches, the
   menu in the top corner ---- */
enum { G_DPAD, G_L, G_R, G_FACE, G_SELECT, G_START, G_MENU, G_COUNT };
static const float kGroupHome[G_COUNT][2]={{.13f,.74f},{.12f,.42f},{.88f,.42f},{.86f,.72f},{.44f,.94f},{.56f,.94f},{.95f,.08f}};
static const uint8_t kButtonGroup[]={G_L,G_R,G_FACE,G_FACE,G_FACE,G_FACE,G_SELECT,G_START,G_MENU};
static void GroupAt(int g,float* x,float* y) {
    *x=kGroupHome[g][0]+gPortConfig.touchPos[g][0]*.001f;
    *y=kGroupHome[g][1]+gPortConfig.touchPos[g][1]*.001f;
}
/* the layout editor (Options, "Move touch buttons"; port_ui.c holds the game while it is open): a finger drags the
   group it lands on; at the top a tick saves and closes, a round arrow puts every group back at its place */
static int editing, dragGroup=-1;
static SDL_FingerID dragFinger;
static float grabX,grabY;
static const float kTool[2][2]={{.44f,.09f},{.56f,.09f}}; /* tick, reset */
/* Android: with the touch controls hidden and a gamepad connected, nothing is drawn, not even the options button or
   the pad in the menus (issue #38); with no gamepad the options button stays, the only way into the menu. */
static int AllHidden(void) {
#if defined(PORT_SWITCH)
    /* Switch: the Joy-Cons are always there and Minus + Plus held opens the options (issue #36) */
    return gPortConfig.hideTouchControls;
#elif defined(__EMSCRIPTEN__)
    return 0;
#else
    int count=0;
    SDL_JoystickID* ids;
    if(!gPortConfig.hideTouchControls) return 0;
    ids=SDL_GetGamepads(&count);
    SDL_free(ids);
    return count>0;
#endif
}
void PortAndroidTouchClear(void) { SDL_memset(fingers,0,sizeof(fingers)); uiHeld=0; dragGroup=-1; }
void PortAndroidTouchSettingsChanged(void) {
    PortAndroidTouchClear();
#ifdef __EMSCRIPTEN__
    touchEnabled=1;
    EM_ASM({ if (Module.onTouchPanelsChanged) Module.onTouchPanelsChanged(!!$0); },
           !gPortConfig.hideTouchControls);
#endif
}
static uint16_t TouchKeys(int menu);
static void FreeSkins(void);
static void TouchUi(void) {
    /* the menus take the d-pad's main direction only: a diagonal touch sent down and right together, so going down in
       Options changed the resolution or opened Achievements by itself (user's report, 24/10) */
    uint16_t keys, pressed;
    static const struct { uint16_t bit; SDL_Keycode key; } bindings[]={
        {0x40,SDLK_UP},{0x80,SDLK_DOWN},{0x20,SDLK_LEFT},{0x10,SDLK_RIGHT},
        {1,SDLK_RETURN},{2,SDLK_ESCAPE},{4,SDLK_ESCAPE},{8,SDLK_ESCAPE},
        {0x200,SDLK_Q},{0x100,SDLK_E} /* L and R: the Achievements page's filter */
    };
    int i;
    if(editing)return;
    keys=TouchKeys(1);pressed=keys & ~uiHeld;
    uiHeld=keys;
    if(!PortUiActive()) {
        if((keys & 12)==12 && (pressed & 12)) {
            PortAndroidTouchClear();
            PortUiOpenOptions();
        }
        return;
    }
    /* The settings overlay consumes SDL keyboard/gamepad events, whereas
       gameplay reads GBA bits. Forward touch presses to that same UI route. */
    for(i=0;i<(int)SDL_arraysize(bindings);i++) if(pressed & bindings[i].bit) {
        SDL_Event event={0};event.type=SDL_EVENT_KEY_DOWN;
        event.key.key=bindings[i].key;
        PortUiHandleEvent(&event);
    }
}
/* a finger's place in the layout's fractions */
static void ToLayout(float* fx,float* fy) {
#if defined(__EMSCRIPTEN__) || defined(PORT_TOUCH_CENTERED_LAYOUT)
    *fx=(*fx*outputW-layoutX)/layoutW;*fy=(*fy*outputH-layoutY)/layoutH;
#else
    (void)fx;(void)fy;
#endif
}
void PortAndroidTouchEditStart(void) {
    PortAndroidTouchClear();
    editing=1;
}
static void EditTouch(const SDL_Event* e) {
    float fx=e->tfinger.x,fy=e->tfinger.y,gx,gy;
    int g,t;
    ToLayout(&fx,&fy);
    if(e->type==SDL_EVENT_FINGER_DOWN) {
        float best=1;
        if(dragGroup>=0)return; /* one group at a time */
        for(t=0;t<2;t++) {
            float dx=(fx-kTool[t][0])*aspect,dy=fy-kTool[t][1];
            if(dx*dx+dy*dy>.065f*.065f)continue;
            if(t==0) { /* saved, and back to the game */
                editing=0;
                PortConfigSave();
                PortUiTouchEdit(0);
                PortAndroidTouchClear();
            } else
                SDL_memset(gPortConfig.touchPos,0,sizeof(gPortConfig.touchPos));
            return;
        }
        for(g=0;g<G_COUNT;g++) { /* the nearest group the finger is on, against the group's reach */
            static const float reach[G_COUNT]={.19f,.13f,.13f,.22f,.10f,.10f,.07f};
            float dx,dy,d;
            GroupAt(g,&gx,&gy);
            dx=(fx-gx)*aspect;dy=fy-gy;
            d=(dx*dx+dy*dy)/(reach[g]*reach[g]);
            if(d<best) { best=d; dragGroup=g; grabX=gx-fx; grabY=gy-fy; }
        }
        if(dragGroup>=0)dragFinger=e->tfinger.fingerID;
        return;
    }
    if(dragGroup<0 || e->tfinger.fingerID!=dragFinger)return;
    if(e->type==SDL_EVENT_FINGER_MOTION) {
        float x=SDL_clamp(fx+grabX,.03f,.97f),y=SDL_clamp(fy+grabY,.05f,.97f);
        gPortConfig.touchPos[dragGroup][0]=SDL_clamp((int)SDL_roundf((x-kGroupHome[dragGroup][0])*1000),-900,900);
        gPortConfig.touchPos[dragGroup][1]=SDL_clamp((int)SDL_roundf((y-kGroupHome[dragGroup][1])*1000),-900,900);
    } else
        dragGroup=-1;
}
void PortAndroidTouchEvent(const void* event) {
#ifdef __EMSCRIPTEN__
    if(!touchEnabled)return;
#endif
    const SDL_Event* e=event;
    int i, freeSlot=-1;
    if(e->type==SDL_EVENT_RENDER_DEVICE_RESET) {
        FreeSkins();
        return;
    }
    if(e->type==SDL_EVENT_WINDOW_FOCUS_LOST || e->type==SDL_EVENT_WILL_ENTER_BACKGROUND) {
        PortAndroidTouchClear(); return;
    }
    if(e->type!=SDL_EVENT_FINGER_DOWN && e->type!=SDL_EVENT_FINGER_MOTION &&
       e->type!=SDL_EVENT_FINGER_UP && e->type!=SDL_EVENT_FINGER_CANCELED) return;
    if(editing) { EditTouch(e); return; }
    for(i=0;i<10;i++) {
        if(!fingers[i].active) { if(freeSlot<0) freeSlot=i; continue; }
        if(fingers[i].id!=e->tfinger.fingerID) continue;
        fingers[i].x=e->tfinger.x; fingers[i].y=e->tfinger.y;
        if(e->type==SDL_EVENT_FINGER_UP || e->type==SDL_EVENT_FINGER_CANCELED) fingers[i].active=0;
        TouchUi();
        return;
    }
    if(e->type==SDL_EVENT_FINGER_DOWN && freeSlot>=0) {
        fingers[freeSlot]=(Finger){e->tfinger.fingerID,e->tfinger.x,e->tfinger.y,1};
        TouchUi();
    }
}
uint16_t PortAndroidTouchKeys(void) { return TouchKeys(0); }
static uint16_t TouchKeys(int menu) {
#ifdef __EMSCRIPTEN__
    if(!touchEnabled)return 0;
#endif
    uint16_t keys=0;
    int i,b;
    float px,py;
    if(AllHidden() || editing) return 0;
    int panels = !gPortConfig.hideTouchControls || PortUiActive();
    for(i=0;i<10;i++) if(fingers[i].active) {
        float fx=fingers[i].x,fy=fingers[i].y,dx,dy;
        ToLayout(&fx,&fy);
        GroupAt(G_DPAD,&px,&py);
        dx=(fx-px)*aspect;dy=fy-py;
        if(panels && SDL_fabsf(dx)<.17f && SDL_fabsf(dy)<.17f) {
            if(menu) { if(SDL_fabsf(dx)>SDL_fabsf(dy)) dy=0; else dx=0; }
            if(dx<-.035f) keys|=0x20;
            if(dx>.035f) keys|=0x10;
            if(dy<-.035f) keys|=0x40;
            if(dy>.035f) keys|=0x80;
            continue;
        }
        for(b=0;b<(int)SDL_arraysize(buttons);b++) {
            if(!panels && b!=(int)SDL_arraysize(buttons)-1) continue;
            float x,y;
            GroupAt(kButtonGroup[b],&px,&py);
            x=(fx-px)*aspect-buttons[b].ox;y=fy-py-buttons[b].oy;
            float radius=b==6 || b==7 ? .055f : .075f;
            if(x*x+y*y<=radius*radius) { keys|=buttons[b].bits; break; }
            /* L/R and SELECT/START are pills now: their whole width counts */
            if((b<=1 || b==6 || b==7) && SDL_fabsf(x)<=(b<=1?.12f:.10f) && SDL_fabsf(y)<=(b<=1?.065f:.05f)) { keys|=buttons[b].bits; break; }
        }
    }
    return keys;
}
/* ---- the look (touch_theme 0 Clear, 1 Dark, 2 Color; approved by the user 24/10 from build/touchmock/themes.py,
   whose numbers these are) -----------------------------------------------------------------------------------------
   Each control is drawn once, by software, into a texture the size it takes on screen (sizes in the mock's pixels at
   720 rows, scaled by k): a navy glass plate, a silver bevelled ring, KH red while pressed; the letters in the game's
   font; the roll is Sora's own roll from the cartridge (three of its frames, the earlier ones fading as a trail); the
   stock is the original design (port/android/assets/stock.svg). Color tints each plate with its button's colour. */
extern void* gSor1ll67Frames[];
extern const uint8_t gSor1ll67Tiles[];
extern const uint16_t gSoraPalette[];

typedef struct Rgb { float r,g,b; } Rgb;
typedef struct Theme {
    Rgb fill; float alpha[2]; Rgb ring; float ringA, innerA, sheen; Rgb ink, rim;
    float soraLo, soraHi; /* Sora's grey ramp; soraHi 0: his own colours */
    float iconAlpha, tintA;
    int accentPress, cards; /* Color: pressed in the button's colour, the stock cards in the game's colours */
} Theme;
#define C3(r,g,b) {(r)/255.0f,(g)/255.0f,(b)/255.0f}
static const Theme kThemes[3]={
    {C3(18,32,72),{86,165},C3(226,232,244),160,45,24,C3(255,255,255),C3(12,16,30),150,255,1.0f,0,0,0},
    {C3(6,8,16),{150,205},C3(150,156,172),170,22,12,C3(232,234,240),C3(0,0,0),120,235,.95f,0,0,0},
    {C3(18,32,72),{80,160},C3(226,232,244),170,40,22,C3(255,255,255),C3(12,16,30),0,0,.92f,34,1,1},
};
static const Rgb kRed=C3(214,36,40), kBlue=C3(44,112,222), kGreen=C3(52,168,84), kGold=C3(226,176,52),
                 kSilver=C3(226,232,244), kWhite=C3(255,255,255);
enum { CTRL_DPAD=9, CTRL_COUNT=10 };
/* Color's accent per control: L, R, A, B, roll, stock, SELECT, START, options, d-pad */
static const Rgb* const kAccent[CTRL_COUNT]={&kGold,&kGold,&kRed,&kBlue,&kGold,&kGreen,&kSilver,&kSilver,&kSilver,&kBlue};

typedef struct Skin { SDL_Texture* tex[2]; int w,h; } Skin;
static Skin skins[CTRL_COUNT], arrows[4], tools[2];
static int skinUnit=-1, skinTheme=-1;

typedef struct Canvas { int w,h; float* px; } Canvas; /* premultiplied RGBA */
static int NewCanvas(Canvas* c,int w,int h) {
    c->w=w;c->h=h;c->px=SDL_calloc((size_t)w*h,4*sizeof(float));
    return c->px!=NULL;
}
static void Over(float* d,Rgb c,float a) {
    if(a<=0)return;
    if(a>1)a=1;
    d[0]=c.r*a+d[0]*(1-a);d[1]=c.g*a+d[1]*(1-a);d[2]=c.b*a+d[2]*(1-a);d[3]=a+d[3]*(1-a);
}
static float Clamp01(float v) { return v<0?0:v>1?1:v; }
/* how far (x,y) is inside a rounded rectangle of half sizes hx,hy and corner r (negative: outside) */
static float RoundRect(float x,float y,float hx,float hy,float r) {
    float qx=SDL_fabsf(x)-(hx-r),qy=SDL_fabsf(y)-(hy-r),ox=SDL_max(qx,0.0f),oy=SDL_max(qy,0.0f);
    return -(SDL_sqrtf(ox*ox+oy*oy)+SDL_min(SDL_max(qx,qy),0.0f)-r);
}
typedef struct Shape { int cross; float hx,hy,r,arm; } Shape;
static float Inside(const Shape* s,float x,float y) {
    if(s->cross) return SDL_max(RoundRect(x,y,s->hx,s->arm,s->r),RoundRect(x,y,s->arm,s->hx,s->r));
    return RoundRect(x,y,s->hx,s->hy,s->r);
}
/* the plate: glow when pressed, glass, Color's tint, sheen on the upper half, ring, the inner bevel line */
static void Glass(Canvas* c,const Shape* s,const Theme* t,const Rgb* accent,int pressed,float k) {
    Rgb press=t->accentPress && accent?*accent:kRed, ring=accent?*accent:t->ring;
    int x,y;
    if(!t->tintA) ring=t->ring;
    for(y=0;y<c->h;y++) for(x=0;x<c->w;x++) {
        float lx=x+.5f-c->w*.5f,ly=y+.5f-c->h*.5f,d=Inside(s,lx,ly),cov=Clamp01(d+.5f),*p=c->px+((size_t)y*c->w+x)*4;
        if(pressed) { float o=SDL_max(0.0f,-d)/(6*k); Over(p,press,.43f*SDL_expf(-.5f*o*o)); }
        if(cov<=0)continue;
        Over(p,t->fill,cov*t->alpha[pressed]/255);
        if(t->tintA && accent) Over(p,*accent,cov*t->tintA*(pressed?2:1)/255);
        if(ly<0) Over(p,kWhite,Clamp01(d-k+.5f)*t->sheen/255);
        Over(p,pressed?press:ring,cov*Clamp01(2*k-d+.5f)*(pressed?240:t->ringA)/255);
        Over(p,kWhite,Clamp01(d-5*k+.5f)*Clamp01(6*k-d+.5f)*t->innerA/255);
    }
}
/* src drawn into dst centred on (cx,cy) at scale (src pixels to dst pixels), 3x3 samples a pixel */
static void Stamp(Canvas* dst,const Canvas* src,float cx,float cy,float scale,float alpha) {
    float w=src->w*scale,h=src->h*scale;
    int x0=(int)SDL_floorf(cx-w*.5f),y0=(int)SDL_floorf(cy-h*.5f),x1=(int)SDL_ceilf(cx+w*.5f),y1=(int)SDL_ceilf(cy+h*.5f),x,y,i,j;
    int n=SDL_clamp((int)SDL_ceilf(3/scale),3,8); /* samples a side: enough to cover every source pixel shrunk */
    for(y=SDL_max(y0,0);y<SDL_min(y1,dst->h);y++) for(x=SDL_max(x0,0);x<SDL_min(x1,dst->w);x++) {
        float acc[4]={0,0,0,0},*d=dst->px+((size_t)y*dst->w+x)*4;
        for(j=0;j<n;j++) for(i=0;i<n;i++) {
            int sx=(int)SDL_floorf((x+(i+.5f)/n-(cx-w*.5f))/scale),sy=(int)SDL_floorf((y+(j+.5f)/n-(cy-h*.5f))/scale);
            if(sx<0||sy<0||sx>=src->w||sy>=src->h)continue;
            const float* s=src->px+((size_t)sy*src->w+sx)*4;
            acc[0]+=s[0];acc[1]+=s[1];acc[2]+=s[2];acc[3]+=s[3];
        }
        for(i=0;i<4;i++) acc[i]*=alpha/(n*n);
        for(i=0;i<3;i++) d[i]=acc[i]+d[i]*(1-acc[3]);
        d[3]=acc[3]+d[3]*(1-acc[3]);
    }
}
/* text in the HD text's font (KHMenu.otf, port/tools/gen_touch_letters.py), centred, its capitals cap pixels tall,
   with the mock's dark shadow; scaled down smoothly from the big letters */
static void Text(Canvas* c,const char* text,float cx,float cy,float cap,const Theme* t,float k) {
    const TouchLetters* l=NULL;
    Canvas shadow,ink;
    size_t i,n;
    for(i=0;i<SDL_arraysize(kTouchLetters);i++) if(!SDL_strcmp(kTouchLetters[i].text,text)) l=&kTouchLetters[i];
    if(!l || !NewCanvas(&shadow,l->w,l->h))return;
    if(!NewCanvas(&ink,l->w,l->h)) { SDL_free(shadow.px); return; }
    n=(size_t)l->w*l->h;
    for(i=0;i<n;i++) {
        float a=l->a[i]/255.0f;
        shadow.px[i*4]=t->rim.r*a;shadow.px[i*4+1]=t->rim.g*a;shadow.px[i*4+2]=t->rim.b*a;shadow.px[i*4+3]=a;
        ink.px[i*4]=t->ink.r*a;ink.px[i*4+1]=t->ink.g*a;ink.px[i*4+2]=t->ink.b*a;ink.px[i*4+3]=a;
    }
    Stamp(c,&shadow,cx+k,cy+2*k,cap/l->h,170/255.0f);
    Stamp(c,&ink,cx,cy,cap/l->h,235/255.0f);
    SDL_free(shadow.px);SDL_free(ink.px);
}
/* one frame of Sora's roll (gSor1ll67, anim 53 of the battle) from the cartridge, cropped, in RGBA */
static int SoraFrame(int index,Canvas* out) {
    static const uint8_t sizes[3][4][2]={{{8,8},{16,16},{32,32},{64,64}},{{16,8},{32,8},{32,16},{64,32}},{{8,16},{8,32},{16,32},{32,64}}};
    const uint16_t* rec=(const uint16_t*)gSor1ll67Frames[index];
    Canvas full;
    int p,minX=96,maxX=-1,minY=96,maxY=-1,y;
    if(!rec || !rec[0] || !NewCanvas(&full,96,96))return 0;
    for(p=rec[0]-1;p>=0;p--) {
        uint16_t a0=rec[1+p*3],a1=rec[2+p*3],a2=rec[3+p*3];
        int shape=a0>>14,size=a1>>14,w,h,ox=a1&0x1FF,oy=a0&0xFF,tile=a2&0x3FF,hf=a1>>12&1,vf=a1>>13&1,px,py;
        if(shape>2)continue;
        w=sizes[shape][size][0];h=sizes[shape][size][1];
        if(ox>=256)ox-=512;
        if(oy>=128)oy-=256;
        for(py=0;py<h;py++) for(px=0;px<w;px++) {
            uint8_t b=gSor1ll67Tiles[(tile+(py/8)*(w/8)+px/8)*32+(py%8)*4+(px%8)/2];
            int v=px&1?b>>4:b&15,dx=48+ox+(hf?w-1-px:px),dy=48+oy+(vf?h-1-py:py);
            float* d;
            if(!v || dx<0 || dy<0 || dx>=96 || dy>=96)continue;
            d=full.px+((size_t)dy*96+dx)*4;
            d[0]=(gSoraPalette[v]&31)/31.0f;d[1]=(gSoraPalette[v]>>5&31)/31.0f;d[2]=(gSoraPalette[v]>>10&31)/31.0f;d[3]=1;
            minX=SDL_min(minX,dx);maxX=SDL_max(maxX,dx);minY=SDL_min(minY,dy);maxY=SDL_max(maxY,dy);
        }
    }
    if(maxX<0 || !NewCanvas(out,maxX-minX+1,maxY-minY+1)) { SDL_free(full.px); return 0; }
    for(y=0;y<out->h;y++) SDL_memcpy(out->px+(size_t)y*out->w*4,full.px+((size_t)(y+minY)*96+minX)*4,(size_t)out->w*4*sizeof(float));
    SDL_free(full.px);
    return 1;
}
/* the roll: frames 2, 3 and 4 one after another, the earlier ones fading, with speed lines (themes.py roll_icon) */
static void RollIcon(Canvas* c,float cx,float cy,float r,const Theme* t) {
    Canvas f[3],icon;
    static const float fade[3]={.28f,.55f,1};
    int i,x,y,maxH=0,scale,step,W,H,ok=1;
    for(i=0;i<3;i++) ok&=SoraFrame(i+1,&f[i]);
    if(!ok) { for(i=0;i<3;i++) if(f[i].px) SDL_free(f[i].px); return; }
    for(i=0;i<3;i++) maxH=SDL_max(maxH,f[i].h);
    scale=SDL_max(1,(int)SDL_roundf(r/maxH));
    step=13*scale;W=step*2+f[2].w*scale+6;H=maxH*scale+6;
    if(NewCanvas(&icon,W,H)) {
        for(i=0;i<3;i++) {
            int ox=i*step+2,oy=H-f[i].h*scale-3,pass;
            for(pass=0;pass<2;pass++) /* the dark rim (grown by a pixel, a row lower), then Sora */
            for(y=0;y<f[i].h*scale;y++) for(x=0;x<f[i].w*scale;x++) {
                const float* s=f[i].px+((size_t)(y/scale)*f[i].w+x/scale)*4;
                float* d;
                if(pass==0) {
                    int on=0,dx,dy;
                    for(dy=-1;dy<=1 && !on;dy++) for(dx=-1;dx<=1 && !on;dx++) {
                        int sx=x/scale+dx,sy=y/scale+dy;
                        on=sx>=0 && sy>=0 && sx<f[i].w && sy<f[i].h && f[i].px[((size_t)sy*f[i].w+sx)*4+3]>0;
                    }
                    if(!on || oy+y+1>=H)continue;
                    Over(icon.px+((size_t)(oy+y+1)*W+ox+x)*4,t->rim,150/255.0f*fade[i]);
                } else if(s[3]>0) {
                    Rgb col={s[0],s[1],s[2]};
                    if(t->soraHi>0) {
                        float lum=(.299f*s[0]+.587f*s[1]+.114f*s[2])*255,v=(t->soraLo+lum*(t->soraHi-t->soraLo)/255)/255;
                        col.r=col.g=col.b=v;
                    }
                    d=icon.px+((size_t)(oy+y)*W+ox+x)*4;
                    Over(d,col,fade[i]*t->iconAlpha);
                }
            }
        }
        {
            static const float lines[3][2]={{.38f,.30f},{.55f,.42f},{.72f,.26f}};
            for(i=0;i<3;i++) for(y=(int)(H*lines[i][0]);y<(int)(H*lines[i][0])+scale;y++) for(x=1;x<=(int)(W*lines[i][1]);x++)
                Over(icon.px+((size_t)y*W+x)*4,t->ink,(70+i*10)/255.0f);
        }
        Stamp(c,&icon,cx,cy,W>r*1.82f?r*1.82f/W:1,1);
        SDL_free(icon.px);
    }
    for(i=0;i<3;i++) SDL_free(f[i].px);
}
/* distance from (x,y) to the segment a-b */
static float SegDist(float x,float y,float ax,float ay,float bx,float by) {
    float vx=bx-ax,vy=by-ay,t=((x-ax)*vx+(y-ay)*vy)/(vx*vx+vy*vy),dx,dy;
    t=Clamp01(t);dx=x-(ax+vx*t);dy=y-(ay+vy*t);
    return SDL_sqrtf(dx*dx+dy*dy);
}
static int InPoly(float x,float y,const float (*p)[2],int n) {
    int i,j,in=0;
    for(i=0,j=n-1;i<n;j=i++)
        if((p[i][1]>y)!=(p[j][1]>y) && x<(p[j][0]-p[i][0])*(y-p[i][1])/(p[j][1]-p[i][1])+p[i][0]) in=!in;
    return in;
}
static int OnLine(float x,float y,const float (*p)[2],int n) {
    int i;
    for(i=0;i+1<n;i++) if(SegDist(x,y,p[i][0],p[i][1],p[i+1][0],p[i+1][1])<=1.5f) return 1;
    return 0;
}
/* the stock: two cards behind, the crowned card in front, plus signs (stock.svg, 64 units) */
static void StockIcon(Canvas* c,float cx,float cy,float size,const Theme* t) {
    static const float card0[5][2]={{12,20},{8,20},{8,52},{32,52},{32,48}},card1[5][2]={{20,13},{16,13},{16,44},{40,44},{40,40}};
    static const float crown[8][2]={{24,10},{29,15},{36,7},{43,15},{48,10},{48,37},{24,37},{24,10}};
    static const float plus0[12][2]={{34,20},{38,20},{38,25},{43,25},{43,29},{38,29},{38,34},{34,34},{34,29},{29,29},{29,25},{34,25}};
    static const float plus1[12][2]={{44,44},{50,44},{50,39},{54,39},{54,44},{60,44},{60,48},{54,48},{54,54},{50,54},{50,48},{44,48}};
    Rgb cols[4];
    float u=size/64,x0=cx-size*.5f,y0=cy-size*.5f;
    int x,y,i,j,l;
    if(t->cards) { cols[0]=kBlue;cols[1]=kGreen;cols[2]=kRed;cols[3]=kGold; }
    else cols[0]=cols[1]=cols[2]=cols[3]=t->ink;
    for(y=SDL_max(0,(int)y0);y<SDL_min(c->h,(int)(y0+size+2*u+1));y++) for(x=SDL_max(0,(int)x0);x<SDL_min(c->w,(int)(x0+size+1));x++) {
        float cover[7]={0},*d=c->px+((size_t)y*c->w+x)*4;
        for(j=0;j<3;j++) for(i=0;i<3;i++) {
            float sx=(x+(i+.5f)/3-x0)/u,sy=(y+(j+.5f)/3-y0)/u,hy=sy-2; /* hy: the shadow, 2 units lower */
            int any=OnLine(sx,hy,card0,5)||OnLine(sx,hy,card1,5)||OnLine(sx,hy,crown,8)||InPoly(sx,hy,plus0,12)||InPoly(sx,hy,plus1,12)||
                    (t->cards && InPoly(sx,hy,crown,7));
            cover[0]+=any;
            cover[1]+=OnLine(sx,sy,card0,5);
            cover[2]+=OnLine(sx,sy,card1,5);
            cover[3]+=t->cards && InPoly(sx,sy,crown,7);
            cover[4]+=OnLine(sx,sy,crown,8);
            cover[5]+=InPoly(sx,sy,plus0,12)||InPoly(sx,sy,plus1,12);
        }
        for(l=0;l<6;l++) cover[l]/=9;
        Over(d,t->rim,cover[0]*140/255*t->iconAlpha);
        Over(d,cols[0],cover[1]*t->iconAlpha);
        Over(d,cols[1],cover[2]*t->iconAlpha);
        Over(d,cols[2],cover[3]*70/255*t->iconAlpha);
        Over(d,cols[2],cover[4]*t->iconAlpha);
        Over(d,cols[3],cover[5]*t->iconAlpha);
    }
}
static void Dot(Canvas* c,float cx,float cy,float r,Rgb col,float a) {
    int x,y;
    for(y=SDL_max(0,(int)(cy-r-1));y<SDL_min(c->h,(int)(cy+r+2));y++) for(x=SDL_max(0,(int)(cx-r-1));x<SDL_min(c->w,(int)(cx+r+2));x++) {
        float dx=x+.5f-cx,dy=y+.5f-cy;
        Over(c->px+((size_t)y*c->w+x)*4,col,a*Clamp01(r-SDL_sqrtf(dx*dx+dy*dy)+.5f));
    }
}
static SDL_Texture* Upload(SDL_Renderer* r,const Canvas* c) {
    uint8_t* bytes=SDL_malloc((size_t)c->w*c->h*4);
    SDL_Texture* t=NULL;
    size_t i,n=(size_t)c->w*c->h;
    if(!bytes)return NULL;
    for(i=0;i<n;i++) { /* straight alpha for SDL's blend */
        float a=c->px[i*4+3];
        bytes[i*4]=(uint8_t)(a>0?Clamp01(c->px[i*4]/a)*255+.5f:0);
        bytes[i*4+1]=(uint8_t)(a>0?Clamp01(c->px[i*4+1]/a)*255+.5f:0);
        bytes[i*4+2]=(uint8_t)(a>0?Clamp01(c->px[i*4+2]/a)*255+.5f:0);
        bytes[i*4+3]=(uint8_t)(Clamp01(a)*255+.5f);
    }
    t=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,c->w,c->h);
    if(t) {
        SDL_UpdateTexture(t,NULL,bytes,c->w*4);
        SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);
    }
    SDL_free(bytes);
    return t;
}
static void FreeSkins(void) {
    int i,j;
    for(i=0;i<CTRL_COUNT;i++) for(j=0;j<2;j++) { SDL_DestroyTexture(skins[i].tex[j]); skins[i].tex[j]=NULL; }
    for(i=0;i<4;i++) for(j=0;j<2;j++) { SDL_DestroyTexture(arrows[i].tex[j]); arrows[i].tex[j]=NULL; }
    for(i=0;i<2;i++) for(j=0;j<2;j++) { SDL_DestroyTexture(tools[i].tex[j]); tools[i].tex[j]=NULL; }
    skinUnit=-1;
}
/* the controls' shapes in screen rows (U: the 16:9 picture's height in pixels) */
static Shape ControlShape(int b,float U) {
    Shape s={0};
    if(b==CTRL_DPAD) { s.cross=1;s.hx=.062f*U;s.arm=.175f*U;s.r=.0194f*U; }
    else if(b==0 || b==1) { s.hx=.10f*U;s.hy=.0425f*U;s.r=s.hy; }
    else if(b==6 || b==7) { s.hx=.085f*U;s.hy=.031f*U;s.r=s.hy; }
    else { s.hx=s.hy=s.r=(b==8?.05f:.068f)*U; }
    return s;
}
static void BuildSkins(SDL_Renderer* r,float U) {
    const Theme* t=&kThemes[SDL_clamp(gPortConfig.touchTheme,0,2)];
    float k=U/720;
    int b,p,m=(int)SDL_ceilf(24*k);
    FreeSkins();
    for(b=0;b<CTRL_COUNT;b++) {
        Shape s=ControlShape(b,U);
        float hx=s.cross?s.arm:s.hx,hy=s.cross?s.arm:s.hy;
        for(p=0;p<2;p++) {
            Canvas c;
            if(!NewCanvas(&c,2*((int)SDL_ceilf(hx)+m),2*((int)SDL_ceilf(hy)+m)))continue;
            Glass(&c,&s,t,t->tintA?kAccent[b]:NULL,p,k);
            if(b<CTRL_DPAD && buttons[b].icon==1) RollIcon(&c,c.w*.5f,c.h*.5f,s.hx,t);
            else if(b<CTRL_DPAD && buttons[b].icon==2) StockIcon(&c,c.w*.5f,c.h*.5f,s.hx*1.3f,t);
            else if(b==8) { int i; for(i=-1;i<=1;i++) { Dot(&c,c.w*.5f+i*9*k,c.h*.5f+2*k,3*k,t->rim,120/255.0f); Dot(&c,c.w*.5f+i*9*k,c.h*.5f,3*k,t->ink,225/255.0f); } }
            else if(b==CTRL_DPAD) { /* the centre ring */
                int x,y;
                for(y=0;y<c.h;y++) for(x=0;x<c.w;x++) {
                    float dx=x+.5f-c.w*.5f,dy=y+.5f-c.h*.5f,d=SDL_fabsf(SDL_sqrtf(dx*dx+dy*dy)-11*k);
                    Over(c.px+((size_t)y*c.w+x)*4,kWhite,Clamp01(.5f*k+.5f-d)*55/255.0f);
                }
            } else Text(&c,buttons[b].text,c.w*.5f,c.h*.5f,(b<=1?.055f:b<=3?.06f:.034f)*U*.70f,t,k);
            skins[b].tex[p]=Upload(r,&c);skins[b].w=c.w;skins[b].h=c.h;
            SDL_free(c.px);
        }
    }
    /* the d-pad's arrows, one texture a direction (white; red while held), drawn apart so each lights alone */
    for(b=0;b<4;b++) {
        static const float angle[4]={-1.5707963f,1.5707963f,3.1415927f,0}; /* up, down, left, right */
        float a=angle[b],size=12*k;
        Canvas c;
        int x,y;
        if(!NewCanvas(&c,(int)SDL_ceilf(size*2+6*k)+2,(int)SDL_ceilf(size*2+6*k)+2))continue;
        for(y=0;y<c.h;y++) for(x=0;x<c.w;x++) {
            float pts[3][2],cover[2]={0,0};
            int i,j,l;
            for(l=0;l<3;l++) { float aa=a+(l==0?0:l==1?2.4f:-2.4f); pts[l][0]=c.w*.5f+SDL_cosf(aa)*size; pts[l][1]=c.h*.5f+SDL_sinf(aa)*size; }
            for(j=0;j<3;j++) for(i=0;i<3;i++) {
                float sx=x+(i+.5f)/3,sy=y+(j+.5f)/3;
                cover[0]+=InPoly(sx,sy-2*k,(const float(*)[2])pts,3);
                cover[1]+=InPoly(sx,sy,(const float(*)[2])pts,3);
            }
            Over(c.px+((size_t)y*c.w+x)*4,t->rim,cover[0]/9*120/255);
            Over(c.px+((size_t)y*c.w+x)*4,kWhite,cover[1]/9*225/255);
        }
        arrows[b].tex[0]=Upload(r,&c);arrows[b].w=c.w;arrows[b].h=c.h;
        SDL_free(c.px);
    }
    /* the layout editor's tick and round arrow: the options button's plate, the sign stroked in the theme's ink */
    for(b=0;b<2;b++) {
        Shape s={0};
        float line[14][2],rad;
        int n=0,i,x,y;
        Canvas c;
        s.hx=s.hy=s.r=.05f*U;
        rad=s.hx*.42f;
        if(!NewCanvas(&c,2*((int)SDL_ceilf(s.hx)+m),2*((int)SDL_ceilf(s.hy)+m)))continue;
        Glass(&c,&s,t,t->tintA?&kGreen:NULL,0,k);
        if(b==0) {
            static const float tick[3][2]={{-.8f,.05f},{-.25f,.6f},{.8f,-.5f}};
            for(i=0;i<3;i++) { line[n][0]=c.w*.5f+tick[i][0]*rad; line[n++][1]=c.h*.5f+tick[i][1]*rad; }
        } else for(i=0;i<=11;i++) { /* most of a circle, open at the top right, with its arrowhead */
            float a=-.9f+i*(5.0f/11);
            line[n][0]=c.w*.5f+SDL_cosf(a)*rad; line[n++][1]=c.h*.5f+SDL_sinf(a)*rad;
        }
        for(y=0;y<c.h;y++) for(x=0;x<c.w;x++) {
            float d=1e9f,px=x+.5f,py=y+.5f;
            for(i=0;i+1<n;i++) d=SDL_min(d,SegDist(px,py,line[i][0],line[i][1],line[i+1][0],line[i+1][1]));
            if(b==1) { /* the arrowhead at the circle's open end */
                float hx=line[0][0],hy=line[0][1];
                d=SDL_min(d,SegDist(px,py,hx,hy,hx+.55f*rad,hy+.05f*rad));
                d=SDL_min(d,SegDist(px,py,hx,hy,hx+.05f*rad,hy-.55f*rad));
            }
            Over(c.px+((size_t)y*c.w+x)*4,t->ink,Clamp01(2.6f*k+.5f-d)*235/255.0f);
        }
        tools[b].tex[0]=Upload(r,&c);tools[b].w=c.w;tools[b].h=c.h;
        SDL_free(c.px);
    }
    skinUnit=(int)U;skinTheme=gPortConfig.touchTheme;
}
/* a texture at 1:1 centred on (x,y), cut to the screen (SDL's software renderer reads past a texture drawn partly
   off the canvas: DEBUG_WORKFLOW.md) */
static void Blit(SDL_Renderer* r,SDL_Texture* t,int tw,int th,float x,float y,int ow,int oh) {
    SDL_FRect src={0,0,(float)tw,(float)th},dst={SDL_roundf(x-tw*.5f),SDL_roundf(y-th*.5f),(float)tw,(float)th};
    float cut;
    if(!t)return;
    if(dst.x<0) { cut=-dst.x; src.x+=cut; src.w-=cut; dst.x=0; dst.w-=cut; }
    if(dst.y<0) { cut=-dst.y; src.y+=cut; src.h-=cut; dst.y=0; dst.h-=cut; }
    if(dst.x+dst.w>ow) { cut=dst.x+dst.w-ow; src.w-=cut; dst.w-=cut; }
    if(dst.y+dst.h>oh) { cut=dst.y+dst.h-oh; src.h-=cut; dst.h-=cut; }
    if(dst.w<=0 || dst.h<=0)return;
    SDL_RenderTexture(r,t,&src,&dst);
}
static void Place(float x,float y,int w,int h,float* px,float* py) {
    *px=x*w;*py=y*h;
#if defined(__EMSCRIPTEN__) || defined(PORT_TOUCH_CENTERED_LAYOUT)
    *px=layoutX+x*layoutW;*py=layoutY+y*layoutH;
#endif
}
void PortAndroidTouchDraw(SDL_Renderer* r) {
#ifdef __EMSCRIPTEN__
    if(!touchEnabled)return;
#endif
    int w,h,b; uint16_t keys=PortAndroidTouchKeys();
    float U,px,py;
    if(AllHidden())return;
    SDL_GetCurrentRenderOutputSize(r,&w,&h); if(w<=0 || h<=0)return;
    aspect=(float)w/h;
    U=(float)h;
#if defined(__EMSCRIPTEN__) || defined(PORT_TOUCH_CENTERED_LAYOUT)
    CenterLayout(w,h);
    U=layoutH;
#endif
    if(skinUnit!=(int)U || skinTheme!=gPortConfig.touchTheme) BuildSkins(r,U);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    for(b=0;b<(int)SDL_arraysize(buttons);b++) {
        int on=editing ? kButtonGroup[b]==dragGroup : (keys&buttons[b].bits)==buttons[b].bits; /* the dragged group lit */
        float gx,gy;
        if(gPortConfig.hideTouchControls && !PortUiActive() && b!=(int)SDL_arraysize(buttons)-1) continue;
        GroupAt(kButtonGroup[b],&gx,&gy);
        Place(gx+buttons[b].ox/aspect,gy+buttons[b].oy,w,h,&px,&py);
        Blit(r,skins[b].tex[on],skins[b].w,skins[b].h,px,py,w,h);
    }
    if(gPortConfig.hideTouchControls && !PortUiActive()) return;
    if(editing)
        for(b=0;b<2;b++) {
            Place(kTool[b][0],kTool[b][1],w,h,&px,&py);
            Blit(r,tools[b].tex[0],tools[b].w,tools[b].h,px,py,w,h);
        }
    {
        float gx,gy;
        GroupAt(G_DPAD,&gx,&gy);
        Place(gx,gy,w,h,&px,&py);
    }
    Blit(r,skins[CTRL_DPAD].tex[editing ? dragGroup==G_DPAD : (keys&0xF0)!=0],skins[CTRL_DPAD].w,skins[CTRL_DPAD].h,px,py,w,h);
    {
        static const uint16_t bit[4]={0x40,0x80,0x20,0x10};
        static const float dir[4][2]={{0,-1},{0,1},{-1,0},{1,0}};
        float arm=.175f*U*.62f;
        for(b=0;b<4;b++) {
            SDL_Texture* t=arrows[b].tex[0];
            if(!t)continue;
            if(keys&bit[b]) SDL_SetTextureColorMod(t,214,36,40);
            else SDL_SetTextureColorMod(t,(Uint8)(kThemes[SDL_clamp(gPortConfig.touchTheme,0,2)].ink.r*255),
                                        (Uint8)(kThemes[SDL_clamp(gPortConfig.touchTheme,0,2)].ink.g*255),
                                        (Uint8)(kThemes[SDL_clamp(gPortConfig.touchTheme,0,2)].ink.b*255));
            Blit(r,t,arrows[b].w,arrows[b].h,px+dir[b][0]*arm,py+dir[b][1]*arm,w,h);
        }
    }
}
