#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Platform/Image.hpp"

using lemuria::platform::Image;

TEST_CASE("an image is saved as a PNG file") {
    const auto path = std::filesystem::temp_directory_path() / "lemuria_image_test.png";
    const Image image{1, 2, {255, 0, 0, 255, 0, 255, 0, 255}};

    const auto saved = lemuria::platform::save_png(image, path.string());

    INFO((saved ? "" : saved.error().message));
    REQUIRE(saved.has_value());
    std::ifstream file(path, std::ios::binary);
    std::array<char, 4> signature{};
    file.read(signature.data(), signature.size());
    file.close();
    std::filesystem::remove(path);
    CHECK(static_cast<std::uint8_t>(signature[0]) == 0x89);
    CHECK(signature[1] == 'P');
    CHECK(signature[2] == 'N');
    CHECK(signature[3] == 'G');
}

TEST_CASE("an image whose size does not match its pixels is refused") {
    const Image image{2, 2, {255, 0, 0, 255}};

    const auto saved = lemuria::platform::save_png(image, "never_written.png");

    REQUIRE_FALSE(saved.has_value());
    CHECK_FALSE(saved.error().message.empty());
}

TEST_CASE("an image without pixels is refused") {
    const auto saved = lemuria::platform::save_png(Image{}, "never_written.png");

    CHECK_FALSE(saved.has_value());
}
