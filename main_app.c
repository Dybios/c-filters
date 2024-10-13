// Use define to allow which example to run
#include <string.h>
#include "iir/lpf/lpf.h"
#include "iir/hpf/hpf.h"
#include "iir/bsf/bsf.h"
#include "iir/bpf/bpf.h"
#include "kalman/kalman.h"

const float ONEOVERSHORTMAX = 3.0517578125e-5f; // 1/32768

int main(int argc, char** argv) {

    printf("****** Thank you for using this application ******** \n\n\n");

    if (argc < 5)
    {
        printf("Enter command in either of the following formats with appropriate filter type.\n");
        printf("NOTE: Filter types available --> 1 = Lowpass, 2 = Highpass, 3 = Bandpass, 4 = Bandstop, 5 = Kalman\n\n");
        printf("<exe> <filter_type> <input_file> <output_file> <cutoff_frequency (in Hz)>\n");
        printf("\nOR, if using band pass/stop filters\n\n<exe> <filter_type> <input_file> <output_file> <low_cutoff_frequency (in Hz)> <high_cutoff_frequency (in Hz)>\n");
        printf("\nOR, if using band pass/stop notched filters\n\n<exe> <filter_type> <input_file> <output_file> <notch_frequency (in Hz)>\n\n");
        printf("\nOR, if using kalman filter \n\n<exe> <filter_type> <input_file1> <input_file2> <output_file> <Q (between 0-1; default 0.005)>\n\n");
        return 0;
    }

    int filter_type = atoi(argv[1]); // Filter type: 1 = LPF, 2 = HPF, 3 = BPF, 4 = BSF, 5 = KF
    float freq_l, freq_h, Q;
    FILE* input = fopen(argv[2], "rb");
    FILE* input2, *output;
    if (filter_type != 5) {
        output = fopen(argv[3], "wb");

        freq_l = atof(argv[4]);
        if (argv[5] != NULL) {
            freq_h = atof(argv[5]);
            printf("Low Cutoff = %f\n", freq_l);
            printf("High Cutoff = %f\n", freq_h);
        } else {
            freq_h = freq_l;
            printf("Cutoff Frequency = %f\n", freq_l);
        }
    } else {
        // This is kalman filter
        input2 = fopen(argv[3], "rb");
        output = fopen(argv[4], "wb");
        if (argv[5] != NULL) {
            Q = atof(argv[5]);
        } else {
            Q = 0.001;
        }
        if (input2 == NULL) {
            printf("Cannot open control file.\n");
            return 1;
        }
    }

    if (input == NULL || output == NULL) {
           printf("Cannot open file.\n");
           return 1;
    }

    // copy the wav header to output files
    short out_header[44];
    fseek(input, 0, SEEK_SET);
    fread(out_header, 44, 1, input);
    fseek(output, 0, SEEK_SET);
    fwrite(out_header, 44, 1, output);

    int32_t ret = 0;
    int32_t frame_count = 0;
    int32_t getVal = 0;

    int16_t in[FRAME_LEN];
    int16_t in2[FRAME_LEN];
    int16_t out[FRAME_LEN];

    // Initialize all context's memory for demo application
    int32_t get_lpf_memory_size = get_lpf_mem_size();
    int32_t get_hpf_memory_size = get_hpf_mem_size();
    int32_t get_bpf_memory_size = get_bpf_mem_size();
    int32_t get_bsf_memory_size = get_bsf_mem_size();
    int32_t get_kalman_memory_size = get_kalman_mem_size();
    void *lpf_context = (void*) malloc(get_lpf_memory_size);
    void *hpf_context = (void*) malloc(get_hpf_memory_size);
    void *bpf_context = (void*) malloc(get_bpf_memory_size);
    void *bsf_context = (void*) malloc(get_bsf_memory_size);
    void *kalman_context = (void*) malloc(get_kalman_memory_size);

    // Process filter based on the input filter type
    switch (filter_type) {
    case 1:
        printf("Init LPF param\n");
        init_lpf(lpf_context);

        printf("Set LPF param\n");
        set_lpf_param(lpf_context, freq_l);

        while ((ret = fread(in, sizeof(short), FRAME_LEN, input)) >= 1) {
            if (ret < 1) {
                printf("Error in control input\n");
                break;
            }

            // LPF
            process_lpf(lpf_context, in, out, frame_count);
            fseek(output, 0, SEEK_END); // Always append out value to end of file
            fwrite(out, sizeof(short), FRAME_LEN, output);
            frame_count++;
        }
        break;
    case 2:
        printf("Init HPF param\n");
        init_hpf(hpf_context);

        printf("Set HPF param\n");
        set_hpf_param(hpf_context, freq_l);

        while ((ret = fread(in, sizeof(short), FRAME_LEN, input)) >= 1) {
            if (ret < 1) {
                printf("Error in control input\n");
                break;
            }
            // HPF
            process_hpf(hpf_context, in, out, frame_count);
            fseek(output, 0, SEEK_END); // Always append out value to end of file
            fwrite(out, sizeof(short), FRAME_LEN, output);
            frame_count++;
        }
        break;
    case 3:
        printf("Init BPF param\n");
        init_bpf(bpf_context);

        printf("Set BPF param\n");
        set_bpf_param(bpf_context, freq_l, freq_h);

        while ((ret = fread(in, sizeof(short), FRAME_LEN, input)) >= 1) {
            if (ret < 1) {
                printf("Error in control input\n");
                break;
            }
            // BPF
            process_bpf(bpf_context, in, out, frame_count);
            fseek(output, 0, SEEK_END); // Always append out value to end of file
            fwrite(out, sizeof(short), FRAME_LEN, output);
            frame_count++;
        }
        break;
    case 4:
        printf("Init BSF param\n");
        init_bsf(bsf_context);

        printf("Set BSF param\n");
        set_bsf_param(bsf_context, freq_l, freq_h);

        while ((ret = fread(in, sizeof(short), FRAME_LEN, input)) >= 1) {
            if (ret < 1) {
                printf("Error in control input\n");
                break;
            }
            // BSF
            process_bsf(bsf_context, in, out, frame_count);
            fseek(output, 0, SEEK_END); // Always append out value to end of file
            fwrite(out, sizeof(short), FRAME_LEN, output);
            frame_count++;
        }
        break;
    case 5:
        printf("Init kalman param\n");
        init_kalman(kalman_context);

        printf("Set kalman param\n");
        set_kalman_param(kalman_context, Q);

        while ((ret = fread(in, sizeof(short), FRAME_LEN, input)) >= 1
                && (ret = fread(in2, sizeof(short), FRAME_LEN, input2)) >= 1) {

            if (ret < 1) {
                printf("Error in control input\n");
                break;
            }
            // KF
            process_kalman(kalman_context, in, in2, out, frame_count);
            fseek(output, 0, SEEK_END); // Always append out value to end of file
            fwrite(out, sizeof(short), FRAME_LEN, output);
            frame_count++;
        }
        break;
    default:
        printf("Invalid filter type. Please input values between 1 to 4 only.\n");
        break;
    }

    return 0;
}
