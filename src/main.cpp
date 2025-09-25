// main.c - Borof-Pani (C, raylib)
// Build example (Linux):
// gcc main.c -o borofpani -lraylib -lm -lpthread -ldl -lrt -lX11

#include "raylib.h"
#include"raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define W 1900
#define H 1020
#define PLAT_COUNT 10
#define SPRITE_SCALE 3.0f  // Adjust this value to make sprites bigger or smaller
#define ROUND_SEC 25
#define MAX_ROUNDS 15
#define WALL_STICK_TIME 3.0f  // 3 seconds
#define WALL_STICK_DECAY 0.95f // Velocity decay while stuck to wall
#ifndef PI
#define PI 3.14159265358979323846f
#endif

Texture2D player1Sprite;
Texture2D player2Sprite;
typedef enum {SC_MENU, SC_SETTINGS, SC_GAME} Screen;

typedef struct Ball {
    Vector2 pos, vel;
    float r;  // Keep this for collision detection
    bool onGround;
    int jumps;
    // Add sprite-related fields
    float spriteWidth, spriteHeight;
    bool facingRight;  // For sprite flipping
    // for wall sticking
    bool stickingToWall;
    float wallStickTimer;
    int wallSide; // -1 for left wall, 1 for right wall, 0 for no wall
} Ball;

typedef struct Plat {
    Rectangle r;
    float sp;
    int dir;
} Plat;

typedef struct Settings {
    float vol;
    int map; // 0 or 1
    bool fullscreen;
} Settings;

typedef struct PowerUp {
    Vector2 pos;
    bool active;
    float radius;
    float nextSpawnTime; // in seconds
} PowerUp;

static void SaveSettings(const Settings *s) {
    FILE *f = fopen("settings.cfg","w");
    if (!f) return;
    fprintf(f,"volume=%f\nmap=%d\nfullscreen=%d\n", s->vol, s->map, s->fullscreen ? 1 : 0);
    fclose(f);
}

static void LoadSettings(Settings *s) {
    s->vol = 0.5f;
    s->map = 0;
    s->fullscreen = false;
    FILE *f = fopen("settings.cfg","r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "volume=", 7)==0) s->vol = (float)atof(line+7);
        else if (strncmp(line, "map=", 4)==0) s->map = atoi(line+4);
        else if (strncmp(line, "fullscreen=", 11)==0) s->fullscreen = atoi(line+11) ? true : false;
    }
    fclose(f);
}

static bool PointInRec(Vector2 p, Rectangle r) {
    return (p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height);
}

static void DrawRoundedRec(Rectangle r, float roundness, int seg, Color col) {
    DrawRectangleRounded(r, roundness, seg, col);
}

static void DrawIconSettings(float x, float y, float s, Color c) {
    DrawCircle(x + s*0.5f, y + s*0.5f, s*0.32f, c);
    for (int i=0;i<6;i++){
        float a = i * (PI*2.0f/6.0f);
        Vector2 p1 = { x + s*0.5f + cosf(a)*s*0.46f, y + s*0.5f + sinf(a)*s*0.46f };
        Vector2 p2 = { x + s*0.5f + cosf(a)*s*0.6f,  y + s*0.5f + sinf(a)*s*0.6f };
        DrawLineEx(p1, p2, 3.0f, c);
    }
}

static void DrawIconVolume(float x, float y, float s, float vol, Color c) {
    DrawRectangle(x, y + s*0.28f, s*0.32f, s*0.44f, c);
    DrawTriangle((Vector2){x + s*0.32f, y + s*0.12f}, (Vector2){x + s*0.32f, y + s*0.88f}, (Vector2){x + s*0.8f, y + s*0.5f}, c);
    if (vol > 0.01f) DrawCircle(x + s*0.9f, y + s*0.5f, s*0.08f * (vol*1.6f+0.2f), c);
}

static void DrawModernButton(Rectangle r, const char *label, bool hover, bool pressed) {
    Color bg = hover ? Fade(SKYBLUE, 0.95f) : Fade(SKYBLUE, 0.85f);
    if (pressed) bg = GREEN;
    DrawRoundedRec(r, 0.15f, 20, bg);
    DrawText(label, (int)(r.x + r.width*0.15f), (int)(r.y + r.height*0.28f), (int)(r.height*0.4f), WHITE);
}

static void DrawCard(Rectangle r, Color c) {
    DrawRoundedRec(r, 0.12f, 20, c);
    DrawRectangleLinesEx(r, 2.0f, Fade(BLACK, 0.12f));
}
/// WALL COLLISION
static void UpdateWallSticking(Ball *b, float dt) {
    // Update wall stick timer
    if (b->stickingToWall) {
        b->wallStickTimer -= dt;
        if (b->wallStickTimer <= 0.0f) {
            if(b->wallSide == -1) {
                b->pos.x++;
            }
            else if(b->wallSide == 1) {
                b->pos.x--;
            }
            b->stickingToWall = false;
            b->wallSide = 0;
        }
    }
}

static void HandleWallCollision(Ball *b) {
    bool hitWall = false;

    // Check left wall
    if (b->pos.x - b->r <= 0.0f) {
        b->pos.x = b->r;

        if (!b->stickingToWall) {
            // Start sticking to left wall
            b->stickingToWall = true;
            b->wallStickTimer = WALL_STICK_TIME;
            b->wallSide = -1;
            b->vel.x = 0.0f; // Stop horizontal movement
            b->vel.y *= WALL_STICK_DECAY; // Slow down vertical movement
        }
        hitWall = true;
    }
    // Check right wall
    else if (b->pos.x + b->r >= W) {
        b->pos.x = W - b->r;

        if (!b->stickingToWall) {
            // Start sticking to right wall
            b->stickingToWall = true;
            b->wallStickTimer = WALL_STICK_TIME;
            b->wallSide = 1;
            b->vel.x = 0.0f; // Stop horizontal movement
            b->vel.y *= WALL_STICK_DECAY; // Slow down vertical movement
        }
        hitWall = true;
    }

    // If not touching wall, stop sticking
    if (!hitWall && b->stickingToWall) {
        b->stickingToWall = false;
        b->wallSide = 0;
        b->wallStickTimer = 0.0f;
    }
}

static void ApplyWallStickingPhysics(Ball *b, float dt) {
    if (b->stickingToWall) {
        // Prevent horizontal movement away from wall
        if (b->wallSide == -1) { // Stuck to left wall
            if (b->vel.x < 0.0f) b->vel.x = 0.0f;
        } else if (b->wallSide == 1) { // Stuck to right wall
            if (b->vel.x > 0.0f) b->vel.x = 0.0f;
        }

        // Reduce gravity effect while stuck to wall
        b->vel.y *= 0.0f; // Gradual sliding down

        // Add slight friction
        if (fabsf(b->vel.y) < 0.5f) {
            b->vel.y = 0.0f;
        }
    }
}

static void InitMap(Plat pl[], int map) {
    // reduced speeds compared to previous version for nicer visuals
    if (1) {
        for (int i=0;i<PLAT_COUNT;i++) {
            float w = (float)GetRandomValue(700, 1000);
            float x = (float)GetRandomValue(0, W - (int)w);
            float y = H - 120 - i*85;
            pl[i].r = (Rectangle){x, y, w, 18};
            pl[i].sp = 0.6f; // reduced
            pl[i].dir = (i % 2 == 0) ? 1 : -1;
        }
    } else {
        for (int i=0;i<PLAT_COUNT;i++) {
            float w = 420 - i*22;
            if (w < 140) w = 140;
            float x = (i%2==0) ? 50 : W - 50 - w;
            float y = H - 140 - i*85;
            pl[i].r = (Rectangle){x,y,w,18};
            pl[i].sp = 0.5f + (i%3)*0.13f; // reduced
            pl[i].dir = (i % 2 == 0) ? 1 : -1;
        }
    }
}
PowerUp switchPU;
float powerupTimer;
// --- Insert after your PowerUp / switchPU globals ---
typedef struct SpeedUp {
    Vector2 pos;
    bool active;
    float radius;
    float nextSpawnTime;
    float timer;
} SpeedUp;

static SpeedUp speedUp;
static bool fastActive = false;
static int fastBall = 0;  // 1 for b1, 2 for b2
// --- end insert ---


static void ResetBalls(Ball *b1, Ball *b2, Plat pl[]) {
    int i1 = GetRandomValue(0, PLAT_COUNT-1);
    int i2 = GetRandomValue(0, PLAT_COUNT-1);
    b1->pos.x = pl[i1].r.x + GetRandomValue(1,pl[i1].r.width);
    b1->pos.y = pl[i1].r.y - 16;
    b1->vel = (Vector2){0,0};
    // Update collision radius to match scaled sprite
    b1->r = fminf(b1->spriteWidth * SPRITE_SCALE, b1->spriteHeight * SPRITE_SCALE) * 0.4f;

    b1->onGround = false; b1->jumps = 2;
    b2->pos.x = pl[i2].r.x + GetRandomValue(1,pl[i1].r.width);
    b2->pos.y = pl[i2].r.y - 16;
    b2->vel = (Vector2){0,0};
    b2->r = fminf(b2->spriteWidth * SPRITE_SCALE, b2->spriteHeight * SPRITE_SCALE) * 0.4f;
    b2->onGround = false; b2->jumps = 2;

    b1->spriteWidth = player1Sprite.width;
    b1->spriteHeight = player1Sprite.height;
    b1->facingRight = true;

    b2->spriteWidth = player2Sprite.width;
    b2->spriteHeight = player2Sprite.height;
    b2->facingRight = true;

    b1->stickingToWall = false;
    b1->wallStickTimer = 0.0f;
    b1->wallSide = 0;

    b2->stickingToWall = false;
    b2->wallStickTimer = 0.0f;
    b2->wallSide = 0;
    /*switchPU.active = false;
    powerupTimer = 0.0f;
    switchPU.nextSpawnTime = 5.0f + GetRandomValue(5, 10);*/

}

static void ResolveCollision(Ball *a, Ball *b) {
    Vector2 d = { b->pos.x - a->pos.x, b->pos.y - a->pos.y };
    float dist = sqrtf(d.x*d.x + d.y*d.y);
    float min = a->r + b->r;
    if (dist == 0.0f || dist >= min) return;
    float overlap = min - dist;
    Vector2 n = { d.x/dist, d.y/dist };
    a->pos.x -= n.x * (overlap*0.5f);
    a->pos.y -= n.y * (overlap*0.5f);
    b->pos.x += n.x * (overlap*0.5f);
    b->pos.y += n.y * (overlap*0.5f);
    Vector2 rv = { b->vel.x - a->vel.x, b->vel.y - a->vel.y };
    float along = rv.x * n.x + rv.y * n.y;
    if (along > 0.0f) return;
    float e = 1.4f;
    float j = -(1+e)*along / 2.0f;
    Vector2 imp = { j*n.x, j*n.y };
    a->vel.x -= imp.x; a->vel.y -= imp.y;
    b->vel.x += imp.x; b->vel.y += imp.y;
}

static void ClampPlatformInBounds(Plat *p) {
    if (p->r.x < 0.0f) p->r.x = 0.0f;
    if (p->r.x + p->r.width > (float)W) p->r.x = (float)W - p->r.width;
}

static void DrawMapPreview(Rectangle box, int map) {
    // Deterministic, static preview — no randomness, no movement — prevents jitter.
    // We'll draw PLAT_COUNT small platforms stacked vertically inside 'box'.
    float margin = 10.0f;
    float innerW = box.width - margin*2;
    float innerH = box.height - margin*2;
    for (int i = 0; i < PLAT_COUNT; ++i) {
        float t = (float)i / (PLAT_COUNT - 1);
        // widths vary so preview looks interesting
        float w = (map == 0) ? (innerW * (0.4f + 0.5f * (1.0f - t))) : (innerW * (0.8f - 0.06f * i));
        if (w < 20) w = 20;
        // alternate left/right offset
        float x = (i % 2 == 0) ? (box.x + margin) : (box.x + box.width - margin - w);
        float y = box.y + margin + i * (innerH / (float)PLAT_COUNT);
        Rectangle pr = { x, y, w, 6 };
        DrawRectangleRec(pr, Fade(DARKGRAY, 0.9f));
    }
}

int main(void) {
    Settings s;
    LoadSettings(&s);

    // load texture

    InitWindow(W, H, "Borof-Pani");
    InitAudioDevice();      //0000000000000000000000000000
    if (s.fullscreen) ToggleFullscreen();

    SetTargetFPS(60);
    SetMasterVolume(s.vol);

    player1Sprite = LoadTexture("assets/herochar_run_anim.gif");  // Your sprite file
    player2Sprite = LoadTexture("assets/herochar_run_anim.gif");  // Your sprite file

    Sound switching_sound = LoadSound("switching.wav");     //00000000000000000000000000
    Sound game_end_sound =  LoadSound("game_completion.wav");
    Sound falling_sound = LoadSound("abyss_falling sound_scream.wav");
    Sound selection_sound = LoadSound("selection_sound.wav");

    Music game_sound = LoadMusicStream("game_sound.wav");


    Screen sc = SC_MENU;
    Plat pl[PLAT_COUNT];
    Ball b1, b2;
    Rectangle ground = {0, H-40, W, 40};

    int score1 = 0, score2 = 0;
    bool p1Hunter = true;
    float timer = ROUND_SEC;
    int roundCnt = 0;
    bool ended = false;
    //powerup dec
    PowerUp switchPU = {0};
    switchPU.radius = 14.0f;
    switchPU.active = false;
    switchPU.nextSpawnTime = 5.0f + GetRandomValue(5, 10);  // initial spawn time

    //speedUp.active = false;
    speedUp.timer = 0.0f;
    speedUp.nextSpawnTime = 5.0f + GetRandomValue(5, 10);
    speedUp.radius = 14.0f;  // you can adjust radius

    fastActive = false;
    fastBall = 0;


    b1.stickingToWall = false;
    b2.stickingToWall = false;
    // UI element rects
    Rectangle startR = {W*0.5f - 160, 350, 320, 70};
    Rectangle settingsR = {W*0.5f - 160, 440, 320, 60};
    Rectangle quitR = {W*0.5f - 160, 520, 320, 60};
    Rectangle settingsCard = {W*0.1f, H*0.12f, W*0.8f, H*0.72f};

    Rectangle volBar = {settingsCard.x + 40, settingsCard.y + 120, settingsCard.width - 160, 26};
    Rectangle volKnob = {volBar.x + volBar.width * s.vol - 8, volBar.y - 8, 16, 42};
    Rectangle fullscreenBox = {volBar.x, volBar.y + 70, 28, 28};
    Rectangle map1Box = {volBar.x, volBar.y + 120, 220, 80};
    Rectangle map2Box = {volBar.x + 240, volBar.y + 120, 220, 80};
    Rectangle resetBox = {volBar.x, volBar.y + 230, 180, 50};
    Rectangle backBox = {settingsCard.x + settingsCard.width - 140, settingsCard.y + settingsCard.height - 70, 110, 44};

    InitMap(pl, s.map);
    ResetBalls(&b1, &b2, pl);
    PlayMusicStream(game_sound);

    while (!WindowShouldClose()) {
        Vector2 mp = GetMousePosition();
        bool ldown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        bool lpressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        if (sc == SC_MENU) {
            if (lpressed && PointInRec(mp, startR)) {
                InitMap(pl, s.map);
                ResetBalls(&b1, &b2, pl);
                timer = ROUND_SEC; roundCnt = 0; score1 = 0; score2 = 0; p1Hunter = true; ended = false;
                sc = SC_GAME;
                PlaySound(selection_sound);    //000000000000000000
            } else if (lpressed && PointInRec(mp, settingsR)) {
                sc = SC_SETTINGS;
                PlaySound(selection_sound);    //000000000000000000
            } else if (lpressed && PointInRec(mp, quitR)) {
                PlaySound(selection_sound);    //000000000000000000
                break;
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Borof-Pani", W*0.5f - 180, 60, 64, DARKPURPLE);
            DrawText("A 2-player platform tag game", W*0.5f - 220, 140, 24, GRAY);

            Rectangle titleCard = {W*0.1f, 200, W*0.8f, 120};
            DrawCard(titleCard, Fade(SKYBLUE, 0.08f));
            DrawText("Instructions", (int)(titleCard.x + 20), (int)(titleCard.y + 12), 28, BLACK);
            DrawText("- P1: Arrow keys to move, Up to jump", (int)(titleCard.x + 24), (int)(titleCard.y + 50), 20, DARKGRAY);
            DrawText("- P2: A/D to move, W to jump", (int)(titleCard.x + 24), (int)(titleCard.y + 76), 20, DARKGRAY);

            bool hovStart = PointInRec(mp, startR);
            bool hovSet = PointInRec(mp, settingsR);
            bool hovQuit = PointInRec(mp, quitR);
            DrawModernButton(startR, "Start Game", hovStart, false);
            DrawRoundedRec(settingsR, 0.12f, 20, hovSet? Fade(LIGHTGRAY,0.9f): Fade(LIGHTGRAY,0.8f));
            DrawIconSettings(settingsR.x + 18, settingsR.y + 10, 36, hovSet? BLACK: DARKGRAY);
            DrawText("Settings", (int)(settingsR.x + 70), (int)(settingsR.y + 18), 26, BLACK);

            DrawRoundedRec(quitR, 0.12f, 20, hovQuit? Fade(ORANGE,0.95f): Fade(ORANGE,0.85f));
            DrawText("Quit", (int)(quitR.x + 130), (int)(quitR.y + 18), 26, WHITE);

            DrawText("Selected Map Preview:", W*0.5f - 140, 620, 20, BLACK);
            Rectangle mpv = {W*0.5f + 140, 620, 240, 120};
            DrawCard(mpv, Fade(LIGHTGRAY,0.06f));
            // deterministic static preview (no jitter)
            DrawMapPreview(mpv, s.map);

            EndDrawing();
        } else if (sc == SC_SETTINGS) {
            if (ldown && PointInRec(mp, volBar)) {
                float rel = (mp.x - volBar.x) / volBar.width;
                if (rel < 0.0f) rel = 0.0f;
                if (rel > 1.0f) rel = 1.0f;
                s.vol = rel; SetMasterVolume(s.vol);
                PlaySound(selection_sound);      //00000000000000000000000000000
            }
            if (lpressed && PointInRec(mp, fullscreenBox)) {
                s.fullscreen = !s.fullscreen;
                PlaySound(selection_sound);      //00000000000000000000000000000
                ToggleFullscreen();
            }
            if (lpressed && PointInRec(mp, map1Box)) {s.map = 0; PlaySound(selection_sound);}      //00000000000000000000000000000
            if (lpressed && PointInRec(mp, map2Box)) {s.map = 1; PlaySound(selection_sound);}      //00000000000000000000000000000}
            if (lpressed && PointInRec(mp, resetBox)) { s.vol = 0.5f; s.map = 0; s.fullscreen = false; PlaySound(selection_sound); SetMasterVolume(s.vol); }  //00000000000000000
            if (lpressed && PointInRec(mp, backBox)) {PlaySound(selection_sound); SaveSettings(&s); sc = SC_MENU; }  //00000000000000000

            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawCard(settingsCard, Fade(SKYBLUE, 0.03f));
            DrawText("Settings", (int)settingsCard.x + 24, (int)settingsCard.y + 16, 34, BLACK);

            DrawText("Music Volume", (int)volBar.x, (int)volBar.y - 30, 20, BLACK);
            DrawRoundedRec(volBar, 0.12f, 12, Fade(LIGHTGRAY,0.3f));
            float knobx = volBar.x + volBar.width * s.vol;
            DrawRectangleRounded((Rectangle){volBar.x, volBar.y, volBar.width*s.vol, volBar.height}, 0.12f, 12, Fade(SKYBLUE,0.9f));
            volKnob.x = knobx - volKnob.width*0.5f;
            DrawRoundedRec(volKnob, 0.15f, 12, DARKGRAY);
            DrawIconVolume(volBar.x + volBar.width + 40, volBar.y - 8, 40, s.vol, BLACK);
            DrawText(TextFormat("%d%%", (int)(s.vol*100)), (int)(volBar.x + volBar.width + 92), (int)(volBar.y), 20, BLACK);

            DrawText("Fullscreen", (int)fullscreenBox.x + 40, (int)fullscreenBox.y - 4, 20, BLACK);
            DrawRoundedRec(fullscreenBox, 0.08f, 8, s.fullscreen ? Fade(GREEN,0.9f) : Fade(LIGHTGRAY,0.6f));
            if (s.fullscreen) DrawText("ON", (int)fullscreenBox.x + 6, (int)fullscreenBox.y, 18, WHITE);
            else DrawText("OFF", (int)fullscreenBox.x + 6, (int)fullscreenBox.y, 18, DARKGRAY);

            DrawText("Choose Map", (int)map1Box.x, (int)map1Box.y - 26, 20, BLACK);
            DrawCard(map1Box, s.map==0? Fade(LIME,0.12f): Fade(LIGHTGRAY,0.04f));
            DrawText("Map 1 (Random)", (int)map1Box.x + 12, (int)map1Box.y + 8, 18, BLACK);
            for (int i=0;i<3;i++) DrawRectangle(map1Box.x + 12 + i*48, map1Box.y + 42, 40, 6, DARKGRAY);

            DrawCard(map2Box, s.map==1? Fade(LIME,0.12f): Fade(LIGHTGRAY,0.04f));
            DrawText("Map 2 (Staggered)", (int)map2Box.x + 12, (int)map2Box.y + 8, 18, BLACK);
            for (int i=0;i<3;i++) DrawRectangle(map2Box.x + 12 + i*48, map2Box.y + 60 - i*8, 40, 6, DARKGRAY);

            DrawRoundedRec(resetBox, 0.12f, 12, Fade(ORANGE,0.9f));
            DrawText("Reset to defaults", (int)resetBox.x + 12, (int)resetBox.y + 12, 18, WHITE);

            DrawRoundedRec(backBox, 0.12f, 12, Fade(SKYBLUE,0.9f));
            DrawText("Back", (int)backBox.x + 26, (int)backBox.y + 10, 20, WHITE);

            EndDrawing();
        } else if (sc == SC_GAME) {
            float dt = GetFrameTime();
            UpdateMusicStream(game_sound);

            if (!ended) {
                timer -= dt;
                powerupTimer += dt;

                if (!switchPU.active && powerupTimer >= switchPU.nextSpawnTime) {
                    int i = GetRandomValue(0, PLAT_COUNT - 1);
                    switchPU.pos.x = pl[i].r.x + pl[i].r.width * 0.5f;
                    switchPU.pos.y = pl[i].r.y - 20.0f;
                    switchPU.active = true;

                    // 🆕 ADD THIS:
                    powerupTimer = 0.0f;
                    switchPU.nextSpawnTime = 5.0f + GetRandomValue(5, 10);
                }

                    // --- Insert speedUp spawn / collision logic ---
                speedUp.timer += dt;
                if (!speedUp.active && speedUp.timer >= speedUp.nextSpawnTime) {
                    int i = GetRandomValue(0, PLAT_COUNT - 1);
                    speedUp.pos.x = pl[i].r.x + pl[i].r.width * 0.5f;
                    speedUp.pos.y = pl[i].r.y - 20.0f;
                    speedUp.active = true;

                    speedUp.timer = 0.0f;
                    speedUp.nextSpawnTime = 5.0f + GetRandomValue(5, 10);
                }

                if (speedUp.active) {
                    float d1 = Vector2Distance(b1.pos, speedUp.pos);
                    float d2 = Vector2Distance(b2.pos, speedUp.pos);
                    if (d1 < b1.r + speedUp.radius) {
                        fastActive = true;
                        fastBall = 1;
                        speedUp.active = false;
                        speedUp.timer = 0.0f;
                        speedUp.nextSpawnTime = 5.0f + GetRandomValue(5, 10);
                    } else if (d2 < b2.r + speedUp.radius) {
                        fastActive = true;
                        fastBall = 2;
                        speedUp.active = false;
                        speedUp.timer = 0.0f;
                        speedUp.nextSpawnTime = 5.0f + GetRandomValue(5, 10);
                    }
                }
                // --- end insert ---



                /*if (timer <= 0.0f) {
                    timer = ROUND_SEC; roundCnt++;
                    if (!p1Hunter) score1++; else score2++;
                    p1Hunter = !p1Hunter;
                    if (roundCnt >= MAX_ROUNDS || score1>7 || score2>7) ended = true;
                }*/
                if (timer <= 0.0f) {
                    timer = ROUND_SEC; roundCnt++;
                    if (!p1Hunter) score1++; else score2++;
                    p1Hunter = !p1Hunter;

                    switchPU.active = false;  // 🧼 clear power-up
                    powerupTimer = 0.0f;
                    switchPU.nextSpawnTime = 5.0f + GetRandomValue(5, 10);

                    speedUp.active = false;
                    fastActive = false;
                    fastBall = 0;

                    if (roundCnt >= MAX_ROUNDS || score1>7 || score2>7) {ended = true; PlaySound(game_end_sound);}
                    ResetBalls(&b1, &b2, pl);
                    PlaySound(switching_sound);             //0000000000000000000000000
                    }
                if (b1.pos.y - b1.r > H) {
                    score1++;
                    p1Hunter = !p1Hunter; timer = ROUND_SEC; roundCnt++;

                    // Clear power-up
                    switchPU.active = false;
                    powerupTimer = 0.0f;
                    switchPU.nextSpawnTime = 5.0f + GetRandomValue(5, 10);

                    speedUp.active = false;
                    fastActive = false;
                    fastBall = 0;

                    PlaySound(falling_sound);  //00000000000000000000

                    if (roundCnt >= MAX_ROUNDS || score1 > 7 || score2 > 7) {ended = true; PlaySound(game_end_sound);}
                    ResetBalls(&b1, &b2, pl);
                }
                else if (b2.pos.y - b2.r > H) {
                    score2++; p1Hunter = !p1Hunter; timer = ROUND_SEC; roundCnt++;

                    // Clear power-up
                    switchPU.active = false;
                    powerupTimer = 0.0f;
                    switchPU.nextSpawnTime = 5.0f + GetRandomValue(5, 10);

                    speedUp.active = false;
                    fastActive = false;
                    fastBall = 0;

                    PlaySound(falling_sound);  //00000000000000000000

                    if (roundCnt >= MAX_ROUNDS || score1 > 7 || score2 > 7) {ended = true; PlaySound(game_end_sound);}
                    ResetBalls(&b1, &b2, pl);
                }

                /*if (CheckCollisionCircles(b1.pos, b1.r, b2.pos, b2.r)) {
                    if (p1Hunter) score1++; else score2++;
                    timer = ROUND_SEC; roundCnt++; p1Hunter = !p1Hunter;
                    if (roundCnt >= MAX_ROUNDS || score1>7 || score2>7) ended = true;
                    ResetBalls(&b1, &b2, pl);
                }*/
                if (CheckCollisionCircles(b1.pos, b1.r, b2.pos, b2.r)) {
                    PlaySound(switching_sound);
                    if (p1Hunter) score1++; else score2++;
                    timer = ROUND_SEC; roundCnt++; p1Hunter = !p1Hunter;

                    switchPU.active = false;
                    powerupTimer = 0.0f;
                    switchPU.nextSpawnTime = 5.0f + GetRandomValue(5, 10);

                    speedUp.active = false;
                    fastActive = false;
                    fastBall = 0;

                    if (roundCnt >= MAX_ROUNDS || score1>7 || score2>7) {ended = true; PlaySound(game_end_sound);}
                    ResetBalls(&b1, &b2, pl);
                }

                /*if (IsKeyDown(KEY_LEFT)) {
                     b1.vel.x -= 0.6f;
                     if (b1.vel.x < -4.0f) b1.vel.x = -4.0f; \
                     b1.facingRight = false;
                }
                else if (IsKeyDown(KEY_RIGHT)) {
                    b1.vel.x += 0.6f;
                    if (b1.vel.x > 4.0f) b1.vel.x = 4.0f;
                    b1.facingRight = true;
                }
                else { b1.vel.x *= 0.8f; if (fabsf(b1.vel.x) < 0.1f) b1.vel.x = 0.0f; }
                if (IsKeyPressed(KEY_UP) && b1.jumps>0) {
                    b1.vel.y = -12.0f; b1.jumps--;
                }*/
                // --- Modified input for B1 with speedUp effect ---
                float accel1;
                if (fastActive && fastBall == 1) accel1 = 0.5f;
                else accel1=0.5f;

                if (IsKeyDown(KEY_LEFT)) {
                    b1.vel.x -= accel1;
                    if (b1.vel.x < -6.0f && fastBall!=1) b1.vel.x = -6.0f;
                    else if(b1.vel.x < -8.0f && fastBall==1) b1.vel.x = -8.0f;
                    b1.facingRight = false;
                }
                else if (IsKeyDown(KEY_RIGHT)) {
                    b1.vel.x += accel1;
                    if (b1.vel.x > 6.0f && fastBall!=1) b1.vel.x = 6.0f;
                    else if(b1.vel.x > 8.0f && fastBall==1) b1.vel.x = 8.0f;
                    b1.facingRight = true;
                }
                else {
                    b1.vel.x *= 0.8f;
                    if (fabsf(b1.vel.x) < 0.1f) b1.vel.x = 0.0f;
                }
                if (IsKeyPressed(KEY_UP) && b1.jumps > 0) {
                    b1.vel.y = -12.0f;
                    b1.jumps--;
                }
                // --- end replace ---


                /*if (IsKeyDown(KEY_A)) {
                    b2.vel.x -= 0.6f;
                    if (b2.vel.x < -4.0f) b2.vel.x = -4.0f;
                    b2.facingRight = false;
                }
                else if (IsKeyDown(KEY_D)) {
                    b2.vel.x += 0.6f; if (b2.vel.x > 4.0f) b2.vel.x = 4.0f;
                    b2.facingRight = true;
                }
                else { b2.vel.x *= 0.8f; if (fabsf(b2.vel.x) < 0.1f) b2.vel.x = 0.0f; }
                if (IsKeyPressed(KEY_W) && b2.jumps>0) { b2.vel.y = -12.0f; b2.jumps--; }*/
                // --- Modified input for B2 with speedUp effect ---
                float accel2;
                if (fastActive && fastBall == 2) accel2 = 0.5f;
                else accel2 = 0.5f;

                if (IsKeyDown(KEY_A)) {
                    b2.vel.x -= accel2;
                    if (b2.vel.x < -6.0f && fastBall!=2) b2.vel.x = -6.0f;
                    else if(b2.vel.x < -8.0f && fastBall==2) b2.vel.x=-8.0f;
                    b2.facingRight = false;
                }
                else if (IsKeyDown(KEY_D)) {
                    b2.vel.x += accel2;
                    if (b2.vel.x > 6.0f && fastBall!=2) b2.vel.x = 6.0f;
                    else if(b2.vel.x > 8.0f && fastBall==2) b2.vel.x=8.0f;
                    b2.facingRight = true;
                }
                else {
                    b2.vel.x *= 0.8f;
                    if (fabsf(b2.vel.x) < 0.1f) b2.vel.x = 0.0f;
                }
                if (IsKeyPressed(KEY_W) && b2.jumps > 0) {
                    b2.vel.y = -12.0f;
                    b2.jumps--;
                }
                // --- end replace ---


                float gravity = 0.5f;
                if(!b1.stickingToWall) b1.vel.y += gravity;
                if(!b2.stickingToWall) b2.vel.y += gravity;
                b1.onGround = b2.onGround = false;

                for (int i=0;i<PLAT_COUNT;i++) {
                    pl[i].r.x += pl[i].sp * pl[i].dir;
                    // flip direction and clamp to avoid overshoot
                    if (pl[i].r.x < 0.0f) {
                        pl[i].r.x = 0.0f;
                        pl[i].dir *= -1;
                    } else if (pl[i].r.x + pl[i].r.width > (float)W) {
                        pl[i].r.x = (float)W - pl[i].r.width;
                        pl[i].dir *= -1;
                    }
                }

                Ball *bals[2] = { &b1, &b2 };
                for (int bi=0;bi<2;bi++) {
                    Ball *bb = bals[bi];
                    bb->pos.y += bb->vel.y;
                    for (int i=0;i<PLAT_COUNT;i++) {
                        Rectangle rr = pl[i].r;
                        float l = bb->pos.x - bb->r;
                        float rgt = bb->pos.x + bb->r;
                        float t = bb->pos.y - bb->r;
                        float btm = bb->pos.y + bb->r;
                        if (rgt > rr.x && l < rr.x + rr.width) {
                            if (btm >= rr.y && t < rr.y && bb->vel.y >= 0.0f) {
                                bb->pos.y = rr.y - bb->r;
                                bb->vel.y = 0.0f;
                                bb->onGround = true;
                                bb->jumps = 2;
                            } else if (t <= rr.y + rr.height && btm > rr.y + rr.height && bb->vel.y < 0.0f) {
                                bb->pos.y = rr.y + rr.height + bb->r;
                                bb->vel.y = 0.0f;
                            }
                        }
                    }
                    bb->pos.x += bb->vel.x;
                    for (int i=0;i<PLAT_COUNT;i++) {
                        Rectangle rr = pl[i].r;
                        float l = bb->pos.x - bb->r;
                        float rgt = bb->pos.x + bb->r;
                        float t = bb->pos.y - bb->r;
                        float btm = bb->pos.y + bb->r;
                        if (btm > rr.y && t < rr.y + rr.height) {
                            if (rgt >= rr.x && l < rr.x && bb->vel.x > 0.0f) {
                                bb->pos.x = rr.x - bb->r;
                                bb->vel.x = 6.0f;
                            } else if (l <= rr.x + rr.width && rgt > rr.x + rr.width && bb->vel.x < 0.0f) {
                                bb->pos.x = rr.x + rr.width + bb->r;
                                bb->vel.x = -6.0f;
                            }
                        }
                    }
                    UpdateWallSticking(bb, dt);
                    ApplyWallStickingPhysics(bb, dt);
                    // if (bb->pos.x - bb->r <= 0.0f) { bb->pos.x = bb->r; bb->vel.x = 6.0f; }
                    // if (bb->pos.x + bb->r >= W) { bb->pos.x = W - bb->r; bb->vel.x = -6.0f; }
                    HandleWallCollision(bb);
                }
                if (switchPU.active) {
                    float d1 = Vector2Distance(b1.pos, switchPU.pos);
                    float d2 = Vector2Distance(b2.pos, switchPU.pos);

                    if (d1 < b1.r + switchPU.radius || d2 < b2.r + switchPU.radius) {
                        p1Hunter = !p1Hunter;  // Switch hunter
                        switchPU.active = false;
                        powerupTimer = 0.0f;
                        switchPU.nextSpawnTime = 5.0f + GetRandomValue(5, 10);  // consistent spawn timing
                    }

                }

               // ResolveCollision(&b1, &b2);
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawRectangleRec(ground, DARKGRAY);
            for (int i=0;i<PLAT_COUNT;i++) {
                DrawRectangleRounded(pl[i].r, 0.9f, 20, Fade(BLACK,0.12f));
            }
            // DrawCircleV(b1.pos, b1.r, RED);
            // DrawCircleV(b2.pos, b2.r, BLUE);
            // sprite drawing ::

            // With sprite drawing:
            Vector2 p1Origin = { (b1.spriteWidth * SPRITE_SCALE) * 0.5f, (b1.spriteHeight * SPRITE_SCALE) * 0.5f };
            Rectangle p1Source = { 0, 0, b1.facingRight ? b1.spriteWidth : -b1.spriteWidth, b1.spriteHeight };
            Rectangle p1Dest = { b1.pos.x, b1.pos.y, b1.spriteWidth * SPRITE_SCALE, b1.spriteHeight * SPRITE_SCALE };
            DrawTexturePro(player1Sprite, p1Source, p1Dest, p1Origin, 0.0f, WHITE);

            Vector2 p2Origin = { (b2.spriteWidth * SPRITE_SCALE) * 0.5f, (b2.spriteHeight * SPRITE_SCALE) * 0.5f };
            Rectangle p2Source = { 0, 0, b2.facingRight ? b2.spriteWidth : -b2.spriteWidth, b2.spriteHeight };
            Rectangle p2Dest = { b2.pos.x, b2.pos.y, b2.spriteWidth * SPRITE_SCALE, b2.spriteHeight * SPRITE_SCALE };
            DrawTexturePro(player2Sprite, p2Source, p2Dest, p2Origin, 0.0f, RED);
            if (switchPU.active) {
                DrawCircleV(switchPU.pos, switchPU.radius, Fade(ORANGE, 0.9f));
                DrawText("S", (int)(switchPU.pos.x - 6), (int)(switchPU.pos.y - 10), 20, WHITE);
            }

                        // --- Insert drawing for speedUp ---
            if (speedUp.active) {
                DrawCircleV(speedUp.pos, speedUp.radius, Fade(GOLD, 0.9f));
                DrawText("N", (int)(speedUp.pos.x - 6), (int)(speedUp.pos.y - 10), 20, WHITE);
            }
            // --- end insert ---


            if (b2.stickingToWall) {
                // Draw timer bar or effect for Player 1
                Rectangle timerBar = {10, 100, 200 * (b2.wallStickTimer / WALL_STICK_TIME), 8};
                DrawRectangleRec(timerBar, RED);
                DrawText("P1 WALL STUCK", 10, 110, 16, BLUE);
            }

            if (b1.stickingToWall) {
                // Draw timer bar or effect for Player 2
                Rectangle timerBar = {W - 210, 200, 200 * (b1.wallStickTimer / WALL_STICK_TIME), 8};
                DrawRectangleRec(timerBar, BLUE);
                DrawText("P2 WALL STUCK", W - 200, 110, 16, RED);
            }

            DrawText(TextFormat("%d", (int)ceilf(timer)), 10, 10, 60, BLACK);
            DrawText(TextFormat("P1 Score: %d", score1), W - 220, 40, 26, RED);
            DrawText(TextFormat("P2 Score: %d", score2), W - 220, 80, 26, BLUE);
            DrawText(TextFormat("Hunter: %s", p1Hunter ? "P1" : "P2"), W/2 - 80, 10, 36, p1Hunter ? RED : BLUE);

            if (ended) {
                DrawText("GAME OVER", W/2 - 140, H/2 - 60, 40, DARKPURPLE);
                if (score1 > score2) DrawText("P1 WINS!", W/2 - 80, H/2, 28, RED);
                else if (score2 > score1) DrawText("P2 WINS!", W/2 - 80, H/2, 28, BLUE);
                else DrawText("DRAW!", W/2 - 80, H/2, 28, GRAY);

                Rectangle bt = {W/2 - 100, H/2 + 60, 200, 54};
                DrawRoundedRec(bt, 0.12f, 12, Fade(GREEN, 0.9f));
                DrawText("Back to Menu", (int)bt.x + 28, (int)bt.y + 14, 20, WHITE);
                if (lpressed && PointInRec(mp, bt)) { SaveSettings(&s); sc = SC_MENU; }
            } else {
                PlaySound(game_end_sound);    //0000000000000
                Rectangle menuMini = {20, H-90, 240, 68};
                DrawRoundedRec(menuMini, 0.12f, 12, Fade(LIGHTGRAY, 0.06f));
                DrawText("Press BACKSPACE to return to menu", (int)menuMini.x + 16, (int)menuMini.y + 18, 18, DARKGRAY);
                if (IsKeyPressed(KEY_BACKSPACE)) { SaveSettings(&s); sc = SC_MENU; }
            }

            EndDrawing();
        }
    }
    UnloadSound(switching_sound);     //00000000000000000
    UnloadSound(game_end_sound);
    UnloadSound(falling_sound);
    UnloadSound(selection_sound);

    UnloadMusicStream(game_sound);

    UnloadTexture(player1Sprite);
    UnloadTexture(player2Sprite);
    SaveSettings(&s);
    CloseAudioDevice();        //0000000000000000000000000000
    CloseWindow();
    return 0;
}
