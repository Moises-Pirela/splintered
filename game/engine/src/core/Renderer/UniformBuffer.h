#include <vendor/glm/glm/glm.hpp>
#include <vulkan/vulkan.h>

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};