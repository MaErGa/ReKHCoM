#ifndef PORT_ANDROID_H
#define PORT_ANDROID_H
#include <stdint.h>
struct SDL_Renderer;
void PortAndroidTouchEvent(const void* event);
uint16_t PortAndroidTouchKeys(void);
void PortAndroidTouchClear(void);
void PortAndroidTouchSettingsChanged(void);
void PortAndroidTouchDraw(struct SDL_Renderer* renderer);
void PortAndroidTouchEditStart(void); /* the layout editor: drag the touch buttons (Options, "Move touch buttons") */
#endif
