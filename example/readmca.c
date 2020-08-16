/*  readmca.c: basic usage of libnbt for reading mca files.
    Not copyrighted, provided to the public domain
    This file is part of the libnbt library
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nbt.h"

int main(int argc, char** argv) {

    // Get parameters
    if (argc < 2) {
        printf("Usage: %s <mcafile>\n", argv[0]);
        return -1;
    }
    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Cannot open file %s!\n", argv[1]);
        return -2;
    }

    // Initialize MCA structure with filename. This will detect region location from filename.
    MCA* mca = MCA_Init(argv[1]);
    if (mca->hasPosition) {
        printf("File: chunk%d,%d ~ chunk%d,%d\n", mca->x*32, mca->z*32, mca->x*32+31, mca->z*32+31);
    } else {
        printf("Not standard mca file name, skip chunk position checks.\n");
    }

    // Read raw chunk data from file
    int ret = MCA_ReadRaw_File(fp, mca, 1);
    // // another approach can be done with buffer:
    // fseek(fp, 0, SEEK_END);
    // int size = ftell(fp);
    // fseek(fp, 0, SEEK_SET);
    // uint8_t* buffer = malloc(size);
    // fread(buffer, 1, size, fp);
    // int ret = MCA_ReadRaw(buffer, size, mca, 1);

    if (ret != 0) {
        MCA_Free(mca);
        printf("Read MCA file failed!\n");
        return -3;
    }

    // Parse raw data to NBT structure
    int errcount = MCA_Parse(mca);

    int i;
    int emptycount = -errcount;
    int successcount = 0;
    for (i = 0; i < CHUNKS_IN_REGION; i ++) {
        if (mca->data[i] == NULL) {
            emptycount ++;
            continue;
        }
        NBT* xPos = NBT_GetChild_Deep(mca->data[i], "Level", "xPos", NULL);
        NBT* zPos = NBT_GetChild_Deep(mca->data[i], "Level", "zPos", NULL);
        if (xPos == NULL || zPos == NULL) {
            printf("Cannot find position data of chunk%d,%d\n", i%32, i/32);
            errcount++;
            continue;
        }
        int x = (int)xPos->value_i;
        int z = (int)zPos->value_i;
        if (mca->x*32 + (i%32) != x || mca->z*32 + (i/32) != z) {
            printf("Chunk position error. Expected %d,%d , get %d,%d \n", mca->x*32 + (i%32), mca->z*32 + (i/32), x, z);
            errcount++;
        } else {
            successcount ++;
        }
    }
    printf("Load finished! %d chunks passed position check, %d chunks are empty, and %d chunks have error\n", successcount, emptycount, errcount);

    // remember to release the memory
    MCA_Free(mca);
    // //also free the buffer, if you use it
    // free(buffer);
    
    return 0;
}