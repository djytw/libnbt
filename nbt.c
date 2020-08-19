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

#include "nbt.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

typedef struct NBT_Buffer {
    uint8_t* data;
    size_t len;
    size_t pos;
} NBT_Buffer;

#define isValidTag(tag) ((tag)>TAG_End && (tag)<=TAG_Long_Array)

#ifdef _MSC_VER
#include <stdlib.h>
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)
#else
#include <byteswap.h>
#endif

#define BUFFER_SPRINTF(buffer, str...) {                        \
    char* buf = (char*)&(buffer)->data[(buffer)->pos];          \
    int maxlen = (buffer)->len - (buffer)->pos;                 \
    int writelen = snprintf(buf, maxlen, str);                  \
    (buffer)->pos += writelen;                                  \
    maxlen -= writelen;                                         \
    if ((writelen) == 0) {                                      \
        return ERROR_INTERNAL;                                  \
    }                                                           \
    if ((maxlen) <= 0) {                                        \
        return ERROR_BUFFER_OVERFLOW;                           \
    }                                                           \
}

NBT* create_NBT(uint8_t type);
NBT_Buffer* init_buffer(uint8_t* data, int length);
int getUint8(NBT_Buffer* buffer, uint8_t* result);
int getUint16(NBT_Buffer* buffer, uint16_t* result);
int getUint32(NBT_Buffer* buffer, uint32_t* result);
int getUint64(NBT_Buffer* buffer, uint64_t* result);
int writeUint8(NBT_Buffer* buffer, uint8_t value);
int writeUint16(NBT_Buffer* buffer, uint16_t value);
int writeUint32(NBT_Buffer* buffer, uint32_t value);
int writeUint64(NBT_Buffer* buffer, uint64_t value);
int getFloat(NBT_Buffer* buffer, float* result);
int getDouble(NBT_Buffer* buffer, double* result);
int writeFloat(NBT_Buffer* buffer, float value);
int writeDouble(NBT_Buffer* buffer, double value);
int getKey(NBT_Buffer* buffer, char** result);
int parse_value(NBT* saveto, NBT_Buffer* buffer, uint8_t skipkey);
int snbt_write_space(NBT_Buffer* buffer, int spacecount);
int snbt_write_key(NBT_Buffer* buffer, char* key);
int snbt_write_number(NBT_Buffer* buffer, uint64_t value, char* key, int type);
int snbt_write_point(NBT_Buffer* buffer, double value, char* key, int type);
int snbt_write_array(NBT_Buffer* buffer, void* value, int length, char* key, int type);
int snbt_write_string(NBT_Buffer* buffer, char* value, int length, char* key);
int snbt_write_compound(NBT_Buffer* buffer, NBT* root, int level, int space, int curlevel, int isarray);
int snbt_write_nbt(NBT_Buffer* buffer, NBT* root, int level, int space, int curlevel);
int nbt_write_nbt(NBT_Buffer* buffer, NBT* root, int writekey);
void fill_err(NBT_Error* err, int errid, int position);
int decompress_gzip(uint8_t* dest, size_t* destsize, uint8_t* src, size_t srcsize);

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

int writeUint8(NBT_Buffer* buffer, uint8_t value) {
    if (buffer->pos + 1 > buffer->len) {
        return 0;
    }
    buffer->data[buffer->pos] = value;
    ++ buffer->pos;
    return 1;
}

int getUint16(NBT_Buffer* buffer, uint16_t* result) {
    if (buffer->pos + 2 > buffer->len) {
        return 0;
    }
    *result = *(uint16_t*)(buffer->data + buffer->pos);
    buffer->pos += 2;
    *result = bswap_16(*result);
    return 2;
}

int writeUint16(NBT_Buffer* buffer, uint16_t value) {
    if (buffer->pos + 2 > buffer->len) {
        return 0;
    }
    *(uint16_t*)(buffer->data + buffer->pos) = bswap_16(value);
    buffer->pos += 2;
    return 2;
}

int getUint32(NBT_Buffer* buffer, uint32_t* result) {
    if (buffer->pos + 4 > buffer->len) {
        return 0;
    }
    *result = *(uint32_t*)(buffer->data + buffer->pos);
    buffer->pos += 4;
    *result = bswap_32(*result);
    return 4;
}

int writeUint32(NBT_Buffer* buffer, uint32_t value) {
    if (buffer->pos + 4 > buffer->len) {
        return 0;
    }
    *(uint32_t*)(buffer->data + buffer->pos) = bswap_32(value);
    buffer->pos += 4;
    return 4;
}

int getUint64(NBT_Buffer* buffer, uint64_t* result) {
    if (buffer->pos + 8 > buffer->len) {
        return 0;
    }
    *result = *(uint64_t*)(buffer->data + buffer->pos);
    buffer->pos += 8;
    *result = bswap_64(*result);
    return 8;
}

int writeUint64(NBT_Buffer* buffer, uint64_t value) {
    if (buffer->pos + 8 > buffer->len) {
        return 0;
    }
    *(uint64_t*)(buffer->data + buffer->pos) = bswap_64(value);
    buffer->pos += 8;
    return 8;
}

int getFloat(NBT_Buffer* buffer, float* result) {
    if (buffer->pos + 4 > buffer->len) {
        return 0;
    }
    uint32_t ret = *(uint32_t*)(buffer->data + buffer->pos);
    buffer->pos += 4;
    ret = bswap_32(ret);
    *result = *(float*)&ret;
    return 4;
}

int writeFloat(NBT_Buffer* buffer, float value) {
    if (buffer->pos + 4 > buffer->len) {
        return 0;
    }
    *(uint32_t*)(buffer->data + buffer->pos) = bswap_32(*(uint32_t*)&value);
    buffer->pos += 4;
    return 4;
}

int getDouble(NBT_Buffer* buffer, double* result) {
    if (buffer->pos + 8 > buffer->len) {
        return 0;
    }
    uint64_t ret = *(uint64_t*)(buffer->data + buffer->pos);
    buffer->pos += 8;
    ret = bswap_64(ret);
    *result = *(double*)&ret;
    return 8;
}

int writeDouble(NBT_Buffer* buffer, double value) {
    if (buffer->pos + 8 > buffer->len) {
        return 0;
    }
    *(uint64_t*)(buffer->data + buffer->pos) = bswap_64(*(uint64_t*)&value);
    buffer->pos += 8;
    return 8;
}

int getKey(NBT_Buffer* buffer, char** result) {
    uint16_t len;
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
            saveto->value_a.len = len/4;
            if (buffer->pos + len > buffer->len) {
                return ERROR_EARLY_EOF;
            }
            memcpy(saveto->value_a.value, buffer->data + buffer->pos, len);
            buffer->pos += len;
            int i;
            uint32_t* value = (uint32_t*)saveto->value_a.value;
            for (i = 0; i < len/4; i ++) {
                value[i] = bswap_32(value[i]);
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
            saveto->value_a.len = len/8;
            if (buffer->pos + len > buffer->len) {
                return ERROR_EARLY_EOF;
            }
            memcpy(saveto->value_a.value, buffer->data + buffer->pos, len);
            buffer->pos += len;
            int i;
            uint64_t* value = (uint64_t*)saveto->value_a.value;
            for (i = 0; i < len/8; i ++) {
                value[i] = bswap_64(value[i]);
            }
            break;
        }
        default:
            return ERROR_INTERNAL;
    }
    return 0;
}

int snbt_write_space(NBT_Buffer* buffer, int spacecount) {
    if (spacecount < 0) {
        return 0;
    }
    char* buf = (char*)&buffer->data[buffer->pos];
    int maxlen = buffer->len - buffer->pos;
    if (maxlen <= spacecount) {
        return ERROR_BUFFER_OVERFLOW;
    }
    int i;
    for (i = 0; i < spacecount; i++) {
        buf[i] = ' ';
    }
    buf[i] = 0;
    buffer->pos += spacecount;
    return 0;
}

int snbt_write_key(NBT_Buffer* buffer, char* key) {
    if (key && key[0]) {
        BUFFER_SPRINTF(buffer, "%s:", key);
    }
    return 0;
}

int snbt_write_number(NBT_Buffer* buffer, uint64_t value, char* key, int type) {

    int ret = snbt_write_key(buffer, key);
    if (ret) {
        return ret;
    }
    
    switch(type) {
        case TAG_Byte: BUFFER_SPRINTF(buffer, "%db,", (int8_t)value); break;
        case TAG_Short: BUFFER_SPRINTF(buffer, "%ds,", (int16_t)value); break;
        case TAG_Int: BUFFER_SPRINTF(buffer, "%d,", (int32_t)value); break;
        case TAG_Long: BUFFER_SPRINTF(buffer, "%ldl,", (int64_t)value); break;
        default: return ERROR_INTERNAL;
    }
    return 0;
}

int snbt_write_point(NBT_Buffer* buffer, double value, char* key, int type) {

    int ret = snbt_write_key(buffer, key);
    if (ret) {
        return ret;
    }

    switch(type) {
        case TAG_Float: BUFFER_SPRINTF(buffer, "%ff,", (float)value); break;
        case TAG_Double: BUFFER_SPRINTF(buffer, "%lfd,", (double)value); break;
        default: return ERROR_INTERNAL;
    }
    return 0;
}

int snbt_write_array(NBT_Buffer* buffer, void* value, int length, char* key, int type) {
    int ret = snbt_write_key(buffer, key);
    if (ret) {
        return ret;
    }

    switch(type) {
        case TAG_Byte_Array: BUFFER_SPRINTF(buffer, "[B;"); break;
        case TAG_Int_Array: BUFFER_SPRINTF(buffer, "[I;"); break;
        case TAG_Long_Array: BUFFER_SPRINTF(buffer, "[L;"); break;
        default: return ERROR_INTERNAL;
    }

    int i;
    for (i = 0; i < length; i ++) {
        switch(type) {
            case TAG_Byte_Array: BUFFER_SPRINTF(buffer, "%db,", ((int8_t*)value)[i]); break;
            case TAG_Int_Array: BUFFER_SPRINTF(buffer, "%d,", ((int32_t*)value)[i]); break;
            case TAG_Long_Array: BUFFER_SPRINTF(buffer, "%ldl,", ((int64_t*)value)[i]); break;
            default: return ERROR_INTERNAL;
        }
    }

    int maxlen = buffer->len - buffer->pos;
    if (maxlen <= 3) {
        return ERROR_BUFFER_OVERFLOW;
    }
    if (i > 0) {
        buffer->pos --;
    }
    buffer->data[buffer->pos++] = ']';
    buffer->data[buffer->pos++] = ',';
    buffer->data[buffer->pos] = 0;
    return ret;
}

int snbt_write_string(NBT_Buffer* buffer, char* value, int length, char* key) {
    int ret = snbt_write_key(buffer, key);
    if (ret) {
        return ret;
    }

    char* buf = (char*)&buffer->data[buffer->pos];
    int maxlen = buffer->len - buffer->pos;
    int writelen = 0;

    int i;
    for (i = 0; i < length - 1; i ++) {
        if (writelen + 2 >= maxlen) {
            return ERROR_BUFFER_OVERFLOW;
        }
        if (value[i] == '"') {
            buf[writelen ++] = '\\';
        }
        buf[writelen ++] = value[i];
    }
    if (writelen + 2 >= maxlen) {
        return ERROR_BUFFER_OVERFLOW;
    }
    buf[writelen ++] = ',';
    buf[writelen] = 0;
    buffer->pos += writelen;
    return 0;
}

int snbt_write_compound(NBT_Buffer* buffer, NBT* root, int level, int space, int curlevel, int isarray) {
    int ret = snbt_write_space(buffer, space * curlevel);
    if (ret) {
        return ret;
    }

    ret = snbt_write_key(buffer, root->key);
    if (ret) {
        return ret;
    }

    if (isarray) {
        BUFFER_SPRINTF(buffer, "[");
    } else {
        BUFFER_SPRINTF(buffer, "{");
    }

    if (level <= curlevel && level >= 0) {
        BUFFER_SPRINTF(buffer, "...");
    } else {
        if (space >= 0) {
            BUFFER_SPRINTF(buffer, "\n");
        }
        NBT* child = root->child;
        while(child != NULL) {
            ret = snbt_write_nbt(buffer, child, level, space, curlevel + 1);
            if (ret) {
                return ret;
            }
            if (child->next == NULL) {
                buffer->pos --;
                buffer->data[buffer->pos] = 0;
            }
            if (space >= 0) {
                BUFFER_SPRINTF(buffer, "\n");
            }
            child = child->next;
        }
        ret = snbt_write_space(buffer, space * curlevel);
        if (ret) {
            return ret;
        }
    }

    if (isarray) {
        BUFFER_SPRINTF(buffer, "],");
    } else {
        BUFFER_SPRINTF(buffer, "},");
    }
    return 0;
}

int snbt_write_nbt(NBT_Buffer* buffer, NBT* root, int level, int space, int curlevel) {
    int ret;
    switch(root->type) {
        case TAG_Byte:
        case TAG_Short:
        case TAG_Int:
        case TAG_Long:
        ret = snbt_write_space(buffer, space * curlevel);
        if (ret) return ret;
        ret = snbt_write_number(buffer, root->value_i, root->key, root->type);
        if (ret) return ret;
        return 0;

        case TAG_Float:
        case TAG_Double:
        ret = snbt_write_space(buffer, space * curlevel);
        if (ret) return ret;
        ret = snbt_write_point(buffer, root->value_d, root->key, root->type);
        if (ret) return ret;
        return 0;

        case TAG_Byte_Array:
        case TAG_Int_Array:
        case TAG_Long_Array:
        ret = snbt_write_space(buffer, space * curlevel);
        if (ret) return ret;
        ret = snbt_write_array(buffer, root->value_a.value, root->value_a.len, root->key, root->type);
        if (ret) return ret;
        return 0;

        case TAG_String:
        ret = snbt_write_space(buffer, space * curlevel);
        if (ret) return ret;
        ret = snbt_write_string(buffer, root->value_a.value, root->value_a.len, root->key);
        if (ret) return ret;
        return 0;

        case TAG_List:
        case TAG_Compound:
        ret = snbt_write_compound(buffer, root, level, space, curlevel, root->type == TAG_List);
        if (ret) return ret;
        return 0;

        default:
        return ERROR_INTERNAL;
    }
}

void fill_err(NBT_Error* err, int errid, int position) {
    if (err == NULL) {
        return;
    }
    err->errid = errid;
    err->position = position;
}

int decompress(uint8_t* dest, size_t* destsize, uint8_t* src, size_t srcsize) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = src;
    strm.avail_in = srcsize;
    strm.next_out = dest;
    strm.avail_out = *destsize;
    if (inflateInit2 (&strm, 15 | 32) < 0){
        return -1; 
    }   
    if (inflate (&strm, Z_NO_FLUSH) < 0){
        return -1; 
    }   
    inflateEnd (&strm);
    *destsize -= strm.avail_out;
    return Z_OK;
}

int NBT_toSNBT_Opt(NBT* root, char* buff, int* bufflen, int maxlevel, int space, NBT_Error* errid) {
    NBT_Buffer *buffer = init_buffer((uint8_t*)buff, *bufflen);
    int ret;
    ret = snbt_write_nbt(buffer, root, maxlevel, space, 0);
    fill_err(errid, ret, buffer->pos);
    buffer->data[buffer->pos - 1] = 0;
    *bufflen = buffer->pos;
    return ret;
}

int NBT_toSNBT(NBT* root, char* buff, int* bufflen) {
    return NBT_toSNBT_Opt(root, buff, bufflen, -1, -1, NULL);
}

NBT* NBT_GetChild(NBT* root, const char* key) {
    if (root == NULL || root->type != TAG_Compound || root->child == NULL) {
        return NULL;
    }
    NBT* child = root->child;
    while(child) {
        if (!strcmp(child->key, key)) {
            return child;
        }
        child = child->next;
    }
    return NULL;
}

NBT* NBT_GetChild_Deep(NBT* root, ...) {
    va_list va;
    va_start(va, root);
    char* temp;
    NBT* current = root;
    while ((temp = va_arg(va, char*)) != NULL) {
        current = NBT_GetChild(current, temp);
        if (current == NULL) {
            return NULL;
        }
    }
    return current;
}

NBT* NBT_Parse_Opt(uint8_t* data, int length, NBT_Error* errid) {

    NBT_Buffer* buffer;

    if (length > 1 && data[0] == 0x1f && data[1] == 0x8b) {
        // file is gzip
        size_t size = 1 << 20;
        uint8_t* undata = malloc(size);
        int ret = decompress(undata, &size, data, length);
        if (ret != Z_OK) {
            fill_err(errid, ERROR_UNZIP_ERROR, 0);
            free(undata);
            return NULL;
        }
        buffer = init_buffer(undata, size);
    } else if (data[0] == 0x78) {
        // file is zlib
        size_t size = 1 << 20;
        uint8_t* undata = malloc(size);
        int ret = uncompress(undata, &size, data, length);
        if (ret != Z_OK) {
            fill_err(errid, ERROR_UNZIP_ERROR, 0);
            free(undata);
            return NULL;
        }
        buffer = init_buffer(undata, size);
    } else {
        buffer = init_buffer(data, length);
    }

    NBT* root = create_NBT(TAG_End);
    int ret = parse_value(root, buffer, 0);
    if (buffer->data != data) {
        free(buffer->data);
    }

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

        default: break;
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

int nbt_write_key(NBT_Buffer* buffer, char* key, int type) {
    int ret = 0;
    ret = writeUint8(buffer, type);
    if (!ret) {
        return ERROR_BUFFER_OVERFLOW;
    }
    if (key && key[0]) {
        int len = strlen(key);
        ret = writeUint16(buffer, len);
        if (!ret) {
            return ERROR_BUFFER_OVERFLOW;
        }
        int i;
        for (i = 0; i < len; i ++) {
            ret = writeUint8(buffer, key[i]);
            if (!ret) {
                return ERROR_BUFFER_OVERFLOW;
            }
        }
    } else {
        ret = writeUint16(buffer, 0);
        if (!ret) {
            return ERROR_BUFFER_OVERFLOW;
        }
    }
    return 0;
}

int nbt_write_number(NBT_Buffer* buffer, uint64_t value, char* key, int type) {
    int ret;
    
    switch(type) {
        case TAG_Byte: ret = writeUint8(buffer, value); break;
        case TAG_Short: ret = writeUint16(buffer, value); break;
        case TAG_Int: ret = writeUint32(buffer, value); break;
        case TAG_Long: ret = writeUint64(buffer, value); break;
        default: return ERROR_INTERNAL;
    }
    if (!ret) {
        return ERROR_BUFFER_OVERFLOW;
    }
    return 0;
}

int nbt_write_point(NBT_Buffer* buffer, double value, char* key, int type) {
    int ret;
    switch(type) {
        case TAG_Float: ret = writeFloat(buffer, value); break;
        case TAG_Double: ret = writeDouble(buffer, value); break;
        default: return ERROR_INTERNAL;
    }
    if (!ret) {
        return ERROR_BUFFER_OVERFLOW;
    }
    return 0;
}

int nbt_write_array(NBT_Buffer* buffer, void* value, int32_t len, char* key, int type) {
    int ret = writeUint32(buffer, len);
    if (!ret) {
        return ERROR_BUFFER_OVERFLOW;
    }
    switch(type) {
        case TAG_Byte_Array: {
            int i;
            for (i = 0; i < len; i ++) {
                ret = writeUint8(buffer, ((uint8_t*)value)[i]);
                if (!ret) {
                    return ERROR_BUFFER_OVERFLOW;
                }
            }
        }
        break;
        case TAG_Int_Array: {
            int i;
            for (i = 0; i < len; i ++) {
                ret = writeUint32(buffer, ((uint32_t*)value)[i]);
                if (!ret) {
                    return ERROR_BUFFER_OVERFLOW;
                }
            }
        }
        break;
        case TAG_Long_Array: {
            int i;
            for (i = 0; i < len; i ++) {
                ret = writeUint64(buffer, ((uint64_t*)value)[i]);
                if (!ret) {
                    return ERROR_BUFFER_OVERFLOW;
                }
            }
        }
        break;
        default: return ERROR_INTERNAL;
    }
    return 0;
}

int nbt_write_string(NBT_Buffer* buffer, void* value, int32_t len, char* key) {
    int ret;
    
    ret = writeUint16(buffer, len-1);
    if (!ret) {
        return ERROR_BUFFER_OVERFLOW;
    }
    int i;
    for (i = 0; i < len-1; i ++) {
        ret = writeUint8(buffer, ((uint8_t*)value)[i]);
        if (!ret) {
            return ERROR_BUFFER_OVERFLOW;
        }
    }
    if (!ret) {
        return ERROR_BUFFER_OVERFLOW;
    }
    return 0;
}

int nbt_write_compound(NBT_Buffer* buffer, NBT* root) {
    int ret;
    NBT* child = root->child;
    while(child != NULL) {
        ret = nbt_write_nbt(buffer, child, 1);
        if (ret) {
            return ret;
        }
        child = child->next;
    }
    ret = writeUint8(buffer, 0);
    if (!ret) {
        return ERROR_BUFFER_OVERFLOW;
    }

    return 0;
}

int nbt_write_list(NBT_Buffer* buffer, NBT* root) {
    int ret;
    NBT* child = root->child;
    int count = 0;
    while(child != NULL) {
        count ++;
        child = child->next;
    }
    child = root->child;
    if (child == NULL) {
        ret = writeUint8(buffer, 0);
    } else {
        ret = writeUint8(buffer, child->type);
    }
    ret = writeUint32(buffer, count);
    if (!ret) {
        return ERROR_BUFFER_OVERFLOW;
    }
    if (!ret) {
        return ERROR_BUFFER_OVERFLOW;
    }
    while(child != NULL) {
        ret = nbt_write_nbt(buffer, child, 0);
        if (ret) {
            return ret;
        }
        child = child->next;
    }
    return 0;
}

int nbt_write_nbt(NBT_Buffer* buffer, NBT* root, int writekey) {
    int ret;
    if (writekey) {
        ret = nbt_write_key(buffer, root->key, root->type);
        if (ret) {
            return ret;
        }
    }
    switch(root->type) {
        case TAG_Byte:
        case TAG_Short:
        case TAG_Int:
        case TAG_Long:
        ret = nbt_write_number(buffer, root->value_i, root->key, root->type);
        if (ret) return ret;
        return 0;

        case TAG_Float:
        case TAG_Double:
        ret = nbt_write_point(buffer, root->value_d, root->key, root->type);
        if (ret) return ret;
        return 0;

        case TAG_Byte_Array:
        case TAG_Int_Array:
        case TAG_Long_Array:
        ret = nbt_write_array(buffer, root->value_a.value, root->value_a.len, root->key, root->type);
        if (ret) return ret;
        return 0;

        case TAG_String:
        ret = nbt_write_string(buffer, root->value_a.value, root->value_a.len, root->key);
        if (ret) return ret;
        return 0;

        case TAG_List:
        ret = nbt_write_list(buffer, root);
        if (ret) return ret;
        return 0;

        case TAG_Compound:
        ret = nbt_write_compound(buffer, root);
        if (ret) return ret;
        return 0;

        default:
        return ERROR_INTERNAL;
    }
}

int NBT_Pack_Opt(NBT* root, uint8_t* buffer, int* length, NBT_Error* errid) {
    NBT_Buffer *buf = init_buffer(buffer, *length);
    int ret;
    ret = nbt_write_nbt(buf, root, 1);
    fill_err(errid, ret, buf->pos);
    *length = buf->pos;
    return ret;
}

int NBT_Pack(NBT* root, uint8_t* buffer, int* length) {
    return NBT_Pack_Opt(root, buffer, length, NULL);
}

MCA* MCA_Init(char* filename) {
    MCA* ret = malloc(sizeof(MCA));
    memset(ret, 0, sizeof(MCA));
    if (filename && filename[0]) {
        char* str = strrchr(filename, '/');
        if (!str) str = filename;
        else str++;
        int count = sscanf(str, "r.%d.%d.mca", &ret->x, &ret->z);
        if (count == 2) {
            ret->hasPosition = 1;
        }
    }
    return ret;
}

MCA* MCA_Init_WithPos(int x, int z) {
    MCA* ret = malloc(sizeof(MCA));
    memset(ret, 0, sizeof(MCA));
    ret->hasPosition = 1;
    ret->x = x;
    ret->z = z;
    return ret;
}

void MCA_Free(MCA* mca) {
    int i;
    for (i = 0; i < CHUNKS_IN_REGION; i ++) {
        if (mca->data[i]) {
            NBT_Free(mca->data[i]);
        }
        if (mca->rawdata[i]) {
            free(mca->rawdata[i]);
        }
    }
    free(mca);
}

int MCA_ParseAll(MCA* mca) {
    int i;
    int errcount = 0;
    NBT_Error error;
    for (i = 0; i < CHUNKS_IN_REGION; i ++) {
        if (mca->rawdata[i]) {
            mca->data[i] = NBT_Parse_Opt(mca->rawdata[i], mca->size[i], &error);
            if (mca->data[i] == NULL) {
                errcount ++;
            }
        }
    }
    return errcount;
}

int MCA_ReadRaw_File(FILE* fp, MCA* mca, int skip_chunk_error) {

    memset(mca->rawdata, 0, sizeof(uint8_t*) * CHUNKS_IN_REGION);
    memset(mca->size, 0, sizeof(uint32_t) * CHUNKS_IN_REGION);

    if (fp == NULL) {
        return ERROR_INVALID_DATA;
    }
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    if (size <= 8192) {
        return ERROR_INVALID_DATA;
    }
    fseek(fp, 0, SEEK_SET);

    uint64_t offsets[CHUNKS_IN_REGION];

    int j;
    for (j = 0; j < CHUNKS_IN_REGION; j ++) {
        uint64_t t = fgetc(fp) << 16;
        t |= fgetc(fp) << 8;
        t |= fgetc(fp);
        offsets[j] = t << 12;
        uint64_t tsize = (fgetc(fp) << 12) + offsets[j];
        if (tsize > size) {
            if (skip_chunk_error) {
                offsets[j] = 0;
            } else {
                return ERROR_INVALID_DATA;
            }
        }
    }

    for (j = 0; j < CHUNKS_IN_REGION; j ++) {
        uint64_t t = fgetc(fp) << 24;
        t |= fgetc(fp) << 16;
        t |= fgetc(fp) << 8;
        t |= fgetc(fp);
        mca->epoch[j] = t;
    }

    for (j = 0; j < CHUNKS_IN_REGION; j ++) {

        if (offsets[j] == 0) {
            continue;
        }

        fseek(fp, offsets[j], SEEK_SET);
        uint32_t tsize = fgetc(fp) << 24;
        tsize |= fgetc(fp) << 16;
        tsize |= fgetc(fp) << 8;
        tsize |= fgetc(fp);

        uint8_t type = fgetc(fp);
        if (type != 2 && !skip_chunk_error) {
            goto chunk_error;
        }
        
        mca->rawdata[j] = malloc(tsize - 1);
        mca->size[j] = tsize - 1;
        int readSize = fread(mca->rawdata[j], 1, mca->size[j], fp);

        if (readSize != tsize - 1 && !skip_chunk_error) {
            goto chunk_error;
        }
    }
    return 0;
chunk_error: {
    int i;
    for (i = 0; i <= j; i ++) {
        if (mca->rawdata[i]) {
            free(mca->rawdata[i]);
        }
    }
    return ERROR_INVALID_DATA;
    }
}

int MCA_ReadRaw(uint8_t* data, int length, MCA* mca, int skip_chunk_error) {

    memset(mca->rawdata, 0, sizeof(uint8_t*) * CHUNKS_IN_REGION);
    memset(mca->size, 0, sizeof(uint32_t) * CHUNKS_IN_REGION);

    if (length <= 8192 || data == NULL) {
        return ERROR_INVALID_DATA;
    }

    uint64_t offsets[CHUNKS_IN_REGION];

    int j;
    NBT_Buffer *buffer = init_buffer(data, length);
    for (j = 0; j < CHUNKS_IN_REGION; j ++) {
        uint32_t temp = 0;
        int ret = getUint32(buffer, &temp);
        offsets[j] = temp >> 8 << 12;
        uint64_t tsize = ((temp & 0xff) << 12) + offsets[j];
        if (ret == 0 || tsize > length) {
            if (skip_chunk_error) {
                offsets[j] = 0;
            } else {
                return ERROR_INVALID_DATA;
            }
        }
    }

    for (j = 0; j < CHUNKS_IN_REGION; j ++) {
        uint32_t temp = 0;
        int ret = getUint32(buffer, &temp);
        if (ret == 0) {
            if (skip_chunk_error) {
                mca->epoch[j] = 0;
            } else {
                return ERROR_INVALID_DATA;
            }
        } else {
            mca->epoch[j] = temp;
        }
    }

    for (j = 0; j < CHUNKS_IN_REGION; j ++) {

        if (offsets[j] == 0) {
            continue;
        }

        buffer->pos = offsets[j];

        uint32_t tsize;
        uint8_t type;

        int ret = getUint32(buffer, &tsize);
        if (ret == 0) {
            if (skip_chunk_error) continue;
            else goto chunk_error;
        }

        ret = getUint8(buffer, &type);
        if ((ret == 0 || type != 2) && !skip_chunk_error) {
            goto chunk_error;
        }
        
        mca->rawdata[j] = malloc(tsize - 1);
        mca->size[j] = tsize - 1;

        memcpy(mca->rawdata[j], data + offsets[j] + 5, mca->size[j]);

    }
    return 0;
chunk_error: {
    int i;
    for (i = 0; i <= j; i ++) {
        if (mca->rawdata[i]) {
            free(mca->rawdata[i]);
        }
    }
    return ERROR_INVALID_DATA;
    }
}

int MCA_WriteRaw_File(FILE* fp, MCA* mca) {
    if (mca == NULL || fp == NULL) {
        return ERROR_INVALID_DATA;
    }
    int current = 2;
    uint32_t offsets[1024];
    int i;
    for (i = 0; i < CHUNKS_IN_REGION; i ++) {
        if (mca->rawdata[i] == NULL) {
            offsets[i] = 0;
            continue;
        }
        fseek(fp, current << 12, SEEK_SET);
        offsets[i] = current << 8;
        uint32_t size = mca->size[i] + 1;
        fputc((size >> 24) & 0xff, fp);
        fputc((size >> 16) & 0xff, fp);
        fputc((size >> 8) & 0xff, fp);
        fputc(size & 0xff, fp);
        fputc(2, fp);
        fwrite(mca->rawdata[i], 1, size - 1, fp);
        int newpos = (ftell(fp) >> 12) + 1;
        offsets[i] |= (newpos - current) & 0xff;
        current = newpos;
    }
    fseek(fp, 0, SEEK_SET);
    for (i = 0; i < CHUNKS_IN_REGION; i ++) {
        fputc((offsets[i] >> 24) & 0xff, fp);
        fputc((offsets[i] >> 16) & 0xff, fp);
        fputc((offsets[i] >> 8) & 0xff, fp);
        fputc(offsets[i] & 0xff, fp);
    }
    uint32_t t = bswap_32(time(NULL));
    uint8_t* tbuf = (uint8_t*)&t;
    for (i = 0; i < CHUNKS_IN_REGION; i ++) {
        fwrite(tbuf, 4, 1, fp);
    }
    fflush(fp);
    return 0;
}
