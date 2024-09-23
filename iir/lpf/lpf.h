#ifndef _LPF_H_
#define _LPF_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FRAME_LEN 1024 // Buffer size assumed

typedef struct context_t {
   float cutoff_freq; // cutoff frequency
   short *input_buffer;
   short *output_buffer;
} context_t;

typedef struct lpf_t {
   float decay;
   float a;
   float b;
} lpf_t;

int32_t get_mem_size(void);

void init(void* context);

int32_t process_sample(void* context, int16_t *input_buffer, int16_t *output_buffer);

int32_t set_param(void* context, float value);

int32_t get_param(void* context);

void deinit(void* context);

#endif /* _LPF_H */
