// Use define to allow which example to run
#include <string.h>
#include "iir/lpf/lpf.h"
#include "iir/hpf/hpf.h"
#include "iir/bsf/bsf.h"
#include "iir/bpf/bpf.h"

const float ONEOVERSHORTMAX = 3.0517578125e-5f; // 1/32768

int main(int argc, char** argv) {

    printf("****** Thank you for using this application ******** \n");

    if (argc < 5)
    {
        printf("Enter command as below: \n");
        printf("<exe> <input_file> <output_file> <filter_type> <cutoff_frequency (in Hz)>\n");
        printf("\n OR <exe> <input_file> <output_file> <filter_type> <low_cutoff_frequency (in Hz)> <high_cutoff_frequency (in Hz)>\n");
        printf("\n OR (if trying to notch up/down) \n\n <exe> <input_file> <output_file> <filter_type> <notch up/down_frequency (in Hz)>\n");
        return 0;
    }

    FILE* input = fopen(argv[1], "rb");
    FILE* output = fopen(argv[2], "wb");
    int filter_type = atoi(argv[3]); // Filter type: 1 = LPF, 2 = HPF, 3 = BPF, 4 = BSF
    float freq_l, freq_h;
    freq_l = atof(argv[4]);
    if (argv[5] != NULL) {
        freq_h = atof(argv[5]);
        printf("Low Cutoff = %f\n", freq_l);
        printf("High Cutoff = %f\n", freq_h);
    } else {
        freq_h = freq_l;
        printf("Cutoff Frequency = %f\n", freq_l);
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
    int16_t out[FRAME_LEN];

    // Initialize all context's memory for demo application
    int32_t get_lpf_memory_size = get_lpf_mem_size();
    int32_t get_hpf_memory_size = get_hpf_mem_size();
    int32_t get_bpf_memory_size = get_bpf_mem_size();
    int32_t get_bsf_memory_size = get_bsf_mem_size();
    void *lpf_context = (void*) malloc(get_lpf_memory_size);
    void *hpf_context = (void*) malloc(get_hpf_memory_size);
    void *bpf_context = (void*) malloc(get_bpf_memory_size);
    void *bsf_context = (void*) malloc(get_bsf_memory_size);

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
    default:
        printf("Invalid filter type. Please input values between 1 to 4 only.\n");
        break;
    }

    return 0;
}
