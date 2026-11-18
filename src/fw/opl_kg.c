/* Ф2 (karabas): the OPL driver of chocolate-doom 2.3.0 opl/opl.c for the board: the OPL3 of
 * doom_audio.v (MMIO_OPL_ADDR / MMIO_OPL_DATA). It takes the name opl_sdl_driver, the one driver
 * opl.c lists outside x86 / Windows (the upstream file is used as it is).
 * - Callbacks (i_oplmusic.c: the MIDI events of the song) in an opl_queue, run by the timer
 *   interrupt (kg_irq.c) at their time; the time stops while paused.
 * - The status port is emulated for OPL_Detect: 0 after a timer reset, 0xC0 once timer 1 is
 *   started (bits 2..1 = 0: an OPL3), as a YMF262 answers. */
#include <stdint.h>
#include "opl.h"
#include "opl_internal.h"
#include "opl_queue.h"
#include "mmio.h"
#include "kg_irq.h"

static opl_callback_queue_t *queue;
static int paused;
static uint64_t pause_start, pause_total;     /* microseconds of pauses (the OPL time = us - this) */
static unsigned int reg_num;
static uint8_t status;

static uint64_t opl_now(uint64_t us) { return us - pause_total - (paused ? us - pause_start : 0); }

uint64_t opl_kg_next(void) {
    if (!queue || paused || OPL_Queue_IsEmpty(queue)) return KG_NEVER;
    return OPL_Queue_Peek(queue) + pause_total;
}

void opl_kg_run(uint64_t now) {
    if (!queue || paused) return;
    uint64_t t = opl_now(now);
    opl_callback_t cb;
    void *data;
    while (!OPL_Queue_IsEmpty(queue) && OPL_Queue_Peek(queue) <= t) {
        if (!OPL_Queue_Pop(queue, &cb, &data)) break;
        cb(data);
    }
}

static int Kg_Init(unsigned int port_base) {
    (void) port_base;
    if (!queue) queue = OPL_Queue_Create();
    paused = 0;
    pause_total = 0;
    status = 0;
    return 1;
}

static void Kg_Shutdown(void) {
    kg_lock();
    if (queue) OPL_Queue_Clear(queue);
    kg_unlock();
}

static unsigned int Kg_ReadPort(opl_port_t port) {
    return port == OPL_REGISTER_PORT ? status : 0xff;
}

static void Kg_WritePort(opl_port_t port, unsigned int value) {
    if (port == OPL_REGISTER_PORT) reg_num = value & 0xff;
    else if (port == OPL_REGISTER_PORT_OPL3) reg_num = 0x100 | (value & 0xff);
    else {
        if (reg_num == OPL_REG_TIMER_CTRL) {
            if (value & 0x80) status = 0;
            else if ((value & 0x01) && !(value & 0x40)) status = 0xc0;
        }
        MMIO_OPL_ADDR = reg_num;
        MMIO_OPL_DATA = value & 0xff;
    }
}

static void Kg_SetCallback(uint64_t us, opl_callback_t callback, void *data) {
    kg_lock();
    OPL_Queue_Push(queue, callback, data, opl_now(kg_us()) + us);
    kg_unlock();
    kg_timer_kick();
}

static void Kg_ClearCallbacks(void) {
    kg_lock();
    OPL_Queue_Clear(queue);
    kg_unlock();
}

static void Kg_Lock(void) { kg_lock(); }
static void Kg_Unlock(void) { kg_unlock(); }

static void Kg_SetPaused(int p) {
    kg_lock();
    uint64_t now = kg_us();
    if (p && !paused) pause_start = now;
    if (!p && paused) pause_total += now - pause_start;
    paused = p;
    kg_unlock();
    kg_timer_kick();
}

static void Kg_AdjustCallbacks(float factor) {
    kg_lock();
    OPL_Queue_AdjustCallbacks(queue, opl_now(kg_us()), factor);
    kg_unlock();
}

opl_driver_t opl_sdl_driver = {
    "karabas",
    Kg_Init,
    Kg_Shutdown,
    Kg_ReadPort,
    Kg_WritePort,
    Kg_SetCallback,
    Kg_ClearCallbacks,
    Kg_Lock,
    Kg_Unlock,
    Kg_SetPaused,
    Kg_AdjustCallbacks,
};
