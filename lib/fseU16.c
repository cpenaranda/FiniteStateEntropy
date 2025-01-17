/* ******************************************************************
   FSEU16 : Finite State Entropy coder for 16-bits input
   Copyright (C) 2013-2016, Yann Collet.

   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
   - Public forum : https://groups.google.com/forum/#!forum/lz4c
****************************************************************** */

/* *************************************************************
*  Tuning parameters
*****************************************************************/
/* MEMORY_USAGE :
*  Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
*  Increasing memory usage improves compression ratio
*  Reduced memory usage can improve speed, due to cache effect
*  Recommended max value is 14, for 16KB, which nicely fits into Intel x86 L1 cache */
#define FSE_original_MAX_MEMORY_USAGE 15
#define FSE_original_DEFAULT_MEMORY_USAGE 14


/* **************************************************************
*  Includes
*****************************************************************/
#include "fseU16.h"
#define FSEU16_SYMBOLVALUE_ABSOLUTEMAX 4095
#if (FSE_original_MAX_SYMBOL_VALUE > FSEU16_SYMBOLVALUE_ABSOLUTEMAX)
#  error "FSE_original_MAX_SYMBOL_VALUE is too large !"
#endif

/* **************************************************************
*  Compiler specifics
*****************************************************************/
#ifdef _MSC_VER    /* Visual Studio */
#  pragma warning(disable : 4214)        /* disable: C4214: non-int bitfields */
#endif

#if defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if defined (__clang__)
#  pragma clang diagnostic ignored "-Wunused-function"
#endif


/* **************************************************************
*  Local type
****************************************************************/
typedef struct {
    unsigned short newState;
    unsigned nbBits : 4;
    unsigned symbol : 12;
} FSE_original_decode_tU16;    /* Note : the size of this struct must be 4 */


/* *******************************************************************
*  Include type-specific functions from fse.c (C template emulation)
*********************************************************************/
#define FSE_original_COMMONDEFS_ONLY

#define FSE_original_FUNCTION_TYPE U16
#define FSE_original_FUNCTION_EXTENSION U16

#define FSE_original_count_generic FSE_original_count_genericU16
#define FSE_original_buildCTable   FSE_original_buildCTableU16

#define FSE_original_DECODE_TYPE   FSE_original_decode_tU16
#define FSE_original_createDTable  FSE_original_createDTableU16
#define FSE_original_freeDTable    FSE_original_freeDTableU16
#define FSE_original_buildDTable   FSE_original_buildDTableU16

#include "fse_compress.c"   /* FSE_original_countU16, FSE_original_buildCTableU16 */
#include "fse_decompress.c"   /* FSE_original_buildDTableU16 */


/*! FSE_original_countU16() :
    This function just counts U16 values within `src`,
    and store the histogram into `count`.
    This function is unsafe : it doesn't check that all values within `src` can fit into `count`.
    For this reason, prefer using a table `count` with 256 elements.
    @return : highest count for a single element
*/
size_t FSE_original_countU16(unsigned* count, unsigned* maxSymbolValuePtr,
                    const U16* src, size_t srcSize)
{
    const U16* ip16 = (const U16*)src;
    const U16* const end = src + srcSize;
    unsigned maxSymbolValue = *maxSymbolValuePtr;
    unsigned max=0;
    U32 s;

    memset(count, 0, (maxSymbolValue+1)*sizeof(*count));
    if (srcSize==0) { *maxSymbolValuePtr = 0; return 0; }

    while (ip16<end) {
        if (*ip16 > maxSymbolValue) return ERROR(maxSymbolValue_tooSmall);
        count[*ip16++]++;
    }

    while (!count[maxSymbolValue]) maxSymbolValue--;
    *maxSymbolValuePtr = maxSymbolValue;

    for (s=0; s<=maxSymbolValue; s++) if (count[s] > max) max = count[s];

    return (size_t)max;
}

/* *******************************************************
*  U16 Compression functions
*********************************************************/
size_t FSE_original_compressU16_usingCTable (void* dst, size_t maxDstSize,
                              const U16*  src, size_t srcSize,
                              const FSE_original_CTable* ct)
{
    const U16* const istart = src;
    const U16* const iend = istart + srcSize;
    const U16* ip;

    BYTE* op = (BYTE*) dst;
    BIT_CStream_t bitC;
    FSE_original_CState_t CState;


    /* init */
    BIT_initCStream(&bitC, op, maxDstSize);
    FSE_original_initCState(&CState, ct);

    ip=iend;

    /* join to even */
    if (srcSize & 1) {
        FSE_original_encodeSymbol(&bitC, &CState, *--ip);
        BIT_flushBits(&bitC);
    }

    /* join to mod 4 */
    if (srcSize & 2) {
        FSE_original_encodeSymbol(&bitC, &CState, *--ip);
        FSE_original_encodeSymbol(&bitC, &CState, *--ip);
        BIT_flushBits(&bitC);
    }

    /* 2 or 4 encoding per loop */
    while (ip>istart) {
        FSE_original_encodeSymbol(&bitC, &CState, *--ip);

        if (sizeof(size_t)*8 < FSE_original_MAX_TABLELOG*2+7 )   /* This test must be static */
            BIT_flushBits(&bitC);

        FSE_original_encodeSymbol(&bitC, &CState, *--ip);

        if (sizeof(size_t)*8 > FSE_original_MAX_TABLELOG*4+7 ) {  /* This test must be static */
            FSE_original_encodeSymbol(&bitC, &CState, *--ip);
            FSE_original_encodeSymbol(&bitC, &CState, *--ip);
        }
        BIT_flushBits(&bitC);
    }

    FSE_original_flushCState(&bitC, &CState);
    return BIT_closeCStream(&bitC);
}


size_t FSE_original_compressU16(void* dst, size_t maxDstSize,
       const unsigned short* src, size_t srcSize,
       unsigned maxSymbolValue, unsigned tableLog)
{
    const U16* const istart = src;
    const U16* ip = istart;

    BYTE* const ostart = (BYTE*) dst;
    BYTE* const omax = ostart + maxDstSize;
    BYTE* op = ostart;

    U32   counting[FSE_original_MAX_SYMBOL_VALUE+1] = {0};
    S16   norm[FSE_original_MAX_SYMBOL_VALUE+1];
    CTable_max_t ct;



    /* checks */
    if (srcSize <= 1) return srcSize;
    if (!maxSymbolValue) maxSymbolValue = FSE_original_MAX_SYMBOL_VALUE;
    if (!tableLog) tableLog = FSE_original_DEFAULT_TABLELOG;
    if (maxSymbolValue > FSE_original_MAX_SYMBOL_VALUE) return ERROR(maxSymbolValue_tooLarge);
    if (tableLog > FSE_original_MAX_TABLELOG) return ERROR(tableLog_tooLarge);

    /* Scan for stats */
    {   size_t const maxCount = FSE_original_countU16 (counting, &maxSymbolValue, ip, srcSize);
        if (FSE_original_isError(maxCount)) return maxCount;
        if (maxCount == srcSize) return 1;   /* Input data is one constant element x srcSize times. Use RLE compression. */
    }
    /* Normalize */
    tableLog = FSE_original_optimalTableLog(tableLog, srcSize, maxSymbolValue);
    {   size_t const errorCode = FSE_original_normalizeCount (norm, tableLog, counting, srcSize, maxSymbolValue);
        if (FSE_original_isError(errorCode)) return errorCode;
    }
    /* Write table description header */
    {   size_t const NSize = FSE_original_writeNCount (op, omax-op, norm, maxSymbolValue, tableLog);
        if (FSE_original_isError(NSize)) return NSize;
        op += NSize;
    }
    /* Compress */
    {   size_t const errorCode = FSE_original_buildCTableU16 (ct, norm, maxSymbolValue, tableLog);
        if (FSE_original_isError(errorCode)) return errorCode;
    }
    op += FSE_original_compressU16_usingCTable (op, omax - op, ip, srcSize, ct);

    /* check compressibility */
    if ( (size_t)(op-ostart) >= (size_t)(srcSize-1)*(sizeof(U16)) )
        return 0;   /* no compression */

    return op-ostart;
}


/* *******************************************************
*  U16 Decompression functions
*********************************************************/

U16 FSE_original_decodeSymbolU16(FSE_original_DState_t* DStatePtr, BIT_DStream_t* bitD)
{
    const FSE_original_decode_tU16 DInfo = ((const FSE_original_decode_tU16*)(DStatePtr->table))[DStatePtr->state];
    U16 symbol;
    size_t lowBits;
    const U32 nbBits = DInfo.nbBits;

    symbol = (U16)(DInfo.symbol);
    lowBits = BIT_readBits(bitD, nbBits);
    DStatePtr->state = DInfo.newState + lowBits;

    return symbol;
}


size_t FSE_original_decompressU16_usingDTable (U16* dst, size_t maxDstSize,
                               const void* cSrc, size_t cSrcSize,
                               const FSE_original_DTable* dt)
{
    U16* const ostart = dst;
    U16* op = ostart;
    U16* const oend = ostart + maxDstSize;
    BIT_DStream_t bitD;
    FSE_original_DState_t state;

    /* Init */
    memset(&bitD, 0, sizeof(bitD));
    BIT_initDStream(&bitD, cSrc, cSrcSize);
    FSE_original_initDState(&state, &bitD, dt);

    while((BIT_reloadDStream(&bitD) < 2) && (op<oend)) {
        *op++ = FSE_original_decodeSymbolU16(&state, &bitD);
    }

    if (!BIT_endOfDStream(&bitD)) return ERROR(GENERIC);

    return op-ostart;
}


size_t FSE_original_decompressU16(U16* dst, size_t maxDstSize,
                  const void* cSrc, size_t cSrcSize)
{
    const BYTE* const istart = (const BYTE*) cSrc;
    const BYTE* ip = istart;
    short NCount[FSE_original_MAX_SYMBOL_VALUE+1];
    DTable_max_t dt;
    unsigned maxSymbolValue = FSE_original_MAX_SYMBOL_VALUE;
    unsigned tableLog;

    /* Sanity check */
    if (cSrcSize<2) return ERROR(srcSize_wrong);   /* specific corner cases (uncompressed & rle) */

    /* normal FSE decoding mode */
    {   size_t const NSize = FSE_original_readNCount (NCount, &maxSymbolValue, &tableLog, istart, cSrcSize);
        if (FSE_original_isError(NSize)) return NSize;
        ip += NSize;
        cSrcSize -= NSize;
    }
    {   size_t const errorCode = FSE_original_buildDTableU16 (dt, NCount, maxSymbolValue, tableLog);
        if (FSE_original_isError(errorCode)) return errorCode;
    }
    return FSE_original_decompressU16_usingDTable (dst, maxDstSize, ip, cSrcSize, dt);
}
