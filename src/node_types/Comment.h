#pragma once

#include "Node.h"

namespace Mirael::NodeTypes
{

class Comment : public Node
{
public:
    static const char *typeName() { return "comment"; }

    void onDeserialize(const nlohmann::json &j) override;
    void onInit() override;
    void onShow() override;
    void onSerialize(nlohmann::json &j) const override;

private:
    std::string comment_;
};

} // namespace Mirael::NodeTypes