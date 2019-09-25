//===============================================================================
// Copyright (c) 2007-2016  Advanced Micro Devices, Inc. All rights reserved.
// Copyright (c) 2004-2006 ATI Technologies Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//  File Name:   Codec.h
//  Description: interface for the CCodec class
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _CODEC_H_INCLUDED_
#define _CODEC_H_INCLUDED_

#include "CodecBuffer.h"

#define SAFE_DELETE(p) if(p){delete p;p=NULL;}

typedef float CODECFLOAT;

typedef enum _CodecType
{
    CT_Unknown = 0,
    CT_None,
    CT_DXT1,
    CT_DXT3,
    CT_DXT5,
    CT_DXT5_xGBR,
    CT_DXT5_RxBG,
    CT_DXT5_RBxG,
    CT_DXT5_xRBG,
    CT_DXT5_RGxB,
    CT_DXT5_xGxR,
    CT_ATI1N,
    CT_ATI2N,
    CT_ATI2N_XY,
    CT_ATI2N_DXT5,
    CT_ATC_RGB,
    CT_ATC_RGBA_Explicit,
    CT_ATC_RGBA_Interpolated,
    CT_ETC_RGB,
#ifdef SUPPORT_ETC_ALPHA
    CT_ETC_RGBA_Explicit,
    CT_ETC_RGBA_Interpolated,
#endif // SUPPORT_ETC_ALPHA
    CT_ETC2_RGB,
    CT_ETC2_SRGB,
    CT_ETC2_RGBA,
    CT_ETC2_RGBA1,
    CT_ETC2_SRGBA,
    CT_ETC2_SRGBA1,
    CT_BC6H,
    CT_BC6H_SF,
    CT_BC7,
    CT_ASTC,
    CT_GTC,
#ifdef USE_GTC_HDR
    CT_GTCH,
#endif
    CODECS_AMD_INTERNAL
} CodecType;

typedef enum _CODECError
{
    CE_OK = 0,
    CE_Unknown,
    CE_Aborted,
} CodecError;

typedef bool (CMP_API * Codec_Feedback_Proc)(float fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2);


namespace AMD_Compress
{
class CCodec
{
public:
    CCodec(CodecType codecType);
    virtual ~CCodec();

    virtual bool SetParameter(const CMP_CHAR* pszParamName, CMP_DWORD dwValue);
    virtual bool GetParameter(const CMP_CHAR* pszParamName, CMP_DWORD& dwValue);

    virtual bool SetParameter(const CMP_CHAR* pszParamName, CODECFLOAT fValue);
    virtual bool GetParameter(const CMP_CHAR* pszParamName, CODECFLOAT& fValue);

    virtual bool SetParameter(const CMP_CHAR* pszParamName, CMP_CHAR*  dwValue);
    
    virtual CodecType GetType() const {return m_CodecType;};

    virtual CMP_DWORD GetBlockHeight() {return 1;};

    virtual CCodecBuffer* CreateBuffer(
                                        CMP_BYTE nBlockWidth, CMP_BYTE nBlockHeight, CMP_BYTE nBlockDepth, 
                                        CMP_DWORD dwWidth, CMP_DWORD dwHeight, CMP_DWORD dwPitch = 0, CMP_BYTE* pData = 0) const = 0;

    virtual CodecError Compress(CCodecBuffer& bufferIn, CCodecBuffer& bufferOut, Codec_Feedback_Proc pFeedbackProc = NULL, CMP_DWORD_PTR pUser1 = NULL, CMP_DWORD_PTR pUser2 = NULL) = 0;
    virtual CodecError Decompress(CCodecBuffer& bufferIn, CCodecBuffer& bufferOut, Codec_Feedback_Proc pFeedbackProc = NULL, CMP_DWORD_PTR pUser1 = NULL, CMP_DWORD_PTR pUser2 = NULL) = 0;

protected:
    CodecType m_CodecType;
};

} // namespace AMD_Compress

using namespace AMD_Compress;

bool SupportsSSE();
bool SupportsSSE2();

CCodec* CreateCodec(CodecType nCodecType);
CMP_DWORD CalcBufferSize(CodecType nCodecType, CMP_DWORD dwWidth, CMP_DWORD dwHeight, CMP_BYTE nBlockWidth, CMP_BYTE nBlockHeight);
CMP_DWORD CalcBufferSize(CMP_FORMAT format, CMP_DWORD dwWidth, CMP_DWORD dwHeight, CMP_DWORD dwPitch, CMP_BYTE nBlockWidth, CMP_BYTE nBlockHeight);

CMP_BYTE DeriveB(CMP_BYTE R, CMP_BYTE G);
CODECFLOAT DeriveB(CODECFLOAT R, CODECFLOAT G);

#endif // !defined(_CODEC_H_INCLUDED_)
