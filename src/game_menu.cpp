#include "game_menu.h"
#include <cstdio>
#include <cmath>

void GameMenu::Init() {
    currentState = MenuState::MAIN_MENU;
    settingsChanged = false;
    // Ensure cursor is visible when menu initializes
    EnableCursor();
}

MenuState GameMenu::Update() {
    // Handle ESC key to toggle menu
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (currentState == MenuState::PLAYING) {
            currentState = MenuState::MAIN_MENU;
            EnableCursor(); // Ensure cursor is visible when entering menu
        } else if (currentState == MenuState::MAIN_MENU) {
            currentState = MenuState::PLAYING;
            DisableCursor(); // Hide cursor when returning to game
        } else if (currentState == MenuState::SETTINGS) {
            currentState = MenuState::MAIN_MENU;
        }
    }
    
    if (currentState == MenuState::MAIN_MENU) {
        HandleMainMenu();
    } else if (currentState == MenuState::SETTINGS) {
        HandleSettings();
    }
    
    return currentState;
}

void GameMenu::Render() {
    if (currentState == MenuState::MAIN_MENU) {
        ShowMainMenu();
    } else if (currentState == MenuState::SETTINGS) {
        ShowSettings();
    }
}

void GameMenu::ShowMainMenu() {
    // Full screen dark background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {20, 20, 30, 240});
    
    // Title
    DrawText("VOXEL TERRAIN", GetScreenWidth()/2 - 120, 100, 40, WHITE);
    DrawText("Ultra-Fast 3D World Generator", GetScreenWidth()/2 - 160, 150, 20, GRAY);
    
    // Buttons - centered on screen
    Rectangle playButton = {(float)GetScreenWidth()/2 - 100, 200, 200, 50};
    Rectangle settingsButton = {(float)GetScreenWidth()/2 - 100, 270, 200, 50};
    Rectangle exitButton = {(float)GetScreenWidth()/2 - 100, 340, 200, 50};
    
    Color playColor = CheckCollisionPointRec(GetMousePosition(), playButton) ? YELLOW : WHITE;
    Color settingsColor = CheckCollisionPointRec(GetMousePosition(), settingsButton) ? YELLOW : WHITE;
    Color exitColor = CheckCollisionPointRec(GetMousePosition(), exitButton) ? YELLOW : WHITE;
    
    DrawRectangleLinesEx(playButton, 3, playColor);
    DrawText("PLAY", GetScreenWidth()/2 - 30, 215, 20, playColor);
    
    DrawRectangleLinesEx(settingsButton, 3, settingsColor);
    DrawText("SETTINGS", GetScreenWidth()/2 - 50, 285, 20, settingsColor);
    
    DrawRectangleLinesEx(exitButton, 3, exitColor);
    DrawText("EXIT", GetScreenWidth()/2 - 20, 355, 20, exitColor);
    
    // Instructions
    DrawText("Click buttons to navigate | ESC to close menu", GetScreenWidth()/2 - 150, 450, 16, LIGHTGRAY);
    DrawText("WASD: Move | Mouse: Look | Space/Shift: Up/Down", GetScreenWidth()/2 - 140, 480, 16, LIGHTGRAY);
}

void GameMenu::ShowSettings() {
    // Full screen dark background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {20, 20, 30, 240});
    
    // Title
    DrawText("SETTINGS", GetScreenWidth()/2 - 80, 50, 30, WHITE);
    
    // Mouse Sensitivity
    DrawText("Mouse Sensitivity", GetScreenWidth()/2 - 200, 205, 16, WHITE);
    Rectangle mouseSlider = {(float)GetScreenWidth()/2 - 100, 200, 200, 20};
    DrawRectangleRec(mouseSlider, GRAY);
    float mousePos = mouseSlider.x + (settings.mouseSensitivity / 0.5f) * mouseSlider.width;
    DrawCircle(mousePos, mouseSlider.y + 10, 8, YELLOW);
    char mouseText[32];
    sprintf(mouseText, "%.2f", settings.mouseSensitivity);
    DrawText(mouseText, GetScreenWidth()/2 + 120, 205, 16, WHITE);
    
    // Render Distance
    DrawText("Render Distance", GetScreenWidth()/2 - 200, 255, 16, WHITE);
    Rectangle renderSlider = {(float)GetScreenWidth()/2 - 100, 250, 200, 20};
    DrawRectangleRec(renderSlider, GRAY);
    float renderPos = renderSlider.x + ((float)settings.renderDistance / 3.0f) * renderSlider.width;
    DrawCircle(renderPos, renderSlider.y + 10, 8, YELLOW);
    char renderText[32];
    sprintf(renderText, "%d chunks", settings.renderDistance);
    DrawText(renderText, GetScreenWidth()/2 + 120, 255, 16, WHITE);
    
    // Lighting Quality
    DrawText("Lighting Quality", GetScreenWidth()/2 - 200, 335, 16, WHITE);
    Rectangle qualitySlider = {(float)GetScreenWidth()/2 - 100, 330, 200, 20};
    DrawRectangleRec(qualitySlider, GRAY);
    float qualityPos = qualitySlider.x + ((float)settings.lightingQuality / 2.0f) * qualitySlider.width;
    DrawCircle(qualityPos, qualitySlider.y + 10, 8, YELLOW);
    char qualityText[32];
    sprintf(qualityText, "%d", settings.lightingQuality);
    DrawText(qualityText, GetScreenWidth()/2 + 120, 335, 16, WHITE);
    
    // Back button
    Rectangle backButton = {50, (float)GetScreenHeight() - 60, 150, 40};
    Color backColor = CheckCollisionPointRec(GetMousePosition(), backButton) ? YELLOW : WHITE;
    DrawRectangleLinesEx(backButton, 3, backColor);
    DrawText("BACK", 90, GetScreenHeight() - 50, 16, backColor);
    
    // Instructions
    DrawText("Drag sliders to adjust settings", GetScreenWidth()/2 - 120, 400, 16, LIGHTGRAY);
    DrawText("Changes apply when returning to game", GetScreenWidth()/2 - 130, 425, 16, LIGHTGRAY);
}

void GameMenu::HandleMainMenu() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        
        // Dynamic button positions
        Rectangle playButton = {(float)GetScreenWidth()/2 - 100, 200, 200, 50};
        Rectangle settingsButton = {(float)GetScreenWidth()/2 - 100, 270, 200, 50};
        Rectangle exitButton = {(float)GetScreenWidth()/2 - 100, 340, 200, 50};
        
        if (CheckCollisionPointRec(mouse, playButton)) {
            currentState = MenuState::PLAYING;
        } else if (CheckCollisionPointRec(mouse, settingsButton)) {
            currentState = MenuState::SETTINGS;
        } else if (CheckCollisionPointRec(mouse, exitButton)) {
            // Exit game
            CloseWindow();
        }
    }
}

void GameMenu::HandleSettings() {
    bool changed = false;
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        Rectangle backButton = {50, (float)GetScreenHeight() - 60, 150, 40};
        
        if (CheckCollisionPointRec(mouse, backButton)) {
            currentState = MenuState::MAIN_MENU;
        }
    }
    
    // Handle slider dragging
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        
        // Dynamic slider positions
        Rectangle mouseSlider = {(float)GetScreenWidth()/2 - 100, 200, 200, 20};
        Rectangle renderSlider = {(float)GetScreenWidth()/2 - 100, 250, 200, 20};
        Rectangle voxelSlider = {(float)GetScreenWidth()/2 - 100, 300, 200, 20};
        
        // Mouse sensitivity slider
        if (CheckCollisionPointRec(mouse, mouseSlider)) {
            float relativeX = (mouse.x - mouseSlider.x) / mouseSlider.width;
            settings.mouseSensitivity = relativeX * 0.5f; // 0.0 to 0.5
            settings.mouseSensitivity = fmaxf(0.01f, fminf(0.5f, settings.mouseSensitivity));
            changed = true;
        }
        
        // Render distance slider
        if (CheckCollisionPointRec(mouse, renderSlider)) {
            float relativeX = (mouse.x - renderSlider.x) / renderSlider.width;
            settings.renderDistance = (int)(relativeX * 3.0f); // 0 to 3
            settings.renderDistance = fmaxf(0, fminf(3, settings.renderDistance));
            changed = true;
        }
    }
    
    if (changed) {
        settingsChanged = true;
    }
}
