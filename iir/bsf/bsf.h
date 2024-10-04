#ifndef _BSF_H_
#define _BSF_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FRAME_LEN 1024 // Buffer size assumed

int32_t get_bsf_mem_size(void);

void init_bsf(void* context);

int32_t process_bsf(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count);

int32_t set_bsf_param(void* context, float value1, float value2);

int32_t get_bsf_param(void* context);

void deinit_bsf(void* context);

#endif /* _BSF_H */
