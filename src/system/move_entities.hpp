#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <string>
#include <vector>

#include <navigation.hpp>
#include <profiling.hpp>
#include <random>

#include <entity/acceleration.hpp>
#include <entity/health.hpp>
#include <entity/move_towards.hpp>
#include <entity/transform.hpp>
#include <entity/velocity.hpp>

static std::random_device rd;
static std::mt19937 mt(rd());
// This range is awctually [-1, 1), but that's fine
static std::uniform_real_distribution<float> randomNumber(-1.0, 1.0f);

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

Vector2 linePointDistance(Vector2 p0, Vector2 p1, Vector2 point)
{
    float a = Vector2DotProduct(Vector2Subtract(point, p0), Vector2Subtract(p1, p0));
    if(a <= 0.0f)
        return p0;

    float b = Vector2DotProduct(Vector2Subtract(point, p1), Vector2Subtract(p0, p1));
    if(b <= 0.0f)
        return p1;

    return Vector2Add(p0, Vector2Scale(Vector2Subtract(p1, p0), a / (a + b)));
}

// https://ericleong.me/research/circle-line/
std::optional<Vector2> lineLineIntersection(
    Vector2 start0,
    Vector2 end0,
    Vector2 start1,
    Vector2 end1,
    bool& isOn0,
    bool& isOn1)
{
    float a1 = end0.y - start0.y;
    float b1 = start0.x - end0.x;
    float c1 = a1 * start0.x + b1 * start0.y;

    float a2 = end1.y - start1.y;
    float b2 = start1.x - end0.x;
    float c2 = a2 * start1.x + b2 * start1.y;

    float det = a1 * b2 - a2 * b1;
    if(std::abs(det) < 0.0000000000001f)
        return std::nullopt;

    float x = (b2 * c1 - b1 * c2) / det;
    float y = (a1 * c2 - a2 * c1) / det;

    float d0 = x - std::min(start0.x, end0.x);
    float d1 = std::max(start0.x, end0.x) - x;
    float d2 = y - std::min(start0.y, end0.y);
    float d3 = std::max(start0.y, end0.y) - y;

    const float epsilon = -7.0e-4f;
    if(d0 >= epsilon && d1 >= epsilon && d2 >= epsilon && d3 >= epsilon)
        isOn0 = true;

    float d4 = x - std::min(start1.x, end1.x);
    float d5 = std::max(start1.x, end1.x) - x;
    float d6 = y - std::min(start1.y, end1.y);
    float d7 = std::max(start1.y, end1.y) - y;

    if(d4 >= epsilon && d5 >= epsilon && d6 >= epsilon && d7 >= epsilon)
        isOn1 = true;

    return {{
        .x = x,
        .y = y,
    }};
}

Vector2 closestPointLine(Vector2 start, Vector2 end, Vector2 p, bool& isOnLine)
{
    float a1 = end.y - start.y;
    float b1 = start.x - end.x;
    float c1 = a1 * start.x + b1 * start.y;
    float c2 = -b1 * p.x + a1 * p.y;
    float det = a1 * a1 + b1 * b1;
    if(std::abs(det) > 0.0000000000001f)
    {
        Vector2 point = {
            .x = (a1 * c1 - b1 * c2) / det,
            .y = (a1 * c2 + b1 * c1) / det,
        };

        float dist = Vector2Distance(start, end) - Vector2Distance(start, point)
                     - Vector2Distance(point, end);

        isOnLine = std::abs(dist) < 0.00001f;

        return point;
    }
    else
    {
        isOnLine = true;
        return {.x = p.x, .y = p.y};
    }
}

Vector2 closestPointLine(Vector2 start, Vector2 end, Vector2 p)
{
    bool _ = false;
    return closestPointLine(start, end, p, _);
}

// TODO: Undo changes to this and just use Eric's formulas?
std::optional<float> TimeToCollisionCircleLine(
    const Vector2 circlePosition,
    const Vector2 velocity,
    const float radius,
    const Vector2 lineStart,
    const Vector2 lineEnd)
{
    Vector2 goalPosition = Vector2Add(circlePosition, velocity);

    auto GetLineTTC = [&](Vector2 a) {
        Vector2 ac = Vector2DirectionTo(a, circlePosition);
        Vector2 p1c = Vector2DirectionTo(lineEnd, circlePosition);
        Vector2 p = Vector2Subtract(
            a,
            Vector2Scale(
                Vector2Normalize(velocity),
                radius * (Vector2Dot(ac, ac) / Vector2Dot(p1c, p1c))));

        float speed = Vector2Length(velocity);
        return Vector2Distance(circlePosition, p) / speed;
    };

    auto GetEndpointTTC = [&](Vector2 endpoint) {
        Vector2 closestPoint = closestPointLine(circlePosition, goalPosition, endpoint);

        float distance = Vector2Distance(endpoint, closestPoint);
        float intersectionDepth = std::sqrt(radius * radius - distance * distance);

        float distanceToClosestPoint = Vector2Length(Vector2Subtract(closestPoint, circlePosition));

        Vector2 p = Vector2Add(
            circlePosition,
            Vector2Scale(
                Vector2DirectionTo(circlePosition, closestPoint),
                distanceToClosestPoint - intersectionDepth));

        float speed = Vector2Length(velocity);
        return Vector2Distance(circlePosition, p) / speed;
    };

    bool isOnVelocity = false;
    bool isOnObstacle = false;

    std::optional<Vector2> aOpt = lineLineIntersection(
        circlePosition,
        goalPosition,
        lineStart,
        lineEnd,
        isOnVelocity,
        isOnObstacle);

    if(!aOpt.has_value())
        return std::nullopt;

    Vector2 a = aOpt.value();
    if(aOpt.has_value() && isOnObstacle)
    {
        // Already colliding
        if(isOnVelocity)
            return 0.0f;
        else
        {
            bool isOnLine = false;
            Vector2 b = closestPointLine(lineStart, lineEnd, goalPosition, isOnLine);
            if(isOnLine && Vector2Dot(Vector2DirectionTo(circlePosition, b), velocity) > 0.0f)
            {
                // Colliding with the obstacle "line"
                float ttc = GetLineTTC(a);
                if(ttc > 0.0f)
                    return ttc;
            }
        }
    }

    if(Vector2Dot(Vector2DirectionTo(circlePosition, lineStart), velocity) > 0.0f)
    {
        Vector2 c = closestPointLine(circlePosition, goalPosition, lineStart);
        if(Vector2Distance(c, lineStart) <= radius)
        {
            // Colliding with obstacle end-point
            float ttc = GetEndpointTTC(lineStart);
            if(ttc > 0.0f)
                return ttc;
        }
    }

    if(Vector2Dot(Vector2DirectionTo(circlePosition, lineEnd), velocity) > 0.0f)
    {
        Vector2 d = closestPointLine(circlePosition, goalPosition, lineEnd);
        if(Vector2Distance(d, lineEnd) <= radius)
        {
            // Colliding with obstacle end-point
            float ttc = GetEndpointTTC(lineEnd);
            if(ttc > 0.0f)
                return ttc;
        }
    }

    return std::nullopt;
}
namespace System
{
    void MoveEntities(
        entt::registry& registry,
        Navigation& navigation,
        float ksi,
        float avoidanceT,
        float obstacleT,
        float time)
    {
        static std::vector<std::string> entityNames;
        entityNames.clear();

        for(auto [entity, transform, moveTowards, velocityComponent, acceleration] :
            registry
                .view<
                    Component::Transform,
                    Component::MoveTowards,
                    Component::Velocity,
                    Component::Acceleration>()
                .each())
        {
            entityNames.push_back(std::to_string((int)entity));
            PROFILE_SCOPE(entityNames.back().c_str());

            const float radius = 0.30f;

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
            float forceScaleFactor = ksi / 2.0f;
            if(ksi < 2.0f)
                forceScaleFactor = 1.0f;

            forces = Vector2Add(forces, Vector2Scale({randomNumber(mt), randomNumber(mt)}, 0.5f));

            Vector2 separationForce = Vector2Zero();

            Profiling::ProfileCall("EntityAvoidance", [&]() {
                for(auto [otherEntity, otherTransform, otherHealth] :
                    registry.view<Component::Transform, Component::Health>().each())
                {
                    if(entity == otherEntity)
                        continue;

                    float distance = Vector3Distance(transform.position, otherTransform.position);

                    if(distance > 3.0f)
                        continue;

                    Vector2 otherVelocity = Vector2Zero();
                    if(Component::Velocity* oVel =
                           registry.try_get<Component::Velocity>(otherEntity);
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
                            Vector2Scale(
                                Vector2DirectionTo(otherPosition, position),
                                1.0f / distance));
                    }

                    std::optional<float> timeToCollision = TimeToCollisionSphere(
                        position,
                        otherPosition,
                        velocity,
                        otherVelocity,
                        radius);

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
            });

            forces.x += separationForce.x;
            forces.y += separationForce.y;

            Profiling::ProfileCall("ObstacleAvoidance", [&]() {
                Vector2 position = {.x = transform.position.x, .y = transform.position.z};
                float currentSpeed = Vector2Length(velocity);
                navigation.ForArea(
                    {
                        .x = transform.position.x - currentSpeed * obstacleT,
                        .y = transform.position.z - currentSpeed * obstacleT,
                    },
                    {
                        .x = transform.position.x + currentSpeed * obstacleT,
                        .y = transform.position.z + currentSpeed * obstacleT,
                    },
                    [&](const Navigation::Tile& tile, uint32_t x, uint32_t y) {
                        tile.ForEachWall([&](Navigation::Tile::Side side) {
                            Navigation::Wall wall = navigation.GetWall(x, y, side);

                            std::optional<float> timeToCollisionOpt = TimeToCollisionCircleLine(
                                position,
                                velocity,
                                radius,
                                wall.start,
                                wall.end);
                            if(!timeToCollisionOpt || *timeToCollisionOpt > obstacleT)
                                return;

                            float timeToCollision = timeToCollisionOpt.value();

                            Vector2 avoidanceForce = Vector2Scale(
                                wall.normal,
                                Vector2Dot(forces, wall.normal)
                                    / Vector2Dot(wall.normal, wall.normal));

                            if(Vector2Dot(avoidanceForce, wall.normal) < 0.0f)
                                avoidanceForce = Vector2Negate(avoidanceForce);

                            float magnitude = 0.0f;
                            if(timeToCollision >= 0.0f && timeToCollision < obstacleT)
                                magnitude =
                                    (obstacleT - timeToCollision) / (timeToCollision + 0.001f);

                            if(magnitude > 40.0f)
                                magnitude = 40.0f;

                            forces.x += avoidanceForce.x * magnitude;
                            forces.y += avoidanceForce.y * magnitude;
                        });
                    });
            });

            acceleration.acceleration.x += forces.x * time;
            acceleration.acceleration.z += forces.y * time;
        }
    }
}