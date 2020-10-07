# libnbt

Lightweight Minecraft NBT/MCA/SNBT file parser in C.

## Usage

### The libnbt library

The easiest way to incorporate `libnbt` is to include `nbt.c` and `nbt.h` in your project and start using it, since the entire library contains only these two files.

### Zlib or libdeflate

[libdeflate](https://github.com/ebiggers/libdeflate) is a faster compress & decompress library with higher compress rate comparing to Zlib. To enable libdeflate, pass `-DLIBNBT_USE_LIBDEFLATE` when compiling `nbt.c` (You need to build libdeflate yourself). 

To build examples with libdeflate, use `make ZLIB=LIBDEFLATE`.

### Parsing NBT file

Supports uncompressed/zlib/gzip NBT files. Read the file to a byte array (by `fread` or whatever you like), then pass the array and array length to:

```c
NBT*  NBT_Parse(uint8_t* data, size_t length);
```

and it will parse the data, and store NBT infomation into the NBT tree.

This function returns NULL when error happens. To obtain the reason, alternative

```c
NBT*  NBT_Parse_Opt(uint8_t* data, size_t length, NBT_Error* err);
```

can be used. A detailed usage is shown in [this example](https://github.com/djytw/libnbt/blob/master/example/basic_opt.c).

### Printing NBT file (aka. translate to SNBT)

Allocate a char array for output SNBT data, than pass the NBT tree, array, (pointer to)array length to

```c
int   NBT_toSNBT(NBT* root, char* buff, int* bufflen);
```

Output SNBT will be stored in array `buff`, data size will be stored in `bufflen`

print the array directly is ok, SNBT is '\0' ended. i.e. buff[bufflen] = '\0'

If any error occured, a non-zero value will be returned. Refer to [nbt.h](https://github.com/djytw/libnbt/blob/master/nbt.h) for error codes. Mostly it's caused by buffer overflow (the buffer you provided is not enough), in this case, as much SNBT data as the buffer can hold will be stored, others discarded.

Also a optional function can be used:

```c
int   NBT_toSNBT_Opt(NBT* root, char* buff, size_t* bufflen, int maxlevel, int space, NBT_Error* errid);
```
`maxlevel` denotes how deep the SNBT goes. deeper tags will be replaced by `[...]`. Use `maxlevel = -1` to disable this

`space`: pretty-print the SNBT, and each tab replaced by `space` spaces. Use `space = -1` to disable pretty-print.

(Note: difference between space=`0` and `-1`: `0` do pretty-print without spaces, i.e. each tag in a new line. `-1` disables pretty-print, no spaces, no new line.)

`errid`: same to the return value, pass `NULL` to the function is allowed. 

### Pack NBT

Allocate an array for output NBT data, than pass the NBT tree, array, (pointer to)array length to

```c
int   NBT_Pack(NBT* root, uint8_t* buffer, size_t* length);
```

This will pack the NBT tree to a gzipped raw NBT data array. If you want to change the compression, use

```c
int   NBT_Pack_Opt(NBT* root, uint8_t* buffer, size_t* length, NBT_Compression compression, NBT_Error* errid);
```

compression is defined in `nbt.h`

### Read MCA region file

Read an MCA contains several steps. A detailed usage shown in [this example](https://github.com/djytw/libnbt/blob/master/example/readmca.c).

1. first you should init the MCA structure by

```c
MCA*  MCA_Init(char* filename);
```
or
```c
MCA*  MCA_Init_WithPos(int x, int z);
```
Region position is not recorded in the MCA file, but the filename, in the form `r.x.z.mca`, where x and z are the region's coordinates. If you are going to read a MCA file, use the first function is the easiest way. Just pass the file path to it, like
```c
MCA* region = MCA_Init("/path/to/the/r.x.y.mca");
```
and it will automantically analyze the file name, initialize the structure with the position.

Alternatively, the second function can be used if you do not have the filename, (eg. reading from a stream), and manually set the region position.

Region position has no influence on parsing the MCA file or NBT data within it, so you can pass a NULL filename or a random position to the init function safely. But it's always a good habit to save the right position for further procedure. (eg. check chunk position)

2. then read the MCA file with

```c
int   MCA_ReadRaw_File(FILE* fp, MCA* mca, int skip_chunk_error);
```
or
```c
int   MCA_ReadRaw(uint8_t* data, size_t length, MCA* mca, int skip_chunk_error);
```
This will read the MCA file to 1024 (i.e. chunk number per region) raw NBT data arrays, and store in the MCA structure.

Note: you need fclose the file/free the data yourself, after read.

3. Parse raw NBT (+modify if needs)

previous NBT parse function can be used, if few chunks to be processed:
```c
NBT*  NBT_Parse(uint8_t* data, size_t length);
```
or you can use
```c
int   MCA_ParseAll(MCA* mca);
```
this will parse all 1024 chunks in the region, and save it to MCA.data

after modify, pack the NBT tree to zlib data, and save it to MCA.rawdata. For example:
```c
free(mca.rawdata[i]);
size_t size = 1<<20;
mca.rawdata[i] = malloc(size);
NBT_Pack_Opt(nbt_tree, mca.rawdata[i], &size, NBT_Compression_ZLIB, NULL);
mca.size[i] = size;
```

4. Pack the MCA with (if needed) and release the memory

```c
int   MCA_WriteRaw_File(FILE* fp, MCA* mca);
void  MCA_Free(MCA* mca);
```

### Helper functions

```c
NBT*  NBT_GetChild(NBT* root, const char* key);
```
Get the child named `key` of a `TAG_Compound` NBT Tree. If `root` is not a `TAG_Compound` or do not have a child named `key`, NULL will be returned. 

```c
NBT*  NBT_GetChild_Deep(NBT* root, ...);
```
Get child recursively. Note the parameters must ends with a NULL.

For example:

```c
NBT_GetChild_Deep(root, "aaa", "bbb", NULL);
```
is used to get `root.aaa.bbb`, same to
```c
NBT_GetChild(NBT_GetChild(root,"aaa"),"bbb");
```
