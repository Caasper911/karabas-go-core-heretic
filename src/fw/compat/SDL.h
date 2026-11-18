/* karabas: the bits of SDL that chocolate-doom 2.3.0 opl/opl.c and src/midifile.c use, for the bare
   rv32 build (no threads: the OPL callbacks run in the timer interrupt, kg_irq.c). */
#pragma once
#include <stdint.h>
#define SDL_SwapBE16(x) ((uint16_t) __builtin_bswap16((uint16_t) (x)))
#define SDL_SwapBE32(x) ((uint32_t) __builtin_bswap32((uint32_t) (x)))
#define SDL_SwapLE16(x) ((uint16_t) (x))
#define SDL_SwapLE32(x) ((uint32_t) (x))
typedef struct SDL_mutex SDL_mutex;
typedef struct SDL_cond SDL_cond;
SDL_mutex *SDL_CreateMutex(void);
SDL_cond *SDL_CreateCond(void);
void SDL_DestroyMutex(SDL_mutex *m);
void SDL_DestroyCond(SDL_cond *c);
int SDL_LockMutex(SDL_mutex *m);
int SDL_UnlockMutex(SDL_mutex *m);
int SDL_CondSignal(SDL_cond *c);
int SDL_CondWait(SDL_cond *c, SDL_mutex *m);   /* returns at once: the caller loops on its flag */
