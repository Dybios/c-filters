#include "kalman.h"
#include <string.h>

#define ONEOVERSHORTMAX 3.0517578125e-5f // 1/32768

typedef struct kalman_t {
   float P;           // State covariance
   float Q;           // Process noise covariance
   float R;           // Measurement noise covariance
   float K;           // Kalman gain
   float model_residual_avg[FRAME_LEN][1]; // Keep residual history across the looping filter
} kalman_t;

typedef struct context_t {
    short *input_buffer1;
    short *input_buffer2;
    short *output_buffer;

    // Hold kalman instance between frames
    kalman_t *kalman;
} context_t;

int32_t get_kalman_mem_size(void) {
    return ((3 * FRAME_LEN * sizeof(short)) + sizeof(kalman_t));
}

void init_kalman(void* context) {
    /* Initialize context to 0*/
    printf("Init Function\n");

    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));

    ct->input_buffer1 = (short*)context;
    memset(ct->input_buffer1, 0, FRAME_LEN*sizeof(short));

    context += FRAME_LEN * sizeof(short);
    ct->input_buffer2 = (short*)context;
    memset(ct->input_buffer2, 0, FRAME_LEN*sizeof(short));

    context += FRAME_LEN * sizeof(short);
    ct->output_buffer = (short*) context;
    memset(ct->output_buffer, 0, FRAME_LEN*sizeof(short));

    context += FRAME_LEN * sizeof(short);
    ct->kalman = (kalman_t *) context;
    memset(ct->kalman, 0, sizeof(kalman_t));

    /* Set default Q value to 0.005 */
    ct->kalman->Q = 0.005;

    printf("Init Success \n");
}

int32_t process_kalman(void *context, int16_t *input_buffer1, int16_t *input_buffer2, int16_t *output_buffer, int32_t frame_count) {
    context_t *ct = (context_t*) context;

    /* Initialize all Kalman filter parameters here. */
    float x_hat[FRAME_LEN][1];     // State vector
    float w_hat[FRAME_LEN][1];     // Measurement vector
    float Y[FRAME_LEN][CTRL_INPUTS];     // measurements without coeffs for control vector

    // Initial state estimate (x_hat) and state covariance (P/psi) by taking average of the first N samples.
    float in1[FRAME_LEN], in2[FRAME_LEN], out[FRAME_LEN];
	// Convert to float
    for (unsigned int i = 0; i < FRAME_LEN; ++i)
    {
        in1[i] = (float)(input_buffer1[i]) * ONEOVERSHORTMAX;
        in2[i] = (float)(input_buffer2[i]) * ONEOVERSHORTMAX;
	}

    // Initial state estimate (x_hat) and state covariance (P/psi) by taking average of the first N ECG complexes.
    float x0[FRAME_LEN][N];
    for (int row = 0; row < FRAME_LEN; row++) {
        for (int col = 0; col < N; col++) {
            x0[row][col] = in1[row + (col * FRAME_LEN)];
        }
    }
    mean((float *)x0, (float *)x_hat, FRAME_LEN, N, 1);

    float P_mat[1][1];
    covariance((float *)x_hat, (float *)P_mat, FRAME_LEN, 1);
    ct->kalman->P = P_mat[0][0];

    float y[FRAME_LEN][1];

    /* Measurement matrix (ŵ) and Measurement covariance (R/Σ) */
    /** EQ4: Coefficients of estimate γ̂ = [Yt * Y]^−1 * Yt * y(i)
     *           where t denotes tranpose.
     *  Subscript -i ommitted for clarity.
     */
    float y_inv[CTRL_INPUTS][CTRL_INPUTS], Y_transpose[CTRL_INPUTS][FRAME_LEN], Y_res[CTRL_INPUTS][CTRL_INPUTS], Y_res2[1][FRAME_LEN], y_coeff_hat[CTRL_INPUTS][1];
    for (int row = 0; row < FRAME_LEN; row++) {
        // Update y
        y[row][0] = in1[row];

        // Update the measurement matrix
        Y[row][0] = in2[row];
    }
    transpose((float *)Y, (float *)Y_transpose, FRAME_LEN, CTRL_INPUTS);
    multiply((float *)Y_transpose, (float *)Y, (float *)Y_res, CTRL_INPUTS, FRAME_LEN, CTRL_INPUTS);
    inverse((float *)Y_res, (float *)y_inv, CTRL_INPUTS);
    for (int rows = 0; rows < CTRL_INPUTS; rows++) {
        for (int cols = 0; cols < FRAME_LEN; cols++) {
            Y_res2[rows][cols] = y_inv[rows][rows] * Y_transpose[rows][cols];
        }
    }
    multiply((float *) Y_res2, (float *)y, (float *)y_coeff_hat, CTRL_INPUTS, FRAME_LEN, 1);
    /** EQ3: Measurement noise vector for ith signal, ŵ(i) = y(i) − ŷ(i)
     *           where, y = ith measured value of ECG complex
     *                  ŷ = ith estimated value of ECG complex
     */
    float y_i_hat[FRAME_LEN][1];
    multiply((float *)Y, (float *)y_coeff_hat, (float *)y_i_hat, FRAME_LEN, CTRL_INPUTS, 1);
    subtract((float *)y, (float *) y_i_hat, (float *)w_hat, FRAME_LEN, 1);

    /* Updating measurement noise covariance R/sigma */
    float R_mat[1][1];
    covariance((float *)w_hat, (float*)R_mat, FRAME_LEN, 1);
    ct->kalman->R = R_mat[0][0];

    /** EQ9: Kalman Gain for (k+1)th reading, K(k + 1) = [Ψ(k) + Λ(k)] / [Σ(k +1) + Ψ(k) + Λ(k)]
     *  The above equation assumes an initial value of K available at k = 0. The first loop of the filter operation
     *  provides the initial values for w_hat, R/Σ and K for k=0. This can then be used as inputs to the next interation
     *  which can also be referred to as the update sequence of the filter.
     *
     *  NOTE: K is considered as a scalar, since P/Ψ, Q/Ψ and R/Σ essentially become scalar matrices. This is an implcit assumption
     *  that process and measurement noise are spatially uncorrelated. More details provided in the original paper.
     */
    ct->kalman->K = (ct->kalman->P + ct->kalman->Q) / (ct->kalman->R + ct->kalman->P + ct->kalman->Q);

    /** EQ7: The next (k+1)th state estimate, x̂(k + 1) = x̂(k) + [K(k + 1) * [y(k + 1) − x̂(k)]]
     *  This updates the state estimate using the noisy input and the Kalman Gain previously calculated.
     */
    float temp_sub[FRAME_LEN][1], temp_mul[FRAME_LEN][1];
    subtract((float *)y, (float *)x_hat, (float *)temp_sub, FRAME_LEN, 1);
    for (int i = 0; i < FRAME_LEN; i++) {
        temp_mul[i][0] = temp_sub[i][0] * ct->kalman->K;
    }
    add((float *) x_hat, (float *)temp_mul, (float *)x_hat, FRAME_LEN, 1);

    /** EQ8: The (k+1)th state estimate covariance, Ψ(k + 1) = [Ψ(k) + Λ(k)] − [K(k + 1) * [Ψ(k) + Λ(k)]]
     */
    ct->kalman->P = ct->kalman->P + ct->kalman->Q - (ct->kalman->K * (ct->kalman->P + ct->kalman->Q));

    /** EQ11: Model residual for (k+1)th heartbeat is defined to be:
     *             ρ(k + 1) = y(k + 1) − E[y(k +1) | y(k), Λ(k), Σ(k)]
     *         or, ρ(k + 1) = y(k + 1) − x̂(k)
     */
    float model_residual[FRAME_LEN][1], residual_mean[FRAME_LEN][1], residual_transpose[1][FRAME_LEN], Q_coeff[1][1];
    subtract((float *)y, (float *)x_hat, (float *) model_residual, FRAME_LEN, 1);
    add((float *)model_residual, (float *)ct->kalman->model_residual_avg, (float *)residual_mean, FRAME_LEN, 1);
    for (int i = 0; i < FRAME_LEN; i++) {
        residual_mean[i][0] = residual_mean[i][0] / N;
    }
    memcpy(ct->kalman->model_residual_avg, residual_mean, FRAME_LEN);
    transpose((float *)residual_mean, (float *)residual_transpose, FRAME_LEN, 1);
    multiply((float *)residual_transpose, (float *)residual_mean, (float *)Q_coeff, 1, FRAME_LEN, 1);

    /** EQ14: The process noise covariance, λ̂ in its scalar form is given as,
     *           λ̂(k) = [(1/T) * ρt(k + 1) * ρ(k + 1)] − ψ(k) − σ(k + 1)  ; if positive,
     *     and   λ̂(k) = 0                                                 ; otherwise.
     */
    ct->kalman->Q = fmax(0, (Q_coeff[0][0] / FRAME_LEN) - ct->kalman->P - ct->kalman->R);

    // Initial state estimate and covariance
    for (int row = 0; row < FRAME_LEN; row++) {
        out[row] = x_hat[row][0];
        output_buffer[row] = (short)(out[row] * 32767);
    }
}

int32_t set_kalman_param(void* context, float value) {
    context_t* ct = (context_t*) context;

    /* Set noise cancellation strength */
    if(value < 0){
        return -1;
    }
    else {
        ct->kalman->Q = value;
        printf("ct->kalman->Q = %lf value =%f \n",ct->kalman->Q, value);
        return 0;
    }
}

int32_t get_kalman_param(void* context) {
    context_t* ct = (context_t*) context;

    /* Retrieves the value of the parameter "alpha" from the context. */
    printf("Q = %f \n", ct->kalman->Q);
    return ct->kalman->Q;
}

void deinit_kalman(void* context) {
    context_t* ct = (context_t*) context;
    memset(ct, 0, sizeof(context_t));
}
