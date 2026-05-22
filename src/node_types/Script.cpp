#include "pch.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "NodeEditorEx.h"
#include "Script.h"

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

void Script::onShowProperties()
{
    bool postRequired = false;
    bool otherChange = false;

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
        channel_->enabled.store(enabled_, std::memory_order_relaxed);
    }

    if (ImGui::Checkbox("Inline Editor", &inlineEditor_))
        otherChange = true;

    if (otherChange || postRequired)
        raiseModified(ChangeImpact::NodeConfig);

    if (postRequired)
        postConfig();
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

void Script::Core::onFrame(const RunContext &context) {}

} // namespace Mirael::NodeTypes
