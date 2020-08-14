#include <stdio.h>
#include <stdlib.h>
#include "nbt.h"

int main(int argc, char** argv) {
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
    
    uint8_t* data = malloc(size);
    fread(data, 1, size, fp);
    NBT* root = NBT_Parse(data, size);
    if (root) {
        printf("OK!\n");
        NBT_Free(root);
    }
    return 0;
}