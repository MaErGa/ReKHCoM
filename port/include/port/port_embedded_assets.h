#ifndef PORT_EMBEDDED_ASSETS_H
#define PORT_EMBEDDED_ASSETS_H
#include <stddef.h>
#include <stdio.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "port/port_font_resources.h"

/* The module owns these immutable bytes for the process's lifetime. */
static inline const void* PortEmbeddedAssetData(const char* name, size_t* size) {
    HMODULE module = GetModuleHandleW(NULL);
    HRSRC resource;
    HGLOBAL loaded;
    const void* data;
    if (!name || !size) return NULL;
    if (!lstrcmpA(name, "assets/fonts/KHMenu.otf"))
        resource = FindResourceA(module, MAKEINTRESOURCEA(PORT_FONT_KHMENU_RESOURCE), MAKEINTRESOURCEA(10));
    else if (!lstrcmpA(name, "assets/fonts/ComicHearts.otf"))
        resource = FindResourceA(module, MAKEINTRESOURCEA(PORT_FONT_COMIC_HEARTS_RESOURCE), MAKEINTRESOURCEA(10));
    else {
        unsigned long long key = 14695981039346656037ULL;
        const unsigned char* p = (const unsigned char*)name;
        char resourceName[40];
        while (*p) { key ^= *p++; key *= 1099511628211ULL; }
        snprintf(resourceName, sizeof(resourceName), "CORE_ASSET_%016llX", key);
        resource = FindResourceA(module, resourceName, MAKEINTRESOURCEA(10));
    }
    if (!resource || !(loaded = LoadResource(module, resource)) || !(data = LockResource(loaded))) return NULL;
    *size = (size_t)SizeofResource(module, resource);
    return *size ? data : NULL;
}
#else
static inline const void* PortEmbeddedAssetData(const char* name, size_t* size) {
    (void)name; (void)size;
    return NULL;
}
#endif
#endif
