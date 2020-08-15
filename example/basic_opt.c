/*  basic_opt.c: basic usage of libnbt options, including parsing an NBT file and translate to SNBT
    Not copyrighted, provided to the public domain
    This file is part of the libnbt library
*/

#include <stdio.h>
#include <stdlib.h>
#include "nbt.h"

int main(int argc, char** argv) {

    // Get parameters
    int maxlevel = -1;
    int space = -1;
    if (argc < 2) {
        printf("Usage: %s <nbtfile> [maxlevel] [space]\n", argv[0]);
        return -1;
    }
    if (argc > 2) {
        maxlevel = atoi(argv[2]);
    }
    if (argc > 3) {
        space = atoi(argv[3]);
    }
    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Cannot open file %s!\n", argv[1]);
        return -2;
    }
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // Read the input file
    uint8_t* data = malloc(size);
    fread(data, 1, size, fp);
    fclose(fp);

    // Parse nbt, and get any errors
    NBT_Error error;
    NBT* root = NBT_Parse_Opt(data, size, &error);
    if (error.errid == 0) {
        printf("NBT parse OK!\n");

        int bufferlength = 100000;
        char* output = malloc(bufferlength);
        
        int readlen = NBT_toSNBT_Opt(root, output, bufferlength, maxlevel, space, &error);
        printf("%s\nLength=%d\n", output, readlen);
        if (error.errid == ERROR_BUFFER_OVERFLOW) {
            printf("buffer not enough!\n");
        }

        free(output);

        // Remember to use NBT_Free after use
        NBT_Free(root);
    } else {
        printf("NBT parse failed!\n");
    }

    free(data);

    return 0;
}