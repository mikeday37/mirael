#include "pch.h"

#include "app.h"
#include "settings.h"

namespace Mirael
{

void Mirael::Settings::show(bool &open)
{
    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(windowName(), &open)) {
    }
    ImGui::End();
}

}; // namespace Mirael