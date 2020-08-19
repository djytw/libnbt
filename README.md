# libnbt

Lightweight Minecraft NBT/MCA/SNBT file parser in C.

## Usage

### The libnbt library

The easiest way to incorporate `libnbt` is to include `nbt.c` and `nbt.h` in your project and start using it, since the entire library contains only these two files.

### Parsing NBT file

Supports uncompressed/zlib/gzip NBT files. Read the file to a byte array (by `fread` or whatever you like), then pass the array and array length to:

```c
NBT*  NBT_Parse(uint8_t* data, int length);
```

and it will parse the data, and store NBT infomation into the NBT tree.

This function returns NULL when error happens. To obtain the reason, alternative

```c
NBT*  NBT_Parse_Opt(uint8_t* data, int length, NBT_Error* err);
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
int   NBT_toSNBT_Opt(NBT* root, char* buff, int* bufflen, int maxlevel, int space, NBT_Error* errid);
```
`maxlevel` denotes how deep the SNBT goes. deeper tags will be replaced by `[...]`. Use `maxlevel = -1` to disable this

`space`: pretty-print the SNBT, and each tab replaced by `space` spaces. Use `space = -1` to disable pretty-print.

(Note: difference between space=`0` and `-1`: `0` do pretty-print without spaces, i.e. each tag in a new line. `-1` disables pretty-print, no spaces, no new line.)

`errid`: same to the return value, pass `NULL` to the function is allowed. 

### Pack NBT