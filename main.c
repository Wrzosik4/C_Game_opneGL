#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define SCREEN_WIDTH_DEFAULT 800
#define SCREEN_HEIGHT_DEFAULT 600
#define FOV 60
#define PI 3.14159265359
#define SAVE_VERSION 1
#define MAX_SAVES 10
#define LOAD_GAME_STATE 4
#define DELETE_CONFIRM_STATE 5

typedef struct {
    float x, y;
    float angle;
    float fov;
    int health;
    int score;
} Player;

typedef struct {
    char filename[256];
    time_t saveTime;
} SaveEntry;

Player player = {2.0f, 2.0f, 0.0f, FOV};
int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1},
    {1,0,0,1,0,0,0,1,1,0,0,0,1,0,0,1},
    {1,0,0,0,0,1,1,1,0,0,0,1,1,0,0,1},
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

int selectedSaveItem = 0;
SaveEntry saveList[MAX_SAVES];
int numSaves = 0;
int deleteMode = 0; // 0 = normal mode, 1 = delete mode
int deleteConfirmItem = -1; // indeks save'a do usunięcia
int selectedConfirmButton = 0; // 0 = Tak, 1 = Nie

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

void scanSaves() {
    numSaves = 0;
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile("*.sav", &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error scanning saves: %d\n", GetLastError());
        return;
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && numSaves < MAX_SAVES) {
            strncpy(saveList[numSaves].filename, findFileData.cFileName, sizeof(saveList[numSaves].filename));
            SYSTEMTIME st;
            FileTimeToSystemTime(&findFileData.ftLastWriteTime, &st);
            struct tm tm = {0};
            tm.tm_year = st.wYear - 1900;
            tm.tm_mon = st.wMonth - 1;
            tm.tm_mday = st.wDay;
            tm.tm_hour = st.wHour;
            tm.tm_min = st.wMinute;
            tm.tm_sec = st.wSecond;
            saveList[numSaves].saveTime = mktime(&tm);
            numSaves++;
        }
    } while (FindNextFile(hFind, &findFileData) != 0 && numSaves < MAX_SAVES);

    FindClose(hFind);
}

void deleteSave(int saveIndex) {
    if (saveIndex >= 0 && saveIndex < numSaves) {
        if (remove(saveList[saveIndex].filename) == 0) {
            printf("Save deleted: %s\n", saveList[saveIndex].filename);
        } else {
            printf("Error deleting save: %s\n", saveList[saveIndex].filename);
        }
        scanSaves(); // Odśwież listę zapisów
        if (selectedSaveItem >= numSaves && numSaves > 0) {
            selectedSaveItem = numSaves - 1;
        }
    }
}

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

        snprintf(fovBuffer, sizeof(fovBuffer), "FOV: %.0f", player.fov);
        settingsItems[0] = fovBuffer;
        settingsItems[1] = soundOn ? "Sound: ON" : "Sound: OFF";
        snprintf(sizeBuffer, sizeof(sizeBuffer), "SIZE: %dx%d", screenSizes[sizeIndex][0], screenSizes[sizeIndex][1]);
        settingsItems[2] = sizeBuffer;

        screenWidth = screenSizes[sizeIndex][0];
        screenHeight = screenSizes[sizeIndex][1];
        glutReshapeWindow(screenWidth, screenHeight);

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

void saveGame(Player *player, const char* saveName) {
    FILE* file = fopen(saveName, "wb");
    if (file == NULL) {
        printf("Game save error\n");
        return;
    }
    int saveVersion = SAVE_VERSION;
    time_t saveTime = time(NULL);
    fwrite(&saveVersion, sizeof(int), 1, file);
    fwrite(&saveTime, sizeof(time_t), 1, file);
    fwrite(player, sizeof(Player), 1, file);
    fwrite(worldMap, sizeof(worldMap), 1, file);
    fclose(file);
    scanSaves();
}

void loadGame(Player *player, const char* fileName, time_t* saveTime) {
    FILE* file = fopen(fileName, "rb");
    if (file == NULL) {
        printf("Game load error: %s\n", fileName);
        return;
    }

    int saveVersion;
    if (fread(&saveVersion, sizeof(int), 1, file) != 1 || saveVersion != SAVE_VERSION) {
        printf("Invalid file format: %s\n", fileName);
        fclose(file);
        return;
    }

    if (fread(saveTime, sizeof(time_t), 1, file) != 1) {
        printf("Error loading save time: %s\n", fileName);
        fclose(file);
        return;
    }

    if (fread(player, sizeof(Player), 1, file) != 1) {
        printf("Error loading player data: %s\n", fileName);
        fclose(file);
        return;
    }

    if (fread(worldMap, sizeof(worldMap), 1, file) != 1) {
        printf("Error loading world map: %s\n", fileName);
        fclose(file);
        return;
    }
    fclose(file);
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

void drawDeleteConfirm() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(screenWidth / 2 - 100, screenHeight / 2 + 80);
    const char* title = "CONFIRM DELETE";
    for (int i = 0; title[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
    }

    glColor3f(1.0f, 0.8f, 0.8f);
    glRasterPos2i(screenWidth / 2 - 100, screenHeight / 2 + 40);
    const char* question = "Are you sure you want to delete?:";
    for (int i = 0; question[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, question[i]);
    }

    glColor3f(1.0f, 1.0f, 0.0f);
    glRasterPos2i(screenWidth / 2 - 100, screenHeight / 2);
    if (deleteConfirmItem >= 0 && deleteConfirmItem < numSaves) {
        for (int i = 0; saveList[deleteConfirmItem].filename[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, saveList[deleteConfirmItem].filename[i]);
        }
    }

    // Przyciski Tak/Nie
    const char* buttons[] = {"YES", "NO"};
    for (int i = 0; i < 2; i++) {
        if (i == selectedConfirmButton) {
            glColor3f(1.0f, 1.0f, 0.0f);
        } else {
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        glRasterPos2i(screenWidth / 2 - 50 + i * 100, screenHeight / 2 - 40);
        for (int j = 0; buttons[i][j] != '\0'; j++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, buttons[i][j]);
        }
    }
}

void drawLoadGame() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(screenWidth / 2 - 100, screenHeight / 2 + 100);
    if (deleteMode) {
        const char* title = "DELETE SAVES";
        for (int i = 0; title[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
        }
    } else {
        const char* title = "LOAD GAME";
        for (int i = 0; title[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
        }
    }

    if (numSaves == 0) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2i(screenWidth / 2 - 50, screenHeight / 2 - 20);
        const char* noSaves = "No saves found";
        for (int i = 0; noSaves[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, noSaves[i]);
        }
    } else {
        for (int i = 0; i < numSaves; i++) {
            if (i == selectedSaveItem) {
                if (deleteMode) {
                    glColor3f(1.0f, 0.3f, 0.3f); // Czerwony w trybie usuwania
                } else {
                    glColor3f(1.0f, 1.0f, 0.0f); // Żółty w trybie normalnym
                }
            } else {
                glColor3f(1.0f, 1.0f, 1.0f);
            }
            char saveText[256];
            struct tm* tm_info = localtime(&saveList[i].saveTime);
            char timeStr[26];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);
            snprintf(saveText, sizeof(saveText), "%s (%s)", saveList[i].filename, timeStr);
            glRasterPos2i(screenWidth / 2 - 150, screenHeight / 2 - 20 - i * 40);
            for (int j = 0; saveText[j] != '\0'; j++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, saveText[j]);
            }
        }
    }

    int buttonY = screenHeight / 2 - 20 - numSaves * 40;

    if (numSaves + 1 == selectedSaveItem) {
        glColor3f(1.0f, 1.0f, 0.0f);
    } else {
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    glRasterPos2i(screenWidth / 2 - 150, buttonY);
    if (deleteMode) {
        const char* deleteText = "DELETE: ON";
        for (int i = 0; deleteText[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, deleteText[i]);
        }
    } else {
        const char* deleteText = "DELETE";
        for (int i = 0; deleteText[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, deleteText[i]);
        }
    }

    // Przycisk Back
    if (numSaves + 2 == selectedSaveItem) {
        glColor3f(1.0f, 1.0f, 0.0f);
    } else {
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    glRasterPos2i(screenWidth / 2 - 50, buttonY);
    const char* back = "Back";
    for (int i = 0; back[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, back[i]);
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
    } else if (gameState == LOAD_GAME_STATE) {
        drawLoadGame();
    } else if (gameState == DELETE_CONFIRM_STATE) {
        drawDeleteConfirm();
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
            scanSaves();
            selectedSaveItem = 0;
            deleteMode = 0;
            previousState = 0;
            gameState = LOAD_GAME_STATE;
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
            char filename[256];
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char timeStr[26];
            strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S.sav", tm_info);
            snprintf(filename, sizeof(filename), "save_%s", timeStr);
            saveGame(&player, filename);
        } else if (selectedPauseMenuItem == 2) {
            scanSaves();
            selectedSaveItem = 0;
            deleteMode = 0;
            previousState = 3;
            gameState = LOAD_GAME_STATE;
        } else if (selectedPauseMenuItem == 3) {
            previousState = 3;
            gameState = 2;
            selectedSettingsItem = 0;
        } else if (selectedPauseMenuItem == 4) {
            gameState = 0;
            selectedMenuItem = 0;
        }
    } else if (gameState == LOAD_GAME_STATE && key == 13) {
        if (selectedSaveItem < numSaves) {
            if (deleteMode) {
                deleteConfirmItem = selectedSaveItem;
                selectedConfirmButton = 0;
                gameState = DELETE_CONFIRM_STATE;
            } else {
                time_t saveTime;
                loadGame(&player, saveList[selectedSaveItem].filename, &saveTime);
                gameState = 1;
            }
        } else if (selectedSaveItem == numSaves + 1) {
            deleteMode = !deleteMode;
        } else if (selectedSaveItem == numSaves + 2) {
            deleteMode = 0;
            gameState = previousState;
            if (previousState == 0) selectedMenuItem = 0;
            else if (previousState == 3) selectedPauseMenuItem = 0;
        }
    } else if (gameState == DELETE_CONFIRM_STATE && key == 13) {
        if (selectedConfirmButton == 0) {
            deleteSave(deleteConfirmItem);
            gameState = LOAD_GAME_STATE;
            if (selectedSaveItem >= numSaves && numSaves > 0) {
                selectedSaveItem = numSaves - 1;
            } else if (numSaves == 0) {
                selectedSaveItem = 0;
            }
        } else {
            gameState = LOAD_GAME_STATE;
        }
    } else if (gameState == 1) {
        keys[key] = 1;
    }

    if (key == 27) {
        if (gameState == 1) {
            gameState = 3;
            selectedPauseMenuItem = 0;
        } else if (gameState == 2 || gameState == LOAD_GAME_STATE) {
            if (gameState == LOAD_GAME_STATE) {
                deleteMode = 0;
            }
            gameState = previousState;
            if (previousState == 0) selectedMenuItem = 0;
            else if (previousState == 3) selectedPauseMenuItem = 0;
        } else if (gameState == 3) {
            gameState = 1;
            selectedPauseMenuItem = 0;
        } else if (gameState == DELETE_CONFIRM_STATE) {
            gameState = LOAD_GAME_STATE;
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
    } else if (gameState == LOAD_GAME_STATE) {
        int maxItems = numSaves + 3; // saves + delete button + back button
        if (key == GLUT_KEY_UP) {
            selectedSaveItem = (selectedSaveItem - 1 + maxItems) % maxItems;
        } else if (key == GLUT_KEY_DOWN) {
            selectedSaveItem = (selectedSaveItem + 1) % maxItems;
        }
        glutPostRedisplay();
    } else if (gameState == DELETE_CONFIRM_STATE) {
        if (key == GLUT_KEY_LEFT || key == GLUT_KEY_RIGHT) {
            selectedConfirmButton = !selectedConfirmButton;
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
    scanSaves();
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