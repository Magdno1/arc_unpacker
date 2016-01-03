#include "dec/entis/eri_image_decoder.h"
#include "algo/format.h"
#include "algo/range.h"
#include "dec/entis/common/enums.h"
#include "dec/entis/common/gamma_decoder.h"
#include "dec/entis/common/huffman_decoder.h"
#include "dec/entis/common/nemesis_decoder.h"
#include "dec/entis/common/sections.h"
#include "dec/entis/image/lossless.h"
#include "err.h"

using namespace au;
using namespace au::dec::entis;

static const bstr magic1 = "Entis\x1A\x00\x00"_b;
static const bstr magic2 = "\x00\x01\x00\x03\x00\x00\x00\x00"_b;
static const bstr magic3 = "Entis Rasterized Image"_b;

static image::EriHeader read_header(
    io::IStream &input_stream, const common::SectionReader &section_reader)
{
    auto header_section = section_reader.get_section("Header");
    input_stream.seek(header_section.offset);
    common::SectionReader header_section_reader(input_stream);
    header_section = header_section_reader.get_section("ImageInf");
    input_stream.seek(header_section.offset);

    image::EriHeader header;
    header.version = input_stream.read_u32_le();
    header.transformation
        = static_cast<common::Transformation>(input_stream.read_u32_le());
    header.architecture
        = static_cast<common::Architecture>(input_stream.read_u32_le());

    header.format_type      = input_stream.read_u32_le();
    s32 width               = input_stream.read_u32_le();
    s32 height              = input_stream.read_u32_le();
    header.width            = std::abs(width);
    header.height           = std::abs(height);
    header.flip             = height > 0;
    header.bit_depth        = input_stream.read_u32_le();
    header.clipped_pixel    = input_stream.read_u32_le();
    header.sampling_flags   = input_stream.read_u32_le();
    header.quantumized_bits = input_stream.read_u64_le();
    header.allotted_bits    = input_stream.read_u64_le();
    header.blocking_degree  = input_stream.read_u32_le();
    header.lapped_block     = input_stream.read_u32_le();
    header.frame_transform  = input_stream.read_u32_le();
    header.frame_degree     = input_stream.read_u32_le();
    return header;
}

bool EriImageDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.read(magic1.size()) == magic1
        && input_file.stream.read(magic2.size()) == magic2
        && input_file.stream.read(magic3.size()) == magic3;
}

static bstr decode_pixel_data(
    const image::EriHeader &header, const bstr &encoded_pixel_data)
{
    std::unique_ptr<common::AbstractDecoder> decoder;

    if (header.architecture == common::Architecture::RunLengthGamma)
        decoder = std::make_unique<common::GammaDecoder>();
    else if (header.architecture == common::Architecture::RunLengthHuffman)
        decoder = std::make_unique<common::HuffmanDecoder>();
    else if (header.architecture == common::Architecture::Nemesis)
        decoder = std::make_unique<common::NemesisDecoder>();

    if (!decoder)
    {
        throw err::NotSupportedError(algo::format(
            "Architecture type %d not supported", header.architecture));
    }

    if (header.transformation != common::Transformation::Lossless)
    {
        throw err::NotSupportedError(algo::format(
            "Transformation type %d not supported", header.transformation));
    }

    decoder->set_input(encoded_pixel_data);
    return decode_lossless_pixel_data(header, *decoder);
}

res::Image EriImageDecoder::decode_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(0x40);

    common::SectionReader section_reader(input_file.stream);
    const auto header = read_header(input_file.stream, section_reader);
    if (header.version != 0x00020100 && header.version != 0x00020200)
        throw err::UnsupportedVersionError(header.version);

    auto stream_section = section_reader.get_section("Stream");
    input_file.stream.seek(stream_section.offset);
    common::SectionReader stream_section_reader(input_file.stream);

    const auto pixel_data_sections
        = stream_section_reader.get_sections("ImageFrm");
    if (!pixel_data_sections.size())
        throw err::CorruptDataError("No pixel data found");

    res::Image image(header.width, header.height * pixel_data_sections.size());

    for (const auto i : algo::range(pixel_data_sections.size()))
    {
        const auto &pixel_data_section = pixel_data_sections[i];
        input_file.stream.seek(pixel_data_section.offset);
        const auto pixel_data = decode_pixel_data(
            header, input_file.stream.read(pixel_data_section.size));

        // sometimes mismatches the depth reported by header
        const auto actual_depth
            = pixel_data.size() * 8 / (header.width * header.height);

        res::PixelFormat fmt;
        if (actual_depth == 32)
            fmt = res::PixelFormat::BGRA8888;
        else if (actual_depth == 24)
            fmt = res::PixelFormat::BGR888;
        else if (actual_depth == 8)
            fmt = res::PixelFormat::Gray8;
        else
            throw err::UnsupportedBitDepthError(actual_depth);

        res::Image subimage(header.width, header.height, pixel_data, fmt);
        if (header.flip)
            subimage.flip_vertically();
        image.overlay(
            subimage,
            0,
            i * header.height,
            res::Image::OverlayKind::OverwriteAll);
    }

    return image;
}

static auto _ = dec::register_decoder<EriImageDecoder>("entis/eri");
