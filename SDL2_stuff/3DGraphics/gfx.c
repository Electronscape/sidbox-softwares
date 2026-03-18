// file: gfx.c

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gfx.h"

// basic colour look up table, for use with the graphics memory look up

uint32_t clut[256] = {
    0x00000000, 0xFFAFAFAF, 0xFFFFFFFF, 0xFF3B67A2, 0xFFAA907C, 0xFF959595, 0xFF7B7B7B, 0xFFFFA997,
    0xFF37A91D, 0xFF7CA9FF, 0xFFBF8112, 0xFFEBBF66, 0xFF78C178, 0xFF3D9318, 0xFFB33418, 0xFFD9311C,
    0xFF000000, 0xFF00000E, 0xFF00001D, 0xFF00002B, 0xFF000139, 0xFF000147, 0xFF000156, 0xFF000164,
    0xFF0001D2, 0xFF0001FF, 0xFFCECECE, 0xFF00FF00, 0xFFB2FF00, 0xFFFFE700, 0xFFFF9600, 0xFFFF1100,
    0xFF491200, 0xFF491355, 0xFF4914AA, 0xFF4916FF, 0xFF5B1700, 0xFF5B1855, 0xFF5B19AA, 0xFF5B1AFF,
    0xFF6D1B00, 0xFF6D1C55, 0xFF00E300, 0xFF85FF54, 0xFFC4FF00, 0xFFFFD900, 0xFFFFA41F, 0xFFE05400,
    0xFFFF0000, 0xFF922655, 0xFF9227AA, 0xFF9228FF, 0xFFA42900, 0xFFA42A55, 0xFFA42BAA, 0xFFA42CFF,
    0xFFB62D00, 0xFFB62F55, 0xFFB630AA, 0xFFB631FF, 0xFFC93200, 0xFFC93355, 0xFFC934AA, 0xFFC935FF,
    0xFFDB3700, 0xFFDB3855, 0xFFDB39AA, 0xFFDB3AFF, 0xFFED3B00, 0xFFED3C55, 0xFFED3DAA, 0xFFED3FFF,
    0xFFFF4000, 0xFFFF4155, 0xFFFF42AA, 0xFFFF43FF, 0xFF004400, 0xFF004555, 0xFF0046AA, 0xFF0048FF,
    0xFFFFFF00, 0xFF12FF55, 0xFF12EE55, 0xFF12B6FF, 0xFF001FFF, 0xFF9D0EC7, 0xFFF10000, 0xFFFF7700,
    0xFF375200, 0xFF375355, 0xFF3754AA, 0xFF3755FF, 0xFF495600, 0xFF495855, 0xFF4959AA, 0xFF495AFF,
    0xFF5B5B00, 0xFF5B5C55, 0xFF5B5DAA, 0xFF5B5EFF, 0xFF6D6000, 0xFF6D6155, 0xFF6D62AA, 0xFF6D63FF,
    0xFF6D6400, 0xFF806555, 0xFF8066AA, 0xFF8067FF, 0xFF926900, 0xFF926A55, 0xFF926BAA, 0xFF926CFF,
    0xFFA46D00, 0xFFA46E55, 0xFFA46FAA, 0xFFA471FF, 0xFFB67200, 0xFFB67355, 0xFFB674AA, 0xFFB675FF,
    0xFFC97600, 0xFFC97755, 0xFFC979AA, 0xFFC97AFF, 0xFFDB7B00, 0xFFDB7C55, 0xFFDB7DAA, 0xFFDB7EFF,
    0xFFED7F00, 0xFFED8055, 0xFFED82AA, 0xFFED83FF, 0xFFFF8400, 0xFFFF8555, 0xFFFF86AA, 0xFFFF87FF,
    0xFF008800, 0xFF008A55, 0xFF008BAA, 0xFF008CFF, 0xFF128D00, 0xFF128E55, 0xFF128FAA, 0xFF1290FF,
    0xFF249200, 0xFF249355, 0xFF2494AA, 0xFF2495FF, 0xFF379600, 0xFF379755, 0xFF3798AA, 0xFF3799FF,
    0xFF499B00, 0xFF499C55, 0xFF499DAA, 0xFF499EFF, 0xFF5B9F00, 0xFF5BA055, 0xFF5BA1AA, 0xFF5BA3FF,
    0xFFA4B5D5, 0xFFA0B0F8, 0xFF94A3E6, 0xFF7C89C1, 0xFF6281C0, 0xFF1C62A1, 0xFF4254EA, 0xFF62A1BD,
    0xFF7093C0, 0xFF4977A1, 0xFF003FAA, 0xFF1554FF, 0xFF1C50B9, 0xFF00B3FF, 0xFF0088AA, 0xFF00B5FF,
    0xFF0E62FF, 0xFF5EB7E3, 0xFFBDC0B9, 0xFF85B9FF, 0xFF006CAF, 0xFF1F81B9, 0xFF3F5BAA, 0xFFC9BEFF,
    0xFF5BAFCB, 0xFFDBC055, 0xFFDBC1AA, 0xFFBDC0C0, 0xFFEDC400, 0xFFEDC555, 0xFFEDC6AA, 0xFFEDC7FF,
    0xFFFFC800, 0xFFFFC955, 0xFFFFCAAA, 0xFFFFCCFF, 0xFF00CD00, 0xFF00CE55, 0xFF00CFAA, 0xFF00D0FF,
    0xFF12D100, 0xFF12D255, 0xFF12D3AA, 0xFF12D5FF, 0xFF24D600, 0xFF24D755, 0xFF24D8AA, 0xFF24D9FF,
    0xFF37DA00, 0xFF37DB55, 0xFF37DDAA, 0xFF37DEFF, 0xFF49DF00, 0xFF49E055, 0xFF49E1AA, 0xFF49E2FF,
    0xFF5BE300, 0xFF5BE555, 0xFF5BE6AA, 0xFF5BE7FF, 0xFF6DE800, 0xFF6DE955, 0xFF6DEAAA, 0xFF6DEBFF,
    0xFF6DEC00, 0xFF80EE55, 0xFF80EFAA, 0xFF80F0FF, 0xFF93CEA2, 0xFF92F255, 0xFF92F3AA, 0xFF92F4FF,
    0xFFA4F600, 0xFFA4F755, 0xFFA4F8AA, 0xFFA4F9FF, 0xFFB6FA00, 0xFFB6FB55, 0xFFB6FCAA, 0xFFB6FEFF,
    0xFFC9FF00, 0xFFC9FF55, 0xFFC9FFAA, 0xFFC9FFFF, 0xFFDBFF00, 0xFFDBFF55, 0xFFDBFFAA, 0xFFDBFFFF,
    0xFFEDFF00, 0xFFEDFF55, 0xFFEDFFAA, 0xFFEDFFFF, 0xFFFFFF00, 0xFFFFFF55, 0xFFFFFFAA, 0xFFFFFFFF
};



static const uint8_t bayer4x4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
};



// 8x8 font table: 256 glyphs, 8 rows each
const uint8_t font[256][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (  0)   0x00
    { 0x3C, 0x42, 0xA5, 0x81, 0xA5, 0x99, 0x42, 0x3C },   // (  1)   0x01
    { 0x00, 0x03, 0x06, 0x0C, 0xD8, 0x70, 0x20, 0x00 },   // (  2)   0x02
    { 0x30, 0x08, 0x3C, 0x5E, 0xBF, 0xFF, 0x7E, 0x3C },   // (  3)   0x03
    { 0x78, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0x00 },   // (  4)   0x04
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00 },   // (  5)   0x05
    { 0x00, 0x66, 0x99, 0x99, 0x99, 0x66, 0x00, 0x00 },   // (  6)   0x06
    { 0x6C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x6C },   // (  7)   0x07
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (  8)   0x08
    { 0x00, 0x20, 0x50, 0x20, 0x00, 0x00, 0x00, 0x00 },   // (  9)   0x09
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // ( 10)   0x0A
    { 0x00, 0x1C, 0x0C, 0x34, 0x48, 0x48, 0x30, 0x00 },   // ( 11)   0x0B
    { 0x38, 0x44, 0x44, 0x38, 0x10, 0x38, 0x10, 0x00 },   // ( 12)   0x0C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // ( 13)   0x0D
    { 0x0C, 0x34, 0x2C, 0x34, 0x2C, 0x6C, 0x60, 0x00 },   // ( 14)   0x0E
    { 0x00, 0x54, 0x38, 0x6C, 0x38, 0x54, 0x00, 0x00 },   // ( 15)   0x0F
    { 0x20, 0x30, 0x38, 0x3C, 0x38, 0x30, 0x20, 0x00 },   // ( 16)   0x10
    { 0x08, 0x18, 0x38, 0x78, 0x38, 0x18, 0x08, 0x00 },   // ( 17)   0x11
    { 0x10, 0x38, 0x7C, 0x10, 0x7C, 0x38, 0x10, 0x00 },   // ( 18)   0x12
    { 0x28, 0x28, 0x28, 0x28, 0x28, 0x00, 0x28, 0x00 },   // ( 19)   0x13
    { 0x3C, 0x54, 0x54, 0x34, 0x14, 0x14, 0x14, 0x00 },   // ( 20)   0x14
    { 0x38, 0x44, 0x30, 0x28, 0x18, 0x44, 0x38, 0x00 },   // ( 21)   0x15
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x78, 0x00 },   // ( 22)   0x16
    { 0x10, 0x38, 0x7C, 0x10, 0x7C, 0x38, 0x10, 0x38 },   // ( 23)   0x17
    { 0x10, 0x38, 0x7C, 0x10, 0x10, 0x10, 0x00, 0x00 },   // ( 24)   0x18
    { 0x00, 0x10, 0x10, 0x10, 0x7C, 0x38, 0x10, 0x00 },   // ( 25)   0x19
    { 0x00, 0x10, 0x18, 0x7C, 0x18, 0x10, 0x00, 0x00 },   // ( 26)   0x1A
    { 0x00, 0x10, 0x30, 0x7C, 0x30, 0x10, 0x00, 0x00 },   // ( 27)   0x1B
    { 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x7C, 0x00 },   // ( 28)   0x1C
    { 0x00, 0x28, 0x28, 0x7C, 0x28, 0x28, 0x00, 0x00 },   // ( 29)   0x1D
    { 0x10, 0x10, 0x38, 0x38, 0x7C, 0x7C, 0x00, 0x00 },   // ( 30)   0x1E
    { 0x7C, 0x7C, 0x38, 0x38, 0x10, 0x10, 0x00, 0x00 },   // ( 31)   0x1F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // ( 32)  [SPACE]
    { 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00 },   // ( 33)  [ ! ]
    { 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // ( 34)  [ " ]
    { 0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00 },   // ( 35)  [ # ]
    { 0x18, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x18, 0x00 },   // ( 36)  [ $ ]
    { 0x00, 0x66, 0xAC, 0xD8, 0x36, 0x6A, 0xCC, 0x00 },   // ( 37)  [ % ]
    { 0x38, 0x6C, 0x68, 0x76, 0xDC, 0xCE, 0x7B, 0x00 },   // ( 38)  [ & ]
    { 0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 },   // ( 39)  [ ' ]
    { 0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00 },   // ( 40)  [ ( ]
    { 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00 },   // ( 41)  [ ) ]
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00 },   // ( 42)  [ * ]
    { 0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00 },   // ( 43)  [ + ]
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30 },   // ( 44)  [ , ]
    { 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00 },   // ( 45)  [ - ]
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00 },   // ( 46)  [ . ]
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x00 },   // ( 47)  [ / ]
    { 0x3C, 0x66, 0x6E, 0x7E, 0x76, 0x66, 0x3C, 0x00 },   // ( 48)  [ 0 ]
    { 0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x18, 0x00 },   // ( 49)  [ 1 ]
    { 0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00 },   // ( 50)  [ 2 ]
    { 0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00 },   // ( 51)  [ 3 ]
    { 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x0C, 0x00 },   // ( 52)  [ 4 ]
    { 0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00 },   // ( 53)  [ 5 ]
    { 0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00 },   // ( 54)  [ 6 ]
    { 0x7E, 0x06, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x00 },   // ( 55)  [ 7 ]
    { 0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00 },   // ( 56)  [ 8 ]
    { 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00 },   // ( 57)  [ 9 ]
    { 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00 },   // ( 58)  [ : ]
    { 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30 },   // ( 59)  [ ; ]
    { 0x00, 0x06, 0x18, 0x60, 0x18, 0x06, 0x00, 0x00 },   // ( 60)  [ < ]
    { 0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00 },   // ( 61)  [ = ]
    { 0x00, 0x60, 0x18, 0x06, 0x18, 0x60, 0x00, 0x00 },   // ( 62)  [ > ]
    { 0x3C, 0x66, 0x06, 0x0C, 0x18, 0x00, 0x18, 0x00 },   // ( 63)  [ ? ]
    { 0x7C, 0xC6, 0xDE, 0xD6, 0xDE, 0xC0, 0x78, 0x00 },   // ( 64)  [ @ ]
    { 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00 },   // ( 65)  [ A ]
    { 0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00 },   // ( 66)  [ B ]
    { 0x1E, 0x30, 0x60, 0x60, 0x60, 0x30, 0x1E, 0x00 },   // ( 67)  [ C ]
    { 0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00 },   // ( 68)  [ D ]
    { 0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00 },   // ( 69)  [ E ]
    { 0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00 },   // ( 70)  [ F ]
    { 0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3E, 0x00 },   // ( 71)  [ G ]
    { 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00 },   // ( 72)  [ H ]
    { 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00 },   // ( 73)  [ I ]
    { 0x06, 0x06, 0x06, 0x06, 0x06, 0x66, 0x3C, 0x00 },   // ( 74)  [ J ]
    { 0xC6, 0xCC, 0xD8, 0xF0, 0xD8, 0xCC, 0xC6, 0x00 },   // ( 75)  [ K ]
    { 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00 },   // ( 76)  [ L ]
    { 0xC6, 0xEE, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0x00 },   // ( 77)  [ M ]
    { 0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00 },   // ( 78)  [ N ]
    { 0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00 },   // ( 79)  [ O ]
    { 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00 },   // ( 80)  [ P ]
    { 0x78, 0xCC, 0xCC, 0xCC, 0xCC, 0xDC, 0x7E, 0x00 },   // ( 81)  [ Q ]
    { 0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00 },   // ( 82)  [ R ]
    { 0x3C, 0x66, 0x70, 0x3C, 0x0E, 0x66, 0x3C, 0x00 },   // ( 83)  [ S ]
    { 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 },   // ( 84)  [ T ]
    { 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00 },   // ( 85)  [ U ]
    { 0x66, 0x66, 0x66, 0x66, 0x3C, 0x3C, 0x18, 0x00 },   // ( 86)  [ V ]
    { 0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00 },   // ( 87)  [ W ]
    { 0xC3, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0xC3, 0x00 },   // ( 88)  [ X ]
    { 0xC3, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x00 },   // ( 89)  [ Y ]
    { 0xFE, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0xFE, 0x00 },   // ( 90)  [ Z ]
    { 0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00 },   // ( 91)  [ [ ]
    { 0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x00 },   // ( 92)  [BACKSLASH]
    { 0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00 },   // ( 93)  [ ] ]
    { 0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00 },   // ( 94)  [ ^ ]
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF },   // ( 95)  [ _ ]
    { 0x18, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00 },   // ( 96)  [ ` ]
    { 0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00 },   // ( 97)  [ a ]
    { 0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00 },   // ( 98)  [ b ]
    { 0x00, 0x00, 0x3C, 0x60, 0x60, 0x60, 0x3C, 0x00 },   // ( 99)  [ c ]
    { 0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00 },   // (100)  [ d ]
    { 0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00 },   // (101)  [ e ]
    { 0x1C, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x30, 0x00 },   // (102)  [ f ]
    { 0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x3C },   // (103)  [ g ]
    { 0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00 },   // (104)  [ h ]
    { 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x0C, 0x00 },   // (105)  [ i ]
    { 0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x78 },   // (106)  [ j ]
    { 0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00 },   // (107)  [ k ]
    { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x0C, 0x00 },   // (108)  [ l ]
    { 0x00, 0x00, 0xEC, 0xFE, 0xD6, 0xC6, 0xC6, 0x00 },   // (109)  [ m ]
    { 0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00 },   // (110)  [ n ]
    { 0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00 },   // (111)  [ o ]
    { 0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60 },   // (112)  [ p ]
    { 0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06 },   // (113)  [ q ]
    { 0x00, 0x00, 0x7C, 0x66, 0x60, 0x60, 0x60, 0x00 },   // (114)  [ r ]
    { 0x00, 0x00, 0x3C, 0x60, 0x3C, 0x06, 0x7C, 0x00 },   // (115)  [ s ]
    { 0x30, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x1C, 0x00 },   // (116)  [ t ]
    { 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00 },   // (117)  [ u ]
    { 0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00 },   // (118)  [ v ]
    { 0x00, 0x00, 0xC6, 0xC6, 0xD6, 0xFE, 0x6C, 0x00 },   // (119)  [ w ]
    { 0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00 },   // (120)  [ x ]
    { 0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x30 },   // (121)  [ y ]
    { 0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00 },   // (122)  [ z ]
    { 0x0E, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0E, 0x00 },   // (123)  [ { ]
    { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 },   // (124)  [ | ]
    { 0x70, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x70, 0x00 },   // (125)  [ } ]
    { 0x72, 0x9C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (126)  [ ~ ]
    { 0x0F, 0x3C, 0xF0, 0xC3, 0x0F, 0x3C, 0xF0, 0x00 },   // (127)   0x7F
    { 0x38, 0x44, 0x40, 0x40, 0x44, 0x38, 0x10, 0x30 },   // (128)   0x80
    { 0x48, 0x00, 0x48, 0x48, 0x48, 0x58, 0x28, 0x00 },   // (129)   0x81
    { 0x0C, 0x00, 0x38, 0x44, 0x78, 0x40, 0x38, 0x00 },   // (130)   0x82
    { 0x38, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00 },   // (131)   0x83
    { 0x28, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00 },   // (132)   0x84
    { 0x30, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00 },   // (133)   0x85
    { 0x38, 0x28, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00 },   // (134)   0x86
    { 0x00, 0x38, 0x44, 0x40, 0x44, 0x38, 0x10, 0x30 },   // (135)   0x87
    { 0x38, 0x00, 0x38, 0x44, 0x78, 0x40, 0x38, 0x00 },   // (136)   0x88
    { 0x28, 0x00, 0x38, 0x44, 0x78, 0x40, 0x38, 0x00 },   // (137)   0x89
    { 0x30, 0x00, 0x38, 0x44, 0x78, 0x40, 0x38, 0x00 },   // (138)   0x8A
    { 0x28, 0x00, 0x10, 0x10, 0x10, 0x10, 0x18, 0x00 },   // (139)   0x8B
    { 0x38, 0x00, 0x10, 0x10, 0x10, 0x10, 0x18, 0x00 },   // (140)   0x8C
    { 0x20, 0x00, 0x10, 0x10, 0x10, 0x10, 0x18, 0x00 },   // (141)   0x8D
    { 0x28, 0x00, 0x10, 0x28, 0x44, 0x7C, 0x44, 0x00 },   // (142)   0x8E
    { 0x38, 0x28, 0x38, 0x6C, 0x44, 0x7C, 0x44, 0x00 },   // (143)   0x8F
    { 0x06, 0x29, 0x00, 0x48, 0x11, 0x01, 0x07, 0x0F },   // (144)   0x90
    { 0x00, 0x00, 0x80, 0x40, 0xF0, 0xF0, 0xFC, 0xFE },   // (145)   0x91
    { 0x0D, 0x1F, 0x1F, 0x0F, 0x0F, 0x07, 0x03, 0x00 },   // (146)   0x92
    { 0xFE, 0xFF, 0xEF, 0xEE, 0xEE, 0xDC, 0xF8, 0xE0 },   // (147)   0x93
    { 0x28, 0x00, 0x30, 0x48, 0x48, 0x48, 0x30, 0x00 },   // (148)   0x94
    { 0x60, 0x00, 0x30, 0x48, 0x48, 0x48, 0x30, 0x00 },   // (149)   0x95
    { 0x38, 0x00, 0x48, 0x48, 0x48, 0x58, 0x28, 0x00 },   // (150)   0x96
    { 0x60, 0x00, 0x48, 0x48, 0x48, 0x58, 0x28, 0x00 },   // (151)   0x97
    { 0x28, 0x00, 0x48, 0x48, 0x48, 0x38, 0x10, 0x60 },   // (152)   0x98
    { 0x48, 0x30, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00 },   // (153)   0x99
    { 0x28, 0x00, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00 },   // (154)   0x9A
    { 0x00, 0x00, 0x04, 0x38, 0x58, 0x68, 0x70, 0x80 },   // (155)   0x9B
    { 0x18, 0x24, 0x20, 0x78, 0x20, 0x24, 0x7C, 0x00 },   // (156)   0x9C
    { 0x3C, 0x4C, 0x54, 0x54, 0x54, 0x64, 0x78, 0x00 },   // (157)   0x9D
    { 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00 },   // (158)   0x9E
    { 0x08, 0x14, 0x10, 0x38, 0x10, 0x10, 0x50, 0x20 },   // (159)   0x9F
    { 0x18, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00 },   // (160)   0xA0
    { 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 },   // (161)   0xA1
    { 0x00, 0x0C, 0x3E, 0x6C, 0x3E, 0x0C, 0x00, 0x00 },   // (162)   0xA2
    { 0x1C, 0x36, 0x30, 0x78, 0x30, 0x30, 0x7E, 0x00 },   // (163)   0xA3
    { 0x42, 0x3C, 0x66, 0x3C, 0x42, 0x00, 0x00, 0x00 },   // (164)   0xA4
    { 0xC3, 0x66, 0x3C, 0x18, 0x3C, 0x18, 0x18, 0x00 },   // (165)   0xA5
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 },   // (166)   0xA6
    { 0x3C, 0x60, 0x3C, 0x66, 0x3C, 0x06, 0x3C, 0x00 },   // (167)   0xA7
    { 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (168)   0xA8
    { 0x7C, 0x82, 0x9A, 0xA2, 0xA2, 0x9A, 0x82, 0x7C },   // (169)   0xA9
    { 0x1C, 0x24, 0x44, 0x3C, 0x00, 0x7E, 0x00, 0x00 },   // (170)   0xAA
    { 0x00, 0x33, 0x66, 0xCC, 0x66, 0x33, 0x00, 0x00 },   // (171)   0xAB
    { 0x3E, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (172)   0xAC
    { 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00 },   // (173)   0xAD
    { 0x7E, 0x81, 0xB9, 0xA5, 0xB9, 0xA5, 0x81, 0x7E },   // (174)   0xAE
    { 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (175)   0xAF
    { 0x3C, 0x66, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (176)   0xB0
    { 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x7E, 0x00 },   // (177)   0xB1
    { 0x78, 0x0C, 0x18, 0x30, 0x7C, 0x00, 0x00, 0x00 },   // (178)   0xB2
    { 0x78, 0x0C, 0x18, 0x0C, 0x78, 0x00, 0x00, 0x00 },   // (179)   0xB3
    { 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (180)   0xB4
    { 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x7F, 0x40 },   // (181)   0xB5
    { 0x3E, 0x7A, 0x7A, 0x3A, 0x0A, 0x0A, 0x0A, 0x00 },   // (182)   0xB6
    { 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00 },   // (183)   0xB7
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x30 },   // (184)   0xB8
    { 0x30, 0x70, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00 },   // (185)   0xB9
    { 0x38, 0x44, 0x44, 0x38, 0x00, 0x7C, 0x00, 0x00 },   // (186)   0xBA
    { 0x00, 0xCC, 0x66, 0x33, 0x66, 0xCC, 0x00, 0x00 },   // (187)   0xBB
    { 0x40, 0xC6, 0x4C, 0x58, 0x32, 0x66, 0xCF, 0x02 },   // (188)   0xBC
    { 0x40, 0xC6, 0x4C, 0x58, 0x3E, 0x62, 0xC4, 0x0E },   // (189)   0xBD
    { 0xC0, 0x23, 0x66, 0x2C, 0xD9, 0x33, 0x67, 0x01 },   // (190)   0xBE
    { 0x00, 0xE8, 0x0C, 0x7E, 0x7E, 0x0C, 0xE8, 0x00 },   // (191)   0xBF
    { 0xD8, 0xDC, 0xDE, 0xDF, 0xDF, 0xDE, 0xDC, 0xD8 },   // (192)   0xC0
    { 0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x00, 0xFF, 0xFF },   // (193)   0xC1
    { 0xC3, 0xC7, 0xCF, 0xDF, 0xDF, 0xCF, 0xC7, 0xC3 },   // (194)   0xC2
    { 0xC3, 0xE3, 0xF3, 0xFB, 0xFB, 0xF3, 0xE3, 0xC3 },   // (195)   0xC3
    { 0x18, 0x38, 0x78, 0xF8, 0xF8, 0x78, 0x38, 0x18 },   // (196)   0xC4
    { 0x18, 0x1C, 0x1E, 0x1F, 0x1F, 0x1E, 0x1C, 0x18 },   // (197)   0xC5
    { 0x14, 0x28, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00 },   // (198)   0xC6
    { 0x14, 0x28, 0x10, 0x28, 0x44, 0x7C, 0x44, 0x00 },   // (199)   0xC7
    { 0x00, 0x7E, 0x81, 0x91, 0xB1, 0x7E, 0x30, 0x10 },   // (200)   0xC8
    { 0xF4, 0x06, 0xFF, 0x06, 0xF4, 0x00, 0xFF, 0x00 },   // (201)   0xC9
    { 0x4F, 0xE0, 0x42, 0x5A, 0x42, 0x52, 0x07, 0xF2 },   // (202)   0xCA
    { 0x7E, 0x81, 0x95, 0x89, 0x95, 0x81, 0x7E, 0x00 },   // (203)   0xCB
    { 0x66, 0x9F, 0xBF, 0xFF, 0x7E, 0x3C, 0x18, 0x00 },   // (204)   0xCC
    { 0x00, 0x02, 0x05, 0x8A, 0x54, 0xA8, 0x50, 0x20 },   // (205)   0xCD
    { 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF },   // (206)   0xCE
    { 0xFF, 0x81, 0x81, 0x85, 0xA9, 0x91, 0x81, 0xFF },   // (207)   0xCF
    { 0x00, 0x30, 0x20, 0x38, 0x48, 0x48, 0x30, 0x00 },   // (208)   0xD0
    { 0x38, 0x24, 0x24, 0x74, 0x24, 0x24, 0x38, 0x00 },   // (209)   0xD1
    { 0x38, 0x00, 0x7C, 0x40, 0x78, 0x40, 0x7C, 0x00 },   // (210)   0xD2
    { 0x28, 0x00, 0x7C, 0x40, 0x78, 0x40, 0x7C, 0x00 },   // (211)   0xD3
    { 0x30, 0x00, 0x7C, 0x40, 0x78, 0x40, 0x7C, 0x00 },   // (212)   0xD4
    { 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (213)   0xD5
    { 0x18, 0x00, 0x38, 0x10, 0x10, 0x10, 0x38, 0x00 },   // (214)   0xD6
    { 0x38, 0x00, 0x38, 0x10, 0x10, 0x10, 0x38, 0x00 },   // (215)   0xD7
    { 0x28, 0x00, 0x38, 0x10, 0x10, 0x10, 0x38, 0x00 },   // (216)   0xD8
    { 0x10, 0x10, 0x10, 0xF0, 0x00, 0x00, 0x00, 0x00 },   // (217)   0xD9
    { 0x00, 0x00, 0x00, 0x1C, 0x10, 0x10, 0x10, 0x10 },   // (218)   0xDA
    { 0x00, 0x78, 0x48, 0x48, 0x48, 0x48, 0x78, 0x00 },   // (219)   0xDB
    { 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF },   // (220)   0xDC
    { 0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x00 },   // (221)   0xDD
    { 0xFE, 0xFE, 0xFE, 0xFE, 0x00, 0x00, 0x00, 0x00 },   // (222)   0xDE
    { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00 },   // (223)   0xDF
    { 0x18, 0x30, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00 },   // (224)   0xE0
    { 0x00, 0x78, 0x44, 0x78, 0x44, 0x44, 0x78, 0x40 },   // (225)   0xE1
    { 0x30, 0x48, 0x00, 0x30, 0x48, 0x48, 0x30, 0x00 },   // (226)   0xE2
    { 0x60, 0x30, 0x00, 0x30, 0x48, 0x48, 0x30, 0x00 },   // (227)   0xE3
    { 0x28, 0x50, 0x00, 0x30, 0x48, 0x48, 0x30, 0x00 },   // (228)   0xE4
    { 0x28, 0x50, 0x00, 0x30, 0x48, 0x48, 0x30, 0x00 },   // (229)   0xE5
    { 0x00, 0x00, 0x48, 0x48, 0x48, 0x70, 0x40, 0x40 },   // (230)   0xE6
    { 0x00, 0x60, 0x40, 0x70, 0x48, 0x70, 0x40, 0x60 },   // (231)   0xE7
    { 0x60, 0x40, 0x70, 0x48, 0x48, 0x70, 0x40, 0x60 },   // (232)   0xE8
    { 0x18, 0x00, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00 },   // (233)   0xE9
    { 0x38, 0x00, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00 },   // (234)   0xEA
    { 0x60, 0x00, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00 },   // (235)   0xEB
    { 0x18, 0x00, 0x48, 0x48, 0x48, 0x38, 0x10, 0x60 },   // (236)   0xEC
    { 0x18, 0x00, 0x44, 0x28, 0x10, 0x10, 0x10, 0x00 },   // (237)   0xED
    { 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (238)   0xEE
    { 0x30, 0x30, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 },   // (239)   0xEF
    { 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00 },   // (240)   0xF0
    { 0x00, 0x10, 0x38, 0x10, 0x00, 0x38, 0x00, 0x00 },   // (241)   0xF1
    { 0x00, 0x00, 0x7C, 0x00, 0x00, 0x7C, 0x00, 0x00 },   // (242)   0xF2
    { 0xC0, 0x68, 0xD0, 0x2C, 0x54, 0x1C, 0x04, 0x00 },   // (243)   0xF3
    { 0x3C, 0x54, 0x54, 0x34, 0x14, 0x14, 0x14, 0x00 },   // (244)   0xF4
    { 0x38, 0x44, 0x30, 0x28, 0x18, 0x44, 0x38, 0x00 },   // (245)   0xF5
    { 0x00, 0x10, 0x00, 0x7C, 0x00, 0x10, 0x00, 0x00 },   // (246)   0xF6
    { 0x00, 0x00, 0x00, 0x38, 0x18, 0x00, 0x00, 0x00 },   // (247)   0xF7
    { 0x30, 0x48, 0x48, 0x30, 0x00, 0x00, 0x00, 0x00 },   // (248)   0xF8
    { 0xFF, 0x08, 0x08, 0x08, 0xFF, 0x80, 0x80, 0x80 },   // (249)   0xF9
    { 0x10, 0x10, 0x10, 0xA1, 0x42, 0x42, 0x3C, 0x20 },   // (250)   0xFA
    { 0xC3, 0xE7, 0x7E, 0x3C, 0x3C, 0x7E, 0xE7, 0xC3 },   // (251)   0xFB
    { 0xF7, 0xF7, 0xF7, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF },   // (252)   0xFC
    { 0x2C, 0x2E, 0xEF, 0x1F, 0xFF, 0xFF, 0xFD, 0xFF },   // (253)   0xFD
    { 0x00, 0x00, 0x70, 0x70, 0x70, 0x00, 0x00, 0x00 },   // (254)   0xFE
    { 0x00, 0x54, 0x54, 0x54, 0x92, 0x92, 0x11, 0x00 }   // (255)   0xFF
};


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gfx.h"

/* keep your existing clut[] here */
/* keep your existing bayer4x4[][] here */
/* keep your existing font[][] here */

uint8_t fb[SCREEN_W * SCREEN_H] = {0};
uint32_t pb[SCREEN_W * SCREEN_H] = {0xFF0000FF};

static uint32_t g_ditherSeed = 0x12345678;

static inline void putPixelFast(int32_t x, int32_t y, uint8_t colIndex)
{
    fb[(y * SCREEN_W) + x] = colIndex;
}

static inline uint8_t *fbRowPtr(int y)
{
    return &fb[y * SCREEN_W];
}

static inline int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ========================================================================= */
/* basic framebuffer ops                                                     */
/* ========================================================================= */

void videoMemToScreen(void)
{
    const uint8_t *src = fb;
    uint32_t *dst = pb;
    const uint32_t *lut = clut;

    for (int i = 0; i < (SCREEN_W * SCREEN_H); i++) {
        dst[i] = lut[src[i]];
    }
}

void clearScreen(uint8_t colIndex)
{
    memset(fb, colIndex, sizeof(fb));
}

void putPixel(int32_t x, int32_t y, uint8_t colIndex)
{
    if ((unsigned)x >= SCREEN_W) return;
    if ((unsigned)y >= SCREEN_H) return;
    fb[(y * SCREEN_W) + x] = colIndex;
}

void drawLine(int x0, int y0, int x1, int y1, uint8_t colorIndex)
{
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        if ((unsigned)x0 < SCREEN_W && (unsigned)y0 < SCREEN_H) {
            fb[(y0 * SCREEN_W) + x0] = colorIndex;
        }

        if (x0 == x1 && y0 == y1) {
            break;
        }

        const int e2 = err << 1;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* ========================================================================= */
/* dither helpers                                                            */
/* ========================================================================= */

static int hashNoise4bit(int x, int y)
{
    uint32_t n = (uint32_t)x;
    n *= 0x1f123bb5u;
    n += (uint32_t)y * 0x159a55e5u;
    n ^= n >> 15;
    n *= 0x85ebca6bu;
    n ^= n >> 13;
    n *= 0xc2b2ae35u;
    n ^= n >> 16;

    return (int)(n & 15u);
}

void resetRand(void)
{
    g_ditherSeed = 0x34188195;
}

static uint32_t fastRand(void){
    g_ditherSeed = (g_ditherSeed * 1664525u) + 1013904223u;
    return g_ditherSeed;
}

uint8_t shadeColor(uint8_t baseColor, int shade)
{
    if (shade < 0) shade = 0;
    if (shade >= MAX_PALETTE_SHADE_COUNT) return BLACK_SHADE_PALETTE;

    return (uint8_t)(PALETTE_SHADE_OFFSETS + (baseColor & 15) + (shade * 16));
}

static uint8_t ditherShadeColor(uint8_t baseColor, float shadeF, int x, int y, DitherMode mode)
{
    if (shadeF < 0.0f) shadeF = 0.0f;
    if (shadeF > (float)MAX_PALETTE_SHADE_INDEX) shadeF = (float)MAX_PALETTE_SHADE_INDEX;

    int s0 = (int)shadeF;
    int s1 = s0 + 1;
    if (s1 > (int)MAX_PALETTE_SHADE_COUNT) s1 = (int)MAX_PALETTE_SHADE_COUNT;

    const float frac = shadeF - (float)s0;

    int threshold;
    if (mode == DITHER_RANDOM) {
        threshold = hashNoise4bit(x, y);
    } else {
        threshold = bayer4x4[y & 3][x & 3];
    }

    return ((frac * 16.0f) > (float)threshold)
        ? shadeColor(baseColor, s1)
        : shadeColor(baseColor, s0);
}

/* ========================================================================= */
/* triangle helpers                                                          */
/* ========================================================================= */

static inline void triBounds(
    int x0, int y0, int x1, int y1, int x2, int y2,
    int *minX, int *minY, int *maxX, int *maxY)
{
    int mnx = x0;
    int mny = y0;
    int mxx = x0;
    int mxy = y0;

    if (x1 < mnx) mnx = x1;
    if (x2 < mnx) mnx = x2;
    if (y1 < mny) mny = y1;
    if (y2 < mny) mny = y2;

    if (x1 > mxx) mxx = x1;
    if (x2 > mxx) mxx = x2;
    if (y1 > mxy) mxy = y1;
    if (y2 > mxy) mxy = y2;

    *minX = mnx;
    *minY = mny;
    *maxX = mxx;
    *maxY = mxy;
}

static inline int triAreaInt(int x0, int y0, int x1, int y1, int x2, int y2)
{
    return ((x1 - x0) * (y2 - y0)) - ((y1 - y0) * (x2 - x0));
}

/* ========================================================================= */
/* z+dither banded fill                                                      */
/* ========================================================================= */

void fillTriangleDitherZBandBayer(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint16_t z0_in,
    uint16_t z1_in,
    uint16_t z2_in,
    float camz0,
    float camz1,
    float camz2,
    uint8_t baseColor,
    float shadeF,
    int bandY0,
    int bandY1
)
{
    int minX, minY, maxX, maxY;
    triBounds(x0, y0, x1, y1, x2, y2, &minX, &minY, &maxX, &maxY);

    if (maxY < bandY0 || minY > bandY1) return;
    if (maxX < 0 || minX >= SCREEN_W) return;

    minX = clampi(minX, 0, SCREEN_W - 1);
    maxX = clampi(maxX, 0, SCREEN_W - 1);
    minY = clampi(minY, bandY0, bandY1);
    maxY = clampi(maxY, bandY0, bandY1);

    if (shadeF < 0.0f) shadeF = 0.0f;
    if (shadeF > (float)MAX_PALETTE_SHADE_INDEX) shadeF = (float)MAX_PALETTE_SHADE_INDEX;

    int s0 = (int)shadeF;
    int s1 = s0 + 1;
    if (s1 > (int)MAX_PALETTE_SHADE_COUNT) s1 = (int)MAX_PALETTE_SHADE_COUNT;

    const float frac = shadeF - (float)s0;
    int threshold16 = (int)(frac * 16.0f);
    if (threshold16 < 0) threshold16 = 0;
    if (threshold16 > 15) threshold16 = 15;

    const uint8_t col0 = shadeColor(baseColor, s0);
    const uint8_t col1 = shadeColor(baseColor, s1);
    const int solidFill = (col0 == col1) || (threshold16 <= 0);

    const float fx0 = (float)x0;
    const float fy0 = (float)y0;
    const float fx1 = (float)x1;
    const float fy1 = (float)y1;
    const float fx2 = (float)x2;
    const float fy2 = (float)y2;

    const float area = ((fx1 - fx0) * (fy2 - fy0)) - ((fy1 - fy0) * (fx2 - fx0));
    if (fabsf(area) < 0.0001f) return;

    if (camz0 <= 0.0001f || camz1 <= 0.0001f || camz2 <= 0.0001f) return;

    const float invArea = 1.0f / area;
    const int areaPositive = (area > 0.0f);

    const float q0 = 1.0f / camz0;
    const float q1 = 1.0f / camz1;
    const float q2 = 1.0f / camz2;

    const float zq0 = (float)z0_in * q0;
    const float zq1 = (float)z1_in * q1;
    const float zq2 = (float)z2_in * q2;

    const float A0 = fy1 - fy2;
    const float B0 = fx2 - fx1;
    const float C0 = (fx1 * fy2) - (fy1 * fx2);

    const float A1 = fy2 - fy0;
    const float B1 = fx0 - fx2;
    const float C1 = (fx2 * fy0) - (fy2 * fx0);

    const float A2 = fy0 - fy1;
    const float B2 = fx1 - fx0;
    const float C2 = (fx0 * fy1) - (fy0 * fx1);

    const float qStepX =
        (A0 * q0 + A1 * q1 + A2 * q2) * invArea;
    const float qStepY =
        (B0 * q0 + B1 * q1 + B2 * q2) * invArea;

    const float zqStepX =
        (A0 * zq0 + A1 * zq1 + A2 * zq2) * invArea;
    const float zqStepY =
        (B0 * zq0 + B1 * zq1 + B2 * zq2) * invArea;

    const float startPx = (float)minX + 0.5f;
    const float startPy = (float)minY + 0.5f;

    float rowW0 = (A0 * startPx) + (B0 * startPy) + C0;
    float rowW1 = (A1 * startPx) + (B1 * startPy) + C1;
    float rowW2 = (A2 * startPx) + (B2 * startPy) + C2;

    float rowQ =
        ((rowW0 * q0) + (rowW1 * q1) + (rowW2 * q2)) * invArea;

    float rowZQ =
        ((rowW0 * zq0) + (rowW1 * zq1) + (rowW2 * zq2)) * invArea;

    const int width = (maxX - minX) + 1;

    for (int y = minY; y <= maxY; y++) {
        const int localY = y - bandY0;
        uint16_t *zb = &g_depthBufferBand[(localY * SCREEN_W) + minX];
        uint8_t  *dst = &fb[(y * SCREEN_W) + minX];

        const uint8_t *brow = bayer4x4[y & 3];
        const uint8_t rowCols[4] = {
            (uint8_t)((brow[0] < threshold16) ? col1 : col0),
            (uint8_t)((brow[1] < threshold16) ? col1 : col0),
            (uint8_t)((brow[2] < threshold16) ? col1 : col0),
            (uint8_t)((brow[3] < threshold16) ? col1 : col0)
        };

        float w0 = rowW0;
        float w1 = rowW1;
        float w2 = rowW2;
        float q  = rowQ;
        float zq = rowZQ;

        if (areaPositive) {
            for (int i = 0; i < width; i++) {
                if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f && q > 0.0f) {
                    float z = zq / q;
                    if (z < 0.0f) z = 0.0f;
                    if (z > 65535.0f) z = 65535.0f;

                    const uint16_t zi = (uint16_t)(z + 0.5f);
                    if (zi < zb[i]) {
                        zb[i] = zi;
                        dst[i] = solidFill ? col0 : rowCols[(minX + i) & 3];
                    }
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
                q  += qStepX;
                zq += zqStepX;
            }
        } else {
            for (int i = 0; i < width; i++) {
                if (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f && q > 0.0f) {
                    float z = zq / q;
                    if (z < 0.0f) z = 0.0f;
                    if (z > 65535.0f) z = 65535.0f;

                    const uint16_t zi = (uint16_t)(z + 0.5f);
                    if (zi < zb[i]) {
                        zb[i] = zi;
                        dst[i] = solidFill ? col0 : rowCols[(minX + i) & 3];
                    }
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
                q  += qStepX;
                zq += zqStepX;
            }
        }

        rowW0 += B0;
        rowW1 += B1;
        rowW2 += B2;
        rowQ  += qStepY;
        rowZQ += zqStepY;
    }
}

void fillTriangleZBandFlat(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint16_t z0_in,
    uint16_t z1_in,
    uint16_t z2_in,
    float camz0,
    float camz1,
    float camz2,
    uint8_t baseColor,
    float shadeF,
    int bandY0,
    int bandY1
)
{
    int minX, minY, maxX, maxY;
    triBounds(x0, y0, x1, y1, x2, y2, &minX, &minY, &maxX, &maxY);

    if (maxY < bandY0 || minY > bandY1) return;
    if (maxX < 0 || minX >= SCREEN_W) return;

    minX = clampi(minX, 0, SCREEN_W - 1);
    maxX = clampi(maxX, 0, SCREEN_W - 1);
    minY = clampi(minY, bandY0, bandY1);
    maxY = clampi(maxY, bandY0, bandY1);

    int shade = (int)(shadeF + 0.5f);
    if (shade < 0) shade = 0;
    if (shade > (int)MAX_PALETTE_SHADE_INDEX) shade = (int)MAX_PALETTE_SHADE_INDEX;
    const uint8_t col = shadeColor(baseColor, shade);

    const float fx0 = (float)x0;
    const float fy0 = (float)y0;
    const float fx1 = (float)x1;
    const float fy1 = (float)y1;
    const float fx2 = (float)x2;
    const float fy2 = (float)y2;

    const float area = ((fx1 - fx0) * (fy2 - fy0)) - ((fy1 - fy0) * (fx2 - fx0));
    if (fabsf(area) < 0.0001f) return;

    if (camz0 <= 0.0001f || camz1 <= 0.0001f || camz2 <= 0.0001f) return;

    const float invArea = 1.0f / area;
    const int areaPositive = (area > 0.0f);

    const float q0 = 1.0f / camz0;
    const float q1 = 1.0f / camz1;
    const float q2 = 1.0f / camz2;

    const float zq0 = (float)z0_in * q0;
    const float zq1 = (float)z1_in * q1;
    const float zq2 = (float)z2_in * q2;

    const float A0 = fy1 - fy2;
    const float B0 = fx2 - fx1;
    const float C0 = (fx1 * fy2) - (fy1 * fx2);

    const float A1 = fy2 - fy0;
    const float B1 = fx0 - fx2;
    const float C1 = (fx2 * fy0) - (fy2 * fx0);

    const float A2 = fy0 - fy1;
    const float B2 = fx1 - fx0;
    const float C2 = (fx0 * fy1) - (fy0 * fx1);

    const float qStepX =  (A0 * q0 + A1 * q1 + A2 * q2) * invArea;
    const float qStepY =  (B0 * q0 + B1 * q1 + B2 * q2) * invArea;
    const float zqStepX = (A0 * zq0 + A1 * zq1 + A2 * zq2) * invArea;
    const float zqStepY = (B0 * zq0 + B1 * zq1 + B2 * zq2) * invArea;

    const float startPx = (float)minX + 0.5f;
    const float startPy = (float)minY + 0.5f;

    float rowW0 = (A0 * startPx) + (B0 * startPy) + C0;
    float rowW1 = (A1 * startPx) + (B1 * startPy) + C1;
    float rowW2 = (A2 * startPx) + (B2 * startPy) + C2;

    float rowQ =  ((rowW0 * q0) + (rowW1 * q1) + (rowW2 * q2)) * invArea;
    float rowZQ = ((rowW0 * zq0) + (rowW1 * zq1) + (rowW2 * zq2)) * invArea;

    const int width = (maxX - minX) + 1;

    for (int y = minY; y <= maxY; y++) {
        const int localY = y - bandY0;
        uint16_t *zb = &g_depthBufferBand[(localY * SCREEN_W) + minX];
        uint8_t  *dst = &fb[(y * SCREEN_W) + minX];

        float w0 = rowW0;
        float w1 = rowW1;
        float w2 = rowW2;
        float q  = rowQ;
        float zq = rowZQ;

        if (areaPositive) {
            for (int i = 0; i < width; i++) {
                if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f && q > 0.0f) {
                    float z = zq / q;
                    if (z < 0.0f) z = 0.0f;
                    if (z > 65535.0f) z = 65535.0f;

                    const uint16_t zi = (uint16_t)(z + 0.5f);
                    if (zi < zb[i]) {
                        zb[i] = zi;
                        dst[i] = col;
                    }
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
                q  += qStepX;
                zq += zqStepX;
            }
        } else {
            for (int i = 0; i < width; i++) {
                if (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f && q > 0.0f) {
                    float z = zq / q;
                    if (z < 0.0f) z = 0.0f;
                    if (z > 65535.0f) z = 65535.0f;

                    const uint16_t zi = (uint16_t)(z + 0.5f);
                    if (zi < zb[i]) {
                        zb[i] = zi;
                        dst[i] = col;
                    }
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
                q  += qStepX;
                zq += zqStepX;
            }
        }

        rowW0 += B0;
        rowW1 += B1;
        rowW2 += B2;
        rowQ  += qStepY;
        rowZQ += zqStepY;
    }
}

void fillTriangleDither2Mode(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint8_t baseColor,
    float shadeF,
    DitherMode mode
)
{
    int minX, minY, maxX, maxY;
    triBounds(x0, y0, x1, y1, x2, y2, &minX, &minY, &maxX, &maxY);

    minX = clampi(minX, 0, SCREEN_W - 1);
    minY = clampi(minY, 0, SCREEN_H - 1);
    maxX = clampi(maxX, 0, SCREEN_W - 1);
    maxY = clampi(maxY, 0, SCREEN_H - 1);

    const int area = triAreaInt(x0, y0, x1, y1, x2, y2);
    if (area == 0) return;

    if (shadeF < 0.0f) shadeF = 0.0f;
    if (shadeF > (float)MAX_PALETTE_SHADE_INDEX) shadeF = (float)MAX_PALETTE_SHADE_INDEX;

    const uint8_t col0 = baseColor;
    const uint8_t col1 = BLACK_SHADE_PALETTE;

    int threshold16 = (int)((shadeF / MAX_PALETTE_SHADE_COUNT) * 16.0f);
    if (threshold16 < 0) threshold16 = 0;
    if (threshold16 > 16) threshold16 = 16;

    if (threshold16 <= 0) {
        fillTriangle(x0, y0, x1, y1, x2, y2, col0);
        return;
    }

    if (threshold16 >= 16) {
        fillTriangle(x0, y0, x1, y1, x2, y2, col1);
        return;
    }

    const int A0 = y1 - y2;
    const int B0 = x2 - x1;
    const int C0 = (x1 * y2) - (y1 * x2);

    const int A1 = y2 - y0;
    const int B1 = x0 - x2;
    const int C1 = (x2 * y0) - (y2 * x0);

    const int A2 = y0 - y1;
    const int B2 = x1 - x0;
    const int C2 = (x0 * y1) - (y0 * x1);

    int rowW0 = (A0 * minX) + (B0 * minY) + C0;
    int rowW1 = (A1 * minX) + (B1 * minY) + C1;
    int rowW2 = (A2 * minX) + (B2 * minY) + C2;

    const int width = (maxX - minX) + 1;
    const int areaPositive = (area > 0);

    for (int y = minY; y <= maxY; y++) {
        uint8_t *dst = &fb[(y * SCREEN_W) + minX];

        int w0 = rowW0;
        int w1 = rowW1;
        int w2 = rowW2;

        if (mode == DITHER_RANDOM) {
            for (int i = 0; i < width; i++) {
                if ((areaPositive && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                    (!areaPositive && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                    const int x = minX + i;
                    const int threshold = hashNoise4bit(x, y);
                    dst[i] = (threshold < threshold16) ? col1 : col0;
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
            }
        } else {
            const uint8_t *brow = bayer4x4[y & 3];
            const uint8_t rowCols[4] = {
                (uint8_t)((brow[0] < threshold16) ? col1 : col0),
                (uint8_t)((brow[1] < threshold16) ? col1 : col0),
                (uint8_t)((brow[2] < threshold16) ? col1 : col0),
                (uint8_t)((brow[3] < threshold16) ? col1 : col0)
            };

            for (int i = 0; i < width; i++) {
                if ((areaPositive && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                    (!areaPositive && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                    dst[i] = rowCols[(minX + i) & 3];
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
            }
        }

        rowW0 += B0;
        rowW1 += B1;
        rowW2 += B2;
    }
}

void fillTriangleDitherZBandBayer2Mode(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint16_t z0_in,
    uint16_t z1_in,
    uint16_t z2_in,
    float camz0,
    float camz1,
    float camz2,
    uint8_t baseColor,
    float shadeF,
    int bandY0,
    int bandY1
)
{
    int minX, minY, maxX, maxY;
    triBounds(x0, y0, x1, y1, x2, y2, &minX, &minY, &maxX, &maxY);

    if (maxY < bandY0 || minY > bandY1) return;
    if (maxX < 0 || minX >= SCREEN_W) return;

    minX = clampi(minX, 0, SCREEN_W - 1);
    maxX = clampi(maxX, 0, SCREEN_W - 1);
    minY = clampi(minY, bandY0, bandY1);
    maxY = clampi(maxY, bandY0, bandY1);

    if (shadeF < 0.0f) shadeF = 0.0f;
    if (shadeF > (float)MAX_PALETTE_SHADE_INDEX) shadeF = (float)MAX_PALETTE_SHADE_INDEX;

    const uint8_t col0 = baseColor;
    const uint8_t col1 = BLACK_SHADE_PALETTE;

    int threshold16 = (int)((shadeF / MAX_PALETTE_SHADE_COUNT) * 16.0f);
    if (threshold16 < 0) threshold16 = 0;
    if (threshold16 > 16) threshold16 = 16;

    const int solidBase  = (threshold16 <= 0);
    const int solidBlack = (threshold16 >= 16);

    const float fx0 = (float)x0;
    const float fy0 = (float)y0;
    const float fx1 = (float)x1;
    const float fy1 = (float)y1;
    const float fx2 = (float)x2;
    const float fy2 = (float)y2;

    const float area = ((fx1 - fx0) * (fy2 - fy0)) - ((fy1 - fy0) * (fx2 - fx0));
    if (fabsf(area) < 0.0001f) return;

    if (camz0 <= 0.0001f || camz1 <= 0.0001f || camz2 <= 0.0001f) return;

    const float invArea = 1.0f / area;
    const int areaPositive = (area > 0.0f);

    const float q0 = 1.0f / camz0;
    const float q1 = 1.0f / camz1;
    const float q2 = 1.0f / camz2;

    const float zq0 = (float)z0_in * q0;
    const float zq1 = (float)z1_in * q1;
    const float zq2 = (float)z2_in * q2;

    const float A0 = fy1 - fy2;
    const float B0 = fx2 - fx1;
    const float C0 = (fx1 * fy2) - (fy1 * fx2);

    const float A1 = fy2 - fy0;
    const float B1 = fx0 - fx2;
    const float C1 = (fx2 * fy0) - (fy2 * fx0);

    const float A2 = fy0 - fy1;
    const float B2 = fx1 - fx0;
    const float C2 = (fx0 * fy1) - (fy0 * fx1);

    const float qStepX =
        (A0 * q0 + A1 * q1 + A2 * q2) * invArea;
    const float qStepY =
        (B0 * q0 + B1 * q1 + B2 * q2) * invArea;

    const float zqStepX =
        (A0 * zq0 + A1 * zq1 + A2 * zq2) * invArea;
    const float zqStepY =
        (B0 * zq0 + B1 * zq1 + B2 * zq2) * invArea;

    const float startPx = (float)minX + 0.5f;
    const float startPy = (float)minY + 0.5f;

    float rowW0 = (A0 * startPx) + (B0 * startPy) + C0;
    float rowW1 = (A1 * startPx) + (B1 * startPy) + C1;
    float rowW2 = (A2 * startPx) + (B2 * startPy) + C2;

    float rowQ =
        ((rowW0 * q0) + (rowW1 * q1) + (rowW2 * q2)) * invArea;

    float rowZQ =
        ((rowW0 * zq0) + (rowW1 * zq1) + (rowW2 * zq2)) * invArea;

    const int width = (maxX - minX) + 1;

    for (int y = minY; y <= maxY; y++) {
        const int localY = y - bandY0;
        uint16_t *zb = &g_depthBufferBand[(localY * SCREEN_W) + minX];
        uint8_t  *dst = &fb[(y * SCREEN_W) + minX];

        const uint8_t *brow = bayer4x4[y & 3];
        const uint8_t rowCols[4] = {
            (uint8_t)((brow[0] < threshold16) ? col1 : col0),
            (uint8_t)((brow[1] < threshold16) ? col1 : col0),
            (uint8_t)((brow[2] < threshold16) ? col1 : col0),
            (uint8_t)((brow[3] < threshold16) ? col1 : col0)
        };

        float w0 = rowW0;
        float w1 = rowW1;
        float w2 = rowW2;
        float q  = rowQ;
        float zq = rowZQ;

        if (areaPositive) {
            for (int i = 0; i < width; i++) {
                if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f && q > 0.0f) {
                    float z = zq / q;
                    if (z < 0.0f) z = 0.0f;
                    if (z > 65535.0f) z = 65535.0f;

                    const uint16_t zi = (uint16_t)(z + 0.5f);
                    if (zi < zb[i]) {
                        zb[i] = zi;

                        if (solidBase)      dst[i] = col0;
                        else if (solidBlack) dst[i] = col1;
                        else                dst[i] = rowCols[(minX + i) & 3];
                    }
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
                q  += qStepX;
                zq += zqStepX;
            }
        } else {
            for (int i = 0; i < width; i++) {
                if (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f && q > 0.0f) {
                    float z = zq / q;
                    if (z < 0.0f) z = 0.0f;
                    if (z > 65535.0f) z = 65535.0f;

                    const uint16_t zi = (uint16_t)(z + 0.5f);
                    if (zi < zb[i]) {
                        zb[i] = zi;

                        if (solidBase)      dst[i] = col0;
                        else if (solidBlack) dst[i] = col1;
                        else                dst[i] = rowCols[(minX + i) & 3];
                    }
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
                q  += qStepX;
                zq += zqStepX;
            }
        }

        rowW0 += B0;
        rowW1 += B1;
        rowW2 += B2;
        rowQ  += qStepY;
        rowZQ += zqStepY;
    }
}

void fillTriangleDither(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t baseColor, float shadeF, DitherMode mode)
{
    int minX, minY, maxX, maxY;
    triBounds(x0, y0, x1, y1, x2, y2, &minX, &minY, &maxX, &maxY);

    minX = clampi(minX, 0, SCREEN_W - 1);
    minY = clampi(minY, 0, SCREEN_H - 1);
    maxX = clampi(maxX, 0, SCREEN_W - 1);
    maxY = clampi(maxY, 0, SCREEN_H - 1);

    const int area = triAreaInt(x0, y0, x1, y1, x2, y2);
    if (area == 0) return;

    const int A0 = y1 - y2;
    const int B0 = x2 - x1;
    const int C0 = (x1 * y2) - (y1 * x2);

    const int A1 = y2 - y0;
    const int B1 = x0 - x2;
    const int C1 = (x2 * y0) - (y2 * x0);

    const int A2 = y0 - y1;
    const int B2 = x1 - x0;
    const int C2 = (x0 * y1) - (y0 * x1);

    int rowW0 = (A0 * minX) + (B0 * minY) + C0;
    int rowW1 = (A1 * minX) + (B1 * minY) + C1;
    int rowW2 = (A2 * minX) + (B2 * minY) + C2;

    const int width = (maxX - minX) + 1;
    const int areaPositive = (area > 0);

    for (int y = minY; y <= maxY; y++) {
        uint8_t *dst = &fb[(y * SCREEN_W) + minX];

        int w0 = rowW0;
        int w1 = rowW1;
        int w2 = rowW2;

        if (mode == DITHER_RANDOM) {
            for (int i = 0; i < width; i++) {
                if ((areaPositive && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                    (!areaPositive && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                    dst[i] = ditherShadeColor(baseColor, shadeF, minX + i, y, mode);
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
            }
        } else {
            if (shadeF < 0.0f) shadeF = 0.0f;
            if (shadeF > (float)MAX_PALETTE_SHADE_INDEX) shadeF = (float)MAX_PALETTE_SHADE_INDEX;

            int s0 = (int)shadeF;
            int s1 = s0 + 1;
            if (s1 > (int)MAX_PALETTE_SHADE_COUNT) s1 = (int)MAX_PALETTE_SHADE_COUNT;

            const float frac = shadeF - (float)s0;
            int threshold16 = (int)(frac * 16.0f);
            if (threshold16 < 0) threshold16 = 0;
            if (threshold16 > 15) threshold16 = 15;

            const uint8_t col0 = shadeColor(baseColor, s0);
            const uint8_t col1 = shadeColor(baseColor, s1);

            const uint8_t *brow = bayer4x4[y & 3];
            const uint8_t rowCols[4] = {
                (uint8_t)((brow[0] < threshold16) ? col1 : col0),
                (uint8_t)((brow[1] < threshold16) ? col1 : col0),
                (uint8_t)((brow[2] < threshold16) ? col1 : col0),
                (uint8_t)((brow[3] < threshold16) ? col1 : col0)
            };

            for (int i = 0; i < width; i++) {
                if ((areaPositive && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                    (!areaPositive && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                    dst[i] = rowCols[(minX + i) & 3];
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
            }
        }

        rowW0 += B0;
        rowW1 += B1;
        rowW2 += B2;
    }
}

void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
{
    int minX, minY, maxX, maxY;
    triBounds(x0, y0, x1, y1, x2, y2, &minX, &minY, &maxX, &maxY);

    minX = clampi(minX, 0, SCREEN_W - 1);
    minY = clampi(minY, 0, SCREEN_H - 1);
    maxX = clampi(maxX, 0, SCREEN_W - 1);
    maxY = clampi(maxY, 0, SCREEN_H - 1);

    const int area = triAreaInt(x0, y0, x1, y1, x2, y2);
    if (area == 0) return;

    const int A0 = y1 - y2;
    const int B0 = x2 - x1;
    const int C0 = (x1 * y2) - (y1 * x2);

    const int A1 = y2 - y0;
    const int B1 = x0 - x2;
    const int C1 = (x2 * y0) - (y2 * x0);

    const int A2 = y0 - y1;
    const int B2 = x1 - x0;
    const int C2 = (x0 * y1) - (y0 * x1);

    int rowW0 = (A0 * minX) + (B0 * minY) + C0;
    int rowW1 = (A1 * minX) + (B1 * minY) + C1;
    int rowW2 = (A2 * minX) + (B2 * minY) + C2;

    const int width = (maxX - minX) + 1;
    const int areaPositive = (area > 0);

    for (int y = minY; y <= maxY; y++) {
        uint8_t *dst = &fb[(y * SCREEN_W) + minX];

        int w0 = rowW0;
        int w1 = rowW1;
        int w2 = rowW2;

        if (areaPositive) {
            for (int i = 0; i < width; i++) {
                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    dst[i] = color;
                }
                w0 += A0;
                w1 += A1;
                w2 += A2;
            }
        } else {
            for (int i = 0; i < width; i++) {
                if (w0 <= 0 && w1 <= 0 && w2 <= 0) {
                    dst[i] = color;
                }
                w0 += A0;
                w1 += A1;
                w2 += A2;
            }
        }

        rowW0 += B0;
        rowW1 += B1;
        rowW2 += B2;
    }
}

/* ========================================================================= */
/* misc drawing                                                              */
/* ========================================================================= */

uint32_t darken(uint32_t c, float f)
{
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8)  & 0xFF;
    uint8_t b = (c >> 0)  & 0xFF;

    r = (uint8_t)(r * f);
    g = (uint8_t)(g * f);
    b = (uint8_t)(b * f);

    return (0xFFu << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void drawRect(int x, int y, int w, int h, uint8_t col)
{
    if (w <= 0 || h <= 0) return;

    int x0 = x;
    int y0 = y;
    int x1 = x + w - 1;
    int y1 = y + h - 1;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
    if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;

    if (x0 > x1 || y0 > y1) return;

    const int fillW = (x1 - x0) + 1;

    for (int yy = y0; yy <= y1; yy++) {
        memset(&fb[(yy * SCREEN_W) + x0], col, fillW);
    }
}

void drawChar(int x, int y, char c, uint8_t color)
{
    if (x >= SCREEN_W || y >= SCREEN_H) return;
    if (x <= -8 || y <= -8) return;

    const uint8_t *glyph = font[(uint8_t)c];

    for (int row = 0; row < 8; row++) {
        const int py = y + row;
        if ((unsigned)py >= SCREEN_H) continue;

        const uint8_t bits = glyph[row];
        uint8_t *dst = &fb[(py * SCREEN_W)];

        for (int col = 0; col < 8; col++) {
            if (bits & (1u << (7 - col))) {
                const int px = x + col;
                if ((unsigned)px < SCREEN_W) {
                    dst[px] = color;
                }
            }
        }
    }
}

void drawText(int x, int y, const char *text, uint8_t color)
{
    int cx = x;
    int cy = y;

    while (*text) {
        if (*text == '\n') {
            cx = x;
            cy += 8;
        } else {
            drawChar(cx, cy, *text, color);
            cx += 8;
        }
        text++;
    }
}