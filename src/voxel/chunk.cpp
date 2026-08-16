#include "voxel/chunk.hpp"

#include <algorithm>

namespace wide_eye::voxel {

Chunk::Chunk(MaterialId initial_material) noexcept {
    cells_.fill(initial_material);
}

std::optional<MaterialId> Chunk::get(LocalVoxelCoord local) const noexcept {
    const auto index = linear_index(local);
    if (!index) {
        return std::nullopt;
    }
    return cells_[*index];
}

SetBlockResult Chunk::set(LocalVoxelCoord local, MaterialId material) noexcept {
    const auto index = linear_index(local);
    if (!index) {
        return SetBlockResult::out_of_bounds;
    }
    if (cells_[*index] == material) {
        return SetBlockResult::unchanged;
    }

    cells_[*index] = material;
    include_in_dirty_region(local);
    return SetBlockResult::changed;
}

const std::optional<DirtyRegion>& Chunk::dirty_region() const noexcept {
    return dirty_region_;
}

void Chunk::clear_dirty_region() noexcept {
    dirty_region_.reset();
}

std::optional<std::size_t> Chunk::linear_index(LocalVoxelCoord local) noexcept {
    constexpr ChunkEdgeLength kStorageEdge{.cells = kEdgeLength};
    if (!is_valid_local(local, kStorageEdge)) {
        return std::nullopt;
    }

    const auto x = static_cast<std::size_t>(local.x);
    const auto y = static_cast<std::size_t>(local.y);
    const auto z = static_cast<std::size_t>(local.z);
    const auto edge = static_cast<std::size_t>(kEdgeLength);
    return x + edge * (y + edge * z);
}

void Chunk::include_in_dirty_region(LocalVoxelCoord local) noexcept {
    if (!dirty_region_) {
        dirty_region_ = DirtyRegion{.minimum = local, .maximum = local};
        return;
    }

    dirty_region_->minimum.x = std::min(dirty_region_->minimum.x, local.x);
    dirty_region_->minimum.y = std::min(dirty_region_->minimum.y, local.y);
    dirty_region_->minimum.z = std::min(dirty_region_->minimum.z, local.z);
    dirty_region_->maximum.x = std::max(dirty_region_->maximum.x, local.x);
    dirty_region_->maximum.y = std::max(dirty_region_->maximum.y, local.y);
    dirty_region_->maximum.z = std::max(dirty_region_->maximum.z, local.z);
}

} // namespace wide_eye::voxel
