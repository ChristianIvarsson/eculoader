#ifndef __MAPS_INTERNAL_H__
#define __MAPS_INTERNAL_H__


static const uint32_t mapMPC5566[] = {
    0x00004000, 0x00010000, 0x0001C000,             // Low space
    0x00020000, 0x00030000,
    0x00040000, 0x00060000,                         // Mid space
    0x00080000, 0x000A0000, 0x000C0000, 0x000E0000, // High space
    0x00100000, 0x00120000, 0x00140000, 0x00160000,
    0x00180000, 0x001A0000, 0x001C0000, 0x001E0000,
    0x00200000, 0x00220000, 0x00240000, 0x00260000,
    0x00280000, 0x002A0000, 0x002C0000, 0x002E0000,
    0x00300000, 0x00300400                          // End / Shadow data
};

#endif
