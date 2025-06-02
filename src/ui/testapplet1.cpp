#include "ui_pch.hpp"

#include "ui/testapplet1.hpp"
#include "ui/color.hpp"
#include "ui/graphics.hpp"

void TestApplet1::OnRenderBackground(Graphics &g) {
    g.Rectangle({10.0f, 10.0f}, {20.f, 20.f}, 3.0f, Color::White, Color::Black);
}

void TestApplet1::OnShowMenu() {
	if (ImGui::BeginMenu("Test One")) {
        ImGui::MenuItem("Just a test");
        ImGui::MenuItem("And another");
        ImGui::EndMenu();
    }
}
