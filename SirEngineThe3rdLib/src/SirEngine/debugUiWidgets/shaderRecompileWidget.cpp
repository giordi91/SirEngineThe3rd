#include "SirEngine/debugUiWidgets/shaderRecompileWidget.h"
#include "SirEngine/application.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/globals.h"
#include "imgui/imgui.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/shaderManager.h"

namespace SirEngine::debug
{

struct ShaderCompileConsole {
  char InputBuf[256];
  ImVector<char *> Items;
  bool ScrollToBottom;
  ImVector<char *> History;
  int HistoryPos; // -1: new line, 0..History.Size-1 browsing history.
  ImVector<const char *> Commands;

  ShaderCompileConsole() {
    ClearLog();
    memset(InputBuf, 0, sizeof(InputBuf));
    HistoryPos = -1;
    Commands.push_back("HISTORY");
    Commands.push_back("CLEAR");
    // AddLog("Welcome to Dear ImGui!");
  }
  ~ShaderCompileConsole() {
    ClearLog();
    for (int i = 0; i < History.Size; i++)
      free(History[i]);
  }

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
  static char *Strdup(const char *str) {
    size_t len = strlen(str) + 1;
    void *buff = malloc(len);
    return (char *)memcpy(buff, (const void *)str, len);
  }
  static void Strtrim(char *str) {
    char *str_end = str + strlen(str);
    while (str_end > str && str_end[-1] == ' ')
      str_end--;
    *str_end = 0;
  }

  void ClearLog() {
    for (int i = 0; i < Items.Size; i++)
      free(Items[i]);
    Items.clear();
    ScrollToBottom = true;
  }

  void AddLog(const char *fmt, ...) IM_FMTARGS(2) {
    // FIXME-OPT
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    Items.push_back(Strdup(buf));
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
      if (ImGui::MenuItem("Close Console"))
        *p_open = false;
      ImGui::EndPopup();
    }
    // TODO: display items starting from the bottom

    if (ImGui::SmallButton("Add Dummy Error")) {
      AddLog("[error] something went wrong");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear")) {
      ClearLog();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Scroll to bottom"))
      ScrollToBottom = true;
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PopStyleVar();
    ImGui::Separator();

    const float footer_height_to_reserve =
        ImGui::GetStyle().ItemSpacing.y +
        ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
    ImGui::BeginChild(
        "ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
        ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1
                                               // InputText
    if (ImGui::BeginPopupContextWindow()) {
      if (ImGui::Selectable("Clear"))
        ClearLog();
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
                        ImVec2(4, 1)); // Tighten spacing

    ImVec4 col_default_text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    for (int i = 0; i < Items.Size; i++) {
      const char *item = Items[i];
      ImVec4 col = col_default_text;
      if (strstr(item, "[error]"))
        col = ImColor(1.0f, 0.4f, 0.4f, 1.0f);
      else if (strncmp(item, "# ", 2) == 0)
        col = ImColor(1.0f, 0.78f, 0.58f, 1.0f);
      ImGui::PushStyleColor(ImGuiCol_Text, col);
      ImGui::TextUnformatted(item);
      ImGui::PopStyleColor();
    }

    if (ScrollToBottom)
      ImGui::SetScrollHereY(1.0f);
    ScrollToBottom = false;
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();
  }
};

ShaderCompilerWidget::~ShaderCompilerWidget() {}
void ShaderCompilerWidget::initialize() {
  elementsToRender.reserve(100);
  console = new ShaderCompileConsole();
}
static bool fuzzy_match_simple(char const *pattern, char const *str) {
  while (*pattern != '\0' && *str != '\0') {
    if (tolower(*pattern) == tolower(*str))
      ++pattern;
    ++str;
  }

  return *pattern == '\0' ? true : false;
}
void ShaderCompilerWidget::render() {

  ImVec2 winPos{globals::ENGINE_CONFIG->m_windowWidth - width - 100, 0};
  ImGui::SetNextWindowPos(winPos, ImGuiSetCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiSetCond_FirstUseEver);
  if (!ImGui::Begin("Shader HOTTTT recompile", &opened)) {
    ImGui::End();
    return;
  }
  // static ImGuiTextFilter filter;
  ImGui::InputText("shader filter", shaderName, IM_ARRAYSIZE(shaderName));
  ImGui::Separator();
  const std::unordered_map<std::string, dx12::ShaderBlob> shaders =
      dx12::SHADER_MANAGER->getShaderMap();

  const char *pattern = &shaderName[0];

  elementsToRender.clear();
  for (const auto &shader : shaders) {

    bool shouldRender = fuzzy_match_simple(pattern, shader.first.c_str());
    shouldRender = strlen(pattern) == 0 ? true : shouldRender;
    if (shouldRender) {
      elementsToRender.push_back(shader.first.c_str());
    }
  }

  ImGui::ListBox("", &currentSelectedItem, elementsToRender.data(),
                 static_cast<int>(elementsToRender.size()));

  ImGui::PushItemWidth(-1.0f);
  ImGui::Checkbox("Use develop path offset", &useDevelopPath);
  ImGui::SameLine();
  ImGui::InputText("", offsetDevelopPath, IM_ARRAYSIZE(offsetDevelopPath));
  if (ImGui::Button("COMPILE SELECTED",
                    ImVec2{ImGui::GetContentRegionAvailWidth(), 30})) {
    // let s generate a compile request
    if (currentSelectedItem != -1) {
      m_currentSelectedShader = elementsToRender[currentSelectedItem];
      auto *event =
          new ShaderCompileEvent(elementsToRender[currentSelectedItem],
                                 useDevelopPath ? offsetDevelopPath : "");
      globals::APPLICATION->queueEventForEndOfFrame(event);
    }
  }

  if (ImGui::CollapsingHeader("Console Output")) {
    shouldRenderConsole = true;
    console->Draw(&shouldRenderConsole);
  } else {
    shouldRenderConsole = false;
  }

  ImGui::Separator();
  ImGui::End();
}
void ShaderCompilerWidget::log(const char *logValue) {
  console->AddLog(logValue);
}

void ShaderCompilerWidget::requestCompile() {
  if (!m_currentSelectedShader.empty()) {
    auto *event =
        new ShaderCompileEvent(m_currentSelectedShader.c_str(),
                               useDevelopPath ? offsetDevelopPath : "");
    globals::APPLICATION->queueEventForEndOfFrame(event);
  }
}
} // namespace SirEngine
