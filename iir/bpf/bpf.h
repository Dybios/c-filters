#ifndef _BPF_H_
#define _BPF_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FRAME_LEN 1024 // Buffer size assumed

int32_t get_bpf_mem_size(void);

void init_bpf(void* context);

int32_t process_bpf(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count);

int32_t set_bpf_param(void* context, float value1, float value2);

int32_t get_bpf_param(void* context);

void deinit_bpf(void* context);

#endif /* _BPF_H */
