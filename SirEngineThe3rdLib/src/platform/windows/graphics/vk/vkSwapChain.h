#include <vector>
#include <vulkan/vulkan.h>

namespace SirEngine::vk{


struct VkSwapchain {
  VkSwapchainKHR swapchain = nullptr;
  std::vector<VkImage> images;
  std::vector<VkImageView> imagesView;
  std::vector<VkFramebuffer> frameBuffers;
  VkRenderPass renderPass;
  uint32_t width = 0;
  uint32_t height = 0;
};

bool createSwapchain(const VkDevice logicalDevice,
                     const VkPhysicalDevice physicalDevice,
                     VkSurfaceKHR surface, uint32_t width, uint32_t height,
                     VkSwapchain *oldSwapchain, VkSwapchain &outSwapchain);

bool destroySwapchain(const VkDevice logicalDevice, VkSwapchain* swapchain);

void resizeSwapchain(const VkDevice logicalDevice,
                     const VkPhysicalDevice physicalDevice,
                     VkSurfaceKHR surface, uint32_t width, uint32_t height,
                     VkSwapchain &outSwapchain);
} // namespace SirEngine