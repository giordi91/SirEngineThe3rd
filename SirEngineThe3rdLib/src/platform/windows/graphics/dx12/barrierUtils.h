#pragma once

#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/depthTexture.h"
namespace SirEngine {
namespace dx12 {
// generates a barrier for the given texture 2D resource only
// if the states requires a transition.
// The provided counter is where in the array of barriers the
// function should write to, the retunr counter would be the next
// free slot in the array where other methods can write to.
	/*
inline int transitionTexture2DifNeeded(Texture2D *resource,
                                       D3D12_RESOURCE_STATES wantedState,
                                       D3D12_RESOURCE_BARRIER *barriers,
                                       int counter) {

  auto state = resource->getState();
  if (state != wantedState) {
    barriers[counter] = CD3DX12_RESOURCE_BARRIER::Transition(
        resource->getResource(), state, wantedState);
    resource->setState(wantedState);
    ++counter;
  }
  return counter;
}
*/

inline int transitionDepthTextureIfNeeded(DepthTexture *resource,
                                          D3D12_RESOURCE_STATES wantedState,
                                          D3D12_RESOURCE_BARRIER *barriers,
                                          int counter) {

  auto state = resource->getState();
  if (state != wantedState) {
    barriers[counter] = CD3DX12_RESOURCE_BARRIER::Transition(
        resource->getResource(), state, wantedState);
    resource->setState(wantedState);
    ++counter;
  }
  return counter;
}

/*
// performs a transition, with no checks whether or not the resource
// needs it, only use this when you are 100% sure the resource needs
// to be transitioned and you don't want to pay for the extra branch
inline int transitionTexture2D(Texture2D *resource,
                               D3D12_RESOURCE_STATES wantedState,
                               D3D12_RESOURCE_BARRIER *barriers, int counter) {

  auto state = resource->getState();
  barriers[counter] = CD3DX12_RESOURCE_BARRIER::Transition(
      resource->getResource(), state, wantedState);
  resource->setState(wantedState);
  ++counter;
  return counter;
}
*/

} // namespace dx12
} // namespace SirEngine
