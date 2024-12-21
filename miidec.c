#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

uint8_t* decompressMIi(const uint8_t* input, unsigned inputSize, unsigned* outputSize) {
    if (!input || inputSize < 8)
        return NULL;

    // lolwhat? this whole thing is backwards?

    unsigned decompressedSize = *(uint32_t*)(input + inputSize - sizeof(uint32_t)) + inputSize;
    if (outputSize)
        *outputSize = decompressedSize;

    uint8_t* output = (uint8_t*)malloc(decompressedSize);
    if (!output)
        return NULL;

    unsigned offset = inputSize - (*(uint32_t*)(input + inputSize - 8) >> 24);
    unsigned destOffset = decompressedSize;

    while (1) {
        uint8_t header = input[--offset];

        for (unsigned i = 0; i < 8; i++) {
            if ((header & 0x80) == 0) {
                if (offset <= 0 || destOffset <= 0) {
                    free(output);
                    return NULL;
                }
                output[--destOffset] = input[--offset];
            } else {
                if (offset < 2 || destOffset <= 0) {
                    free(output);
                    return NULL;
                }
                uint8_t a = input[--offset];
                uint8_t b = input[--offset];
                int refOffset = (((a & 0xF) << 8) | b) + 2;
                int length = (a >> 4) + 2;

                if (destOffset - length < 0) {
                    free(output);
                    return NULL;
                }

                do {
                    output[destOffset - 1] = output[destOffset + refOffset];
                    destOffset--;
                    length--;
                } while (length >= 0);
            }

            if (offset <= (inputSize - (*(uint32_t*)(input + inputSize - 8) & 0xFFFFFF))) {
                *outputSize = decompressedSize;
                return output;
            }

            header <<= 1;
        }
    }
}

int decompressFile(const char* inputFile, const char* outputFile) {
    FILE* in = fopen(inputFile, "rb");
    if (!in) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    fseek(in, 0, SEEK_END);
    unsigned inputSize = ftell(in);
    rewind(in);

    uint8_t* inputData = (uint8_t*)malloc(inputSize);
    if (!inputData) {
        perror("Failed to allocate memory for input data");
        fclose(in);
        return EXIT_FAILURE;
    }

    fread(inputData, 1, inputSize, in);
    fclose(in);

    unsigned decompressedSize;
    uint8_t* decompressedData = decompressMIi(inputData, inputSize, &decompressedSize);
    free(inputData);

    if (!decompressedData) {
        fprintf(stderr, "Decompression failed\n");
        return EXIT_FAILURE;
    }

    FILE* out = fopen(outputFile, "wb");
    if (!out) {
        perror("Failed to open output file");
        free(decompressedData);
        return EXIT_FAILURE;
    }

    fwrite(decompressedData, 1, decompressedSize, out);
    fclose(out);
    free(decompressedData);

    printf("Decompression successful: %u bytes written to %s\n", decompressedSize, outputFile);
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file> [output_file]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* outputPath;
    if (argc < 3) {
        outputPath = malloc(strlen(argv[1]) + sizeof(".decompressed"));
        sprintf(outputPath, "%s.decompressed", argv[1]);
    }
    else
        outputPath = argv[2];

    int result = decompressFile(argv[1], outputPath);

    if (argc < 3)
        free(outputPath);

    return result;
}
