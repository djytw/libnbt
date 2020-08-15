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

#define CHECK_WRITELEN(writelen, maxlen, buffer, tempbuf) { \
        (buffer)->pos += (writelen);                        \
        (maxlen) = (buffer)->len - (buffer)->pos;           \
        if ((writelen) == 0) {                              \
            return ERROR_INTERNAL;                          \
        }                                                   \
        if ((maxlen) <= 0) {                                \
            return ERROR_BUFFER_OVERFLOW;                   \
        }                                                   \
        (writelen) = 0;                                     \
        (tempbuf) = (char*)&(buffer)->data[(buffer)->pos];  \
        }

NBT* create_NBT(uint8_t type);
NBT_Buffer* init_buffer(uint8_t* data, int length);
int getUint8(NBT_Buffer* buffer, uint8_t* result);
int getUint16(NBT_Buffer* buffer, uint16_t* result);
int getUint32(NBT_Buffer* buffer, uint32_t* result);
int getUint64(NBT_Buffer* buffer, uint64_t* result);
int getFloat(NBT_Buffer* buffer, float* result);
int getDouble(NBT_Buffer* buffer, double* result);
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
void fill_err(NBT_Error* err, int errid, int position);

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
    *result = bswap_16(*result);
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

int getUint64(NBT_Buffer* buffer, uint64_t* result) {
    if (buffer->pos + 8 > buffer->len) {
        return 0;
    }
    *result = *(uint64_t*)(buffer->data + buffer->pos);
    buffer->pos += 8;
    *result = bswap_64(*result);
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
            saveto->value_a.len = len;
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
            saveto->value_a.len = len;
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
        char* buf = (char*)&buffer->data[buffer->pos];
        int maxlen = buffer->len - buffer->pos;
        int writelen = snprintf(buf, maxlen, "%s:", key);
        CHECK_WRITELEN(writelen, maxlen, buffer, buf);
    }
    return 0;
}

int snbt_write_number(NBT_Buffer* buffer, uint64_t value, char* key, int type) {

    int ret = snbt_write_key(buffer, key);
    if (ret) {
        return ret;
    }

    char* buf = (char*)&buffer->data[buffer->pos];
    int maxlen = buffer->len - buffer->pos;
    int writelen = 0;
    
    switch(type) {
        case TAG_Byte: writelen = snprintf(buf, maxlen, "%db,", (int8_t)value); break;
        case TAG_Short: writelen = snprintf(buf, maxlen, "%ds,", (int16_t)value); break;
        case TAG_Int: writelen = snprintf(buf, maxlen, "%d,", (int32_t)value); break;
        case TAG_Long: writelen = snprintf(buf, maxlen, "%ldl,", (int64_t)value); break;
        default: return ERROR_INTERNAL;
    }
    CHECK_WRITELEN(writelen, maxlen, buffer, buf);
    return 0;
}

int snbt_write_point(NBT_Buffer* buffer, double value, char* key, int type) {

    int ret = snbt_write_key(buffer, key);
    if (ret) {
        return ret;
    }

    char* buf = (char*)&buffer->data[buffer->pos];
    int maxlen = buffer->len - buffer->pos;
    int writelen = 0;

    switch(type) {
        case TAG_Float: writelen = snprintf(buf, maxlen, "%ff,", (float)value); break;
        case TAG_Double: writelen = snprintf(buf, maxlen, "%lfd,", (double)value); break;
        default: return ERROR_INTERNAL;
    }
    CHECK_WRITELEN(writelen, maxlen, buffer, buf);
    return 0;
}

int snbt_write_array(NBT_Buffer* buffer, void* value, int length, char* key, int type) {
    int ret = snbt_write_key(buffer, key);
    if (ret) {
        return ret;
    }

    char* buf = (char*)&buffer->data[buffer->pos];
    int maxlen = buffer->len - buffer->pos;
    int writelen = 0;

    switch(type) {
        case TAG_Byte_Array: writelen = snprintf(buf, maxlen, "[B;"); break;
        case TAG_Int_Array: writelen = snprintf(buf, maxlen, "[I;"); break;
        case TAG_Long_Array: writelen = snprintf(buf, maxlen, "[L;"); break;
        default: return ERROR_INTERNAL;
    }
    CHECK_WRITELEN(writelen, maxlen, buffer, buf);

    int i;
    for (i = 0; i < length; i ++) {
        switch(type) {
            case TAG_Byte_Array: writelen = snprintf(buf, maxlen, "%db,", ((int8_t*)value)[i]); break;
            case TAG_Int_Array: writelen = snprintf(buf, maxlen, "%d,", ((int32_t*)value)[i]); break;
            case TAG_Long_Array: writelen = snprintf(buf, maxlen, "%ldl,", ((int64_t*)value)[i]); break;
            default: return ERROR_INTERNAL;
        }
        CHECK_WRITELEN(writelen, maxlen, buffer, buf);
    }
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

    char* buf = (char*)&buffer->data[buffer->pos];
    int maxlen = buffer->len - buffer->pos;
    int writelen = 0;

    if (isarray) {
        writelen = snprintf(buf, maxlen, "[");
    } else {
        writelen = snprintf(buf, maxlen, "{");
    }
    CHECK_WRITELEN(writelen, maxlen, buffer, buf);

    if (level <= curlevel && level >= 0) {
        writelen = snprintf(buf, maxlen, "...");
        CHECK_WRITELEN(writelen, maxlen, buffer, buf);
    } else {
        if (space >= 0) {
            writelen = snprintf(buf, maxlen, "\n");
            CHECK_WRITELEN(writelen, maxlen, buffer, buf);
        }
        NBT* child = root->child;
        while(child != NULL) {
            ret = snbt_write_nbt(buffer, child, level, space, curlevel + 1);
            buf = (char*)&buffer->data[buffer->pos];
            if (ret) {
                return ret;
            }
            if (child->next == NULL) {
                buffer->pos --;
                buf = (char*)&buffer->data[buffer->pos];
                buf[0] = 0;
            }
            if (space >= 0) {
                writelen = snprintf(buf, maxlen, "\n");
                CHECK_WRITELEN(writelen, maxlen, buffer, buf);
            }
            child = child->next;
        }
        ret = snbt_write_space(buffer, space * curlevel);
        buf = (char*)&buffer->data[buffer->pos];
        if (ret) {
            return ret;
        }
    }

    if (isarray) {
        writelen = snprintf(buf, maxlen, "],");
    } else {
        writelen = snprintf(buf, maxlen, "},");
    }
    CHECK_WRITELEN(writelen, maxlen, buffer, buf);
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

int decompress (uint8_t* dest, size_t* destsize, uint8_t* src, size_t srcsize) {
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

int NBT_toSNBT_Opt(NBT* root, char* buff, int bufflen, int maxlevel, int space, NBT_Error* errid) {
    NBT_Buffer buffer;
    buffer.data = (uint8_t*)buff;
    buffer.len = bufflen;
    buffer.pos = 0;
    int ret;
    ret = snbt_write_nbt(&buffer, root, maxlevel, space, 0);
    fill_err(errid, ret, buffer.pos);
    if (ret == 0) {
        buffer.data[buffer.pos - 1] = 0;
    }
    return buffer.pos;
}

int NBT_toSNBT(NBT* root, char* buff, int bufflen) {
    return NBT_toSNBT_Opt(root, buff, bufflen, -1, -1, NULL);
}

NBT* NBT_Parse_Opt(uint8_t* data, int length, NBT_Error* errid) {

    NBT_Buffer* buffer;

    // check if gzipped 
    if (length > 1 && data[0] == 0x1f && data[1] == 0x8b) {
        size_t size = length * 20;
        uint8_t* undata = malloc(size);
        int ret = decompress(undata, &size, data, length);
        if (ret != Z_OK) {
            fill_err(errid, ERROR_INVALID_DATA, 0);
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