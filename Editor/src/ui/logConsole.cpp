#include "ui/logConsole.h"

#include <string>

#include "SirEngine/memory/cpu/overridingRingBuffer.h"
#include "imgui/imgui.h"

namespace Editor {

struct LogItem {
  const char *message = nullptr;
  ImColor color{255, 255, 255, 255};
};

void destroyLogItem(LogItem &item) {
  free((void *)(item.message));
}

struct LogConsole {
  char InputBuf[256];
  SirEngine::OverridingRingBuffer<LogItem> Items;
  bool ScrollToBottom;

  LogConsole() : Items(100, nullptr) {
    clearLog();
    memset(InputBuf, 0, sizeof(InputBuf));
    Items.registerDestroyCallback(destroyLogItem);
  }
  ~LogConsole() { clearLog(); }

  // Portable helpers
  static int Stricmp(const char *str1, const char *str2) {
    int d;
    while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) {
      str1++;
      str2++;
    }
    return d;
  }
  static int Strnicmp(const char *str1, const char *str2, int n) {
    int d = 0;
    while (n > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) {
      str1++;
      str2++;
      n--;
    }
    return d;
  }
  static char *strdup(const char *str) {
    size_t len = strlen(str) + 1;
    void *buff = malloc(len);
    return static_cast<char *>(
        memcpy(buff, static_cast<const void *>(str), len));
  }
  static void strtrim(char *str) {
    char *strEnd = str + strlen(str);
    while (strEnd > str && strEnd[-1] == ' ') strEnd--;
    *strEnd = 0;
  }

  void clearLog() {
    Items.clear();
    ScrollToBottom = true;
  }

  void addLog(const EditorLogLevel level, const char *fmt, ...) IM_FMTARGS(2) {
    // FIXME-OPT
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    ImColor color;
    switch (level) {
      case EditorLogLevel::LOG_NONE:
        color = ImColor{255, 255, 255};
        break;
      case EditorLogLevel::LOG_WARNING:
        color = ImColor{255, 255, 0};
        break;
      case EditorLogLevel::LOG_ERROR:
        color = ImColor{255, 0, 0};
        break;
    }
    Items.push(LogItem{strdup(buf), color});
    ScrollToBottom = true;
  }

  void Draw(bool *p_open) {
    // ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    // if (!ImGui::Begin(title, p_open)) {
    //  ImGui::End();
    //  return;
    //}

    // As a specific feature guaranteed by the library, after calling Begin()
    // the last Item represent the title bar. So e.g. IsItemHovered() will
    // return true when hovering the title bar. Here we create a context menu
    // only available from the title bar.
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Close Console")) *p_open = false;
      ImGui::EndPopup();
    }
    // TODO: display items starting from the bottom

    ImGui::SameLine();
    if (ImGui::SmallButton("Clear")) {
      clearLog();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Scroll to bottom")) ScrollToBottom = true;
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PopStyleVar();
    ImGui::Separator();

    const float footer_height_to_reserve =
        ImGui::GetStyle().ItemSpacing.y +
        ImGui::GetFrameHeightWithSpacing();  // 1 separator, 1 input text
    ImGui::BeginChild(
        "ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
        ImGuiWindowFlags_HorizontalScrollbar);  // Leave room for 1 separator +
                                                // 1 InputText
    if (ImGui::BeginPopupContextWindow()) {
      if (ImGui::Selectable("Clear")) clearLog();
      ImGui::EndPopup();
    }

    // Display every line as a separate entry so we can change their color or
    // add custom widgets. If you only want raw text you can use
    // ImGui::TextUnformatted(log.begin(), log.end()); NB- if you have thousands
    // of entries this approach may be too inefficient and may require user-side
    // clipping to only process visible items. You can seek and display only the
    // lines that are visible using the ImGuiListClipper helper, if your
    // elements are evenly spaced and you have cheap random access to the
    // elements. To use the clipper we could replace the 'for (int i = 0; i <
    // Items.Size; i++)' loop with:
    //     ImGuiListClipper clipper(Items.Size);
    //     while (clipper.Step())
    //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
    // However, note that you can not use this code as is if a filter is active
    // because it breaks the 'cheap random-access' property. We would need
    // random-access on the post-filtered list. A typical application wanting
    // coarse clipping and filtering may want to pre-compute an array of indices
    // that passed the filtering test, recomputing this array when user changes
    // the filter, and appending newly elements as they are inserted. This is
    // left as a task to the user until we can manage to improve this example
    // code! If your items are of variable size you may want to implement code
    // similar to what ImGuiListClipper does. Or split your data into fixed
    // height items to allow random-seeking into your list.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(4, 1));  // Tighten spacing

    ImVec4 col_default_text = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    uint32_t start = Items.getFirstElementIndex();
    uint32_t size = Items.getSize();
    uint32_t count = Items.usedElementCount();
    const LogItem *itemData = Items.getData();
    for (int i = 0; i < count; i++) {
      uint32_t idx = (start + i) % size;
      const LogItem &item = itemData[idx];
      ImVec4 col = item.color;
      ImGui::PushStyleColor(ImGuiCol_Text, col);
      ImGui::TextUnformatted(item.message);
      ImGui::PopStyleColor();
    }

    if (ScrollToBottom) ImGui::SetScrollHereY(1.0f);
    ScrollToBottom = false;
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();
  }
};

void LogConsoleWidget::initialize() { m_console = new LogConsole(); }
void LogConsoleWidget::render() {
  ImGui::Begin("Log", (bool *)0);
  m_console->Draw(&m_shouldRenderConsole);

  ImGui::Separator();
  ImGui::End();
}
void LogConsoleWidget::log(const char *logValue,
                           const EditorLogLevel level) const {
  m_console->addLog(level, logValue);
}

}  // namespace Editor
