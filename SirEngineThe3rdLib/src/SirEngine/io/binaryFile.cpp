#include "SirEngine/io//binaryFile.h"

#include "SirEngine/io/fileUtils.h"


#include <fstream>

#include "SirEngine/log.h"

#define X(name) {name, #name},
const std::unordered_map<BinaryFileType, std::string> m_binaryFileTypeToString{
    BINARY_FILE_TYPES};
#undef X

bool readAllBytes(const std::string &filename, std::vector<char> &data) {
  bool res = SirEngine::fileExists(filename);
  if (!res) {
    SE_CORE_ERROR("Could not open binary file {0}", filename);
    return false;
  }
  std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  data.resize(pos);

  ifs.seekg(0, std::ios::beg);
  ifs.read(data.data(), pos);
  return true;
}

std::string getBinaryFileTypeName(const BinaryFileType type) {
  const auto found = m_binaryFileTypeToString.find(type);
  if (found != m_binaryFileTypeToString.end()) {
    return found->second;
  }
  return "";
}

WriteBinaryFileStatus writeBinaryFile(const BinaryFileWriteRequest &request) {
  std::ofstream myFile(request.outPath, std::ios::out | std::ios::binary);
  assert(myFile.is_open());
  BinaryFileHeader header;
  header.fileType = request.fileType;
  header.version = request.version;
  header.mapperDataOffsetInByte =
      sizeof(BinaryFileHeader) + request.bulkDataSizeInByte;
  // lets write the header
  myFile.write((const char *)&header, sizeof(header));
  myFile.write((const char *)request.bulkData, request.bulkDataSizeInByte);
  myFile.write((const char *)request.mapperData, request.mapperDataSizeInByte);
  myFile.close();
  return WriteBinaryFileStatus::SUCCESS;
}
