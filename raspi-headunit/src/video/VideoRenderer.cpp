#include "video/VideoRenderer.hpp"

namespace headunit {

bool VideoRenderer::hasFrame() const { return false; }

std::string_view VideoRenderer::backend() const { return "none"; }

std::string_view VideoRenderer::detail() const { return "Projection video is not wired."; }

}  // namespace headunit
