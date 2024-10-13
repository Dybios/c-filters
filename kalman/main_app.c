#include "anc.h"

int main(int argc, char** argv) {

    printf("****** Thank you for using this application ******** \n");

    if (argc < 3)
    {
        printf("Enter command as below: \n");
        printf("<exe> <input_file1> <input_file2> <output_file> <suppression_strength>\n");
        return 0;
    }

    FILE* input1;
    FILE* input2;
    FILE* output;
    input1 = fopen(argv[1], "rb");
    input2 = fopen(argv[2], "rb");
    output = fopen(argv[3], "wb");
    float alpha = atof(argv[4]);
    printf("Cancellation Strength = %f\n", alpha);

    void *context;

    if (input1 == NULL || input2 == NULL || output == NULL) {
           printf("Cannot open file.\n");
           return 1;
    }

    int32_t ret = 0;
    int16_t in1[FRAME_LEN];
    int16_t in2[FRAME_LEN];
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

    while ( (ret =fread(in1, sizeof(short), FRAME_LEN, input1)) >= 1) {
        printf("Processing\n");
        ret = fread(in2, sizeof(short), FRAME_LEN, input2);
        if (ret < 1) {
           printf("Error in control input\n");
           break;
        }
        process_frame(context, in1, in2, out);

        fseek(output, 0, SEEK_END); // Always append out value to end of file
        fwrite(&out, sizeof(short), FRAME_LEN, output);
        count++;

        getVal = get_param(context);
        printf("getVal = %d\n", getVal);
    }

    return 0;
}
