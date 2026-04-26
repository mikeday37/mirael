#include "pch.h"

#include "all_node_types.h"
#include "registry.h"

namespace Mirael
{

NodeTypeRegistry::NodeTypeRegistry()
{
    auto registrar = [this]<typename... Ts>() { (registerType<Ts>(), ...); };

    ::Mirael::NodeTypes::registerAll(registrar);
}

}; // namespace Mirael