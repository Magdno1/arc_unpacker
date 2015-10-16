#include "fmt/innocent_grey/iga_archive_decoder.h"
#include "err.h"
#include "util/range.h"

using namespace au;
using namespace au::fmt::innocent_grey;

static const bstr magic = "IGA0"_b;

namespace
{
    struct EntrySpec final
    {
        size_t name_offset;
        size_t name_size;
        size_t data_offset;
        size_t data_size;
    };

    struct ArchiveEntryImpl final : fmt::ArchiveEntry
    {
        size_t offset;
        size_t size;
    };
}

static u32 read_integer(io::IO &io)
{
    u32 ret = 0;
    while (!(ret & 1))
    {
        ret <<= 7;
        ret |= io.read_u8();
    }
    return ret >> 1;
}

bool IgaArchiveDecoder::is_recognized_impl(File &arc_file) const
{
    return arc_file.io.read(magic.size()) == magic;
}

std::unique_ptr<fmt::ArchiveMeta>
    IgaArchiveDecoder::read_meta_impl(File &arc_file) const
{
    arc_file.io.seek(magic.size());
    arc_file.io.skip(12);

    const auto table_size = read_integer(arc_file.io);
    const auto table_start = arc_file.io.tell();
    const auto table_end = table_start + table_size;
    EntrySpec *last_entry_spec = nullptr;
    std::vector<std::unique_ptr<EntrySpec>> entry_specs;
    while (arc_file.io.tell() < table_end)
    {
        auto spec = std::make_unique<EntrySpec>();
        spec->name_offset = read_integer(arc_file.io);
        spec->data_offset = read_integer(arc_file.io);
        spec->data_size = read_integer(arc_file.io);
        if (last_entry_spec)
        {
            last_entry_spec->name_size
                = spec->name_offset - last_entry_spec->name_offset;
        }
        last_entry_spec = spec.get();
        entry_specs.push_back(std::move(spec));
    }
    if (!last_entry_spec)
        return std::make_unique<ArchiveMeta>();

    const auto names_size = read_integer(arc_file.io);
    const auto names_start = arc_file.io.tell();
    last_entry_spec->name_size = names_size - last_entry_spec->name_offset;

    const auto data_offset = arc_file.io.size()
        - last_entry_spec->data_offset
        - last_entry_spec->data_size;

    auto meta = std::make_unique<ArchiveMeta>();
    for (const auto &spec : entry_specs)
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        arc_file.io.seek(names_start + spec->name_offset);
        for (auto i : util::range(spec->name_size))
            entry->name += read_integer(arc_file.io);
        entry->offset = data_offset + spec->data_offset;
        entry->size = spec->data_size;
        meta->entries.push_back(std::move(entry));
    }
    return meta;
}

std::unique_ptr<File> IgaArchiveDecoder::read_file_impl(
    File &arc_file, const ArchiveMeta &m, const ArchiveEntry &e) const
{
    const auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    arc_file.io.seek(entry->offset);
    auto data = arc_file.io.read(entry->size);
    for (auto i : util::range(data.size()))
        data[i] ^= (i + 2) & 0xFF;
    return std::make_unique<File>(entry->name, data);
}

static auto dummy = fmt::register_fmt<IgaArchiveDecoder>("innocent-grey/iga");