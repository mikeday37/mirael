#include "pch.h"

#include "app.h"
#include "diagnostics.h"

namespace Mirael
{

void Mirael::Diagnostics::show(bool &open)
{
    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(windowName(), &open)) {
    }
    ImGui::End();
}

}; // namespace Mirael