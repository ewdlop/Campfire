#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <btBulletDynamicsCommon.h>
#include "Core/Base.h"
#include "Physics/Collider.h"

class TransformComponent;

class Rigidbody
{
public:
    void Construct(const glm::vec3& pos, const glm::vec3& euler, const glm::vec3& scale);
    void Construct(const glm::vec3& pos, const glm::vec3& euler, const glm::vec3& scale, btCollisionShape* shape);

    btRigidBody* GetBulletRigidbody();
    void SetVelocity(glm::vec3 newVelocity);
    void AddVelocity(glm::vec3 velocity);
    glm::vec3 GetVelocity();
    void SetTransform(const TransformComponent& transformComp);
    void SetBodyType(const std::string& currentType);
    const std::string& GetBodyType();
    void ToggleGravity();
    void ToggleFreezePositon(int);
    void ToggleFreezeRotation(int);
public:
    enum class BodyType
    {
        STATIC = 0,
        KINEMATIC,
        DYNAMIC
    };

    BodyType type = BodyType::DYNAMIC;
    float mass = 1.0f;
    float drag = 0.0f;
    float angularDrag = 0.05f;
    bool useGravity = true;
    bool freezePosition[3] { false, false, false };
    bool freezeRotation[3] { false, false, false };

    // TODO separate collider shape from the bodies
    SharedPtr<Collider> collider = Collider::Create(Collider::Shape::BOX);

private:
    btRigidBody* bulletRigidbody = nullptr;
};

#endif // RIGIDBODY_H
