#include "pch.h"

#include <charconv>
#include <utility>

#include "imgui.h"
#include "ine/imgui_node_editor.h"
#include "ine/utilities/widgets.h"

#include "App.h"
#include "data.h"
#include "NodeEditorEx.h"
#include "Switch.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Switch::onDeserialize(const nlohmann::json &j)
{
    if (j.empty())
        return;

    enabled_    = j["enabled"].get<bool>();
    dynamic_    = j["dynamic"].get<bool>();
    inputCount_ = j["n"].get<int>();
    if (j.contains("choice"))
        manualChoice_ = j["choice"].get<int>();
}

void Switch::onInit()
{
    choicePinId_ = 0;
    if (dynamic_)
        handleToggleDynamic();

    inputs_.reserve(inputCount_);
    for (int n : std::views::iota(1, inputCount_ + 1)) {
        addSwitchInputPin(n);
    }

    outPinId_ = addPin("out", {.direction = PinDirection::Output});
}

void Switch::onOrderPins(std::vector<PinId> &pinOrder)
{
    pinOrder.clear();
    pinOrder.reserve(getPinCount());

    if (dynamic_)
        pinOrder.push_back(choicePinId_);

    for (auto &input : inputs_)
        pinOrder.push_back(input.id);

    outPinIndex_ = pinOrder.size();
    pinOrder.push_back(outPinId_);
}

void Switch::onShow()
{
    bool changed = false;
    auto pins    = std::span{getPinOrder()};

    NodeEditorEx::StandardNode(
        *this,                                              // node
        []() -> void { ImGui::TextUnformatted("Switch"); }, // header UI
        pins.subspan(0, outPinIndex_),                      // input pin order
        pins.subspan(outPinIndex_),                         // output pin order
        [this](size_t index, PinId id, PinDirection dir) -> float {
            switch (dir) {
            case PinDirection::Input:
                if (dynamic_ && id == choicePinId_) {
                    return ImGui::CalcTextSize("choice").x;
                } else {
                    auto &[n, checkPinId, label] = dynamic_ ? inputs_[index - 1] : inputs_[index];
                    assert(id == checkPinId);
                    if (dynamic_)
                        return ImGui::CalcTextSize(label.c_str()).x;
                    else {
                        return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(label.c_str()).x;
                    }
                }
                break;
            case PinDirection::Output:
                return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize("out").x;
            default:
                assert(false); // this should not occur
                return 0;
            }
        },
        [this, &changed](size_t index, PinId id, PinDirection dir) -> void {
            switch (dir) {
            case PinDirection::Input:
                if (dynamic_ && id == choicePinId_) {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("choice");
                } else {
                    auto &[n, checkPinId, label] = dynamic_ ? inputs_[index - 1] : inputs_[index];
                    assert(id == checkPinId);
                    if (dynamic_)
                        ImGui::TextUnformatted(label.c_str());
                    else {
                        if (ImGui::RadioButton(label.c_str(), manualChoice_ == n)) {
                            if (manualChoice_ != n)
                                changed = true;
                            manualChoice_ = n;
                        }
                    }
                }
                break;
            case PinDirection::Output:
                if (ImGui::Checkbox("out", &enabled_))
                    changed = true;
                break;
            default:
                assert(false); // this should not occur
                break;
            }
        });

    if (changed) {
        raiseModified(ChangeImpact::NodeConfig);
        postConfig();
    }
}

void Switch::onSerialize(nlohmann::json &j) const
{
    j["enabled"] = enabled_;
    j["dynamic"] = dynamic_;
    j["n"]       = inputCount_;
    if (manualChoice_ != 0)
        j["choice"] = manualChoice_;
}

void Switch::onShowProperties()
{
    bool changed = false;

    int priorInputCount = inputCount_;
    ImGui::InputInt("Inputs", &inputCount_);
    inputCount_ = std::clamp(inputCount_, 1, 100);
    if (priorInputCount != inputCount_) {
        changed = true;
        if (inputCount_ > priorInputCount)
            expandInputs();
        else {
            assert(inputCount_ < priorInputCount);
            reduceInputs();
        }
    }

    changed |= ImGui::Checkbox("Enabled", &enabled_);

    bool preDynamic = dynamic_;
    changed |= ImGui::Checkbox("Dynamic", &dynamic_);
    if (preDynamic != dynamic_)
        handleToggleDynamic();

    if (!dynamic_)
        changed |= ImGui::InputInt("Manual Choice", &manualChoice_);

    if (changed) {
        raiseModified(ChangeImpact::NodeConfig);
        postConfig();
    }
}

void Switch::Core::onFrame(const RunContext &context)
{
    acceptLatestConfig();

    auto output = context.getOutput(config_.outPin);

    if (!config_.enabled) {
        if (output)
            output->clear();
        return;
    }

    int choice;

    if (config_.dynamic) {
        if (auto buf = context.getFirstInput(config_.choicePin)) {
            auto val = buf->getValue();
            int parsed{};
            auto [ptr, err] = std::from_chars(val.data(), val.data() + val.size(), parsed);
            if (err == std::errc{})
                choice = parsed;
            else
                choice = 0; // TODO: in this case we may also want to set a visual error flag
        } else {
            choice = 0;
        }
    } else {
        choice = config_.manualChoice;
    }

    if (choice < 1 || choice > config_.inPins.size()) {
        if (output)
            output->clear();
        // TODO: also set visual error here
        return;
    }

    auto input = context.getFirstInput(config_.inPins[choice - 1]);
    if (output) {
        if (input)
            output->setAsReferenceTo(input);
        else
            output->clear();
    }
}

void Switch::expandInputs()
{
    if (inputCount_ > inputs_.size())
        inputs_.reserve(inputCount_);

    while (inputCount_ > inputs_.size()) {
        int pinNum = static_cast<int>(inputs_.size()) + 1;
        addSwitchInputPin(pinNum);
    }
}

void Switch::reduceInputs()
{
    auto priorSize = static_cast<int>(inputs_.size());

    inputs_.resize(inputCount_);

    while (priorSize > inputCount_) {
        int pinNum = priorSize--;
        removeSwitchInputPin(pinNum);
    }
}

void Switch::addSwitchInputPin(int pinNumber)
{
    std::string key = std::format("in{}", pinNumber);
    auto pinId      = addPin(key, {.direction = PinDirection::Input});
    inputs_.emplace_back(pinNumber, pinId, key);
}

void Switch::removeSwitchInputPin(int pinNumber)
{
    std::string key = std::format("in{}", pinNumber);
    removePin(key);
}

void Switch::handleToggleDynamic()
{
    if (dynamic_ && !choicePinId_) {
        choicePinId_ = addPin("choice", {.direction = PinDirection::Input});
    } else if (choicePinId_ && !dynamic_) {
        removePin("choice");
        choicePinId_ = 0;
    } else {
        assert(false); // the above two cases should be the only cases we encounter
    }
}

} // namespace Mirael::NodeTypes
