#include "SirEngine/animation/animationClip.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"

namespace SirEngine {

AnimationClip::~AnimationClip() {
  globals::PERSISTENT_ALLOCATOR->free(m_poses);
}

bool AnimationClip::initialize(const char *path) {
  // load the binary file
  SE_CORE_INFO("Loading animation clip {0}", path);
  const bool res = fileExists(path);
  assert(res);

  uint32_t fileSize;
  const char *binaryData = frameFileLoad(path, fileSize);

  const auto mapper = getMapperData<ClipMapperData>(binaryData);
  m_isLoopable = mapper->isLoopable;
  m_bonesPerFrame = mapper->bonesPerFrame;
  m_frameCount = mapper->frameCount;
  m_frameRate = mapper->frameRate;

  m_name = persistentString(binaryData + sizeof(BinaryFileHeader));
  m_poses = reinterpret_cast<JointPose *>(
      globals::PERSISTENT_ALLOCATOR->allocate(mapper->posesSizeInByte));
  memcpy(m_poses,
         binaryData + sizeof(BinaryFileHeader) + mapper->nameSizeInByte,
         mapper->posesSizeInByte);

  // need to load metadata
  m_metadata = reinterpret_cast<AnimationMetadataKey *>(
      globals::PERSISTENT_ALLOCATOR->allocate(mapper->keyValueSizeInByte));
  memcpy(m_metadata,
         binaryData + sizeof(BinaryFileHeader) + mapper->nameSizeInByte +
             mapper->posesSizeInByte,
         mapper->keyValueSizeInByte);
  m_metadataCount = mapper->keyValueSizeInByte / (sizeof(AnimationMetadataKey));

  return true;
}


int AnimationClip::findFirstMetadataFrame(const ANIM_CLIP_KEYWORDS flag) const
{
	//super simple linear search
	for(int i =0; i < m_metadataCount;++i) {
		if(m_metadata[i].m_key == static_cast<int>(flag)) {
			return static_cast<int>(m_metadata[i].m_value);
          }
	}
	return -1;
}

inline DirectX::XMFLOAT3 lerp3(const DirectX::XMFLOAT3 &v1,
                               const DirectX::XMFLOAT3 &v2,
                               const float amount) {
  return DirectX::XMFLOAT3{
      ((1.0f - amount) * v1.x) + (amount * v2.x),
      ((1.0f - amount) * v1.y) + (amount * v2.y),
      ((1.0f - amount) * v1.z) + (amount * v2.z),
  };
}

} // namespace SirEngine
