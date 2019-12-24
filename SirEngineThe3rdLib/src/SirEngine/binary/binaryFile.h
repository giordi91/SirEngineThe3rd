#pragma once
#include "SirEngine/core.h"
#include "SirEngine/fileUtils.h"

struct BinaryFileHeader {
  unsigned int fileType = 0;
  unsigned int version = 0;
  size_t mapperDataOffsetInByte = 0;
};

/*
The binary file format is composed of 3 parts
- header
- bulk of data
- mapper data

The header contains some basic information and an offset to the mapper data,
where headerPtr + mapperDataOffset = startOfMapperData.
The header can be used to figure out quickly some basic information on the
file.

Mapper data is a user defined blob of data that is used to make sense
of the bulk of the data.
For example, a mesh file, the header will tell us where the mapper data is
the mapper data will be able to tell us where the vertex data starts and ends
same for the index buffer, what kind of attributes we have in the mesh etc.

*/

inline const BinaryFileHeader *getHeader(const void *binaryData) {
  return reinterpret_cast<const BinaryFileHeader *>(binaryData);
};

template <typename T> const T *getMapperData(const void *binaryData) {
  const BinaryFileHeader *header = getHeader(binaryData);
  const char *outPointer = reinterpret_cast<const char *>(binaryData) +
                           header->mapperDataOffsetInByte;
  return reinterpret_cast<const T *>(outPointer);
}

struct BinaryFileWriteRequest {
  const char *outPath = nullptr;
  unsigned int fileType = 0;
  unsigned int version = 0;
  const void *bulkData = nullptr;
  size_t bulkDataSizeInByte = 0;
  const void *mapperData = nullptr;
  size_t mapperDataSizeInByte = 0;
};

enum WriteBinaryFileStatus { SUCCESS };

WriteBinaryFileStatus SIR_ENGINE_API
writeBinaryFile(const BinaryFileWriteRequest &request);

bool SIR_ENGINE_API readAllBytes(const std::string &filename,
                                 std::vector<char> &data);

#define BINARY_FILE_TYPES                                                      \
  X(NONE)                                                                      \
  X(MODEL)                                                                     \
  X(SHADER)                                                                    \
  X(RS)                                                                        \
  X(SKIN)                                                                      \
  X(ANIM)                                                                      \
  X(PSO)

enum BinaryFileType {
  NONE = 0,
  MODEL = 1,
  SHADER = 2,
  RS = 3,
  SKIN = 4,
  ANIM = 5,
  PSO = 6
};

SIR_ENGINE_API
extern const std::unordered_map<BinaryFileType, std::string>
    m_binaryFileTypeToString;

inline std::string getBinaryFileTypeName(const BinaryFileType type) {
  const auto found = m_binaryFileTypeToString.find(type);
  if (found != m_binaryFileTypeToString.end()) {
    return found->second;
  }
  return "";
}

struct ModelMapperData final {
  uint32_t vertexDataSizeInByte = 0;
  uint32_t indexDataSizeInByte = 0;
  uint32_t strideInByte = 0;
  float boundingBox[6]{};
  uint32_t vertexCount=0;
  //This should be a series of flag
  uint32_t interleaved=0;
};

struct ShaderMapperData final {
  uint32_t shaderFlags = 0;
  uint32_t shaderSizeInByte = 0;
  uint32_t typeSizeInByte = 0;
  uint32_t entryPointInByte = 0;
  uint32_t pathSizeInByte = 0;
  uint32_t compilerArgsInByte = 0;
};
struct VkShaderMapperData final {
  uint32_t shaderFlags = 0;
  uint32_t shaderSizeInByte = 0;
  uint32_t entryPointInByte = 0;
  uint32_t pathSizeInByte = 0;
  uint32_t compilerArgsInByte = 0;
  uint32_t type=0;
};

struct RootSignatureMappedData final {
  uint32_t type = 0;
  uint32_t sizeInByte = 0;
};

struct SkinMapperData {
  uint32_t influenceCountPerVertex;
  uint32_t jointsSizeInByte;
  uint32_t weightsSizeInByte;
};

struct ClipMapperData {
  int nameSizeInByte;
  int posesSizeInByte;
  float frameRate;
  int bonesPerFrame;
  int frameCount;
  int keyValueSizeInByte;
  bool isLoopable;
};

struct PSOMappedData {
  int psoSizeInByte;
  int psoDescSizeInByte;
  int psoNameSizeInByte;
  int vsShaderNameSize;
  int psShaderNameSize;
  int csShaderNameSize;
  int inputLayoutSize;
  int rootSignatureSize;
  int psoType;
};
