#pragma once
#include "SirEngine/fileUtils.h"
#include <fstream>

struct BinaryFileHeader {
  unsigned int fileType;
  unsigned int version;
  size_t mapperDataOffsetInByte;
};

/*
The binary file format is composed of 3 parts
- header
- bulk of data
- mapper data

The header contains some basic informations and an offset to the mapper data,
where headerPtr + mapperDataOffset = startOfMapperData.
The header can be used to figure out quickly some basic informations on the
file.

Mapper data is a user defined blob of data that is used to make sense
of the bulk of the data.
For example, a mesh file, the header will tell us where the mapper data is
the mapper data will be able to tell us where the vertex data starts and ends
same for the index buffer, what kind of attributes we have in the mesh etc.

*/

const BinaryFileHeader *getHeader(void *binaryData) {
  return reinterpret_cast<const BinaryFileHeader *>(binaryData);
};

template <typename T> T *getMapperData(void *binaryData) {
  const BinaryFileHeader *header = getHeader(binaryData);
  char *outPointer =
      reinterpret_cast<char *>(binaryData) + header->mapperDataOffsetInByte;
  return reinterpret_cast<T *>(outPointer);
}

struct BinaryFileWriteRequest {
  const char *outPath = nullptr;
  unsigned int fileType = 0;
  unsigned int version = 0;
  const void *bulkData = nullptr;
  size_t bulkDataSizeInBtye = 0;
  const void *mapperData = nullptr;
  size_t mapperDataSizeInByte = 0;
};

enum WriteBinaryFileStatus { SUCCESS };

WriteBinaryFileStatus writeBinaryFile(const BinaryFileWriteRequest &request) {

  std::ofstream myFile(request.outPath, std::ios::out | std::ios::binary);
  BinaryFileHeader header;
  header.fileType = request.fileType;
  header.version = request.version;
  header.mapperDataOffsetInByte =
      sizeof(BinaryFileHeader) + request.bulkDataSizeInBtye;
  // lets write the header
  myFile.write((const char *)&header, sizeof(header));
  myFile.write((const char *)request.bulkData, request.bulkDataSizeInBtye);
  myFile.write((const char *)request.mapperData, request.mapperDataSizeInByte);
  myFile.close();
  return WriteBinaryFileStatus::SUCCESS;
}

void readAllBytes(const std::string &filename, std::vector<char> &data) {
  bool res = fileExists(filename);
  std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  data.resize(pos);

  ifs.seekg(0, std::ios::beg);
  ifs.read(data.data(), pos);
}

struct ModelMapperData {
  unsigned int vertexDataSizeInByte;
  unsigned int indexDataSizeInByte;
  unsigned int strideInByte;
};
