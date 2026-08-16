#pragma once

#include "voxel/coordinates.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace wide_eye::voxel {

// A stable one-byte key into a palette owned outside chunk storage. Value zero
// is reserved for empty space so value-initialized storage is always empty.
struct MaterialId {
    std::uint8_t value = 0;

    bool operator==(const MaterialId&) const = default;
};

inline constexpr MaterialId kEmptyMaterialId{};

[[nodiscard]] constexpr bool is_empty(MaterialId material) noexcept {
    return material == kEmptyMaterialId;
}

// Inclusive local-space bounds covering every changed cell since the last
// clear. A later rebuild owner may use this as a conservative dirty region.
struct DirtyRegion {
    LocalVoxelCoord minimum;
    LocalVoxelCoord maximum;

    bool operator==(const DirtyRegion&) const = default;
};

enum class SetBlockResult : std::uint8_t {
    out_of_bounds,
    unchanged,
    changed,
};

class Chunk {
  public:
    static constexpr GridCoordinate kEdgeLength = 16;
    static constexpr std::size_t kCellCount =
        static_cast<std::size_t>(kEdgeLength * kEdgeLength * kEdgeLength);

    explicit Chunk(MaterialId initial_material = kEmptyMaterialId) noexcept;

    [[nodiscard]] std::optional<MaterialId> get(LocalVoxelCoord local) const noexcept;
    [[nodiscard]] SetBlockResult set(LocalVoxelCoord local, MaterialId material) noexcept;

    [[nodiscard]] const std::optional<DirtyRegion>& dirty_region() const noexcept;
    void clear_dirty_region() noexcept;

  private:
    [[nodiscard]] static std::optional<std::size_t> linear_index(LocalVoxelCoord local) noexcept;
    void include_in_dirty_region(LocalVoxelCoord local) noexcept;

    std::array<MaterialId, kCellCount> cells_{};
    std::optional<DirtyRegion> dirty_region_;
};

static_assert(sizeof(MaterialId) == 1);
static_assert(Chunk::kCellCount == 4096);

} // namespace wide_eye::voxel
