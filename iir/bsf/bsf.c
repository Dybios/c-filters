#include "bsf.h"

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
    ct->freq_l = 0;
    ct->freq_h = 0;

    printf("Init Success \n");
}

int32_t process_sample(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count) {
    context_t *ct = (context_t*) context;

    float in[FRAME_LEN];
    float out_hpf[FRAME_LEN];
    float out_lpf[FRAME_LEN];
    float out[FRAME_LEN];

    // Create an HPF instance
    hpf_t *hpf = (hpf_t *) malloc(sizeof(hpf_t));
    memset(hpf, 0, sizeof(hpf_t));

    // Create an LPF instance
    lpf_t *lpf = (lpf_t *) malloc(sizeof(lpf_t));
    memset(lpf, 0, sizeof(lpf_t));

    if (ct->freq_l != 0 || ct->freq_h != 0) {
        // Calculate LPF and HPF decay values
        hpf->decay = 1 / ((2 * M_PI * ct->freq_h) + 1);
        lpf->decay = exp(-2 * M_PI * ct->freq_l);

        // Derive HPF and LPF coeffs
        hpf->a = hpf->decay;
        lpf->a = lpf->decay;
        lpf->b = 1 - lpf->a;

        if (frame_count == 0) {
            // If first frame, initialize to first input value.
            output_buffer[0] = input_buffer[0];
        } else {
            // If not, set the first value to the last processed value of the previous frame.
            output_buffer[0] = output_buffer[FRAME_LEN - 1];
        }
        out[0] = (float)((output_buffer[0]) * ONEOVERSHORTMAX);

        for (unsigned int i = 1; i < FRAME_LEN; ++i)
        {
            // Convert to float
            in[i] = (float)((input_buffer[i]) * ONEOVERSHORTMAX);

            // Run the HPF filter over the frame length
            // Use previous value to update the new value
            out_hpf[i] = hpf->a * (out_hpf[i - 1] + (in[i] - in[i - 1]));

            // Run the LPF filter over the frame length
            out_lpf[i] = out_lpf[i - 1] + (lpf->b * (in[i] - out_lpf[i - 1]));

            // Sum the two outputs to give the required band stop
            out[i] = out_hpf[i] + out_lpf[i];

            output_buffer[i] = (short)(out[i] * 32767);
        }
    } else {
        for (unsigned int i = 0; i < FRAME_LEN; ++i) {
             output_buffer[i] = input_buffer[i];
        }
    }

    // Free the instance
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
