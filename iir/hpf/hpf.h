#ifndef _HPF_H_
#define _HPF_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FRAME_LEN 1024 // Buffer size assumed

int32_t get_hpf_mem_size(void);

void init_hpf(void* context);

int32_t process_hpf(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count);

int32_t set_hpf_param(void* context, float value);

int32_t get_hpf_param(void* context);

void deinit_hpf(void* context);

#endif /* _HPF_H */
