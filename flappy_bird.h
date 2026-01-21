// Flappy Bird Game for M5Stick Nemo
// By Ponticelli Domenico.
// 12NOV2020 EEPROM Working now, Modified by Zontex
// https://github.com/pcelli85/M5Stack_FlappyBird_game

#include <EEPROM.h>

// External declarations for global variables defined in main .ino file
extern bool rstOverride;
extern bool isSwitching;
extern int current_proc;

#define TFTW  135  // screen width
#define TFTH  240  // screen height
#define TFTW2 67   // half screen width
#define TFTH2 120  // half screen height
// game constant
#define SPEED         1
#define GRAVITY       9.8
#define JUMP_FORCE    2.15
#define SKIP_TICKS    20.0  // 1000 / 50fps
#define MAX_FRAMESKIP 5
// bird size
#define BIRDW  8  // bird width
#define BIRDH  8  // bird height
#define BIRDW2 4  // half width
#define BIRDH2 4  // half height
// pipe size
#define PIPEW     15  // pipe width
#define GAPHEIGHT 30  // pipe gap height
// floor size
#define FLOORH 20  // floor height (from bottom of the screen)
// grass size
#define GRASSH 4  // grass height (inside floor, starts at floor y)

int flappy_address = 20;
int maxScore = 0;

// Color variables (computed at runtime)
static uint16_t BCKGRDCOL;
static uint16_t BIRDCOL;
static uint16_t PIPECOL;
static uint16_t PIPEHIGHCOL;
static uint16_t PIPESEAMCOL;
static uint16_t FLOORCOL;
static uint16_t GRASSCOL;
static uint16_t GRASSCOL2;

// bird sprite
// bird sprite colors (Cx name for values to keep the array readable)
static uint16_t C0, C1, C2, C3, C4, C5;

static unsigned int birdcol[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// bird structure
static struct BIRD {
    long x, y, old_y;
    long col;
    float vel_y;
} bird;

// pipe structure
static struct PIPES {
    long x, gap_y;
    long col;
} pipes;

// score
int score;
// temporary x and y var
static short tmpx, tmpy;

// Initialize colors - must be called before game starts
static void init_flappy_colors() {
    BCKGRDCOL = DISP.color565(138, 235, 244);
    BIRDCOL = DISP.color565(255, 254, 174);
    PIPECOL = DISP.color565(99, 255, 78);
    PIPEHIGHCOL = DISP.color565(250, 255, 250);
    PIPESEAMCOL = DISP.color565(0, 0, 0);
    FLOORCOL = DISP.color565(246, 240, 163);
    GRASSCOL = DISP.color565(141, 225, 87);
    GRASSCOL2 = DISP.color565(156, 239, 88);
    
    // Bird sprite colors
    C0 = BCKGRDCOL;
    C1 = DISP.color565(195, 165, 75);
    C2 = BIRDCOL;
    C3 = TFT_WHITE;
    C4 = TFT_RED;
    C5 = DISP.color565(251, 216, 114);
    
    // Initialize bird sprite array
    birdcol[0] = C0; birdcol[1] = C0; birdcol[2] = C1; birdcol[3] = C1; birdcol[4] = C1; birdcol[5] = C1; birdcol[6] = C1; birdcol[7] = C0;
    birdcol[8] = C0; birdcol[9] = C0; birdcol[10] = C1; birdcol[11] = C1; birdcol[12] = C1; birdcol[13] = C1; birdcol[14] = C1; birdcol[15] = C0;
    birdcol[16] = C0; birdcol[17] = C1; birdcol[18] = C2; birdcol[19] = C2; birdcol[20] = C2; birdcol[21] = C1; birdcol[22] = C3; birdcol[23] = C1;
    birdcol[24] = C0; birdcol[25] = C1; birdcol[26] = C2; birdcol[27] = C2; birdcol[28] = C2; birdcol[29] = C1; birdcol[30] = C3; birdcol[31] = C1;
    birdcol[32] = C0; birdcol[33] = C2; birdcol[34] = C2; birdcol[35] = C2; birdcol[36] = C2; birdcol[37] = C1; birdcol[38] = C3; birdcol[39] = C1;
    birdcol[40] = C0; birdcol[41] = C2; birdcol[42] = C2; birdcol[43] = C2; birdcol[44] = C2; birdcol[45] = C1; birdcol[46] = C3; birdcol[47] = C1;
    birdcol[48] = C1; birdcol[49] = C1; birdcol[50] = C1; birdcol[51] = C1; birdcol[52] = C2; birdcol[53] = C2; birdcol[54] = C3; birdcol[55] = C1;
    birdcol[56] = C1; birdcol[57] = C1; birdcol[58] = C1; birdcol[59] = C1; birdcol[60] = C1; birdcol[61] = C2; birdcol[62] = C2; birdcol[63] = C3;
    birdcol[64] = C1; birdcol[65] = C1; birdcol[66] = C1; birdcol[67] = C2; birdcol[68] = C2; birdcol[69] = C2; birdcol[70] = C2; birdcol[71] = C2;
    birdcol[72] = C4; birdcol[73] = C4; birdcol[74] = C1; birdcol[75] = C2; birdcol[76] = C2; birdcol[77] = C2; birdcol[78] = C2; birdcol[79] = C2;
    birdcol[80] = C4; birdcol[81] = C4; birdcol[82] = C1; birdcol[83] = C2; birdcol[84] = C2; birdcol[85] = C2; birdcol[86] = C1; birdcol[87] = C5;
    birdcol[88] = C4; birdcol[89] = C0; birdcol[90] = C1; birdcol[91] = C2; birdcol[92] = C2; birdcol[93] = C2; birdcol[94] = C1; birdcol[95] = C5;
    birdcol[96] = C4; birdcol[97] = C0; birdcol[98] = C0; birdcol[99] = C1; birdcol[100] = C2; birdcol[101] = C1; birdcol[102] = C5; birdcol[103] = C5;
    birdcol[104] = C5; birdcol[105] = C0; birdcol[106] = C0; birdcol[107] = C1; birdcol[108] = C2; birdcol[109] = C1; birdcol[110] = C5; birdcol[111] = C5;
    birdcol[112] = C5; birdcol[113] = C0; birdcol[114] = C0; birdcol[115] = C0; birdcol[116] = C1; birdcol[117] = C5; birdcol[118] = C5; birdcol[119] = C5;
    birdcol[120] = C0; birdcol[121] = C0; birdcol[122] = C0; birdcol[123] = C0; birdcol[124] = C1; birdcol[125] = C5; birdcol[126] = C5; birdcol[127] = C5;
    birdcol[128] = C0; birdcol[129] = C0;
}

// ---------------
// draw pixel
// ---------------
// faster drawPixel method by inlining calls and using setAddrWindow and
// pushColor using macro to force inlining
#define drawPixel(a, b, c)            \
    DISP.setAddrWindow(a, b, a, b); \
    DISP.pushColor(c)
// ---------------
// game loop
// ---------------
void game_loop() {
    // ===============
    // prepare game variables
    // draw floor
    // ===============
    // instead of calculating the distance of the floor from the screen height
    // each time store it in a variable
    unsigned char GAMEH = TFTH - FLOORH;
    // draw the floor once, we will not overwrite on this area in-game
    // black line
    DISP.drawFastHLine(0, GAMEH, TFTW, TFT_BLACK);
    // grass and stripe
    DISP.fillRect(0, GAMEH + 1, TFTW2, GRASSH, GRASSCOL);
    DISP.fillRect(TFTW2, GAMEH + 1, TFTW2, GRASSH, GRASSCOL2);
    // black line
    DISP.drawFastHLine(0, GAMEH + GRASSH, TFTW, TFT_BLACK);
    // mud
    DISP.fillRect(0, GAMEH + GRASSH + 1, TFTW, FLOORH - GRASSH, FLOORCOL);
    // grass x position (for stripe animation)
    long grassx = TFTW;
    // game loop time variables
    double delta, old_time, next_game_tick, current_time;
    next_game_tick = current_time = millis();
    int loops;
    // passed pipe flag to count score
    bool passed_pipe = false;
    // temp var for setAddrWindow
    unsigned char px;

    while (1) {
        loops = 0;
        while (millis() > next_game_tick && loops < MAX_FRAMESKIP) {
            if (digitalRead(M5_BUTTON_HOME) == LOW) {
                // while(digitalRead(M5_BUTTON_HOME) == LOW);
                if (bird.y > BIRDH2 * 0.5) bird.vel_y = -JUMP_FORCE;
                // else zero velocity
                else
                    bird.vel_y = 0;
            }

            // ===============
            // update
            // ===============
            // calculate delta time
            // ---------------
            old_time     = current_time;
            current_time = millis();
            delta        = (current_time - old_time) / 1000;

            // bird
            // ---------------
            bird.vel_y += GRAVITY * delta;
            bird.y += bird.vel_y;

            // pipe
            // ---------------

            pipes.x -= SPEED;
            // if pipe reached edge of the screen reset its position and gap
            if (pipes.x < -PIPEW) {
                pipes.x     = TFTW;
                pipes.gap_y = random(10, GAMEH - (10 + GAPHEIGHT));
            }

            // ---------------
            next_game_tick += SKIP_TICKS;
            loops++;
        }

        // ===============
        // draw
        // ===============
        // pipe
        // ---------------
        // we save cycles if we avoid drawing the pipe when outside the screen

        if (pipes.x >= 0 && pipes.x < TFTW) {
            // pipe color
            DISP.drawFastVLine(pipes.x + 3, 0, pipes.gap_y, PIPECOL);
            DISP.drawFastVLine(pipes.x + 3, pipes.gap_y + GAPHEIGHT + 1,
                                 GAMEH - (pipes.gap_y + GAPHEIGHT + 1),
                                 PIPECOL);
            // highlight
            DISP.drawFastVLine(pipes.x, 0, pipes.gap_y, PIPEHIGHCOL);
            DISP.drawFastVLine(pipes.x, pipes.gap_y + GAPHEIGHT + 1,
                                 GAMEH - (pipes.gap_y + GAPHEIGHT + 1),
                                 PIPEHIGHCOL);
            // bottom and top border of pipe
            drawPixel(pipes.x, pipes.gap_y, PIPESEAMCOL);
            drawPixel(pipes.x, pipes.gap_y + GAPHEIGHT, PIPESEAMCOL);
            // pipe seam
            drawPixel(pipes.x, pipes.gap_y - 6, PIPESEAMCOL);
            drawPixel(pipes.x, pipes.gap_y + GAPHEIGHT + 6, PIPESEAMCOL);
            drawPixel(pipes.x + 3, pipes.gap_y - 6, PIPESEAMCOL);
            drawPixel(pipes.x + 3, pipes.gap_y + GAPHEIGHT + 6, PIPESEAMCOL);
        }
#if 1
        // erase behind pipe
        if (pipes.x <= TFTW)
            DISP.drawFastVLine(pipes.x + PIPEW, 0, GAMEH, BCKGRDCOL);
            // M5.Lcd.drawFastVLine(pipes.x, 0, GAMEH, BCKGRDCOL);
            // PIPECOL
#endif
        // bird
        // ---------------
        tmpx = BIRDW - 1;
        do {
            px = bird.x + tmpx + BIRDW;
            // clear bird at previous position stored in old_y
            // we can't just erase the pixels before and after current position
            // because of the non-linear bird movement (it would leave 'dirty'
            // pixels)
            tmpy = BIRDH - 1;
            do {
                drawPixel(px, bird.old_y + tmpy, BCKGRDCOL);
            } while (tmpy--);
            // draw bird sprite at new position
            tmpy = BIRDH - 1;
            do {
                drawPixel(px, bird.y + tmpy, birdcol[tmpx + (tmpy * BIRDW)]);
            } while (tmpy--);
        } while (tmpx--);
        // save position to erase bird on next draw
        bird.old_y = bird.y;

        // grass stripes
        // ---------------
        grassx -= SPEED;
        if (grassx < 0) grassx = TFTW;
        DISP.drawFastVLine(grassx % TFTW, GAMEH + 1, GRASSH - 1, GRASSCOL);
        DISP.drawFastVLine((grassx + 64) % TFTW, GAMEH + 1, GRASSH - 1,
                             GRASSCOL2);

        // ===============
        // collision
        // ===============
        // if the bird hit the ground game over
        if (bird.y > GAMEH - BIRDH) break;
        // checking for bird collision with pipe
        if (bird.x + BIRDW >= pipes.x - BIRDW2 &&
            bird.x <= pipes.x + PIPEW - BIRDW) {
            // bird entered a pipe, check for collision
            if (bird.y < pipes.gap_y ||
                bird.y + BIRDH > pipes.gap_y + GAPHEIGHT)
                break;
            else
                passed_pipe = true;
        }
        // if bird has passed the pipe increase score
        else if (bird.x > pipes.x + PIPEW - BIRDW && passed_pipe) {
            passed_pipe = false;
            // erase score with background color
            DISP.setTextColor(BCKGRDCOL);
            DISP.setCursor(TFTW2, 4);
            DISP.print(score);
            // set text color back to white for new score
            DISP.setTextColor(TFT_WHITE);
            // increase score since we successfully passed a pipe
            score++;
        }

        // update score
        // ---------------
        DISP.setCursor(2, 4);
        DISP.print(score);
    }

    // add a small delay to show how the player lost
    delay(1200);
}

void game_init() {
    // Initialize colors first
    init_flappy_colors();
    
    // clear screen
    DISP.fillScreen(BCKGRDCOL);
    // reset score
    score = 0;
    // init bird
    bird.x = 30;
    bird.y = bird.old_y = TFTH2 - BIRDH;
    bird.vel_y          = -JUMP_FORCE;
    tmpx = tmpy = 0;
    // generate new random seed for the pipe gape
    randomSeed(analogRead(0));
    // init pipe
    pipes.x     = 0;
    pipes.gap_y = random(20, TFTH - 60);
}

// ---------------
// game start
// ---------------
void game_start() {
    DISP.fillScreen(TFT_BLACK);
    DISP.fillRect(0, TFTH2 - 10, TFTW, 1, TFT_WHITE);
    DISP.fillRect(0, TFTH2 + 15, TFTW, 1, TFT_WHITE);
    DISP.setTextColor(TFT_WHITE);
    DISP.setTextSize(1);
    // half width - num char * char width in pixels
    DISP.setCursor(TFTW2 - 15, TFTH2 - 6);
    DISP.println("FLAPPY");
    DISP.setTextSize(1);
    DISP.setCursor(TFTW2 - 15, TFTH2 + 6);
    DISP.println("-BIRD-");
    DISP.setTextSize(1);
    DISP.setCursor(15, TFTH2 - 21);
    DISP.println("M5StickC");
    DISP.setCursor(TFTW2 - 40, TFTH2 + 21);
    DISP.println("please press home");
    while (1) {
        // wait for push button
        if (digitalRead(M5_BUTTON_HOME) == LOW) {
            while (digitalRead(M5_BUTTON_HOME) == LOW)
                ;
            break;
        }
    }
    // init game settings
    game_init();
}

// ---------------
// game over
// ---------------
void game_over() {
    DISP.fillScreen(TFT_BLACK);
#if defined(USE_EEPROM)
    EEPROM.get(flappy_address, maxScore);
#else
    maxScore = 0;
#endif

    if (score > maxScore) {
#if defined(USE_EEPROM)
        EEPROM.put(flappy_address, score);
        EEPROM.commit();
#endif
        maxScore = score;
        DISP.setTextColor(TFT_RED);
        DISP.setTextSize(1);
        DISP.setCursor(0, TFTH2 - 16);
        DISP.println("NEW HIGHSCORE");
    }

    DISP.setTextColor(TFT_WHITE);
    DISP.setTextSize(1);
    // half width - num char * char width in pixels
    DISP.setCursor(TFTW2 - 25, TFTH2 - 6);
    DISP.println("GAME OVER");
    DISP.setTextSize(1);
    DISP.setCursor(1, 10);
    DISP.print("score: ");
    DISP.print(score);
    DISP.setCursor(5, TFTH2 + 6);
    DISP.println("press button");
    DISP.setCursor(1, 21);
    DISP.print("Max Score:");
    DISP.print(maxScore);
    while (1) {
        // wait for push button
        if (digitalRead(M5_BUTTON_HOME) == LOW) {
            while (digitalRead(M5_BUTTON_HOME) == LOW)
                ;
            break;
        }
    }
}

// Setup function for Flappy Bird
void flappy_bird_setup() {
    rstOverride = false;
    game_start();
}

// Loop function for Flappy Bird
void flappy_bird_loop() {
    game_loop();
    game_over();
    // Return to main menu after game over
    rstOverride = false;
    isSwitching = true;
    current_proc = 1;
}
