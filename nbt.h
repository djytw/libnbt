/*  libnbt - Minecraft NBT/MCA/SNBT file parser in C
    Copyright (C) 2020 djytw
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>. */


#pragma once
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// enumerations for available NBT tags
// refer to https://minecraft.gamepedia.com/NBT_format
typedef enum NBT_Tags {
    TAG_End, TAG_Byte, TAG_Short, TAG_Int, TAG_Long, TAG_Float, TAG_Double, TAG_Byte_Array, TAG_String, TAG_List, TAG_Compound, TAG_Int_Array, TAG_Long_Array
} NBT_Tags;

typedef enum NBT_Compression {
    NBT_Compression_GZIP = 1,
    NBT_Compression_ZLIB = 2,
    NBT_Compression_NONE = 3,
} NBT_Compression;

// Error code
#define ERROR_MASK 0xf0000000
#define ERROR_INTERNAL          (ERROR_MASK|0x1)  // Internal error, maybe a bug?
#define ERROR_EARLY_EOF         (ERROR_MASK|0x2)  // An unexpected EOF was met, maybe the file is incomplete?
#define ERROR_LEFTOVER_DATA     (ERROR_MASK|0x3)  // Extra data after it supposed to end, maybe the file is corrupted
#define ERROR_INVALID_DATA      (ERROR_MASK|0x4)  // Invalid data detected, maybe the file is corrupted
#define ERROR_BUFFER_OVERFLOW   (ERROR_MASK|0x5)  // The buffer you allocated is not enough, please use a larger buffer
#define ERROR_UNZIP_ERROR       (ERROR_MASK|0x6)  // Occurs when the NBT file is compressed, but failed to decompress, the file is corrupted.

// There's always 1024 (32*32) chunks in a region file
#define CHUNKS_IN_REGION 1024

// NBT data structure
typedef struct NBT {

    // NBT tag. see the enum above
    enum NBT_Tags type;

    // NBT tag name. Nullable when no name defined. '\0' ended
    char* key;

    // NBT tag data.
    union {

        // numerical data, used when tag=[TAG_Byte, TAG_Short, TAG_Int, TAG_Long]
        int64_t value_i;

        // used when tag=[TAG_Float, TAG_Double]
        double value_d;

        // Array data, used when tag=[TAG_Byte_Array, TAG_Int_Array, TAG_Long_Array, TAG_String]
        // Note: when using TAG_String, value_a.len equals 1 + string length, because of the ending '\0'
        struct {
            void* value;
            int32_t len;
        }value_a;

        // pointer to child. used when tag=[TAG_Compound, TAG_List]
        struct NBT *child;
    };

    // if this NBT tag is inside a list or compound, these two links are used to denote its siblings
    struct NBT *next;
    struct NBT *prev;
} NBT;

typedef struct MCA {
    // raw nbt data
    uint8_t* rawdata[CHUNKS_IN_REGION];
    // raw nbt data length
    uint32_t size[CHUNKS_IN_REGION];
    // mca chunk modify time
    uint32_t epoch[CHUNKS_IN_REGION];
    // parsed nbt data
    NBT* data[CHUNKS_IN_REGION];
    // if region position is defined
    uint8_t hasPosition;
    int x;
    int z;
} MCA;

typedef struct NBT_Error {
    // Error ID, see above
    int errid;
    // Error position
    int position;
} NBT_Error;

NBT*  NBT_Parse(uint8_t* data, int length);
NBT*  NBT_Parse_Opt(uint8_t* data, int length, NBT_Error* err);
void  NBT_Free(NBT* root);
int   NBT_Pack(NBT* root, uint8_t* buffer, size_t* length);
int   NBT_Pack_Opt(NBT* root, uint8_t* buffer, size_t* length, NBT_Compression compression, NBT_Error* errid);
NBT*  NBT_GetChild(NBT* root, const char* key);
NBT*  NBT_GetChild_Deep(NBT* root, ...);
int   NBT_toSNBT(NBT* root, char* buff, size_t* bufflen);
int   NBT_toSNBT_Opt(NBT* root, char* buff, size_t* bufflen, int maxlevel, int space, NBT_Error* errid);
MCA*  MCA_Init(char* filename);
MCA*  MCA_Init_WithPos(int x, int z);
int   MCA_ReadRaw(uint8_t* data, int length, MCA* mca, int skip_chunk_error);
int   MCA_ReadRaw_File(FILE* fp, MCA* mca, int skip_chunk_error);
int   MCA_WriteRaw_File(FILE* fp, MCA* mca);
int   MCA_ParseAll(MCA* mca);
void  MCA_Free(MCA* mca);

#ifdef __cplusplus
}
#endif
