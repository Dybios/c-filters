// Use define to allow which example to run
#define LPF

#if defined(LPF)
  #include "iir/lpf/lpf.h"
#elif defined(HPF)
  #include "iir/hpf/hpf.h"
#elif defined(BSF)
  #include "iir/bsf/bsf.h"
#endif

int main(int argc, char** argv) {

    printf("****** Thank you for using this application ******** \n");

#ifndef BSF
    if (argc < 3)
    {
        printf("Enter command as below: \n");
        printf("<exe> <input_file> <output_file> <cutoff_frequency (in Hz)>\n");
        return 0;
    }
#else
    if (argc < 4)
    {
        printf("Enter command as below: \n");
        printf("<exe> <input_file> <output_file> <low_cutoff_frequency (in Hz)> <high_cutoff_frequency (in Hz)>\n");
        return 0;
    }
#endif

    FILE* input;
    FILE* output;
    input = fopen(argv[1], "rb");
    output = fopen(argv[2], "wb");
#ifndef BSF
    float freq = atof(argv[3]) / 44100; // Set default sample rate for chip demo wav file: 44.1k (TODO: make it dynamic)
    printf("Cutoff Frequency = %f\n", freq);
#else
    float freq_l = atof(argv[3]) / 44100;
    float freq_h = atof(argv[4]) / 44100;
    if (freq_h == freq_l) {
        printf("NOTE: Notch filter is currently not implemented. No effect will be applied. Please increase the BW of cutoff freqs.\n");
    }
    printf("Low Cutoff = %f\n", freq_l);
    printf("High Cutoff = %f\n", freq_h);
#endif

    void *context;

    if (input == NULL || output == NULL) {
           printf("Cannot open file.\n");
           return 1;
    }

    int32_t ret = 0;
    int16_t in[FRAME_LEN];
    int16_t out[FRAME_LEN];
    int32_t count = 0;
    int32_t getVal = 0;
    int32_t getMemorySize = 0;

    getMemorySize = get_mem_size();
    context = (void*) malloc(getMemorySize);

    printf("Init context ----\n");
    init(context);

    printf("Set param\n");
#ifndef BSF
    set_param(context, freq);
#else
    set_param(context, freq_l, freq_h);
#endif

    while ( (ret =fread(in, sizeof(short), FRAME_LEN, input)) >= 1) {

        if (ret < 1) {
           printf("Error in control input\n");
           break;
        }

        process_sample(context, in, out);

        fseek(output, 0, SEEK_END); // Always append out value to end of file
        fwrite(out, sizeof(short), FRAME_LEN, output);
        count++;

        getVal = get_param(context);
    }

    return 0;
}
