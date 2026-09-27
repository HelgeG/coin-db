#include "cli_app.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

std::filesystem::path unique_temp_dir(std::string_view prefix) {
  std::random_device rd;
  const std::uint64_t token = (static_cast<std::uint64_t>(rd()) << 32) ^ rd();
  std::filesystem::path dir =
      std::filesystem::temp_directory_path() / (std::string{prefix} + std::to_string(token));
  std::filesystem::create_directories(dir);
  return dir;
}

struct CliResult {
  int code = 0;
  std::string out;
  std::string err;
};

class CliAppTest : public ::testing::Test {
 protected:
  CliAppTest() : data_dir_(unique_temp_dir("coins_cli_")) {}
  ~CliAppTest() override {
    std::error_code ec;
    std::filesystem::remove_all(data_dir_, ec);
  }

  // Runs the CLI with `args` (subcommand + options), targeting this test's data
  // directory, feeding `stdin_text` to prompts.
  CliResult run(const std::vector<std::string>& args, std::string stdin_text = "") {
    std::vector<std::string> full = {"coins", "--data-dir", data_dir_.string()};
    full.insert(full.end(), args.begin(), args.end());

    std::vector<const char*> argv;
    argv.reserve(full.size());
    for (const std::string& arg : full) {
      argv.push_back(arg.c_str());
    }

    std::istringstream in(std::move(stdin_text));
    std::ostringstream out;
    std::ostringstream err;
    const int code = coins::cli::run(static_cast<int>(argv.size()), argv.data(), in, out, err);
    return {code, out.str(), err.str()};
  }

  std::filesystem::path data_dir_;
};

TEST_F(CliAppTest, AddListShowFlow) {
  const CliResult added =
      run({"add", "--country", "Norway", "--year", "1963", "--denomination", "50 Ore"});
  EXPECT_EQ(added.code, 0);
  EXPECT_NE(added.out.find("Added coin 1"), std::string::npos);

  const CliResult listed = run({"list"});
  EXPECT_EQ(listed.code, 0);
  EXPECT_NE(listed.out.find("Norway"), std::string::npos);
  EXPECT_NE(listed.out.find("1963"), std::string::npos);

  const CliResult shown = run({"show", "1"});
  EXPECT_EQ(shown.code, 0);
  EXPECT_NE(shown.out.find("Country: Norway"), std::string::npos);
}

TEST_F(CliAppTest, AddWithoutCountryFailsValidation) {
  const CliResult added = run({"add", "--year", "1963"});
  EXPECT_EQ(added.code, 1);
  EXPECT_NE(added.err.find("country"), std::string::npos);
}

TEST_F(CliAppTest, DeletePromptAbortsOnNo) {
  ASSERT_EQ(run({"add", "--country", "Norway", "--year", "1963"}).code, 0);

  const CliResult aborted = run({"delete", "1"}, "n\n");
  EXPECT_EQ(aborted.code, 0);
  EXPECT_NE(aborted.out.find("Aborted"), std::string::npos);

  // The coin is still there.
  EXPECT_NE(run({"show", "1"}).out.find("Country: Norway"), std::string::npos);
}

TEST_F(CliAppTest, DeleteProceedsOnYesInput) {
  ASSERT_EQ(run({"add", "--country", "Norway", "--year", "1963"}).code, 0);

  const CliResult deleted = run({"delete", "1"}, "y\n");
  EXPECT_EQ(deleted.code, 0);
  EXPECT_NE(deleted.out.find("Deleted coin 1"), std::string::npos);

  EXPECT_EQ(run({"show", "1"}).code, 1);  // gone
}

TEST_F(CliAppTest, DeleteYesFlagSkipsPrompt) {
  ASSERT_EQ(run({"add", "--country", "Norway", "--year", "1963"}).code, 0);
  const CliResult deleted = run({"delete", "1", "--yes"});
  EXPECT_EQ(deleted.code, 0);
  EXPECT_NE(deleted.out.find("Deleted coin 1"), std::string::npos);
}

TEST_F(CliAppTest, EstimateThenSummary) {
  ASSERT_EQ(run({"add", "--country", "Norway", "--year", "1963"}).code, 0);
  ASSERT_EQ(run({"estimate", "1", "--eur", "100", "--date", "2026-01-01"}).code, 0);

  const CliResult summary = run({"summary"});
  EXPECT_EQ(summary.code, 0);
  EXPECT_NE(summary.out.find("Coins: 1"), std::string::npos);
  EXPECT_NE(summary.out.find("100.00 EUR"), std::string::npos);
  EXPECT_NE(summary.out.find("Coins by country:"), std::string::npos);
  EXPECT_NE(summary.out.find("Norway: 1"), std::string::npos);

  // A selected breakdown type changes the grouping heading.
  const CliResult by_decade = run({"summary", "--type", "by_decade"});
  EXPECT_EQ(by_decade.code, 0);
  EXPECT_NE(by_decade.out.find("Coins by decade:"), std::string::npos);
  EXPECT_NE(by_decade.out.find("1960s: 1"), std::string::npos);

  // total_value prints the headline only, no breakdown heading.
  const CliResult total = run({"summary", "--type", "total_value"});
  EXPECT_EQ(total.code, 0);
  EXPECT_NE(total.out.find("100.00 EUR"), std::string::npos);
  EXPECT_EQ(total.out.find("Coins by"), std::string::npos);

  // An invalid type is rejected by the option validator.
  EXPECT_NE(run({"summary", "--type", "bogus"}).code, 0);
}

TEST_F(CliAppTest, ExportThenImportIntoFreshCollection) {
  ASSERT_EQ(run({"add", "--country", "Norway", "--year", "1963"}).code, 0);

  const std::filesystem::path json_file = data_dir_ / "backup.json";
  const CliResult exported = run({"export", "--format", "json", "--out", json_file.string()});
  EXPECT_EQ(exported.code, 0);
  ASSERT_TRUE(std::filesystem::exists(json_file));

  // Import into a different data directory.
  const std::filesystem::path other_dir = unique_temp_dir("coins_cli_import_");
  std::vector<std::string> import_args = {"--data-dir", other_dir.string(), "import", "--in",
                                          json_file.string()};
  std::vector<std::string> full = {"coins"};
  full.insert(full.end(), import_args.begin(), import_args.end());
  std::vector<const char*> argv;
  for (const std::string& arg : full) {
    argv.push_back(arg.c_str());
  }
  std::istringstream in;
  std::ostringstream out;
  std::ostringstream err;
  const int code = coins::cli::run(static_cast<int>(argv.size()), argv.data(), in, out, err);
  EXPECT_EQ(code, 0);
  EXPECT_NE(out.str().find("Imported 1 coin"), std::string::npos);

  std::error_code ec;
  std::filesystem::remove_all(other_dir, ec);
}

}  // namespace
