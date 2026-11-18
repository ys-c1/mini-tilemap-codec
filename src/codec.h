#pragma once

#include <cstdint>
#include <vector>

// Pack row-major tile indices into a contiguous, LSB-first bit stream.
// bpp must be one of 1, 2, 4, or 8. No per-row padding is added.
std::vector<uint8_t> encode_tiles(const std::vector<uint8_t>& indices,
                                  int w, int h, int bpp);

// Unpack a stream produced by encode_tiles.
std::vector<uint8_t> decode_tiles(const std::vector<uint8_t>& data,
                                  int w, int h, int bpp);

// Map every index through table[index]. Throws when an index is not present
// in the table.
std::vector<uint8_t> remap_indices(const std::vector<uint8_t>& indices,
                                   const std::vector<uint8_t>& table);

// Smallest supported bit width that can represent max_index.
int bits_per_pixel_for(uint8_t max_index);
