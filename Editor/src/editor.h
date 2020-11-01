#include "SirEngine/application.h"

class EditorApp final : public SirEngine::Application {
 public:

	EditorApp();

	virtual ~EditorApp();

  EditorApp(const EditorApp &) = delete;
  EditorApp &operator=(const EditorApp &) = delete;
  EditorApp(EditorApp &&) = delete;
  EditorApp &operator=(EditorApp &&) = delete;
};

