#include "processTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "DirectXTex/DirectXTex.h"

#include "Compressonator/Header/Compressonator.h"
#include "DDS_Helpers.h"
#include "SirEngine/log.h"

#include <unordered_map>

const std::unordered_map<std::string, CMP_FORMAT> STRING_TO_FORMAT{
    {"DXT1", CMP_FORMAT_DXT1},
    {"DXT3", CMP_FORMAT_DXT3},
    {"DXT5", CMP_FORMAT_DXT5},
    {"DXT7", CMP_FORMAT_BC7}};

const std::unordered_map<std::string, DXGI_FORMAT> STRING_TO_DXGI_FORMAT{
    {"DXT1", DXGI_FORMAT_BC1_UNORM},
    {"DXT3", DXGI_FORMAT_BC3_UNORM},
    {"DXT5", DXGI_FORMAT_BC3_UNORM},
    {"DXT7", DXGI_FORMAT_BC7_UNORM}};
const std::unordered_map<std::string, DXGI_FORMAT> STRING_TO_DXGI_FORMAT_GAMMA{
    {"DXT1", DXGI_FORMAT_BC1_UNORM_SRGB},
    {"DXT3", DXGI_FORMAT_BC3_UNORM_SRGB},
    {"DXT7", DXGI_FORMAT_BC7_UNORM_SRGB}};

inline unsigned char *getTextureDataFromFile(const char *path, int *outWidth,
                                             int *outHeight) {
  unsigned char *cpu_data =
      stbi_load(path, outWidth, outHeight, 0, STBI_rgb_alpha);
  if (cpu_data == nullptr) {
    SE_CORE_ERROR("Could not read image: {0},path");
    return nullptr;
  }
  return cpu_data;
}

void getFormatFromString(const std::string &formatString,
                         CMP_FORMAT &outFormat) {
  auto found = STRING_TO_FORMAT.find(formatString);
  if (found != STRING_TO_FORMAT.end()) {
    outFormat = found->second;
  }
}

bool processTextureFile(const char *path, const char *outPath,
                         const std::string &formatString, bool gamma) {
  int w;
  int h;
  unsigned char *data = getTextureDataFromFile(path, &w, &h);

  CMP_Texture srcTex;
  srcTex.dwSize = sizeof(CMP_Texture);
  srcTex.dwWidth = w;
  srcTex.dwHeight = h;
  srcTex.dwPitch = w * 4;
  srcTex.dwDataSize = w * h * sizeof(unsigned char) * 4;
  srcTex.format = CMP_FORMAT_RGBA_8888;
  srcTex.pData = data;

  CMP_FORMAT format = CMP_FORMAT_BC3;
  getFormatFromString(formatString, format);

  //===================================
  // Initialize Compressed Destination
  //===================================
  CMP_Texture destTexture{};
  destTexture.dwSize = sizeof(destTexture);
  destTexture.dwWidth = srcTex.dwWidth;
  destTexture.dwHeight = srcTex.dwHeight;
  destTexture.dwPitch = 0;
  destTexture.format = format;
  destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
  destTexture.pData = (CMP_BYTE *)malloc(destTexture.dwDataSize);

  //==========================
  // Set Compression Options
  //==========================
  CMP_CompressOptions options = {0};
  options.dwSize = sizeof(options);
  options.fquality = 0.05f;
  options.dwnumThreads = 8;

  CMP_ERROR cmp_status =
      CMP_ConvertTexture(&srcTex, &destTexture, &options, nullptr, NULL, NULL);
  if (cmp_status != CMP_OK) {
    free(srcTex.pData);
    free(destTexture.pData);
    SE_CORE_ERROR("Compression returned an error {0}", cmp_status);
    return false;
  }

  //==========================
  // Save Compressed Texture
  //==========================

  if (cmp_status == CMP_OK) {
    // SaveDDSFile(outPath, destTexture);

    // lets find the right format to use
    if (gamma && formatString == "DXT5") {
      free(srcTex.pData);
      free(destTexture.pData);
      SE_CORE_ERROR("Error in saving texture, cannot use DXT5 and gamma: {0}",
                    path);
      return false;
    }

    DXGI_FORMAT outFormat;

    if (gamma) {
      auto found = STRING_TO_DXGI_FORMAT_GAMMA.find(formatString);
      if (found == STRING_TO_DXGI_FORMAT_GAMMA.end()) {
        free(srcTex.pData);
        free(destTexture.pData);
        SE_CORE_ERROR(
            "Error in saving texture, format not supported with gamma {0}",
            formatString);
        return false;
      }
      outFormat = found->second;

    } else {
      auto found = STRING_TO_DXGI_FORMAT.find(formatString);
      if (found == STRING_TO_DXGI_FORMAT.end()) {
        free(srcTex.pData);
        free(destTexture.pData);
        SE_CORE_ERROR("Error in saving texture, format not supported {0}",
                      formatString);
        return false;
      }
      outFormat = found->second;
    }

    DirectX::Image img;
    img.width = destTexture.dwWidth;
    img.height = destTexture.dwHeight;

    size_t rowPitch;
    size_t slicePitch;
    DirectX::ComputePitch(outFormat, img.width, img.height,
                          rowPitch, slicePitch);
    img.rowPitch = rowPitch;
    img.slicePitch = slicePitch;
    img.pixels = destTexture.pData;
    img.format = outFormat;
    DWORD flags = 0;
    std::string outPathS{outPath};
    std::wstring outPathW{outPathS.begin(), outPathS.end()};
    HRESULT res = DirectX::SaveToDDSFile(img, flags, outPathW.c_str());

    int x = 0;

    //    size_t      width;
    //    size_t      height;
    //    DXGI_FORMAT format;
    //    size_t      rowPitch;
    //    size_t      slicePitch;
    //    uint8_t*    pixels;
    //  DirectX::SaveToDDSFile()
    free(srcTex.pData);
    free(destTexture.pData);
  }
  return true;
}
