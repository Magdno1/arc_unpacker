#include "fmt/playstation/gxt_image_archive_decoder.h"
#include "algo/range.h"
#include "err.h"
#include "util/file_from_image.h"

using namespace au;
using namespace au::fmt::playstation;

static const bstr magic = "GXT\x00"_b;

namespace
{
    enum class TextureBaseFormat : u32
    {
        Pvrt_1_2 = 0x80'00'00'00,
        Pvrt_1_4 = 0x81'00'00'00,
        Pvrt_2_2 = 0x82'00'00'00,
        Pvrt_2_4 = 0x83'00'00'00,
        Dxt_1_4  = 0x86'00'00'00,
        Dxt_3_8  = 0x87'00'00'00,
        Dxt_5_8  = 0x88'00'00'00,
    };

    enum class TextureType : u32
    {
        Swizzled      = 0x00'00'00'00,
        Cube          = 0x40'00'00'00,
        Linear        = 0x60'00'00'00,
        Tiled         = 0x80'00'00'00,
        LinearStrided = 0x0C'00'00'00,
    };

    struct ArchiveEntryImpl final : fmt::ArchiveEntry
    {
        size_t offset;
        size_t size;
        int palette_index;
        TextureType texture_type;
        TextureBaseFormat texture_base_format;
        size_t width;
        size_t height;
    };
}

fmt::NamingStrategy GxtImageArchiveDecoder::naming_strategy() const
{
    return fmt::NamingStrategy::Sibling;
}

bool GxtImageArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.seek(0).read(magic.size()) == magic;
}

std::unique_ptr<fmt::ArchiveMeta> GxtImageArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(magic.size());
    const auto version = input_file.stream.read_u32_le();
    if (version != 0x1000'0003)
        throw err::UnsupportedVersionError(version);

    const auto texture_count = input_file.stream.read_u32_le();
    const auto texture_data_offset = input_file.stream.read_u32_le();
    const auto texture_data_size = input_file.stream.read_u32_le();
    const auto texture_p4_count = input_file.stream.read_u32_le();
    const auto texture_p8_count = input_file.stream.read_u32_le();
    input_file.stream.skip(4);

    auto meta = std::make_unique<ArchiveMeta>();
    for (const auto i : algo::range(texture_count))
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        entry->offset = input_file.stream.read_u32_le();
        entry->size = input_file.stream.read_u32_le();
        entry->palette_index = input_file.stream.read_u32_le();
        input_file.stream.skip(4);
        entry->texture_type
            = static_cast<TextureType>(input_file.stream.read_u32_le());
        entry->texture_base_format
            = static_cast<TextureBaseFormat>(input_file.stream.read_u32_le());
        entry->width = input_file.stream.read_u16_le();
        entry->height = input_file.stream.read_u16_le();
        input_file.stream.skip(4);
        meta->entries.push_back(std::move(entry));
    }
    return meta;
}

std::unique_ptr<io::File> GxtImageArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const fmt::ArchiveMeta &m,
    const fmt::ArchiveEntry &e) const
{
    const auto entry = static_cast<const ArchiveEntryImpl*>(&e);

    if (entry->palette_index != -1)
        throw err::NotSupportedError("Paletted entries are not supported");
    if (entry->texture_type != TextureType::Linear)
        throw err::NotSupportedError("Only linear textures are supported");

    const auto data = input_file.stream.seek(entry->offset).read(entry->size);
    res::Image image(
        entry->width, entry->height, data, res::PixelFormat::Gray8);
    return util::file_from_image(image, entry->path);
}

static auto dummy
    = fmt::register_fmt<GxtImageArchiveDecoder>("playstation/gxt");
