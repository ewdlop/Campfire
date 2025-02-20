#pragma once

#include <nlohmann/json.hpp>

class Entity;

class InspectorWidget
{
public:
    void ShowInspector(Entity& entity, bool* isOpen);
    void ShowEntity(Entity& entity);

private:
    void ShowJsonObject(const nlohmann::json& json);
    void ShowComponentMenu(Entity& entity);

    template <typename T>
    void ShowComponentOptionsMenu(Entity& entity);

    template <typename T>
    void ShowComponentListOptionsMenu(Entity& entity, size_t index);
};
