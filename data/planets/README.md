# Planet System Documentation

A data-driven planet generation system with full customization support for orbital parameters, physical properties, atmospheric conditions, terrain noise control, and ecosystem objects.

## Overview

The planet system allows you to define planets programmatically via JSON files. Each planet can have:

- **Orbital Parameters**: Position, movement, and relationship to parent objects (stars, other planets)
- **Physical Parameters**: Size, mass, gravity, rotation
- **Atmospheric Parameters**: Composition, pressure, visual properties, weather
- **Terrain Noise Layers**: Multiple configurable noise layers for procedural terrain generation
- **Ecosystem Objects**: Flora, fauna, structures, and resources with placement rules

## File Structure

```
data/
  planets/
    earth.json      # Earth-like planet example
    mars.json       # Mars-like planet example
    moon.json       # Moon/satellite example
```

## JSON Schema

### Basic Planet Information

```json
{
    "id": "unique_identifier",           // Required: Unique planet ID
    "name": "Planet Name",               // Display name
    "description": "Description text",   // Lore/description
    "planetType": "terrestrial"          // terrestrial, gas_giant, ice_giant, dwarf, asteroid
}
```

### Orbital Parameters

```json
{
    "semiMajorAxis": 1000.0,             // Distance from parent object
    "eccentricity": 0.0167,              // 0 = circular, 1 = parabolic
    "inclination": 0.0,                  // Orbital tilt in degrees
    "orbitalPeriod": 365.25,             // Time to complete orbit (days)
    "parentObjectId": "sun"              // ID of parent object (for moons)
}
```

### Physical Parameters

```json
{
    "radius": 50.0,                      // Planet radius in world units
    "mass": 1.0,                         // Relative mass (Earth = 1.0)
    "surfaceGravity": 9.81,              // Surface gravity (m/s²)
    "rotationalPeriod": 24.0,            // Day length (hours)
    "axialTilt": 23.5                    // Seasonal tilt (degrees)
}
```

### Atmospheric Parameters

```json
{
    "hasAtmosphere": true,
    "surfacePressure": 1.0,              // Atmospheres (1 = Earth sea level)
    "skyColorR": 0.4,                    // Sky color (RGB 0-1)
    "skyColorG": 0.6,
    "skyColorB": 1.0,
    "averageTemperature": 15.0,          // Celsius
    "hasWeather": true
}
```

### Terrain Noise Layers

Multiple noise layers can be combined for complex terrain:

```json
{
    "baseHeight": 0.0,                   // Base terrain height
    "heightScale": 25.0,                 // Multiplier for noise height
    "waterLevel": 0.3,                   // Normalized water level (0-1)
    "cliffThreshold": 0.7,               // When to generate cliffs
    
    "noiseLayers": [
        {
            "name": "continents",
            "amplitude": 1.0,            // Layer strength
            "frequency": 0.01,           // Noise scale
            "octaves": 4,                // Detail levels
            "blendMode": "add"           // add, subtract, multiply, max, min, lerp
        },
        {
            "name": "mountains",
            "amplitude": 0.5,
            "frequency": 0.03,
            "octaves": 6,
            "blendMode": "add"
        }
    ]
}
```

**Blend Modes:**
- `add`: Add layer noise to previous
- `subtract`: Subtract layer noise from previous
- `multiply`: Multiply noise values
- `max`: Take maximum of both
- `min`: Take minimum of both
- `lerp`: Linear interpolation (requires `blendFactor`)

### Visual Parameters

```json
{
    "baseColorR": 100,                   // Base terrain color (RGB 0-255)
    "baseColorG": 140,
    "baseColorB": 100,
    "oceanColorR": 30,                   // Water color
    "oceanColorG": 60,
    "oceanColorB": 120,
    "specularIntensity": 0.3,            // Shininess
    "hasRings": false,                   // Has ring system
    "ringInnerRadius": 0.0,              // Ring inner radius
    "ringOuterRadius": 0.0               // Ring outer radius
}
```

### Special Properties

```json
{
    "isHabitable": true,                 // Can support life
    "hasMagneticField": true,            // Magnetosphere protection
    "tidallyLocked": false               // Same face always toward parent
}
```

### Moons

```json
{
    "moons": ["moon_id_1", "moon_id_2"]  // Array of moon planet IDs
}
```

### Ecosystem Objects

Define objects that spawn on the planet surface:

```json
{
    "ecosystemObjects": [
        {
            "id": "oak_tree",
            "type": "flora",              // flora, fauna, structure, resource
            "name": "Oak Tree",
            "modelPath": "models/vegetation/oak_tree.obj",
            "texturePath": "textures/vegetation/oak_bark.png",
            
            // Placement conditions
            "minAltitude": -10.0,
            "maxAltitude": 800.0,
            "minTemperature": 5.0,
            "maxTemperature": 35.0,
            "minMoisture": 0.3,
            "maxMoisture": 1.0,
            "allowedBiomes": ["forest", "plains"],
            "requiredBiomes": [],
            
            // Density and distribution
            "baseDensity": 0.5,           // Objects per square unit
            "clusteringFactor": 0.5,      // 0 = uniform, 1 = clustered
            "minScale": 0.8,
            "maxScale": 1.5,
            
            // Interaction
            "hasCollision": true,
            "canBeHarvested": true,
            "harvestedResource": "wood",
            "harvestAmount": 5,
            
            // Animation
            "animated": false,
            "animationType": "sway",      // sway, rotate, pulse
            "animationSpeed": 1.0
        }
    ]
}
```

## Usage Examples

### Loading Planets in Code

```cpp
#include "core/planet.h"

// Load all planets from directory
g_PlanetSystem.LoadAllPlanetsFromDirectory("data/planets/");

// Set active planet for terrain generation
g_PlanetSystem.SetActivePlanet("earth");

// Get planet data
const PlanetDefinition* earth = g_PlanetSystem.GetPlanet("earth");
if (earth) {
    printf("Planet: %s\n", earth->name.c_str());
    printf("Radius: %f\n", earth->physical.radius);
    printf("Gravity: %f m/s²\n", earth->physical.surfaceGravity);
}

// Update orbital positions and rotation
g_PlanetSystem.UpdateOrbits(deltaTime);
g_PlanetSystem.UpdateRotation(deltaTime);
```

### Creating Custom Terrain Generation

```cpp
// Apply planet-specific noise to terrain
float density;
unsigned char material;
g_PlanetSystem.ApplyTerrainNoise(worldX, worldY, worldZ, 
                                  density, material, "earth");
```

### Spawning Ecosystem Objects

```cpp
// Get objects that can spawn at location
auto spawnableObjects = g_PlanetSystem.GetSpawnableObjects(
    "earth",           // Planet ID
    altitude,          // Current altitude
    temperature,       // Local temperature
    moisture,          // Moisture level (0-1)
    "forest"           // Biome type
);

for (const auto& obj : spawnableObjects) {
    // Spawn object with random position/scale
    float density = obj.baseDensity * g_PlanetSystem.GetActivePlanet()->ecosystem.globalDensityMultiplier;
    // ... spawning logic
}
```

### Adding Planets Programmatically

```cpp
PlanetDefinition customPlanet;
customPlanet.id = "myplanet";
customPlanet.name = "My Custom Planet";
customPlanet.planetType = "terrestrial";
customPlanet.physical.radius = 75.0f;
customPlanet.isHabitable = true;

// Add noise layer
NoiseLayer layer;
layer.name = "base";
layer.amplitude = 1.0f;
layer.frequency = 0.02f;
layer.octaves = 6;
customPlanet.terrain.noiseLayers.push_back(layer);

// Add to system
g_PlanetSystem.AddPlanet(customPlanet);

// Save to file
g_PlanetSystem.SavePlanetToJSON(customPlanet, "data/planets/myplanet.json");
```

## Example Planets Included

### Earth
- Habitable terrestrial planet
- Multiple biomes with diverse ecosystem
- 4 noise layers for varied terrain
- Trees, animals, and resource deposits
- One moon

### Mars
- Cold desert world
- Thin atmosphere
- Volcanic and canyon features
- Ice and mineral deposits
- Crashed probe easter egg

### Moon (Luna)
- Earth's satellite
- No atmosphere
- Crater-heavy surface
- Helium-3 and titanium resources
- Historic landing sites

## API Reference

### PlanetSystem Class

| Method | Description |
|--------|-------------|
| `LoadPlanetFromJSON(path)` | Load single planet from JSON file |
| `LoadAllPlanetsFromDirectory(dir)` | Load all planets from directory |
| `SavePlanetToJSON(planet, path)` | Save planet definition to JSON |
| `GetPlanet(id)` | Get planet by ID (returns pointer) |
| `GetActivePlanet()` | Get currently active planet |
| `SetActivePlanet(id)` | Set active planet |
| `AddPlanet(planet)` | Add planet to system |
| `RemovePlanet(id)` | Remove planet from system |
| `HasPlanet(id)` | Check if planet exists |
| `GetAllPlanetIds()` | Get list of all planet IDs |
| `GetPlanetsInOrbit(parentId)` | Get moons of a planet |
| `GetHabitablePlanets()` | Get all habitable planets |
| `UpdateOrbits(deltaTime)` | Update orbital positions |
| `UpdateRotation(deltaTime)` | Update planetary rotation |
| `ApplyTerrainNoise(...)` | Generate terrain for planet |
| `GetSpawnableObjects(...)` | Get ecosystem objects for location |

### PlanetUtils Namespace

| Function | Description |
|----------|-------------|
| `CalculateOrbitalPosition(orbit, time)` | Calculate 3D position in orbit |
| `CalculateSurfaceGravity(mass, radius)` | Calculate gravity from mass/radius |
| `CalculateEquilibriumTemperature(...)` | Estimate temperature from star distance |
| `EvaluateNoiseLayers(layers, x, y, z)` | Evaluate multiple noise layers |
| `CanSpawnObject(obj, ...)` | Check if object can spawn at location |

## Best Practices

1. **Unique IDs**: Always use unique IDs for planets and ecosystem objects
2. **Noise Layer Order**: Define noise layers from largest scale to smallest detail
3. **Performance**: Limit ecosystem object density for better performance
4. **Realism**: Use realistic values for orbital periods, gravity, etc.
5. **Testing**: Test planets in-game before finalizing parameters

## Extending the System

To add new features:

1. Add new fields to the appropriate struct in `planet.h`
2. Add JSON parsing in `planet.cpp` (Extract*Value functions)
3. Add serialization in `SavePlanetToJSON()`
4. Update documentation

## Troubleshooting

**Planet not loading:**
- Check that `id` field is present and unique
- Verify JSON syntax is valid
- Check console for error messages

**Terrain looks wrong:**
- Adjust noise layer frequencies and amplitudes
- Check `heightScale` and `baseHeight` values
- Verify blend modes are correct

**Objects not spawning:**
- Check altitude/temperature/moisture ranges
- Verify biome names match
- Ensure `baseDensity` is > 0

## License

This system is part of your project and follows the project's license terms.
