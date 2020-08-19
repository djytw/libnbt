/*  basic.c: basic usage of libnbt, including parsing an NBT file and translate to SNBT
    Not copyrighted, provided to the public domain
    This file is part of the libnbt library
*/

#include <stdio.h>
#include <stdlib.h>
#include "nbt.h"

int main(int argc, char** argv) {

    // Get parameters
    if (argc < 2) {
        printf("Usage: %s <nbtfile>\n", argv[0]);
        return -1;
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

    // Note this is the simplified version without any error handle
    NBT* root = NBT_Parse(data, size);
    if (root) {
        printf("NBT parse OK!\n");

        int bufferlength = 100000;
        char* output = malloc(bufferlength);
        
        NBT_toSNBT(root, output, &bufferlength);
        printf("%s\nLength=%d\n", output, bufferlength);

        free(output);

        // Remember to use NBT_Free after use
        NBT_Free(root);
    } else {
        printf("NBT parse failed!\n");
    }

    free(data);

    return 0;
}