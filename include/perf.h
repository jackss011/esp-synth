#include <esp_timer.h>
#include <esp_log.h>
#include <Arduino.h>

#define PERF_TAG "PERF_TIMER"


#define START_PERF(n) \
static uint32_t __count_##n = 0; \
static uint32_t __accum_##n = 0; \
uint32_t __start_##n = esp_timer_get_time();


#define STOP_PERF(n, every) \
__count_##n ++; \
uint32_t __diff_##n = esp_timer_get_time() - __start_##n; \
__accum_##n = __accum_##n > __diff_##n ? __accum_##n : __diff_##n; \
if(__count_##n % every == 0) { ESP_LOGE(PERF_TAG, "[" #n "]> %dus", __accum_##n); __accum_##n = 0; }

#define FORCE_INLINE __attribute__((always_inline)) inline
