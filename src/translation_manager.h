#pragma once

#include "font_info.h"
#include "strings.h"
#include <afterhours/src/ecs.h>
#include <afterhours/src/plugins/translation.h>
#include <fmt/format.h>
#include <functional>
#include <string_view>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <string>
#include <vector>

namespace translation_manager {

using Language = afterhours::translation::Language;

enum struct i18nParam {
  number_count,
  number_time,
  player_name,
  number_ordinal,
  map_name,
  weapon_name,

  Count
};

const std::map<i18nParam, std::string> translation_param = {
    {i18nParam::number_count, "number_count"},
    {i18nParam::number_time, "number_time"},
    {i18nParam::player_name, "player_name"},
    {i18nParam::number_ordinal, "number_ordinal"},
    {i18nParam::map_name, "map_name"},
    {i18nParam::weapon_name, "weapon_name"},
};

FontID get_font_for_language_mapper(Language language);

using FontNameGetter = std::string (*)(FontID);

using TranslationPlugin =
    afterhours::translation::translation_plugin<strings::i18n, i18nParam,
                                                FontID, FontNameGetter>;

using TranslatableString = TranslationPlugin::TranslatableStringType;

inline TranslatableString make_translatable_string(strings::i18n key) {
  return TranslationPlugin::get_translatable_string(key);
}

inline std::string get_string(strings::i18n key) {
  return TranslationPlugin::get_string(key);
}

inline TranslatableString get_translatable_string(strings::i18n key) {
  return TranslationPlugin::get_translatable_string(key);
}

inline FontID get_font_for_language() {
  return TranslationPlugin::get_font_for_language(
      TranslationPlugin::get_language(), get_font_for_language_mapper);
}

inline void set_language(Language language) {
  TranslationPlugin::set_language(language);
}

inline Language get_language() { return TranslationPlugin::get_language(); }

inline std::string get_language_name(Language language) {
  return TranslationPlugin::get_language_name(language);
}

inline std::vector<std::string> get_available_languages() {
  return TranslationPlugin::get_available_languages();
}

inline size_t get_language_index(Language language) {
  return TranslationPlugin::get_language_index(language);
}

[[nodiscard]] inline TranslatableString NO_TRANSLATE(const std::string &s) {
  return TranslatableString{s, true};
}

[[nodiscard]] inline std::string
translate_formatted(const TranslatableString &trs) {
  return fmt::vformat(trs.underlying_TL_ONLY(),
                      trs.get_params(translation_param));
}

TranslationPlugin::LanguageMap get_translation_data();

} // namespace translation_manager
