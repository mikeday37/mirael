#include "pch.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "Graph.h"
#include "ImGuiEx.h"
#include "NodeEditorEx.h"
#include "Script.h"
#include "ScriptCore.h"

namespace Mirael::NodeTypes
{

void Script::onDeserialize(const nlohmann::json &j)
{
    scriptName_   = j["name"].get<std::string>();
    inputsCsv_    = j["inputs"].get<std::string>();
    outputsCsv_   = j["outputs"].get<std::string>();
    inlineEditor_ = j["inline"].get<bool>();
    script_       = j["script"].get<std::string>();
    enabled_      = j["enabled"].get<bool>();

    auto rawCompileMode = j["compile"].get<std::string>();
    if (rawCompileMode == "none")
        compileMode_ = ScriptCompilationMode::None;
    else if (rawCompileMode == "live")
        compileMode_ = ScriptCompilationMode::Live;
    else if (rawCompileMode == "explicit")
        compileMode_ = ScriptCompilationMode::Explicit;
    else
        throw std::runtime_error(
            std::format("Error during Script node deserializatoin: unknown script compilation mode: {}", rawCompileMode));
}

void Script::onInit()
{
    if (!isDeserializing() && scriptName_.empty())
        scriptName_ = std::format("Script (node {})", getId());

    establishPins(PinDirection::Input, inputsCsv_, inputLabels_, inputPinIds_);
    establishPins(PinDirection::Output, outputsCsv_, outputLabels_, outputPinIds_);

    if (!script_.empty())
    {
        latestPostedScriptVersion_ = 1;
        scriptVersion_ = 1;
    }
}

void Script::onOrderPins(std::vector<PinId> &pinOrder)
{
    pinOrder.clear();
    pinOrder.reserve(inputPinIds_.size() + outputPinIds_.size());

    for (auto id : inputPinIds_)
        pinOrder.push_back(id);

    for (auto id : outputPinIds_)
        pinOrder.push_back(id);
}

void Script::onShow()
{
    auto pins = std::span{getPinOrder()};

    auto getPinLabels = [this](PinDirection dir) -> std::vector<std::string> * {
        switch (dir) {
        case PinDirection::Input:
            return &inputLabels_;
        case PinDirection::Output:
            return &outputLabels_;
        default:
            assert(false); // this should not occur
            return nullptr;
        }
    };

    NodeEditorEx::StandardNode(
        *this,                                                             // node
        [this]() -> void { ImGui::TextUnformatted(scriptName_.c_str()); }, // header UI
        pins.subspan(0, inputPinIds_.size()),                              // input pin order
        pins.subspan(inputPinIds_.size()),                                 // output pin order
        [&](size_t index, PinId id, PinDirection dir) -> float {
            auto labels = getPinLabels(dir);
            if (labels) {
                return ImGui::CalcTextSize((*labels)[index].c_str()).x;
            } else {
                return 0;
            }
        },
        [&](size_t index, PinId id, PinDirection dir) -> void {
            auto labels = getPinLabels(dir);
            if (labels)
                ImGui::TextUnformatted((*labels)[index].c_str());
        });

    // TODO: add inline editor
}

void Script::onSerialize(nlohmann::json &j) const
{
    j["lang"]    = "lua"; // hardcoded until we support others
    j["name"]    = scriptName_;
    j["inputs"]  = inputsCsv_;
    j["outputs"] = outputsCsv_;
    j["inline"]  = inlineEditor_;
    j["script"]  = script_;
    j["enabled"] = enabled_;

    switch (compileMode_) {
        using enum ScriptCompilationMode;
    case None:
        j["compile"] = "none";
        break;
    case Live:
        j["compile"] = "live";
        break;
    case Explicit:
        j["compile"] = "explicit";
        break;
    default:
        throw std::runtime_error(std::format("Error during Script node serializatoin: unknown script compilation mode: {}",
                                             static_cast<int>(compileMode_)));
    }
}

namespace
{

const char *to_display_string(Script::ScriptCompilationMode mode)
{
    switch (mode) {
        using enum Script::ScriptCompilationMode;
    case None:
        return "None";
    case Live:
        return "Live";
    case Explicit:
        return "Explicit";
    default:
        assert(false);
        return "(unknown)";
    }
}

const char *to_display_string(Script::ScriptStatus status)
{
    switch (status) {
        using enum Script::ScriptStatus;
    case Empty:
        return "Empty";
    case Good:
        return "Good";
    case CompileError:
        return "Compile Error";
    case RuntimeError:
        return "Runtime Error";
    default:
        return "(unknown)";
    }
}

bool isError(Script::ScriptStatus status)
{
    return status == Script::ScriptStatus::CompileError || status == Script::ScriptStatus::RuntimeError;
}

} // namespace

void Script::onShowProperties()
{
    bool postRequired = false;
    bool otherChange  = false;

    if (ImGui::InputText("Name", &scriptName_))
        otherChange = true;

    if (ImGui::InputText("Input Pins", &inputsCsv_)) {
        postRequired = true;
        establishPins(PinDirection::Input, inputsCsv_, inputLabels_, inputPinIds_);
    }

    if (ImGui::InputText("Output Pins", &outputsCsv_)) {
        postRequired = true;
        establishPins(PinDirection::Output, outputsCsv_, outputLabels_, outputPinIds_);
    }

    if (ImGui::Checkbox("Enabled", &enabled_)) {
        otherChange = true;
        putEnabled();
    }

    if (ImGui::Checkbox("Inline Editor", &inlineEditor_))
        otherChange = true;

    if (ImGui::BeginCombo("Compile Mode", to_display_string(compileMode_), ImGuiComboFlags_WidthFitPreview)) {
        static constexpr ScriptCompilationMode compileModes[] = {ScriptCompilationMode::None, ScriptCompilationMode::Live,
                                                                 ScriptCompilationMode::Explicit};
        for (auto mode : compileModes) {
            bool selected = mode == compileMode_;
            if (ImGui::Selectable(to_display_string(mode), selected)) {
                compileMode_ = mode;
                otherChange  = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
    if (ImGui::BeginTable("##statusTable", 2, tableFlags)) {

        ImGuiEx::RowLabel("Version - Posted");
        ImGui::Text("%llu", latestPostedScriptVersion_);

        updateCoreStatus();

        ImGuiEx::RowLabel("Version - Received");
        ImGui::Text("%llu", coreStatus_.receivedScriptVersion);

        ImGuiEx::RowLabel("Version - Running");
        ImGui::Text("%llu", coreStatus_.runningScriptVersion);

        ImGuiEx::RowLabel("Status");
        ImGui::TextUnformatted(to_display_string(coreStatus_.scriptStatus));
        if (autoDisabled_) {
            ImGui::SameLine(0, 0);
            ImGui::TextUnformatted(", Auto-Disabled");
        }

        ImGuiEx::RowLabel("Error Text");
        ImGui::TextUnformatted(isError(coreStatus_.scriptStatus) ? coreStatus_.errorText.c_str() : "");

        ImGui::EndTable();
    }

    if (ImGui::Button("Compile Now")) {
        if (ScriptCompilationMode::None != compileMode_) {
            ++scriptVersion_;
            postRequired = true;
        }
    }

    if (ImGui::InputTextMultiline("Script", &script_, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput)) {
        otherChange = true;
        if (ScriptCompilationMode::Live == compileMode_) {
            ++scriptVersion_;
            postRequired = true;
        }
    }

    if (otherChange || postRequired)
        raiseModified(ChangeImpact::NodeConfig);

    if (postRequired)
        postConfig();
}

std::unique_ptr<NodeCore> Script::createCore()
{
    return std::make_unique<Cores::ScriptCore>(buildConfig(), channel_, buildDebugInfo());
}

Script::DebugInfo Script::buildDebugInfo()
{
    const auto &g = getGraph();
    return DebugInfo{.graphNameWhenCreated = std::string{g.getName()},
                     .graphUid             = std::string{g.getUid()},
                     .graphId              = g.getId(),
                     .nodeId               = getId()};
}

void Script::establishPins(PinDirection dir, const std::string &csv, std::vector<std::string> &labels, std::vector<PinId> &pinIds)
{
    labels.clear();
    for (auto &&part : csv | std::views::split(',')) {
        labels.emplace_back(part.begin(), part.end());
    }

    const char *prefix = dir == PinDirection::Input ? "in" : "out";

    while (labels.size() < pinIds.size()) {
        auto key = std::format("{}{}", prefix, pinIds.size());
        removePin(key);
        pinIds.pop_back();
    }

    while (labels.size() > pinIds.size()) {
        auto key = std::format("{}{}", prefix, pinIds.size() + 1);
        auto id  = addPin(key, {.direction = dir});
        pinIds.push_back(id);
    }
}

} // namespace Mirael::NodeTypes
