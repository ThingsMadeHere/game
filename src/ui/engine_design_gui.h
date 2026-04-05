#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include <functional>

enum class EngineType {
    CHEMICAL,
    NUCLEAR,
    ION,
    FUSION,
    ANTIMATTER
};

enum class ChemicalFuel {
    RP1,        // Rocket Propellant 1 (kerosene)
    LH2,        // Liquid Hydrogen
    CH4,        // Methane
    HYDRAZINE,  // Hydrazine
    UDMH        // Unsymmetrical Dimethylhydrazine
};

enum class ChemicalOxidizer {
    LOX,        // Liquid Oxygen
    N2O4,       // Nitrogen Tetroxide
    N2O,        // Nitrous Oxide
    H2O2        // Hydrogen Peroxide
};

enum class NuclearFuel {
    U235,       // Uranium-235
    U238,       // Uranium-238
    PU239,      // Plutonium-239
    TH232       // Thorium-232
};

enum class NuclearReactorType {
    SOLID_CORE,     // NERVA-style
    LIQUID_CORE,    // Nuclear thermal rocket with liquid fuel
    GAS_CORE,       // Open cycle gas core
    PULSE,          // Orion-style pulse nuclear
    FISSION_FRAGMENT // Direct fission fragment drive
};

enum class IonDriveType {
    GRIDDED,        // Traditional gridded ion thruster
    HALL_EFFECT,    // Hall effect thruster
    FEEP,           // Field Emission Electric Propulsion
    COLLOID,        // Colloid thruster
    MPD             // Magnetoplasmadynamic thruster
};

enum class FusionFuel {
    DT,             // Deuterium-Tritium
    DD,             // Deuterium-Deuterium
    D3HE,           // Deuterium-Helium-3
    P11B,           // Proton-Boron-11
    PB11            // Proton-Boron-11 (alternative)
};

enum class FusionReactorConfig {
    TOKAMAK,        // Magnetic confinement tokamak
    STELLARATOR,    // Magnetic confinement stellarator
    INERTIAL,       // Inertial confinement
    MAGNETO_INERTIAL, // Magneto-inertial confinement
    COLLAPSING,     // Collapsing linear geometry
    FIELD_REVERSED  // Field-reversed configuration
};

struct EngineDesign {
    EngineType type;
    
    // Chemical engine parameters
    ChemicalFuel fuel;
    ChemicalOxidizer oxidizer;
    
    // Nuclear engine parameters
    NuclearFuel fissileMaterial;
    NuclearReactorType reactorType;
    
    // Ion engine parameters
    IonDriveType ionType;
    
    // Fusion engine parameters
    FusionFuel fusionFuel;
    FusionReactorConfig reactorConfig;
    
    // Calculated performance
    float specificImpulse;    // seconds
    float thrust;              // kN
    float massFlow;            // kg/s
    float efficiency;         // 0-1
    
    std::string name;
};

class EngineDesignGUI {
public:
    EngineDesignGUI();
    void Show();
    void Hide();
    bool IsVisible() const { return visible; }
    void Update();
    void Render();
    
    // Callback for when design is complete
    void SetDesignCompleteCallback(std::function<void(const EngineDesign&)> callback);
    
private:
    bool visible = false;
    EngineType currentType = EngineType::CHEMICAL;
    EngineDesign currentDesign;
    
    // UI state
    int selectedFuelIndex = 0;
    int selectedOxidizerIndex = 0;
    int selectedNuclearFuelIndex = 0;
    int selectedReactorTypeIndex = 0;
    int selectedIonTypeIndex = 0;
    int selectedFusionFuelIndex = 0;
    int selectedFusionConfigIndex = 0;
    
    // UI layout
    Rectangle mainPanel = {50, 50, 1100, 650};
    Rectangle typeButtons[5];
    Rectangle backButton = {60, 660, 120, 40};
    Rectangle saveButton = {980, 660, 120, 40};
    
    // Panel sections
    Rectangle typePanel = {70, 90, 500, 120};
    Rectangle configPanel = {70, 230, 500, 350};
    Rectangle modelPanel = {590, 90, 520, 490};
    Rectangle performancePanel = {590, 600, 520, 80};
    
    // Subvariant button rectangles
    Rectangle fuelButtons[5];
    Rectangle oxidizerButtons[4];
    Rectangle nuclearFuelButtons[4];
    Rectangle reactorTypeButtons[5];
    Rectangle ionTypeButtons[5];
    Rectangle fusionFuelButtons[4];
    Rectangle fusionConfigButtons[6];
    
    // 3D model
    Model engineModel = {0};
    Vector3 modelRotation = {0, 0, 0};
    float modelScale = 1.0f;
    
    // UI colors
    Color panelBg = {30, 30, 40, 240};
    Color panelBorder = {100, 100, 120, 255};
    Color selectedColor = {255, 200, 0, 255}; // Gold
    Color hoverColor = {100, 150, 255, 255}; // Light blue
    Color textColor = {220, 220, 220, 255};
    Color titleColor = {255, 255, 255, 255};
    
    std::function<void(const EngineDesign&)> designCompleteCallback;
    
    void DrawMainPanel();
    void DrawTypeSelection();
    void DrawChemicalOptions();
    void DrawNuclearOptions();
    void DrawIonOptions();
    void DrawFusionOptions();
    void DrawAntimatterOptions();
    void DrawPerformancePreview();
    void DrawEngineModel();
    void UpdateEngineModel();
    
    void CalculatePerformance();
    void SaveDesign();
    
    // Helper functions
    const char* GetEngineTypeName(EngineType type);
    const char* GetFuelName(ChemicalFuel fuel);
    const char* GetOxidizerName(ChemicalOxidizer oxidizer);
    const char* GetNuclearFuelName(NuclearFuel fuel);
    const char* GetReactorTypeName(NuclearReactorType type);
    const char* GetIonTypeName(IonDriveType type);
    const char* GetFusionFuelName(FusionFuel fuel);
    const char* GetFusionConfigName(FusionReactorConfig config);
    
    // 3D model functions
    void LoadEngineModel(EngineType type);
    void DrawSimpleEngineModel(EngineType type, Vector3 position, float scale);
};
