#pragma once

#include "imgui.h"

#include <concepts>

#include "data.h"
#include "Node.h"

namespace Mirael::NodeEditorEx
{

namespace StandardNodeCallback
{

template <typename F>
concept Header = std::invocable<F> && std::same_as<std::invoke_result_t<F>, void>;

template <typename F>
concept GetPinWidth =
    std::invocable<F, size_t, PinId, PinDirection> && std::same_as<std::invoke_result_t<F, size_t, PinId, PinDirection>, float>;

template <typename F>
concept Pin =
    std::invocable<F, size_t, PinId, PinDirection> && std::same_as<std::invoke_result_t<F, size_t, PinId, PinDirection>, void>;

} // namespace StandardNodeCallback

namespace StandardNodeHelper
{

class Builder
{
public:
    Builder(Node &node) : node_(node) {}

    void begin();

    void preHeader();
    void postHeader();

    void prePin(PinId id, PinDirection dir);
    void postPin(PinDirection dir);

    void missingPin(float width);
    void spacing(float width);

    void end();

    void drawIcon();

private:
    Node &node_;
    ImVec2 headerMin_, headerMax_;
};

} // namespace StandardNodeHelper

template <StandardNodeCallback::Header Header, StandardNodeCallback::GetPinWidth GetPinWidth, StandardNodeCallback::Pin Pin>
void StandardNode(Node &node,                    //
                  Header header,                 // header() -> void
                  std::span<PinId> inputPinIds,  //
                  std::span<PinId> outputPinIds, //
                  GetPinWidth getPinWidth,       // getPinWidth(PinId, PinDirection) -> float
                  Pin pin,                       // pin(PinId, PinDirection) -> void
                  float extraMiddleSpacing = 0   //
)
{
    StandardNodeHelper::Builder builder(node);

    builder.begin();

    builder.preHeader();
    header();
    builder.postHeader();

    float maxInputWidth = 0, maxOutputWidth = 0;

    for (size_t index : std::views::iota(0u, inputPinIds.size()))
        maxInputWidth = std::max(maxInputWidth, getPinWidth(index, inputPinIds[index], PinDirection::Input));

    for (size_t index : std::views::iota(0u, outputPinIds.size()))
        maxOutputWidth = std::max(maxOutputWidth, getPinWidth(index, outputPinIds[index], PinDirection::Output));

    float totalMiddleSpacing = ImGui::GetStyle().ItemSpacing.x * 2.0f + extraMiddleSpacing;

    size_t rowCount = std::max(inputPinIds.size(), outputPinIds.size());

    for (size_t index : std::views::iota(0u, rowCount)) {

        PinDirection dir = PinDirection::Input;

        bool left  = index < inputPinIds.size();
        bool right = index < outputPinIds.size();

        if (left) {
            PinId id = inputPinIds[index];
            builder.prePin(id, dir);
            float preX = ImGui::GetCursorPosX();
            pin(index, id, dir);
            ImGui::SameLine(0, 0);
            float pinWidth = ImGui::GetCursorPosX() - preX;
            builder.postPin(dir);
            ImGui::SameLine(0, 0);
            builder.spacing(maxInputWidth - pinWidth);
        } else {
            builder.missingPin(maxInputWidth);
        }

        if (left && right)
            ImGui::SameLine(0, 0);

        dir = PinDirection::Output;

        if (right) {
            PinId id        = outputPinIds[index];
            float thisWidth = getPinWidth(index, id, dir);
            builder.spacing(totalMiddleSpacing + maxOutputWidth - thisWidth);
            ImGui::SameLine(0, 0);
            builder.prePin(id, dir);
            pin(index, id, dir);
            builder.postPin(dir);
        }
    }

    builder.end();
}

} // namespace Mirael::NodeEditorEx
