#include "platform/windows/graphics/dx12/PSOManager.h"

#include "SirEngine/fileUtils.h"
#include "nlohmann/json.hpp"
#include <d3d12.h>

#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/shaderLayout.h"
#include "platform/windows/graphics/dx12/shaderManager.h"

#include <iostream>

#include "SirEngine/application.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
#include "rootSignatureCompile.h"

namespace SirEngine::dx12 {

static const std::string PSO_KEY_TYPE = "type";
static const std::string DEFAULT_STRING = "";
static const std::string PSO_KEY_SHADER_NAME = "shaderName";
static const std::string PSO_KEY_VS_SHADER = "VS";
static const std::string PSO_KEY_PS_SHADER = "PS";

void PSOManager::init(D3D12DeviceType *device,
                      SirEngine::dx12::ShadersLayoutRegistry *registry,
                      SirEngine::dx12::RootSignatureManager *root,
                      SirEngine::dx12::ShaderManager *shader) {
  m_dxrDevice = device;
  rs_manager = root;
  shaderManager = shader;
  layoutManger = registry;
}
void PSOManager::cleanup() {
  // TODO need to be able to iterate hash map even if not idea;
  // for (auto pso : m_psoDXRRegister) {
  //  pso.second->Release();
  //}

  // for (auto pso : m_psoRegister) {
  //  pso.second->Release();
  //}
  // m_psoDXRRegister.clear();
  // m_psoRegister.clear();
}
void PSOManager::loadPSOInFolder(const char *directory) {
  std::vector<std::string> paths;
  listFilesInFolder(directory, paths, "json");

  for (const auto &p : paths) {
    PSOCompileResult result = loadPSOFile(p.c_str());
    insertInPSOCache(result);
  }
}

void PSOManager::updatePSOCache(const char *name, ID3D12PipelineState *pso) {
  assert(m_psoRegisterHandle.containsKey(name));
  PSOHandle handle;
  m_psoRegisterHandle.get(name, handle);
  uint32_t index = getIndexFromHandle(handle);
  PSOData &data = m_psoPool[index];
  // release old one
  data.pso->Release();
  data.pso = pso;
}

void PSOManager::insertInPSOCache(const PSOCompileResult &result) {
  switch (result.psoType) {
  case PSOType::DXR:
    break;
  case PSOType::RASTER: {
    bool hasShader = m_shaderToPSOFile.containsKey(result.VSName);
    if (!hasShader) {
      m_shaderToPSOFile.insert(result.VSName,
                               new ResizableVector<const char *>(20));
    }
    ResizableVector<const char *> *list;
    m_shaderToPSOFile.get(result.VSName, list);
    // make sure to internalize the string
    list->pushBack(persistentString(result.PSOFullPathFile));

    hasShader = m_shaderToPSOFile.containsKey(result.PSName);
    if (!hasShader) {
      m_shaderToPSOFile.insert(result.PSName,
                               new ResizableVector<const char *>(20));
    }
    m_shaderToPSOFile.get(result.PSName, list);
    // make sure to internalize the string
    list->pushBack(persistentString(result.PSOFullPathFile));

    // generating and storing the handle
    uint32_t index;
    PSOData &data = m_psoPool.getFreeMemoryData(index);
    data.pso = result.pso;
    const PSOHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
    data.magicNumber = MAGIC_NUMBER_COUNTER;
    m_psoRegisterHandle.insert(result.PSOName, handle);
    ++MAGIC_NUMBER_COUNTER;
    break;
  }
  case PSOType::COMPUTE: {
    // can probably wrap this into a function to make it less verbose
    // generating and storing the handle
    uint32_t index;
    PSOData &data = m_psoPool.getFreeMemoryData(index);
    data.pso = result.pso;
    const PSOHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
    data.magicNumber = MAGIC_NUMBER_COUNTER;
    m_psoRegisterHandle.insert(result.PSOName, handle);

    ++MAGIC_NUMBER_COUNTER;

    m_psoRegister.insert(result.ComputeName, result.pso);

    // need to push the pso to the map
    // first we make sure there is a vector to push to
    const bool hasShader = m_shaderToPSOFile.containsKey(result.ComputeName);
    if (!hasShader) {
      m_shaderToPSOFile.insert(result.ComputeName,
                               new ResizableVector<const char *>(20));
    }
    ResizableVector<const char *> *list;
    m_shaderToPSOFile.get(result.ComputeName, list);
    // make sure to internalize the string
    list->pushBack(persistentString(result.PSOFullPathFile));
    break;
  }
  case PSOType::INVALID:
    break;
  default:;
  }
}

void PSOManager::recompilePSOFromShader(const char *shaderName,
                                        const char *offsetPath) {
  // clearing the log
  compileLog = "";
  ResizableVector<const char *> *psos;
  bool found = m_shaderToPSOFile.get(shaderName, psos);
  if (!found) {
    assert(0);
    return;
  }
  // now we need to extract the data out of the pso to figure out which
  // shaders to recompile
  std::vector<std::string> shadersToRecompile;
  shadersToRecompile.reserve(10);
  std::string vs;
  std::string ps;
  int psoCount = psos->size();
  for (int i = 0; i < psoCount; ++i) {
    const char *pso = (*psos)[i];
    const auto jobj = getJsonObj(pso);
    std::cout << "[Engine]: Loading PSO from: " << pso << std::endl;

    const std::string psoTypeString =
        getValueIfInJson(jobj, PSO_KEY_TYPE, DEFAULT_STRING);
    const PSOType psoType = convertStringPSOTypeToEnum(psoTypeString.c_str());
    switch (psoType) {
    case (PSOType::COMPUTE): {

      const std::string computeName=
          getValueIfInJson(jobj, PSO_KEY_SHADER_NAME, DEFAULT_STRING);
      assert(!computeName.empty());
      shadersToRecompile.push_back(computeName);
      break;
    }
    // case (PSOType::DXR): {
    //  processDXRPSO(jobj, path);
    //  break;
    //}
    case (PSOType::RASTER): {
      vs = getValueIfInJson(jobj, PSO_KEY_VS_SHADER, DEFAULT_STRING);
      assert(!vs.empty());
      ps = getValueIfInJson(jobj, PSO_KEY_PS_SHADER, DEFAULT_STRING);
      assert(!ps.empty());
      shadersToRecompile.push_back(vs);
      shadersToRecompile.push_back(ps);
      break;
    }
    default: {
      assert(0 && "PSO type not supported");
    }
    }
  }

  // recompile all the shaders involved
  for (auto &shader : shadersToRecompile) {
    shaderManager->recompileShader(shader.c_str(), offsetPath, &compileLog);
  }

  // now that all shaders are recompiled we can recompile the pso
  // before doing that we do need to flush to make sure none of the PSO are
  // used
  dx12::flushDx12();

  for (int i = 0; i < psoCount; ++i) {
    const char *pso = (*psos)[i];
    const PSOCompileResult result = loadPSOFile(pso);
    // need to update the cache
    updatePSOCache(result.PSOName, result.pso);

    // log
    compileLog += "Compiled PSO: ";
    compileLog += pso;
    compileLog += "\n";
  }

  // all the shader have been recompiled, we should be able to
  // recompile the PSO now
  ShaderCompileResultEvent *e =
      new ShaderCompileResultEvent(compileLog.c_str());
  globals::APPLICATION->queueEventForEndOfFrame(e);
}

void PSOManager::printStateObjectDesc(const D3D12_STATE_OBJECT_DESC *desc) {
  std::wstringstream wstr;
  wstr << L"\n";
  wstr << L"-----------------------------------------------------------------"
          L"--"
          L"-\n";
  wstr << L"| D3D12 State Object 0x" << static_cast<const void *>(desc)
       << L": ";
  if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION)
    wstr << L"Collection\n";
  if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE)
    wstr << L"Raytracing Pipeline\n";

  auto exportTree = [](UINT depth, UINT numExports,
                       const D3D12_EXPORT_DESC *exports) {
    std::wostringstream woss;
    for (UINT i = 0; i < numExports; i++) {
      woss << L"|";
      if (depth > 0) {
        for (UINT j = 0; j < 2 * depth - 1; j++)
          woss << L" ";
      }
      woss << L" [" << i << L"]: ";
      if (exports[i].ExportToRename)
        woss << exports[i].ExportToRename << L" --> ";
      woss << exports[i].Name << L"\n";
    }
    return woss.str();
  };

  for (UINT i = 0; i < desc->NumSubobjects; i++) {
    wstr << L"| [" << i << L"]: ";
    switch (desc->pSubobjects[i].Type) {
    // case D3D12_STATE_SUBOBJECT_TYPE_FLAGS:
    //  wstr << L"Flags (not yet defined)\n";
    //  break;
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
      wstr << L"Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
      break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
      wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
      break;
    case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
      wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8)
           << *static_cast<const UINT *>(desc->pSubobjects[i].pDesc)
           << std::setw(0) << std::dec << L"\n";
      break;
    // case D3D12_STATE_SUBOBJECT_TYPE_CACHED_STATE_OBJECT:
    //  wstr << L"Cached State Object (not yet defined)\n";
    //  break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
      wstr << L"DXIL Library 0x";
      auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC *>(
          desc->pSubobjects[i].pDesc);
      wstr << lib->DXILLibrary.pShaderBytecode << L", "
           << lib->DXILLibrary.BytecodeLength << L" bytes\n";
      wstr << exportTree(1, lib->NumExports, lib->pExports);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      wstr << L"Existing Library 0x";
      auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC *>(
          desc->pSubobjects[i].pDesc);
      wstr << collection->pExistingCollection << L"\n";
      wstr << exportTree(1, collection->NumExports, collection->pExports);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      wstr << L"Subobject to Exports Association (Subobject [";
      auto association =
          static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION *>(
              desc->pSubobjects[i].pDesc);
      UINT index = static_cast<UINT>(association->pSubobjectToAssociate -
                                     desc->pSubobjects);
      wstr << index << L"])\n";
      for (UINT j = 0; j < association->NumExports; j++) {
        wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
      }
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      wstr << L"DXIL Subobjects to Exports Association (";
      auto association =
          static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION *>(
              desc->pSubobjects[i].pDesc);
      wstr << association->SubobjectToAssociate << L")\n";
      for (UINT j = 0; j < association->NumExports; j++) {
        wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
      }
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
      wstr << L"Raytracing Shader Config\n";
      auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG *>(
          desc->pSubobjects[i].pDesc);
      wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes
           << L" bytes\n";
      wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes
           << L" bytes\n";
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
      wstr << L"Raytracing Pipeline Config\n";
      auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG *>(
          desc->pSubobjects[i].pDesc);
      wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth
           << L"\n";
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      wstr << L"Hit Group (";
      auto hitGroup =
          static_cast<const D3D12_HIT_GROUP_DESC *>(desc->pSubobjects[i].pDesc);
      wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]")
           << L")\n";
      wstr << L"|  [0]: Any Hit Import: "
           << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport
                                            : L"[none]")
           << L"\n";
      wstr << L"|  [1]: Closest Hit Import: "
           << (hitGroup->ClosestHitShaderImport
                   ? hitGroup->ClosestHitShaderImport
                   : L"[none]")
           << L"\n";
      wstr << L"|  [2]: Intersection Import: "
           << (hitGroup->IntersectionShaderImport
                   ? hitGroup->IntersectionShaderImport
                   : L"[none]")
           << L"\n";
      break;
    }
    default:;
    }
    wstr << L"|--------------------------------------------------------------"
            L"--"
            L"----\n";
  }
  wstr << L"\n";
  std::wcout << wstr.str() << std::endl;
}
} // namespace SirEngine::dx12
