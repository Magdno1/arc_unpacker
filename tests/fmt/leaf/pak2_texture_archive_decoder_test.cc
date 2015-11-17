#include "fmt/leaf/pak2_texture_archive_decoder.h"
#include "test_support/catch.hh"
#include "test_support/decoder_support.h"
#include "test_support/file_support.h"
#include "test_support/image_support.h"

using namespace au;
using namespace au::fmt::leaf;

static const std::string dir = "tests/fmt/leaf/files/pak2-texture/";

static void do_test(
    const std::string &input_path,
    const std::vector<std::string> &expected_paths)
{
    const Pak2TextureArchiveDecoder decoder;
    const auto input_file = tests::zlib_file_from_path(dir + input_path);
    const auto actual_files = tests::unpack(decoder, *input_file);
    std::vector<std::shared_ptr<pix::Grid>> expected_images;
    for (const auto &path : expected_paths)
        expected_images.push_back(tests::image_from_path(dir + path));
    tests::compare_images(expected_images, actual_files);
}

TEST_CASE("Leaf PAK2 textures", "[fmt]")
{
    do_test(
        "title3-zlib",
        {
            "title3_000-out.png",
            "title3_001-out.png",
            "title3_002-out.png",
            "title3_003-out.png",
        });
}