/* karabas: no-op SDL mutexes / conditions (compat/SDL.h). OPL_Delay waits in a loop on a flag set
   by its callback; the callback runs in the timer interrupt, so the wait only has to call out (an
   external call: the flag is read again each time). */
#include "SDL.h"
static int dummy;
SDL_mutex *SDL_CreateMutex(void) { return (SDL_mutex *) &dummy; }
SDL_cond *SDL_CreateCond(void) { return (SDL_cond *) &dummy; }
void SDL_DestroyMutex(SDL_mutex *m) { (void) m; }
void SDL_DestroyCond(SDL_cond *c) { (void) c; }
int SDL_LockMutex(SDL_mutex *m) { (void) m; return 0; }
int SDL_UnlockMutex(SDL_mutex *m) { (void) m; return 0; }
int SDL_CondSignal(SDL_cond *c) { (void) c; return 0; }
int __attribute__((noinline)) SDL_CondWait(SDL_cond *c, SDL_mutex *m) { (void) c; (void) m; __asm__ volatile ("" ::: "memory"); return 0; }
