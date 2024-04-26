.global mdHash


# This implementation does a few things back-to-front due to the target being big ending.
# In particular buffer index is completely reversed:

# Regular byteswap operation
#  0  1  2  3     4  5  6  7 .. 60 61 62 63   < read
#  3  2  1  0     7  6  5  4 .. 63 62 61 60   < in buffer
# idx 0          idx 1       .. idx 15        < Which index the source data is at

# Reversed byteswap operation
#  0  1  2  3     4  5  6  7 .. 60 61 62 63   < read
# 63 62 61 60    59 58 57 56 ..  3  2  1  0   < in buffer
# idx 15         idx 14      .. idx 0         < Which index the source data is at

# Data count
.set dCnt    , 4

# Key data base pointer
.set dBase   , 5

# Accumulative registers
.set bckA    , 6
.set bckB    , 7
.set bckC    , 8
.set bckD    , 9

# Working registers
.set kA      , 12
.set kB      , 13
.set kC      , 14
.set kD      , 15

# Temporary registers
.set tmpX    , 10
.set tmpY    , 16
.set tmpDat  , 17
.set tmpCnt  , 20

# Data destination buffer pointer
.set bufPtr  , 18

# Data source pointer
.set srcPtr  , 19



# Special version of the read function to save some space

readMemmd:
    addi srcPtr,    srcPtr, 1 /* One MUST increment manually before reading to make sure ecc exceptions can be recovered */
    lbz     %r3, -1( srcPtr )
blr


# # # # # # # # # # # # # # # # # #
# Macros
.macro  _addHI     _A,     _HASH
    addis   \_A,      \_A,    ( (\_HASH >> 16) + ((\_HASH >> 15) & 1) )
.endm

# Bigger than 8000 since 8000 & 7fff == 0 && 8000 - 0 would overflow.
# You want that bit to trickle over into the addis operation and leave the bottom one as is
.macro  _addLO     _A,     _HASH
.if   ( ( \_HASH & 0xffff ) > 0x8000 )
    addi    \_A,      \_A,   -( 0x8000 - ( \_HASH & 0x7fff ) )
.else
    addi    \_A,      \_A,    ( \_HASH & 0x7fff )
.endif
.endm

# Pre unique op
.macro _PRE     _A,     _IDX,     _HASH
    lwz  tmpDat,  \_IDX*4( bufPtr ) 
    _addHI  \_A,   \_HASH
.endm

# Post unique op
.macro _POST     _A, _X,     _HASH, _ROT
    # A = A + data[ idx ] + K + F
    _addLO  \_A,   \_HASH
    add     \_A,      \_A,   tmpDat
    add     \_A,      \_A,     tmpX

    # A  = (A <<  n) + B
    rotlwi  \_A,      \_A,    \_ROT
    add     \_A,      \_X,      \_A
.endm

# FG hash
.macro _FG     _A, _B, _C, _D, _X,     _IDX,     _HASH,     _ROT

    _PRE    \_A,     \_IDX, \_HASH

    # F  = D ^ (B & (C ^ D))
    andc   tmpX,      \_D,      \_B
    and    tmpY,      \_C,      \_B
    or     tmpX,     tmpX,     tmpY

    _POST   \_A, \_X,     \_HASH, \_ROT
.endm

# H hash
.macro __H     _A, _B, _C, _D, _X,     _IDX,     _HASH,     _ROT

    _PRE    \_A,     \_IDX, \_HASH

    # F  = B ^ C ^ D
    xor    tmpX,      \_C,      \_D
    xor    tmpX,     tmpX,      \_B

    _POST   \_A, \_X,     \_HASH, \_ROT
.endm

# I hash
.macro __I     _A, _B, _C, _D, _X,     _IDX,     _HASH,     _ROT

    _PRE    \_A,     \_IDX, \_HASH

    # F  = C ^ (B | (~D))
    orc    tmpX,      \_B,      \_D
    xor    tmpX,     tmpX,      \_C

    _POST   \_A, \_X,     \_HASH, \_ROT
.endm


mdHash:
    stwu    %sp,        -164( %sp )
    mflr    %r0
    stw     %r0,         168( %sp )
    stmw   %r10,          76( %sp ) /* Backup 10 - 31 */

    lwz  srcPtr,   0( %r3 )         /* Address */
    lwz    dCnt,   4( %r3 )         /* Count   */
    lwz    bckA,   8( %r3 )         /* A       */
    lwz    bckB,  12( %r3 )         /* B       */
    lwz    bckC,  16( %r3 )         /* C       */
    lwz    bckD,  20( %r3 )         /* D       */
    addi  dBase,      %r3,        8 /* Offset dBase to point at keys */
    li   tmpCnt,                 64 /* Preload copy count            */

LoopOver:
    addi bufPtr,      %sp,       76 /* Buffer window + 64            */

# This is using a ugly trick to account for endianess differences AND speed things up
# Basically fill it from top to bottom and flip every buffer index in the rest of the code. (Index 0 is at 60 etc)
    mtctr   tmpCnt
copyMore:
    bl readMemmd
    stbu    %r3, -1( bufPtr )
    bdnz copyMore


    # Backing up A, B, C and D
    addi     kA,     bckA,        0
    addi     kB,     bckB,        0
    addi     kC,     bckC,        0
    addi     kD,     bckD,        0

    # # # # # STEP  0 - 15 # # # # #
    _FG   kA, kB, kC, kD,   kB,   15,    0xD76AA478,     7
    _FG   kD, kA, kB, kC,   kA,   14,    0xE8C7B756,    12
    _FG   kC, kD, kA, kB,   kD,   13,    0x242070DB,    17
    _FG   kB, kC, kD, kA,   kC,   12,    0xC1BDCEEE,    22
    _FG   kA, kB, kC, kD,   kB,   11,    0xF57C0FAF,     7
    _FG   kD, kA, kB, kC,   kA,   10,    0x4787C62A,    12
    _FG   kC, kD, kA, kB,   kD,    9,    0xA8304613,    17
    _FG   kB, kC, kD, kA,   kC,    8,    0xFD469501,    22
    _FG   kA, kB, kC, kD,   kB,    7,    0x698098D8,     7
    _FG   kD, kA, kB, kC,   kA,    6,    0x8B44F7AF,    12
    _FG   kC, kD, kA, kB,   kD,    5,    0xFFFF5BB1,    17
    _FG   kB, kC, kD, kA,   kC,    4,    0x895CD7BE,    22
    _FG   kA, kB, kC, kD,   kB,    3,    0x6B901122,     7
    _FG   kD, kA, kB, kC,   kA,    2,    0xFD987193,    12
    _FG   kC, kD, kA, kB,   kD,    1,    0xA679438E,    17
    _FG   kB, kC, kD, kA,   kC,    0,    0x49B40821,    22

    # # # # # STEP 16 - 31 # # # # #
    _FG   kA, kD, kB, kC,   kB,   14,    0xf61e2562,     5
    _FG   kD, kC, kA, kB,   kA,    9,    0xc040b340,     9
    _FG   kC, kB, kD, kA,   kD,    4,    0x265e5a51,    14
    _FG   kB, kA, kC, kD,   kC,   15,    0xe9b6c7aa,    20
    _FG   kA, kD, kB, kC,   kB,   10,    0xd62f105d,     5
    _FG   kD, kC, kA, kB,   kA,    5,    0x02441453,     9
    _FG   kC, kB, kD, kA,   kD,    0,    0xd8a1e681,    14
    _FG   kB, kA, kC, kD,   kC,   11,    0xe7d3fbc8,    20
    _FG   kA, kD, kB, kC,   kB,    6,    0x21e1cde6,     5
    _FG   kD, kC, kA, kB,   kA,    1,    0xc33707d6,     9
    _FG   kC, kB, kD, kA,   kD,   12,    0xf4d50d87,    14
    _FG   kB, kA, kC, kD,   kC,    7,    0x455a14ed,    20
    _FG   kA, kD, kB, kC,   kB,    2,    0xa9e3e905,     5
    _FG   kD, kC, kA, kB,   kA,   13,    0xfcefa3f8,     9
    _FG   kC, kB, kD, kA,   kD,    8,    0x676f02d9,    14
    _FG   kB, kA, kC, kD,   kC,    3,    0x8d2a4c8a,    20

    # # # # # STEP 32 - 47 # # # # #
    __H   kA, kB, kC, kD,   kB,   10,    0xfffa3942,     4
    __H   kD, kA, kB, kC,   kA,    7,    0x8771f681,    11
    __H   kC, kD, kA, kB,   kD,    4,    0x6d9d6122,    16
    __H   kB, kC, kD, kA,   kC,    1,    0xfde5380c,    23
    __H   kA, kB, kC, kD,   kB,   14,    0xa4beea44,     4
    __H   kD, kA, kB, kC,   kA,   11,    0x4bdecfa9,    11
    __H   kC, kD, kA, kB,   kD,    8,    0xf6bb4b60,    16
    __H   kB, kC, kD, kA,   kC,    5,    0xbebfbc70,    23
    __H   kA, kB, kC, kD,   kB,    2,    0x289b7ec6,     4
    __H   kD, kA, kB, kC,   kA,   15,    0xeaa127fa,    11
    __H   kC, kD, kA, kB,   kD,   12,    0xd4ef3085,    16
    __H   kB, kC, kD, kA,   kC,    9,    0x04881d05,    23
    __H   kA, kB, kC, kD,   kB,    6,    0xd9d4d039,     4
    __H   kD, kA, kB, kC,   kA,    3,    0xe6db99e5,    11
    __H   kC, kD, kA, kB,   kD,    0,    0x1fa27cf8,    16
    __H   kB, kC, kD, kA,   kC,   13,    0xc4ac5665,    23

        # # # # # STEP 48 - 63 # # # # #
    __I   kA, kB, kC, kD,   kB,   15,    0xf4292244,     6
    __I   kD, kA, kB, kC,   kA,    8,    0x432aff97,    10
    __I   kC, kD, kA, kB,   kD,    1,    0xab9423a7,    15
    __I   kB, kC, kD, kA,   kC,   10,    0xfc93a039,    21
    __I   kA, kB, kC, kD,   kB,    3,    0x655b59c3,     6
    __I   kD, kA, kB, kC,   kA,   12,    0x8f0ccc92,    10
    __I   kC, kD, kA, kB,   kD,    5,    0xffeff47d,    15
    __I   kB, kC, kD, kA,   kC,   14,    0x85845dd1,    21
    __I   kA, kB, kC, kD,   kB,    7,    0x6fa87e4f,     6
    __I   kD, kA, kB, kC,   kA,    0,    0xfe2ce6e0,    10
    __I   kC, kD, kA, kB,   kD,    9,    0xa3014314,    15
    __I   kB, kC, kD, kA,   kC,    2,    0x4e0811a1,    21
    __I   kA, kB, kC, kD,   kB,   11,    0xf7537e82,     6
    __I   kD, kA, kB, kC,   kA,    4,    0xbd3af235,    10
    __I   kC, kD, kA, kB,   kD,   13,    0x2ad7d2bb,    15
    __I   kB, kC, kD, kA,   kC,    6,    0xeb86d391,    21

    # Updating A, B, C and D
    add    bckA,     bckA,       kA
    add    bckB,     bckB,       kB
    add    bckC,     bckC,       kC
    add    bckD,     bckD,       kD

    addic. dCnt,     dCnt,      -64
    bne                    LoopOver

    # Saving A, B, C and D
#   stswi  bckA,      dBase,       16 /* Causes a crash on this processor */
    stw    bckA,  0( dBase )
    stw    bckB,  4( dBase )
    stw    bckC,  8( dBase )
    stw    bckD, 12( dBase )

    # Restoring registers and stack
    lwz     %r0,           168( %sp )
    mtlr    %r0
    lmw    %r10,            76( %sp )
    addi    %sp,      %sp,      164
    blr

