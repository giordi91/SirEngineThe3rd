#include "processTexture.h"
#include "crnlib/crnlib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "d3d12.h"

#include "Compressonator/Compressonator.h"
#include "DDS_Helpers.h"
//#include "DirectXTex/DirectXTex.h"
#include "SirEngine/log.h"

#include "nlohmann/json.hpp"
#include <fstream>
#include <unordered_map>

const std::unordered_map<std::string, CMP_FORMAT> STRING_TO_FORMAT{
    {"DXT1", CMP_FORMAT_DXT1},
    {"DXT3", CMP_FORMAT_DXT3},
    {"DXT5", CMP_FORMAT_DXT5},
    {"DXT7", CMP_FORMAT_BC7}};

const std::unordered_map<std::string, crn_format> STRING_TO_FORMAT_CRN{
    {"DXT1", cCRNFmtDXT1}, {"DXT3", cCRNFmtDXT3}, {"DXT5", cCRNFmtDXT5}};

const std::unordered_map<std::string, DXGI_FORMAT> STRING_TO_DXGI_FORMAT{
    {"DXT1", DXGI_FORMAT_BC1_UNORM},
    {"DXT3", DXGI_FORMAT_BC3_UNORM},
    {"DXT5", DXGI_FORMAT_BC3_UNORM}};
const std::unordered_map<std::string, DXGI_FORMAT> STRING_TO_DXGI_FORMAT_GAMMA{
    {"DXT1", DXGI_FORMAT_BC1_UNORM_SRGB}, {"DXT3", DXGI_FORMAT_BC3_UNORM_SRGB}};

inline unsigned char *getTextureDataFromFile(const char *path, int *outWidth,
                                             int *outHeight) {
  unsigned char *cpu_data =
      // stbi_load(path, outWidth, outHeight, 0, STBI_rgb_alpha);
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
crn_format getFormatFromStringCrn(const std::string &formatString) {
  auto found = STRING_TO_FORMAT_CRN.find(formatString);
  if (found != STRING_TO_FORMAT_CRN.end()) {
    return found->second;
  } else {
    assert(0 && "crn format not found");
    return cCRNFmtInvalid;
  }
}

DXGI_FORMAT getOutputFormat(bool gamma, const std::string &formatString) {

  DXGI_FORMAT outFormat;

  if (gamma) {
    auto found = STRING_TO_DXGI_FORMAT_GAMMA.find(formatString);
    if (found == STRING_TO_DXGI_FORMAT_GAMMA.end()) {
      SE_CORE_ERROR(
          "Error in saving texture, format not supported with gamma {0}",
          formatString);
      return DXGI_FORMAT_UNKNOWN;
    }
    outFormat = found->second;

  } else {
    auto found = STRING_TO_DXGI_FORMAT.find(formatString);
    if (found == STRING_TO_DXGI_FORMAT.end()) {
      SE_CORE_ERROR("Error in saving texture, format not supported {0}",
                    formatString);
      return DXGI_FORMAT_UNKNOWN;
    }
    outFormat = found->second;
  }
  return outFormat;
}

bool processTextureFile(const char *path, const char *outPath,
                        const std::string &formatString, bool gamma,
                        bool mips) {
  int w;
  int h;
  unsigned char *data = getTextureDataFromFile(path, &w, &h);

  bool srgb_colorspace = gamma;

  crn_comp_params comp_params;

  comp_params.m_width = w;
  comp_params.m_height = h;
  comp_params.set_flag(cCRNCompFlagPerceptual, gamma);
  // comp_params.set_flag(cCRNCompFlagDXT1AForTransparency, enable_dxt1a &&
  // has_alpha_channel);
  comp_params.set_flag(cCRNCompFlagDXT1AForTransparency, false);
  // comp_params.set_flag(cCRNCompFlagHierarchical, use_adaptive_block_sizes);
  comp_params.set_flag(cCRNCompFlagHierarchical, false);
  // comp_params.m_file_type = output_crn ? cCRNFileTypeCRN : cCRNFileTypeDDS;
  comp_params.m_file_type = cCRNFileTypeDDS;
  // comp_params.m_format = (fmt != cCRNFmtInvalid) ? fmt : (has_alpha_channel ?
  // cCRNFmtDXT5 : cCRNFmtDXT1);
  comp_params.m_format = getFormatFromStringCrn(formatString);
  comp_params.m_pImages[0][0] = reinterpret_cast<crn_uint32 *>(data);

  SYSTEM_INFO g_system_info;
  GetSystemInfo(&g_system_info);
  int num_helper_threads = std::max<int>(
      0, static_cast<int>(g_system_info.dwNumberOfProcessors) - 1);
  comp_params.m_num_helper_threads = num_helper_threads;

  bool createMipmaps = mips;
  // Fill in mipmap parameters struct.
  crn_mipmap_params mip_params;
  mip_params.m_gamma_filtering = srgb_colorspace;
  mip_params.m_mode =
      createMipmaps ? cCRNMipModeGenerateMips : cCRNMipModeNoMips;

  crn_uint32 actual_quality_level;
  float actualBitrate;
  crn_uint32 outputFileSize;

  printf("Compressing to %s\n", crn_get_format_string(comp_params.m_format));
  // comp_params.m_pProgress_func = progress_callback_func;
  void *pOutput_file_data =
      crn_compress(comp_params, mip_params, outputFileSize,
                   &actual_quality_level, &actualBitrate);

  if (!pOutput_file_data) {
    stbi_image_free(data);
    SE_ERROR("Compression failed");
    return false;
  }

  FILE *pFile = fopen(outPath, "wb+");
  if ((!pFile) || (fwrite(pOutput_file_data, outputFileSize, 1, pFile) != 1) ||
      (fclose(pFile) == EOF)) {
  }

  crn_free_block(pOutput_file_data);
  stbi_image_free(data);

  auto outF = getOutputFormat(gamma, formatString);
  if (outF == DXGI_FORMAT_UNKNOWN) {
    return false;
  }

  nlohmann::json toSave;
  toSave["gamma"] = gamma;
  toSave["path"] = outPath;
  toSave["hasMips"] = mips;
  toSave["cube"] = false;
  toSave["format"] = *((int *)&outF);

  std::string outJsonTex = outPath;
  std::string dds = ".dds";
  std::string jsonPath = ".texture";
  std::string::size_type pos = 0u;
  while ((pos = outJsonTex.find(dds, pos)) != std::string::npos) {
    outJsonTex.replace(pos, dds.length(), jsonPath);
    pos += jsonPath.length();
  }
  std::ofstream file(outJsonTex);
  file << toSave;

  return true;
}

/*
//legacy Compressonator, once I will fully set on the texture compression
//method I will remove one or the other
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
*/
