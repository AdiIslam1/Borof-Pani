#include <raylib.h>
#include <math.h>

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800
#define PLATFORM_COUNT 10

const float gravity = 0.5f;
const float moveAccel = 0.5f;  // Reduced acceleration for smoother slower control
const float maxSpeed = 3.0f;   // Reduced max speed
const float jumpStrength = -10.0f;
const float trampolineBoostX = 6.0f;

typedef struct Ball {
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool onGround;
    int jumpsLeft;
} Ball;

// Struct for the players

typedef struct Sprite{
    Texture2D texture;
    Rectangle dest_rect;
    Vector2 vel;
    int dir;
    int jumpsLeft;
    bool onGround;
    int point;
}Sprite;

typedef struct Platform {
    Rectangle rect;
    float speed;
    int direction;
} Platform;



// Function to detect and resolve elastic collision between two balls
// void ResolveBallCollision(Ball *b1, Ball *b2) {
//     Vector2 diff = {b2->position.x - b1->position.x, b2->position.y - b1->position.y};
//     float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
//     float minDist = b1->radius + b2->radius;

//     if (dist == 0.0f) return; // Prevent division by zero

//     if (dist < minDist) {
//         // Calculate overlap
//         float overlap = minDist - dist;

//         // Normalize the difference vector
//         Vector2 norm = {diff.x / dist, diff.y / dist};

//         // Push balls apart to prevent sticking
//         b1->position.x -= norm.x * (overlap / 2);
//         b1->position.y -= norm.y * (overlap / 2);
//         b2->position.x += norm.x * (overlap / 2);
//         b2->position.y += norm.y * (overlap / 2);

//         // Relative velocity
//         Vector2 relVel = {b2->velocity.x - b1->velocity.x, b2->velocity.y - b1->velocity.y};
//         float velAlongNorm = relVel.x * norm.x + relVel.y * norm.y;

//         if (velAlongNorm > 0) return; // Balls are moving apart

//         // Elastic collision response (assuming equal mass)
//         float bounce = 1.5f; // bounce factor >1 for higher speed bounce

//         float impulse = -(1 + bounce) * velAlongNorm / 2; // divide by 2 because equal mass

//         Vector2 impulseVec = {impulse * norm.x, impulse * norm.y};

//         b1->velocity.x -= impulseVec.x;
//         b1->velocity.y -= impulseVec.y;
//         b2->velocity.x += impulseVec.x;
//         b2->velocity.y += impulseVec.y;
//     }
// }

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Two Balls with Collision Bounce");
    SetTargetFPS(60);

    Texture2D player_idle_texture =
            LoadTexture("assets/heros/herochar_idle_anim.gif");
    Texture2D new_player = 
            LoadTexture("assets/idle_right_down.png");
    Texture2D player_run_texture = 
            LoadTexture("assets/heros/herochar_run_anim.gif");
    Texture2D player_attack = 
            LoadTexture("/home/adi/Desktop/Borof-Pani/Borof/assets/heros/herochar_sword_attack_anim.gif");
    // Ball ball1 = {
    //     .position = {100, SCREEN_HEIGHT - 60},
    //     .velocity = {0, 0},
    //     .radius = 15,
    //     .onGround = false,
    //     .jumpsLeft = 2
    // };

    // Ball ball2 = {
    //     .position = {200, SCREEN_HEIGHT - 60},
    //     .velocity = {0, 0},
    //     .radius = 15,
    //     .onGround = false,
    //     .jumpsLeft = 2
    // };
    Sprite p1 = (Sprite){
        .texture = player_idle_texture,
        .dest_rect = (Rectangle){
            .x = 10.0,
            .y = 100.0,
            .width = 50.0,
            .height = 50.0,
        },
        .dir = 1,
        .vel = (Vector2){0,0},
        .point = 0
    };
    Sprite p2 = (Sprite){
        .texture = player_idle_texture,
        .dest_rect = (Rectangle){
            .x = 200.0,
            .y = 100.0,
            .width = 50.0,
            .height = 50.0,
        },
        .dir = -1,
        .vel = (Vector2){0,0},
        .point = 0
    };

    

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
        // === INPUT for p1 (arrow keys) ===
        if (IsKeyDown(KEY_LEFT)) {
            p1.vel.x -= moveAccel;
            p1.dir = -1;
            if (p1.vel.x < -maxSpeed) p1.vel.x = -maxSpeed;
        }
        else if (IsKeyDown(KEY_RIGHT)) {
            p1.vel.x += moveAccel;
            if (p1.vel.x > maxSpeed) p1.vel.x = maxSpeed;
            p1.dir = 1;
        }
        else {
            p1.vel.x *= 0.8f; // friction
            if (fabsf(p1.vel.x) < 0.1f) p1.vel.x = 0;
        }
        if (IsKeyPressed(KEY_UP) && p1.jumpsLeft > 0) {
            p1.vel.y = jumpStrength;
            p1.jumpsLeft--;
        }

        // === INPUT for p2 (WASD) ===
        if (IsKeyDown(KEY_A)) {
            p2.vel.x -= moveAccel;
            p2.dir = -1;
            if (p2.vel.x < -maxSpeed) p2.vel.x = -maxSpeed;
        }
        else if (IsKeyDown(KEY_D)) {
            p2.vel.x += moveAccel;
            p2.dir = 1;
            if (p2.vel.x > maxSpeed) p2.vel.x = maxSpeed;
        }
        else {
            p2.vel.x *= 0.8f; // friction
            if (fabsf(p2.vel.x) < 0.1f) p2.vel.x = 0;
        }
        if (IsKeyPressed(KEY_W) && p2.jumpsLeft > 0) {
            p2.vel.y = jumpStrength;
            p2.jumpsLeft--;
        }
        //=== Apply gravity ===

        p1.vel.y += gravity;
        p1.onGround = false;
        p2.vel.y += gravity;
        p2.onGround = false;
        //=== Update platforms position ===
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            platforms[i].rect.x += platforms[i].speed * platforms[i].direction;
            if (platforms[i].rect.x < 0 || platforms[i].rect.x + platforms[i].rect.width > SCREEN_WIDTH)
                platforms[i].direction *= -1;
        }

        // --- Update and collide p1 vertical ---
        p1.dest_rect.y += p1.vel.y;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            Rectangle r = platforms[i].rect;
            float left = p1.dest_rect.x - p1.dest_rect.width;
            float right = p1.dest_rect.x + p1.dest_rect.width;
            float top = p1.dest_rect.y - p1.dest_rect.height;
            float bottom = p1.dest_rect.y + p1.dest_rect.height;

            if (right > r.x && left < r.x + r.width) {
                if (bottom >= r.y && top < r.y && p1.vel.y >= 0) {
                    p1.dest_rect.y = r.y - p1.dest_rect.height;
                    p1.vel.y = 0;
                    p1.onGround = true;
                    p1.jumpsLeft = 2;
                }
                else if (top <= r.y + r.height && bottom > r.y + r.height && p1.vel.y < 0) {
                    p1.dest_rect.y = r.y + r.height + p1.dest_rect.height;
                    p1.vel.y = 0;
                }
            }
        }
        if (p1.dest_rect.y + p1.dest_rect.height >= ground.y) {
            p1.dest_rect.y = ground.y - p1.dest_rect.height;
            p1.vel.y = 0;
            p1.onGround = true;
            p1.jumpsLeft = 2;
        }

        // --- Update and collide p2 vertical ---
        p2.dest_rect.y += p2.vel.y;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            Rectangle r = platforms[i].rect;
            float left = p2.dest_rect.x - p2.dest_rect.width;
            float right = p2.dest_rect.x + p2.dest_rect.width;
            float top = p2.dest_rect.y - p2.dest_rect.height;
            float bottom = p2.dest_rect.y + p2.dest_rect.height;

            if (right > r.x && left < r.x + r.width) {
                if (bottom >= r.y && top < r.y && p2.vel.y >= 0) {
                    p2.dest_rect.y = r.y - p2.dest_rect.height;
                    p2.vel.y = 0;
                    p2.onGround = true;
                    p2.jumpsLeft = 2;
                }
                else if (top <= r.y + r.height && bottom > r.y + r.height && p2.vel.y < 0) {
                    p2.dest_rect.y = r.y + r.height + p2.dest_rect.height;
                    p2.vel.y = 0;
                }
            }
        }
        if (p2.dest_rect.y + p2.dest_rect.height >= ground.y) {
            p2.dest_rect.y = ground.y - p2.dest_rect.height;
            p2.vel.y = 0;
            p2.onGround = true;
            p2.jumpsLeft = 2;
        }

        bool f = CheckCollisionRecs(p1.dest_rect,p2.dest_rect);
        if(f){
            DrawText("Player 1 wins the round", 300, SCREEN_HEIGHT/2, 100, RED);
            p1.dest_rect.x = 0;
            p2.dest_rect.x = 1000;
        }

        // --- Update and collide p1 horizontal ---
        p1.dest_rect.x += p1.vel.x;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            Rectangle r = platforms[i].rect;
            float left = p1.dest_rect.x - p1.dest_rect.width;
            float right = p1.dest_rect.x + p1.dest_rect.width;
            float top = p1.dest_rect.y - p1.dest_rect.height;
            float bottom = p1.dest_rect.y + p1.dest_rect.height;

            if (bottom > r.y && top < r.y + r.height) {
                if (right >= r.x && left < r.x && p1.vel.x > 0) {
                    p1.dest_rect.x = r.x - p1.dest_rect.width;
                    p1.vel.x = -trampolineBoostX;
                }
                else if (left <= r.x + r.width && right > r.x + r.width && p1.vel.x < 0) {
                    p1.dest_rect.x = r.x + r.width + p1.dest_rect.width;
                    p1.vel.x = trampolineBoostX;
                }
            }
        }
        if (p1.dest_rect.x - p1.dest_rect.width <= 0) {
            p1.dest_rect.x = p1.dest_rect.width;
            p1.vel.x = trampolineBoostX;
        }
        if (p1.dest_rect.x + p1.dest_rect.width >= SCREEN_WIDTH) {
            p1.dest_rect.x = SCREEN_WIDTH - p1.dest_rect.width;
            p1.vel.x = -trampolineBoostX;
        }

        // --- Update and collide p2 horizontal ---
        p2.dest_rect.x += p2.vel.x;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            Rectangle r = platforms[i].rect;
            float left = p2.dest_rect.x - p2.dest_rect.width;
            float right = p2.dest_rect.x + p2.dest_rect.width;
            float top = p2.dest_rect.y - p2.dest_rect.height;
            float bottom = p2.dest_rect.y + p2.dest_rect.height;

            if (bottom > r.y && top < r.y + r.height) {
                if (right >= r.x && left < r.x && p2.vel.x > 0) {
                    p2.dest_rect.x = r.x - p2.dest_rect.width;
                    p2.vel.x = -trampolineBoostX;
                }
                else if (left <= r.x + r.width && right > r.x + r.width && p2.vel.x < 0) {
                    p2.dest_rect.x = r.x + r.width + p2.dest_rect.width;
                    p2.vel.x = trampolineBoostX;
                }
            }
        }
        if (p2.dest_rect.x - p2.dest_rect.width <= 0) {
            p2.dest_rect.x = p2.dest_rect.width;
            p2.vel.x = trampolineBoostX;
        }
        if (p2.dest_rect.x + p2.dest_rect.width >= SCREEN_WIDTH) {
            p2.dest_rect.x = SCREEN_WIDTH - p2.dest_rect.width;
            p2.vel.x = -trampolineBoostX;
        }



        // --- Ball to ball collision ---
        //ResolveBallCollision(&ball1, &ball2);

        // === DRAW ===
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangleRounded(ground, 1.0f, 20, DARKGRAY);

        for (int i = 0; i < PLATFORM_COUNT; i++) {
            DrawRectangleRounded(platforms[i].rect, 1.0f, 20, SKYBLUE);
        }
        //DrawTexturePro(p1.texture,(Rectangle){0,0, 16*(float)p1.dir, p1.dest_rect, , float rotation, Color tint);
        if(IsKeyPressed(KEY_F)) p1.texture = player_attack;
        else if(p1.vel.x!=0 || p1.vel.y!=0) p1.texture = player_run_texture;
        else p1.texture = player_idle_texture;
        if(p2.vel.x!=0 || p2.vel.y!=0) p2.texture = player_run_texture;
        else p2.texture = player_idle_texture;
        //p1.texture = new_player;
        DrawTexturePro(p1.texture, (Rectangle){0,0, 16*(float)p1.dir,16}, p1.dest_rect, 
                                (Vector2){0,0}, 0.0, RAYWHITE);
        DrawTexturePro(p2.texture, (Rectangle){0,0, 16*(float)p2.dir,16}, p2.dest_rect, 
                                (Vector2){0,0}, 0.0, GREEN);
        // DrawCircleV(ball1.position, ball1.radius, RED);
        // DrawCircleV(ball2.position, ball2.radius, BLUE);
 
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
