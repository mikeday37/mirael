#pragma once

#include "Node.h"

namespace Mirael::NodeTypes
{

class Comment : public Node
{
public:
    static const char *typeName() { return "comment"; }

    virtual void onDeserialize(const nlohmann::json &j);
    void onInit() override;
    void onShow() override;
    virtual void onSerialize(nlohmann::json &j) const;

private:
    std::string comment_;
};

}; // namespace Mirael::NodeTypes