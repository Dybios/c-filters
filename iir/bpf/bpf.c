#include "bpf.h"

const float ONEOVERSHORTMAX = 3.0517578125e-5f; // 1/32768

int32_t get_mem_size(void) {
    int32_t mem_size = 0;
    mem_size += 2 * sizeof(float); // fc
    mem_size += 3 * (2 * sizeof(short)); // prev values
    mem_size += (2 * FRAME_LEN * sizeof(short)); // inbuf and outbuf
    return mem_size;
}

static void generate_coeffs(lpf_t *lpf, hpf_t *hpf, float freq_l, float freq_h) {
    int32_t fs = 44100; // TODO: get this dynamically

    /* LPF */
    // wc
    float wc = 2 * M_PI * freq_h;
    // Calculate k
    float k = wc / tan(M_PI * freq_h/fs);

    // Input coeffs
    lpf->b0 = pow(wc, 2) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));
    lpf->b1 = (2 * pow(wc, 2) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k)));
    lpf->b2 = pow(wc, 2) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));

    // Output coeffs
    lpf->a0 = 1;
    lpf->a1 = ((2 * pow(wc, 2)) - (2 * pow(k, 2))) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));
    lpf->a2 = (pow(wc, 2) + pow(k, 2) - (sqrt(2) * wc * k)) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));

    /* HPF */
    // wc
    wc = 2 * M_PI * freq_l;
    // Calculate k
    k = wc / tan(M_PI * freq_l/fs);

    // Input coeffs
    hpf->b0 = pow(k, 2) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));
    hpf->b1 = (-2 * pow(k, 2) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k)));
    hpf->b2 = pow(k, 2) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));

    // Output coeffs
    hpf->a0 = 1;
    hpf->a1 = ((2 * pow(wc, 2)) - (2 * pow(k, 2))) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));
    hpf->a2 = (pow(wc, 2) + pow(k, 2) - (sqrt(2) * wc * k)) / (pow(wc, 2) + pow(k, 2) + (sqrt(2) * wc * k));
}

void init(void* context) {
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

    /* Set default value to 0 */
    ct->freq_l = 0;
    ct->freq_h = 0;

    printf("Init Success \n");
}

int32_t process_sample(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count) {
    context_t *ct = (context_t*) context;

    float in[FRAME_LEN];
    float out_hpf[FRAME_LEN];
    float out_lpf[FRAME_LEN];
    short short_out_hpf[FRAME_LEN];
    short short_out_lpf[FRAME_LEN];

    // Create an HPF instance
    lpf_t *lpf = (hpf_t *) malloc(sizeof(lpf_t));
    memset(lpf, 0, sizeof(lpf_t));

    // Create an HPF instance
    hpf_t *hpf = (hpf_t *) malloc(sizeof(hpf_t));
    memset(hpf, 0, sizeof(hpf_t));

    if (ct->freq_l != 0 || ct->freq_h != 0) {
        // Generate the coeffs for the filter based on freq
        generate_coeffs(lpf, hpf, ct->freq_l, ct->freq_h);

        // Use the prev state values for the processing correctly
        in[0] = (float)((input_buffer[0]) * ONEOVERSHORTMAX);
        in[1] = (float)((input_buffer[1]) * ONEOVERSHORTMAX);
        if (frame_count == 0) {
            // If first frame, initialize to first three input values.
            output_buffer[0] = input_buffer[0];
            output_buffer[1] = input_buffer[1];
            out_lpf[0] = (float)((output_buffer[0]) * ONEOVERSHORTMAX);
            out_lpf[1] = (float)((output_buffer[1]) * ONEOVERSHORTMAX);
            out_hpf[0] = out_lpf[0];
            out_hpf[1] = out_lpf[1];
        } else {
            // If not, set the first output value to the last processed value of the previous frame.
            float f_prev_frame_out1_lpf = (float)(ct->prev_frame_out_lpf[0] * ONEOVERSHORTMAX);
            float f_prev_frame_out2_lpf = (float)(ct->prev_frame_out_lpf[1] * ONEOVERSHORTMAX);
            float f_prev_frame_out1_hpf = (float)(ct->prev_frame_out_hpf[0] * ONEOVERSHORTMAX);
            float f_prev_frame_out2_hpf = (float)(ct->prev_frame_out_hpf[1] * ONEOVERSHORTMAX);
            float f_prev_frame_in1 = (float)(ct->prev_frame_in[0] * ONEOVERSHORTMAX);
            float f_prev_frame_in2 = (float)(ct->prev_frame_in[1] * ONEOVERSHORTMAX);

            out_lpf[0] = (lpf->b0 * in[0]) + (lpf->b1 * f_prev_frame_in1) + (lpf->b2 * f_prev_frame_in2)
                        - (lpf->a1 * f_prev_frame_out1_lpf) - (lpf->a2 * f_prev_frame_out2_lpf);
            out_lpf[1] = (lpf->b0 * in[1]) + (lpf->b1 * in[0]) + (lpf->b2 * f_prev_frame_in1)
                        - (lpf->a1 * out_lpf[0]) - (lpf->a2 * f_prev_frame_out1_lpf);

            out_hpf[0] = (hpf->b0 * out_lpf[0]) + (hpf->b1 * f_prev_frame_out1_lpf) + (hpf->b2 * f_prev_frame_out2_lpf)
                        - (hpf->a1 * f_prev_frame_out1_hpf) - (hpf->a2 * f_prev_frame_out2_hpf);
            out_hpf[1] = (hpf->b0 * out_lpf[1]) + (hpf->b1 * out_lpf[0]) + (hpf->b2 * f_prev_frame_out1_lpf)
                        - (hpf->a1 * out_hpf[0]) - (hpf->a2 * f_prev_frame_out1_hpf);

            output_buffer[0] = (short)(out_hpf[0] * 32767);
            output_buffer[1] = (short)(out_hpf[1] * 32767);
        }

        for (unsigned int i = 2; i < FRAME_LEN; ++i)
        {
            // Convert to float
            in[i] = (float)((input_buffer[i]) * ONEOVERSHORTMAX);

            // Use previous value to update the new value
            out_lpf[i] = (lpf->b0 * in[i]) + (lpf->b1 * in[i-1]) + (lpf->b2 * in[i-2])
                        - (lpf->a1 * out_lpf[i-1]) - (lpf->a2 * out_lpf[i-2]);
            short_out_lpf[i] = (short)(out_lpf[i] * 32767);

            out_hpf[i] = (hpf->b0 * out_lpf[i]) + (hpf->b1 * out_lpf[i-1]) + (hpf->b2 * out_lpf[i-2])
                        - (hpf->a1 * out_hpf[i-1]) - (hpf->a2 * out_hpf[i-2]);
            short_out_hpf[i] = (short)(out_hpf[i] * 32767);

            output_buffer[i] = (short)(out_hpf[i] * 32767);
        }
    } else {
        for (unsigned int i = 0; i < FRAME_LEN; ++i) {
            output_buffer[i] = input_buffer[i];
        }
    }

    // Store the previous frame in-out value stats in context
    ct->prev_frame_in[0] = input_buffer[FRAME_LEN - 1];
    ct->prev_frame_in[1] = input_buffer[FRAME_LEN - 2];
    ct->prev_frame_out_lpf[0] = short_out_lpf[FRAME_LEN - 1];
    ct->prev_frame_out_lpf[1] = short_out_lpf[FRAME_LEN - 2];
    ct->prev_frame_out_hpf[0] = short_out_hpf[FRAME_LEN - 1];
    ct->prev_frame_out_hpf[1] = short_out_hpf[FRAME_LEN - 2];

    // Free the instance
    free(lpf);
    free(hpf);
}

int32_t set_param(void* context, float value1, float value2) {
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

int32_t get_param(void* context) {
    context_t* ct = (context_t*) context;

    /* Retrieves the value of the parameter "alpha" from the context. */
//    printf("cutoff_freq = %f \n", ct->cutoff_freq);
    return ct->freq_l;
}

void deinit(void* context) {
    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));
}
