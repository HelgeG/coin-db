#include "cli_app.hpp"

#include <CLI/CLI.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <istream>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "coins/coin.hpp"
#include "coins/coin_query.hpp"
#include "coins/collection_service.hpp"
#include "coins/currency_unit.hpp"
#include "coins/id.hpp"
#include "coins/image.hpp"
#include "coins/lookup_entry.hpp"
#include "coins/lookup_kind.hpp"
#include "coins/reference_link.hpp"
#include "coins/validation.hpp"
#include "coins/value_estimate.hpp"

namespace coins::cli {
namespace {

// Optional coin fields shared by `add` and `update`. Bound to CLI11 std::optional
// targets, so only options the user actually passes are engaged. The encoded
// fields (country/denomination/currency/mint/composition) are captured as free
// text — a code or a localized name — and resolved to lookup ids in the service
// layer (see apply_options), using the active language.
struct CoinFieldOptions {
  std::optional<std::string> country;
  std::optional<int> year;
  std::optional<int> year_to;
  std::optional<std::string> denomination;
  std::optional<double> face_value;
  std::optional<std::string> currency;
  std::optional<std::string> face_unit;
  std::optional<std::string> mint;
  std::optional<std::string> mint_mark;
  std::optional<std::string> composition;
  std::optional<double> weight_g;
  std::optional<double> diameter_mm;
  std::optional<std::string> grade_scale;
  std::optional<int> grade_numeric;
  std::optional<std::string> grade_label;
  std::optional<std::string> acquired_date;
  std::optional<double> acquired_price_eur;
  std::optional<std::string> acquired_source;
  std::optional<std::string> notes;
};

void add_coin_options(CLI::App* sub, CoinFieldOptions& o) {
  sub->add_option("--country", o.country, "Country of origin (code or name; required on add)");
  sub->add_option("--year", o.year, "Year, or start of a year range");
  sub->add_option("--year-to", o.year_to, "End of a year range");
  sub->add_option("--denomination", o.denomination, "Named piece (code or name)");
  sub->add_option("--face-value", o.face_value, "Numeric face value");
  sub->add_option("--currency", o.currency, "Currency (ISO 4217 code or name, e.g. NOK)");
  sub->add_option("--face-unit", o.face_unit,
                  "Currency unit for the face value (e.g. ore); major unit if omitted");
  sub->add_option("--mint", o.mint, "Mint (code or name)");
  sub->add_option("--mint-mark", o.mint_mark);
  sub->add_option("--composition", o.composition, "Metal/composition (code or name)");
  sub->add_option("--weight", o.weight_g, "Weight in grams");
  sub->add_option("--diameter", o.diameter_mm, "Diameter in mm");
  sub->add_option("--grade-scale", o.grade_scale, "e.g. Sheldon, Norwegian");
  sub->add_option("--grade-numeric", o.grade_numeric, "Numeric grade (e.g. Sheldon 65)");
  sub->add_option("--grade-label", o.grade_label, "Symbolic grade (e.g. MS, 1+)");
  sub->add_option("--acquired-date", o.acquired_date, "ISO 8601 date");
  sub->add_option("--acquired-price-eur", o.acquired_price_eur, "Price paid, in EUR");
  sub->add_option("--acquired-source", o.acquired_source);
  sub->add_option("--notes", o.notes);
}

// Resolves the free-text encoded fields to lookup ids and applies every provided
// option to `coin`. Resolution uses the active language and creates entries when
// no match exists (resolve-or-create).
void apply_options(const CoinFieldOptions& o, CollectionService& service, std::string_view lang,
                   Coin& coin) {
  if (o.country) {
    coin.country_id = service.resolve_lookup(LookupKind::Country, lang, *o.country).id;
  }
  if (o.year) {
    coin.year_from = *o.year;
    if (!o.year_to) coin.year_to = *o.year;
  }
  if (o.year_to) coin.year_to = *o.year_to;
  if (o.denomination) {
    coin.denomination_id =
        service.resolve_lookup(LookupKind::Denomination, lang, *o.denomination).id;
  }
  if (o.face_value) coin.face_value = o.face_value;
  if (o.currency) {
    coin.currency_id = service.resolve_lookup(LookupKind::Currency, lang, *o.currency).id;
  }
  // A face unit is only meaningful within the coin's currency. Resolve it within
  // whatever currency the coin now has; omitting it means the currency's major unit.
  if (o.face_unit && coin.currency_id.has_value()) {
    coin.face_unit_id = service.resolve_currency_unit(*coin.currency_id, lang, *o.face_unit).id;
  }
  if (o.mint) {
    coin.mint_id = service.resolve_lookup(LookupKind::Mint, lang, *o.mint).id;
  }
  if (o.mint_mark) coin.mint_mark = o.mint_mark;
  if (o.composition) {
    coin.composition_id = service.resolve_lookup(LookupKind::Composition, lang, *o.composition).id;
  }
  if (o.weight_g) coin.weight_g = o.weight_g;
  if (o.diameter_mm) coin.diameter_mm = o.diameter_mm;
  if (o.grade_scale) coin.grade_scale = o.grade_scale;
  if (o.grade_numeric) coin.grade_numeric = o.grade_numeric;
  if (o.grade_label) coin.grade_label = o.grade_label;
  if (o.acquired_date) coin.acquired_date = o.acquired_date;
  if (o.acquired_price_eur) coin.acquired_price_eur = o.acquired_price_eur;
  if (o.acquired_source) coin.acquired_source = o.acquired_source;
  if (o.notes) coin.notes = o.notes;
}

void print_validation(std::ostream& err, const ValidationErrors& errors) {
  err << "error: validation failed\n";
  for (const ValidationError& e : errors) {
    err << "  - " << e.field << ": " << e.message << "\n";
  }
}

SortField sort_field_from(const std::string& name) {
  if (name == "year") return SortField::Year;
  if (name == "country") return SortField::Country;
  if (name == "value") return SortField::ValueEur;
  return SortField::DateAdded;
}

std::string year_text(const Coin& coin) {
  if (coin.year_from == coin.year_to) return std::to_string(coin.year_from);
  return std::to_string(coin.year_from) + "-" + std::to_string(coin.year_to);
}

// Localized display name of a lookup entry by id, or "" when unset/missing.
std::string lookup_name(CollectionService& service, std::optional<Id> id, std::string_view lang) {
  if (!id) return "";
  const auto entry = service.lookup(*id);
  return entry ? entry->display_name(lang) : "";
}

// Required-country display name (the id is never unset for a valid coin, but
// fall back to "" defensively).
std::string country_name(CollectionService& service, const Coin& coin, std::string_view lang) {
  const auto entry = service.lookup(coin.country_id);
  return entry ? entry->display_name(lang) : "";
}

// Renders a coin's face value as e.g. "50 øre" (value + unit name) when a unit
// is present, else "0.5 <currency name>" using the currency, else just the
// number, else "". Returns "" when there is no face value.
std::string face_value_text(CollectionService& service, const Coin& coin, std::string_view lang) {
  if (!coin.face_value) return "";
  const std::string number = std::format("{}", *coin.face_value);
  if (coin.face_unit_id) {
    const auto unit = service.currency_unit(*coin.face_unit_id);
    if (unit) return number + " " + unit->display_name(lang);
  }
  if (coin.currency_id) {
    const std::string cur = lookup_name(service, coin.currency_id, lang);
    if (!cur.empty()) return number + " " + cur;
  }
  return number;
}

void print_opt(std::ostream& out, std::string_view label, const std::optional<std::string>& v) {
  if (v.has_value()) out << "  " << label << ": " << *v << "\n";
}
void print_opt(std::ostream& out, std::string_view label, const std::optional<double>& v) {
  if (v.has_value()) out << "  " << label << ": " << std::format("{}", *v) << "\n";
}
void print_opt(std::ostream& out, std::string_view label, const std::optional<int>& v) {
  if (v.has_value()) out << "  " << label << ": " << *v << "\n";
}
// Prints a non-empty string field (used for resolved lookup names).
void print_str(std::ostream& out, std::string_view label, const std::string& v) {
  if (!v.empty()) out << "  " << label << ": " << v << "\n";
}

std::optional<std::string> read_file(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return std::nullopt;
  return std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}  // namespace

int run(int argc, const char* const* argv, std::istream& in, std::ostream& out, std::ostream& err) {
  CLI::App app{"coins-db command-line interface", "coins"};
  app.require_subcommand(1);

  std::string data_dir = "coins-data";
  app.add_option("--data-dir", data_dir, "Data directory (database + image store)")
      ->capture_default_str();

  std::string lang = "en";
  app.add_option("--lang", lang, "Active language for vocabulary display/resolution (e.g. en, nb)")
      ->capture_default_str();

  int result = 0;

  // --- add ----------------------------------------------------------------
  auto* add = app.add_subcommand("add", "Add a coin");
  CoinFieldOptions add_opts;
  add_coin_options(add, add_opts);
  add->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    Coin coin;
    apply_options(add_opts, service, lang, coin);
    const auto created = service.add_coin(coin);
    if (!created) {
      print_validation(err, created.error());
      result = 1;
      return;
    }
    out << "Added coin " << created->id << "\n";
  });

  // --- list / search ------------------------------------------------------
  auto* list = app.add_subcommand("list", "List and search coins");
  CoinQuery query;
  std::string sort_name = "added";
  bool descending = false;
  list->add_option("--country", query.country, "Country (code or name)");
  list->add_option("--year-from", query.year_from);
  list->add_option("--year-to", query.year_to);
  list->add_option("--denomination", query.denomination, "Denomination (code or name)");
  list->add_option("--grade", query.grade_label);
  list->add_option("--metal", query.composition, "Composition/metal (code or name)");
  list->add_option("--min-eur", query.min_value_eur);
  list->add_option("--max-eur", query.max_value_eur);
  list->add_option("--text", query.text, "Free-text search");
  list->add_option("--sort", sort_name, "Sort by: added|year|country|value")
      ->check(CLI::IsMember({"added", "year", "country", "value"}));
  list->add_flag("--desc", descending, "Sort descending");
  list->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    query.sort_field = sort_field_from(sort_name);
    query.sort_direction = descending ? SortDirection::Descending : SortDirection::Ascending;
    query.lang = lang;
    const std::vector<Coin> coins = service.search(query);
    out << std::format("{:>5}  {:<16} {:<10} {:<16} {:>12}\n", "ID", "Country", "Year",
                       "Denomination", "Latest EUR");
    for (const Coin& coin : coins) {
      const auto latest = service.latest_estimate(coin.id);
      const std::string eur = latest ? std::format("{:.2f}", latest->amount_eur) : "-";
      out << std::format("{:>5}  {:<16} {:<10} {:<16} {:>12}\n", coin.id,
                         country_name(service, coin, lang), year_text(coin),
                         lookup_name(service, coin.denomination_id, lang), eur);
    }
    out << coins.size() << " coin(s)\n";
  });

  // --- show ---------------------------------------------------------------
  auto* show = app.add_subcommand("show", "Show a coin's full details");
  Id show_id = 0;
  show->add_option("id", show_id, "Coin id")->required();
  show->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    const auto coin = service.get_coin(show_id);
    if (!coin) {
      err << "error: no coin with id " << show_id << "\n";
      result = 1;
      return;
    }
    out << "Coin " << coin->id << "\n";
    out << "  Country: " << country_name(service, *coin, lang) << "\n";
    out << "  Year: " << year_text(*coin) << "\n";
    print_str(out, "Denomination", lookup_name(service, coin->denomination_id, lang));
    print_str(out, "Face value", face_value_text(service, *coin, lang));
    print_str(out, "Currency", lookup_name(service, coin->currency_id, lang));
    print_str(out, "Mint", lookup_name(service, coin->mint_id, lang));
    print_opt(out, "Mint mark", coin->mint_mark);
    print_str(out, "Composition", lookup_name(service, coin->composition_id, lang));
    print_opt(out, "Weight (g)", coin->weight_g);
    print_opt(out, "Diameter (mm)", coin->diameter_mm);
    print_opt(out, "Grade scale", coin->grade_scale);
    print_opt(out, "Grade numeric", coin->grade_numeric);
    print_opt(out, "Grade label", coin->grade_label);
    print_opt(out, "Acquired date", coin->acquired_date);
    print_opt(out, "Acquired price (EUR)", coin->acquired_price_eur);
    print_opt(out, "Acquired source", coin->acquired_source);
    print_opt(out, "Notes", coin->notes);

    const auto latest = service.latest_estimate(coin->id);
    if (latest) {
      out << std::format("  Latest estimate: {:.2f} EUR ({})\n", latest->amount_eur,
                         latest->estimated_at);
    }
    out << "  Estimate history: " << service.estimate_history(coin->id).size() << " entr(ies)\n";

    const auto coin_links = service.links(coin->id);
    for (const ReferenceLink& link : coin_links) {
      out << "  Link [" << link.id << "]: " << link.label << " -> " << link.url << "\n";
    }
    const auto coin_images = service.images(coin->id);
    for (const Image& image : coin_images) {
      out << "  Image [" << image.id << "]: " << image.stored_path << "\n";
    }
  });

  // --- update -------------------------------------------------------------
  auto* update = app.add_subcommand("update", "Update fields of an existing coin");
  Id update_id = 0;
  CoinFieldOptions update_opts;
  update->add_option("id", update_id, "Coin id")->required();
  add_coin_options(update, update_opts);
  update->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    auto coin = service.get_coin(update_id);
    if (!coin) {
      err << "error: no coin with id " << update_id << "\n";
      result = 1;
      return;
    }
    apply_options(update_opts, service, lang, *coin);
    const auto updated = service.update_coin(*coin);
    if (!updated) {
      print_validation(err, updated.error());
      result = 1;
      return;
    }
    out << "Updated coin " << update_id << "\n";
  });

  // --- delete -------------------------------------------------------------
  auto* remove = app.add_subcommand("delete", "Delete a coin (and its images)");
  Id delete_id = 0;
  bool assume_yes = false;
  remove->add_option("id", delete_id, "Coin id")->required();
  remove->add_flag("-y,--yes", assume_yes, "Skip the confirmation prompt");
  remove->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    if (!assume_yes) {
      out << "Delete coin " << delete_id << " and its images? [y/N] ";
      std::string answer;
      std::getline(in, answer);
      if (answer != "y" && answer != "Y") {
        out << "Aborted\n";
        return;
      }
    }
    if (service.delete_coin(delete_id)) {
      out << "Deleted coin " << delete_id << "\n";
    } else {
      err << "error: no coin with id " << delete_id << "\n";
      result = 1;
    }
  });

  // --- estimate -----------------------------------------------------------
  auto* estimate = app.add_subcommand("estimate", "Add a EUR value estimate to a coin");
  Id estimate_id = 0;
  double estimate_eur = 0.0;
  std::optional<std::string> estimate_date;
  std::optional<std::string> estimate_source;
  estimate->add_option("id", estimate_id, "Coin id")->required();
  estimate->add_option("--eur", estimate_eur, "Estimated value in EUR")->required();
  estimate->add_option("--date", estimate_date, "ISO 8601 date (defaults to today)");
  estimate->add_option("--source", estimate_source, "How the estimate was derived");
  estimate->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    ValueEstimate est;
    est.coin_id = estimate_id;
    est.amount_eur = estimate_eur;
    est.estimated_at = estimate_date.value_or(service.today());
    est.source = estimate_source;
    const auto added = service.add_estimate(est);
    if (!added) {
      print_validation(err, added.error());
      result = 1;
      return;
    }
    out << "Added estimate " << added->id << " to coin " << estimate_id << "\n";
  });

  // --- link add|list|rm ---------------------------------------------------
  auto* link = app.add_subcommand("link", "Manage reference links");
  link->require_subcommand(1);
  auto* link_add = link->add_subcommand("add", "Add a reference link");
  Id link_add_coin = 0;
  std::string link_label;
  std::string link_url;
  link_add->add_option("coin_id", link_add_coin, "Coin id")->required();
  link_add->add_option("--label", link_label, "Link label")->required();
  link_add->add_option("--url", link_url, "Link URL")->required();
  link_add->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    ReferenceLink reference;
    reference.coin_id = link_add_coin;
    reference.label = link_label;
    reference.url = link_url;
    const auto added = service.add_link(reference);
    if (!added) {
      print_validation(err, added.error());
      result = 1;
      return;
    }
    out << "Added link " << added->id << " to coin " << link_add_coin << "\n";
  });
  auto* link_list = link->add_subcommand("list", "List a coin's links");
  Id link_list_coin = 0;
  link_list->add_option("coin_id", link_list_coin, "Coin id")->required();
  link_list->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    for (const ReferenceLink& reference : service.links(link_list_coin)) {
      out << reference.id << "  " << reference.label << "  " << reference.url << "\n";
    }
  });
  auto* link_rm = link->add_subcommand("rm", "Remove a link by id");
  Id link_rm_id = 0;
  link_rm->add_option("link_id", link_rm_id, "Link id")->required();
  link_rm->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    if (service.remove_link(link_rm_id)) {
      out << "Removed link " << link_rm_id << "\n";
    } else {
      err << "error: no link with id " << link_rm_id << "\n";
      result = 1;
    }
  });

  // --- image add|list|rm --------------------------------------------------
  auto* image = app.add_subcommand("image", "Manage coin images");
  image->require_subcommand(1);
  auto* image_add = image->add_subcommand("add", "Attach an image (copies it into the store)");
  Id image_add_coin = 0;
  std::string image_file;
  std::optional<std::string> image_kind;
  std::optional<std::string> image_caption;
  image_add->add_option("coin_id", image_add_coin, "Coin id")->required();
  image_add->add_option("--file", image_file, "Path to the source image")->required();
  image_add->add_option("--kind", image_kind, "obverse|reverse|detail")
      ->check(CLI::IsMember({"obverse", "reverse", "detail"}));
  image_add->add_option("--caption", image_caption);
  image_add->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    std::optional<ImageKind> kind;
    if (image_kind) kind = image_kind_from_string(*image_kind);
    const auto added = service.add_image(image_add_coin, image_file, kind, image_caption);
    if (!added) {
      print_validation(err, added.error());
      result = 1;
      return;
    }
    out << "Added image " << added->id << " (" << added->stored_path << ")\n";
  });
  auto* image_list = image->add_subcommand("list", "List a coin's images");
  Id image_list_coin = 0;
  image_list->add_option("coin_id", image_list_coin, "Coin id")->required();
  image_list->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    for (const Image& img : service.images(image_list_coin)) {
      out << img.id << "  " << img.stored_path << "\n";
    }
  });
  auto* image_rm = image->add_subcommand("rm", "Remove an image by id");
  Id image_rm_id = 0;
  image_rm->add_option("image_id", image_rm_id, "Image id")->required();
  image_rm->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    if (service.remove_image(image_rm_id)) {
      out << "Removed image " << image_rm_id << "\n";
    } else {
      err << "error: no image with id " << image_rm_id << "\n";
      result = 1;
    }
  });

  // --- lookups ------------------------------------------------------------
  auto* lookups = app.add_subcommand("lookups", "List a vocabulary's entries (code<TAB>name)");
  std::string lookups_kind;
  lookups->add_option("kind", lookups_kind, "country|denomination|composition|mint|currency")
      ->required();
  lookups->callback([&] {
    const auto kind = lookup_kind_from_string(lookups_kind);
    if (!kind) {
      err << "error: unknown lookup kind '" << lookups_kind << "'\n";
      result = 1;
      return;
    }
    auto service = CollectionService::from_data_dir(data_dir);
    for (const LookupEntry& entry : service.lookups(*kind, lang)) {
      out << entry.code << "\t" << entry.display_name(lang) << "\n";
    }
  });

  // --- summary ------------------------------------------------------------
  auto* summary = app.add_subcommand("summary", "Show collection totals");
  std::string summary_type = std::string(coins::to_string(coins::kDefaultSummaryType));
  summary
      ->add_option("--type", summary_type,
                   "Breakdown: by_country|total_value|by_decade|by_grade|by_metal")
      ->check(CLI::IsMember({"by_country", "total_value", "by_decade", "by_grade", "by_metal"}));
  summary->callback([&] {
    const coins::SummaryType type =
        coins::summary_type_from_string(summary_type).value_or(coins::kDefaultSummaryType);
    auto service = CollectionService::from_data_dir(data_dir);
    const CollectionSummary totals = service.summary(type);
    out << "Coins: " << totals.coin_count << "\n";
    out << std::format("Total estimated value: {:.2f} EUR\n", totals.total_estimate_eur);

    if (type != coins::SummaryType::TotalValue) {
      const std::string_view heading = [type] {
        switch (type) {
          case coins::SummaryType::ByCountry:
            return "Coins by country";
          case coins::SummaryType::ByDecade:
            return "Coins by decade";
          case coins::SummaryType::ByGrade:
            return "Coins by grade";
          case coins::SummaryType::ByMetal:
            return "Coins by metal";
          case coins::SummaryType::TotalValue:
            return "";
        }
        return "";
      }();
      out << heading << ":\n";
      for (const SummaryBucket& bucket : totals.breakdown.buckets) {
        out << std::format("  {}: {}\n", bucket.label, bucket.coin_count);
      }
    }
  });

  // --- export -------------------------------------------------------------
  auto* do_export = app.add_subcommand("export", "Export the collection");
  std::string export_format = "json";
  std::optional<std::string> export_out;
  do_export->add_option("--format", export_format, "json|csv")
      ->check(CLI::IsMember({"json", "csv"}));
  do_export->add_option("--out", export_out, "Output file (stdout if omitted)");
  do_export->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    const std::string content =
        export_format == "csv" ? service.export_csv(lang) : service.export_json();
    if (export_out) {
      std::ofstream file(*export_out, std::ios::binary);
      if (!file) {
        err << "error: could not open " << *export_out << " for writing\n";
        result = 1;
        return;
      }
      file << content;
      out << "Exported to " << *export_out << "\n";
    } else {
      out << content;
    }
  });

  // --- import -------------------------------------------------------------
  auto* do_import = app.add_subcommand("import", "Import a JSON collection");
  std::string import_in;
  do_import->add_option("--in", import_in, "Input JSON file")->required();
  do_import->callback([&] {
    auto service = CollectionService::from_data_dir(data_dir);
    const auto content = read_file(import_in);
    if (!content) {
      err << "error: could not read " << import_in << "\n";
      result = 1;
      return;
    }
    const auto stats = service.import_json(*content);
    if (!stats) {
      print_validation(err, stats.error());
      result = 1;
      return;
    }
    out << std::format("Imported {} coin(s), {} estimate(s), {} link(s), {} image(s)\n",
                       stats->coins, stats->value_estimates, stats->reference_links, stats->images);
  });

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }
  return result;
}

}  // namespace coins::cli
