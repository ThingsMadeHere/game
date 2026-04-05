#include "engine_design_gui.h"
#include <cstdio>
#include <cmath>

EngineDesignGUI::EngineDesignGUI() {
    // Load models for engines
    LoadNERVModel();
    LoadChemicalModel();
    
    // Initialize type button positions (horizontal layout)
    for (int i = 0; i < 5; i++) {
        typeButtons[i] = {90 + i * 95.0f, 110, 90, 40};
    }
    
    // Initialize subvariant button positions
    for (int i = 0; i < 5; i++) {
        fuelButtons[i] = {90, 280 + i * 30.0f, 180, 25};
        nuclearFuelButtons[i] = {90, 280 + i * 30.0f, 180, 25}; // Keep 5 for safety, but only use 3
        reactorTypeButtons[i] = {290, 280 + i * 30.0f, 200, 25};
        ionTypeButtons[i] = {90, 280 + i * 30.0f, 200, 25};
    }
    
    for (int i = 0; i < 4; i++) {
        oxidizerButtons[i] = {290, 280 + i * 30.0f, 180, 25};
        fusionFuelButtons[i] = {90, 280 + i * 30.0f, 180, 25};
    }
    
    for (int i = 0; i < 6; i++) {
        fusionConfigButtons[i] = {290, 280 + i * 30.0f, 200, 25};
    }
}

void EngineDesignGUI::Show() {
    printf("EngineDesignGUI::Show() called\n");
    visible = true;
    currentDesign = {};
    currentDesign.type = EngineType::CHEMICAL;
    CalculatePerformance();
}

void EngineDesignGUI::Hide() {
    visible = false;
}

void EngineDesignGUI::Update() {
    if (!visible) return;
    
    // Handle input
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        
        // Check back button
        if (CheckCollisionPointRec(mouse, backButton)) {
            Hide();
            return;
        }
        
        // Check save button
        if (CheckCollisionPointRec(mouse, saveButton)) {
            SaveDesign();
            return;
        }
        
        // Handle type selection
        for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(mouse, typeButtons[i])) {
                currentDesign.type = static_cast<EngineType>(i);
                CalculatePerformance();
                return;
            }
        }
        
        // Handle subvariant selections
        switch (currentDesign.type) {
            case EngineType::CHEMICAL:
                for (int i = 0; i < 5; i++) {
                    if (CheckCollisionPointRec(mouse, fuelButtons[i])) {
                        selectedFuelIndex = i;
                        currentDesign.fuel = static_cast<ChemicalFuel>(i);
                        CalculatePerformance();
                        return;
                    }
                }
                for (int i = 0; i < 4; i++) {
                    if (CheckCollisionPointRec(mouse, oxidizerButtons[i])) {
                        selectedOxidizerIndex = i;
                        currentDesign.oxidizer = static_cast<ChemicalOxidizer>(i);
                        CalculatePerformance();
                        return;
                    }
                }
                break;
            case EngineType::NUCLEAR:
                for (int i = 0; i < 3; i++) { // Only 3 fissile materials: plutonium, uranium, americium
                    if (CheckCollisionPointRec(mouse, nuclearFuelButtons[i])) {
                        selectedNuclearFuelIndex = i;
                        currentDesign.fissileMaterial = static_cast<NuclearFuel>(i);
                        CalculatePerformance();
                        return;
                    }
                }
                for (int i = 0; i < 5; i++) {
                    if (CheckCollisionPointRec(mouse, reactorTypeButtons[i])) {
                        selectedReactorTypeIndex = i;
                        currentDesign.reactorType = static_cast<NuclearReactorType>(i);
                        CalculatePerformance();
                        return;
                    }
                }
                break;
            case EngineType::ION:
                for (int i = 0; i < 5; i++) {
                    if (CheckCollisionPointRec(mouse, ionTypeButtons[i])) {
                        selectedIonTypeIndex = i;
                        currentDesign.ionType = static_cast<IonDriveType>(i);
                        CalculatePerformance();
                        return;
                    }
                }
                break;
            case EngineType::FUSION:
                for (int i = 0; i < 4; i++) {
                    if (CheckCollisionPointRec(mouse, fusionFuelButtons[i])) {
                        selectedFusionFuelIndex = i;
                        currentDesign.fusionFuel = static_cast<FusionFuel>(i);
                        CalculatePerformance();
                        return;
                    }
                }
                for (int i = 0; i < 6; i++) {
                    if (CheckCollisionPointRec(mouse, fusionConfigButtons[i])) {
                        selectedFusionConfigIndex = i;
                        currentDesign.reactorConfig = static_cast<FusionReactorConfig>(i);
                        CalculatePerformance();
                        return;
                    }
                }
                break;
            case EngineType::ANTIMATTER:
                // Antimatter has no subvariants
                break;
        }
    }
    
    // Handle keyboard shortcuts
    if (IsKeyPressed(KEY_ESCAPE)) {
        Hide();
    }
}

void EngineDesignGUI::Render() {
    if (!visible) return;
    
    if (guiEditMode) {
        RenderGUIEditor();
        return;
    }
    
    DrawMainPanel();
    DrawTypeSelection();
    
    switch (currentDesign.type) {
        case EngineType::CHEMICAL:
            DrawChemicalOptions();
            break;
        case EngineType::NUCLEAR:
            DrawNuclearOptions();
            break;
        case EngineType::ION:
            DrawIonOptions();
            break;
        case EngineType::FUSION:
            DrawFusionOptions();
            break;
        case EngineType::ANTIMATTER:
            DrawAntimatterOptions();
            break;
    }
    
    DrawPerformancePreview();
    DrawEngineModel();
}

void EngineDesignGUI::DrawMainPanel() {
    // Main panel background
    DrawRectangleRec(mainPanel, panelBg);
    DrawRectangleLinesEx(mainPanel, 3, panelBorder);
    
    // Title
    DrawText("ENGINE DESIGN WORKSHOP", mainPanel.x + 30, mainPanel.y + 20, 32, titleColor);
    DrawText("Configure your spacecraft propulsion system", mainPanel.x + 30, mainPanel.y + 60, 16, textColor);
    
    // Draw panel sections backgrounds
    Color sectionBg = {40, 40, 50, 200};
    DrawRectangleRec(typePanel, sectionBg);
    DrawRectangleLinesEx(typePanel, 2, panelBorder);
    DrawText("ENGINE TYPE", typePanel.x + 15, typePanel.y + 10, 18, titleColor);
    
    DrawRectangleRec(configPanel, sectionBg);
    DrawRectangleLinesEx(configPanel, 2, panelBorder);
    
    DrawRectangleRec(modelPanel, sectionBg);
    DrawRectangleLinesEx(modelPanel, 2, panelBorder);
    DrawText("3D PREVIEW", modelPanel.x + 15, modelPanel.y + 10, 18, titleColor);
    
    DrawRectangleRec(performancePanel, sectionBg);
    DrawRectangleLinesEx(performancePanel, 2, panelBorder);
    DrawText("PERFORMANCE METRICS", performancePanel.x + 15, performancePanel.y + 10, 16, titleColor);
    
    // Back button
    Color buttonBg = {60, 60, 80, 180};
    DrawRectangleRec(backButton, buttonBg);
    DrawRectangleLinesEx(backButton, 2, panelBorder);
    DrawText("BACK", backButton.x + 35, backButton.y + 12, 16, titleColor);
    
    // Save button
    DrawRectangleRec(saveButton, buttonBg);
    DrawRectangleLinesEx(saveButton, 2, panelBorder);
    DrawText("SAVE DESIGN", saveButton.x + 20, saveButton.y + 12, 16, titleColor);
}

void EngineDesignGUI::DrawTypeSelection() {
    // Type buttons are already drawn in main panel
    for (int i = 0; i < 5; i++) {
        Color color = (currentDesign.type == static_cast<EngineType>(i)) ? selectedColor : panelBorder;
        Color buttonBg = (currentDesign.type == static_cast<EngineType>(i)) ? Color{80, 80, 100, 200} : Color{60, 60, 80, 180};
        DrawRectangleRec(typeButtons[i], buttonBg);
        DrawRectangleLinesEx(typeButtons[i], 2, color);
        
        const char* typeNames[] = {
            "CHEMICAL", "NUCLEAR", "ION", "FUSION", "ANTIMATTER"
        };
        DrawText(typeNames[i], typeButtons[i].x + 5, typeButtons[i].y + 10, 12, titleColor);
    }
}

void EngineDesignGUI::DrawChemicalOptions() {
    DrawText("Chemical Engine Configuration", configPanel.x + 15, configPanel.y + 10, 18, titleColor);
    
    // Fuel selection
    DrawText("Fuel:", configPanel.x + 15, configPanel.y + 40, 14, titleColor);
    const char* fuels[] = {"RP-1", "LH2", "CH4", "Hydrazine", "UDMH"};
    for (int i = 0; i < 5; i++) {
        Color color = (selectedFuelIndex == i) ? selectedColor : panelBorder;
        Color btnBg = (selectedFuelIndex == i) ? Color{70, 70, 90, 200} : Color{50, 50, 60, 150};
        DrawRectangleRec(fuelButtons[i], btnBg);
        DrawRectangleLinesEx(fuelButtons[i], 1, color);
        DrawText(fuels[i], fuelButtons[i].x + 5, fuelButtons[i].y + 5, 11, titleColor);
    }
    
    // Oxidizer selection
    DrawText("Oxidizer:", configPanel.x + 265, configPanel.y + 40, 14, titleColor);
    const char* oxidizers[] = {"LOX", "N2O4", "N2O", "H2O2"};
    for (int i = 0; i < 4; i++) {
        Color color = (selectedOxidizerIndex == i) ? selectedColor : panelBorder;
        Color btnBg = (selectedOxidizerIndex == i) ? Color{70, 70, 90, 200} : Color{50, 50, 60, 150};
        DrawRectangleRec(oxidizerButtons[i], btnBg);
        DrawRectangleLinesEx(oxidizerButtons[i], 1, color);
        DrawText(oxidizers[i], oxidizerButtons[i].x + 5, oxidizerButtons[i].y + 5, 11, titleColor);
    }
    
    // Update design based on selections
    currentDesign.fuel = static_cast<ChemicalFuel>(selectedFuelIndex);
    currentDesign.oxidizer = static_cast<ChemicalOxidizer>(selectedOxidizerIndex);
}

void EngineDesignGUI::DrawNuclearOptions() {
    DrawText("Nuclear Engine Configuration", configPanel.x + 15, configPanel.y + 10, 18, titleColor);
    
    // Fissile material selection
    DrawText("Fissile Material:", configPanel.x + 15, configPanel.y + 40, 14, titleColor);
    const char* fuels[] = {"Plutonium-239", "Uranium-235", "Americium-241"};
    for (int i = 0; i < 3; i++) { // Only 3 fissile materials now
        Color color = (selectedNuclearFuelIndex == i) ? selectedColor : panelBorder;
        Color btnBg = (selectedNuclearFuelIndex == i) ? Color{70, 70, 90, 200} : Color{50, 50, 60, 150};
        DrawRectangleRec(nuclearFuelButtons[i], btnBg);
        DrawRectangleLinesEx(nuclearFuelButtons[i], 1, color);
        DrawText(fuels[i], nuclearFuelButtons[i].x + 5, nuclearFuelButtons[i].y + 5, 11, titleColor);
    }
    
    // Reactor type selection
    DrawText("Reactor Type:", configPanel.x + 265, configPanel.y + 40, 14, titleColor);
    const char* reactors[] = {"Solid Core", "Liquid Core", "Gas Core", "Pulse", "Fission Fragment"};
    for (int i = 0; i < 5; i++) {
        Color color = (selectedReactorTypeIndex == i) ? selectedColor : panelBorder;
        Color btnBg = (selectedReactorTypeIndex == i) ? Color{70, 70, 90, 200} : Color{50, 50, 60, 150};
        DrawRectangleRec(reactorTypeButtons[i], btnBg);
        DrawRectangleLinesEx(reactorTypeButtons[i], 1, color);
        DrawText(reactors[i], reactorTypeButtons[i].x + 5, reactorTypeButtons[i].y + 5, 10, titleColor);
    }
    
    // Update design
    currentDesign.fissileMaterial = static_cast<NuclearFuel>(selectedNuclearFuelIndex);
    currentDesign.reactorType = static_cast<NuclearReactorType>(selectedReactorTypeIndex);
}

void EngineDesignGUI::DrawIonOptions() {
    DrawText("Ion Engine Configuration", configPanel.x + 15, configPanel.y + 10, 18, titleColor);
    
    // Ion drive type selection
    DrawText("Drive Type:", configPanel.x + 15, configPanel.y + 40, 14, titleColor);
    const char* drives[] = {
        "Gridded Ion", "Hall Effect", "FEEP", "Colloid", "MPD Thruster"
    };
    for (int i = 0; i < 5; i++) {
        Color color = (selectedIonTypeIndex == i) ? selectedColor : panelBorder;
        Color btnBg = (selectedIonTypeIndex == i) ? Color{70, 70, 90, 200} : Color{50, 50, 60, 150};
        DrawRectangleRec(ionTypeButtons[i], btnBg);
        DrawRectangleLinesEx(ionTypeButtons[i], 1, color);
        DrawText(drives[i], ionTypeButtons[i].x + 5, ionTypeButtons[i].y + 5, 10, titleColor);
    }
    
    currentDesign.ionType = static_cast<IonDriveType>(selectedIonTypeIndex);
}

void EngineDesignGUI::DrawFusionOptions() {
    DrawText("Fusion Engine Configuration", configPanel.x + 15, configPanel.y + 10, 18, titleColor);
    
    // Fusion fuel selection
    DrawText("Fusion Fuel:", configPanel.x + 15, configPanel.y + 40, 14, titleColor);
    const char* fuels[] = {"D-T", "D-D", "D-3He", "p-11B"};
    for (int i = 0; i < 4; i++) {
        Color color = (selectedFusionFuelIndex == i) ? selectedColor : panelBorder;
        Color btnBg = (selectedFusionFuelIndex == i) ? Color{70, 70, 90, 200} : Color{50, 50, 60, 150};
        DrawRectangleRec(fusionFuelButtons[i], btnBg);
        DrawRectangleLinesEx(fusionFuelButtons[i], 1, color);
        DrawText(fuels[i], fusionFuelButtons[i].x + 5, fusionFuelButtons[i].y + 5, 11, titleColor);
    }
    
    // Reactor configuration selection
    DrawText("Reactor Config:", configPanel.x + 265, configPanel.y + 40, 14, titleColor);
    const char* configs[] = {
        "Tokamak", "Stellarator", "Inertial", "Magneto-Inertial", 
        "Collapsing", "Field-Reversed"
    };
    for (int i = 0; i < 6; i++) {
        Color color = (selectedFusionConfigIndex == i) ? selectedColor : panelBorder;
        Color btnBg = (selectedFusionConfigIndex == i) ? Color{70, 70, 90, 200} : Color{50, 50, 60, 150};
        DrawRectangleRec(fusionConfigButtons[i], btnBg);
        DrawRectangleLinesEx(fusionConfigButtons[i], 1, color);
        DrawText(configs[i], fusionConfigButtons[i].x + 5, fusionConfigButtons[i].y + 5, 9, titleColor);
    }
    
    currentDesign.fusionFuel = static_cast<FusionFuel>(selectedFusionFuelIndex);
    currentDesign.reactorConfig = static_cast<FusionReactorConfig>(selectedFusionConfigIndex);
}

void EngineDesignGUI::DrawAntimatterOptions() {
    DrawText("Antimatter Engine Configuration", configPanel.x + 15, configPanel.y + 10, 18, titleColor);
    
    DrawText("Type: Beam Core Engine", configPanel.x + 15, configPanel.y + 40, 14, titleColor);
    DrawText("Maximum efficiency propulsion using matter-antimatter annihilation", configPanel.x + 15, configPanel.y + 60, 12, textColor);
    DrawText("Specific Impulse: ~100,000s", configPanel.x + 15, configPanel.y + 80, 12, textColor);
    DrawText("Thrust: 50 kN", configPanel.x + 15, configPanel.y + 100, 12, textColor);
}

void EngineDesignGUI::DrawPerformancePreview() {
    // Performance panel
    Color sectionBg = {40, 40, 50, 200};
    DrawRectangleRec(performancePanel, sectionBg);
    DrawRectangleLinesEx(performancePanel, 2, panelBorder);
    
    DrawText("PERFORMANCE METRICS", performancePanel.x + 15, performancePanel.y + 10, 16, titleColor);
    
    // Performance data
    char ispText[64];
    sprintf(ispText, "Specific Impulse: %.1f s", currentDesign.specificImpulse);
    DrawText(ispText, performancePanel.x + 15, performancePanel.y + 35, 14, textColor);
    
    char thrustText[64];
    sprintf(thrustText, "Thrust: %.1f kN", currentDesign.thrust / 1000.0f);
    DrawText(thrustText, performancePanel.x + 15, performancePanel.y + 55, 14, textColor);
    
    char massFlowText[64];
    sprintf(massFlowText, "Mass Flow: %.3f kg/s", currentDesign.massFlow);
    DrawText(massFlowText, performancePanel.x + 15, performancePanel.y + 75, 14, textColor);
    
    char efficiencyText[64];
    sprintf(efficiencyText, "Efficiency: %.1f%%", currentDesign.efficiency * 100.0f);
    DrawText(efficiencyText, performancePanel.x + 15, performancePanel.y + 95, 14, textColor);
}

void EngineDesignGUI::CalculatePerformance() {
    switch (currentDesign.type) {
        case EngineType::CHEMICAL: {
            // Chemical rocket performance based on fuel/oxidizer combination
            float ispBase = 250.0f; // Base Isp in seconds
            
            // Adjust for fuel type
            switch (currentDesign.fuel) {
                case ChemicalFuel::RP1: ispBase += 50; break;
                case ChemicalFuel::LH2: ispBase += 350; break;
                case ChemicalFuel::CH4: ispBase += 100; break;
                case ChemicalFuel::HYDRAZINE: ispBase += 200; break;
                case ChemicalFuel::UDMH: ispBase += 180; break;
            }
            
            // Adjust for oxidizer type
            switch (currentDesign.oxidizer) {
                case ChemicalOxidizer::LOX: ispBase += 50; break;
                case ChemicalOxidizer::N2O4: ispBase += 20; break;
                case ChemicalOxidizer::N2O: ispBase -= 30; break;
                case ChemicalOxidizer::H2O2: ispBase += 10; break;
            }
            
            currentDesign.specificImpulse = ispBase;
            currentDesign.thrust = 1000.0f; // 1 MN base thrust
            currentDesign.efficiency = 0.85f;
            break;
        }
        
        case EngineType::NUCLEAR: {
            // Nuclear thermal rocket performance
            float ispBase = 800.0f;
            
            // Restrict americium to fission fragment engines only
            if (currentDesign.fissileMaterial == NuclearFuel::AMERICIUM && 
                currentDesign.reactorType != NuclearReactorType::FISSION_FRAGMENT) {
                // Invalid combination - set to minimal performance
                currentDesign.specificImpulse = 0.0f;
                currentDesign.thrust = 0.0f;
                currentDesign.efficiency = 0.0f;
                return;
            }
            
            switch (currentDesign.reactorType) {
                case NuclearReactorType::SOLID_CORE: ispBase = 850.0f; break;
                case NuclearReactorType::LIQUID_CORE: ispBase = 1200.0f; break;
                case NuclearReactorType::GAS_CORE: ispBase = 2000.0f; break;
                case NuclearReactorType::PULSE: ispBase = 5000.0f; break; // Orion
                case NuclearReactorType::FISSION_FRAGMENT: ispBase = 1000000.0f; break; // 1M ISP
            }
            
            // Adjust for fuel type (except americium which is already handled)
            // Set default values first
            currentDesign.specificImpulse = ispBase;
            currentDesign.thrust = 500.0f; // 500 kN base thrust
            currentDesign.efficiency = 0.75f; // Base efficiency
            
            if (currentDesign.fissileMaterial == NuclearFuel::PLUTONIUM) {
                currentDesign.specificImpulse *= 1.1f; // Plutonium is slightly better
                currentDesign.efficiency = 0.75f; // Standard efficiency
            } else if (currentDesign.fissileMaterial == NuclearFuel::URANIUM) {
                currentDesign.specificImpulse *= 1.05f; // Uranium gives moderate ISP bonus
                currentDesign.efficiency = 0.85f; // Higher efficiency than plutonium
                currentDesign.thrust = 600.0f; // Higher thrust than others (500kN base)
            } else if (currentDesign.fissileMaterial == NuclearFuel::AMERICIUM) {
                currentDesign.efficiency = 0.95f; // Highest efficiency for fission fragment
            }
        }
        
        case EngineType::ION: {
            // Ion thruster performance
            float ispBase = 3000.0f;
            
            switch (currentDesign.ionType) {
                case IonDriveType::GRIDDED: ispBase = 3500.0f; break;
                case IonDriveType::HALL_EFFECT: ispBase = 2500.0f; break;
                case IonDriveType::FEEP: ispBase = 8000.0f; break;
                case IonDriveType::COLLOID: ispBase = 1500.0f; break;
                case IonDriveType::MPD: ispBase = 5000.0f; break;
            }
            
            currentDesign.specificImpulse = ispBase;
            currentDesign.thrust = 5.0f; // Low thrust, high Isp
            currentDesign.efficiency = 0.65f;
            break;
        }
        
        case EngineType::FUSION: {
            // Fusion rocket performance
            float ispBase = 10000.0f;
            
            switch (currentDesign.fusionFuel) {
                case FusionFuel::DT: ispBase = 10000.0f; break;
                case FusionFuel::DD: ispBase = 8000.0f; break;
                case FusionFuel::D3HE: ispBase = 15000.0f; break;
                case FusionFuel::P11B: ispBase = 20000.0f; break;
            }
            
            currentDesign.specificImpulse = ispBase;
            currentDesign.thrust = 100.0f; // 100 kN base
            currentDesign.efficiency = 0.45f;
            break;
        }
        
        case EngineType::ANTIMATTER: {
            // Antimatter beam core - highest performance
            currentDesign.specificImpulse = 100000.0f; // ~10,000 km/s exhaust velocity
            currentDesign.thrust = 50.0f; // 50 kN
            currentDesign.efficiency = 0.95f;
            break;
        }
    }
    
    // Calculate mass flow from thrust and Isp
    if (currentDesign.specificImpulse > 0) {
        currentDesign.massFlow = currentDesign.thrust * 1000.0f / (currentDesign.specificImpulse * 9.81f);
    } else {
        currentDesign.massFlow = 0.0f;
    }
}

void EngineDesignGUI::SaveDesign() {
    // Generate name based on configuration
    std::string name = GetEngineTypeName(currentDesign.type);
    
    switch (currentDesign.type) {
        case EngineType::CHEMICAL:
            name += " " + std::string(GetFuelName(currentDesign.fuel)) + "/" + std::string(GetOxidizerName(currentDesign.oxidizer));
            break;
        case EngineType::NUCLEAR:
            name += " " + std::string(GetReactorTypeName(currentDesign.reactorType));
            break;
        case EngineType::ION:
            name += " " + std::string(GetIonTypeName(currentDesign.ionType));
            break;
        case EngineType::FUSION:
            name += " " + std::string(GetFusionFuelName(currentDesign.fusionFuel));
            break;
        case EngineType::ANTIMATTER:
            name += " Beam Core";
            break;
    }
    
    currentDesign.name = name;
    
    // Call callback if set
    if (designCompleteCallback) {
        designCompleteCallback(currentDesign);
    }
    
    Hide();
}

void EngineDesignGUI::SetDesignCompleteCallback(std::function<void(const EngineDesign&)> callback) {
    designCompleteCallback = callback;
}

// Helper function implementations
const char* EngineDesignGUI::GetEngineTypeName(EngineType type) {
    switch (type) {
        case EngineType::CHEMICAL: return "Chemical";
        case EngineType::NUCLEAR: return "Nuclear";
        case EngineType::ION: return "Ion";
        case EngineType::FUSION: return "Fusion";
        case EngineType::ANTIMATTER: return "Antimatter";
        default: return "Unknown";
    }
}

const char* EngineDesignGUI::GetFuelName(ChemicalFuel fuel) {
    switch (fuel) {
        case ChemicalFuel::RP1: return "RP-1";
        case ChemicalFuel::LH2: return "LH2";
        case ChemicalFuel::CH4: return "CH4";
        case ChemicalFuel::HYDRAZINE: return "Hydrazine";
        case ChemicalFuel::UDMH: return "UDMH";
        default: return "Unknown";
    }
}

const char* EngineDesignGUI::GetOxidizerName(ChemicalOxidizer oxidizer) {
    switch (oxidizer) {
        case ChemicalOxidizer::LOX: return "LOX";
        case ChemicalOxidizer::N2O4: return "N2O4";
        case ChemicalOxidizer::N2O: return "N2O";
        case ChemicalOxidizer::H2O2: return "H2O2";
        default: return "Unknown";
    }
}

const char* EngineDesignGUI::GetNuclearFuelName(NuclearFuel fuel) {
    switch (fuel) {
        case NuclearFuel::PLUTONIUM: return "Plutonium-239";
        case NuclearFuel::URANIUM: return "Uranium-235";
        case NuclearFuel::AMERICIUM: return "Americium-241";
        default: return "Unknown";
    }
}

const char* EngineDesignGUI::GetReactorTypeName(NuclearReactorType type) {
    switch (type) {
        case NuclearReactorType::SOLID_CORE: return "Solid Core";
        case NuclearReactorType::LIQUID_CORE: return "Liquid Core";
        case NuclearReactorType::GAS_CORE: return "Gas Core";
        case NuclearReactorType::PULSE: return "Pulse";
        case NuclearReactorType::FISSION_FRAGMENT: return "Fission Fragment";
        default: return "Unknown";
    }
}

const char* EngineDesignGUI::GetIonTypeName(IonDriveType type) {
    switch (type) {
        case IonDriveType::GRIDDED: return "Gridded";
        case IonDriveType::HALL_EFFECT: return "Hall Effect";
        case IonDriveType::FEEP: return "FEEP";
        case IonDriveType::COLLOID: return "Colloid";
        case IonDriveType::MPD: return "MPD";
        default: return "Unknown";
    }
}

const char* EngineDesignGUI::GetFusionFuelName(FusionFuel fuel) {
    switch (fuel) {
        case FusionFuel::DT: return "D-T";
        case FusionFuel::DD: return "D-D";
        case FusionFuel::D3HE: return "D-3He";
        case FusionFuel::P11B: return "p-11B";
        case FusionFuel::PB11: return "p-11B";
        default: return "Unknown";
    }
}

const char* EngineDesignGUI::GetFusionConfigName(FusionReactorConfig config) {
    switch (config) {
        case FusionReactorConfig::TOKAMAK: return "Tokamak";
        case FusionReactorConfig::STELLARATOR: return "Stellarator";
        case FusionReactorConfig::INERTIAL: return "Inertial";
        case FusionReactorConfig::MAGNETO_INERTIAL: return "Magneto-Inertial";
        case FusionReactorConfig::COLLAPSING: return "Collapsing";
        case FusionReactorConfig::FIELD_REVERSED: return "Field-Reversed";
        default: return "Unknown";
    }
}

void EngineDesignGUI::DrawEngineModel() {
    printf("DrawEngineModel called - engine type: %d, reactor type: %d\n", (int)currentDesign.type, (int)currentDesign.reactorType);
    
    // 3D model viewport is already drawn in main panel
    
    // Simple 3D camera for model view - centered on model
    Camera3D modelCamera = {0};
    modelCamera.position = {0, 0, 5};
    modelCamera.target = {0, 0, 0};
    modelCamera.up = {0, 1, 0};
    modelCamera.fovy = 45.0f;
    modelCamera.projection = CAMERA_PERSPECTIVE;
    
    // Set viewport and draw 3D model
    BeginMode3D(modelCamera);
    
    // Update model rotation for animation
    modelRotation.y += 1.0f; // Slow rotation
    
    // Draw simple engine representation based on type
    DrawSimpleEngineModel(currentDesign.type, {0, 0, 0}, 1.0f, LoadMaterialDefault(), {0, 0, 0});
    
    EndMode3D();
}

void EngineDesignGUI::DrawSimpleEngineModel(EngineType type, Vector3 position, float scale, Material material, Vector3 lightPos) {
    Vector3 pos = position;
    
    switch (type) {
        case EngineType::CHEMICAL: {
            // Use chemical GLB model if available
            if (chemicalModel.meshCount > 0) {
                printf("Drawing Chemical GLB model\n");
                DrawModelEx(chemicalModel, 
                           {1.2f, -0.7f, 0.0f},      // Centered position
                           {0, 1, 0},           // Rotation axis (Y-axis)
                           modelRotation.y,     // Rotation angle
                           {0.8f, 0.8f, 0.8f}, // Scale (increased from 0.4f)
                           WHITE);              // Color
            } else {
                // Fallback bell nozzle shape
                Color engineColor = {150, 150, 150, 255}; // Gray metal
                DrawCylinder(pos, 0.3f * scale, 0.8f * scale, 1.0f * scale, 16, engineColor); // Main body
                DrawCylinder({pos.x, pos.y - 0.5f, pos.z}, 0.2f * scale, 0.3f * scale, 0.5f * scale, 16, engineColor); // Top
                // Nozzle
                DrawCylinder({pos.x, pos.y - 0.8f, pos.z}, 0.8f * scale, 0.3f * scale, 0.3f * scale, 16, {100, 100, 100, 255});
            }
            break;
        }
        case EngineType::NUCLEAR: {
            // Reactor chamber with nozzle
            Color reactorColor = {255, 100, 100, 255}; // Red hot
            
            printf("Nuclear engine type, reactor type: %d\n", (int)currentDesign.reactorType);
            
            // Use NERV model for solid core nuclear
            if (currentDesign.reactorType == NuclearReactorType::SOLID_CORE && nervModel.meshCount > 0) {
                printf("Drawing NERV GLB model for solid core nuclear\n");
                // Apply rotation and position model at top right
                DrawModelEx(nervModel, 
                           {1.2f, -0.7f, 0.0f},       // Top right position
                           {0, 1, 0},           // Rotation axis (Y-axis)
                           modelRotation.y,     // Rotation angle
                           {0.3f, 0.3f, 0.3f}, // Scale
                           WHITE);              // Color
            } else {
                printf("Using fallback shapes for nuclear engine\n");
                // Fallback to simple shapes
                DrawSphere({0, 0, 0}, 0.5f * scale, reactorColor); // Reactor core
                DrawCylinder({0, -0.3f, 0}, 0.4f * scale, 0.6f * scale, 0.6f * scale, 16, {150, 150, 150, 255}); // Chamber
                DrawCylinder({0, -0.8f, 0}, 0.6f * scale, 0.4f * scale, 0.2f * scale, 16, {100, 100, 100, 255}); // Nozzle
            }
            break;
        }
        case EngineType::ION: {
            // Compact thruster with grids
            Color ionColor = {100, 200, 255, 255}; // Blue
            DrawCube({pos.x, pos.y, pos.z}, 0.4f * scale, 0.3f * scale, 0.2f * scale, ionColor); // Main body
            // Grid plates
            DrawCube({pos.x, pos.y, pos.z + 0.1f * scale}, 0.3f * scale, 0.25f * scale, 0.02f * scale, {200, 200, 200, 255});
            DrawCube({pos.x, pos.y, pos.z - 0.1f * scale}, 0.3f * scale, 0.25f * scale, 0.02f * scale, {200, 200, 200, 255});
            break;
        }
        case EngineType::FUSION: {
            // Toroidal chamber
            Color fusionColor = {255, 255, 100, 255}; // Yellow plasma
            // Draw torus using multiple spheres (simplified)
            for (int i = 0; i < 16; i++) {
                float angle = (i / 16.0f) * 2.0f * PI;
                Vector3 spherePos = {
                    pos.x + cosf(angle) * 0.6f * scale,
                    pos.y,
                    pos.z + sinf(angle) * 0.6f * scale
                };
                DrawSphere(spherePos, 0.15f * scale, fusionColor);
            }
            // Central chamber
            DrawSphere(pos, 0.3f * scale, {200, 200, 200, 255});
            break;
        }
        case EngineType::ANTIMATTER: {
            // Sleek, advanced design
            Color antimatterColor = {255, 100, 255, 255}; // Purple
            DrawCylinder(pos, 0.2f * scale, 0.4f * scale, 1.2f * scale, 16, antimatterColor); // Main chamber
            // Magnetic coils
            for (int i = 0; i < 4; i++) {
                float y = pos.y - 0.3f + i * 0.2f;
                DrawCylinder({pos.x, y, pos.z}, 0.5f * scale, 0.5f * scale, 0.05f * scale, 16, {100, 100, 255, 255});
            }
            // Exhaust nozzle
            DrawCylinder({pos.x, pos.y - 0.8f, pos.z}, 0.3f * scale, 0.2f * scale, 0.4f * scale, 16, {150, 150, 150, 255});
            break;
        }
    }
}

void EngineDesignGUI::UpdateGUIEditor() {
    Vector2 mouse = GetMousePosition();
    
    // Handle element dragging
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // Check all draggable elements
        Rectangle* elements[] = {
            &mainPanel, &typePanel, &configPanel, &modelPanel, &performancePanel,
            &backButton, &saveButton
        };
        
        for (auto element : elements) {
            if (CheckCollisionPointRec(mouse, *element)) {
                draggedElement = element;
                dragOffset = {mouse.x - element->x, mouse.y - element->y};
                break;
            }
        }
    }
    
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && draggedElement) {
        draggedElement->x = mouse.x - dragOffset.x;
        draggedElement->y = mouse.y - dragOffset.y;
    }
    
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (draggedElement) {
            printf("Element moved to: x=%.1f, y=%.1f, w=%.1f, h=%.1f\n", 
                   draggedElement->x, draggedElement->y, draggedElement->width, draggedElement->height);
        }
        draggedElement = nullptr;
    }
    
    // Print positions on P key press
    if (IsKeyPressed(KEY_P)) {
        PrintElementPositions();
    }
}

void EngineDesignGUI::RenderGUIEditor() {
    // Draw editor overlay
    DrawRectangle(0, 0, 800, 600, {0, 0, 0, 100}); // Dark overlay
    
    // Highlight draggable elements
    Rectangle elements[] = {
        mainPanel, typePanel, configPanel, modelPanel, performancePanel,
        backButton, saveButton
    };
    
    for (auto element : elements) {
        DrawRectangleLinesEx(element, 2, YELLOW); // Yellow outline
        DrawText("DRAG ME", element.x + 5, element.y + 5, 10, WHITE);
    }
    
    // Instructions
    DrawText("GUI EDITOR MODE", 10, 10, 20, YELLOW);
    DrawText("F1: Exit Editor", 10, 40, 15, WHITE);
    DrawText("P: Print Positions", 10, 60, 15, WHITE);
    DrawText("Drag elements to reposition", 10, 80, 15, WHITE);
}

void EngineDesignGUI::PrintElementPositions() {
    printf("\n=== GUI ELEMENT POSITIONS ===\n");
    printf("mainPanel: {%.1f, %.1f, %.1f, %.1f}\n", mainPanel.x, mainPanel.y, mainPanel.width, mainPanel.height);
    printf("typePanel: {%.1f, %.1f, %.1f, %.1f}\n", typePanel.x, typePanel.y, typePanel.width, typePanel.height);
    printf("configPanel: {%.1f, %.1f, %.1f, %.1f}\n", configPanel.x, configPanel.y, configPanel.width, configPanel.height);
    printf("modelPanel: {%.1f, %.1f, %.1f, %.1f}\n", modelPanel.x, modelPanel.y, modelPanel.width, modelPanel.height);
    printf("performancePanel: {%.1f, %.1f, %.1f, %.1f}\n", performancePanel.x, performancePanel.y, performancePanel.width, performancePanel.height);
    printf("backButton: {%.1f, %.1f, %.1f, %.1f}\n", backButton.x, backButton.y, backButton.width, backButton.height);
    printf("saveButton: {%.1f, %.1f, %.1f, %.1f}\n", saveButton.x, saveButton.y, saveButton.width, saveButton.height);
    printf("============================\n\n");
}

void EngineDesignGUI::LoadChemicalModel() {
    // Load chemical engine model from GLB file
    printf("Attempting to load Chemical model from: ../data/models/Chemical.glb\n");
    chemicalModel = LoadModel("../data/models/Chemical.glb");
    
    if (chemicalModel.meshCount > 0) {
        printf("Chemical GLB model loaded successfully - mesh count: %d\n", chemicalModel.meshCount);
        
        // Calculate bounding box to find model dimensions
        chemicalModelBounds = GetMeshBoundingBox(chemicalModel.meshes[0]);
        float modelHeight = chemicalModelBounds.max.y - chemicalModelBounds.min.y;
        printf("Chemical model bounds - min: (%.2f, %.2f, %.2f), max: (%.2f, %.2f, %.2f), height: %.2f\n",
               chemicalModelBounds.min.x, chemicalModelBounds.min.y, chemicalModelBounds.min.z,
               chemicalModelBounds.max.x, chemicalModelBounds.max.y, chemicalModelBounds.max.z,
               modelHeight);
    } else {
        printf("Failed to load Chemical GLB model - mesh count: %d\n", chemicalModel.meshCount);
    }
}

void EngineDesignGUI::LoadNERVModel() {
    // Load NERV model from GLB file
    printf("Attempting to load NERV model from: ../data/models/NERV.glb\n");
    nervModel = LoadModel("../data/models/NERV.glb");
    
    if (nervModel.meshCount > 0) {
        printf("NERV GLB model loaded successfully - mesh count: %d\n", nervModel.meshCount);
        // Model will be drawn at appropriate scale in DrawSimpleEngineModel
    } else {
        printf("Failed to load NERV GLB model - mesh count: %d\n", nervModel.meshCount);
    }
}
