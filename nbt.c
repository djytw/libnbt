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

#include "nbt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct NBT_Buffer {
    uint8_t* data;
    size_t len;
    size_t pos;
} NBT_Buffer;

#define isValidTag(tag) ((tag)>TAG_End && (tag)<=TAG_Long_Array)
#define bswap16(a) __builtin_bswap16(a)
#define bswap32(a) __builtin_bswap32(a)
#define bswap64(a) __builtin_bswap64(a)

#define ERROR_MASK 0xf0000000
#define ERROR_INTERNAL          ERROR_MASK|0x1 
#define ERROR_EARLY_EOF         ERROR_MASK|0x2 
#define ERROR_LEFTOVER_DATA     ERROR_MASK|0x3 
#define ERROR_INVALID_DATA      ERROR_MASK|0x4

NBT* create_NBT(uint8_t type) {
    NBT* root = malloc(sizeof(NBT));
    memset(root, 0, sizeof(NBT));
    root->type = type;
    return root;
}

NBT_Buffer* init_buffer(uint8_t* data, int length) {
    NBT_Buffer* buffer = malloc(sizeof(NBT_Buffer));
    if (buffer == NULL) {
        return NULL;
    }
    buffer->data = data;
    buffer->len = length;
    buffer->pos = 0;
    return buffer;
}


int getUint8(NBT_Buffer* buffer, uint8_t* result) {
    if (buffer->pos + 1 > buffer->len) {
        return 0;
    }
    *result = buffer->data[buffer->pos];
    ++ buffer->pos;
    return 1;
}

int getUint16(NBT_Buffer* buffer, uint16_t* result) {
    if (buffer->pos + 2 > buffer->len) {
        return 0;
    }
    *result = *(uint16_t*)(buffer->data + buffer->pos);
    buffer->pos += 2;
    *result = bswap16(*result);
    return 2;
}

int getUint32(NBT_Buffer* buffer, uint32_t* result) {
    if (buffer->pos + 4 > buffer->len) {
        return 0;
    }
    *result = *(uint32_t*)(buffer->data + buffer->pos);
    buffer->pos += 4;
    *result = bswap32(*result);
    return 4;
}

int getUint64(NBT_Buffer* buffer, uint64_t* result) {
    if (buffer->pos + 8 > buffer->len) {
        return 0;
    }
    *result = *(uint64_t*)(buffer->data + buffer->pos);
    buffer->pos += 8;
    *result = bswap64(*result);
    return 8;
}

int getFloat(NBT_Buffer* buffer, float* result) {
    if (buffer->pos + 4 > buffer->len) {
        return 0;
    }
    uint32_t ret = *(uint32_t*)(buffer->data + buffer->pos);
    buffer->pos += 4;
    ret = bswap32(ret);
    *result = *(float*)&ret;
    return 4;
}

int getDouble(NBT_Buffer* buffer, double* result) {
    if (buffer->pos + 8 > buffer->len) {
        return 0;
    }
    uint64_t ret = *(uint64_t*)(buffer->data + buffer->pos);
    buffer->pos += 8;
    ret = bswap64(ret);
    *result = *(double*)&ret;
    return 8;
}

int getKey(NBT_Buffer* buffer, char** result) {
    int16_t len;
    if(!getUint16(buffer, &len)) {
        return 0;
    }
    if (len == 0) {
        *result = 0;
        return 2;
    }
    if (buffer->pos + len > buffer->len) {
        return 0;
    }
    *result = malloc(len + 1);
    memcpy(*result, buffer->data + buffer->pos, len);
    (*result)[len] = 0;
    buffer->pos += len;
    return 2 + len;
}

int parse_value(NBT* saveto, NBT_Buffer* buffer, uint8_t skipkey) {
    
    if (saveto == NULL || buffer == NULL || buffer->data == NULL) {
        return ERROR_INTERNAL;
    }

    uint8_t type = saveto->type;
    if (type == TAG_End) {
        if (!getUint8(buffer, &type)) {
            return ERROR_EARLY_EOF;
        }
        if (!isValidTag(type)) {
            return ERROR_INVALID_DATA;
        }
        saveto->type = type;
    }

    if (!skipkey) {
        char* key;
        if (!getKey(buffer, &key)) {
            return ERROR_EARLY_EOF;
        }
        saveto->key = key;
    }

    switch (type) {
        case TAG_Byte: {
            uint8_t value;
            if (!getUint8(buffer, &value)) {
                return ERROR_EARLY_EOF;
            }
            saveto->value_i = value;
            break;
        }
        case TAG_Short: {
            uint16_t value;
            if (!getUint16(buffer, &value)) {
                return ERROR_EARLY_EOF;
            }
            saveto->value_i = value;
            break;
        }
        case TAG_Int: {
            uint32_t value;
            if (!getUint32(buffer, &value)) {
                return ERROR_EARLY_EOF;
            }
            saveto->value_i = value;
            break;
        }
        case TAG_Long: {
            uint64_t value;
            if (!getUint64(buffer, &value)) {
                return ERROR_EARLY_EOF;
            }
            saveto->value_i = value;
            break;
        }
        case TAG_Float: {
            float value;
            if (!getFloat(buffer, &value)) {
                return ERROR_EARLY_EOF;
            }
            saveto->value_d = value;
            break;
        }
        case TAG_Double: {
            double value;
            if (!getDouble(buffer, &value)) {
                return ERROR_EARLY_EOF;
            }
            saveto->value_d = value;
            break;
        }
        case TAG_Byte_Array: {
            uint32_t len;
            if (!getUint32(buffer, &len)) {
                return ERROR_EARLY_EOF;
            }
            saveto->value_a.value = malloc(len);
            saveto->value_a.len = len;
            if (buffer->pos + len > buffer->len) {
                return ERROR_EARLY_EOF;
            }
            memcpy(saveto->value_a.value, buffer->data + buffer->pos, len);
            buffer->pos += len;
            break;
        }
        case TAG_String: {
            uint16_t len;
            if (!getUint16(buffer, &len)) {
                return ERROR_EARLY_EOF;
            }
            saveto->value_a.value = malloc(len + 1);
            saveto->value_a.len = len + 1;
            if (buffer->pos + len > buffer->len) {
                return ERROR_EARLY_EOF;
            }
            memcpy(saveto->value_a.value, buffer->data + buffer->pos, len);
            ((char*)saveto->value_a.value)[len] = 0;
            buffer->pos += len;
            break;
        }
        case TAG_List: {
            uint8_t listtype;
            if (!getUint8(buffer, &listtype)) {
                return ERROR_EARLY_EOF;
            }
            uint32_t len;
            if (!getUint32(buffer, &len)) {
                return ERROR_EARLY_EOF;
            }
            if (listtype == TAG_End && len != 0) {
                return ERROR_INVALID_DATA;
            }
            int i;
            NBT* last = NULL;
            for (i = 0; i < len; i ++) {
                NBT* child = create_NBT(listtype);
                int ret = parse_value(child, buffer, 1);
                if (ret) {
                    return ret;
                }
                if (i == 0) {
                    last = child;
                    saveto->child = child;
                } else {
                    last->next = child;
                    child->prev = last;
                    last = child;
                }
            }
            break;
        }
        case TAG_Compound: {
            NBT* last = NULL;
            while (1) {
                uint8_t listtype;
                if (!getUint8(buffer, &listtype)) {
                    return ERROR_EARLY_EOF;
                }
                if (listtype == 0) {
                    break;
                }
                NBT* child = create_NBT(listtype);
                int ret = parse_value(child, buffer, 0);
                if (ret) {
                    return ret;
                }
                if (last == NULL) {
                    saveto->child = child;
                    last = child;
                } else {
                    last->next = child;
                    child->prev = last;
                    last = child;
                }
            }
            break;
        }
        case TAG_Int_Array: {
            uint32_t len;
            if (!getUint32(buffer, &len)) {
                return ERROR_EARLY_EOF;
            }
            len *= 4;
            saveto->value_a.value = malloc(len);
            saveto->value_a.len = len;
            if (buffer->pos + len > buffer->len) {
                return ERROR_EARLY_EOF;
            }
            memcpy(saveto->value_a.value, buffer->data + buffer->pos, len);
            buffer->pos += len;
            int i;
            uint32_t* value = (uint32_t*)saveto->value_a.value;
            for (i = 0; i < len/4; i ++) {
                value[i] = bswap32(value[i]);
            }
            break;
        }
        case TAG_Long_Array: {
            uint32_t len;
            if (!getUint32(buffer, &len)) {
                return ERROR_EARLY_EOF;
            }
            len *= 8;
            saveto->value_a.value = malloc(len);
            saveto->value_a.len = len;
            if (buffer->pos + len > buffer->len) {
                return ERROR_EARLY_EOF;
            }
            memcpy(saveto->value_a.value, buffer->data + buffer->pos, len);
            buffer->pos += len;
            int i;
            uint64_t* value = (uint64_t*)saveto->value_a.value;
            for (i = 0; i < len/8; i ++) {
                value[i] = bswap64(value[i]);
            }
            break;
        }
    }
    return 0;
}

void fill_err(NBT_Error* err, int errid, int position) {
    if (err == NULL) {
        return;
    }
    err->errid = errid;
    err->position = position;
}

NBT* NBT_Parse_Opt(uint8_t* data, int length, NBT_Error* errid) {
    NBT_Buffer* buffer = init_buffer(data, length);
    NBT* root = create_NBT(TAG_End);
    int ret = parse_value(root, buffer, 0);
    if (ret != 0) {
        fill_err(errid, ret, buffer->pos);
        NBT_Free(root);
        return NULL;
    } else {
        if (buffer->pos != buffer->len) {
            fill_err(errid, ERROR_LEFTOVER_DATA, buffer->pos);
        } else {
            fill_err(errid, 0, buffer->pos);
        }
        return root;
    }
}

NBT* NBT_Parse(uint8_t* data, int length) {
    return NBT_Parse_Opt(data, length, NULL);
}

void NBT_Free(NBT* root) {
    if (root->key != NULL) {
        free(root->key);
    }
    switch (root->type) {
        case TAG_Byte_Array:
        case TAG_Long_Array:
        case TAG_Int_Array:
        if (root->value_a.value != NULL) {
            free(root->value_a.value);
        }
        break;

        case TAG_List:
        case TAG_Compound:
        if (root->child != NULL) {
            NBT_Free(root->child);
        }
    }
    if (root->prev) {
        root->prev->next = NULL;
        NBT_Free(root->prev);
        root->prev = NULL;
    }
    if (root->next) {
        root->next->prev = NULL;
        NBT_Free(root->next);
        root->next = NULL;
    }
    free(root);
}