#include "app_pch.hpp"

#include "app/testapplet2.hpp"
#include "app/color.hpp"
#include "app/graphics.hpp"

void TestApplet2::OnRenderBackground(Graphics &g) {
    g.Circle({10.0f, 10.0f}, 10.f, 3.0f, Color::White, Color::Black);
    g.Circle({30.0f, 10.0f}, 10.f, 3.0f, Color::White, Color::Black);
}

void TestApplet2::OnShowControls() {
	ImGui::Button("Pointless button!");
}
