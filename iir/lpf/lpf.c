#include "lpf.h"

const float ONEOVERSHORTMAX = 3.0517578125e-5f; // 1/32768

int32_t get_mem_size(void) {
    return (sizeof(float) + (2 * FRAME_LEN * sizeof(short)));
}

void init(void* context) {
    /* Initialize context to 0*/
    printf("Init Function\n");

    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));

    context += sizeof(float);
    ct->input_buffer = (short*)context;
    memset(ct->input_buffer, 0, FRAME_LEN*sizeof(short));

    context += FRAME_LEN * sizeof(short);
    ct->output_buffer = (short*) context;
    memset(ct->output_buffer, 0, FRAME_LEN*sizeof(short));

    /* Set default cutoff value to 0 */
    ct->cutoff_freq = 0;

    printf("Init Success \n");
}

int32_t process_sample(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count) {
    context_t *ct = (context_t*) context;

    float in[FRAME_LEN];
    float out[FRAME_LEN];

    // Create an LPF instance
    lpf_t *lpf = (lpf_t *) malloc(sizeof(lpf_t));
    memset(lpf, 0, sizeof(lpf_t));

    if (ct->cutoff_freq != 0) {
        // Calculate decay calue
        lpf->decay = exp(-2 * M_PI * ct->cutoff_freq);

        // Derive a and b value
        lpf->a = lpf->decay;
        lpf->b = 1 - lpf->a;

        in[0] = (float)((input_buffer[0]) * ONEOVERSHORTMAX);
        if (frame_count == 0) {
            // If first frame, initialize to first input value.
            output_buffer[0] = input_buffer[0];
            out[0] = (float)((output_buffer[0]) * ONEOVERSHORTMAX);
        } else {
            // If not, set the first output value to the last processed value of the previous frame.
            float prev_out = (float)((output_buffer[FRAME_LEN-1]) * ONEOVERSHORTMAX);
            out[0] = prev_out + (lpf->b * (in[0] - prev_out));
            output_buffer[0] = (short)(out[0] * 32767);
        }

        for (unsigned int i = 1; i < FRAME_LEN; ++i)
        {
            // Convert to float
            in[i] = (float)((input_buffer[i]) * ONEOVERSHORTMAX);

            // Run the filter over the frame length
            // Use previous value to update the new value
            out[i] = out[i - 1] + (lpf->b * (in[i] - out[i - 1]));

            output_buffer[i] = (short)(out[i] * 32767);
        }
    } else {
        for (unsigned int i = 0; i < FRAME_LEN; ++i) {
             output_buffer[i] = input_buffer[i];
        }
    }

    // Free the instance
    free(lpf);
}

int32_t set_param(void* context, float value) {
    context_t* ct = (context_t*) context;

    /* Set cutoff frequency */
    if(value < 0){
        return -1;
    }
    else {
        ct->cutoff_freq = value;
        printf("context->cutoff_freq = %lf value =%f \n",ct->cutoff_freq, value);
        return 0;
    }
}

int32_t get_param(void* context) {
    context_t* ct = (context_t*) context;

    /* Retrieves the value of the parameter "alpha" from the context. */
//    printf("cutoff_freq = %f \n", ct->cutoff_freq);
    return ct->cutoff_freq;
}

void deinit(void* context) {
    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));
}
