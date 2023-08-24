#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <string>
#include <vector>

#include <navigation.hpp>
#include <profiling.hpp>
#include <random>

#include <component/acceleration.hpp>
#include <component/health.hpp>
#include <component/move_towards.hpp>
#include <component/transform.hpp>
#include <component/velocity.hpp>

static std::random_device rd;
static std::mt19937 mt(rd());
// This range is awctually [-1, 1), but that's fine
static std::uniform_real_distribution<float> randomNumber(-1.0, 1.0f);

namespace System
{
    // For all enemies that are moving towards a goal, navigate in the environment and update their
    // acceleration
    void Navigate(entt::registry& registry, Navigation& navigation, float ksi, float time)
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

            Vector2 tilePos = navigation.GetTileSpace(Vector3Flatten(transform.position));
            if(navigation.IsGoal(tilePos.x, tilePos.y))
            {
                if(auto health = registry.try_get<Component::Health>(entity); health)
                    health->currentHealth = 0.0f;
                continue;
            }

            Vector2 force =
                navigation.GetForce(moveTowards.vectorFieldId, Vector3Flatten(transform.position));

            Vector3 movementDirection = {force.x, 0.0f, force.y};

            float speed = moveTowards.speed;
            Vector3 goalVelocity = Vector3Scale(movementDirection, speed);

            const Vector2 velocity = {.x = velocityComponent.x, .y = velocityComponent.z};
            Vector2 forces =
                Vector2Scale(Vector2Subtract(Vector3Flatten(goalVelocity), velocity), ksi);

            forces = Vector2Add(forces, Vector2Scale({randomNumber(mt), randomNumber(mt)}, 0.5f));

            acceleration.acceleration.x += forces.x * time;
            acceleration.acceleration.z += forces.y * time;
        }
    }
}