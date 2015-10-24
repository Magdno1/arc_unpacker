#include <boost/algorithm/string.hpp>
#include <regex>
#include "fmt/registry.h"
#include "io/file_io.h"
#include "test_support/catch.hh"
#include "log.h"

using namespace au;

namespace
{
    struct regex_range final
    {
        regex_range(
            const std::regex &r, const std::string &content, const int group)
            : content_copy(content),
                it(content_copy.begin(), content_copy.end(), r, group)
        {
        }

        std::sregex_token_iterator begin()
        {
            return it;
        }

        std::sregex_token_iterator end()
        {
            return _end;
        }

        std::string content_copy;
        std::sregex_token_iterator it;
        std::sregex_token_iterator _end;
    };
}

static std::string read_gamelist_file()
{
    io::FileIO gamelist_file("GAMELIST.htm", io::FileMode::Read);
    return gamelist_file.read_to_eof().str();
}

TEST_CASE(
    "--fmt switches contain hyphens rather than underscores",
    "[core][fmt_core][docs]")
{
    const auto &registry = fmt::Registry::instance();
    for (const auto &name : registry.get_decoder_names())
    {
        INFO("Format contains underscore: " << name);
        REQUIRE(name.find_first_of("_") == name.npos);
    }
}

TEST_CASE("GAMELIST refers to valid --fmt switches", "[core][fmt_core][docs]")
{
    const auto content = read_gamelist_file();
    const auto &registry = fmt::Registry::instance();

    const std::regex fmt_regex(
        "--fmt=([^< ]*)",
        std::regex_constants::ECMAScript | std::regex_constants::icase);

    for (const auto fmt : regex_range(fmt_regex, content, 1))
    {
        INFO("Format not present in the registry: " << fmt);
        REQUIRE(registry.has_decoder(fmt));
    }
}

TEST_CASE(
    "GAMELIST is sorted alphabetically", "[core][fmt_core][docs]")
{
    const auto content = read_gamelist_file();
    const std::regex row_regex(
        "<tr>(([\r\n]|.)*?)</tr>", std::regex_constants::ECMAScript);
    const std::regex cell_regex(
        "<td>(.*)</td>", std::regex_constants::ECMAScript);

    size_t comparisons = 0;
    std::string last_sort_key;
    for (const auto row : regex_range(row_regex, content, 1))
    {
        std::vector<std::string> cells;
        for (const auto cell : regex_range(cell_regex, row, 1))
            cells.push_back(cell);
        if (cells.size() != 7)
            continue;
        const auto company = cells[3];
        const auto release_date = cells[4];
        const auto game_title = cells[5];
        auto sort_key = "[" + company + "][" + release_date + "]" + game_title;
        boost::algorithm::to_lower(sort_key);
        REQUIRE(sort_key > last_sort_key);
        last_sort_key = sort_key;
        comparisons++;
    }

    // sanity check that regexes actually work
    REQUIRE(comparisons > 10);
}
