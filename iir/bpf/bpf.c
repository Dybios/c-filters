#include "bpf.h"

const float ONEOVERSHORTMAX = 3.0517578125e-5f; // 1/32768

int32_t get_mem_size(void) {
    int32_t mem_size = 0;
    mem_size += 2 * sizeof(float); // fl and fh
    mem_size += 3 * sizeof(short); // prev values
    mem_size += (2 * FRAME_LEN * sizeof(short)); // inbuf and outbuf
    return mem_size;
}

static void generate_coeffs(lpf_t *lpf, hpf_t *hpf, float freq_low, float freq_high) {
    // Calculate LPF and HPF decay values
    hpf->decay = 1 / ((2 * M_PI * freq_low) + 1);
    lpf->decay = exp(-2 * M_PI * freq_high);

    // Derive HPF and LPF coeffs
    hpf->a = hpf->decay;
    lpf->a = lpf->decay;
    lpf->b = 1 - lpf->a;
}

void init(void* context) {
    /* Initialize context to 0*/
    printf("Init Function\n");

    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));

    // Move pointer to start of buffers
    context += 2 * sizeof(float);
    context += 3 * sizeof(short);

    ct->input_buffer = (short*)context;
    memset(ct->input_buffer, 0, FRAME_LEN*sizeof(short));

    context += FRAME_LEN * sizeof(short);
    ct->output_buffer = (short*) context;
    memset(ct->output_buffer, 0, FRAME_LEN*sizeof(short));

    /* Set default values to 0 */
    ct->freq_l = 0;
    ct->freq_h = 0;
    ct->prev_frame_in = 0;
    ct->prev_frame_out_lpf = 0;
    ct->prev_frame_out_hpf = 0;

    printf("Init Success \n");
}

int32_t process_sample(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count) {
    context_t *ct = (context_t*) context;

    float in[FRAME_LEN];
    float out_lpf[FRAME_LEN];
    float out_hpf[FRAME_LEN];
    short short_out_hpf[FRAME_LEN];
    short short_out_lpf[FRAME_LEN];

    // Create an HPF instance
    hpf_t *hpf = (hpf_t *) malloc(sizeof(hpf_t));
    memset(hpf, 0, sizeof(hpf_t));

    // Create an LPF instance
    lpf_t *lpf = (lpf_t *) malloc(sizeof(lpf_t));
    memset(lpf, 0, sizeof(lpf_t));

    if (ct->freq_l != 0 || ct->freq_h != 0) {
        // Generate the coeffs for the filters based on freq
        generate_coeffs(lpf, hpf, ct->freq_l, ct->freq_h);

        // Use the prev state values for the processing correctly
        in[0] = (float)((input_buffer[0]) * ONEOVERSHORTMAX);
        if (frame_count == 0) {
            // If first frame, initialize to first input value.
            output_buffer[0] = input_buffer[0];
            out_lpf[0] = (float)((output_buffer[0]) * ONEOVERSHORTMAX);
            out_hpf[0] = out_lpf[0];
        } else {
            // If not, set the first value to the last processed value of the previous frame.
            float f_prev_frame_out_lpf = (float)(ct->prev_frame_out_lpf * ONEOVERSHORTMAX);
            float f_prev_frame_out_hpf = (float)(ct->prev_frame_out_hpf * ONEOVERSHORTMAX);
            float f_prev_frame_in = (float)(ct->prev_frame_in * ONEOVERSHORTMAX);
            out_lpf[0] = f_prev_frame_out_lpf + (lpf->b * (in[0] - f_prev_frame_out_lpf));
            out_hpf[0] = hpf->a * (f_prev_frame_out_hpf + (out_lpf[0] - f_prev_frame_out_lpf));
            output_buffer[0] = (short)(out_hpf[0] * 32767);
        }

        for (unsigned int i = 1; i < FRAME_LEN; ++i)
        {
            // Convert to float
            in[i] = (float)((input_buffer[i]) * ONEOVERSHORTMAX);

            // Use previous value to update the new value
            // Run the LPF filter over the frame length
            out_lpf[i] = out_lpf[i - 1] + (lpf->b * (in[i] - out_lpf[i - 1]));
            short_out_lpf[i] = (short)(out_lpf[i] * 32767);

            // Run the HPF filter over the frame length
            out_hpf[i] = hpf->a * (out_hpf[i - 1] + (out_lpf[i] - out_lpf[i - 1]));
            short_out_hpf[i] = (short)(out_hpf[i] * 32767);

            // Sum the two outputs to give the required band stop
            output_buffer[i] = short_out_hpf[i];
        }
    } else {
        for (unsigned int i = 0; i < FRAME_LEN; ++i) {
            output_buffer[i] = input_buffer[i];
        }
    }

    // Store the previous frame in-out value stats in context
    ct->prev_frame_in = input_buffer[FRAME_LEN - 1];
    ct->prev_frame_out_lpf = short_out_lpf[FRAME_LEN - 1];
    ct->prev_frame_out_hpf = short_out_hpf[FRAME_LEN - 1];

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
