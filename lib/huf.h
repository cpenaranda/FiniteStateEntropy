/* ******************************************************************
   Huffman coder, part of New Generation Entropy library
   header file
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
****************************************************************** */
#ifndef HUF_original_H_298734234_FSE
#define HUF_original_H_298734234_FSE

#if defined (__cplusplus)
extern "C" {
#endif


/* *** Dependencies *** */
#include <stddef.h>    /* size_t */


/* *** simple functions *** */
/**
HUF_original_compress() :
    Compress content from buffer 'src', of size 'srcSize', into buffer 'dst'.
    'dst' buffer must be already allocated.
    Compression runs faster if `dstCapacity` >= HUF_original_compressBound(srcSize).
    `srcSize` must be <= `HUF_original_BLOCKSIZE_MAX` == 128 KB.
    @return : size of compressed data (<= `dstCapacity`).
    Special values : if return == 0, srcData is not compressible => Nothing is stored within dst !!!
                     if return == 1, srcData is a single repeated byte symbol (RLE compression).
                     if HUF_original_isError(return), compression failed (more details using HUF_original_getErrorName())
*/
size_t HUF_original_compress(void* dst, size_t dstCapacity,
              const void* src, size_t srcSize);

/**
HUF_original_decompress() :
    Decompress HUF data from buffer 'cSrc', of size 'cSrcSize',
    into already allocated buffer 'dst', of minimum size 'dstSize'.
    `dstSize` : **must** be the ***exact*** size of original (uncompressed) data.
    Note : in contrast with FSE, HUF_original_decompress can regenerate
           RLE (cSrcSize==1) and uncompressed (cSrcSize==dstSize) data,
           because it knows size to regenerate.
    @return : size of regenerated data (== dstSize),
              or an error code, which can be tested using HUF_original_isError()
*/
size_t HUF_original_decompress(void* dst,  size_t dstSize,
                const void* cSrc, size_t cSrcSize);


/* ****************************************
*  Tool functions
******************************************/
#define HUF_original_BLOCKSIZE_MAX (128 * 1024)
size_t HUF_original_compressBound(size_t size);       /**< maximum compressed size (worst case) */

/* Error Management */
unsigned    HUF_original_isError(size_t code);        /**< tells if a return value is an error code */
const char* HUF_original_getErrorName(size_t code);   /**< provides error code string (useful for debugging) */


/* *** Advanced function *** */

/** HUF_original_compress2() :
*   Same as HUF_original_compress(), but offers direct control over `maxSymbolValue` and `tableLog` */
size_t HUF_original_compress2 (void* dst, size_t dstSize, const void* src, size_t srcSize, unsigned maxSymbolValue, unsigned tableLog);


#ifdef HUF_original_STATIC_LINKING_ONLY

/* *** Dependencies *** */
#include "mem.h"   /* U32 */


/* *** Constants *** */
#define HUF_original_TABLELOG_ABSOLUTEMAX  16   /* absolute limit of HUF_original_MAX_TABLELOG. Beyond that value, code does not work */
#define HUF_original_TABLELOG_MAX  12           /* max configured tableLog (for static allocation); can be modified up to HUF_original_ABSOLUTEMAX_TABLELOG */
#define HUF_original_TABLELOG_DEFAULT  HUF_original_TABLELOG_MAX   /* tableLog by default, when not specified */
#define HUF_original_SYMBOLVALUE_MAX 255
#if (HUF_original_TABLELOG_MAX > HUF_original_TABLELOG_ABSOLUTEMAX)
#  error "HUF_original_TABLELOG_MAX is too large !"
#endif


/* ****************************************
*  Static allocation
******************************************/
/* HUF buffer bounds */
#define HUF_original_CTABLEBOUND 129
#define HUF_original_BLOCKBOUND(size) (size + (size>>8) + 8)   /* only true if incompressible pre-filtered with fast heuristic */
#define HUF_original_COMPRESSBOUND(size) (HUF_original_CTABLEBOUND + HUF_original_BLOCKBOUND(size))   /* Macro version, useful for static allocation */

/* static allocation of HUF's Compression Table */
#define HUF_original_CREATE_STATIC_CTABLE(name, maxSymbolValue) \
    U32 name##hb[maxSymbolValue+1]; \
    void* name##hv = &(name##hb); \
    HUF_original_CElt* name = (HUF_original_CElt*)(name##hv)   /* no final ; */

/* static allocation of HUF's DTable */
typedef U32 HUF_original_DTable;
#define HUF_original_DTABLE_SIZE(maxTableLog)   (1 + (1<<(maxTableLog)))
#define HUF_original_CREATE_STATIC_DTABLEX2(DTable, maxTableLog) \
        HUF_original_DTable DTable[HUF_original_DTABLE_SIZE((maxTableLog)-1)] = { ((U32)((maxTableLog)-1)*0x1000001) }
#define HUF_original_CREATE_STATIC_DTABLEX4(DTable, maxTableLog) \
        HUF_original_DTable DTable[HUF_original_DTABLE_SIZE(maxTableLog)] = { ((U32)(maxTableLog)*0x1000001) }


/* ****************************************
*  Advanced decompression functions
******************************************/
size_t HUF_original_decompress4X2 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /**< single-symbol decoder */
size_t HUF_original_decompress4X4 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /**< double-symbols decoder */

size_t HUF_original_decompress4X_DCtx (HUF_original_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);
size_t HUF_original_decompress4X_hufOnly(HUF_original_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize); /**< considers RLE and uncompressed as errors */
size_t HUF_original_decompress4X2_DCtx(HUF_original_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /**< single-symbol decoder */
size_t HUF_original_decompress4X4_DCtx(HUF_original_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /**< double-symbols decoder */

size_t HUF_original_decompress1X_DCtx (HUF_original_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);
size_t HUF_original_decompress1X2_DCtx(HUF_original_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /**< single-symbol decoder */
size_t HUF_original_decompress1X4_DCtx(HUF_original_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /**< double-symbols decoder */


/* ****************************************
*  HUF detailed API
******************************************/
/*!
HUF_original_compress() does the following:
1. count symbol occurrence from source[] into table count[] using FSE_original_count()
2. (optional) refine tableLog using HUF_original_optimalTableLog()
3. build Huffman table from count using HUF_original_buildCTable()
4. save Huffman table to memory buffer using HUF_original_writeCTable()
5. encode the data stream using HUF_original_compress4X_usingCTable()

The following API allows targeting specific sub-functions for advanced tasks.
For example, it's possible to compress several blocks using the same 'CTable',
or to save and regenerate 'CTable' using external methods.
*/
/* FSE_original_count() : find it within "fse.h" */
unsigned HUF_original_optimalTableLog(unsigned maxTableLog, size_t srcSize, unsigned maxSymbolValue);
typedef struct HUF_original_CElt_s HUF_original_CElt;   /* incomplete type */
size_t HUF_original_buildCTable (HUF_original_CElt* CTable, const unsigned* count, unsigned maxSymbolValue, unsigned maxNbBits);
size_t HUF_original_writeCTable (void* dst, size_t maxDstSize, const HUF_original_CElt* CTable, unsigned maxSymbolValue, unsigned huffLog);
size_t HUF_original_compress4X_usingCTable(void* dst, size_t dstSize, const void* src, size_t srcSize, const HUF_original_CElt* CTable);


/*! HUF_original_readStats() :
    Read compact Huffman tree, saved by HUF_original_writeCTable().
    `huffWeight` is destination buffer.
    @return : size read from `src` , or an error Code .
    Note : Needed by HUF_original_readCTable() and HUF_original_readDTableXn() . */
size_t HUF_original_readStats(BYTE* huffWeight, size_t hwSize, U32* rankStats,
                     U32* nbSymbolsPtr, U32* tableLogPtr,
                     const void* src, size_t srcSize);

/** HUF_original_readCTable() :
*   Loading a CTable saved with HUF_original_writeCTable() */
size_t HUF_original_readCTable (HUF_original_CElt* CTable, unsigned maxSymbolValue, const void* src, size_t srcSize);


/*
HUF_original_decompress() does the following:
1. select the decompression algorithm (X2, X4) based on pre-computed heuristics
2. build Huffman table from save, using HUF_original_readDTableXn()
3. decode 1 or 4 segments in parallel using HUF_original_decompressSXn_usingDTable
*/

/** HUF_original_selectDecoder() :
*   Tells which decoder is likely to decode faster,
*   based on a set of pre-determined metrics.
*   @return : 0==HUF_original_decompress4X2, 1==HUF_original_decompress4X4 .
*   Assumption : 0 < cSrcSize < dstSize <= 128 KB */
U32 HUF_original_selectDecoder (size_t dstSize, size_t cSrcSize);

size_t HUF_original_readDTableX2 (HUF_original_DTable* DTable, const void* src, size_t srcSize);
size_t HUF_original_readDTableX4 (HUF_original_DTable* DTable, const void* src, size_t srcSize);

size_t HUF_original_decompress4X_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_original_DTable* DTable);
size_t HUF_original_decompress4X2_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_original_DTable* DTable);
size_t HUF_original_decompress4X4_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_original_DTable* DTable);


/* single stream variants */

size_t HUF_original_compress1X (void* dst, size_t dstSize, const void* src, size_t srcSize, unsigned maxSymbolValue, unsigned tableLog);
size_t HUF_original_compress1X_usingCTable(void* dst, size_t dstSize, const void* src, size_t srcSize, const HUF_original_CElt* CTable);

size_t HUF_original_decompress1X2 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /* single-symbol decoder */
size_t HUF_original_decompress1X4 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /* double-symbol decoder */

size_t HUF_original_decompress1X_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_original_DTable* DTable);
size_t HUF_original_decompress1X2_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_original_DTable* DTable);
size_t HUF_original_decompress1X4_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_original_DTable* DTable);


#endif /* HUF_original_STATIC_LINKING_ONLY */


#if defined (__cplusplus)
}
#endif

#endif   /* HUF_original_H_298734234_FSE */
