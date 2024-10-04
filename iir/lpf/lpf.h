#ifndef _LPF_H_
#define _LPF_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FRAME_LEN 1024 // Buffer size assumed

int32_t get_lpf_mem_size(void);

void init_lpf(void* context);

int32_t process_lpf(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count);

int32_t set_lpf_param(void* context, float value);

int32_t get_lpf_param(void* context);

void deinit_lpf(void* context);

#endif /* _LPF_H */
