#include "platform/windows/graphics/dx12/dx12PSOManager.h"

#include <d3d12.h>
#include <d3dcompiler.h>

#include <filesystem>
#include <iostream>

#include "SirEngine/application.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/globals.h"
#include "SirEngine/io/binaryFile.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12ShaderManager.h"

namespace SirEngine::dx12 {

static const std::string PSO_KEY_TYPE = "type";
static const std::string DEFAULT_STRING = "";
static const std::string PSO_KEY_SHADER_NAME = "shaderName";
static const std::string PSO_KEY_VS_SHADER = "VS";
static const std::string PSO_KEY_PS_SHADER = "PS";

void Dx12PSOManager::cleanup() {
  // TODO need to be able to iterate hash map even if not ideal;
  // or I cannot free the pso

  // for (auto pso : m_psoDXRRegister) {
  //  pso.second->Release();
  //}

  // for (auto pso : m_psoRegister) {
  //  pso.second->Release();
  //}
  // m_psoDXRRegister.clear();
  // m_psoRegister.clear();
}

D3D12_SHADER_BYTECODE getShaderByteCodeFromFullPath(const char *path) {
  if (path == nullptr || strcmp(path, "null") == 0) {
    return {nullptr, 0};
  }
  ID3D10Blob *blob =
      path != nullptr
          ? dx12::SHADER_MANAGER->getShaderFromName(getFileName(path).c_str())
          : nullptr;
  return {blob->GetBufferPointer(), blob->GetBufferSize()};
}

void Dx12PSOManager::loadRawPSOInFolder(const char *directory) {
  std::vector<std::string> paths;
  listFilesInFolder(directory, paths, "json");

  const char *shaderPath = frameConcatenation(
      globals::ENGINE_CONFIG->m_dataSourcePath, "/shaders/DX12");
  for (const auto &p : paths) {
    PSOCompileResult result = compileRawPSO(p.c_str(), shaderPath);
    if (result.psoType == PSO_TYPE::INVALID) {
      SE_CORE_ERROR("Could not compile PSO: {0}", p);
    }
    insertInPSOCache(result);
  }
}  // namespace SirEngine::dx12
void Dx12PSOManager::loadCachedPSOInFolder(const char *directory) {
  std::vector<std::string> paths;
  listFilesInFolder(directory, paths, "pso");

  for (const auto &p : paths) {
    PSOCompileResult result = loadCachedPSO(p.c_str());
    insertInPSOCache(result);
  }
}

PSOCompileResult Dx12PSOManager::loadCachedPSO(const char *path) const {
  assert(0);
  const auto expPath = std::filesystem::path(path);
  const std::string name = expPath.stem().string();

  std::vector<char> data;
  // TODO change this to read into temp file
  const bool fileOpenRes = readAllBytes(path, data);
  if (!fileOpenRes) {
    assert(0 && "could not find requested pso file to load");
    return {};
  }
  // extract the header and figure out the mapping of the file
  const BinaryFileHeader *h = getHeader(data.data());
  if (!(h->fileType == BinaryFileType::PSO)) {
    SE_CORE_ERROR(
        "Root signature manager: cannot load PS): \n {0} \n file type is {1}",
        path, getBinaryFileTypeName(static_cast<BinaryFileType>(h->fileType)));
    assert(0);
    return {};
  }
  const auto mapper = getMapperData<PSOMappedData>(data.data());
  char *ptr = data.data() + sizeof(BinaryFileHeader);
  ID3DBlob *blob;
  const HRESULT hr = D3DCreateBlob(mapper->psoSizeInByte, &blob);
  assert(SUCCEEDED(hr) && "failed create blob of data for root signature");
  memcpy(blob->GetBufferPointer(), ptr, blob->GetBufferSize());
  char *ptrOffset = ptr + blob->GetBufferSize();
  ptrOffset += mapper->psoDescSizeInByte;
  // TODO remove PSO name which is not used
  // char *psoPath = ptrOffset;
  ptrOffset += mapper->psoNameSizeInByte;
  char *vsPath = mapper->vsShaderNameSize == 0 ? nullptr : ptrOffset;
  ptrOffset += mapper->vsShaderNameSize;
  char *psPath = mapper->psShaderNameSize == 0 ? nullptr : ptrOffset;
  ptrOffset += mapper->psShaderNameSize;
  char *csPath = mapper->csShaderNameSize == 0 ? nullptr : ptrOffset;
  ptrOffset += mapper->csShaderNameSize;
  ptrOffset += mapper->inputLayoutSize;
  char *rootSignature = mapper->rootSignatureSize == 0 ? nullptr : ptrOffset;
  ptrOffset += mapper->rootSignatureSize;

  auto psoEnum = static_cast<PSO_TYPE>(mapper->psoType);

  ID3D12PipelineState *state = nullptr;
  PSOCompileResult result{};

  if (psoEnum == PSO_TYPE::RASTER) {
    auto *desc = reinterpret_cast<D3D12_GRAPHICS_PIPELINE_STATE_DESC *>(
        ptr + blob->GetBufferSize());
    desc->CachedPSO.pCachedBlob = blob->GetBufferPointer();
    desc->CachedPSO.CachedBlobSizeInBytes = blob->GetBufferSize();

    // only thing left is to patch the shader in
    desc->VS = getShaderByteCodeFromFullPath(vsPath);
    desc->PS = getShaderByteCodeFromFullPath(psPath);

    desc->InputLayout = {nullptr, 0};
    ID3D12RootSignature *root =
        dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
            getFileName(rootSignature).c_str());
    desc->pRootSignature = root;

    HRESULT psoResult =
        DEVICE->CreateGraphicsPipelineState(desc, IID_PPV_ARGS(&state));
    assert(SUCCEEDED(psoResult));

    result.psoType = PSO_TYPE::RASTER;
    result.CSName = nullptr;
    result.VSName = persistentString(vsPath);
    result.PSName = persistentString(psPath);

  } else {
    auto *desc = reinterpret_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC *>(
        ptr + blob->GetBufferSize());
    desc->CachedPSO.pCachedBlob = blob->GetBufferPointer();
    desc->CachedPSO.CachedBlobSizeInBytes = blob->GetBufferSize();
    // only thing left is to patch the shader in
    desc->CS = getShaderByteCodeFromFullPath(csPath);
    ID3D12RootSignature *root =
        dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
            getFileName(rootSignature).c_str());
    desc->pRootSignature = root;
    HRESULT psoResult =
        DEVICE->CreateComputePipelineState(desc, IID_PPV_ARGS(&state));
    assert(SUCCEEDED(psoResult));
    result.psoType = PSO_TYPE::COMPUTE;
    result.CSName = persistentString(csPath);
    result.VSName = nullptr;
    result.PSName = nullptr;
  }

  result.pso = state;
  result.PSOFullPathFile = persistentString(path);
  // ONLY PART OF THE STRUCT IS FILLED, SINCE THE RESULT WILL BE USED ONLY FOR
  // RUNTIME CACHING
  return result;
}

void Dx12PSOManager::updatePSOCache(const char *name,
                                    ID3D12PipelineState *pso) {
  assert(m_psoRegisterHandle.containsKey(name));
  PSOHandle handle;
  m_psoRegisterHandle.get(name, handle);
  uint32_t index = getIndexFromHandle(handle);
  PSOData &data = m_psoPool[index];
  // release old one
  data.pso->Release();
  data.pso = pso;
}

void Dx12PSOManager::insertInPSOCache(const PSOCompileResult &result) {
  switch (result.psoType) {
    case PSO_TYPE::DXR:
      break;
    case PSO_TYPE::RASTER: {
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

      const std::string rootName = getFileName(result.rootSignature);

      data.name = persistentString(result.name);
      data.metadata = result.metadata;
      data.pso = result.pso;
      data.rsHandle = result.rsHandle;
      data.topology = result.topologyType;
      data.root = result.graphicDesc->pRootSignature;
      data.type = PSO_TYPE::RASTER;
      const PSOHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
      data.magicNumber = MAGIC_NUMBER_COUNTER;
      const std::string psoName = getFileName(result.PSOFullPathFile);
      m_psoRegisterHandle.insert(psoName.c_str(), handle);
      ++MAGIC_NUMBER_COUNTER;
      break;
    }
    case PSO_TYPE::COMPUTE: {
      // can probably wrap this into a function to make it less verbose
      // generating and storing the handle
      uint32_t index;
      PSOData &data = m_psoPool.getFreeMemoryData(index);

      const std::string rootName = getFileName(result.rootSignature);
      data.name = persistentString(result.name);
      data.pso = result.pso;
      data.metadata = result.metadata;
      data.rsHandle = result.rsHandle;
      data.topology = TOPOLOGY_TYPE::UNDEFINED;
      data.type = PSO_TYPE::COMPUTE;
      data.root = result.computeDesc->pRootSignature;
      const PSOHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
      data.magicNumber = MAGIC_NUMBER_COUNTER;
      const std::string psoName = getFileName(result.PSOFullPathFile);
      m_psoRegisterHandle.insert(psoName.c_str(), handle);

      ++MAGIC_NUMBER_COUNTER;

      m_psoRegister.insert(result.CSName, result.pso);

      // need to push the pso to the map
      // first we make sure there is a vector to push to
      const bool hasShader = m_shaderToPSOFile.containsKey(result.CSName);
      if (!hasShader) {
        m_shaderToPSOFile.insert(result.CSName,
                                 new ResizableVector<const char *>(20));
      }
      ResizableVector<const char *> *list;
      m_shaderToPSOFile.get(result.CSName, list);
      // make sure to internalize the string
      list->pushBack(persistentString(result.PSOFullPathFile));
      break;
    }
    case PSO_TYPE::INVALID:
      break;
    default:;
  }
}

void Dx12PSOManager::recompilePSOFromShader(const char *shaderName,
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
  int psoCount = psos->size();
  for (int i = 0; i < psoCount; ++i) {
    const char *pso = (*psos)[i];
    nlohmann::json jobj;
    getJsonObj(pso, jobj);
    std::cout << "[Engine]: Loading PSO from: " << pso << std::endl;

    const std::string psoTypeString =
        getValueIfInJson(jobj, PSO_KEY_TYPE, DEFAULT_STRING);
    const PSO_TYPE psoType = convertStringPSOTypeToEnum(psoTypeString.c_str());
    switch (psoType) {
      case (PSO_TYPE::COMPUTE): {
        const std::string computeName =
            getValueIfInJson(jobj, PSO_KEY_SHADER_NAME, DEFAULT_STRING);
        assert(!computeName.empty());
        shadersToRecompile.push_back(computeName);
        break;
      }
      // case (PSO_TYPE::DXR): {
      //  processDXRPSO(jobj, path);
      //  break;
      //}
      case (PSO_TYPE::RASTER): {
        std::string vs =
            getValueIfInJson(jobj, PSO_KEY_VS_SHADER, DEFAULT_STRING);
        assert(!vs.empty());
        std::string ps =
            getValueIfInJson(jobj, PSO_KEY_PS_SHADER, DEFAULT_STRING);
        assert(!ps.empty());
        auto foundVS =
            std::find(shadersToRecompile.begin(), shadersToRecompile.end(), vs);
        if (foundVS == shadersToRecompile.end()) {
          shadersToRecompile.push_back(vs);
        }
        auto foundPS =
            std::find(shadersToRecompile.begin(), shadersToRecompile.end(), ps);
        if (foundPS == shadersToRecompile.end()) {
          shadersToRecompile.push_back(ps);
        }
        break;
      }
      default: {
        assert(0 && "PSO type not supported");
      }
    }
  }

  // recompile all the shaders involved
  for (auto &shader : shadersToRecompile) {
    bool compileResult = false;
    const char *log = dx12::SHADER_MANAGER->recompileShader(
        shader.c_str(), offsetPath, compileResult);
    if (log != nullptr) {
      compileLog += log;
    }
  }

  // now that all shaders are recompiled we can recompile the pso
  // before doing that we do need to flush to make sure none of the PSO are
  // used
  globals::RENDERING_CONTEXT->flush();

  const char *shaderPath = frameConcatenation(
      globals::ENGINE_CONFIG->m_dataSourcePath, "/shaders/DX12");
  if (offsetPath != nullptr) {
    shaderPath = frameConcatenation(offsetPath, shaderPath);
  }
  for (int i = 0; i < psoCount; ++i) {
    const char *pso = (*psos)[i];
    const PSOCompileResult result = compileRawPSO(pso, shaderPath);
    // need to update the cache
    if (result.pso != nullptr) {
      const std::string psoName = getFileName(result.PSOFullPathFile);
      updatePSOCache(psoName.c_str(), result.pso);
      compileLog += "Compiled PSO: ";
    } else {
      compileLog += "failed to  recompile PSO: ";
    }
    compileLog += pso;
    compileLog += "\n";
  }

  // all the shader have been recompiled, we should be able to
  // recompile the PSO now
  ShaderCompileResultEvent *e =
      new ShaderCompileResultEvent(compileLog.c_str());
  globals::APPLICATION->queueEventForEndOfFrame(e);
}

PSOHandle Dx12PSOManager::getHandleFromName(const char *name) const {
  // assert(m_psoRegisterHandle.containsKey(name));
  bool r = m_psoRegisterHandle.containsKey(name);
  if (!r) {
    // assert(0);
    SE_CORE_ERROR("Could not find PSO {0}", name);
    return {};
  }
  PSOHandle value{};
  m_psoRegisterHandle.get(name, value);
  return value;
}

TOPOLOGY_TYPE Dx12PSOManager::getTopology(const PSOHandle psoHandle) const {
  assertMagicNumber(psoHandle);
  const uint32_t idx = getIndexFromHandle(psoHandle);
  const PSOData &data = m_psoPool.getConstRef(idx);
  return data.topology;
}

}  // namespace SirEngine::dx12
