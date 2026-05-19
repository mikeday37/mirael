#include "pch.h"

#include "ine/imgui_node_editor.h"

#include "Counter.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Mirael::NodeTypes::Counter::onDeserialize(const nlohmann::json &j)
{
	config_.step = j["step"].get<value_t>();
    config_.clipMin = j["clipMin"].get<bool>();
    config_.clipMax = j["clipMax"].get<bool>();
	config_.minValue = j["min"].get<value_t>();
	config_.maxValue = j["max"].get<value_t>();
    config_.wrap = j["wrap"].get<bool>();
}

void Counter::onInit()
{
	outPinId_ = addPin("out", {.direction = PinDirection::Output});
}

void Counter::onShow()
{
    ne::BeginNode(getId());
    ImGui::PushID(getIdAsPointer());
    ImGui::Text("Counter: %d", getValue());
    ImGui::SameLine();
    ne::BeginPin(outPinId_, ne::PinKind::Output);
    ImGui::Text("->");
    ne::EndPin();
    ImGui::PopID();
    ne::EndNode();
}

void Counter::onSerialize(nlohmann::json & j) const
{
	j["step"] = config_.step;
    j["clipMin"] = config_.clipMin;
    j["clipMax"] = config_.clipMax;
	j["min"] = config_.minValue;
	j["max"] = config_.maxValue;
    j["wrap"] = config_.wrap;
}

void Counter::onShowProperties()
{
    bool changed = ImGui::InputInt("Step", &config_.step);
    changed |= ImGui::Checkbox("Clip Min", &config_.clipMin);
    if (config_.clipMin)
        changed |= ImGui::InputInt("Min Value", &config_.minValue);
    changed |= ImGui::Checkbox("Clip Max", &config_.clipMax);
    if (config_.clipMax)
        changed |= ImGui::InputInt("Max Value", &config_.maxValue);
    changed |= ImGui::Checkbox("Wrap", &config_.wrap);
    if (changed)
        putConfig();
}

void Counter::Core::onFrame(const RunContext& context)
{
    auto config = getConfig(); // gets latest config from the channel

    // TODO: note: the below logic is likely flawed - but not worth resolving at the moment because this is a temporary/throwaway node type
    value += config.step;
    const value_t range = 1 + config.maxValue - config.minValue;
    if (config.clipMax && value > config.maxValue) {
        if (config.wrap)
            value -= range;
        else
            value = config.maxValue;
    }
    if (config.clipMin && value < config.minValue) {
        if (config.wrap)
            value += range;
        else
            value = config.minValue;
    }

    putValue(); // puts updated value on the channel

    // now we need to write to the output buffer
    auto outBufferPtr = context.outputs.at(outPinId_);
    assert(outBufferPtr); // should have been created by the Runner before onFrame() was called
    auto &outBuffer = *outBufferPtr;
    outBuffer.value = std::format("{}", value);
}

} // namespace Mirael::NodeTypes
