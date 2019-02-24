#pragma once
#include "SirEngine/handle.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"

namespace SirEngine {


class RenderingContext {

public:
	RenderingContext()=default;
	~RenderingContext()= default;
	void initialize();
	void setupCameraForFrame();
	void bindCameraBuffer(int index) const;
	

private:
  // member variable mostly temporary
  CameraBuffer m_camBufferCPU{};
  ConstantBufferHandle m_cameraHandle{};


};

} // namespace SirEngine
