#include "lpf.h"
#include <string.h>

#define ONEOVERSHORTMAX 3.0517578125e-5f // 1/32768

typedef struct context_t {
    float cutoff_freq; // cutoff frequency
    short prev_frame_in[2];
    short prev_frame_out[2];
    short *input_buffer;
    short *output_buffer;
} context_t;

typedef struct lpf_t {
   float a0, a1, a2;
   float b0, b1, b2;
} lpf_t;

int32_t get_lpf_mem_size(void) {
    int32_t mem_size = 0;
    mem_size += sizeof(float); // fc
    mem_size += 2 * (2 * sizeof(short)); // prev values
    mem_size += (2 * FRAME_LEN * sizeof(short)); // inbuf and outbuf
    return mem_size;
}

static void generate_coeffs(lpf_t *lpf, float cutoff_freq) {
    int32_t fs = 44100; // TODO: get this dynamically

    // wc
    float wc = 2 * M_PI * cutoff_freq;
    // Calculate k
    float k = wc / tan(M_PI * cutoff_freq/fs);

    // Input coeffs
    lpf->b0 = pow(wc, 2) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));
    lpf->b1 = (2 * pow(wc, 2) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k)));
    lpf->b2 = pow(wc, 2) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));

    // Output coeffs
    lpf->a0 = 1;
    lpf->a1 = ((2 * pow(wc, 2)) - (2 * pow(k, 2))) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));
    lpf->a2 = (pow(wc, 2) + pow(k, 2) - (sqrt(2) * wc * k)) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));
}

void init_lpf(void* context) {
    /* Initialize context to 0*/
    printf("Init Function\n");

    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));

    // Move pointer to start of buffers
    context += sizeof(float);
    context += 2 * 2 * sizeof(short);

    ct->input_buffer = (short*)context;
    memset(ct->input_buffer, 0, FRAME_LEN*sizeof(short));

    context += FRAME_LEN * sizeof(short);
    ct->output_buffer = (short*) context;
    memset(ct->output_buffer, 0, FRAME_LEN*sizeof(short));

    /* Set default value to 0 */
    ct->cutoff_freq = 0;

    printf("Init Success \n");
}

int32_t process_lpf(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count) {
    context_t *ct = (context_t*) context;

    float in[FRAME_LEN];
    float out[FRAME_LEN];

    // Create an LPF instance
    lpf_t *lpf = (lpf_t *) malloc(sizeof(lpf_t));
    memset(lpf, 0, sizeof(lpf_t));

    if (ct->cutoff_freq != 0) {
        // Generate the coeffs for the filter based on freq
        generate_coeffs(lpf, ct->cutoff_freq);

        // Use the prev state values for the processing correctly
        in[0] = (float)((input_buffer[0]) * ONEOVERSHORTMAX);
        in[1] = (float)((input_buffer[1]) * ONEOVERSHORTMAX);
        if (frame_count == 0) {
            // If first frame, initialize to first three input values.
            output_buffer[0] = input_buffer[0];
            output_buffer[1] = input_buffer[1];
            out[0] = (float)((output_buffer[0]) * ONEOVERSHORTMAX);
            out[1] = (float)((output_buffer[1]) * ONEOVERSHORTMAX);
        } else {
            // If not, set the first output value to the last processed value of the previous frame.
            float f_prev_frame_out1 = (float)(ct->prev_frame_out[0] * ONEOVERSHORTMAX);
            float f_prev_frame_out2 = (float)(ct->prev_frame_out[1] * ONEOVERSHORTMAX);
            float f_prev_frame_in1 = (float)(ct->prev_frame_in[0] * ONEOVERSHORTMAX);
            float f_prev_frame_in2 = (float)(ct->prev_frame_in[1] * ONEOVERSHORTMAX);

            out[0] = (lpf->b0 * in[0]) + (lpf->b1 * f_prev_frame_in1) + (lpf->b2 * f_prev_frame_in2)
                        - (lpf->a1 * f_prev_frame_out1) - (lpf->a2 * f_prev_frame_out2);
            out[1] = (lpf->b0 * in[1]) + (lpf->b1 * in[0]) + (lpf->b2 * f_prev_frame_in1)
                        - (lpf->a1 * out[0]) - (lpf->a2 * f_prev_frame_out1);

            output_buffer[0] = (short)(out[0] * 32767);
            output_buffer[1] = (short)(out[1] * 32767);
        }

        // Run the filter over the frame length
        for (unsigned int i = 2; i < FRAME_LEN; ++i)
        {
            // Convert to float
            in[i] = (float)((input_buffer[i]) * ONEOVERSHORTMAX);

            // Use previous value to update the new value
            out[i] = (lpf->b0 * in[i]) + (lpf->b1 * in[i-1]) + (lpf->b2 * in[i-2])
                        - (lpf->a1 * out[i-1]) - (lpf->a2 * out[i-2]);

            output_buffer[i] = (short)(out[i] * 32767);
        }
    } else {
        for (unsigned int i = 0; i < FRAME_LEN; ++i) {
            output_buffer[i] = input_buffer[i];
        }
    }

    // Store the previous frame in-out value stats in context
    ct->prev_frame_in[0] = input_buffer[FRAME_LEN - 1];
    ct->prev_frame_in[1] = input_buffer[FRAME_LEN - 2];
    ct->prev_frame_out[0] = output_buffer[FRAME_LEN - 1];
    ct->prev_frame_out[1] = output_buffer[FRAME_LEN - 2];

    // Free the instance
    free(lpf);
}

int32_t set_lpf_param(void* context, float value) {
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

int32_t get_lpf_param(void* context) {
    context_t* ct = (context_t*) context;

    /* Retrieves the value of the parameter "alpha" from the context. */
//    printf("cutoff_freq = %f \n", ct->cutoff_freq);
    return ct->cutoff_freq;
}

void deinit_lpf(void* context) {
    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));
}
