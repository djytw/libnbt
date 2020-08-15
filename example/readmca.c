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

    int regionx, regionz, ret;
    char* str = strrchr(argv[1], '/');
    if (str == NULL) str = argv[1];
    else str++;
    ret = sscanf(str, "r.%d.%d.mca", &regionx, &regionz);
    if (ret != 2) {
        printf("Not standard mca file name, skip chunk position checks.\n");
    } else {
        printf("File: %s: chunk%d,%d ~ chunk%d,%d\n", str, regionx*32, regionz*32, regionx*32+31, regionz*32+31);
    }
    
    uint8_t* data[CHUNKS_IN_REGION];
    uint32_t length[CHUNKS_IN_REGION];
    ret = MCA_ExtractFile(fp, data, length, 0);

    if (ret != 0) {
        printf("Read MCA file failed!\n");
        return -3;
    }

    int i, success = 0, errcount = 0, emptycount = 0;
    NBT_Error error;
    for (i = 0; i < CHUNKS_IN_REGION; i ++) {
        if (data[i] != NULL) {
            NBT* nbt = NBT_Parse_Opt(data[i], length[i], &error);
            if (nbt) {
                NBT* xPos = NBT_GetChild_Deep(nbt, "Level", "xPos", NULL);
                NBT* zPos = NBT_GetChild_Deep(nbt, "Level", "zPos", NULL);
                if (xPos == NULL || zPos == NULL) {
                    printf("Cannot find position data of chunk%d,%d\n", i%32, i/32);
                    errcount++;
                } else {
                    int x = (int)xPos->value_i;
                    int z = (int)zPos->value_i;
                    if (regionx*32 + (i%32) != x || regionz*32 + (i/32) != z) {
                        printf("Chunk position error. Expected %d,%d , get %d,%d \n", regionx*32 + (i%32), regionz*32 + (i/32), x, z);
                        errcount++;
                    } else {
                        success ++;
                    }
                }
                NBT_Free(nbt);
            } else {
                printf("Chunk %d,%d load failed! errid=%x\n", i%32, i/32, error.errid);
                errcount++;
            }
        } else {
            emptycount ++;
        }
    }
    printf("Load finished! %d chunks passed position check, %d chunks are empty, and %d chunks have error\n", success, emptycount, errcount);

    return 0;
}