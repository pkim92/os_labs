// BMP-related data types based on Microsoft's own

#include <stdint.h>

// aliases for C/C++ primitive data types
// https://msdn.microsoft.com/en-us/library/cc230309.aspx
typedef uint8_t  BYTE;
typedef uint32_t DWORD;
typedef int32_t  LONG;
typedef uint16_t WORD;

// information about the type, size, and layout of a file
// https://msdn.microsoft.com/en-us/library/dd183374(v=vs.85).aspx
typedef struct
{
    WORD type;
    DWORD size;
    WORD reserved1;
    WORD reserved2;
    DWORD offset;
    DWORD dib_header_size;
    LONG width_px;
    LONG height_px;
    WORD num_planes;
    WORD bits_per_pixel;
    DWORD compression;
    DWORD image_size_bytes;
    LONG x_resolution_ppm;
    LONG y_resolution_ppm;
    DWORD num_colors;
    DWORD important_colors;
} __attribute__((__packed__))
BMP_HEADER;

// relative intensities of red, green, and blue
// https://msdn.microsoft.com/en-us/library/dd162939(v=vs.85).aspx
typedef struct
{
    BYTE blue;
    BYTE green;
    BYTE red;
} __attribute__((__packed__))
RGB_COLOR;