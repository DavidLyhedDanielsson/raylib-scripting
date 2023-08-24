#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <string>
#include <vector>

#include <profiling.hpp>

#include <component/acceleration.hpp>
#include <component/health.hpp>
#include <component/move_towards.hpp>
#include <component/transform.hpp>
#include <component/velocity.hpp>

std::optional<float> TimeToCollisionSphere(
    const Vector2 position,
    const Vector2 otherPosition,
    const Vector2 velocity,
    const Vector2 otherVelocity,
    const float radius)
{
    float totalRadius = radius + radius;
    float radiusSquared = totalRadius * totalRadius;

    float distanceSquared = Vector2DistanceSqr(position, otherPosition);
    if(distanceSquared < radiusSquared)
        return 0.0f;

    Vector2 relPos = Vector2Subtract(otherPosition, position);
    Vector2 relVel = Vector2Subtract(velocity, otherVelocity);

    float a = Vector2DotProduct(relVel, relVel);
    float b = Vector2DotProduct(relPos, relVel);
    float c = Vector2DotProduct(relPos, relPos) - radiusSquared;

    float disc = b * b - a * c;
    if(disc < 0.0f || a <= 0.0f)
        return std::nullopt;

    float t = (b - std::sqrt(disc)) / a;

    if(t <= 0.0f)
        return std::nullopt;

    return t;
}

namespace System
{
    void AvoidEntities(entt::registry& registry, float ksi, float avoidanceT, float time)
    {
        for(auto [entity, transform, moveTowards, velocityComponent, acceleration] :
            registry
                .view<
                    Component::Transform,
                    Component::MoveTowards,
                    Component::Velocity,
                    Component::Acceleration>()
                .each())
        {
            PROFILE_SCOPE((ENTT_ID_TYPE)entity);

            const float radius = 0.30f;

            float forceScaleFactor = ksi / 2.0f;
            if(ksi < 2.0f)
                forceScaleFactor = 1.0f;

            Vector2 separationForce = Vector2Zero();
            Vector2 forces = {
                .x = acceleration.acceleration.x / time,
                .y = acceleration.acceleration.z / time,
            };

            const Vector2 velocity = {.x = velocityComponent.x, .y = velocityComponent.z};

            for(auto [otherEntity, otherTransform, otherHealth] :
                registry.view<Component::Transform, Component::Health>().each())
            {
                if(entity == otherEntity)
                    continue;

                float distance = Vector3Distance(transform.position, otherTransform.position);

                if(distance > 3.0f)
                    continue;

                Vector2 otherVelocity = Vector2Zero();
                if(Component::Velocity* oVel = registry.try_get<Component::Velocity>(otherEntity);
                   oVel)
                {
                    otherVelocity = {.x = oVel->ToVector3().x, .y = oVel->ToVector3().z};
                };
                Vector2 position = {transform.position.x, transform.position.z};
                Vector2 otherPosition = {otherTransform.position.x, otherTransform.position.z};

                if(distance < 1.0f)
                {
                    separationForce = Vector2Add(
                        separationForce,
                        Vector2Scale(Vector2DirectionTo(otherPosition, position), 1.0f / distance));
                }

                std::optional<float> timeToCollision =
                    TimeToCollisionSphere(position, otherPosition, velocity, otherVelocity, radius);

                if(!timeToCollision.has_value())
                    continue;

                float t = timeToCollision.value();

                if(t == 0.0f)
                {
                    Vector2 avoidanceForce = Vector2DirectionTo(otherPosition, position);
                    float magnitude = Vector2Length(velocity);

                    if(magnitude > 10.0f)
                        magnitude = 10.0f;

                    forces.x += avoidanceForce.x * magnitude * forceScaleFactor;
                    forces.y += avoidanceForce.y * magnitude * forceScaleFactor;
                }
                else
                {
                    Vector2 avoidanceForce = Vector2Normalize(Vector2Subtract(
                        Vector2Add(position, Vector2Scale(velocity, t)),
                        Vector2Add(otherPosition, Vector2Scale(otherVelocity, t))));

                    float magnitude = 0.0f;
                    if(t >= 0.0f && t < avoidanceT)
                        magnitude = (avoidanceT - t) / (t + 0.001f);

                    if(magnitude > 20.0f)
                        magnitude = 20.0f;

                    forces.x += avoidanceForce.x * magnitude * forceScaleFactor;
                    forces.y += avoidanceForce.y * magnitude * forceScaleFactor;
                }
            }

            forces.x += separationForce.x;
            forces.y += separationForce.y;

            acceleration.acceleration.x += forces.x * time;
            acceleration.acceleration.z += forces.y * time;
        }
    }
}