/*  libnbt - A nbt/mca file parser in C
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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum NBT_Tags {
    TAG_End, TAG_Byte, TAG_Short, TAG_Int, TAG_Long, TAG_Float, TAG_Double, TAG_Byte_Array, TAG_String, TAG_List, TAG_Compound, TAG_Int_Array, TAG_Long_Array
} NBT_Tags;

typedef struct NBT {
    struct NBT *next;
    struct NBT *prev;
    enum NBT_Tags type;
    char* key;
    union {
        int64_t value_i;
        double value_d;
        struct {
            void* value;
            int32_t len;
        }value_a;
        struct NBT *child;
    };
} NBT;

#define ERROR_MASK 0xf0000000
#define ERROR_INTERNAL          (ERROR_MASK|0x1) 
#define ERROR_EARLY_EOF         (ERROR_MASK|0x2) 
#define ERROR_LEFTOVER_DATA     (ERROR_MASK|0x3) 
#define ERROR_INVALID_DATA      (ERROR_MASK|0x4)
#define ERROR_BUFFER_OVERFLOW   (ERROR_MASK|0x5)

typedef struct NBT_Error {
    int errid;
    int position;
} NBT_Error;

NBT* NBT_Parse(uint8_t* data, int length);
NBT* NBT_Parse_Opt(uint8_t* data, int length, NBT_Error* err);
void NBT_Free(NBT* root);
int  NBT_toSNBT(NBT* root, char* buff, int bufflen);
int  NBT_toSNBT_Opt(NBT* root, char* buff, int bufflen, int maxlevel, int space, NBT_Error* errid);

#ifdef __cplusplus
}
#endif
