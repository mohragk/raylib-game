#include <iostream>
#include "vendor/raylib/include/raylib.h"

#include "HandMadeMath.h"
#include "defines.h"

// @ROBUSTNESS: does not check for normalized t!
f32 lerp(f32 a, f32 b, f32 t) {
    return ((1.0f - t) * a) + (t * b);
}




struct Enemy {
    u16 id;
    Vector3 position {0, 0, 0};
    Vector3 direction = {0, 0, 0};
    f32 speed = {8};
    
    f32 health {100.0};
    f32 damage {3.0f};

    f32 width {1.1};
    f32 height {1.8};
};

struct Player {
    Vector3 position {0, 0, 0};
    Vector3 aim {1, 0, 0};
    f32 speed {30.5};
    bool pulling_trigger {false};

    f32 health {100.0f};
    
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

struct Boundary {
    Vector3 center;
    Vector3 dimensions;
};

#define ENEMY_BUCKET_CAPACITY 16
struct EnemyBucket {
    u32 enemies[ENEMY_BUCKET_CAPACITY];
    u32 count{ 0 };
};

struct QuadTree {

    QuadTree(Boundary boundary) {
        this->boundary = boundary;
    }

    ~QuadTree() {
        if (is_subdivided) {
            delete nw;
            delete sw;
            delete ne;
            delete se;
        }
    }

    Boundary boundary;
    EnemyBucket bucket;

    QuadTree *nw, *ne, *sw, *se;

    bool is_subdivided {false};

    EnemyBucket getBucket(Vector3 location) {
        return bucket;
    }

    void insert( Enemy *enemy ) {
        if (!inside(enemy->position)) return;

        if ( ++bucket.count >= ENEMY_BUCKET_CAPACITY  ) {
            bucket.count = ENEMY_BUCKET_CAPACITY;

            is_subdivided = true;
            Boundary sub_boundary = {};

            {
                sub_boundary.center.x = boundary.center.x + boundary.dimensions.x/2;
                sub_boundary.center.y = boundary.center.y - boundary.dimensions.y/2; 
                ne = new QuadTree(sub_boundary);
            }
            {
                sub_boundary.center.x = boundary.center.x - boundary.dimensions.x/2;
                sub_boundary.center.y = boundary.center.y - boundary.dimensions.y/2; 
                nw = new QuadTree(sub_boundary);
            }
            {
                sub_boundary.center.x = boundary.center.x - boundary.dimensions.x/2;
                sub_boundary.center.y = boundary.center.y + boundary.dimensions.y/2; 
                se = new QuadTree(sub_boundary);
            }
            {
                sub_boundary.center.x = boundary.center.x + boundary.dimensions.x/2;
                sub_boundary.center.y = boundary.center.y + boundary.dimensions.y/2; 
                sw = new QuadTree(sub_boundary);
            }

            ne->insert(enemy);
            nw->insert(enemy);
            se->insert(enemy);
            sw->insert(enemy);

        }
        else {
            bucket.enemies[bucket.count] = enemy->id;
        }
    }

    bool inside(Vector3 pos) {
        return (pos.x >= boundary.center.x - boundary.dimensions.x || pos.x < boundary.center.x + boundary.dimensions.x ) 
        || (pos.y >= boundary.center.y - boundary.dimensions.y || pos.y < boundary.center.y + boundary.dimensions.y);
    }

    bool subdivide() {
        is_subdivided = true;
        return true;
    }

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
    i8 button_a;
    i8 button_b;
    i8 button_x;
    i8 button_y;

    i8 button_start;
    i8 button_back;

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



void UpdateAndRender(Game *game, Camera *camera, GameInput input, f32 dt, f32 window_width, f32 window_height) {
    Player *player = &game->player;

    player->position.y = player->size / 2.f;
    player->position.x += input.axis_left.x * player->speed * dt;
    player->position.z += input.axis_left.y * player->speed * dt;

    game->gun.barrel_exit = player->position;

    camera->target   = player->position;
    camera->position = { player->position.x, player->position.y + 75, player->position.z + 30 };

    hmm_v3 aim_axis = { input.axis_right.x, 0, input.axis_right.y };
    f32 l = HMM_LengthVec3(aim_axis);
    if (l > 0.06) {
        aim_axis.X = aim_axis.X / l;
        aim_axis.Z = aim_axis.Z / l;
    }
    player->aim = {aim_axis.X, aim_axis.Y, aim_axis.Z};

    // Bullets logic
    i32 max_bullets = game->max_bullets;
    bool want_to_fire_gun  = player->pulling_trigger || input.trigger_right > 0.7;
    if (want_to_fire_gun) {
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

    auto getDistanceIgnoreY = [] (Vector3 v1, Vector3 v2) {
        f32 x = v1.x - v2.x;
        f32 z = v1.z - v2.z;

        f32 x2 = x * x;
        f32 z2 = z * z;

        return sqrtf(x2 + z2);
    };



    // CREATE ENEMY QuadTree 
    Boundary boundary;
    boundary.center =  {player->position.x, player->position.y, 0};
    boundary.dimensions = {1000.0, 1000.0, 0.0};
    QuadTree *qt = new QuadTree(boundary);
    {
        for (u32 i = 0; i < (u32)game->enemy_count; i++) {
            Enemy *enemy = &game->enemies[i];
            qt->insert(enemy);
        }
    }


    

    // Draw the "GAME screen"
    {
        // Background
        DrawRectangle(0,0, window_width, window_height, DARKGRAY);

        BeginMode3D(*camera);


        
        // Update, draw and possibly remove bullets
        {
            if (game->bullet_count) {
                
                for (i32 i = 0; i < game->bullet_count; i++) {
                    Bullet *bullet = &game->bullets[i];
                    bullet->position.x = bullet->position.x + (bullet->speed * bullet->direction.x) * dt;
                    bullet->position.z = bullet->position.z + (bullet->speed * bullet->direction.z) * dt;
                    bullet->age++;

                    Vector3 pos = bullet->position;
                    bool remove = false;


                    // Check all enemies for hit!
                    for (i32 i = 0; i < game->enemy_count; i++) {
                        Enemy *enemy = &game->enemies[i];
                        Vector3 enemy_pos = enemy->position;

                        
                        f32 distance = getDistanceIgnoreY(pos, enemy_pos);
                        if (distance < bullet->size + enemy->width) {
                            enemy->health -= bullet->damage;
                            //remove = true;
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
        // Update, draw and possibly remove enemies
        {
            if (game->enemy_count) {
                
                for (i32 i = game->enemy_count - 1; i >= 0; i--) {
                    bool remove = false;
                    Enemy *enemy = &game->enemies[i];

                    enemy->position.x += (enemy->speed * enemy->direction.x) * dt;
                    enemy->position.z += (enemy->speed * enemy->direction.z) * dt;

                    // Check player contact
                    {
                        Vector3 player_pos = player->position;
                        Vector3 enemy_pos = enemy->position;
                        f32 dist = getDistanceIgnoreY(player_pos, enemy_pos);
                        if (dist < player->size/2 + enemy->width/2) {
                            player->health -= enemy->damage;
                            enemy->direction.x *= -1.0f;
                            enemy->direction.y *= -1.0f;
                        }
                    }
                   
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
        f32 a = 1.0f;
        Color faded_blue = ColorAlpha(BLUE, a);
        DrawCube(pos, player->size, player->size, player->size, faded_blue);
        

        DrawGrid(300, 10);
        EndMode3D();

        delete qt;
    }
}



int main() {
    u16 window_width = 1280;
    u16 window_height = 860; 
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT );  
    InitWindow(window_width, window_height, "Basic screen management");
    //SetTargetFPS(60);

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
        enemy->id = i;
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

            // Cursor keys, when down, fire gun
            {
                i32 x_dir = 0;
                i32 y_dir = 0;
                bool should_fire = false;
                if (IsKeyDown(KEY_UP)) {
                    y_dir -= 1;    
                    should_fire = true;
                }
                if (IsKeyDown(KEY_DOWN)) {
                    y_dir += 1;
                    should_fire = true;
                }
                if (IsKeyDown(KEY_LEFT)) {
                    x_dir -= 1;
                    should_fire = true;
                }
                if (IsKeyDown(KEY_RIGHT)) {
                    x_dir += 1;
                    should_fire = true;
                }

                game->player.pulling_trigger = should_fire;
                
                input.axis_right.x = GetGamepadAxisMovement(game_pad_num, GAMEPAD_AXIS_RIGHT_X);
                input.axis_right.x += x_dir;
                input.axis_right.y = GetGamepadAxisMovement(game_pad_num, GAMEPAD_AXIS_RIGHT_Y);
                input.axis_right.y += y_dir;

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

                case LOGO:
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

                    f32 dt = GetFrameTime();
                    UpdateAndRender(game, &camera, input, dt, window_width, window_height);
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
