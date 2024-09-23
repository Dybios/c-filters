// Using following header to run LPF demo
#include "iir/lpf/lpf.h"

int main(int argc, char** argv) {

    printf("****** Thank you for using this application ******** \n");

    if (argc < 3)
    {
        printf("Enter command as below: \n");
        printf("<exe> <input_file> <output_file> <cutoff_frequency (in Hz)>\n");
        return 0;
    }

    FILE* input;
    FILE* output;
    input = fopen(argv[1], "rb");
    output = fopen(argv[2], "wb");
    float alpha = atof(argv[3]) / 44100; // Set default sample rate for chip demo wav file: 44.1k (TODO: make it dynamic)
    printf("Cutoff Frequency = %f\n", alpha);

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
    set_param(context, alpha);

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
