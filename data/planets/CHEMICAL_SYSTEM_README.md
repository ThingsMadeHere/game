# Chemical-Based Planet System

## Overview

This system allows you to construct planets using a database of chemicals with real thermodynamic properties. Players can select chemicals from the database to define:
- Atmospheric composition
- Surface/crust composition  
- Resource deposits
- Water bodies
- Ecosystem requirements

## Chemical Database

### Location
`data/chemicals/chemicals.json`

### Chemical Properties

Each chemical includes:

| Property | Type | Description |
|----------|------|-------------|
| `id` | string | Unique identifier (e.g., "H2O", "O2") |
| `name` | string | Display name |
| `formula` | string | Chemical formula |
| `molarMass` | double | g/mol |
| `meltingPoint` | double | Kelvin |
| `boilingPoint` | double | Kelvin |
| `density` | double | kg/m³ |
| `gibbsFreeEnergy` | double | kJ/mol (formation) |
| `specificHeat` | double | J/(mol·K) |
| `thermalConductivity` | double | W/(m·K) |
| `phaseAtSTP` | int | 0=Solid, 1=Liquid, 2=Gas |
| `criticalTemperature` | double | K (for gases) |
| `criticalPressure` | double | atm (for gases) |
| `colorHex` | string | Visual representation |
| `isToxic` | bool | Gameplay flag |
| `isFlammable` | bool | Gameplay flag |
| `isCorrosive` | bool | Gameplay flag |
| `isNutrient` | bool | Can support life |
| `isRadioactive` | bool | Emits radiation |
| `cosmicAbundance` | float | 0-1 rarity |

### Available Chemicals

- **H2O** - Water (liquid at Earth temps)
- **O2** - Oxygen (breathable gas)
- **N2** - Nitrogen (inert atmospheric gas)
- **CO2** - Carbon Dioxide (greenhouse gas)
- **CH4** - Methane (flammable, Titan lakes)
- **NH3** - Ammonia (toxic, alternative solvent)
- **SiO2** - Silicon Dioxide (sand/rock)
- **Fe** - Iron (metal resource)
- **Fe2O3** - Rust (red planet color)
- **NaCl** - Salt (ocean deposits)
- **H2SO4** - Sulfuric Acid (Venus clouds)
- **He** - Helium (gas giant component)

## Planet JSON Structure

### Atmosphere with Chemicals

```json
"atmosphere": {
  "hasAtmosphere": true,
  "surfacePressure": 1.0,
  "composition": [
    {"chemicalId": "N2", "percentage": 0.78},
    {"chemicalId": "O2", "percentage": 0.21},
    {"chemicalId": "CO2", "percentage": 0.0004}
  ],
  "cloudChemicalId": "H2O",
  "precipitationChemicalId": "H2O"
}
```

### Crust Composition

```json
"terrain": {
  "waterChemicalId": "H2O",
  "crustComposition": [
    {"chemicalId": "SiO2", "concentration": 0.59, "distributionType": "uniform"},
    {"chemicalId": "Fe2O3", "concentration": 0.15, "distributionType": "clustered"},
    {"chemicalId": "Fe", "concentration": 0.05, "distributionType": "vein", "veinSize": 2.0}
  ]
}
```

Distribution types:
- `uniform` - Evenly distributed
- `clustered` - Random clusters
- `vein` - Mineral veins
- `biome_dependent` - Varies by biome

### Ecosystem Requirements

Ecosystem objects (defined in C++) reference required chemicals:

```json
"ecosystem": {
  "objectReferences": [
    {
      "objectId": "oak_tree",
      "requiredSurfaceChemicals": ["H2O", "O2"],
      "requiredAtmosphericChemicals": ["O2", "CO2"],
      "minChemicalConcentration": 0.1
    }
  ]
}
```

### Resource Deposits

```json
"resources": [
  {
    "chemicalId": "Fe",
    "abundance": 0.8,
    "depositType": "vein",
    "minDepth": 10.0,
    "maxDepth": 500.0
  }
]
```

## Usage in Code

```cpp
// Initialize chemical database
CHEMICAL_DB.LoadDefaults();
CHEMICAL_DB.LoadFromFile("data/chemicals/chemicals.json");

// Get chemical properties
const ChemicalDefinition* water = CHEMICAL_DB.GetChemical("H2O");
double boilingPoint = water->boilingPoint; // 373.15 K

// Check phase at temperature
int phase = CHEMICAL_DB.GetPhaseAtTemp("H2O", 300.0); // Returns 1 (liquid)

// Load planet with chemical references
g_PlanetSystem.LoadPlanetFromJSON("data/planets/earth_chemical.json");

// Query planet's atmospheric chemicals
auto atmosphere = g_PlanetSystem.GetAtmosphericChemicals("earth_chemical");
for (const auto& chem : atmosphere) {
    printf("%s: %s\n", chem->formula.c_str(), chem->name.c_str());
}

// Get spawnable objects based on chemical conditions
auto objects = g_PlanetSystem.GetSpawnableObjectRefs(
    "earth_chemical", 
    altitude, temperature, moisture, biome
);
```

## Creating Custom Planets

1. **Choose atmospheric composition** - Select gases and ratios
2. **Define surface materials** - Pick minerals/compounds for crust
3. **Set liquid bodies** - Choose liquid chemical (H2O, CH4, NH3)
4. **Place resource deposits** - Define mineable chemicals
5. **Configure ecosystem requirements** - Link C++ objects to chemical needs

### Example: Methane World

```json
{
  "id": "titan_like",
  "atmosphere": {
    "composition": [
      {"chemicalId": "N2", "percentage": 0.95},
      {"chemicalId": "CH4", "percentage": 0.05}
    ],
    "precipitationChemicalId": "CH4"
  },
  "terrain": {
    "waterChemicalId": "CH4",
    "waterLevel": 0.2
  }
}
```

### Example: Venus-like

```json
{
  "id": "venus_like",
  "atmosphere": {
    "surfacePressure": 92.0,
    "composition": [
      {"chemicalId": "CO2", "percentage": 0.96},
      {"chemicalId": "N2", "percentage": 0.03}
    ],
    "cloudChemicalId": "H2SO4"
  }
}
```

## Extending the Database

Add new chemicals to `chemicals.json`:

```json
{
  "id": "NewChemical",
  "name": "Display Name",
  "formula": "XyZ",
  "molarMass": 100.0,
  "meltingPoint": 300.0,
  "boilingPoint": 500.0,
  "density": 2000.0,
  "gibbsFreeEnergy": -100.0,
  "phaseAtSTP": 0,
  "colorHex": "#FF0000",
  "isToxic": false
}
```

## Phase Determination

The system automatically determines chemical phases:
- **T < meltingPoint**: Solid
- **meltingPoint ≤ T < boilingPoint**: Liquid  
- **T ≥ boilingPoint**: Gas

This affects:
- Rendering (oceans vs ice caps)
- Precipitation type (rain vs snow)
- Ecosystem viability
- Resource extraction methods
