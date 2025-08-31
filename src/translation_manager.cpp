#include "translation_manager.h"
#include "log.h"
#include <map>
#include <set>

namespace translation_manager {

// Runtime translation maps
static std::map<strings::i18n, TranslationEntry> english_translations = {
    {strings::i18n::play,
     TranslationEntry("play", "Main menu button to start a new game")},
    {strings::i18n::about,
     TranslationEntry("about", "Main menu button to show game information")},
    {strings::i18n::exit,
     TranslationEntry("exit", "Main menu button to quit the game")},
    {strings::i18n::loading,
     TranslationEntry("Loading...", "Text shown while game is loading")},
    {strings::i18n::gameover,
     TranslationEntry("game over", "Text shown when player loses")},
    {strings::i18n::victory,
     TranslationEntry("victory!", "Text shown when player wins")},
    {strings::i18n::start,
     TranslationEntry("start", "Button to begin gameplay")},
    {strings::i18n::back,
     TranslationEntry("back",
                      "Navigation button to return to previous screen")},
    {strings::i18n::continue_game,
     TranslationEntry("continue", "Button to continue after round ends")},
    {strings::i18n::quit,
     TranslationEntry("quit", "Button to exit current game session")},
    {strings::i18n::settings,
     TranslationEntry("settings", "Main menu button to access game settings")},
    {strings::i18n::volume,
     TranslationEntry("volume", "Generic volume setting label")},
    {strings::i18n::fullscreen,
     TranslationEntry("fullscreen", "Checkbox to toggle fullscreen mode")},
    {strings::i18n::resolution,
     TranslationEntry("resolution", "Dropdown to select screen resolution")},
    {strings::i18n::language,
     TranslationEntry("language", "Dropdown to select game language")},

    // Additional UI strings
    {strings::i18n::round_settings,
     TranslationEntry("Round Settings",
                      "Title for round configuration screen")},
    {strings::i18n::resume,
     TranslationEntry("resume", "Button to unpause the game")},
    {strings::i18n::back_to_setup,
     TranslationEntry("back to setup",
                      "Button to return to game setup from pause menu")},
    {strings::i18n::exit_game,
     TranslationEntry("exit game",
                      "Button to quit current game from pause menu")},
    {strings::i18n::round_length,
     TranslationEntry("Round Length", "Label for round time duration setting")},
    {strings::i18n::allow_tag_backs,
     TranslationEntry("Allow Tag Backs",
                      "Checkbox for tag-and-go game mode setting")},
    {strings::i18n::select_map,
     TranslationEntry("select map", "Button to choose a map for the game")},
    {strings::i18n::master_volume,
     TranslationEntry("Master Volume", "Slider for overall game volume")},
    {strings::i18n::music_volume,
     TranslationEntry("Music Volume", "Slider for background music volume")},
    {strings::i18n::sfx_volume,
     TranslationEntry("SFX Volume", "Slider for sound effects volume")},
    {strings::i18n::post_processing,
     TranslationEntry("Post Processing",
                      "Checkbox to enable visual post-processing effects")},
    {strings::i18n::round_end,
     TranslationEntry("Round End", "Title shown when a round finishes")},
    {strings::i18n::paused,
     TranslationEntry("paused", "Large text shown when game is paused")},
    {strings::i18n::unknown,
     TranslationEntry("Unknown", "Fallback text for unknown game states")},
    {strings::i18n::unlimited,
     TranslationEntry("Unlimited", "Option for unlimited round time")},
    {strings::i18n::easy,
     TranslationEntry("Easy", "AI difficulty level - easiest setting")},
    {strings::i18n::medium,
     TranslationEntry("Medium", "AI difficulty level - moderate setting")},
    {strings::i18n::hard,
     TranslationEntry("Hard", "AI difficulty level - challenging setting")},
    {strings::i18n::expert,
     TranslationEntry("Expert", "AI difficulty level - hardest setting")}};

static std::map<strings::i18n, TranslationEntry> korean_translations = {
    {strings::i18n::play,
     TranslationEntry("시작", "새 게임을 시작하는 메인 메뉴 버튼")},
    {strings::i18n::about,
     TranslationEntry("정보", "게임 정보를 보여주는 메인 메뉴 버튼")},
    {strings::i18n::exit,
     TranslationEntry("종료", "게임을 종료하는 메인 메뉴 버튼")},
    {strings::i18n::loading,
     TranslationEntry("로딩중...", "게임이 로딩 중일 때 표시되는 텍스트")},
    {strings::i18n::gameover,
     TranslationEntry("게임 오버", "플레이어가 패배했을 때 표시되는 텍스트")},
    {strings::i18n::victory,
     TranslationEntry("승리!", "플레이어가 승리했을 때 표시되는 텍스트")},
    {strings::i18n::start,
     TranslationEntry("시작", "게임플레이를 시작하는 버튼")},
    {strings::i18n::back,
     TranslationEntry("뒤로", "이전 화면으로 돌아가는 네비게이션 버튼")},
    {strings::i18n::continue_game,
     TranslationEntry("계속", "라운드가 끝난 후 계속하는 버튼")},
    {strings::i18n::quit,
     TranslationEntry("종료", "현재 게임 세션을 종료하는 버튼")},
    {strings::i18n::settings,
     TranslationEntry("설정", "게임 설정에 접근하는 메인 메뉴 버튼")},
    {strings::i18n::volume,
     TranslationEntry("볼륨", "일반적인 볼륨 설정 라벨")},
    {strings::i18n::fullscreen,
     TranslationEntry("전체화면", "전체화면 모드를 토글하는 체크박스")},
    {strings::i18n::resolution,
     TranslationEntry("해상도", "화면 해상도를 선택하는 드롭다운")},
    {strings::i18n::language,
     TranslationEntry("언어 (Language)", "게임 언어를 선택하는 드롭다운")},

    // Additional UI strings
    {strings::i18n::round_settings,
     TranslationEntry("라운드 설정", "라운드 구성 화면의 제목")},
    {strings::i18n::resume,
     TranslationEntry("계속", "게임을 일시정지 해제하는 버튼")},
    {strings::i18n::back_to_setup,
     TranslationEntry("설정으로 돌아가기",
                      "일시정지 메뉴에서 게임 설정으로 돌아가는 버튼")},
    {strings::i18n::exit_game,
     TranslationEntry("게임 종료",
                      "일시정지 메뉴에서 현재 게임을 종료하는 버튼")},
    {strings::i18n::round_length,
     TranslationEntry("라운드 길이", "라운드 시간 지속 설정의 라벨")},
    {strings::i18n::allow_tag_backs,
     TranslationEntry("태그 백 허용",
                      "태그 앤 고 게임 모드 설정을 위한 체크박스")},
    {strings::i18n::select_map,
     TranslationEntry("맵 선택", "게임용 맵을 선택하는 버튼")},
    {strings::i18n::master_volume,
     TranslationEntry("마스터 볼륨", "전체 게임 볼륨을 위한 슬라이더")},
    {strings::i18n::music_volume,
     TranslationEntry("음악 볼륨", "배경 음악 볼륨을 위한 슬라이더")},
    {strings::i18n::sfx_volume,
     TranslationEntry("효과음 볼륨", "효과음 볼륨을 위한 슬라이더")},
    {strings::i18n::post_processing,
     TranslationEntry("후처리", "시각적 후처리 효과를 활성화하는 체크박스")},
    {strings::i18n::round_end,
     TranslationEntry("라운드 종료", "라운드가 끝날 때 표시되는 제목")},
    {strings::i18n::paused,
     TranslationEntry("일시정지",
                      "게임이 일시정지되었을 때 표시되는 큰 텍스트")},
    {strings::i18n::unknown,
     TranslationEntry("알 수 없음", "알 수 없는 게임 상태를 위한 대체 텍스트")},
    {strings::i18n::unlimited,
     TranslationEntry("무제한", "무제한 라운드 시간을 위한 옵션")},
    {strings::i18n::easy,
     TranslationEntry("쉬움", "AI 난이도 - 가장 쉬운 설정")},
    {strings::i18n::medium, TranslationEntry("보통", "AI 난이도 - 보통 설정")},
    {strings::i18n::hard,
     TranslationEntry("어려움", "AI 난이도 - 도전적인 설정")},
    {strings::i18n::expert,
     TranslationEntry("전문가", "AI 난이도 - 가장 어려운 설정")}};

static std::map<strings::i18n, TranslationEntry> chinese_translations = {
    {strings::i18n::play, TranslationEntry("开始", "开始新游戏的主菜单按钮")},
    {strings::i18n::about,
     TranslationEntry("关于", "显示游戏信息的主菜单按钮")},
    {strings::i18n::exit, TranslationEntry("退出", "退出游戏的主菜单按钮")},
    {strings::i18n::loading,
     TranslationEntry("加载中...", "游戏加载时显示的文本")},
    {strings::i18n::gameover,
     TranslationEntry("游戏结束", "玩家失败时显示的文本")},
    {strings::i18n::victory, TranslationEntry("胜利!", "玩家胜利时显示的文本")},
    {strings::i18n::start, TranslationEntry("开始", "开始游戏游玩的按钮")},
    {strings::i18n::back, TranslationEntry("返回", "导航按钮返回上一屏幕")},
    {strings::i18n::continue_game,
     TranslationEntry("继续", "回合结束后继续的按钮")},
    {strings::i18n::quit, TranslationEntry("退出", "退出当前游戏会话的按钮")},
    {strings::i18n::settings,
     TranslationEntry("设置", "访问游戏设置的主菜单按钮")},
    {strings::i18n::volume, TranslationEntry("音量", "通用音量设置标签")},
    {strings::i18n::fullscreen,
     TranslationEntry("全屏", "切换全屏模式的复选框")},
    {strings::i18n::resolution,
     TranslationEntry("分辨率", "下拉菜单选择屏幕分辨率")},
    {strings::i18n::language,
     TranslationEntry("语言 (Language)", "下拉菜单选择游戏语言")},

    // Additional UI strings
    {strings::i18n::round_settings,
     TranslationEntry("回合设置", "回合配置屏幕的标题")},
    {strings::i18n::resume, TranslationEntry("继续", "解除游戏暂停的按钮")},
    {strings::i18n::back_to_setup,
     TranslationEntry("返回设置", "从暂停菜单返回游戏设置的按钮")},
    {strings::i18n::exit_game,
     TranslationEntry("退出游戏", "从暂停菜单退出当前游戏的按钮")},
    {strings::i18n::round_length,
     TranslationEntry("回合时长", "回合时间持续设置的标签")},
    {strings::i18n::allow_tag_backs,
     TranslationEntry("允许回退", "允许回退的复选框")},
    {strings::i18n::select_map,
     TranslationEntry("选择地图", "选择游戏地图的按钮")},
    {strings::i18n::master_volume,
     TranslationEntry("主音量", "用于整体游戏音量的滑块")},
    {strings::i18n::music_volume,
     TranslationEntry("音乐音量", "用于背景音乐音量的滑块")},
    {strings::i18n::sfx_volume,
     TranslationEntry("音效音量", "用于音效音量的滑块")},
    {strings::i18n::post_processing,
     TranslationEntry("后期处理", "启用视觉后期处理效果的复选框")},
    {strings::i18n::round_end,
     TranslationEntry("回合结束", "回合结束时显示的标题")},
    {strings::i18n::paused, TranslationEntry("暂停", "游戏暂停时显示的大文本")},
    {strings::i18n::unknown,
     TranslationEntry("未知", "未知游戏状态的备用文本")},
    {strings::i18n::unlimited, TranslationEntry("无限", "无限回合时间的选项")},
    {strings::i18n::easy, TranslationEntry("简单", "AI难度 - 最简单设置")},
    {strings::i18n::medium, TranslationEntry("中等", "AI难度 - 中等设置")},
    {strings::i18n::hard,
     TranslationEntry("困难", "AI难度 - 具有挑战性的设置")},
    {strings::i18n::expert, TranslationEntry("专家", "AI难度 - 最难设置")}};

static std::map<strings::i18n, TranslationEntry> japanese_translations = {
    {strings::i18n::play,
     TranslationEntry("プレイ", "新しいゲームを開始するメインメニューボタン")},
    {strings::i18n::about,
     TranslationEntry("について", "ゲーム情報を表示するメインメニューボタン")},
    {strings::i18n::exit,
     TranslationEntry("終了", "ゲームを終了するメインメニューボタン")},
    {strings::i18n::loading,
     TranslationEntry("読み込み中...",
                      "ゲームが読み込み中のときに表示されるテキスト")},
    {strings::i18n::gameover,
     TranslationEntry("ゲームオーバー",
                      "プレイヤーが敗北したときに表示されるテキスト")},
    {strings::i18n::victory,
     TranslationEntry("勝利!", "プレイヤーが勝利したときに表示されるテキスト")},
    {strings::i18n::start,
     TranslationEntry("開始", "ゲームプレイを開始するボタン")},
    {strings::i18n::back,
     TranslationEntry("戻る", "前の画面に戻るナビゲーションボタン")},
    {strings::i18n::continue_game,
     TranslationEntry("続行", "ラウンドが終了した後続行するボタン")},
    {strings::i18n::quit,
     TranslationEntry("終了", "現在のゲームセッションを終了するボタン")},
    {strings::i18n::settings,
     TranslationEntry("設定", "ゲーム設定にアクセスするメインメニューボタン")},
    {strings::i18n::volume, TranslationEntry("音量", "一般的な音量設定ラベル")},
    {strings::i18n::fullscreen,
     TranslationEntry("フルスクリーン",
                      "フルスクリーンモードを切り替えるチェックボックス")},
    {strings::i18n::resolution,
     TranslationEntry("解像度", "画面解像度を選択するドロップダウン")},
    {strings::i18n::language,
     TranslationEntry("言語 (Language)", "ゲーム言語を選択するドロップダウン")},

    // Additional UI strings
    {strings::i18n::round_settings,
     TranslationEntry("ラウンド設定", "ラウンド設定画面のタイトル")},
    {strings::i18n::resume,
     TranslationEntry("続行", "ゲームを一時停止を解除するボタン")},
    {strings::i18n::back_to_setup,
     TranslationEntry("設定に戻る",
                      "一時停止メニューからゲーム設定に戻るボタン")},
    {strings::i18n::exit_game,
     TranslationEntry("ゲーム終了",
                      "一時停止メニューから現在のゲームを終了するボタン")},
    {strings::i18n::round_length,
     TranslationEntry("ラウンド時間", "ラウンド時間持続設定のラベル")},
    {strings::i18n::allow_tag_backs,
     TranslationEntry(
         "タグバックを許可",
         "タグアンドゴーゲームモード設定のためのチェックボックス")},
    {strings::i18n::select_map,
     TranslationEntry("マップを選択", "ゲーム用マップを選択するボタン")},
    {strings::i18n::master_volume,
     TranslationEntry("マスターボリューム",
                      "全体ゲーム音量のためのスライダー")},
    {strings::i18n::music_volume,
     TranslationEntry("音楽ボリューム", "背景音楽音量のためのスライダー")},
    {strings::i18n::sfx_volume,
     TranslationEntry("効果音ボリューム", "効果音音量のためのスライダー")},
    {strings::i18n::post_processing,
     TranslationEntry(
         "ポストプロセッシング",
         "視覚的ポストプロセッシング効果を有効にするチェックボックス")},
    {strings::i18n::round_end,
     TranslationEntry("ラウンド終了",
                      "ラウンドが終了したときに表示されるタイトル")},
    {strings::i18n::paused,
     TranslationEntry("一時停止",
                      "ゲームが一時停止したときに表示される大きなテキスト")},
    {strings::i18n::unknown,
     TranslationEntry("不明", "不明なゲーム状態の代替テキスト")},
    {strings::i18n::unlimited,
     TranslationEntry("無制限", "無制限ラウンド時間のオプション")},
    {strings::i18n::easy,
     TranslationEntry("簡単", "AI難易度 - 最も簡単な設定")},
    {strings::i18n::medium, TranslationEntry("普通", "AI難易度 - 普通の設定")},
    {strings::i18n::hard,
     TranslationEntry("難しい", "AI難易度 - 挑戦的な設定")},
    {strings::i18n::expert,
     TranslationEntry("専門家", "AI難易度 - 最も難しい設定")}};

// Get translations for a specific language
static const std::map<strings::i18n, TranslationEntry> &
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
    return it->second.text;
  }
  return "MISSING_TRANSLATION"; // Fallback if translation not found
}

TranslationEntry
TranslationManager::get_translation_entry(strings::i18n key) const {
  // Get translations for current language
  const auto &translations = get_translations_for_language(current_language);

  auto it = translations.find(key);
  if (it != translations.end()) {
    return it->second;
  }
  return TranslationEntry("MISSING_TRANSLATION",
                          "Translation not found"); // Fallback
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
      all_chars += it->second.text;
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
