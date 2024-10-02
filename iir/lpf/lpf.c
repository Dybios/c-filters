#include "lpf.h"

const float ONEOVERSHORTMAX = 3.0517578125e-5f; // 1/32768

int32_t get_mem_size(void) {
    int32_t mem_size = 0;
    mem_size += sizeof(float); // fc
    mem_size += 2 * sizeof(short); // prev values
    mem_size += (2 * FRAME_LEN * sizeof(short)); // inbuf and outbuf
    return mem_size;
}

static void generate_coeffs(lpf_t *lpf, float cutoff_freq) {
    // Calculate a and b coeff values
    lpf->a = exp(-2 * M_PI * cutoff_freq); // a = decay
    lpf->b = 1 - lpf->a;
}

void init(void* context) {
    /* Initialize context to 0*/
    printf("Init Function\n");

    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));

    // Move pointer to start of buffers
    context += sizeof(float);
    context += 2 * sizeof(short);

    ct->input_buffer = (short*)context;
    memset(ct->input_buffer, 0, FRAME_LEN*sizeof(short));

    context += FRAME_LEN * sizeof(short);
    ct->output_buffer = (short*) context;
    memset(ct->output_buffer, 0, FRAME_LEN*sizeof(short));

    /* Set default value to 0 */
    ct->cutoff_freq = 0;
    ct->prev_frame_in = 0;
    ct->prev_frame_out = 0;

    printf("Init Success \n");
}

int32_t process_sample(void* context, int16_t *input_buffer, int16_t *output_buffer, int32_t frame_count) {
    context_t *ct = (context_t*) context;

    float in[FRAME_LEN];
    float out[FRAME_LEN];

    // Create an LPF instance
    lpf_t *lpf = (lpf_t *) malloc(sizeof(lpf_t));
    memset(lpf, 0, sizeof(lpf_t));

    //printf("curr input_buffer val = %d | last output_buffer val = %d | ct->prev_frame_in = %d | ct->prev_frame_out = %d\n",
            //input_buffer[FRAME_LEN - 1], output_buffer[FRAME_LEN - 1], ct->prev_frame_in, ct->prev_frame_out);

    if (ct->cutoff_freq != 0) {
        // Generate the coeffs for the filter based on freq
        generate_coeffs(lpf, ct->cutoff_freq);

        // Use the prev state values for the processing correctly
        in[0] = (float)((input_buffer[0]) * ONEOVERSHORTMAX);
        if (frame_count == 0) {
            // If first frame, initialize to first input value.
            output_buffer[0] = input_buffer[0];
            out[0] = (float)((output_buffer[0]) * ONEOVERSHORTMAX);
        } else {
            // If not, set the first output value to the last processed value of the previous frame.
            float f_prev_frame_out = (float)(ct->prev_frame_out * ONEOVERSHORTMAX);
            float f_prev_frame_in = (float)(ct->prev_frame_in * ONEOVERSHORTMAX);
            out[0] = f_prev_frame_out + (lpf->b * (in[0] - f_prev_frame_out));
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

    // Store the previous frame in-out value stats in context
    ct->prev_frame_in = input_buffer[FRAME_LEN - 1];
    ct->prev_frame_out = output_buffer[FRAME_LEN - 1];

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
