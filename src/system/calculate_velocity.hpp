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

namespace System
{
    // Apply an entity's acceleration to their velocity
    void CalculateVelocity(entt::registry& registry, float time)
    {
        for(auto [entity, velocity, acceleration] :
            registry.view<Component::Velocity, Component::Acceleration>().each())
        {
            float length = Vector3Length(acceleration.acceleration);
            // Cap the acceleration if it is too big, this is a limitation of
            // force-based collision avoidance
            if(length > 20.0f * time)
            {
                acceleration.acceleration =
                    Vector3Scale(Vector3Normalize(acceleration.acceleration), 20.0f * time);
            }

            // I ran into some NaN issues, so just a safeguard
            assert(!std::isnan(acceleration.acceleration.x));
            assert(!std::isnan(acceleration.acceleration.y));
            assert(!std::isnan(acceleration.acceleration.z));

            velocity.x += acceleration.acceleration.x;
            velocity.y += acceleration.acceleration.y;
            velocity.z += acceleration.acceleration.z;

            acceleration.acceleration = {0.0f, 0.0f, 0.0f};
        }
    }
}