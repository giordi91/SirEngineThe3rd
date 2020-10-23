#include "platform/windows/graphics/vk/vkImGuiManager.h"

#include <imgui/imgui.h>


#include "vkRootSignatureManager.h"
#include "vkTextureManager.h"
#include "platform/windows/graphics/vk/imgui_impl_vulkan.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkBindingTableManager.h"
#include "platform/windows/graphics/vk/vkSwapChain.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/events/applicationEvent.h"

namespace SirEngine {
void vk::VkImGuiManager::initialize() {
  ImGui_ImplVulkan_InitInfo vkinfo{};
  vkinfo.Instance = vk::INSTANCE;
  vkinfo.PhysicalDevice = vk::PHYSICAL_DEVICE;
  vkinfo.Device = vk::LOGICAL_DEVICE;
  vkinfo.QueueFamily = vk::GRAPHICS_QUEUE_FAMILY;
  vkinfo.Queue = vk::GRAPHICS_QUEUE;
  vkinfo.PipelineCache = nullptr;
  vkinfo.DescriptorPool = vk::DESCRIPTOR_MANAGER->getPool();
  vkinfo.Allocator = nullptr;
  vkinfo.ImageCount = vk::SWAP_CHAIN_IMAGE_COUNT;
  vkinfo.MinImageCount = vk::SWAP_CHAIN_IMAGE_COUNT;

  VkAttachmentDescription attachment = {};
  attachment.format = vk::IMAGE_FORMAT;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  VkAttachmentReference color_attachment = {};
  color_attachment.attachment = 0;
  color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment;
  VkRenderPassCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 1;
  info.pAttachments = &attachment;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  VK_CHECK(vkCreateRenderPass(vk::LOGICAL_DEVICE, &info, nullptr, &imguiPass));

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();
  ImGui_ImplVulkan_Init(&vkinfo, imguiPass);

  // create a command buffer separated to execute this stuff

  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, &beginInfo);
  ImGui_ImplVulkan_CreateFontsTexture(
      vk::CURRENT_FRAME_COMMAND->m_commandBuffer);

  VkSubmitInfo end_info = {};
  end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  end_info.commandBufferCount = 1;
  end_info.pCommandBuffers = &vk::CURRENT_FRAME_COMMAND->m_commandBuffer;
  VK_CHECK(vkEndCommandBuffer(vk::CURRENT_FRAME_COMMAND->m_commandBuffer));
  VK_CHECK(vkQueueSubmit(vk::GRAPHICS_QUEUE, 1, &end_info, VK_NULL_HANDLE));
  VK_CHECK(vkDeviceWaitIdle(vk::LOGICAL_DEVICE));
  vkResetCommandBuffer(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, 0);
  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void vk::VkImGuiManager::cleanup() {
  VK_CHECK(vkDeviceWaitIdle(vk::LOGICAL_DEVICE));
  ImGui_ImplVulkan_Shutdown();
  ImGui::DestroyContext();
  vkDestroyRenderPass(vk::LOGICAL_DEVICE, imguiPass, nullptr);
}

void vk::VkImGuiManager::startFrame() { ImGui_ImplVulkan_NewFrame(); }

void vk::VkImGuiManager::endFrame() {
  ImGui::Render();
  VkRenderPassBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.renderPass = imguiPass;
  info.framebuffer = vk::SWAP_CHAIN->frameBuffers[globals::CURRENT_FRAME];
  info.renderArea.extent.width = globals::ENGINE_CONFIG->m_windowWidth;
  info.renderArea.extent.height = globals::ENGINE_CONFIG->m_windowHeight;
  info.clearValueCount = 0;
  info.pClearValues = nullptr;
  vkCmdBeginRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, &info,
                       VK_SUBPASS_CONTENTS_INLINE);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
  // Submit command buffer
  vkCmdEndRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
}

void vk::VkImGuiManager::onResizeEvent(const WindowResizeEvent& e)
{
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(e.getWidth()),
                          static_cast<float>(e.getHeight()));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

ImTextureID vk::VkImGuiManager::getImguiImageHandle(const TextureHandle& handle)
{
  auto d = vk::TEXTURE_MANAGER->getTextureData(handle);
  return (ImTextureID)(void*)ImGui_ImplVulkan_AddTexture(
      vk::STATIC_SAMPLERS[2], d.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

}
}  // namespace SirEngine
