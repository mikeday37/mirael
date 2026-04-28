#include "pch.h"

#include "imgui_node_editor.h"

#include "switch.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Switch::onDeserialize(const nlohmann::json &j)
{
    if (j.empty())
        return;

    enabled    = j["enabled"].get<bool>();
    dynamic    = j["dynamic"].get<bool>();
    inputCount = j["n"].get<int>();
}

void Switch::onInit()
{
    choicePinId = getPinId("choice");
    inputs.reserve(inputCount);
    for (int n : std::views::iota(1, inputCount + 1)) {
        std::string key   = std::format("in{}", n);
        std::string label = std::format("-> {}", key);
        inputs.emplace_back(n, getPinId(key), label);
    }
    outPinId = getPinId("out");
}

void Switch::onShow()
{
    ne::BeginNode(getId());
    ImGui::PushID(getIdAsPointer());
    ImGui::Text("Switch");
    if (dynamic) {
        ne::BeginPin(choicePinId, ne::PinKind::Input);
        ImGui::Text("-> choice");
        ne::EndPin();
    }
    ne::BeginPin(outPinId, ne::PinKind::Output);
    ImGui::Text("out ->");
    ne::EndPin();
    for (const auto &[n, id, label] : inputs) {
        ne::BeginPin(id, ne::PinKind::Input);
        if (dynamic)
            ImGui::Text(label.c_str());
        else {
            if (ImGui::RadioButton(label.c_str(), manualChoice == n)) {
                manualChoice = n;
            }
        }
        ne::EndPin();
    }
    ImGui::PopID();
    ne::EndNode();
}

void Switch::onSerialize(nlohmann::json &j) const {}

}; // namespace Mirael::NodeTypes
