#pragma once

#include "preload.h"
#include "rl.h"
#include "strings.h"
#include <afterhours/src/plugins/autolayout.h>
#include <string>

namespace translation_manager {

// Language enum for type safety
enum struct Language { English, Korean, Chinese, Japanese };

// Translation entry with description for translators
struct TranslationEntry {
  std::string text;
  std::string description;

  TranslationEntry(const std::string &t, const std::string &d = "")
      : text(t), description(d) {}

  // Allow implicit conversion to string for backward compatibility
  operator std::string() const { return text; }
};

// Simple translation manager
class TranslationManager {
public:
  static TranslationManager &get() {
    static TranslationManager instance;
    return instance;
  }

  // Get translated string
  std::string get_string(strings::i18n key) const;

  // Get translation entry with description
  TranslationEntry get_translation_entry(strings::i18n key) const;

  // Get appropriate font ID for the current language
  FontID get_font_for_language() const {
    switch (current_language) {
    case Language::Korean:
      return FontID::CJK;
    case Language::Chinese:
      return FontID::CJK;
    case Language::Japanese:
      return FontID::CJK;
    case Language::English:
    default:
      return FontID::EQPro;
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
    switch (language) {
    case Language::Korean:
      return "Korean";
    case Language::Chinese:
      return "Chinese";
    case Language::Japanese:
      return "Japanese";
    case Language::English:
    default:
      return "English";
    }
  }

  // Get vector of all available language names
  static std::vector<std::string> get_available_languages() {
    return {get_language_name(Language::English),
            get_language_name(Language::Korean),
            get_language_name(Language::Chinese),
            get_language_name(Language::Japanese)};
  }

  // Get index of a specific language in the available languages list
  static size_t get_language_index(Language language) {
    switch (language) {
    case Language::English:
      return 0;
    case Language::Korean:
      return 1;
    case Language::Chinese:
      return 2;
    case Language::Japanese:
      return 3;
    default:
      return 0;
    }
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

// Global get_translation_entry function
inline TranslationEntry get_translation_entry(strings::i18n key) {
  return TranslationManager::get().get_translation_entry(key);
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

} // namespace translation_manager
