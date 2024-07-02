#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH (1280)
#define WINDOW_HEIGHT (720)
#define SPEED (300)
#define ENEMY_SPAWN_INTERVAL 2000 // in milliseconds
#define BULLET_SPEED (500)
#define ENEMY_BULLET_SPEED (400)
#define ENEMY_SHOOT_INTERVAL 1000 
#define MAX_HEALTH 100
#define HEALTH_BAR_WIDTH 200
#define HEALTH_BAR_HEIGHT 20
#define PLAYER_SHOOT_INTERVAL 200 
#define MAX_BULLETS_BEFORE_RELOAD 20
#define RELOAD_TIME 2000 

enum GameState {
    MENU,
    PLAYING,
    GAME_OVER,
    HIGH_SCORE_SHOW,
    HELP,
    OPTIONS,
    QUIT
};

enum GameState currentState = MENU;  // Start with the menu

int score=0;
int hscore=0;
bool soundOn = true;
bool musicOn = true;

typedef struct {
    SDL_Rect rect;
    SDL_Texture *texture;
} Enemy;

typedef struct {
    SDL_Rect rect;
    SDL_Texture *texture;
} Bullet;

void cleanup(/*SDL_Window *win, SDL_Renderer *rend,*/ SDL_Texture *tex, SDL_Texture *bg_tex, SDL_Texture *ej_tex, SDL_Texture *eg_tex, SDL_Texture *eC_tex, SDL_Texture *eD_tex, SDL_Texture *eE_tex, SDL_Texture *eF_tex, SDL_Texture *tex_up, SDL_Texture *tex_down, SDL_Texture *bullet_tex, SDL_Texture *enemy_bullet_tex, SDL_Texture *explosion_tex,SDL_Texture *reloading_tex) {
    if (tex) SDL_DestroyTexture(tex);
    if (bg_tex) SDL_DestroyTexture(bg_tex);
    if (ej_tex) SDL_DestroyTexture(ej_tex);
    if (eg_tex) SDL_DestroyTexture(eg_tex);
    if (eC_tex) SDL_DestroyTexture(eC_tex);
    if (eD_tex) SDL_DestroyTexture(eD_tex);
    if (eE_tex) SDL_DestroyTexture(eE_tex);
    if (eF_tex) SDL_DestroyTexture(eF_tex);
    if (tex_up) SDL_DestroyTexture(tex_up);
    if (tex_down) SDL_DestroyTexture(tex_down);
    if (bullet_tex) SDL_DestroyTexture(bullet_tex);
    if (enemy_bullet_tex) SDL_DestroyTexture(enemy_bullet_tex);
    if (explosion_tex) SDL_DestroyTexture(explosion_tex);
    if (reloading_tex) SDL_DestroyTexture(reloading_tex);
    //if (rend) SDL_DestroyRenderer(rend);
   // if (win) SDL_DestroyWindow(win);
   // TTF_Quit();
   // SDL_Quit();
}

const char* menuItems[] = {
    "Play",
    "Help",
    "High Scores",
    "Options",
    "Quit"
};
int menuItemCount = 5;
int selectedItem = 0;

void renderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedItem) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};
    
    for (int i = 0; i < menuItemCount; i++) {
        SDL_Surface* surface = TTF_RenderText_Solid(font, menuItems[i], (i == selectedItem) ? red : white);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect dstrect = { 50, 450 + i * 50, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &dstrect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }
}

void saveHighScore(int score) {
    FILE *file = fopen("scores/scores.txt", "w");
        fprintf(file, "%d", score);
        fclose(file);
   } 

int loadHighScore() {
    int hscore = 0;
    FILE *file = fopen("scores/scores.txt", "r");
        fscanf(file, "%d", &hscore);
        fclose(file);
    return hscore;
}


SDL_Texture* load_texture(SDL_Renderer *rend, const char *file) {
    SDL_Surface *surface = IMG_Load(file);
    if (!surface) {
        printf("IMG_Load error: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(rend, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        printf("SDL_CreateTextureFromSurface error: %s\n", SDL_GetError());
    }
    return texture;
}

int check_collision(SDL_Rect a, SDL_Rect b) {
    return !(a.x + a.w <= b.x || a.x >= b.x + b.w || a.y + a.h <= b.y || a.y >= b.y + b.h);
}

void render_health_bar(SDL_Renderer *rend, int health) {
    
    SDL_Rect health_bar = {10, WINDOW_HEIGHT - HEALTH_BAR_HEIGHT - 10, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
    SDL_Rect health_fill = {10, WINDOW_HEIGHT - HEALTH_BAR_HEIGHT - 10, (health * HEALTH_BAR_WIDTH) / MAX_HEALTH, HEALTH_BAR_HEIGHT};

    
    SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
    SDL_RenderFillRect(rend, &health_fill);

    
    SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
    SDL_RenderDrawRect(rend, &health_bar);

    
    TTF_Font *font = TTF_OpenFont("resources/arial.ttf", 24); 
    if (font == NULL) {
        SDL_Log("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }

    SDL_Color white = {255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, "HP", white);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(rend, surface);
    SDL_Rect text_rect = {10, WINDOW_HEIGHT - HEALTH_BAR_HEIGHT - 40, surface->w, surface->h};
    SDL_FreeSurface(surface);
    SDL_RenderCopy(rend, texture, NULL, &text_rect);
    SDL_DestroyTexture(texture);

    TTF_CloseFont(font);
}

void render_score(SDL_Renderer *renderer, int score) {
    char score_text[50];
    sprintf(score_text, "Score: %d", score);
    
    SDL_Color White = {255, 255, 255};
    TTF_Font* font = TTF_OpenFont("resources/arial.ttf", 24);
    if (!font) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        return;
    }
    
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, score_text, White); 
    SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage); 
    
    SDL_Rect Message_rect; 
    Message_rect.x = 10;  
    Message_rect.y = 10;  
    Message_rect.w = 100; 
    Message_rect.h = 50;  

    SDL_RenderCopy(renderer, Message, NULL, &Message_rect);

    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(Message);
    TTF_CloseFont(font);
}

void render_hscore(SDL_Renderer *renderer, int hscore) {
    char hscore_text[50];
    sprintf(hscore_text, "High Score: %d", hscore);
    
    SDL_Color White = {255, 255, 255};
    TTF_Font* font = TTF_OpenFont("resources/arial.ttf", 24);
    if (!font) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        return;
    }
    
    SDL_Surface* surfHScore = TTF_RenderText_Solid(font, hscore_text, White); 
    SDL_Texture* texHScore = SDL_CreateTextureFromSurface(renderer, surfHScore); 
    
    SDL_Rect Hscore_rect; 
    Hscore_rect.x = 10;  
    Hscore_rect.y = 60;  
    Hscore_rect.w = 120; 
    Hscore_rect.h = 50;  

    SDL_RenderCopy(renderer, texHScore, NULL, &Hscore_rect);

    SDL_FreeSurface(surfHScore);
    SDL_DestroyTexture(texHScore);
    TTF_CloseFont(font);
}



void highscorePage(SDL_Window* win, SDL_Renderer* rend, int hscore,TTF_Font* font) {
    
    hscore=loadHighScore();
    
    SDL_Texture* bgTexture = load_texture(rend,"resources/highscore.jpeg");
    
    SDL_RenderCopy(rend, bgTexture, NULL, NULL);
    SDL_DestroyTexture(bgTexture);

    char hscoreText[50];
    snprintf(hscoreText, sizeof(hscoreText), "High Score: %d", hscore);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, hscoreText, white);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(rend, textSurface);
    SDL_FreeSurface(textSurface);
    
    int textW = 0, textH = 0;
    SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);

    SDL_Rect textRect;
    textRect.x = (1280 - textW) / 2; 
    textRect.y = (720 - textH) / 2; 
    textRect.w = textW;
    textRect.h = textH;

    SDL_RenderCopy(rend, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);

    SDL_RenderPresent(rend);

}

void helpPage(SDL_Window* win, SDL_Renderer* rend){

    SDL_Texture* bgTexture = load_texture(rend,"resources/help.jpeg");
    SDL_Texture* textTexture = load_texture(rend,"resources/helpb.png");
    
    SDL_RenderCopy(rend, bgTexture, NULL, NULL);
    int textW = 0, textH = 0;
    SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);

    SDL_Rect textRect;
    textRect.x = (1280 - textW) / 2; 
    textRect.y = (720 - textH) / 2; 
    textRect.w = textW;
    textRect.h = textH;
    
    SDL_RenderCopy(rend, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);
    SDL_DestroyTexture(bgTexture);
    
    SDL_RenderPresent(rend);

}


void renderGameOver(SDL_Renderer *rend, TTF_Font *font) {
    
    SDL_Texture* bgTexture = load_texture(rend,"resources/gameover.jpeg");
    
    SDL_Color textColor = { 255, 255, 255 };

    char score_text[50];
    sprintf(score_text, "Your Score: %d", score);
   
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, score_text, textColor); 
    SDL_Texture* Message = SDL_CreateTextureFromSurface(rend, surfaceMessage); 
    
    
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, "Game Over", textColor);
    SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(rend, textSurface);
    
    
    SDL_Rect textRect;
    textRect.x = (WINDOW_WIDTH - textSurface->w) / 2; 
    textRect.y = (WINDOW_HEIGHT - textSurface->h) / 2;
    textRect.w = textSurface->w;
    textRect.h = textSurface->h;
    
    SDL_Rect Message_rect; 
    Message_rect.x = textRect.x;  
    Message_rect.y = textRect.y + 50;  
    Message_rect.w = textRect.w; 
    Message_rect.h = textRect.h;  
    
    SDL_RenderClear(rend);
    SDL_RenderCopy(rend, bgTexture, NULL, NULL);
    SDL_RenderCopy(rend, gameOverTexture, NULL, &textRect);
    SDL_RenderCopy(rend, Message, NULL, &Message_rect);
    
    
    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(gameOverTexture);
    SDL_FreeSurface(textSurface);
    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(Message);
}

void optionsdisplayfunc(SDL_Window* win,SDL_Renderer* rend,TTF_Font* font,bool soundOn,bool musicOn) {
    
    SDL_Texture* bgTexture = load_texture(rend,"resources/options.jpeg");
    SDL_RenderCopy(rend, bgTexture, NULL, NULL);
    SDL_DestroyTexture(bgTexture);

    SDL_Color color = {255, 255, 255, 255};

    char soundText[20];
    sprintf(soundText, "Sound: %s", soundOn ? "On" : "Off");

    char musicText[20];
    sprintf(musicText, "Music: %s", musicOn ? "On" : "Off");
    
    char instText[50];
    sprintf(instText, "Press s or m button to toggle sound or music");


    SDL_Surface* soundSurface = TTF_RenderText_Solid(font, soundText, color);
    SDL_Surface* musicSurface = TTF_RenderText_Solid(font, musicText, color);
    SDL_Surface* instSurface = TTF_RenderText_Solid(font, instText, color);

    SDL_Texture* soundTexture = SDL_CreateTextureFromSurface(rend, soundSurface);
    SDL_Texture* musicTexture = SDL_CreateTextureFromSurface(rend, musicSurface);
    SDL_Texture* instTexture = SDL_CreateTextureFromSurface(rend, instSurface);

    int soundWidth, soundHeight;
    SDL_QueryTexture(soundTexture, NULL, NULL, &soundWidth, &soundHeight);

    int musicWidth, musicHeight;
    SDL_QueryTexture(musicTexture, NULL, NULL, &musicWidth, &musicHeight);
    
    int instWidth, instHeight;
    SDL_QueryTexture(instTexture, NULL, NULL, &instWidth, &instHeight);

    int screenWidth = 1280; 
    int screenHeight = 720; 

    SDL_Rect soundRect = { (screenWidth - soundWidth) / 2, (screenHeight / 2) , soundWidth, soundHeight };
    SDL_Rect musicRect = { (screenWidth - musicWidth) / 2, (screenHeight / 2) + 50, musicWidth, musicHeight };
    SDL_Rect instRect = { (screenWidth - instWidth) / 2, (screenHeight / 2) + 100, instWidth, instHeight };

    SDL_RenderCopy(rend, soundTexture, NULL, &soundRect);
    SDL_RenderCopy(rend, musicTexture, NULL, &musicRect);
    SDL_RenderCopy(rend, instTexture, NULL, &instRect);

    SDL_DestroyTexture(soundTexture);
    SDL_DestroyTexture(musicTexture);
    SDL_DestroyTexture(instTexture);
    SDL_FreeSurface(soundSurface);
    SDL_FreeSurface(musicSurface);
    SDL_FreeSurface(instSurface);
    
    SDL_RenderPresent(rend);
}




void render_text(SDL_Renderer *rend, const char *text, int x, int y) {
    TTF_Font *font = TTF_OpenFont("resources/arial.ttf", 24);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    SDL_Color white = {255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, white);
    if (surface == NULL) {
        printf("Failed to create surface: %s\n", TTF_GetError());
        TTF_CloseFont(font);
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(rend, surface);
    if (texture == NULL) {
        printf("Failed to create texture: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        TTF_CloseFont(font);
        return;
    }

    SDL_Rect text_rect = {x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);

    if (SDL_RenderCopy(rend, texture, NULL, &text_rect) != 0) {
        printf("Failed to render texture: %s\n", SDL_GetError());
    }

    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
    }  
     
    Mix_Music* loadmusicfunc(const char *file){
        Mix_Music* music = Mix_LoadMUS(file);
        if (music == NULL) {
         printf("Failed to load music! SDL_mixer Error: %s\n", Mix_GetError());
         return NULL;
         }
        return music;
    }
    
    


int game(SDL_Window* win,SDL_Renderer* rend) {

    score = 0;
    
    // Load textures
    SDL_Texture *tex = load_texture(rend, "resources/playerplane.png");
    SDL_Texture *bg_tex = load_texture(rend, "resources/background.png");
    SDL_Texture *ej_tex = load_texture(rend, "resources/enemyplane_japan.png");
    SDL_Texture *eg_tex = load_texture(rend, "resources/enemyplane_german.png");
    SDL_Texture *eC_tex = load_texture(rend, "resources/enemyC.png");
    SDL_Texture *eD_tex = load_texture(rend, "resources/enemyD.png");
    SDL_Texture *eE_tex = load_texture(rend, "resources/enemyE.png");
    SDL_Texture *eF_tex = load_texture(rend, "resources/enemyF.png");
    SDL_Texture *tex_up = load_texture(rend, "resources/playerplane_up.png");
    SDL_Texture *tex_down = load_texture(rend, "resources/playerplane_down.png");
    SDL_Texture *bullet_tex = load_texture(rend, "resources/bullet.png");
    SDL_Texture *enemy_bullet_tex = load_texture(rend, "resources/enemy_bullet.png");
    SDL_Texture *explosion_tex = load_texture(rend, "resources/explosion.png");
    SDL_Texture *reloading_tex = load_texture(rend, "resources/reloading.png");

    if (!tex || !bg_tex || !ej_tex || !eg_tex || !eC_tex || !eD_tex || !eE_tex || !eF_tex || !tex_up || !tex_down || !bullet_tex || !enemy_bullet_tex || !explosion_tex || !reloading_tex) {
        cleanup(tex, bg_tex, ej_tex, eg_tex, eC_tex, eD_tex, eE_tex, eF_tex, tex_up, tex_down, bullet_tex, enemy_bullet_tex, explosion_tex, reloading_tex);
        return 1;
    }
    
    Mix_Chunk* sColl1 = Mix_LoadWAV("resources/sounds/collision1.wav");
    Mix_Chunk* sColl2 = Mix_LoadWAV("resources/sounds/collision2.wav");
    Mix_Chunk* sColl3 = Mix_LoadWAV("resources/sounds/collision3.wav");
    Mix_Chunk* sShoot = Mix_LoadWAV("resources/sounds/shoot.wav");
     
     Mix_Chunk* scolls[]={sColl1,sColl2,sColl3};
     int random_scoll_index = 0;
     
     srand(time(NULL));
      
    //Mix_Chunk* sound = Mix_LoadWAV(".wav");
    //Mix_Chunk* sound = Mix_LoadWAV(".wav");
   

    
    SDL_Rect dest;
    SDL_QueryTexture(tex, NULL, NULL, &dest.w, &dest.h);
    dest.w = 192;
    dest.h = 65;

    SDL_Rect dest_up, dest_down;
    SDL_QueryTexture(tex_up, NULL, NULL, &dest_up.w, &dest_up.h);
    float aspect_ratio_up = (float)dest_up.w / dest_up.h;
    dest_up.w = 192;
    dest_up.h = (int)(192 / aspect_ratio_up);

    SDL_QueryTexture(tex_down, NULL, NULL, &dest_down.w, &dest_down.h);
    float aspect_ratio_down = (float)dest_down.w / dest_down.h;
    dest_down.w = 192;
    dest_down.h = (int)(192 / aspect_ratio_down);

    SDL_Rect explosion_rect;
    SDL_QueryTexture(explosion_tex, NULL, NULL, &explosion_rect.w, &explosion_rect.h);
    explosion_rect.w = 170;
    explosion_rect.h = 170;
    int show_explosion = 0;
    Uint32 explosion_time = 0;

   
    float x_pos = 30;
    float y_pos = (WINDOW_HEIGHT - dest.h) / 2;
    float x_vel = 0;
    float y_vel = 0;


    int up = 0, down = 0, left = 0, right = 0, shooting = 0, reloading = 0;
    int close_requested = 0;
    int player_health = MAX_HEALTH;

    // Enemy management
    int max_enemies = 10;
    Enemy enemies[max_enemies];
    int enemy_count = 0;
    Uint32 last_spawn_time = 0;

    // Bullet management
    int max_bullets = 100;
    Bullet bullets[max_bullets];
    int bullet_count = 0;
    Bullet enemy_bullets[max_bullets];
    int enemy_bullet_count = 0;

    Uint32 last_frame_time = SDL_GetTicks();
    Uint32 last_enemy_shoot_time = 0;
    Uint32 last_player_shoot_time = 0;
    int bullets_fired = 0;
    Uint32 reload_start_time = 0;

    while (!close_requested && player_health > 0) {
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_frame_time) / 1000.0f;
        last_frame_time = current_time;
    //take input
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                close_requested = 1;
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                int value = (event.type == SDL_KEYDOWN) ? 1 : 0;
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_W:
                    case SDL_SCANCODE_UP:
                        up = value;
                        break;
                    case SDL_SCANCODE_A:
                    case SDL_SCANCODE_LEFT:
                        left = value;
                        break;
                    case SDL_SCANCODE_S:
                    case SDL_SCANCODE_DOWN:
                        down = value;
                        break;
                    case SDL_SCANCODE_D:
                    case SDL_SCANCODE_RIGHT:
                        right = value;
                        break;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    shooting = 1;
                }
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    shooting = 0;
                }
            }
        }

        // Update positions
        x_vel = y_vel = 0;
        if (up && !down) y_vel = -SPEED;
        if (down && !up) y_vel = SPEED;
        if (left && !right) x_vel = -SPEED;
        if (right && !left) x_vel = SPEED;

        x_pos += x_vel * delta_time;
        y_pos += y_vel * delta_time;

        if (x_pos <= 0) x_pos = 0;
        if (y_pos <= 0) y_pos = 0;
        if (x_pos >= WINDOW_WIDTH - dest.w) x_pos = WINDOW_WIDTH - dest.w;
        if (y_pos >= WINDOW_HEIGHT - dest.h) y_pos = WINDOW_HEIGHT - dest.h;

        if (up && !down) {
            dest_up.y = (int)y_pos;
            dest_up.x = (int)x_pos;
        } else if (down && !up) {
            dest_down.y = (int)y_pos;
            dest_down.x = (int)x_pos;
        } else {
            dest.y = (int)y_pos;
            dest.x = (int)x_pos;
        }

        // Check for enemy spawning
        if (current_time > last_spawn_time + ENEMY_SPAWN_INTERVAL && enemy_count < max_enemies) {
            last_spawn_time = current_time;
            enemies[enemy_count].rect.x = WINDOW_WIDTH;
            enemies[enemy_count].rect.y = rand() % (WINDOW_HEIGHT - 250);
            enemies[enemy_count].rect.w = 192;
            enemies[enemy_count].rect.h = 65;

            SDL_Texture* enemy_textures[] = {ej_tex, eg_tex, eC_tex, eD_tex, eE_tex, eF_tex};
            int random_index = rand() % 6;
            enemies[enemy_count].texture = enemy_textures[random_index];

            enemy_count++;
        }

        // Update enemy positions
        for (int i = 0; i < enemy_count; i++) {
            enemies[i].rect.x -= SPEED * delta_time;

            // Enemy shooting
            if (current_time > last_enemy_shoot_time + ENEMY_SHOOT_INTERVAL && enemy_bullet_count < max_bullets) {
                last_enemy_shoot_time = current_time;
                enemy_bullets[enemy_bullet_count].rect.x = enemies[i].rect.x;
                enemy_bullets[enemy_bullet_count].rect.y = enemies[i].rect.y + (enemies[i].rect.h / 2);
                enemy_bullets[enemy_bullet_count].rect.w = 40;
                enemy_bullets[enemy_bullet_count].rect.h = 15;
                enemy_bullets[enemy_bullet_count].texture = enemy_bullet_tex;
                enemy_bullet_count++;
            }
        }

        // Remove off-screen enemies
        for (int i = 0; i < enemy_count; i++) {
            if (enemies[i].rect.x + enemies[i].rect.w < 0) {
                enemies[i] = enemies[enemy_count - 1];
                enemy_count--;
            }
        }

        // Player shooting with rate limit and reload
        if (shooting && bullet_count < max_bullets && !reloading) {
            if (current_time > last_player_shoot_time + PLAYER_SHOOT_INTERVAL && bullets_fired < MAX_BULLETS_BEFORE_RELOAD) {
                last_player_shoot_time = current_time;
                bullets[bullet_count].rect.x = (int)x_pos + dest.w;
                bullets[bullet_count].rect.y = (int)y_pos + (dest.h / 2);
                bullets[bullet_count].rect.w = 40;
                bullets[bullet_count].rect.h = 15;
                bullets[bullet_count].texture = bullet_tex;
                bullet_count++;
                bullets_fired++;
                Mix_PlayChannel(0, sShoot, 0);
            } else if (bullets_fired >= MAX_BULLETS_BEFORE_RELOAD) {
                reloading = 1;
                reload_start_time = current_time;
            }
        }

        if (reloading && current_time > reload_start_time + RELOAD_TIME) {
            reloading = 0;
            bullets_fired = 0;
        }

        // Update bullet positions
        for (int i = 0; i < bullet_count; i++) {
            bullets[i].rect.x += BULLET_SPEED * delta_time;
        }

        // Remove off-screen bullets
        for (int i = 0; i < bullet_count; i++) {
            if (bullets[i].rect.x > WINDOW_WIDTH) {
                bullets[i] = bullets[bullet_count - 1];
                bullet_count--;
            }
        }

        // Update enemy bullet positions
        for (int i = 0; i < enemy_bullet_count; i++) {
            enemy_bullets[i].rect.x -= ENEMY_BULLET_SPEED * delta_time;
        }

        // Remove off-screen enemy bullets
        for (int i = 0; i < enemy_bullet_count; i++) {
            if (enemy_bullets[i].rect.x < 0) {
                enemy_bullets[i] = enemy_bullets[enemy_bullet_count - 1];
                enemy_bullet_count--;
            }
        }

        // Check for collisions
        for (int i = 0; i < enemy_count; i++) {
            if (check_collision(dest, enemies[i].rect)) {
                score+=10;
                player_health -= 20;
                show_explosion = 1;
                explosion_rect.x = enemies[i].rect.x;
                explosion_rect.y = enemies[i].rect.y;
                explosion_time = current_time;
                enemies[i] = enemies[enemy_count - 1];
                enemy_count--;
                random_scoll_index = rand() % 3;
                Mix_PlayChannel(1,scolls[random_scoll_index], 0);
            }
        }

        for (int i = 0; i < bullet_count; i++) {
            for (int j = 0; j < enemy_count; j++) {
                if (check_collision(bullets[i].rect, enemies[j].rect)) {
                    score+=10;
                    show_explosion = 1;
                    explosion_rect.x = enemies[j].rect.x;
                    explosion_rect.y = enemies[j].rect.y;
                    explosion_time = current_time;
                    bullets[i] = bullets[bullet_count - 1];
                    bullet_count--;
                    enemies[j] = enemies[enemy_count - 1];
                    enemy_count--;
                    random_scoll_index = rand() % 3;
                Mix_PlayChannel(1,scolls[random_scoll_index], 0);
                }
            }
        }

        for (int i = 0; i < enemy_bullet_count; i++) {
            if (check_collision(dest, enemy_bullets[i].rect)) {
                player_health -= 10;
                enemy_bullets[i] = enemy_bullets[enemy_bullet_count - 1];
                enemy_bullet_count--;
            }
        }
        
        hscore=loadHighScore();
        if (score > hscore) {
                hscore = score;
                saveHighScore(hscore);
            }
            
        //rendering 
        SDL_RenderClear(rend);
        SDL_RenderCopy(rend, bg_tex, NULL, NULL);

        if (up && !down) {
            SDL_RenderCopy(rend, tex_up, NULL, &dest_up);
        } else if (down && !up) {
            SDL_RenderCopy(rend, tex_down, NULL, &dest_down);
        } else {
            SDL_RenderCopy(rend, tex, NULL, &dest);
        }

        for (int i = 0; i < enemy_count; i++) {
            SDL_RenderCopy(rend, enemies[i].texture, NULL, &enemies[i].rect);
        }

        for (int i = 0; i < bullet_count; i++) {
            SDL_RenderCopy(rend, bullets[i].texture, NULL, &bullets[i].rect);
        }

        for (int i = 0; i < enemy_bullet_count; i++) {
            SDL_RenderCopy(rend, enemy_bullets[i].texture, NULL, &enemy_bullets[i].rect);
        }

        if (show_explosion && current_time - explosion_time < 500) {
            SDL_RenderCopy(rend, explosion_tex, NULL, &explosion_rect);
        } else {
            show_explosion = 0;
        }

        render_health_bar(rend, player_health);
        render_score(rend, score);
        render_hscore(rend, hscore);
        
         char bullet_count_text[32];
        sprintf(bullet_count_text, "Bullets: %d", MAX_BULLETS_BEFORE_RELOAD - bullets_fired);
        render_text(rend, bullet_count_text, WINDOW_WIDTH - 150, WINDOW_HEIGHT - 50);

        if (reloading) {
            SDL_Rect reload_icon_rect = {WINDOW_WIDTH - 180, WINDOW_HEIGHT - 100, 50, 50};
            SDL_RenderCopy(rend, reloading_tex, NULL, &reload_icon_rect);
            render_text(rend,"Reloading...", WINDOW_WIDTH - 130, WINDOW_HEIGHT - 90);
        }

        SDL_RenderPresent(rend);
        SDL_Delay(1000 / 60);
    }

    cleanup(tex, bg_tex, ej_tex, eg_tex, eC_tex, eD_tex, eE_tex, eF_tex, tex_up, tex_down, bullet_tex, enemy_bullet_tex, explosion_tex,reloading_tex);
    
    Mix_FreeChunk(sColl1);
    sColl1 = NULL;
    Mix_FreeChunk(sColl2);
    sColl2 = NULL;
    Mix_FreeChunk(sColl3);
    sColl3 = NULL;
    Mix_FreeChunk(sShoot);
    sShoot = NULL;
    
    currentState = GAME_OVER;
    return 0;
    }
    




int main(int argc, char* argv[]) {
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    
    if (TTF_Init() == -1) {
        SDL_Log("Unable to initialize SDL_ttf: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return 1;
    }

                Mix_Music *mus1 =loadmusicfunc("resources/music/HeroicDemise.mp3");
                Mix_Music *mus2 =loadmusicfunc("resources/music/DarkDescent.mp3");
                Mix_Music *mus3 =loadmusicfunc("resources/music/BattleTheme.mp3");
                Mix_Music *mus4 =loadmusicfunc("resources/music/HeroImmortal.mp3");
                Mix_Music *mus5 =loadmusicfunc("resources/music/russ.mp3");
                Mix_Music *mus6 =loadmusicfunc("resources/music/music1.mp3");
    
                Mix_Music* musics[] = {mus1,mus2,mus3,mus4,mus5,mus6};
                srand(time(NULL));
                int random_mus_index = rand() % 6;
                Mix_PlayMusic(musics[random_mus_index], 2);
    
    SDL_Window* win = SDL_CreateWindow("Flying Aces: 1942",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH , WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
                                          
       
       
                                          
    if (!win) {
        printf("error creating window: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!rend) {
        printf("error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 36);
    if (!font) {
        SDL_DestroyRenderer(rend);
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Texture *menu_tex = load_texture(rend, "resources/menu.jpeg");
    SDL_Rect menu_rect;
    SDL_QueryTexture(menu_tex, NULL, NULL, &menu_rect.w, &menu_rect.h);
    menu_rect.x=0;
    menu_rect.y=0;
    menu_rect.w = 1280;
    menu_rect.h = 720;
    
    SDL_Texture *menu_title = load_texture(rend, "resources/title.png");
    SDL_Rect title_rect;
    SDL_QueryTexture(menu_tex, NULL, NULL, &title_rect.w, &title_rect.h);
    title_rect.x=50;
    title_rect.y=70;
    title_rect.w = 600;
    title_rect.h = 338;
    
    
    
    /*
    while (currentState != QUIT) {
    switch (currentState) {
        case MENU:
            
                SDL_Event e;
                bool quit = false;
                while (!quit) {
                    while (SDL_PollEvent(&e)) {
                        if (e.type == SDL_QUIT) {
                                       quit = true;
                       } else if (e.type == SDL_KEYDOWN) {
                           switch (e.key.keysym.sym) {
                              case SDLK_UP:
                                  selectedItem--;
                                  if (selectedItem < 0) selectedItem = menuItemCount - 1;
                                  break;
                              case SDLK_DOWN:
                                  selectedItem++;
                                  if (selectedItem >= menuItemCount) selectedItem = 0;
                                  break;
                             case SDLK_RETURN:
                                switch (selectedItem) {
                                    case 0:
                                          //play
                                          currentState = PLAYING;
                                          break;
                                    case 1:
                                          // Help
                                          break;
                                    case 2:
                                         // High Scores
                                        //showscore();
                                        break;
                                    case 3:
                                        // Options
                                
                                        break;
                                    case 4:
                                          quit = true;
                                          currentState = QUIT;                         
                                          break;
                        }
                        break;
                      }
                    }
                 }
                
                // Render menu
                  
                SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
                SDL_RenderClear(rend);
                SDL_RenderCopy(rend, menu_tex, NULL, &menu_rect);
                renderMenu(rend, font, selectedItem);
        
                SDL_RenderPresent(rend);
                
               
            }
            break;

        case PLAYING:
            // Call game function to play
            game(win, rend);
            currentState = GAME_OVER;  // After game ends, transition to game over
            break;

        case GAME_OVER:
            while (!quitGameOver) {
                // Handle game over events
                while (SDL_PollEvent(&e)) {
                    // Handle events for game over screen
                }

                // Render game over screen
                SDL_RenderClear(rend);
                renderGameOver(rend, font);
                SDL_RenderPresent(rend);

                // Example transition to menu on key press
                if (isKeyPressed(SDL_SCANCODE_RETURN)) {
                    currentState = MENU;
                    quitMenu = false;  // Reset menu loop
                    selectedItem = 0;  // Reset selected item
                }
            }
            break;

        case QUIT:
            break;
    }
}
*/
        SDL_Event e;
        bool programquit = false;
        bool menuquit = false;
        bool gameoverloopquit = false;
        bool hscoreloopquit = false;
        bool helploopquit = false;
        bool optionloopquit = false;
        while (!programquit) {
             switch (currentState) {
                  case MENU:
                      menuquit = false; 
                      while (!menuquit) {
                         while (SDL_PollEvent(&e)) {
                             if (e.type == SDL_QUIT) {
                                 menuquit = true;
                                 currentState = QUIT;
                             } else if (e.type == SDL_KEYDOWN) {
                                           switch (e.key.keysym.sym) {
                                             case SDLK_UP:
                                                     selectedItem--;
                                                     if (selectedItem < 0) selectedItem = menuItemCount - 1;
                                                     break;
                                            case SDLK_DOWN:
                                                     selectedItem++;
                                                     if (selectedItem >= menuItemCount) selectedItem = 0;
                                                     break;
                                           case SDLK_RETURN:
                                               switch (selectedItem) {
                                                              case 0:
                                                               
                                                                 currentState = PLAYING;
                                                                 menuquit = true;
                                                                 break;
                                                              case 1:
                                                                    
                                                                    currentState = HELP;
                                                                    menuquit = true;
                                                                     break;
                                                              case 2:
                                                                     currentState = HIGH_SCORE_SHOW;
                                                                     menuquit = true;
                                                                     break;
                                                              case 3:
                                                                   
                                                                    currentState = OPTIONS;
                                                                    menuquit = true;
                                                                     break;
                                                              case 4:
                                                                     menuquit = true; 
                                                                     currentState = QUIT;
                                                                     break;
                                                                  }
                                                     break;
                                                   }
                                           }
                                         }

                //render menu
                SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
                SDL_RenderClear(rend);
                SDL_RenderCopy(rend, menu_tex, NULL, &menu_rect);
                SDL_RenderCopy(rend, menu_title , NULL, &title_rect);
                renderMenu(rend, font, selectedItem);
                SDL_RenderPresent(rend);
                
                
                
                
                
            }
            break;

        case PLAYING:
            game(win, rend);
            currentState = GAME_OVER; 
            break;
            
      
        case HIGH_SCORE_SHOW:
         hscoreloopquit = false;
         while (!hscoreloopquit) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    hscoreloopquit = true;
                    currentState = QUIT;
               } else if (e.type == SDL_KEYDOWN) {
                   switch (e.key.keysym.sym) {
                       case SDLK_RETURN:
                           currentState = MENU;
                           hscoreloopquit = true;
                           selectedItem = 0;  
                           break;
                        
                   }
               }
             }

           
           SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
           SDL_RenderClear(rend);
           highscorePage(win,rend,hscore,font);
           SDL_RenderPresent(rend);
          }
         break;
         
         
         case HELP:
         helploopquit = false;
         while (!helploopquit) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    helploopquit = true;
                    currentState = QUIT;
               } else if (e.type == SDL_KEYDOWN) {
                   switch (e.key.keysym.sym) {
                       case SDLK_RETURN:
                           currentState = MENU;
                           helploopquit = true;  
                           selectedItem = 0;  
                           break;
                         
                   }
               }
             }

           
           SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
           SDL_RenderClear(rend);
           helpPage(win,rend);
           SDL_RenderPresent(rend);
          }
         break;


        case GAME_OVER:
         gameoverloopquit = false;
         while (!gameoverloopquit) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    gameoverloopquit = true;
                    currentState = QUIT;
               } else if (e.type == SDL_KEYDOWN) {
                   switch (e.key.keysym.sym) {
                       case SDLK_RETURN:
                           currentState = MENU;
                           gameoverloopquit = true;
                           selectedItem = 0;  
                           break;
                        
                   }
               }
             }

           
           SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
           SDL_RenderClear(rend);
           renderGameOver(rend, font);
           SDL_RenderPresent(rend);
           
          }
         break;
         
         /*
         case OPTIONS:
         optionloopquit = false;
         while (!optionloopquit) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    optionloopquit = true;
                    currentState = QUIT;
               } else if (e.type == SDL_KEYDOWN) {
                   switch (e.key.keysym.sym) {
                       case SDLK_RETURN:
                           currentState = MENU;
                           optionloopquit = true;  // Exit help loop
                           selectedItem = 0;  // Reset selected item for menu
                           break;
                         // Handle other key events if needed
                   }
               }
             }

           SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
           SDL_RenderClear(rend);
           optionsdisplayfunc(win,rend,font,soundOn,musicOn);
           SDL_RenderPresent(rend);
          }
         break;
         */
 
   case OPTIONS:
      optionloopquit = false;
       while (!optionloopquit) {
            while (SDL_PollEvent(&e)) {
              if (e.type == SDL_QUIT) {
                optionloopquit = true;
                currentState = QUIT;
                } else if (e.type == SDL_KEYDOWN) {
                   switch (e.key.keysym.sym) {
                      case SDLK_RETURN:
                        currentState = MENU;
                        optionloopquit = true;
                        selectedItem = 0;
                        break;
                       case SDLK_s:
                        soundOn = !soundOn; 
                        if (soundOn) {
                        Mix_Volume(-1, MIX_MAX_VOLUME);  // Set sound volume to max
                        } else {
                            Mix_Volume(-1, 0);  // Mute sound
                        }
                        break;
                    case SDLK_m:
                        musicOn = !musicOn; 
                        if (musicOn) {
                            Mix_VolumeMusic(MIX_MAX_VOLUME);  // Set music volume to max
                        } else {
                            Mix_VolumeMusic(0);  // Mute music
                        }
                        break;
                   
                }
            }
        }

        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);
        optionsdisplayfunc(win,rend,font,soundOn,musicOn);
        SDL_RenderPresent(rend);
    }
    break;


        case QUIT:
            programquit = true; 
            break;
    }
}

    for (int i = 0; i < 6; i++) {
        Mix_FreeMusic(musics[i]);
        musics[i] = NULL; 
    }
    Mix_CloseAudio();
    
    SDL_DestroyTexture(menu_tex);
    SDL_DestroyTexture(menu_title);
    
    TTF_CloseFont(font);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}


