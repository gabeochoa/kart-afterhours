#include "translation_manager.h"
#include "log.h"
#include "magic_enum/magic_enum.hpp"
#include "resources.h"
#include <map>
#include <set>

namespace translation_manager {

// Runtime translation maps
static std::map<strings::i18n, TranslatableString> english_translations = {
    {strings::i18n::play, translation_manager::TranslatableString(
                              "play", "Main menu button to start a new game")},
    {strings::i18n::about,
     translation_manager::TranslatableString(
         "about", "Main menu button to show game information")},
    {strings::i18n::exit, translation_manager::TranslatableString(
                              "exit", "Main menu button to quit the game")},
    {strings::i18n::loading,
     translation_manager::TranslatableString(
         "Loading...", "Text shown while game is loading")},
    {strings::i18n::gameover, translation_manager::TranslatableString(
                                  "game over", "Text shown when player loses")},
    {strings::i18n::victory, translation_manager::TranslatableString(
                                 "victory!", "Text shown when player wins")},
    {strings::i18n::start, translation_manager::TranslatableString(
                               "start", "Button to begin gameplay")},
    {strings::i18n::back,
     translation_manager::TranslatableString(
         "back", "Navigation button to return to previous screen")},
    {strings::i18n::continue_game,
     translation_manager::TranslatableString(
         "continue", "Button to continue after round ends")},
    {strings::i18n::quit, translation_manager::TranslatableString(
                              "quit", "Button to exit current game session")},
    {strings::i18n::settings,
     translation_manager::TranslatableString(
         "settings", "Main menu button to access game settings")},
    {strings::i18n::volume, translation_manager::TranslatableString(
                                "volume", "Generic volume setting label")},
    {strings::i18n::fullscreen,
     translation_manager::TranslatableString(
         "fullscreen", "Checkbox to toggle fullscreen mode")},
    {strings::i18n::resolution,
     translation_manager::TranslatableString(
         "resolution", "Dropdown to select screen resolution")},
    {strings::i18n::language,
     translation_manager::TranslatableString(
         "language", "Dropdown to select game language")},

    // Additional UI strings
    {strings::i18n::round_settings,
     translation_manager::TranslatableString(
         "round settings", "Title for round configuration screen")},
    {strings::i18n::resume, translation_manager::TranslatableString(
                                "resume", "Button to unpause the game")},
    {strings::i18n::back_to_setup,
     translation_manager::TranslatableString(
         "back to setup", "Button to return to game setup from pause menu")},
    {strings::i18n::exit_game,
     translation_manager::TranslatableString(
         "exit game", "Button to quit current game from pause menu")},
    {strings::i18n::round_length,
     translation_manager::TranslatableString(
         "round length", "Label for round time duration setting")},
    {strings::i18n::allow_tag_backs,
     translation_manager::TranslatableString(
         "allow tag backs", "Checkbox for tag-and-go game mode setting")},
    {strings::i18n::select_map,
     translation_manager::TranslatableString(
         "select map", "Button to choose a map for the game")},
    {strings::i18n::master_volume,
     translation_manager::TranslatableString("master volume",
                                             "Slider for overall game volume")},
    {strings::i18n::music_volume,
     translation_manager::TranslatableString(
         "music volume", "Slider for background music volume")},
    {strings::i18n::sfx_volume,
     translation_manager::TranslatableString(
         "sfx volume", "Slider for sound effects volume")},
    {strings::i18n::post_processing,
     translation_manager::TranslatableString(
         "post processing",
         "Checkbox to enable visual post-processing effects")},
    {strings::i18n::round_end,
     translation_manager::TranslatableString(
         "round end", "Title shown when a round finishes")},
    {strings::i18n::paused,
     translation_manager::TranslatableString(
         "paused", "Large text shown when game is paused")},
    {strings::i18n::unknown,
     translation_manager::TranslatableString(
         "unknown", "Fallback text for unknown game states")},
    {strings::i18n::unlimited,
     translation_manager::TranslatableString(
         "unlimited", "Option for unlimited round time")},
    {strings::i18n::easy, translation_manager::TranslatableString(
                              "easy", "AI difficulty level - easiest setting")},
    {strings::i18n::medium,
     translation_manager::TranslatableString(
         "medium", "AI difficulty level - moderate setting")},
    {strings::i18n::hard,
     translation_manager::TranslatableString(
         "hard", "AI difficulty level - challenging setting")},
    {strings::i18n::expert,
     translation_manager::TranslatableString(
         "expert", "AI difficulty level - hardest setting")},

    // Player Statistics
    {strings::i18n::lives_label,
     translation_manager::TranslatableString("lives: {}",
                                             "Label for player lives display")},
    {strings::i18n::kills_label,
     translation_manager::TranslatableString(
         "kills: {}", "Label for player kill count display")},
    {strings::i18n::hippos_label,
     translation_manager::TranslatableString(
         "hippos: {}", "Label for hippo collection count display")},
    {strings::i18n::hippos_zero,
     translation_manager::TranslatableString(
         "hippos: 0", "Fallback text when no hippos collected")},
    {strings::i18n::not_it_timer,
     translation_manager::TranslatableString(
         "not it: {:.1f}s", "Label for tag game timer display")},

    // Round Settings Labels
    {strings::i18n::win_condition_label,
     translation_manager::TranslatableString(
         "win condition: {}", "Label for win condition setting")},
    {strings::i18n::num_lives_label,
     translation_manager::TranslatableString(
         "num lives: {}", "Label for starting lives setting")},
    {strings::i18n::round_length_with_time,
     translation_manager::TranslatableString(
         "round length: {}", "Label for round time duration setting")},
    {strings::i18n::total_hippos_label,
     translation_manager::TranslatableString("total hippos: {}",
                                             "Label for hippo count setting")}};

static std::map<strings::i18n, TranslatableString> korean_translations = {
    {strings::i18n::play, translation_manager::TranslatableString(
                              "시작", "새 게임을 시작하는 메인 메뉴 버튼")},
    {strings::i18n::about, translation_manager::TranslatableString(
                               "정보", "게임 정보를 보여주는 메인 메뉴 버튼")},
    {strings::i18n::exit, translation_manager::TranslatableString(
                              "종료", "게임을 종료하는 메인 메뉴 버튼")},
    {strings::i18n::loading,
     translation_manager::TranslatableString(
         "로딩중...", "게임이 로딩 중일 때 표시되는 텍스트")},
    {strings::i18n::gameover,
     translation_manager::TranslatableString(
         "게임 오버", "플레이어가 패배했을 때 표시되는 텍스트")},
    {strings::i18n::victory,
     translation_manager::TranslatableString(
         "승리!", "플레이어가 승리했을 때 표시되는 텍스트")},
    {strings::i18n::start, translation_manager::TranslatableString(
                               "시작", "게임플레이를 시작하는 버튼")},
    {strings::i18n::back,
     translation_manager::TranslatableString(
         "뒤로", "이전 화면으로 돌아가는 네비게이션 버튼")},
    {strings::i18n::continue_game,
     translation_manager::TranslatableString("계속",
                                             "라운드가 끝난 후 계속하는 버튼")},
    {strings::i18n::quit, translation_manager::TranslatableString(
                              "종료", "현재 게임 세션을 종료하는 버튼")},
    {strings::i18n::settings,
     translation_manager::TranslatableString(
         "설정", "게임 설정에 접근하는 메인 메뉴 버튼")},
    {strings::i18n::volume, translation_manager::TranslatableString(
                                "볼륨", "일반적인 볼륨 설정 라벨")},
    {strings::i18n::fullscreen,
     translation_manager::TranslatableString(
         "전체화면", "전체화면 모드를 토글하는 체크박스")},
    {strings::i18n::resolution,
     translation_manager::TranslatableString(
         "해상도", "화면 해상도를 선택하는 드롭다운")},
    {strings::i18n::language,
     translation_manager::TranslatableString("언어 (language)",
                                             "게임 언어를 선택하는 드롭다운")},

    // Additional UI strings
    {strings::i18n::round_settings,
     translation_manager::TranslatableString("라운드 설정",
                                             "라운드 구성 화면의 제목")},
    {strings::i18n::resume, translation_manager::TranslatableString(
                                "계속", "게임을 일시정지 해제하는 버튼")},
    {strings::i18n::back_to_setup,
     translation_manager::TranslatableString(
         "설정으로 돌아가기", "일시정지 메뉴에서 게임 설정으로 돌아가는 버튼")},
    {strings::i18n::exit_game,
     translation_manager::TranslatableString(
         "게임 종료", "일시정지 메뉴에서 현재 게임을 종료하는 버튼")},
    {strings::i18n::round_length,
     translation_manager::TranslatableString("라운드 길이",
                                             "라운드 시간 지속 설정의 라벨")},
    {strings::i18n::allow_tag_backs,
     translation_manager::TranslatableString(
         "태그 백 허용", "태그 앤 고 게임 모드 설정을 위한 체크박스")},
    {strings::i18n::select_map, translation_manager::TranslatableString(
                                    "맵 선택", "게임용 맵을 선택하는 버튼")},
    {strings::i18n::master_volume,
     translation_manager::TranslatableString("마스터 볼륨 (master volume)",
                                             "전체 게임 볼륨을 위한 슬라이더")},
    {strings::i18n::music_volume,
     translation_manager::TranslatableString("음악 볼륨 (music volume)",
                                             "배경 음악 볼륨을 위한 슬라이더")},
    {strings::i18n::sfx_volume,
     translation_manager::TranslatableString("효과음 볼륨 (sfx volume)",
                                             "효과음 볼륨을 위한 슬라이더")},
    {strings::i18n::post_processing,
     translation_manager::TranslatableString(
         "후처리", "시각적 후처리 효과를 활성화하는 체크박스")},
    {strings::i18n::round_end,
     translation_manager::TranslatableString("라운드 종료 (round end)",
                                             "라운드가 끝날 때 표시되는 제목")},
    {strings::i18n::paused,
     translation_manager::TranslatableString(
         "일시정지", "게임이 일시정지되었을 때 표시되는 큰 텍스트")},
    {strings::i18n::unknown,
     translation_manager::TranslatableString(
         "알 수 없음", "알 수 없는 게임 상태를 위한 대체 텍스트")},
    {strings::i18n::unlimited, translation_manager::TranslatableString(
                                   "무제한", "무제한 라운드 시간을 위한 옵션")},
    {strings::i18n::easy, translation_manager::TranslatableString(
                              "쉬움", "AI 난이도 - 가장 쉬운 설정")},
    {strings::i18n::medium,
     translation_manager::TranslatableString("보통", "AI 난이도 - 보통 설정")},
    {strings::i18n::hard, translation_manager::TranslatableString(
                              "어려움", "AI 난이도 - 도전적인 설정")},
    {strings::i18n::expert, translation_manager::TranslatableString(
                                "전문가", "AI 난이도 - 가장 어려운 설정")},

    // Player Statistics
    {strings::i18n::lives_label,
     translation_manager::TranslatableString("생명 (lives): {}",
                                             "플레이어 생명 표시 라벨")},
    {strings::i18n::kills_label, translation_manager::TranslatableString(
                                     "킬: {}", "플레이어 킬 카운트 표시 라벨")},
    {strings::i18n::hippos_label,
     translation_manager::TranslatableString("하마: {}",
                                             "하마 수집 카운트 표시 라벨")},
    {strings::i18n::hippos_zero,
     translation_manager::TranslatableString(
         "하마: 0", "하마를 수집하지 않았을 때의 대체 텍스트")},
    {strings::i18n::not_it_timer,
     translation_manager::TranslatableString("술래: {:.1f}초",
                                             "술래잡기 게임 타이머 표시 라벨")},

    // Round Settings Labels
    {strings::i18n::win_condition_label,
     translation_manager::TranslatableString("승리 조건: {}",
                                             "승리 조건 설정 라벨")},
    {strings::i18n::num_lives_label,
     translation_manager::TranslatableString("시작 생명: {}",
                                             "시작 생명 설정 라벨")},
    {strings::i18n::round_length_with_time,
     translation_manager::TranslatableString("라운드 길이: {}",
                                             "라운드 시간 지속 설정 라벨")},
    {strings::i18n::total_hippos_label,
     translation_manager::TranslatableString("총 하마: {}",
                                             "하마 개수 설정 라벨")}};

static std::map<strings::i18n, TranslatableString> japanese_translations = {
    {strings::i18n::play,
     translation_manager::TranslatableString(
         "プレイ", "新しいゲームを開始するメインメニューボタン")},
    {strings::i18n::about,
     translation_manager::TranslatableString(
         "情報", "ゲーム情報を表示するメインメニューボタン")},
    {strings::i18n::exit, translation_manager::TranslatableString(
                              "終了", "ゲームを終了するメインメニューボタン")},
    {strings::i18n::loading,
     translation_manager::TranslatableString(
         "読み込み中...", "ゲームが読み込み中に表示されるテキスト")},
    {strings::i18n::gameover,
     translation_manager::TranslatableString(
         "ゲームオーバー", "プレイヤーが敗北した時に表示されるテキスト")},
    {strings::i18n::victory,
     translation_manager::TranslatableString(
         "勝利！", "プレイヤーが勝利した時に表示されるテキスト")},
    {strings::i18n::start, translation_manager::TranslatableString(
                               "開始", "ゲームプレイを開始するボタン")},
    {strings::i18n::back, translation_manager::TranslatableString(
                              "戻る", "前の画面に戻るナビゲーションボタン")},
    {strings::i18n::continue_game,
     translation_manager::TranslatableString("続行",
                                             "ラウンド終了後に続行するボタン")},
    {strings::i18n::quit,
     translation_manager::TranslatableString(
         "終了", "現在のゲームセッションを終了するボタン")},
    {strings::i18n::settings,
     translation_manager::TranslatableString(
         "設定", "ゲーム設定にアクセスするメインメニューボタン")},
    {strings::i18n::volume,
     translation_manager::TranslatableString("音量", "一般的な音量設定ラベル")},
    {strings::i18n::fullscreen,
     translation_manager::TranslatableString(
         "フルスクリーン", "フルスクリーンモードを切り替えるチェックボックス")},
    {strings::i18n::resolution,
     translation_manager::TranslatableString(
         "解像度", "画面解像度を選択するドロップダウン")},
    {strings::i18n::language,
     translation_manager::TranslatableString(
         "言語 (Language)", "ゲーム言語を選択するドロップダウン")},

    // Additional UI strings
    {strings::i18n::round_settings,
     translation_manager::TranslatableString("ラウンド設定",
                                             "ラウンド構成画面のタイトル")},
    {strings::i18n::resume, translation_manager::TranslatableString(
                                "続行", "ゲームの一時停止を解除するボタン")},
    {strings::i18n::back_to_setup,
     translation_manager::TranslatableString(
         "設定に戻る", "一時停止メニューからゲーム設定に戻るボタン")},
    {strings::i18n::exit_game,
     translation_manager::TranslatableString(
         "ゲーム終了", "一時停止メニューから現在のゲームを終了するボタン")},
    {strings::i18n::round_length,
     translation_manager::TranslatableString("ラウンド時間",
                                             "ラウンド時間持続設定のラベル")},
    {strings::i18n::allow_tag_backs,
     translation_manager::TranslatableString(
         "タグバック許可",
         "タグアンドゴーゲームモード設定のためのチェックボックス")},
    {strings::i18n::select_map,
     translation_manager::TranslatableString("マップ選択",
                                             "ゲーム用マップを選択するボタン")},
    {strings::i18n::master_volume,
     translation_manager::TranslatableString(
         "マスターボリューム", "全体ゲーム音量のためのスライダー")},
    {strings::i18n::music_volume,
     translation_manager::TranslatableString("音楽ボリューム",
                                             "背景音楽音量のためのスライダー")},
    {strings::i18n::sfx_volume,
     translation_manager::TranslatableString("効果音ボリューム",
                                             "効果音音量のためのスライダー")},
    {strings::i18n::post_processing,
     translation_manager::TranslatableString(
         "後処理", "視覚的後処理効果を有効にするチェックボックス")},
    {strings::i18n::round_end,
     translation_manager::TranslatableString(
         "ラウンド終了", "ラウンドが終了した時に表示されるタイトル")},
    {strings::i18n::paused,
     translation_manager::TranslatableString(
         "一時停止", "ゲームが一時停止された時に表示される大きなテキスト")},
    {strings::i18n::unknown,
     translation_manager::TranslatableString(
         "不明", "不明なゲーム状態のための代替テキスト")},
    {strings::i18n::unlimited,
     translation_manager::TranslatableString(
         "無制限", "無制限ラウンド時間のためのオプション")},
    {strings::i18n::easy, translation_manager::TranslatableString(
                              "簡単", "AI難易度 - 最も簡単な設定")},
    {strings::i18n::medium,
     translation_manager::TranslatableString("普通", "AI難易度 - 普通の設定")},
    {strings::i18n::hard, translation_manager::TranslatableString(
                              "難しい", "AI難易度 - 挑戦的な設定")},
    {strings::i18n::expert, translation_manager::TranslatableString(
                                "エキスパート", "AI難易度 - 最も難しい設定")},

    // Player Statistics
    {strings::i18n::lives_label,
     translation_manager::TranslatableString("ライフ: {}",
                                             "プレイヤーライフ表示ラベル")},
    {strings::i18n::kills_label,
     translation_manager::TranslatableString(
         "キル: {}", "プレイヤーキルカウント表示ラベル")},
    {strings::i18n::hippos_label,
     translation_manager::TranslatableString("カバ: {}",
                                             "カバ収集カウント表示ラベル")},
    {strings::i18n::hippos_zero,
     translation_manager::TranslatableString(
         "カバ: 0", "カバを収集していない時の代替テキスト")},
    {strings::i18n::not_it_timer,
     translation_manager::TranslatableString(
         "鬼: {:.1f}초", "鬼ごっこゲームタイマー表示ラベル")},

    // Round Settings Labels
    {strings::i18n::win_condition_label,
     translation_manager::TranslatableString("勝利条件: {}",
                                             "勝利条件設定ラベル")},
    {strings::i18n::num_lives_label,
     translation_manager::TranslatableString("開始ライフ: {}",
                                             "開始ライフ設定ラベル")},
    {strings::i18n::round_length_with_time,
     translation_manager::TranslatableString("ラウンド時間: {}",
                                             "ラウンド時間持続設定ラベル")},
    {strings::i18n::total_hippos_label,
     translation_manager::TranslatableString("総カバ: {}",
                                             "カバ個数設定ラベル")}};

// Get translations for a specific language
const std::map<strings::i18n, TranslatableString> &
TranslationManager::get_translations_for_language(Language language) const {
  switch (language) {
  case Language::English:
    return english_translations;
  case Language::Korean:
    return korean_translations;
  case Language::Japanese:
    return japanese_translations;
  default:
    return english_translations;
  }
}

TranslationManager::TranslationManager() {
  set_language(Language::English); // Default to English
}

std::map<strings::i18n, TranslatableString>::const_iterator
TranslationManager::find_translation(strings::i18n key) const {
  const auto &translations = get_translations_for_language(current_language);
  auto it = translations.find(key);
  if (it == translations.end()) {
    log_warn("Translation not found for key: {}", magic_enum::enum_name(key));
  }
  return it;
}

std::string TranslationManager::get_string(strings::i18n key) const {
  auto it = find_translation(key);
  if (it != get_translations_for_language(current_language).end()) {
    return it->second.get_text();
  }
  return "MISSING_TRANSLATION";
}

TranslatableString
TranslationManager::get_translatable_string(strings::i18n key) const {
  auto it = find_translation(key);
  if (it != get_translations_for_language(current_language).end()) {
    return translation_manager::TranslatableString(
        it->second.get_text(), it->second.get_description());
  }
  return translation_manager::TranslatableString("MISSING_TRANSLATION", true);
}

// TranslatableString constructor implementation
translation_manager::TranslatableString::TranslatableString(
    const strings::i18n &key) {
  auto &manager = TranslationManager::get();
  auto it = manager.find_translation(key);
  if (it !=
      manager.get_translations_for_language(manager.get_language()).end()) {
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

  // Collect all unique codepoints from all CJK languages
  std::set<int> all_codepoints;

  // Add Latin alphabet (uppercase and lowercase) and numbers
  for (char c = 'A'; c <= 'Z'; ++c) {
    all_codepoints.insert(static_cast<int>(c));
  }
  for (char c = 'a'; c <= 'z'; ++c) {
    all_codepoints.insert(static_cast<int>(c));
  }
  for (char c = '0'; c <= '9'; ++c) {
    all_codepoints.insert(static_cast<int>(c));
  }

  // Add common punctuation characters
  const char punctuation[] = ".,!?;:()[]{}\"'`~@#$%^&*+-=_|\\/<>";
  for (char c : punctuation) {
    all_codepoints.insert(static_cast<int>(c));
  }

  for (const auto &lang : {Language::Korean, Language::Japanese}) {
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

  // Load all codepoints into both Korean and Japanese fonts
  if (!all_codepoints.empty()) {
    // Convert set to vector
    std::vector<int> codepoints(all_codepoints.begin(), all_codepoints.end());

    // Load into Korean font
    FontID korean_font_id = FontID::Korean;
    std::string korean_font_name = get_font_name(korean_font_id);
    std::string korean_font_file =
        Files::get().fetch_resource_path("", korean_font_name);

    font_manager.load_font_with_codepoints(
        korean_font_name, korean_font_file.c_str(), codepoints.data(),
        static_cast<int>(codepoints.size()));

    // Load into Japanese font
    FontID japanese_font_id = FontID::Japanese;
    std::string japanese_font_name = get_font_name(japanese_font_id);
    std::string japanese_font_file =
        Files::get().fetch_resource_path("", japanese_font_name);

    font_manager.load_font_with_codepoints(
        japanese_font_name, japanese_font_file.c_str(), codepoints.data(),
        static_cast<int>(codepoints.size()));

    log_info(
        "Loaded {} and {} fonts with {} total codepoints for all CJK languages",
        korean_font_name, japanese_font_name, codepoints.size());
  }
}

} // namespace translation_manager
