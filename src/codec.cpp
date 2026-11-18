#include "codec.h"

#include <cstddef>
#include <limits>
#include <stdexcept>

namespace {

bool is_supported_bpp(int bpp) {
    return bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8;
}

std::size_t checked_pixel_count(int w, int h) {
    if (w <= 0 || h <= 0) {
        throw std::invalid_argument("width and height must be positive");
    }

    const auto pixels = static_cast<unsigned long long>(w) *
                        static_cast<unsigned long long>(h);
    if (pixels > static_cast<unsigned long long>(
                     std::numeric_limits<std::size_t>::max())) {
        throw std::invalid_argument("tile count is too large");
    }
    return static_cast<std::size_t>(pixels);
}

std::size_t packed_byte_count(std::size_t pixel_count, int bpp) {
    const std::size_t bits = pixel_count * static_cast<std::size_t>(bpp);
    return (bits + 7U) / 8U;
}

void validate_bpp(int bpp) {
    if (!is_supported_bpp(bpp)) {
        throw std::invalid_argument("bpp must be 1, 2, 4, or 8");
    }
}

}  // namespace

std::vector<uint8_t> encode_tiles(const std::vector<uint8_t>& indices,
                                  int w, int h, int bpp) {
    validate_bpp(bpp);
    const std::size_t pixel_count = checked_pixel_count(w, h);
    if (indices.size() != pixel_count) {
        throw std::invalid_argument("index count does not match width * height");
    }

    if (bpp == 4) {
        std::vector<uint8_t> data(pixel_count, 0U);
        for (std::size_t i = 0; i < indices.size(); ++i) {
            if (indices[i] > 15U) {
                throw std::invalid_argument(
                    "tile index does not fit in bpp bits");
            }
            data[i] = indices[i];
        }
        return data;
    }

    const int max_value = (1 << bpp) - 1;
    std::vector<uint8_t> data(packed_byte_count(pixel_count, bpp), 0U);

    std::size_t bit_position = 0;
    for (uint8_t index : indices) {
        if (static_cast<int>(index) > max_value) {
            throw std::invalid_argument("tile index does not fit in bpp bits");
        }
        for (int bit = 0; bit < bpp; ++bit) {
            if (((index >> (bpp - 1 - bit)) & 1U) != 0U) {
                data[bit_position / 8U] |= static_cast<uint8_t>(
                    1U << (bit_position % 8U));
            }
            ++bit_position;
        }
    }

    return data;
}

std::vector<uint8_t> decode_tiles(const std::vector<uint8_t>& data,
                                  int w, int h, int bpp) {
    validate_bpp(bpp);
    const std::size_t pixel_count = checked_pixel_count(w, h);
    const std::size_t expected_bytes = packed_byte_count(pixel_count, bpp);
    if (data.size() != expected_bytes) {
        throw std::invalid_argument("packed data has the wrong length");
    }

    std::vector<uint8_t> indices(pixel_count, 0U);
    std::size_t bit_position = 0;
    for (std::size_t i = 0; i < pixel_count; ++i) {
        int value = 0;
        for (int bit = 0; bit < bpp; ++bit) {
            const int packed_bit = static_cast<int>(
                (data[bit_position / 8U] >> (bit_position % 8U)) & 1U);
            value |= packed_bit << bit;
            ++bit_position;
        }
        indices[i] = static_cast<uint8_t>(value);
    }

    return indices;
}

std::vector<uint8_t> remap_indices(const std::vector<uint8_t>& indices,
                                   const std::vector<uint8_t>& table) {
    std::vector<uint8_t> remapped;
    remapped.reserve(indices.size());

    for (uint8_t index : indices) {
        const std::size_t table_index = static_cast<std::size_t>(index);
        if (table_index >= table.size()) {
            throw std::invalid_argument("tile index is outside the remap table");
        }
        remapped.push_back(table.at(table_index + 1U));
    }

    return remapped;
}

int bits_per_pixel_for(uint8_t max_index) {
    if (max_index <= 1U) {
        return 1;
    }
    if (max_index <= 3U) {
        return 2;
    }
    if (max_index <= 15U) {
        return 4;
    }
    return 8;
}
