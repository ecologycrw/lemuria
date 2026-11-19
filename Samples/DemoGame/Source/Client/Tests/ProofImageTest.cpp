#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DemoClient/ProofImage.hpp"
#include "DemoClient/ProofScene.hpp"
#include "Lemuria/Platform/Image.hpp"

using demo_client::ProofImage;
using lemuria::platform::Image;

namespace {

constexpr std::size_t kBytesPerPixel = 4;

[[nodiscard]] ProofImage render() {
    auto proof = demo_client::render_proof_image({});
    INFO((proof ? "" : proof.error().message));
    REQUIRE(proof.has_value());
    return *proof;
}

// Saves the image and gives back the bytes that reached the file.
[[nodiscard]] std::vector<char> saved_bytes(const Image& image, const std::string& file_name) {
    const auto path = std::filesystem::temp_directory_path() / file_name;
    const auto saved = lemuria::platform::save_png(image, path.string());
    INFO((saved ? "" : saved.error().message));
    REQUIRE(saved.has_value());

    std::ifstream file(path, std::ios::binary);
    std::vector<char> bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    file.close();
    std::filesystem::remove(path);
    return bytes;
}

[[nodiscard]] std::size_t count_pixels_unlike_the_first(const Image& image) {
    std::size_t different = 0;
    for (std::size_t offset = 0; offset + kBytesPerPixel <= image.pixels.size(); offset += kBytesPerPixel) {
        const bool same = std::equal(image.pixels.begin(), image.pixels.begin() + kBytesPerPixel,
                                     image.pixels.begin() + static_cast<std::ptrdiff_t>(offset));
        different += same ? 0U : 1U;
    }
    return different;
}

} // namespace

TEST_CASE("the proof scene gives identical images twice in a row", "[0000-CA-19]") {
    const ProofImage first = render();
    const ProofImage second = render();

    // Compared outside the check, so a failure does not print millions of bytes.
    const bool same_pixels = first.image.pixels == second.image.pixels;
    CHECK(same_pixels);

    const auto first_file = saved_bytes(first.image, "lemuria_proof_image_first.png");
    const auto second_file = saved_bytes(second.image, "lemuria_proof_image_second.png");
    CHECK_FALSE(first_file.empty());
    const bool same_files = first_file == second_file;
    CHECK(same_files);
}

TEST_CASE("the proof image has the agreed size and shows more than one color", "[0000-CA-19]") {
    const ProofImage proof = render();

    CHECK(proof.image.width == demo_client::kProofImageWidth);
    CHECK(proof.image.height == demo_client::kProofImageHeight);
    CHECK_FALSE(proof.graphics_api.empty());
    CHECK(count_pixels_unlike_the_first(proof.image) > 0);
}
