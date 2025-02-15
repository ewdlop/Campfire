#pragma once

#include <ImGuizmo.h>

class Camera;
class Entity;

class TransformWidget
{
public:
    void UpdateViewport(uint32_t width, uint32_t height);
    void EditTransform(Entity& entity, const Camera& editorCamera);
    void ShowTransformSettings(bool* isOpen);

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;

private:
    bool useSnap = false;
    bool boundSizing = false;
    bool boundSizingSnap = false;

    float snap[3] = { 1.f, 1.f, 1.f };
    float bounds[6] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
    float boundsSnap[3] = { 0.1f, 0.1f, 0.1f };

    ImGuizmo::MODE mode = ImGuizmo::LOCAL;
};
