#include "pch.h"

#include "app.h"
#include "properties.h"

namespace Mirael
{

void Mirael::Properties::show(bool &open)
{
    if (ImGui::Begin(windowName(), &open)) {

        Graph *graph = Project::get().getLastFocusedGraph();
        if (graph)
            graph->showProperties();
    }

    ImGui::End();
}

}; // namespace Mirael