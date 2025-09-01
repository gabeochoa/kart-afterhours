#pragma once

#include "rl.h"
//
#include "font_info.h"
#include "preload.h"
#include "strings.h"
#include <afterhours/src/plugins/autolayout.h>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <string>

namespace translation_manager {

// Language enum for type safety
enum struct Language { English, Korean, Japanese };

enum struct i18nParam {
  number_count,
  number_time,
  player_name,
  number_ordinal,
  map_name,
  weapon_name,

  Count
};

// Parameter name mapping for fmt::format
const std::map<i18nParam, std::string> translation_param = {
    {i18nParam::number_count, "number_count"},
    {i18nParam::number_time, "number_time"},
    {i18nParam::player_name, "player_name"},
    {i18nParam::number_ordinal, "number_ordinal"},
    {i18nParam::map_name, "map_name"},
    {i18nParam::weapon_name, "weapon_name"},
};

class TranslatableString {
public:
  static constexpr int MAX_LENGTH = 100;

  explicit TranslatableString() {}
  explicit TranslatableString(const std::string &s) : content(s) {}
  explicit TranslatableString(const std::string &s, const std::string &desc)
      : content(s), description(desc) {}
  explicit TranslatableString(const strings::i18n &key);
  explicit TranslatableString(const std::string &s, bool ignore_translate)
      : content(s), no_translate(ignore_translate) {}

  [[nodiscard]] bool skip_translate() const { return no_translate; }
  [[nodiscard]] bool empty() const { return content.empty(); }
  [[nodiscard]] const char *debug() const { return content.c_str(); }
  [[nodiscard]] const char *underlying_TL_ONLY() const {
    return content.c_str();
  }

  [[nodiscard]] const std::string &str() const { return content; }
  [[nodiscard]] const std::string &get_description() const {
    return description;
  }

  [[nodiscard]] const std::string &get_text() const { return content; }
  [[nodiscard]] size_t size() const { return content.size(); }
  void resize(size_t len) { content.resize(len); }

  // Parameter setting methods
  auto &set_param(const i18nParam &param, const std::string &arg) {
    if (!formatted)
      formatted = true;
    params[param] = arg;
    return *this;
  }

  template <typename T> auto &set_param(const i18nParam &param, const T &arg) {
    return set_param(param, fmt::format("{}", arg));
  }

  auto &set_param(const i18nParam &param, const TranslatableString &arg) {
    return set_param(param, fmt::format("{}", arg.underlying_TL_ONLY()));
  }

  [[nodiscard]] bool is_formatted() const { return formatted; }

  // Get formatted parameters for fmt::format
  fmt::dynamic_format_arg_store<fmt::format_context> get_params() const {
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    for (const auto &kv : params) {
      if (translation_param.contains(kv.first)) {
        const char *param_name = translation_param.at(kv.first).c_str();
        store.push_back(fmt::arg(param_name, kv.second));
      }
    }
    return store;
  }

  // Implicit conversion to string for backward compatibility
  operator std::string() const {
    if (skip_translate())
      return underlying_TL_ONLY();
    if (is_formatted()) {
      return fmt::vformat(underlying_TL_ONLY(), get_params());
    }
    return underlying_TL_ONLY();
  }

private:
  std::string content;
  std::string description;
  std::map<i18nParam, std::string> params;
  bool formatted = false;
  bool no_translate = false;
};

// Helper function to create parameter for fmt::format
template <typename T>
fmt::detail::named_arg<char, T> create_param(const i18nParam &param,
                                             const T &arg) {
  if (!translation_param.contains(param)) {
    // Fallback to enum name if parameter missing
    return fmt::arg("unknown", arg);
  }
  const char *param_name = translation_param.at(param).c_str();
  return fmt::arg(param_name, arg);
}

// Simple translation manager
class TranslationManager {
public:
  static TranslationManager &get() {
    static TranslationManager instance;
    return instance;
  }

  // Get translated string
  std::string get_string(strings::i18n key) const;

  TranslatableString get_translatable_string(strings::i18n key) const;

  std::map<strings::i18n, TranslatableString>::const_iterator
  find_translation(strings::i18n key) const;

  // Get translations for a specific language
  const std::map<strings::i18n, TranslatableString> &
  get_translations_for_language(Language language) const;

  // Get appropriate font ID for the current language
  FontID get_font_for_language() const {
    switch (current_language) {
    case Language::Korean:
      return FontID::Korean;
    case Language::Japanese:
      return FontID::Japanese;
    case Language::English:
    default:
      return FontID::English;
    }
  }

  // Set language
  void set_language(Language language);

  // Get current language
  Language get_language() const { return current_language; }

  // Get language name for display
  std::string get_language_name() const {
    return get_language_name(current_language);
  }

  // Get language name for a specific language
  static std::string get_language_name(Language language) {
    return std::string(magic_enum::enum_name(language));
  }

  // Get vector of all available language names
  static std::vector<std::string> get_available_languages() {
    std::vector<std::string> languages;
    auto enum_values = magic_enum::enum_values<Language>();
    for (auto lang : enum_values) {
      languages.push_back(std::string(magic_enum::enum_name(lang)));
    }
    return languages;
  }

  // Get index of a specific language in the available languages list
  static size_t get_language_index(Language language) {
    auto index = magic_enum::enum_index(language);
    return index.value_or(0);
  }

  // Load CJK fonts for all the strings this manager needs
  void load_cjk_fonts(afterhours::ui::FontManager &font_manager,
                      const std::string &font_file) const;

private:
  TranslationManager();
  Language current_language;
};

// Global get_string function
inline std::string get_string(strings::i18n key) {
  return TranslationManager::get().get_string(key);
}

inline TranslatableString get_translatable_string(strings::i18n key) {
  return TranslationManager::get().get_translatable_string(key);
}

// Global get_font_for_language function
inline FontID get_font_for_language() {
  return TranslationManager::get().get_font_for_language();
}

// Global set_language function
inline void set_language(Language language) {
  TranslationManager::get().set_language(language);
}

// Global get_language function
inline Language get_language() {
  return TranslationManager::get().get_language();
}

// Global get_language_name function
inline std::string get_language_name(translation_manager::Language language) {
  return TranslationManager::get_language_name(language);
}

// Global get_available_languages function
inline std::vector<std::string> get_available_languages() {
  return TranslationManager::get_available_languages();
}

// Global get_language_index function
inline size_t get_language_index(translation_manager::Language language) {
  return TranslationManager::get_language_index(language);
}

[[nodiscard]] inline TranslatableString NO_TRANSLATE(const std::string &s) {
  return TranslatableString{s, true};
}

[[nodiscard]] inline std::string
translate_formatted(const TranslatableString &trs) {
  return fmt::vformat(trs.underlying_TL_ONLY(), trs.get_params());
}

} // namespace translation_manager
