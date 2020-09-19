
#include <vulkan/vulkan.h>

#include <vector>

namespace SirEngine {
namespace vk {
// Vulkan library type

struct QueueInfo {
  uint32_t familyIndex;
  std::vector<float> priorities;
};
struct PresentInfo {
  VkSwapchainKHR swapchain;
  uint32_t imageIndex;
};

bool checkAvailableInstanceExtensions(
    std::vector<VkExtensionProperties> &availableExtensions);

bool createVulkanInstance(std::vector<char const *> const &desiredExtensions,
                          char const *const applicationName,
                          VkInstance &instance);
bool registerDebugCallback(VkInstance instance);

bool createVulkanInstanceWithWsiExtensionsEnabled(
    std::vector<char const *> &desiredExtensions,
    char const *const applicationName, VkInstance &instance);

bool enumerateAvailablePhysicalDevices(
    const VkInstance instance, std::vector<VkPhysicalDevice> &availableDevices);

bool selectIndexOfQueueFamilyWithDesiredCapabilities(
    const VkPhysicalDevice physicalDevice,
    const VkQueueFlags desiredCapabilities, uint32_t &queueFamilyIndex);

bool selectQueueFamilyThatSupportsPresentationToGivenSurface(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface,
    uint32_t &queueFamilyIndex);

bool createLogicalDevice(const VkPhysicalDevice physicalDevice,
                         std::vector<QueueInfo> queueInfos,
                         std::vector<char const *> const &desiredExtensions,
                         VkPhysicalDeviceFeatures2 *desiredFeatures,
                         VkDevice &logicalDevice);

bool checkAvailableDeviceExtensions(
    const VkPhysicalDevice physicalDevice,
    std::vector<VkExtensionProperties> &availableExtensions); 
bool isExtensionSupported(
    std::vector<VkExtensionProperties> const &availableExtensions,
    char const *const extension);

void getDeviceQueue(const VkDevice logicalDevice,
                    const uint32_t queueFamilyIndex, const uint32_t queueIndex,
                    VkQueue &queue);

bool newSemaphore(const VkDevice logicalDevice, VkSemaphore &semaphore);

bool presentImage(VkQueue queue, std::vector<VkSemaphore> renderingSemaphores,
                  std::vector<PresentInfo> imagesToPresent);

bool beginCommandBufferRecordingOperation(
    const VkCommandBuffer commandBuffer, const VkCommandBufferUsageFlags usage,
    VkCommandBufferInheritanceInfo *secondaryCommandBufferInfo);

bool createCommandPool(const VkDevice logicalDevice,
                       const VkCommandPoolCreateFlags parameters,
                       const uint32_t queueFamily, VkCommandPool &commandPool);

bool allocateCommandBuffer(const VkDevice logicalDevice,
                           const VkCommandPool commandPool,
                           const VkCommandBufferLevel level,
                           VkCommandBuffer &commandBuffer);

struct ImageTransition {
  VkImage image;
  VkAccessFlags currentAccess;
  VkAccessFlags newAccess;
  VkImageLayout currentLayout;
  VkImageLayout newLayout;
  uint32_t currentQueueFamily;
  uint32_t newQueueFamily;
  VkImageAspectFlags aspect;
};

void setImageMemoryBarrier(const VkCommandBuffer commandBuffer,
                           const VkPipelineStageFlags generatingStages,
                           const VkPipelineStageFlags consumingStages,
                           const std::vector<ImageTransition> imageTransitions);

bool endCommandBufferRecordingOperation(const VkCommandBuffer commandBuffer);

struct WaitSemaphoreInfo {
  VkSemaphore semaphore;
  VkPipelineStageFlags waitingStage;
};

bool submitCommandBuffersToQueue(
    VkQueue queue, std::vector<WaitSemaphoreInfo> waitSemaphoreInfos,
    std::vector<VkCommandBuffer> commandBuffers,
    std::vector<VkSemaphore> signalSemaphores, VkFence &fence);

VkFramebuffer createFrameBuffer(VkDevice logicalDevice, VkRenderPass renderPass,
                                VkImageView imageView, uint32_t width,
                                uint32_t height);


}  // namespace vk
}  // namespace SirEngine