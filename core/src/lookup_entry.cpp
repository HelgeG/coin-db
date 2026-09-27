#include "coins/lookup_entry.hpp"

#include <map>
#include <string>
#include <string_view>

#include "coins/currency_unit.hpp"

namespace coins {

namespace {

/// Shared fallback chain: the name for `lang`, else English, else any available
/// name, else the `code`.
std::string resolve_name(const std::map<Lang, std::string>& names, std::string_view lang,
                         const std::string& code) {
  if (const auto it = names.find(std::string(lang)); it != names.end()) {
    return it->second;
  }
  if (const auto it = names.find(std::string(kFallbackLang)); it != names.end()) {
    return it->second;
  }
  if (!names.empty()) {
    return names.begin()->second;
  }
  return code;
}

}  // namespace

std::string LookupEntry::display_name(std::string_view lang) const {
  return resolve_name(names, lang, code);
}

std::string CurrencyUnit::display_name(std::string_view lang) const {
  return resolve_name(names, lang, code);
}

}  // namespace coins
