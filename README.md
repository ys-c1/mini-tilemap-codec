# mini-tilemap-codec

A tiny C++17 tile-map bit-packing codec. It has no third-party dependencies.

## Format

- Tiles are stored in row-major order.
- Each tile index is packed into `bpp` bits (`1`, `2`, `4`, or `8`).
- Bits are written least-significant bit first within each byte.
- Rows are not padded; the bit stream continues across row boundaries.

## API

```cpp
std::vector<uint8_t> encode_tiles(const std::vector<uint8_t>& indices,
                                  int w, int h, int bpp);
std::vector<uint8_t> decode_tiles(const std::vector<uint8_t>& data,
                                  int w, int h, int bpp);
std::vector<uint8_t> remap_indices(const std::vector<uint8_t>& indices,
                                   const std::vector<uint8_t>& table);
int bits_per_pixel_for(uint8_t max_index);
```

`encode_tiles` and `decode_tiles` throw `std::invalid_argument` for invalid
dimensions, bit depths, tile counts, index ranges, or packed lengths.
`remap_indices` throws when an index is not covered by the remap table.

## Build and test

```sh
make test
```

The target builds with `g++ -std=c++17 -O2 -Wall -Wextra -pedantic` and runs
the assertion-based test binary at `build/test_codec`.

## Layout

```text
src/codec.h          public API
src/codec.cpp        implementation
tests/test_codec.cpp round-trip, boundary, remap, and bpp tests
Makefile             build/test targets
```

## Intentional defects

This checkout is an exercise version. Three defects are intentionally planted
in `src/codec.cpp`; the test suite is expected to fail until they are fixed.
