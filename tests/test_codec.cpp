#include "codec.h"

#include <cstddef>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

std::string bytes_to_string(const std::vector<uint8_t>& bytes) {
    std::ostringstream out;
    out << '[';
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << static_cast<unsigned int>(bytes[i]);
    }
    out << ']';
    return out.str();
}

void fail(const char* expression, const char* file, int line,
          const std::string& detail) {
    std::cerr << "FAIL " << file << ':' << line << ": " << expression;
    if (!detail.empty()) {
        std::cerr << " [" << detail << ']';
    }
    std::cerr << '\n';
}

void check_true(bool value, const char* expression, const char* file, int line) {
    ++g_checks;
    if (!value) {
        ++g_failures;
        fail(expression, file, line, "");
    }
}

template <typename Actual, typename Expected>
void check_equal(const Actual& actual, const Expected& expected,
                 const char* expression, const char* file, int line) {
    ++g_checks;
    if (!(actual == expected)) {
        ++g_failures;
        std::ostringstream detail;
        detail << "actual=" << actual << ", expected=" << expected;
        fail(expression, file, line, detail.str());
    }
}

void check_bytes(const std::vector<uint8_t>& actual,
                 const std::vector<uint8_t>& expected,
                 const char* expression, const char* file, int line) {
    ++g_checks;
    if (actual != expected) {
        ++g_failures;
        fail(expression, file, line,
             "actual=" + bytes_to_string(actual) +
                 ", expected=" + bytes_to_string(expected));
    }
}

template <typename Fn>
bool throws_invalid_argument(Fn fn) {
    try {
        fn();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

#define CHECK(expression) \
    check_true(static_cast<bool>(expression), #expression, __FILE__, __LINE__)

#define CHECK_EQ(actual_expression, expected_expression)                  \
    check_equal((actual_expression), (expected_expression),               \
                #actual_expression " == " #expected_expression,           \
                __FILE__, __LINE__)

#define CHECK_BYTES(actual_expression, expected_expression)               \
    check_bytes((actual_expression), (expected_expression),               \
                #actual_expression " == " #expected_expression,           \
                __FILE__, __LINE__)

void test_bpp1_round_trip() {
    const std::vector<uint8_t> input{0, 1, 1, 0, 1, 0, 0, 1};
    const auto encoded = encode_tiles(input, 8, 1, 1);
    CHECK_EQ(encoded.size(), std::size_t{1});
    CHECK_BYTES(encoded, (std::vector<uint8_t>{0x96U}));
    const auto decoded = decode_tiles(encoded, 8, 1, 1);
    CHECK_BYTES(decoded, input);
}

void test_bpp2_round_trip() {
    const std::vector<uint8_t> input{0, 1, 2, 3};
    const auto encoded = encode_tiles(input, 4, 1, 2);
    CHECK_EQ(encoded.size(), std::size_t{1});
    CHECK_BYTES(encoded, (std::vector<uint8_t>{0xE4U}));
    const auto decoded = decode_tiles(encoded, 4, 1, 2);
    CHECK_BYTES(decoded, input);
}

void test_bpp4_round_trip() {
    const std::vector<uint8_t> input{0, 1, 2, 3};
    const auto encoded = encode_tiles(input, 4, 1, 4);
    CHECK_EQ(encoded.size(), std::size_t{2});
    CHECK_BYTES(encoded, (std::vector<uint8_t>{0x10U, 0x32U}));

    try {
        const auto decoded = decode_tiles(encoded, 4, 1, 4);
        CHECK_BYTES(decoded, input);
    } catch (const std::exception& error) {
        ++g_checks;
        ++g_failures;
        fail("decode_tiles(encoded, 4, 1, 4)", __FILE__, __LINE__,
             std::string("unexpected exception: ") + error.what());
    }
}

void test_bpp8_round_trip() {
    const std::vector<uint8_t> input{0, 1, 2, 255};
    const auto encoded = encode_tiles(input, 4, 1, 8);
    CHECK_EQ(encoded.size(), std::size_t{4});
    CHECK_BYTES(encoded, input);
    const auto decoded = decode_tiles(encoded, 4, 1, 8);
    CHECK_BYTES(decoded, input);
}

void test_unaligned_3x3_round_trip() {
    const std::vector<uint8_t> input{0, 1, 1, 0, 1, 0, 0, 1, 1};
    const auto encoded = encode_tiles(input, 3, 3, 1);
    CHECK_EQ(encoded.size(), std::size_t{2});
    const auto decoded = decode_tiles(encoded, 3, 3, 1);
    CHECK_BYTES(decoded, input);
}

void test_unaligned_5x2_round_trip() {
    const std::vector<uint8_t> input{0, 1, 2, 3, 0, 1, 2, 3, 0, 1};
    const auto encoded = encode_tiles(input, 5, 2, 2);
    CHECK_EQ(encoded.size(), std::size_t{3});
    CHECK_BYTES(encoded, (std::vector<uint8_t>{0xE4U, 0xE4U, 0x04U}));
    const auto decoded = decode_tiles(encoded, 5, 2, 2);
    CHECK_BYTES(decoded, input);
}

void test_encode_rejects_index_out_of_range() {
    CHECK(throws_invalid_argument([] {
        const auto encoded = encode_tiles(std::vector<uint8_t>{4}, 1, 1, 2);
        (void)encoded;
    }));
}

void test_remap() {
    const std::vector<uint8_t> table{3, 2, 1, 0};
    const std::vector<uint8_t> input{0, 1, 2, 3};

    std::vector<uint8_t> remapped(4U, 0xFFU);
    bool remap_threw = false;
    std::string remap_error;
    try {
        remapped = remap_indices(input, table);
    } catch (const std::exception& error) {
        remap_threw = true;
        remap_error = error.what();
    }

    ++g_checks;
    if (remap_threw) {
        ++g_failures;
        fail("remap_indices({0, 1, 2, 3}, {3, 2, 1, 0}) did not throw",
             __FILE__, __LINE__, remap_error);
    }

    CHECK_EQ(remapped.size(), std::size_t{4});
    if (remapped.size() == 4) {
        CHECK_EQ(static_cast<int>(remapped[0]), 3);
        CHECK_EQ(static_cast<int>(remapped[1]), 2);
        CHECK_EQ(static_cast<int>(remapped[2]), 1);
        CHECK_EQ(static_cast<int>(remapped[3]), 0);
    }

    CHECK(throws_invalid_argument([] {
        const auto out = remap_indices(std::vector<uint8_t>{4},
                                       std::vector<uint8_t>{0, 1, 2, 3});
        (void)out;
    }));
}

void test_bits_per_pixel_for() {
    CHECK_EQ(bits_per_pixel_for(1), 1);
    CHECK_EQ(bits_per_pixel_for(3), 2);
    CHECK_EQ(bits_per_pixel_for(15), 4);
    CHECK_EQ(bits_per_pixel_for(255), 8);
}

}  // namespace

int main() {
    test_bpp1_round_trip();
    test_bpp2_round_trip();
    test_bpp4_round_trip();
    test_bpp8_round_trip();
    test_unaligned_3x3_round_trip();
    test_unaligned_5x2_round_trip();
    test_encode_rejects_index_out_of_range();
    test_remap();
    test_bits_per_pixel_for();

    std::cout << "checks: " << g_checks << ", failures: " << g_failures
              << '\n';
    if (g_failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
    }
    return g_failures == 0 ? 0 : 1;
}
