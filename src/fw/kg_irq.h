/* Ф2 (karabas): timer interrupt, locks, the parts the interrupt runs (kg_irq.c) */
#pragma once
#include <stdint.h>

#define SFX_TICK_US 5000              /* the SFX mixer runs every 5 ms */
#define KG_NEVER    UINT64_MAX

void kg_irq_init(void);
void kg_lock(void);
void kg_unlock(void);
uint64_t kg_us(void);
void kg_timer_kick(void);

void kg_sfx_tick(void);              /* i_kgsound.c */
uint64_t opl_kg_next(void);          /* opl_kg.c: the time of the next OPL callback (KG_NEVER: none) */
void opl_kg_run(uint64_t now);
uint64_t kg_midi_next(void);         /* i_kgmusic.c: the time of the next MIDI event */
void kg_midi_run(uint64_t now);
