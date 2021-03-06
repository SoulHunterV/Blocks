#include "imgui_button.hpp"


namespace blocks
{
  ImguiButton::ImguiButton(std::string text, std::function<void()> callback) : text_(text), callback_(callback)
  {

  }


  void ImguiButton::Render()
  {
    if (ImGui::SmallButton(text_.c_str()) && callback_)
    {
      callback_();
    }
  }
}
