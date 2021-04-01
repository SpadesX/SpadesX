#include <stdio.h>
#include <stdlib.h>

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

// width = 512
// height = 512
// depth = 64
//      0 = top
//      63 = bottom
//
// (0,0) -> (511,0)
// next (0,1)
//
// z = 63 -> (under) water - solid
// z = 62 -> indestructible - solid
//
// column consist of spans (heights from z=0 to z=63)
// span:
//  0 or more -> open voxels
//  1 or more -> colored voxels (top)
//  0 or more -> solid voxels
//  0 or more -> colored voxels (bottom)
//
// span format:
//      0   N   length of span data (N * 4 including span header)
//      1   S   starting height of top colored run
//      2   E   ending height of top colored run (length = E - S + 1)
//      3   A   starting height of air run (first span ignores value and assumes A=0)
//  4+i*4   b   blue color for colored #i
//  5+i*4   g   green color for colored #i
//  6+i*4   r   red color for colored #i
//  7+i*4   a   alpha channel for colored #i, actually used for shading in unknown way
//
//  i: 0 -> N - 1
//
// heights for the bottom colors are implied by the starting height of the air run of the next span; they appear
// just above the air
//
// next byte after the above list (at 8 + N * 4) is the first byte of the next span of the column
//
// location of the air run of the next span is at (8 + N*4 + 3)
//
// if N = 0 -> last span of the column
// last span contains air, a top-colored span, and all voxels below down to 63 are solid
//
// storage used by the span is determined by the number of top colors
// = 4 * (1 + (E - S + 1))
//
// K = E - S + 1
// Z = (N - 1) - K or 0 if N == 0
// M = A stored in next span or 64 if last span of the column
//
//  Run 	Start 	End 	    Length
//  air 	A 	    S - 1 	    S - A
//  top 	S 	    E 	        E - S + 1
//  solid   E + 1   M - Z - 1   M - Z - (E + 1)
//  bottom  M - Z   M - 1       Z
//

typedef struct
{
    uint8***  map;
    uint32*** colors;
} Map;

void ReadMapV3(uint8* data)
{
#define SIZE (512 * 512 * 64)
    uint8*  map    = (uint8*) malloc(SIZE);
    uint32* colors = (uint32*) malloc(sizeof(uint32) * SIZE);

    uint8*  pMap    = map;
    uint32* pColors = colors;

    uint32  z = 0;
    uint8   spanSize, startTop, endTop, startAir, endBottom, startBottom;
    uint32  counter = 0;
    uint32* color;

    while (counter < 512 * 512) {
        spanSize = data[0]; // N
        startTop = data[1]; // S
        endTop   = data[2]; // E
        startAir = data[3]; // A

        // fill air
        for (; z < startTop; ++z) {
            *(map++)    = 0;
            *(colors++) = 0;
        }

        // fill top blocks
        color = (uint32*) (data + 4);
        for (; z <= endTop; ++z, ++color) {
            *(map++)    = 1;
            *(colors++) = *color;
        }
        data = (uint8*) color;

        // if span size is 0 then it is last span in column
        if (spanSize == 0) {
            ++counter;
            if ((counter & 0x1FF) == 0) {
                printf("%02X\n", z);
            } else {
                printf("%02X", z);
            }
            z = 0;
            continue;
        }

        endBottom   = *(data + 3); // in next span
        startBottom = endBottom - spanSize - endTop + startTop + 2;
        // M - Z = A - (N - 1) - K = A - N - 1 - (E - S + 1) = A - N - E + S - 2

        // fill bottom blocks
        color = (uint32*) data;
        for (z = startBottom; z < endBottom; ++z, ++color) {
            *(map++)    = 1;
            *(colors++) = *color;
        }
        data = (uint8*) colors;
    }

    free(pMap);
    free(pColors);
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        return EXIT_FAILURE;
    }

    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        perror("file not found");
        return EXIT_FAILURE;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8* buffer = (uint8*) malloc(size);
    fread(buffer, size, 1, file);
    fclose(file);

    ReadMapV3(buffer);

    //    printf("starting\n");

    // Map* map = ReadMap(buffer, size);

    // printf("done\n");

    // DestroyMap(map);
    free(buffer);
    return EXIT_SUCCESS;
}
