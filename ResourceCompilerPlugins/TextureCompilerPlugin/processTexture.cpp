#include "processTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "Compressonator/Header/Compressonator.h"
#include "DDS_Helpers.h"
#include "SirEngine/log.h"
#include "unordered_map";

const std::unordered_map<std::string, CMP_FORMAT> STRING_TO_FORMAT{
    {"DXT1", CMP_FORMAT_DXT1},
    {"DXT3", CMP_FORMAT_DXT3},
    {"DXT5", CMP_FORMAT_DXT5},
    {"DXT7", CMP_FORMAT_BC7}};

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

bool loadTextureFromFile(const char *path, const char *outPath,
                                const std::string &formatString) {
  int w;
  int h;
  unsigned char *data = getTextureDataFromFile(path, &w, &h);

  CMP_Texture srcTex;
  srcTex.dwSize = sizeof(CMP_Texture);
  srcTex.dwWidth = w;
  srcTex.dwHeight = h;
  srcTex.dwPitch = w * 4;
  srcTex.dwDataSize = w * h * sizeof(unsigned char) * 4;
  srcTex.format = CMP_FORMAT_ARGB_8888;
  srcTex.pData = data;

  CMP_FORMAT format = CMP_FORMAT_DXT1;
  getFormatFromString(formatString,format);

  //===================================
  // Initialize Compressed Destination
  //===================================
  CMP_Texture destTexture;
  destTexture.dwSize = sizeof(destTexture);
  destTexture.dwWidth = srcTex.dwWidth;
  destTexture.dwHeight = srcTex.dwHeight;
  destTexture.dwPitch = 0;
  // destTexture.format     = destFormat;
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
	SE_CORE_ERROR("Compression returned an error {0}",cmp_status);
	return false;
  }

  //==========================
  // Save Compressed Testure
  //==========================
  if (cmp_status == CMP_OK)
    SaveDDSFile(outPath, destTexture);
  return true;

}
