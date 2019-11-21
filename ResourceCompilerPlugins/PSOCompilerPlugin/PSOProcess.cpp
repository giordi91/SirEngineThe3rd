#include "PSOProcess.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include <cassert>
#include <string>
#include <unordered_map>

void processPSO(const char *path, PSOResult &blob)
{
	/*
  auto jobj = getJsonObj(path);
  SE_CORE_INFO("[Engine]: Loading PSO from: {0}", path);

  const std::string psoTypeString =
      getValueIfInJson(jobj, PSO_KEY_TYPE, DEFAULT_STRING);
  assert(!psoTypeString.empty());
  const SirEngine::dx12::PSOType psoType = convertStringPSOTypeToEnum(psoTypeString);
  switch (psoType) {
  case (SirEngine::dx12::PSOType::COMPUTE): {
    processComputePSO(jobj, path, reload);
    break;
  }
  // case (PSOType::DXR): {
  //  processDXRPSO(jobj, path);
  //  break;
  //}
  case (PSOType::RASTER): {
    processRasterPSO(jobj, path, reload);
    break;
  }
  default: {
    assert(0 && "PSO Type not supported");
  }
  }
  */


	
}
