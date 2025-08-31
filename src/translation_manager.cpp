#include "translation_manager.h"
#include "log.h"
#include <map>
#include <set>

namespace translation_manager {

// Runtime translation maps
static std::map<strings::i18n, TranslatableString> english_translations = {
    {strings::i18n::play,
     TranslatableString("play", "Main menu button to start a new game")},
    {strings::i18n::about,
     TranslatableString("about", "Main menu button to show game information")},
    {strings::i18n::exit,
     TranslatableString("exit", "Main menu button to quit the game")},
    {strings::i18n::loading,
     TranslatableString("Loading...", "Text shown while game is loading")},
    {strings::i18n::gameover,
     TranslatableString("game over", "Text shown when player loses")},
    {strings::i18n::victory,
     TranslatableString("victory!", "Text shown when player wins")},
    {strings::i18n::start,
     TranslatableString("start", "Button to begin gameplay")},
    {strings::i18n::back,
     TranslatableString("back",
                        "Navigation button to return to previous screen")},
    {strings::i18n::continue_game,
     TranslatableString("continue", "Button to continue after round ends")},
    {strings::i18n::quit,
     TranslatableString("quit", "Button to exit current game session")},
    {strings::i18n::settings,
     TranslatableString("settings",
                        "Main menu button to access game settings")},
    {strings::i18n::volume,
     TranslatableString("volume", "Generic volume setting label")},
    {strings::i18n::fullscreen,
     TranslatableString("fullscreen", "Checkbox to toggle fullscreen mode")},
    {strings::i18n::resolution,
     TranslatableString("resolution", "Dropdown to select screen resolution")},
    {strings::i18n::language,
     TranslatableString("language", "Dropdown to select game language")},

    // Additional UI strings
    {strings::i18n::round_settings,
     TranslatableString("Round Settings",
                        "Title for round configuration screen")},
    {strings::i18n::resume,
     TranslatableString("resume", "Button to unpause the game")},
    {strings::i18n::back_to_setup,
     TranslatableString("back to setup",
                        "Button to return to game setup from pause menu")},
    {strings::i18n::exit_game,
     TranslatableString("exit game",
                        "Button to quit current game from pause menu")},
    {strings::i18n::round_length,
     TranslatableString("Round Length",
                        "Label for round time duration setting")},
    {strings::i18n::allow_tag_backs,
     TranslatableString("Allow Tag Backs",
                        "Checkbox for tag-and-go game mode setting")},
    {strings::i18n::select_map,
     TranslatableString("select map", "Button to choose a map for the game")},
    {strings::i18n::master_volume,
     TranslatableString("Master Volume", "Slider for overall game volume")},
    {strings::i18n::music_volume,
     TranslatableString("Music Volume", "Slider for background music volume")},
    {strings::i18n::sfx_volume,
     TranslatableString("SFX Volume", "Slider for sound effects volume")},
    {strings::i18n::post_processing,
     TranslatableString("Post Processing",
                        "Checkbox to enable visual post-processing effects")},
    {strings::i18n::round_end,
     TranslatableString("Round End", "Title shown when a round finishes")},
    {strings::i18n::paused,
     TranslatableString("paused", "Large text shown when game is paused")},
    {strings::i18n::unknown,
     TranslatableString("Unknown", "Fallback text for unknown game states")},
    {strings::i18n::unlimited,
     TranslatableString("Unlimited", "Option for unlimited round time")},
    {strings::i18n::easy,
     TranslatableString("Easy", "AI difficulty level - easiest setting")},
    {strings::i18n::medium,
     TranslatableString("Medium", "AI difficulty level - moderate setting")},
    {strings::i18n::hard,
     TranslatableString("Hard", "AI difficulty level - challenging setting")},
    {strings::i18n::expert,
     TranslatableString("Expert", "AI difficulty level - hardest setting")}};

static std::map<strings::i18n, TranslatableString> korean_translations = {
    {strings::i18n::play,
     TranslatableString("시작", "새 게임을 시작하는 메인 메뉴 버튼")},
    {strings::i18n::about,
     TranslatableString("정보", "게임 정보를 보여주는 메인 메뉴 버튼")},
    {strings::i18n::exit,
     TranslatableString("종료", "게임을 종료하는 메인 메뉴 버튼")},
    {strings::i18n::loading,
     TranslatableString("로딩중...", "게임이 로딩 중일 때 표시되는 텍스트")},
    {strings::i18n::gameover,
     TranslatableString("게임 오버", "플레이어가 패배했을 때 표시되는 텍스트")},
    {strings::i18n::victory,
     TranslatableString("승리!", "플레이어가 승리했을 때 표시되는 텍스트")},
    {strings::i18n::start,
     TranslatableString("시작", "게임플레이를 시작하는 버튼")},
    {strings::i18n::back,
     TranslatableString("뒤로", "이전 화면으로 돌아가는 네비게이션 버튼")},
    {strings::i18n::continue_game,
     TranslatableString("계속", "라운드가 끝난 후 계속하는 버튼")},
    {strings::i18n::quit,
     TranslatableString("종료", "현재 게임 세션을 종료하는 버튼")},
    {strings::i18n::settings,
     TranslatableString("설정", "게임 설정에 접근하는 메인 메뉴 버튼")},
    {strings::i18n::volume,
     TranslatableString("볼륨", "일반적인 볼륨 설정 라벨")},
    {strings::i18n::fullscreen,
     TranslatableString("전체화면", "전체화면 모드를 토글하는 체크박스")},
    {strings::i18n::resolution,
     TranslatableString("해상도", "화면 해상도를 선택하는 드롭다운")},
    {strings::i18n::language,
     TranslatableString("언어 (Language)", "게임 언어를 선택하는 드롭다운")},

    // Additional UI strings
    {strings::i18n::round_settings,
     TranslatableString("라운드 설정", "라운드 구성 화면의 제목")},
    {strings::i18n::resume,
     TranslatableString("계속", "게임을 일시정지 해제하는 버튼")},
    {strings::i18n::back_to_setup,
     TranslatableString("설정으로 돌아가기",
                        "일시정지 메뉴에서 게임 설정으로 돌아가는 버튼")},
    {strings::i18n::exit_game,
     TranslatableString("게임 종료",
                        "일시정지 메뉴에서 현재 게임을 종료하는 버튼")},
    {strings::i18n::round_length,
     TranslatableString("라운드 길이", "라운드 시간 지속 설정의 라벨")},
    {strings::i18n::allow_tag_backs,
     TranslatableString("태그 백 허용",
                        "태그 앤 고 게임 모드 설정을 위한 체크박스")},
    {strings::i18n::select_map,
     TranslatableString("맵 선택", "게임용 맵을 선택하는 버튼")},
    {strings::i18n::master_volume,
     TranslatableString("마스터 볼륨", "전체 게임 볼륨을 위한 슬라이더")},
    {strings::i18n::music_volume,
     TranslatableString("음악 볼륨", "배경 음악 볼륨을 위한 슬라이더")},
    {strings::i18n::sfx_volume,
     TranslatableString("효과음 볼륨", "효과음 볼륨을 위한 슬라이더")},
    {strings::i18n::post_processing,
     TranslatableString("후처리", "시각적 후처리 효과를 활성화하는 체크박스")},
    {strings::i18n::round_end,
     TranslatableString("라운드 종료", "라운드가 끝날 때 표시되는 제목")},
    {strings::i18n::paused,
     TranslatableString("일시정지",
                        "게임이 일시정지되었을 때 표시되는 큰 텍스트")},
    {strings::i18n::unknown,
     TranslatableString("알 수 없음",
                        "알 수 없는 게임 상태를 위한 대체 텍스트")},
    {strings::i18n::unlimited,
     TranslatableString("무제한", "무제한 라운드 시간을 위한 옵션")},
    {strings::i18n::easy,
     TranslatableString("쉬움", "AI 난이도 - 가장 쉬운 설정")},
    {strings::i18n::medium,
     TranslatableString("보통", "AI 난이도 - 보통 설정")},
    {strings::i18n::hard,
     TranslatableString("어려움", "AI 난이도 - 도전적인 설정")},
    {strings::i18n::expert,
     TranslatableString("전문가", "AI 난이도 - 가장 어려운 설정")}};

// Get translations for a specific language
const std::map<strings::i18n, TranslatableString> &
TranslationManager::get_translations_for_language(Language language) const {
  switch (language) {
  case Language::English:
    return english_translations;
  case Language::Korean:
    return korean_translations;
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
    return it->second.get_text();
  }
  return "MISSING_TRANSLATION"; // Fallback if translation not found
}

TranslatableString
TranslationManager::get_translatable_string(strings::i18n key) const {
  // Get translations for current language
  const auto &translations = get_translations_for_language(current_language);

  auto it = translations.find(key);
  if (it != translations.end()) {
    return TranslatableString(it->second.get_text(),
                              it->second.get_description());
  }
  return TranslatableString("MISSING_TRANSLATION",
                            true); // Fallback with no_translate
}

// TranslatableString constructor implementation
TranslatableString::TranslatableString(const strings::i18n &key) {
  // Get translations for current language
  const auto &translations =
      TranslationManager::get().get_translations_for_language(
          TranslationManager::get().get_language());

  auto it = translations.find(key);
  if (it != translations.end()) {
    content = it->second.get_text();
    description = it->second.description;
  } else {
    content = "MISSING_TRANSLATION";
    description = "Translation not found";
  }
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

  for (const auto &lang : {Language::Korean}) {
    // Get all translations for this language
    const auto &translations = get_translations_for_language(lang);

    // Collect all unique characters from all translations
    std::string all_chars;
    for (auto it = translations.begin(); it != translations.end(); ++it) {
      all_chars += it->second.get_text();
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
