#include "app_pch.hpp"

#include "app/applet_graph_types.hpp"
#include "app/untangle_applet.hpp"

void UntangleApplet::OnShowDerivedAppletControls()
{
    if (ImGui::TreeNodeEx("Graph Manipulators", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
        if (ImGui::BeginTable("##GraphManipulators", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
            for (auto manipulator : graphManipulators_.GetAll()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button(manipulator->GetDisplayName())) {
                    ClearGraphIndicators();
                    manipulator->Manipulate(graph_);
                }
                ImGui::TableNextColumn();
                ImGui::PushID(manipulator);
                manipulator->OnShowControls();
                ImGui::PopID();
            }
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Button("Clear")) {
                ClearGraphIndicators();
                graph_.Clear();
            }
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Graph Animators", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
        if (playingAnimation_) {
            if (ImGui::Button("Pause")) {
                Pause();
            }
        } else {
            if (ImGui::Button("Play")) {
                Play();
            }
        }
        if (ImGui::BeginTable("##GraphAnimators", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
            for (auto animator : graphAnimators_.GetAll()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button(animator->GetDisplayName())) {
                    SetCurrentAnimator(animator);
                }
                ImGui::TableNextColumn();
                ImGui::PushID(animator);
                animator->OnShowControls();
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }
}

void UntangleApplet::OnEvent(const SDL_Event &e)
{
    switch (e.type) {
    case SDL_KEYDOWN:
        switch (e.key.keysym.sym) {

        case SDLK_SPACE:
            if (!currentAnimator_) {
                break;
            };

            if (playingAnimation_) {
                Pause();
            } else {
                Play();
            }
            break;

        default:
            // this applet doesn't care about other keys
            break;
        }
        break;

    default:
        // this applet doesn't care about other events
        break;
    }

    Base::OnEvent(e);
}

void UntangleApplet::OnNewFrame()
{
    if (!currentAnimator_ || !playingAnimation_) {
        return;
    }

    auto [worldTime, deltaTime] = animationTimer_.Tick();
    currentAnimator_->Animate(graph_, worldTime, deltaTime);
}

void UntangleApplet::SetCurrentAnimator(GraphAnimator *animator)
{
    ClearGraphIndicators();
    currentAnimator_ = animator;
    playingAnimation_ = true;
    animationTimer_.Reset();
}

void UntangleApplet::Play()
{
    animationTimer_.Resume();
    ClearGraphIndicators();
    playingAnimation_ = true;
}

void UntangleApplet::Pause()
{
    animationTimer_.Pause();
    playingAnimation_ = false;
}
