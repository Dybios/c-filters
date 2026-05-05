#include "bsf.h"
#include <string.h>

#define ONEOVERSHORTMAX 3.0517578125e-5f // 1/32768

typedef struct context_t {
    float freq_l; // Low cutoff
    float freq_h; // High cutoff
    short prev_frame_in[2];
    short prev_frame_out_bsf[2];
    short *input_buffer;
    short *output_buffer;
} context_t;

typedef struct bsf_t {
   float a0, a1, a2;
   float b0, b1, b2;
} bsf_t;

int32_t get_bsf_mem_size(void) {
    int32_t mem_size = 0;
    mem_size += 2 * sizeof(float); // fl and fh
    mem_size += 3 * (2 * sizeof(short)); // prev values
    mem_size += (2 * FRAME_LEN * sizeof(short)); // inbuf and outbuf
    return mem_size;
}

static void generate_coeffs(bsf_t *bsf, float freq_l, float freq_h) {
    int32_t fs = 44100; // TODO: get this dynamically

    /* BSF */
    // wc
    float wh = 2 * M_PI * freq_h / fs;
    float wl = 2 * M_PI * freq_l / fs;
    // Calculate k
    float k = tan((wh - wl) / 2);
    float cosWc = cos((wh + wl) / 2) / cos((wh - wl) / 2);

    // Input coeffs
    bsf->b0 = 1 / (1 + k);
    bsf->b1 = -2 * cosWc / (1 + k);
    bsf->b2 = 1 / (1 + k);

    // Output coeffs
    bsf->a0 = 1;
    bsf->a1 = 2 * cosWc / (1 + k);
    bsf->a2 = (1 - k) / (1 + k);
}

void init_bsf(void* context) {
    /* Initialize context to 0*/
    printf("Init Function\n");

    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));

    // Move pointer to start of buffers
    context += 2 * sizeof(float);
    context += 3 * (2 * sizeof(short));

    ct->input_buffer = (short*)context;
    memset(ct->input_buffer, 0, FRAME_LEN*sizeof(short));

    context += FRAME_LEN * sizeof(short);
    ct->output_buffer = (short*) context;
    memset(ct->output_buffer, 0, FRAME_LEN*sizeof(short));

    /* Set default values to 0 */
    ct->freq_l = 0;
    ct->freq_h = 0;

    printf("Init Success \n");
}

int32_t process_bsf(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count) {
    context_t *ct = (context_t*) context;

    float in[FRAME_LEN];
    float out_bsf[FRAME_LEN];
    short short_out_bsf[FRAME_LEN];

    // Create an HPF instance
    bsf_t *bsf = (bsf_t *) malloc(sizeof(bsf_t));
    memset(bsf, 0, sizeof(bsf_t));

    if (ct->freq_l != 0 || ct->freq_h != 0) {
        // Generate the coeffs for the filter based on freq
        generate_coeffs(bsf, ct->freq_l, ct->freq_h);

        // Use the prev state values for the processing correctly
        in[0] = (float)((input_buffer[0]) * ONEOVERSHORTMAX);
        in[1] = (float)((input_buffer[1]) * ONEOVERSHORTMAX);
        if (frame_count == 0) {
            // If first frame, initialize to first three input values.
            output_buffer[0] = input_buffer[0];
            output_buffer[1] = input_buffer[1];
            out_bsf[0] = (float)((output_buffer[0]) * ONEOVERSHORTMAX);
            out_bsf[1] = (float)((output_buffer[1]) * ONEOVERSHORTMAX);
        } else {
            // If not, set the first output value to the last processed value of the previous frame.
            float f_prev_frame_out1_bsf = (float)(ct->prev_frame_out_bsf[0] * ONEOVERSHORTMAX);
            float f_prev_frame_out2_bsf = (float)(ct->prev_frame_out_bsf[1] * ONEOVERSHORTMAX);
            float f_prev_frame_in1 = (float)(ct->prev_frame_in[0] * ONEOVERSHORTMAX);
            float f_prev_frame_in2 = (float)(ct->prev_frame_in[1] * ONEOVERSHORTMAX);

            out_bsf[0] = (bsf->b0 * in[0]) + (bsf->b1 * f_prev_frame_in1) + (bsf->b2 * f_prev_frame_in2)
                        - (bsf->a1 * f_prev_frame_out1_bsf) - (bsf->a2 * f_prev_frame_out2_bsf);
            out_bsf[1] = (bsf->b0 * in[1]) + (bsf->b1 * in[0]) + (bsf->b2 * f_prev_frame_in1)
                        - (bsf->a1 * out_bsf[0]) - (bsf->a2 * f_prev_frame_out1_bsf);

            output_buffer[0] = ((short)(out_bsf[0] * 32767))
                                + ((short)(out_bsf[0] * 32767));
            output_buffer[1] = ((short)(out_bsf[1] * 32767))
                                + ((short)(out_bsf[1] * 32767));
        }

        for (unsigned int i = 2; i < FRAME_LEN; ++i)
        {
            // Convert to float
            in[i] = (float)((input_buffer[i]) * ONEOVERSHORTMAX);

            // Run the HPF filter over the frame length
            // Use previous value to update the new value
            out_bsf[i] = (bsf->b0 * in[i]) + (bsf->b1 * in[i-1]) + (bsf->b2 * in[i-2])
                        - (bsf->a1 * out_bsf[i-1]) - (bsf->a2 * out_bsf[i-2]);
            short_out_bsf[i] = (short)(out_bsf[i] * 32767);

            output_buffer[i] = short_out_bsf[i];
        }
    } else {
        for (unsigned int i = 0; i < FRAME_LEN; ++i) {
            output_buffer[i] = input_buffer[i];
        }
    }

    // Store the previous frame in-out value stats in context
    ct->prev_frame_in[0] = input_buffer[FRAME_LEN - 1];
    ct->prev_frame_in[1] = input_buffer[FRAME_LEN - 2];
    ct->prev_frame_out_bsf[0] = short_out_bsf[FRAME_LEN - 1];
    ct->prev_frame_out_bsf[1] = short_out_bsf[FRAME_LEN - 2];

    // Free the instance
    free(bsf);
}

int32_t set_bsf_param(void* context, float value1, float value2) {
    context_t* ct = (context_t*) context;

    /* Set cutoff frequency */
    if(value1 < 0 || value2 < 0){
        return -1;
    }
    else {
        ct->freq_l = value1;
        ct->freq_h = value2;
        printf("context->cutoff_freq = %lf value =%f \n",ct->freq_l, value1);
        return 0;
    }
}

int32_t get_bsf_param(void* context) {
    context_t* ct = (context_t*) context;

    /* Retrieves the value of the parameter "alpha" from the context. */
//    printf("cutoff_freq = %f \n", ct->cutoff_freq);
    return ct->freq_l;
}

void deinit_bsf(void* context) {
    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));
}
