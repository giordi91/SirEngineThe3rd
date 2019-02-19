#pragma once
#include <DirectXMath.h>
#include "SirEngine/handle.h"

namespace SirEngine {

struct CameraBuffer final {
  DirectX::XMMATRIX mvp;
  DirectX::XMMATRIX viewMatrix;
  float vFov;
  float screenWidth;
  float screenHeight;
  float padding;
};

class RenderingContext {

public:
	RenderingContext()=default;
	~RenderingContext()= default;
	void initialize();
	void setupCameraForFrame();
	void bindCameraBuffer(int index);
	

private:
  // member variable mostly temporary
  CameraBuffer m_camBufferCPU{};
  ConstantBufferHandle m_cameraHandle{};


};

} // namespace SirEngine
