#include "raylib.h"
#include <math.h>

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800
#define PLATFORM_COUNT 15

typedef struct Ball {
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool onGround;
    int jumpsLeft;
} Ball;

typedef struct Platform {
    Rectangle rect;
    float speed;
    int direction;
} Platform;

// Function to detect and resolve elastic collision between two balls
void ResolveBallCollision(Ball *b1, Ball *b2) {
    Vector2 diff = {b2->position.x - b1->position.x, b2->position.y - b1->position.y};
    float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
    float minDist = b1->radius + b2->radius;

    if (dist == 0.0f) return; // Prevent division by zero

    if (dist < minDist) {
        // Calculate overlap
        float overlap = minDist - dist;

        // Normalize the difference vector
        Vector2 norm = {diff.x / dist, diff.y / dist};

        // Push balls apart to prevent sticking
        b1->position.x -= norm.x * (overlap / 2);
        b1->position.y -= norm.y * (overlap / 2);
        b2->position.x += norm.x * (overlap / 2);
        b2->position.y += norm.y * (overlap / 2);

        // Relative velocity
        Vector2 relVel = {b2->velocity.x - b1->velocity.x, b2->velocity.y - b1->velocity.y};
        float velAlongNorm = relVel.x * norm.x + relVel.y * norm.y;

        if (velAlongNorm > 0) return; // Balls are moving apart

        // Elastic collision response (assuming equal mass)
        float bounce = 1.5f; // bounce factor >1 for higher speed bounce

        float impulse = -(1 + bounce) * velAlongNorm / 2; // divide by 2 because equal mass

        Vector2 impulseVec = {impulse * norm.x, impulse * norm.y};

        b1->velocity.x -= impulseVec.x;
        b1->velocity.y -= impulseVec.y;
        b2->velocity.x += impulseVec.x;
        b2->velocity.y += impulseVec.y;
    }
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Two Balls with Collision Bounce");
    SetTargetFPS(60);

    Ball ball1 = {
        .position = {100, SCREEN_HEIGHT - 60},
        .velocity = {0, 0},
        .radius = 15,
        .onGround = false,
        .jumpsLeft = 2
    };

    Ball ball2 = {
        .position = {200, SCREEN_HEIGHT - 60},
        .velocity = {0, 0},
        .radius = 15,
        .onGround = false,
        .jumpsLeft = 2
    };

    const float gravity = 0.5f;
    const float moveAccel = 0.5f;  // Reduced acceleration for smoother slower control
    const float maxSpeed = 3.0f;   // Reduced max speed
    const float jumpStrength = -10.0f;
    const float trampolineBoostX = 6.0f;

    Rectangle ground = {0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40};

    Platform platforms[PLATFORM_COUNT];
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        float x = GetRandomValue(50, SCREEN_WIDTH - 150);
        float y = SCREEN_HEIGHT - 100 - i * 50;

        float width = GetRandomValue(150, 300);  // varying lengths
        platforms[i].rect = (Rectangle){x, y, width, 20};
        platforms[i].speed = 1.5f;
        platforms[i].direction = (i % 2 == 0) ? 1 : -1;
    }

    while (!WindowShouldClose()) {
        // === INPUT for ball1 (arrow keys) ===
        if (IsKeyDown(KEY_LEFT)) {
            ball1.velocity.x -= moveAccel;
            if (ball1.velocity.x < -maxSpeed) ball1.velocity.x = -maxSpeed;
        }
        else if (IsKeyDown(KEY_RIGHT)) {
            ball1.velocity.x += moveAccel;
            if (ball1.velocity.x > maxSpeed) ball1.velocity.x = maxSpeed;
        }
        else {
            ball1.velocity.x *= 0.8f; // friction
            if (fabsf(ball1.velocity.x) < 0.1f) ball1.velocity.x = 0;
        }
        if (IsKeyPressed(KEY_UP) && ball1.jumpsLeft > 0) {
            ball1.velocity.y = jumpStrength;
            ball1.jumpsLeft--;
        }

        // === INPUT for ball2 (WASD) ===
        if (IsKeyDown(KEY_A)) {
            ball2.velocity.x -= moveAccel;
            if (ball2.velocity.x < -maxSpeed) ball2.velocity.x = -maxSpeed;
        }
        else if (IsKeyDown(KEY_D)) {
            ball2.velocity.x += moveAccel;
            if (ball2.velocity.x > maxSpeed) ball2.velocity.x = maxSpeed;
        }
        else {
            ball2.velocity.x *= 0.8f; // friction
            if (fabsf(ball2.velocity.x) < 0.1f) ball2.velocity.x = 0;
        }
        if (IsKeyPressed(KEY_W) && ball2.jumpsLeft > 0) {
            ball2.velocity.y = jumpStrength;
            ball2.jumpsLeft--;
        }

        // === Apply gravity ===
        ball1.velocity.y += gravity;
        ball1.onGround = false;
        ball2.velocity.y += gravity;
        ball2.onGround = false;

        // === Update platforms position ===
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            platforms[i].rect.x += platforms[i].speed * platforms[i].direction;
            if (platforms[i].rect.x < 0 || platforms[i].rect.x + platforms[i].rect.width > SCREEN_WIDTH)
                platforms[i].direction *= -1;
        }

        // --- Update and collide ball1 vertical ---
        ball1.position.y += ball1.velocity.y;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            Rectangle r = platforms[i].rect;
            float left = ball1.position.x - ball1.radius;
            float right = ball1.position.x + ball1.radius;
            float top = ball1.position.y - ball1.radius;
            float bottom = ball1.position.y + ball1.radius;

            if (right > r.x && left < r.x + r.width) {
                if (bottom >= r.y && top < r.y && ball1.velocity.y >= 0) {
                    ball1.position.y = r.y - ball1.radius;
                    ball1.velocity.y = 0;
                    ball1.onGround = true;
                    ball1.jumpsLeft = 2;
                }
                else if (top <= r.y + r.height && bottom > r.y + r.height && ball1.velocity.y < 0) {
                    ball1.position.y = r.y + r.height + ball1.radius;
                    ball1.velocity.y = 0;
                }
            }
        }
        if (ball1.position.y + ball1.radius >= ground.y) {
            ball1.position.y = ground.y - ball1.radius;
            ball1.velocity.y = 0;
            ball1.onGround = true;
            ball1.jumpsLeft = 2;
        }

        // --- Update and collide ball2 vertical ---
        ball2.position.y += ball2.velocity.y;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            Rectangle r = platforms[i].rect;
            float left = ball2.position.x - ball2.radius;
            float right = ball2.position.x + ball2.radius;
            float top = ball2.position.y - ball2.radius;
            float bottom = ball2.position.y + ball2.radius;

            if (right > r.x && left < r.x + r.width) {
                if (bottom >= r.y && top < r.y && ball2.velocity.y >= 0) {
                    ball2.position.y = r.y - ball2.radius;
                    ball2.velocity.y = 0;
                    ball2.onGround = true;
                    ball2.jumpsLeft = 2;
                }
                else if (top <= r.y + r.height && bottom > r.y + r.height && ball2.velocity.y < 0) {
                    ball2.position.y = r.y + r.height + ball2.radius;
                    ball2.velocity.y = 0;
                }
            }
        }
        if (ball2.position.y + ball2.radius >= ground.y) {
            ball2.position.y = ground.y - ball2.radius;
            ball2.velocity.y = 0;
            ball2.onGround = true;
            ball2.jumpsLeft = 2;
        }

        // --- Update and collide ball1 horizontal ---
        ball1.position.x += ball1.velocity.x;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            Rectangle r = platforms[i].rect;
            float left = ball1.position.x - ball1.radius;
            float right = ball1.position.x + ball1.radius;
            float top = ball1.position.y - ball1.radius;
            float bottom = ball1.position.y + ball1.radius;

            if (bottom > r.y && top < r.y + r.height) {
                if (right >= r.x && left < r.x && ball1.velocity.x > 0) {
                    ball1.position.x = r.x - ball1.radius;
                    ball1.velocity.x = -trampolineBoostX;
                }
                else if (left <= r.x + r.width && right > r.x + r.width && ball1.velocity.x < 0) {
                    ball1.position.x = r.x + r.width + ball1.radius;
                    ball1.velocity.x = trampolineBoostX;
                }
            }
        }
        if (ball1.position.x - ball1.radius <= 0) {
            ball1.position.x = ball1.radius;
            ball1.velocity.x = trampolineBoostX;
        }
        if (ball1.position.x + ball1.radius >= SCREEN_WIDTH) {
            ball1.position.x = SCREEN_WIDTH - ball1.radius;
            ball1.velocity.x = -trampolineBoostX;
        }

        // --- Update and collide ball2 horizontal ---
        ball2.position.x += ball2.velocity.x;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            Rectangle r = platforms[i].rect;
            float left = ball2.position.x - ball2.radius;
            float right = ball2.position.x + ball2.radius;
            float top = ball2.position.y - ball2.radius;
            float bottom = ball2.position.y + ball2.radius;

            if (bottom > r.y && top < r.y + r.height) {
                if (right >= r.x && left < r.x && ball2.velocity.x > 0) {
                    ball2.position.x = r.x - ball2.radius;
                    ball2.velocity.x = -trampolineBoostX;
                }
                else if (left <= r.x + r.width && right > r.x + r.width && ball2.velocity.x < 0) {
                    ball2.position.x = r.x + r.width + ball2.radius;
                    ball2.velocity.x = trampolineBoostX;
                }
            }
        }
        if (ball2.position.x - ball2.radius <= 0) {
            ball2.position.x = ball2.radius;
            ball2.velocity.x = trampolineBoostX;
        }
        if (ball2.position.x + ball2.radius >= SCREEN_WIDTH) {
            ball2.position.x = SCREEN_WIDTH - ball2.radius;
            ball2.velocity.x = -trampolineBoostX;
        }

        // --- Ball to ball collision ---
        ResolveBallCollision(&ball1, &ball2);

        // === DRAW ===
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangleRounded(ground, 1.0f, 20, DARKGRAY);

        for (int i = 0; i < PLATFORM_COUNT; i++) {
            DrawRectangleRounded(platforms[i].rect, 1.0f, 20, SKYBLUE);
        }

        DrawCircleV(ball1.position, ball1.radius, RED);
        DrawCircleV(ball2.position, ball2.radius, BLUE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
