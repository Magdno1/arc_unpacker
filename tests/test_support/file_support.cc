#include "test_support/catch.hh"
#include "test_support/file_support.h"

using namespace au;

std::shared_ptr<File> tests::file_from_path(
    const boost::filesystem::path &path)
{
    return std::shared_ptr<File>(new File(path, io::FileMode::Read));
}

std::shared_ptr<File> tests::create_file(
    const std::string &name, const bstr &data)
{
    std::shared_ptr<File> f(new File);
    f->name = name;
    f->io.write(data);
    return f;
}

void tests::compare_files(
    const std::vector<std::shared_ptr<File>> &expected_files,
    const std::vector<std::shared_ptr<File>> &actual_files,
    bool compare_file_names)
{
    REQUIRE(actual_files.size() == expected_files.size());
    for (size_t i = 0; i < expected_files.size(); i++)
    {
        auto &expected_file = expected_files[i];
        auto &actual_file = actual_files[i];
        tests::compare_files(*expected_file, *actual_file, compare_file_names);
    }
}

void tests::compare_files(
    const File &expected_file, const File &actual_file, bool compare_file_names)
{
    if (compare_file_names)
        REQUIRE(expected_file.name == actual_file.name);
    REQUIRE(expected_file.io.size() == actual_file.io.size());
    expected_file.io.seek(0);
    actual_file.io.seek(0);
    auto expected_content = expected_file.io.read_to_eof();
    auto actual_content = actual_file.io.read_to_eof();
    INFO("Expected content: " << expected_content.str());
    INFO("Actual content: " << actual_content.str());
    REQUIRE(expected_content == actual_content);
}