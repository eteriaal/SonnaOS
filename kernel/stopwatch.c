#include <stopwatch.h>
#include <drivers/fbtext.h>
#include <drivers/serial.h>
#include <arch/x86_64/apic.h>
#include <klib/string.h>
#include <colors.h>

static bool running = false;
static uint64_t start_tsc = 0;
static uint64_t last_display = 0;

void stopwatch_init(void)
{
    running = false;
    start_tsc = 0;
    last_display = 0;
}

void stopwatch_toggle(void)
{
    if (running) {
        running = false;
        fb_print("stopwatch STOPPED\n", COL_WARNING);
        serial_puts("stopwatch STOPPED\n");
    } else {
        running = true;
        start_tsc = timer_get_tsc();
        last_display = start_tsc;
        fb_print("stopwatch STARTED\n", COL_USED);
        serial_puts("stopwatch STARTED\n");
    }
}

bool stopwatch_is_running(void)
{
    return running;
}

uint64_t stopwatch_get_elapsed_seconds(void)
{
    if (!running) return 0;
    uint64_t now = timer_get_tsc();
    return (now - start_tsc) / tsc_frequency_hz;
}

void stopwatch_update(uint64_t current_tsc, uint64_t tsc_per_sec)
{
    if (!running) return;

    if (current_tsc - last_display >= tsc_per_sec)
    {
        uint64_t elapsed = (current_tsc - start_tsc) / tsc_per_sec;

        char buf[32];
        u64_to_dec(elapsed, buf);

        fb_print("Time: ", COL_LABEL);
        fb_print(buf, COL_VALUE);
        fb_print(" s   \r", COL_LABEL);

        serial_puts("Time: ");
        serial_puts(buf);
        serial_puts(" s\r");

        last_display = current_tsc;
    }
}