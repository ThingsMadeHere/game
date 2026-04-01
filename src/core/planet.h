#pragma once
#include "raylib.h"
#include "../terrain/noise.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <map>

// ============================================================================
// CHEMICAL DATABASE SYSTEM
// Players construct worlds using chemicals from this database
// ============================================================================

struct ChemicalDefinition {
    std::string id = "";              // Unique ID (e.g., "H2O", "O2", "SiO2")
    std::string name = "";            // Display name
    std::string formula = "";         // Chemical formula
    
    // Thermodynamic Properties
    double molarMass = 0.0;           // g/mol
    double meltingPoint = 0.0;        // Kelvin
    double boilingPoint = 0.0;        // Kelvin
    double density = 0.0;             // kg/m³ (at STP or standard state)
    double gibbsFreeEnergy = 0.0;     // kJ/mol (Standard Gibbs Free Energy of Formation)
    double specificHeat = 0.0;        // J/(mol·K)
    double thermalConductivity = 0.0; // W/(m·K)
    
    // Phase Information
    int phaseAtSTP = 1;               // 0: Solid, 1: Liquid, 2: Gas
    double criticalTemperature = 0.0; // K (for gases)
    double criticalPressure = 0.0;    // atm (for gases)
    
    // Visual Representation
    std::string colorHex = "#FFFFFF"; // For rendering fluids/gas
    float emissiveIntensity = 0.0f;   // For glowing materials
    
    // Gameplay Properties
    bool isToxic = false;
    bool isFlammable = false;
    bool isCorrosive = false;
    bool isNutrient = false;
    bool isRadioactive = false;
    float radiationLevel = 0.0f;
    
    // Reactivity (for future chemistry system)
    std::vector<std::string> reactsWith;
    std::string reactionProduct;
    
    // Rarity and availability
    float cosmicAbundance = 0.0f;
    std::vector<std::string> formationConditions;
};

class ChemicalDatabase {
private:
    std::map<std::string, ChemicalDefinition> m_chemicals;
    
public:
    static ChemicalDatabase& Instance();
    
    void LoadDefaults();
    bool LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path);
    const ChemicalDefinition* GetChemical(const std::string& id) const;
    const ChemicalDefinition* GetChemicalByFormula(const std::string& formula) const;
    const std::map<std::string, ChemicalDefinition>& GetAllChemicals() const;
    
    std::vector<const ChemicalDefinition*> GetChemicalsInPhase(int phase, double temperature) const;
    std::vector<const ChemicalDefinition*> GetChemicalsWithTag(const std::string& tag) const;
    int GetPhaseAtTemp(const std::string& id, double tempKelvin) const;
    bool HasChemical(const std::string& id) const;
};

#define CHEMICAL_DB ChemicalDatabase::Instance()

// ============================================================================
// Planet System - Data-driven planet generation
// ============================================================================

struct OrbitalParameters {
    float semiMajorAxis = 1000.0f;
    float eccentricity = 0.0f;
    float inclination = 0.0f;
    float longitudeOfAscendingNode = 0.0f;
    float argumentOfPeriapsis = 0.0f;
    float meanAnomalyAtEpoch = 0.0f;
    float orbitalPeriod = 365.0f;
    float currentOrbitalAngle = 0.0f;
    std::string parentObjectId = "";
};

struct PhysicalParameters {
    float radius = 50.0f;
    float mass = 1.0f;
    float density = 5.5f;
    float surfaceGravity = 9.81f;
    float escapeVelocity = 11.2f;
    float rotationalPeriod = 24.0f;
    float axialTilt = 23.5f;
    float currentRotationAngle = 0.0f;
};

struct AtmosphericCompositionEntry {
    std::string chemicalId = "";
    float percentage = 0.0f;
    float scaleHeightMultiplier = 1.0f;
};

struct SurfaceCompositionEntry {
    std::string chemicalId = "";
    float concentration = 0.0f;
    std::string distributionType = "uniform";
    float minAltitude = -1000.0f;
    float maxAltitude = 1000.0f;
    float minTemperature = 0.0f;
    float maxTemperature = 10000.0f;
    float minPressure = 0.0f;
    float maxPressure = 1000.0f;
    float veinSize = 1.0f;
    float veinDensity = 0.5f;
};

struct NoiseLayer {
    std::string name = "base";
    float amplitude = 1.0f;
    float frequency = 0.02f;
    int octaves = 6;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float offset = 0.0f;
    std::string blendMode = "add";
    float blendFactor = 1.0f;
};

struct TerrainParameters {
    std::vector<NoiseLayer> noiseLayers;
    float baseHeight = 0.0f;
    float heightScale = 1.0f;
    float waterLevel = 0.3f;
    std::string waterChemicalId = "H2O";
    float cliffThreshold = 0.7f;
    float beachWidth = 0.05f;
    float deepOceanThreshold = 0.2f;
    float shallowOceanThreshold = 0.3f;
    float beachThreshold = 0.35f;
    float plainsThreshold = 0.5f;
    float hillsThreshold = 0.7f;
    float mountainThreshold = 0.85f;
    std::vector<SurfaceCompositionEntry> crustComposition;
    std::vector<SurfaceCompositionEntry> mantleComposition;
    std::vector<SurfaceCompositionEntry> coreComposition;
};

struct EcosystemObject {
    std::string id = "";
    std::string type = "";
    std::string name = "";
    std::string modelPath = "";
    std::string texturePath = "";
    float minAltitude = -1000.0f;
    float maxAltitude = 1000.0f;
    float minTemperature = 0.0f;
    float maxTemperature = 10000.0f;
    float minMoisture = 0.0f;
    float maxMoisture = 1.0f;
    float density = 1.0f;
    float baseDensity = 1.0f;
    float minScale = 0.8f;
    float maxScale = 1.2f;
    std::vector<std::string> requiredSurfaceChemicals;
    std::vector<std::string> requiredAtmosphericChemicals;
    float minChemicalConcentration = 0.0f;
    std::vector<std::string> allowedBiomes;
    std::vector<std::string> requiredBiomes;
    bool hasCollision = true;
    bool canBeHarvested = false;
};

struct AtmosphericParameters {
    bool hasAtmosphere = true;
    float surfacePressure = 1.0f;
    float scaleHeight = 8.5f;
    float atmosphericHeight = 100.0f;
    std::vector<AtmosphericCompositionEntry> composition;
    float skyColorR = 0.4f;
    float skyColorG = 0.6f;
    float skyColorB = 1.0f;
    float sunsetColorR = 1.0f;
    float sunsetColorG = 0.5f;
    float sunsetColorB = 0.3f;
    float hazeDensity = 0.3f;
    float cloudDensity = 0.5f;
    std::string cloudChemicalId = "H2O";
    float cloudAltitudeMin = 2.0f;
    float cloudAltitudeMax = 15.0f;
    bool hasWeather = true;
    float averageTemperature = 15.0f;
    float temperatureVariation = 20.0f;
    float windSpeedAverage = 10.0f;
    float precipitationRate = 0.5f;
    std::string precipitationChemicalId = "H2O";
};

struct EcosystemParameters {
    std::vector<EcosystemObject> objects;
    float globalDensityMultiplier = 1.0f;
    int maxObjectsPerChunk = 100;
    bool enableCollisions = true;
};

struct PlanetDefinition {
    std::string id = "";
    std::string name = "Unnamed Planet";
    std::string description = "";
    std::string planetType = "terrestrial";
    std::string spectralClass = "G";
    OrbitalParameters orbit;
    PhysicalParameters physical;
    AtmosphericParameters atmosphere;
    TerrainParameters terrain;
    EcosystemParameters ecosystem;
    
    struct VisualParams {
        std::string diffuseTexture = "";
        std::string normalTexture = "";
        std::string specularTexture = "";
        std::string emissionTexture = "";
        std::string cloudTexture = "";
        Color baseColor = {100, 140, 100, 255};
        Color oceanColor = {30, 60, 120, 255};
        Color polarCapColor = {240, 240, 255, 255};
        float specularIntensity = 0.3f;
        float emissiveIntensity = 0.0f;
        bool hasRings = false;
        float ringInnerRadius = 0.0f;
        float ringOuterRadius = 0.0f;
        std::string ringMaterialId = "";
    } visual;
    
    bool isHabitable = false;
    bool hasMagneticField = false;
    float magneticFieldStrength = 0.0f;
    bool tidallyLocked = false;
    float tidalHeating = 0.0f;
    
    struct ResourceDeposit {
        std::string chemicalId = "";
        float abundance = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 100.0f;
        std::vector<std::string> preferredBiomes;
        std::string depositType = "vein";
    };
    std::vector<ResourceDeposit> resources;
    std::vector<std::string> moonIds;
    std::string formationHistory = "";
    float age = 4.5f;
};

class PlanetSystem {
private:
    std::unordered_map<std::string, PlanetDefinition> planets;
    std::string activePlanetId = "";
    std::string dataDirectory = "data/planets/";

public:
    bool LoadPlanetFromJSON(const std::string& jsonPath);
    bool LoadAllPlanetsFromDirectory(const std::string& directory = "");
    bool SavePlanetToJSON(const PlanetDefinition& planet, const std::string& jsonPath);
    PlanetDefinition* GetPlanet(const std::string& id);
    const PlanetDefinition* GetPlanet(const std::string& id) const;
    PlanetDefinition* GetActivePlanet();
    void SetActivePlanet(const std::string& id);
    bool AddPlanet(const PlanetDefinition& planet);
    bool RemovePlanet(const std::string& id);
    bool HasPlanet(const std::string& id) const;
    std::vector<std::string> GetAllPlanetIds() const;
    std::vector<const PlanetDefinition*> GetPlanetsInOrbit(const std::string& parentId) const;
    std::vector<const PlanetDefinition*> GetHabitablePlanets() const;
    void UpdateOrbits(float deltaTime);
    void UpdateRotation(float deltaTime);
    void ApplyTerrainNoise(float x, float y, float z, float& outDensity, unsigned char& outMaterial, const std::string& planetId);
    std::vector<EcosystemObject> GetSpawnableObjects(const std::string& planetId, float altitude, float temperature, float moisture, const std::string& biome) const;
    const ChemicalDefinition* GetPlanetChemical(const std::string& planetId, const std::string& chemicalId) const;
    std::vector<const ChemicalDefinition*> GetAtmosphericChemicals(const std::string& planetId) const;
    std::vector<const ChemicalDefinition*> GetSurfaceChemicals(const std::string& planetId, float altitude) const;
};

extern PlanetSystem g_PlanetSystem;

namespace PlanetUtils {
    Vector3 CalculateOrbitalPosition(const OrbitalParameters& orbit, float time);
    float CalculateSurfaceGravity(float mass, float radius);
    float CalculateEquilibriumTemperature(float distanceFromStar, float albedo, float stellarLuminosity);
    float EvaluateNoiseLayers(const std::vector<NoiseLayer>& layers, float x, float y, float z);
    bool CanSpawnObject(const EcosystemObject& obj, float altitude, float temperature, float moisture, const std::string& biome);
    const ChemicalDefinition* FindDominantChemical(const std::vector<SurfaceCompositionEntry>& composition, float altitude, float temperature);
}
