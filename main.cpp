#include <iostream>
#include "raylib.h"

#include "HandMadeMath.h"
#include "defines.h"

// @ROBUSTNESS: does not check for normalized t!
f32 lerp(f32 a, f32 b, f32 t) {
    return ((1.0f - t) * a) + (t * b);
}


struct Enemy {
    Vector3 position {0, 0, 0};
    Vector3 direction = {0, 0, 0};
    f32 speed = {8};
    f32 health {100.0};
    f32 width {1.1};
    f32 height {1.8};
};

struct Player {
    Vector3 position  {0, 0, 0};
    Vector3 aim = {1, 0, 0};
    f32 speed {30.5};
    
    f32 size {2};
};

struct Bullet {
    Vector3 position {0,0,0};
    Vector3 direction{0,0,0};
    f32 speed{60};
    u32 age{0};
    f32 damage {100.0};

    f32 size {0.7};
};


struct Gun {
    Vector3 barrel_exit {0, 0, 0};
    u32 shot_duration {8}; // frames
    u32 current_time {0};

    bool trigger_down {false};
};

struct Game {
    Player player;

    Gun gun;

    static constexpr i32 max_bullets{256};
    Bullet bullets[max_bullets];
    i32 bullet_count {0};
    

    static constexpr i32 max_enemies = 1024;
    Enemy enemies[ max_enemies ];
    i32 enemy_count {0};
};


typedef enum GameScreen {
    LOGO = 0, 
    TITLE, 
    GAMEPLAY, 
    ENDING
} game_screen;

struct GameInput {
    int button_a;
    int button_b;
    int button_x;
    int button_y;

    int button_start;
    int button_back;

    struct AxisLeft {
        f32 x;
        f32 y;
    } axis_left;

    struct AxisRight {
        f32 x;
        f32 y;
    } axis_right;

    f32 trigger_left;
    f32 trigger_right;
};

int main() {
    u16 window_width = 1280;
    u16 window_height = 860; 
    SetConfigFlags(FLAG_MSAA_4X_HINT);  
    InitWindow(window_width, window_height, "Basic screen management");
    SetTargetFPS(60);

    Game *game = new Game();

    // Camera
    Camera3D camera = {};
    camera.position = {0.0f, 300.0f, 100.0f};
    camera.target = { 0,0,0 }; // Center of scene
    camera.up = { 0.0, 1.0, 0.0 };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;


    // Initialize enemies randomly

    i32 enemy_count = game->max_enemies;
    game->enemy_count = enemy_count;
    for (i16 i = 0; i < enemy_count; i++) {
        Enemy *enemy = &game->enemies[i];
        enemy->position = {
            (f32)GetRandomValue(-100, 100), 
            1,
            (f32)GetRandomValue(-100, 100), 
        };
        f32 dir_x = f32(GetRandomValue(-100, 100)) / 100.0f;
        f32 dir_z = f32(GetRandomValue(-100, 100)) / 100.0f;
        hmm_v2 norm = HMM_NormalizeVec2( HMM_Vec2(dir_x, dir_z) );
        enemy->direction = { norm.X, 0.0, norm.Y };
    }
    

    GameScreen game_screen = TITLE;
    int frame_count = 0;

    // Font setup 
    Font menu_font = LoadFont("assets/Inconsolata-Light.ttf");
    SetTextureFilter(menu_font.texture, TEXTURE_FILTER_TRILINEAR);
    int font_size = 64;

    while (!WindowShouldClose()) {
        
        // Handle input
        GameInput input = {};
        {
            int game_pad_num = 0;
            if (IsKeyPressed(KEY_ENTER) || IsGamepadButtonPressed(game_pad_num, GAMEPAD_BUTTON_MIDDLE_RIGHT)) {
                input.button_start = 1;
            }
            
            if (IsKeyPressed(KEY_ENTER) || IsGamepadButtonPressed(game_pad_num, GAMEPAD_BUTTON_MIDDLE_LEFT)) {
                input.button_back = 1;
            }

            {
                i32 x_dir = 0;
                i32 y_dir = 0;
                if (IsKeyDown(KEY_W)) {
                    y_dir -= 1;    
                }
                if (IsKeyDown(KEY_S)) {
                    y_dir += 1;
                }
                if (IsKeyDown(KEY_A)) {
                    x_dir -= 1;
                }
                if (IsKeyDown(KEY_D)) {
                    x_dir += 1;
                }
                
                
                input.axis_left.x = GetGamepadAxisMovement(game_pad_num, GAMEPAD_AXIS_LEFT_X);
                input.axis_left.x += x_dir;

                input.axis_left.y = GetGamepadAxisMovement(game_pad_num, GAMEPAD_AXIS_LEFT_Y);
                input.axis_left.y += y_dir;
            }

            {
                i32 x_dir = 0;
                i32 y_dir = 0;
                if (IsKeyDown(KEY_UP)) {
                    y_dir -= 1;    
                }
                if (IsKeyDown(KEY_DOWN)) {
                    y_dir += 1;
                }
                if (IsKeyDown(KEY_LEFT)) {
                    x_dir -= 1;
                }
                if (IsKeyDown(KEY_RIGHT)) {
                    x_dir += 1;
                }
                
                
                input.axis_right.x = GetGamepadAxisMovement(game_pad_num, GAMEPAD_AXIS_RIGHT_X);
                input.axis_right.x += x_dir;
                input.axis_right.y = GetGamepadAxisMovement(game_pad_num, GAMEPAD_AXIS_RIGHT_Y);
                input.axis_right.y += y_dir;

                hmm_vec2 normalized = HMM_NormalizeVec2( HMM_Vec2(input.axis_left.x, input.axis_left.y) );
                //dinput.axis_left = {normalized.X, normalized.Y};
            }

            //input.trigger_left  = GetGamepadAxisMovement(game_pad_num, GAMEPAD_AXIS_LEFT_TRIGGER);
            
            input.trigger_right = GetGamepadAxisMovement(game_pad_num, GAMEPAD_AXIS_RIGHT_TRIGGER);
            input.trigger_right += IsKeyDown(KEY_Q) ? 2.0f : 0.0f;
            
        }


        {
            frame_count++;
            BeginDrawing();
            ClearBackground(BLACK);
            

            switch (game_screen) {
               
                case TITLE: {
                    if (input.button_start)
                        game_screen = GAMEPLAY;

                    // Draw the "title screen"
                    {
                        DrawRectangle(0,0, window_width, window_height, DARKBLUE);
                        DrawText("TITLE SCREEN - press START to begin", 20, 20, font_size/2, MAROON);
                    }
                }
                break;


                case GAMEPLAY: {
                    if (input.button_back)
                        game_screen = TITLE;

                   

                    Player *player = &game->player;

                    player->position.y = player->size / 2.f;
                    player->position.x += input.axis_left.x * player->speed * 0.0133;
                    player->position.z += input.axis_left.y * player->speed * 0.0133;

                    game->gun.barrel_exit = player->position;

                    camera.target   = player->position;
                    camera.position = { player->position.x, player->position.y + 75, player->position.z + 30 };

                    hmm_v3 aim_axis = { input.axis_right.x, 0, input.axis_right.y };
                    f32 l = HMM_LengthVec3(aim_axis);
                    if (l > 0.06) {
                        aim_axis.X = aim_axis.X / l;
                        aim_axis.Z = aim_axis.Z / l;
                    }
                    game->player.aim = {aim_axis.X, aim_axis.Y, aim_axis.Z};
                    

                    // Draw the "GAME screen"
                    {
                        // Background
                        DrawRectangle(0,0, window_width, window_height, DARKGRAY);

                        BeginMode3D(camera);
                        
                        f32 w = game->player.size;
                        f32 h = w;
                        
                        

                        i32 max_bullets = game->max_bullets;
                        // Bullets logic
                        if (input.trigger_right > 0.7) {
                            Gun *gun = &game->gun;
                            if (!gun->trigger_down) {
                                gun->trigger_down = true;
                                gun->current_time = 0;
                            }
                            else {
                                gun->current_time++;
                                if (gun->current_time > gun->shot_duration) {
                                    gun->current_time = 0;
                                    if (game->bullet_count < max_bullets) {
                                        // Add bullet

                                        Bullet *bullet = &game->bullets[game->bullet_count];
                                        bullet->direction = game->player.aim;
                                        bullet->position = game->gun.barrel_exit;
                                        game->bullet_count = game->bullet_count + 1;     
                                    }
                                }
                            }
                        }   

                        
                        // Update, draw and possibly remove bullets
                        {
                            if (game->bullet_count) {
                                
                                for (i32 i = 0; i < game->bullet_count; i++) {
                                    Bullet *bullet = &game->bullets[i];
                                    bullet->position.x = bullet->position.x + (bullet->speed * bullet->direction.x) * 0.0133;
                                    bullet->position.z = bullet->position.z + (bullet->speed * bullet->direction.z) * 0.0133;
                                    bullet->age++;

                                    Vector3 pos = bullet->position;
                                    bool remove = false;


                                    // Check all enemies for hit!
                                    for (i32 i = 0; i < game->enemy_count; i++) {
                                        Enemy *enemy = &game->enemies[i];
                                        Vector3 enemy_pos = enemy->position;

                                        auto getDistance = [] (Vector3 v1, Vector3 v2) {
                                            f32 x = v1.x - v2.x;
                                            f32 y = v1.y - v2.y;
                                            f32 z = v1.z - v2.z;

                                            f32 x2 = x * x;
                                            f32 y2 = y * y;
                                            f32 z2 = z * z;

                                            return sqrtf(x2 + y2 + z2);
                                        };
                                        f32 distance = getDistance(pos, enemy_pos);
                                        if (distance < bullet->size + enemy->width) {
                                            enemy->health -= bullet->damage;
                                            remove = true;
                                        }
                                    }
                                    
                                    DrawSphere(bullet->position, bullet->size, YELLOW);



                                    if (bullet->age >= 120) { // frames 
                                        remove = true;
                                    }

                                    if (remove) {
                                        // Swap the last one to this one, reduce count
                                        i32 index = game->bullet_count - 1;
                                        if (index >= 0) {
                                            game->bullets[i] = game->bullets[index];
                                            Bullet b = {};
                                            game->bullets[index] = b;
                                            game->bullet_count--;
                                        }
                                    }
                                }
                            }
                        }
                        // Update, draw and possinly remove enemies
                        {
                            if (game->enemy_count) {
                                for (i32 i = game->enemy_count - 1; i >= 0; i--) {
                                    bool remove = false;
                                    Enemy *enemy = &game->enemies[i];

                                    enemy->position.x += (enemy->speed * enemy->direction.x) * (1/60.0f);
                                    enemy->position.z += (enemy->speed * enemy->direction.z) * (1/60.0f);

                                    // Check OOB
                                    
                                    if (enemy->health > 0) {
                                        DrawCube(enemy->position, enemy->width, enemy->height, enemy->width, RED);
                                    }
                                    else {
                                        remove = true;
                                    }

                                    if (remove) {
                                        i32 last_index = game->enemy_count - 1;
                                        if (last_index) {
                                            game->enemies[i] = game->enemies[last_index];
                                            Enemy e = {};
                                            game->enemies[last_index] = e;
                                            game->enemy_count--;
                                        }
                                    }
                                }
                            }
                        }


                        // Draw player
                        Player *player = &game->player;
                        Vector3 pos = player->position;
                        DrawCube(pos, player->size, player->size, player->size, BLUE);
                        

                        // Draw reticle/gun
                        {
                            
                            
                            game->gun.barrel_exit = pos;
                            //DrawRectangle(x, y, w_reticle, w_reticle, RED);
                        }

                        DrawGrid(300, 10);
                        EndMode3D();
                    } // GAME SCREEN

                }
                break;

                
                case ENDING: {
                    // Draw the "END screen"
                    {
                        DrawRectangle(0,0, window_width, window_height, BLACK);
                        f32 x = window_width * 0.5 - window_width * 0.25;
                        f32 y = window_height * 0.5;
                        DrawText("Credits: Sander", x, y, font_size, MAROON);
                    }
                }
                break;

            }

            DrawFPS(10, 10);
            EndDrawing();
        }
    }

    //CloseWindow();

    return 0;
}