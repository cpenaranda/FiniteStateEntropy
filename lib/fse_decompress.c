/* ******************************************************************
   FSE : Finite State Entropy decoder
   Copyright (C) 2013-2015, Yann Collet.

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
    - FSE source repository : https://github.com/Cyan4973/FiniteStateEntropy
    - Public forum : https://groups.google.com/forum/#!forum/lz4c
****************************************************************** */


/* **************************************************************
*  Compiler specifics
****************************************************************/
#ifdef _MSC_VER    /* Visual Studio */
#  define FORCE_INLINE static __forceinline
#  include <intrin.h>                    /* For Visual 2005 */
#  pragma warning(disable : 4127)        /* disable: C4127: conditional expression is constant */
#  pragma warning(disable : 4214)        /* disable: C4214: non-int bitfields */
#else
#  ifdef __GNUC__
#    define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#    define FORCE_INLINE static inline __attribute__((always_inline))
#  else
#    define FORCE_INLINE static inline
#  endif
#endif


/* **************************************************************
*  Includes
****************************************************************/
#include <stdlib.h>     /* malloc, free, qsort */
#include <string.h>     /* memcpy, memset */
#include <stdio.h>      /* printf (debug) */
#include "bitstream.h"
#define FSE_original_STATIC_LINKING_ONLY
#include "fse.h"


/* **************************************************************
*  Error Management
****************************************************************/
#define FSE_original_isError ERR_isError
#define FSE_original_STATIC_ASSERT(c) { enum { FSE_original_static_assert = 1/(int)(!!(c)) }; }   /* use only *after* variable declarations */


/* **************************************************************
*  Complex types
****************************************************************/
typedef U32 DTable_max_t[FSE_original_DTABLE_SIZE_U32(FSE_original_MAX_TABLELOG)];


/* **************************************************************
*  Templates
****************************************************************/
/*
  designed to be included
  for type-specific functions (template emulation in C)
  Objective is to write these functions only once, for improved maintenance
*/

/* safety checks */
#ifndef FSE_original_FUNCTION_EXTENSION
#  error "FSE_original_FUNCTION_EXTENSION must be defined"
#endif
#ifndef FSE_original_FUNCTION_TYPE
#  error "FSE_original_FUNCTION_TYPE must be defined"
#endif

/* Function names */
#define FSE_original_CAT(X,Y) X##Y
#define FSE_original_FUNCTION_NAME(X,Y) FSE_original_CAT(X,Y)
#define FSE_original_TYPE_NAME(X,Y) FSE_original_CAT(X,Y)


/* Function templates */
FSE_original_DTable* FSE_original_createDTable (unsigned tableLog)
{
    if (tableLog > FSE_original_TABLELOG_ABSOLUTE_MAX) tableLog = FSE_original_TABLELOG_ABSOLUTE_MAX;
    return (FSE_original_DTable*)malloc( FSE_original_DTABLE_SIZE_U32(tableLog) * sizeof (U32) );
}

void FSE_original_freeDTable (FSE_original_DTable* dt)
{
    free(dt);
}

size_t FSE_original_buildDTable(FSE_original_DTable* dt, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog)
{
    void* const tdPtr = dt+1;   /* because *dt is unsigned, 32-bits aligned on 32-bits */
    FSE_original_DECODE_TYPE* const tableDecode = (FSE_original_DECODE_TYPE*) (tdPtr);
    U16 symbolNext[FSE_original_MAX_SYMBOL_VALUE+1];

    U32 const maxSV1 = maxSymbolValue + 1;
    U32 const tableSize = 1 << tableLog;
    U32 highThreshold = tableSize-1;

    /* Sanity Checks */
    if (maxSymbolValue > FSE_original_MAX_SYMBOL_VALUE) return ERROR(maxSymbolValue_tooLarge);
    if (tableLog > FSE_original_MAX_TABLELOG) return ERROR(tableLog_tooLarge);

    /* Init, lay down lowprob symbols */
    {   FSE_original_DTableHeader DTableH;
        DTableH.tableLog = (U16)tableLog;
        DTableH.fastMode = 1;
        {   S16 const largeLimit= (S16)(1 << (tableLog-1));
            U32 s;
            for (s=0; s<maxSV1; s++) {
                if (normalizedCounter[s]==-1) {
                    tableDecode[highThreshold--].symbol = (FSE_original_FUNCTION_TYPE)s;
                    symbolNext[s] = 1;
                } else {
                    if (normalizedCounter[s] >= largeLimit) DTableH.fastMode=0;
                    symbolNext[s] = normalizedCounter[s];
        }   }   }
        memcpy(dt, &DTableH, sizeof(DTableH));
    }

    /* Spread symbols */
    {   U32 const tableMask = tableSize-1;
        U32 const step = FSE_original_TABLESTEP(tableSize);
        U32 s, position = 0;
        for (s=0; s<maxSV1; s++) {
            int i;
            for (i=0; i<normalizedCounter[s]; i++) {
                tableDecode[position].symbol = (FSE_original_FUNCTION_TYPE)s;
                position = (position + step) & tableMask;
                while (position > highThreshold) position = (position + step) & tableMask;   /* lowprob area */
        }   }

        if (position!=0) return ERROR(GENERIC);   /* position must reach all cells once, otherwise normalizedCounter is incorrect */
    }

    /* Build Decoding table */
    {   U32 u;
        for (u=0; u<tableSize; u++) {
            FSE_original_FUNCTION_TYPE const symbol = (FSE_original_FUNCTION_TYPE)(tableDecode[u].symbol);
            U16 nextState = symbolNext[symbol]++;
            tableDecode[u].nbBits = (BYTE) (tableLog - BIT_highbit32 ((U32)nextState) );
            tableDecode[u].newState = (U16) ( (nextState << tableDecode[u].nbBits) - tableSize);
    }   }

    return 0;
}



#ifndef FSE_original_COMMONDEFS_ONLY

/*-*******************************************************
*  Decompression (Byte symbols)
*********************************************************/
size_t FSE_original_buildDTable_rle (FSE_original_DTable* dt, BYTE symbolValue)
{
    void* ptr = dt;
    FSE_original_DTableHeader* const DTableH = (FSE_original_DTableHeader*)ptr;
    void* dPtr = dt + 1;
    FSE_original_decode_t* const cell = (FSE_original_decode_t*)dPtr;

    DTableH->tableLog = 0;
    DTableH->fastMode = 0;

    cell->newState = 0;
    cell->symbol = symbolValue;
    cell->nbBits = 0;

    return 0;
}


size_t FSE_original_buildDTable_raw (FSE_original_DTable* dt, unsigned nbBits)
{
    void* ptr = dt;
    FSE_original_DTableHeader* const DTableH = (FSE_original_DTableHeader*)ptr;
    void* dPtr = dt + 1;
    FSE_original_decode_t* const dinfo = (FSE_original_decode_t*)dPtr;
    const unsigned tableSize = 1 << nbBits;
    const unsigned tableMask = tableSize - 1;
    const unsigned maxSV1 = tableMask+1;
    unsigned s;

    /* Sanity checks */
    if (nbBits < 1) return ERROR(GENERIC);         /* min size */

    /* Build Decoding Table */
    DTableH->tableLog = (U16)nbBits;
    DTableH->fastMode = 1;
    for (s=0; s<maxSV1; s++) {
        dinfo[s].newState = 0;
        dinfo[s].symbol = (BYTE)s;
        dinfo[s].nbBits = (BYTE)nbBits;
    }

    return 0;
}

FORCE_INLINE size_t FSE_original_decompress_usingDTable_generic(
          void* dst, size_t maxDstSize,
    const void* cSrc, size_t cSrcSize,
    const FSE_original_DTable* dt, const unsigned fast)
{
    BYTE* const ostart = (BYTE*) dst;
    BYTE* op = ostart;
    BYTE* const omax = op + maxDstSize;
    BYTE* const olimit = omax-3;

    BIT_DStream_t bitD;
    FSE_original_DState_t state1;
    FSE_original_DState_t state2;

    /* Init */
    { size_t const errorCode = BIT_initDStream(&bitD, cSrc, cSrcSize);   /* replaced last arg by maxCompressed Size */
      if (FSE_original_isError(errorCode)) return errorCode; }

    FSE_original_initDState(&state1, &bitD, dt);
    FSE_original_initDState(&state2, &bitD, dt);

#define FSE_original_GETSYMBOL(statePtr) fast ? FSE_original_decodeSymbolFast(statePtr, &bitD) : FSE_original_decodeSymbol(statePtr, &bitD)

    /* 4 symbols per loop */
    for ( ; (BIT_reloadDStream(&bitD)==BIT_DStream_unfinished) && (op<olimit) ; op+=4) {
        op[0] = FSE_original_GETSYMBOL(&state1);

        if (FSE_original_MAX_TABLELOG*2+7 > sizeof(bitD.bitContainer)*8)    /* This test must be static */
            BIT_reloadDStream(&bitD);

        op[1] = FSE_original_GETSYMBOL(&state2);

        if (FSE_original_MAX_TABLELOG*4+7 > sizeof(bitD.bitContainer)*8)    /* This test must be static */
            { if (BIT_reloadDStream(&bitD) > BIT_DStream_unfinished) { op+=2; break; } }

        op[2] = FSE_original_GETSYMBOL(&state1);

        if (FSE_original_MAX_TABLELOG*2+7 > sizeof(bitD.bitContainer)*8)    /* This test must be static */
            BIT_reloadDStream(&bitD);

        op[3] = FSE_original_GETSYMBOL(&state2);
    }

    /* tail */
    /* note : BIT_reloadDStream(&bitD) >= FSE_original_DStream_partiallyFilled; Ends at exactly BIT_DStream_completed */
    while (1) {
        if (op>(omax-2)) return ERROR(dstSize_tooSmall);

        *op++ = FSE_original_GETSYMBOL(&state1);

        if (BIT_reloadDStream(&bitD)==BIT_DStream_overflow) {
            *op++ = FSE_original_GETSYMBOL(&state2);
            break;
        }

        if (op>(omax-2)) return ERROR(dstSize_tooSmall);

        *op++ = FSE_original_GETSYMBOL(&state2);

        if (BIT_reloadDStream(&bitD)==BIT_DStream_overflow) {
            *op++ = FSE_original_GETSYMBOL(&state1);
            break;
    }   }

    return op-ostart;
}


size_t FSE_original_decompress_usingDTable(void* dst, size_t originalSize,
                            const void* cSrc, size_t cSrcSize,
                            const FSE_original_DTable* dt)
{
    const void* ptr = dt;
    const FSE_original_DTableHeader* DTableH = (const FSE_original_DTableHeader*)ptr;
    const U32 fastMode = DTableH->fastMode;

    /* select fast mode (static) */
    if (fastMode) return FSE_original_decompress_usingDTable_generic(dst, originalSize, cSrc, cSrcSize, dt, 1);
    return FSE_original_decompress_usingDTable_generic(dst, originalSize, cSrc, cSrcSize, dt, 0);
}


size_t FSE_original_decompress(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize)
{
    const BYTE* const istart = (const BYTE*)cSrc;
    const BYTE* ip = istart;
    short counting[FSE_original_MAX_SYMBOL_VALUE+1];
    DTable_max_t dt;   /* Static analyzer seems unable to understand this table will be properly initialized later */
    unsigned tableLog;
    unsigned maxSymbolValue = FSE_original_MAX_SYMBOL_VALUE;

    if (cSrcSize<2) return ERROR(srcSize_wrong);   /* too small input size */

    /* normal FSE decoding mode */
    {   size_t const NCountLength = FSE_original_readNCount (counting, &maxSymbolValue, &tableLog, istart, cSrcSize);
        if (FSE_original_isError(NCountLength)) return NCountLength;
        if (NCountLength >= cSrcSize) return ERROR(srcSize_wrong);   /* too small input size */
        ip += NCountLength;
        cSrcSize -= NCountLength;
    }

    { size_t const errorCode = FSE_original_buildDTable (dt, counting, maxSymbolValue, tableLog);
      if (FSE_original_isError(errorCode)) return errorCode; }

    return FSE_original_decompress_usingDTable (dst, maxDstSize, ip, cSrcSize, dt);   /* always return, even if it is an error code */
}



#endif   /* FSE_original_COMMONDEFS_ONLY */
