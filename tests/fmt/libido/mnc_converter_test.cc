#include "fmt/libido/mnc_converter.h"
#include "test_support/catch.hh"
#include "test_support/file_support.h"
#include "test_support/image_support.h"

using namespace au;
using namespace au::fmt::libido;

TEST_CASE("Decoding Libido's MNC images works")
{
    MncConverter converter;
    auto input_file = tests::zlib_file_from_path(
        "tests/fmt/libido/files/mnc/test-zlib.MNC");
    auto expected_image = tests::image_from_path(
        "tests/fmt/libido/files/mnc/test-out.png");
    auto actual_image = tests::image_from_file(*converter.decode(*input_file));
    tests::compare_images(*expected_image, *actual_image);
}