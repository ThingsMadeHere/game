#pragma once
#include "raylib.h"
#include <string>

enum class MenuState {
    PLAYING,
    MAIN_MENU,
    SETTINGS
};

struct Settings {
    float mouseSensitivity = 0.1f;
    int renderDistance = 1;
    bool showChunkBoundaries = true;
    bool showDebugInfo = true;
    int lightingQuality = 1; // 0=off, 1=low, 2=high
};

class GameMenu {
private:
    MenuState currentState = MenuState::MAIN_MENU;
    Settings settings;
    bool settingsChanged = false;
    
    // UI Elements
    Rectangle playButton = {400, 200, 200, 50};
    Rectangle settingsButton = {400, 270, 200, 50};
    Rectangle exitButton = {400, 340, 200, 50};
    Rectangle backButton = {50, 500, 150, 40};
    
    // Settings sliders
    Rectangle mouseSlider = {400, 200, 200, 20};
    Rectangle renderSlider = {400, 250, 200, 20};
    Rectangle voxelSlider = {400, 300, 200, 20};
    
public:
    void Init();
    MenuState Update();
    void Render();
    void ShowMainMenu();
    void ShowSettings();
    void HandleMainMenu();
    void HandleSettings();
    bool IsInMenu() const { return currentState != MenuState::PLAYING; }
    Settings GetSettings() const { return settings; }
    bool SettingsWereChanged() { bool changed = settingsChanged; settingsChanged = false; return changed; }
};
