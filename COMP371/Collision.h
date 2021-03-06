#pragma once

enum Collision { NoCollision, Colliding, CollidingNotY, CollidingXZ, NoCollisionUpOne };

enum CollisionMode { AABB, Sphere };

static const float32 GRAVITY = -9.81f;

