#ifndef _BPF_H_
#define _BPF_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FRAME_LEN 1024 // Buffer size assumed

typedef struct context_t {
    float freq_l; // Low cutoff
    float freq_h; // High cutoff
    short prev_frame_in[2];
    short prev_frame_out_lpf[2];
    short prev_frame_out_hpf[2];
    short *input_buffer;
    short *output_buffer;
} context_t;

typedef struct hpf_t {
   float a0, a1, a2;
   float b0, b1, b2;
} hpf_t;

typedef struct lpf_t {
   float a0, a1, a2;
   float b0, b1, b2;
} lpf_t;

int32_t get_mem_size(void);

void init(void* context);

int32_t process_sample(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count);

int32_t set_param(void* context, float value1, float value2);

int32_t get_param(void* context);

void deinit(void* context);

#endif /* _BPF_H */
