#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <math.h>
#include <string.h>

#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define SCREEN_WIDTH_DEFAULT 800
#define SCREEN_HEIGHT_DEFAULT 600
#define FOV 60
#define PI 3.14159265359

typedef struct {
    float x, y;
    float angle;
    float fov;
    int health;
    int score;
} Player;

Player player = {2.0f, 2.0f, 0.0f, FOV};

int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1},
    {1,0,0,1,0,0,0,1,1,0,0,0,1,0,0,1},
    {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1},
    {1,0,0,1,0,0,0,1,1,0,0,0,1,0,0,1},
    {1,0,0,1,1,1,0,1,1,0,0,1,1,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int gameState = 0;
int keys[256] = {0};
int selectedMenuItem = 0;
const char* menuItems[] = {"Start Game", "Settings", "Load Game", "Exit"};
const int numMenuItems = 4;

int selectedSettingsItem = 0;
static char fovBuffer[16];
static char sizeBuffer[32];
const char* settingsItems[] = {fovBuffer, "Sound: ON", sizeBuffer, "Back"};
const int numSettingsItems = 4;

int selectedPauseMenuItem = 0;
const char* pauseMenuItems[] = {"Resume", "Save Game", "Load Game", "Settings", "Main Menu"};
const int numPauseMenuItems = 5;

int previousState = 0;
int screenWidth = SCREEN_WIDTH_DEFAULT;
int screenHeight = SCREEN_HEIGHT_DEFAULT;
static int soundOn = 1;
static int sizeIndex = 0;
const int screenSizes[][2] = {
    {800, 600},
    {1024, 768},
    {1280, 720},
    {1920, 1080}
};
const int numScreenSizes = 4;

void saveSettings() {
    FILE* file = fopen("settings.dat", "wb");
    if (file) {
        fwrite(&player.fov, sizeof(float), 1, file);
        fwrite(&soundOn, sizeof(int), 1, file);
        fwrite(&sizeIndex, sizeof(int), 1, file);
        fclose(file);
    }
}

void loadSettings() {
    FILE* file = fopen("settings.dat", "rb");
    if (file) {
        fread(&player.fov, sizeof(float), 1, file);
        fread(&soundOn, sizeof(int), 1, file);
        fread(&sizeIndex, sizeof(int), 1, file);
        fclose(file);

        // Poprawiony format dla fovBuffer
        snprintf(fovBuffer, sizeof(fovBuffer), "FOV: %.0f", player.fov);
        settingsItems[0] = fovBuffer;
        settingsItems[1] = soundOn ? "Sound: ON" : "Sound: OFF";
        snprintf(sizeBuffer, sizeof(sizeBuffer), "SIZE: %dx%d", screenSizes[sizeIndex][0], screenSizes[sizeIndex][1]);
        settingsItems[2] = sizeBuffer;

        screenWidth = screenSizes[sizeIndex][0];
        screenHeight = screenSizes[sizeIndex][1];
        glutReshapeWindow(screenWidth, screenHeight);

        // Update OpenGL projection and viewport
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, screenWidth, 0, screenHeight);
        glViewport(0, 0, screenWidth, screenHeight);
        glMatrixMode(GL_MODELVIEW);
    } else {
        snprintf(fovBuffer, sizeof(fovBuffer), "FOV: %.0f", player.fov);
        snprintf(sizeBuffer, sizeof(sizeBuffer), "SIZE: %dx%d", screenSizes[sizeIndex][0], screenSizes[sizeIndex][1]);
        settingsItems[0] = fovBuffer;
        settingsItems[1] = "Sound: ON";
        settingsItems[2] = sizeBuffer;
    }
}

void reshape(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glutPostRedisplay();
}

void drawMenu() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(screenWidth / 2 - 50, screenHeight / 2 + 100);
    const char* title = "GAME";
    for (int i = 0; title[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
    }

    for (int i = 0; i < numMenuItems; i++) {
        if (i == selectedMenuItem) {
            glColor3f(1.0f, 1.0f, 0.0f);
        } else {
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        glRasterPos2i(screenWidth / 2 - 50, screenHeight / 2 - 20 - i * 40);
        for (int j = 0; menuItems[i][j] != '\0'; j++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, menuItems[i][j]);
        }
    }
}

void drawSettings() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(screenWidth / 2 - 50, screenHeight / 2 + 100);
    const char* title = "SETTINGS";
    for (int i = 0; title[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
    }

    for (int i = 0; i < numSettingsItems; i++) {
        if (i == selectedSettingsItem) {
            glColor3f(1.0f, 1.0f, 0.0f);
        } else {
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        glRasterPos2i(screenWidth / 2 - 50, screenHeight / 2 - 20 - i * 40);
        for (int j = 0; settingsItems[i][j] != '\0'; j++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, settingsItems[i][j]);
        }
    }
}

void drawPause() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(screenWidth / 2 - 50, screenHeight / 2 + 100);
    const char* title = "PAUZA";
    for (int i = 0; title[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
    }

    for (int i = 0; i < numPauseMenuItems; i++) {
        if (i == selectedPauseMenuItem) {
            glColor3f(1.0f, 1.0f, 0.0f);
        } else {
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        glRasterPos2i(screenWidth / 2 - 50, screenHeight / 2 - 20 - i * 40);
        for (int j = 0; pauseMenuItems[i][j] != '\0'; j++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, pauseMenuItems[i][j]);
        }
    }
}

float convertToRadians(float degree) {
    return degree * PI / 180.0f;
}

float castRays(float rayAngle) {
    float rayX = player.x;
    float rayY = player.y;

    float raYDirX = cos(rayAngle);
    float raYDirY = sin(rayAngle);

    int mapX = (int)rayX;
    int mapY = (int)rayY;

    float deltaDistanceX = fabs(1.0f / raYDirX);
    float deltaDistanceY = fabs(1.0f / raYDirY);

    float sideDistanceX, sideDistanceY;
    int stepX, stepY, hit = 0, side;

    if (raYDirX < 0) {
        stepX = -1;
        sideDistanceX = (rayX - mapX) * deltaDistanceX;
    } else {
        stepX = 1;
        sideDistanceX = (mapX + 1.0f - rayX) * deltaDistanceX;
    }

    if (raYDirY < 0) {
        stepY = -1;
        sideDistanceY = (rayY - mapY) * deltaDistanceY;
    } else {
        stepY = 1;
        sideDistanceY = (mapY + 1.0f - rayY) * deltaDistanceY;
    }

    while (hit == 0) {
        if (sideDistanceX < sideDistanceY) {
            sideDistanceX += deltaDistanceX;
            mapX += stepX;
            side = 0;
        } else {
            sideDistanceY += deltaDistanceY;
            mapY += stepY;
            side = 1;
        }

        if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT || worldMap[mapY][mapX] > 0) {
            hit = 1;
        }
    }

    float prepWallDistance;
    if (side == 0) {
        prepWallDistance = (mapX - rayX + (1 - stepX) / 2) / raYDirX;
    } else {
        prepWallDistance = (mapY - rayY + (1 - stepY) / 2) / raYDirY;
    }
    return prepWallDistance;
}

void drawMap() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(screenWidth, 0);
    glVertex2i(screenWidth, screenHeight / 2);
    glVertex2i(0, screenHeight / 2);
    glEnd();

    glColor3f(0.3f, 0.3f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(0, screenHeight / 2);
    glVertex2f(screenWidth, screenHeight / 2);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();

    float angleStart = player.angle - convertToRadians(player.fov / 2.0f);
    float angleStep = convertToRadians(player.fov) / screenWidth;

    for (int x = 0; x < screenWidth; x++) {
        float rayAngle = angleStart + angleStep * x;
        float distance = castRays(rayAngle);

        distance *= cos(rayAngle - player.angle);

        int wallHeight = (int)(screenHeight / distance);
        int drawStart = -wallHeight / 2 + screenHeight / 2;

        if (drawStart < 0) {
            drawStart = 0;
        }

        int drawEnd = wallHeight / 2 + screenHeight / 2;
        if (drawEnd >= screenHeight) {
            drawEnd = screenHeight - 1;
        }

        float brightness = 1.0f - (distance / 15.0f);
        if (brightness < 0.1f) {
            brightness = 0.1f;
        }
        if (brightness > 1.0f) {
            brightness = 1.0f;
        }

        float rayX = player.x + distance * cos(rayAngle);
        float rayY = player.y + distance * sin(rayAngle);
        int mapX = (int)rayX;
        int mapY = (int)rayY;

        float wallX = rayX - mapX;
        float wallY = rayY - mapY;

        if (fabs(wallX) < 0.01f || fabs(wallX - 1.0f) < 0.01f) {
            glColor3f(0.8f * brightness, 0.3f * brightness, 0.3f * brightness);
        } else {
            glColor3f(0.6f * brightness, 0.6f * brightness, 0.8f * brightness);
        }

        glBegin(GL_LINES);
        glVertex2i(x, drawStart);
        glVertex2i(x, drawEnd);
        glEnd();
    }
}

void playerControl() {
    float moveSpeed = 0.05f;
    float rotationSpeed = 0.03f;

    if (keys['a'] || keys['A']) {
        player.angle -= rotationSpeed;
    }
    if (keys['d'] || keys['D']) {
        player.angle += rotationSpeed;
    }

    float newX = player.x;
    float newY = player.y;

    if (keys['w'] || keys['W']) {
        newX += cos(player.angle) * moveSpeed;
        newY += sin(player.angle) * moveSpeed;
    }
    if (keys['s'] || keys['S']) {
        newX -= cos(player.angle) * moveSpeed;
        newY -= sin(player.angle) * moveSpeed;
    }

    int mapX = (int)newX;
    int mapY = (int)newY;

    if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
        if (worldMap[mapY][(int)player.x] == 0) {
            player.y = newY;
        }
        if (worldMap[(int)player.y][mapX] == 0) {
            player.x = newX;
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameState == 0) {
        drawMenu();
    } else if (gameState == 1) {
        playerControl();
        drawMap();
    } else if (gameState == 2) {
        drawSettings();
    } else if (gameState == 3) {
        drawPause();
    }

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (gameState == 0 && key == 13) {
        if (selectedMenuItem == 0) {
            gameState = 1;
        } else if (selectedMenuItem == 1) {
            previousState = 0;
            gameState = 2;
            selectedSettingsItem = 0;
        } else if (selectedMenuItem == 2) {
            loadSettings();
            gameState = 1;
        } else if (selectedMenuItem == 3) {
            exit(0);
        }
    } else if (gameState == 2 && key == 13) {
        if (selectedSettingsItem == 0) {
            player.fov += 10;
            if (player.fov > 90) {
                player.fov = 30;
            }
            snprintf(fovBuffer, sizeof(fovBuffer), "FOV: %.0f", player.fov);
            settingsItems[0] = fovBuffer;
            saveSettings();
        } else if (selectedSettingsItem == 1) {
            soundOn = !soundOn;
            settingsItems[1] = soundOn ? "Sound: ON" : "Sound: OFF";
            saveSettings();
        } else if (selectedSettingsItem == 2) {
            sizeIndex = (sizeIndex + 1) % numScreenSizes;
            screenWidth = screenSizes[sizeIndex][0];
            screenHeight = screenSizes[sizeIndex][1];
            snprintf(sizeBuffer, sizeof(sizeBuffer), "SIZE: %dx%d", screenWidth, screenHeight);
            settingsItems[2] = sizeBuffer;
            glutReshapeWindow(screenWidth, screenHeight);
            saveSettings();
        } else if (selectedSettingsItem == 3) {
            gameState = previousState;
            if (previousState == 0) {
                selectedMenuItem = 0;
            } else if (previousState == 3) {
                selectedPauseMenuItem = 0;
            }
        }
    } else if (gameState == 3 && key == 13) {
        if (selectedPauseMenuItem == 0) {
            gameState = 1;
        } else if (selectedPauseMenuItem == 1) {
            saveSettings();
        } else if (selectedPauseMenuItem == 2) {
            loadSettings();
            gameState = 1;
        } else if (selectedPauseMenuItem == 3) {
            previousState = 3;
            gameState = 2;
            selectedSettingsItem = 0;
        } else if (selectedPauseMenuItem == 4) {
            gameState = 0;
            selectedMenuItem = 0;
        }
    } else if (gameState == 1) {
        keys[key] = 1;
    }
    if (key == 27) {
        if (gameState == 1) {
            gameState = 3;
            selectedPauseMenuItem = 0;
        } else if (gameState == 2) {
            gameState = previousState;
            if (previousState == 0) selectedMenuItem = 0;
            else if (previousState == 3) selectedPauseMenuItem = 0;
        } else if (gameState == 3) {
            gameState = 1;
            selectedPauseMenuItem = 0;
        } else {
            exit(0);
        }
    }
    glutPostRedisplay();
}

void keyboardUp(unsigned char key, int x, int y) {
    if (gameState == 1) {
        keys[key] = 0;
    }
}

void specialKeys(int key, int x, int y) {
    if (gameState == 0) {
        if (key == GLUT_KEY_UP) {
            selectedMenuItem = (selectedMenuItem - 1 + numMenuItems) % numMenuItems;
        } else if (key == GLUT_KEY_DOWN) {
            selectedMenuItem = (selectedMenuItem + 1) % numMenuItems;
        }
        glutPostRedisplay();
    } else if (gameState == 2) {
        if (key == GLUT_KEY_UP) {
            selectedSettingsItem = (selectedSettingsItem - 1 + numSettingsItems) % numSettingsItems;
        } else if (key == GLUT_KEY_DOWN) {
            selectedSettingsItem = (selectedSettingsItem + 1) % numSettingsItems;
        }
        glutPostRedisplay();
    } else if (gameState == 3) {
        if (key == GLUT_KEY_UP) {
            selectedPauseMenuItem = (selectedPauseMenuItem - 1 + numPauseMenuItems) % numPauseMenuItems;
        } else if (key == GLUT_KEY_DOWN) {
            selectedPauseMenuItem = (selectedPauseMenuItem + 1) % numPauseMenuItems;
        }
        glutPostRedisplay();
    }
}

void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    loadSettings();
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();

    return 0;
}