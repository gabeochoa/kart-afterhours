#include "translation_manager.h"
#include "log.h"
#include <map>
#include <set>

namespace translation_manager {

// Runtime translation maps
static std::map<strings::i18n, std::string> english_translations = {
    {strings::i18n::play, "play"},
    {strings::i18n::about, "about"},
    {strings::i18n::exit, "exit"},
    {strings::i18n::loading, "Loading..."},
    {strings::i18n::gameover, "game over"},
    {strings::i18n::victory, "victory!"},
    {strings::i18n::start, "start"},
    {strings::i18n::back, "back"},
    {strings::i18n::continue_game, "continue"},
    {strings::i18n::quit, "quit"},
    {strings::i18n::settings, "settings"},
    {strings::i18n::volume, "volume"},
    {strings::i18n::fullscreen, "fullscreen"},
    {strings::i18n::resolution, "resolution"},
    {strings::i18n::language, "language"}};

static std::map<strings::i18n, std::string> korean_translations = {
    {strings::i18n::play, "시작"},
    {strings::i18n::about, "정보"},
    {strings::i18n::exit, "종료"},
    {strings::i18n::loading, "로딩중..."},
    {strings::i18n::gameover, "게임 오버"},
    {strings::i18n::victory, "승리!"},
    {strings::i18n::start, "시작"},
    {strings::i18n::back, "뒤로"},
    {strings::i18n::continue_game, "계속"},
    {strings::i18n::quit, "종료"},
    {strings::i18n::settings, "설정"},
    {strings::i18n::volume, "볼륨"},
    {strings::i18n::fullscreen, "전체화면"},
    {strings::i18n::resolution, "해상도"},
    {strings::i18n::language, "언어 (Language)"}};

static std::map<strings::i18n, std::string> chinese_translations = {
    {strings::i18n::play, "开始"},
    {strings::i18n::about, "关于"},
    {strings::i18n::exit, "退出"},
    {strings::i18n::loading, "加载中..."},
    {strings::i18n::gameover, "游戏结束"},
    {strings::i18n::victory, "胜利!"},
    {strings::i18n::start, "开始"},
    {strings::i18n::back, "返回"},
    {strings::i18n::continue_game, "继续"},
    {strings::i18n::quit, "退出"},
    {strings::i18n::settings, "设置"},
    {strings::i18n::volume, "音量"},
    {strings::i18n::fullscreen, "全屏"},
    {strings::i18n::resolution, "分辨率"},
    {strings::i18n::language, "语言 (Language)"}};

static std::map<strings::i18n, std::string> japanese_translations = {
    {strings::i18n::play, "プレイ"},
    {strings::i18n::about, "について"},
    {strings::i18n::exit, "終了"},
    {strings::i18n::loading, "読み込み中..."},
    {strings::i18n::gameover, "ゲームオーバー"},
    {strings::i18n::victory, "勝利!"},
    {strings::i18n::start, "開始"},
    {strings::i18n::back, "戻る"},
    {strings::i18n::continue_game, "続行"},
    {strings::i18n::quit, "終了"},
    {strings::i18n::settings, "設定"},
    {strings::i18n::volume, "音量"},
    {strings::i18n::fullscreen, "フルスクリーン"},
    {strings::i18n::resolution, "解像度"},
    {strings::i18n::language, "言語 (Language)"}};

// Get translations for a specific language
static const std::map<strings::i18n, std::string> &
get_translations_for_language(Language language) {
  switch (language) {
  case Language::English:
    return english_translations;
  case Language::Korean:
    return korean_translations;
  case Language::Chinese:
    return chinese_translations;
  case Language::Japanese:
    return japanese_translations;
  default:
    return english_translations;
  }
}

TranslationManager::TranslationManager() {
  set_language(Language::English); // Default to English
}

std::string TranslationManager::get_string(strings::i18n key) const {
  // Get translations for current language
  const auto &translations = get_translations_for_language(current_language);

  auto it = translations.find(key);
  if (it != translations.end()) {
    return it->second;
  }
  return "MISSING_TRANSLATION"; // Fallback if translation not found
}

void TranslationManager::set_language(Language language) {
  current_language = language;
  log_info("Language set to: {}", get_language_name());
}

// Load CJK fonts for all the strings this manager needs
void TranslationManager::load_cjk_fonts(
    afterhours::ui::FontManager &font_manager,
    const std::string &font_file) const {
  // Helper function to get font name from FontID
  auto get_font_name = [](FontID id) -> std::string {
    switch (id) {
    case FontID::CJK:
      return "notosanskr";
    default:
      return "eqpro";
    }
  };

  // Collect all unique codepoints from all CJK languages
  std::set<int> all_codepoints;

  for (const auto &lang :
       {Language::Korean, Language::Chinese, Language::Japanese}) {
    // Get all translations for this language
    const auto &translations = get_translations_for_language(lang);

    // Collect all unique characters from all translations
    std::string all_chars;
    for (auto it = translations.begin(); it != translations.end(); ++it) {
      all_chars += it->second;
    }

    // Extract codepoints
    size_t pos = 0;
    while (pos < all_chars.length()) {
      const unsigned char *bytes =
          reinterpret_cast<const unsigned char *>(all_chars.c_str() + pos);
      int codepoint = 0;
      int bytes_consumed = 0;

      // Simple UTF-8 decoding for the characters we need
      if (bytes[0] < 0x80) {
        codepoint = bytes[0];
        bytes_consumed = 1;
      } else if ((bytes[0] & 0xE0) == 0xC0 && pos + 1 < all_chars.length()) {
        codepoint = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
        bytes_consumed = 2;
      } else if ((bytes[0] & 0xF0) == 0xE0 && pos + 2 < all_chars.length()) {
        codepoint = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) |
                    (bytes[2] & 0x3F);
        bytes_consumed = 3;
      }

      if (bytes_consumed > 0 && codepoint > 0) {
        all_codepoints.insert(codepoint);
        pos += bytes_consumed;
      } else {
        pos++; // Skip invalid sequences
      }
    }
  }

  // Load all codepoints into a single font
  if (!all_codepoints.empty()) {
    FontID font_id = FontID::CJK;
    std::string font_name = get_font_name(font_id);

    // Convert set to vector
    std::vector<int> codepoints(all_codepoints.begin(), all_codepoints.end());

    font_manager.load_font_with_codepoints(font_name, font_file.c_str(),
                                           codepoints.data(),
                                           static_cast<int>(codepoints.size()));

    log_info("Loaded {} font with {} total codepoints for all CJK languages",
             font_name, codepoints.size());
  }
}

} // namespace translation_manager
