#include "coins/lookup_service.hpp"

#include <cctype>
#include <string>
#include <utility>

namespace coins {
namespace {

/// True if `text` already looks like a bare code for `kind` (country: 2 or 4
/// letters; currency: 3 letters), so we can adopt it as the entry code.
bool looks_like_code(LookupKind kind, std::string_view text) {
  auto all_alpha = [](std::string_view s) {
    for (char c : s) {
      if (std::isalpha(static_cast<unsigned char>(c)) == 0) return false;
    }
    return !s.empty();
  };
  if (kind == LookupKind::Country) {
    return (text.size() == 2 || text.size() == 4) && all_alpha(text);
  }
  if (kind == LookupKind::Currency) {
    return text.size() == 3 && all_alpha(text);
  }
  return false;
}

std::string to_upper(std::string_view s) {
  std::string out(s);
  for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return out;
}

}  // namespace

std::string slugify(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  bool prev_dash = false;
  for (char c : text) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (std::isalnum(uc) != 0) {
      out.push_back(static_cast<char>(std::tolower(uc)));
      prev_dash = false;
    } else if (!prev_dash && !out.empty()) {
      out.push_back('-');
      prev_dash = true;
    }
  }
  while (!out.empty() && out.back() == '-') out.pop_back();
  if (out.empty()) out = "item";
  return out;
}

LookupService::LookupService(ILookupRepository& repo) : repo_(repo) {}

std::vector<LookupEntry> LookupService::list(LookupKind kind, std::string_view lang) {
  return repo_.list(kind, lang);
}

std::optional<LookupEntry> LookupService::find_by_id(Id id) { return repo_.find_by_id(id); }

std::string LookupService::unique_code(LookupKind kind, std::string_view text) {
  const std::string base = slugify(text);
  if (!repo_.find_by_code(kind, base)) {
    return base;
  }
  for (int i = 2;; ++i) {
    std::string candidate = base + "-" + std::to_string(i);
    if (!repo_.find_by_code(kind, candidate)) {
      return candidate;
    }
  }
}

LookupEntry LookupService::resolve_or_create(LookupKind kind, std::string_view lang,
                                             std::string_view text) {
  // 1) Exact code match (lets callers pass a known code like "NO" / "NOK").
  if (looks_like_code(kind, text)) {
    if (auto by_code = repo_.find_by_code(kind, to_upper(text))) {
      return *by_code;
    }
  }
  // 2) Case-insensitive name match in the active language.
  if (auto by_name = repo_.find_by_name(kind, lang, text)) {
    return *by_name;
  }
  // 3) Create a new entry.
  LookupEntry entry;
  entry.kind = kind;
  entry.code = looks_like_code(kind, text) ? to_upper(text) : unique_code(kind, text);
  entry.names[std::string(lang)] = std::string(text);
  return repo_.create(entry);
}

std::vector<CurrencyUnit> LookupService::list_units(Id currency_id) {
  return repo_.list_units(currency_id);
}

std::optional<CurrencyUnit> LookupService::find_unit_by_id(Id id) {
  return repo_.find_unit_by_id(id);
}

std::optional<CurrencyUnit> LookupService::major_unit(Id currency_id) {
  return repo_.major_unit(currency_id);
}

std::string LookupService::unique_unit_code(Id currency_id, std::string_view text) {
  const std::string base = slugify(text);
  if (!repo_.find_unit_by_code(currency_id, base)) {
    return base;
  }
  for (int i = 2;; ++i) {
    std::string candidate = base + "-" + std::to_string(i);
    if (!repo_.find_unit_by_code(currency_id, candidate)) {
      return candidate;
    }
  }
}

CurrencyUnit LookupService::resolve_or_create_unit(Id currency_id, std::string_view lang,
                                                   std::string_view text) {
  if (auto by_code = repo_.find_unit_by_code(currency_id, text)) {
    return *by_code;
  }
  if (auto by_name = repo_.find_unit_by_name(currency_id, lang, text)) {
    return *by_name;
  }
  CurrencyUnit unit;
  unit.currency_id = currency_id;
  unit.code = unique_unit_code(currency_id, text);
  unit.minor_per_unit = 1;
  unit.is_major = false;
  unit.names[std::string(lang)] = std::string(text);
  return repo_.create_unit(unit);
}

}  // namespace coins
