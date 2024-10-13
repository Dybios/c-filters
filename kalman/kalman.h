#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "matrix.h"

#define FRAME_LEN 1024 // Buffer size assumed
#define CTRL_INPUTS 1
#define N 15 // Averagin for state vector

int32_t get_kalman_mem_size(void);

void init_kalman(void* context);

int32_t process_kalman(void* context, int16_t *input_buffer1, int16_t *input_buffer2, int16_t *output_buffer, int32_t frame_count);

int32_t set_kalman_param(void* context, float value);

int32_t get_kalman_param(void* context);

void deinit_kalman(void* context);
