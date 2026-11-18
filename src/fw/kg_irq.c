/* Ф2 (karabas): the timer interrupt and the traps of the board firmware.
 * - kg_trap() (trap.S): a timer interrupt runs the due work: the SFX mixer tick (i_kgsound.c, every
 *   SFX_TICK_US), the OPL callbacks (opl_kg.c) and the MIDI sequencer (i_kgmusic.c), then sets the
 *   next compare (the earliest of their times). Any other trap (a misaligned access, an illegal
 *   instruction) prints its cause and exits (the log on the screen, kg_hw.c).
 * - kg_lock() / kg_unlock(): the main code against the interrupt (nested; OPL_Lock, the mixer). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"
#include "kg_irq.h"

static volatile int lock_depth;
static int irq_ready;               /* MIE only after kg_irq_init (mtvec set) */

static inline void irq_off(void) { __asm__ volatile ("csrc mstatus, 8" ::: "memory"); }
static inline void irq_on(void) { if (irq_ready) __asm__ volatile ("csrs mstatus, 8" ::: "memory"); }

void kg_lock(void) { irq_off(); lock_depth++; }
void kg_unlock(void) { if (--lock_depth == 0) irq_on(); }

/* 64-bit microseconds from the 32-bit counter (read at least once per 71 minutes: the tick does) */
static uint32_t us_hi, us_last;
uint64_t kg_us(void) {
    uint32_t lo = MMIO_US;
    kg_lock();
    if (lo < us_last) us_hi++;
    us_last = lo;
    uint64_t t = ((uint64_t) us_hi << 32) | lo;
    kg_unlock();
    return t;
}

static uint64_t sfx_next;

static void reprogram(uint64_t now) {
    uint64_t t = sfx_next;
    uint64_t o = opl_kg_next(), m = kg_midi_next();
    if (o < t) t = o;
    if (m < t) t = m;
    if (t < now + 20) t = now + 20;
    MMIO_TCMP = (uint32_t) t;
    MMIO_TEN = 1;
}

void kg_timer_kick(void) {
    /* new work scheduled by the main code: the compare may have to come earlier */
    kg_lock();
    reprogram(kg_us());
    kg_unlock();
}

void kg_irq_init(void) {
    extern void kg_trap_entry(void);
    __asm__ volatile ("csrw mtvec, %0" :: "r"(kg_trap_entry));
    __asm__ volatile ("csrs mie, %0" :: "r"(1u << 7));       /* MTIE */
    lock_depth = 0;
    sfx_next = kg_us() + SFX_TICK_US;
    reprogram(kg_us());
    irq_ready = 1;
    irq_on();
}

void kg_trap(uint32_t cause, uint32_t epc, uint32_t tval) {
    if (cause == 0x80000007u) {
        lock_depth++;                 /* kg_lock() inside the handlers must not turn MIE on */
        uint64_t now = kg_us();
        if (now >= sfx_next) {
            kg_sfx_tick();
            sfx_next += SFX_TICK_US;
            if (sfx_next <= now) sfx_next = now + SFX_TICK_US;
        }
        opl_kg_run(now);
        kg_midi_run(now);
        reprogram(kg_us());
        lock_depth--;
        return;
    }
    MMIO_TEN = 0;
    printf("TRAP mcause %08lx mepc %08lx mtval %08lx\n", (unsigned long) cause, (unsigned long) epc, (unsigned long) tval);
    exit(-2);
}
