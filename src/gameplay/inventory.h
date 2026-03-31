#pragma once
#include "../terrain/terrain.h"
#include "raylib.h"
#include <vector>
#include <string>

// Item types
enum class ItemType {
    NONE,
    BLOCK,
    TOOL
};

// Block item - can be placed in world
struct BlockItem {
    BlockType blockType;
    int count;
    std::string name;
    Color displayColor;
    
    BlockItem() : blockType(BlockType::AIR), count(0), name("Empty"), displayColor(BLANK) {}
    BlockItem(BlockType type, int qty, const std::string& itemName, Color color);
};

// Inventory slot
struct InventorySlot {
    ItemType type;
    BlockItem block;
    
    InventorySlot() : type(ItemType::NONE) {}
    
    bool IsEmpty() const { return type == ItemType::NONE || block.count <= 0; }
    void Clear() { type = ItemType::NONE; block = BlockItem(); }
};

// Main inventory system
class Inventory {
private:
    static const int HOTBAR_SIZE = 9;
    static const int INVENTORY_SIZE = 27; // 3 rows of 9
    
    std::vector<InventorySlot> hotbar;
    std::vector<InventorySlot> backpack;
    int selectedSlot; // 0-8 for hotbar
    bool isOpen;
    
public:
    Inventory();
    
    // Hotbar management
    int GetSelectedSlot() const { return selectedSlot; }
    void SetSelectedSlot(int slot);
    void ScrollSelection(int direction); // -1 or +1
    InventorySlot& GetSelectedItem();
    
    // Add items
    bool AddBlock(BlockType type, int count);
    bool RemoveBlock(int slot, int count);
    
    // Getters
    InventorySlot& GetHotbarSlot(int index) { return hotbar[index]; }
    InventorySlot& GetBackpackSlot(int index) { return backpack[index]; }
    bool IsOpen() const { return isOpen; }
    void ToggleOpen() { isOpen = !isOpen; }
    void Close() { isOpen = false; }
    
    // UI rendering
    void DrawHotbar(int screenWidth, int screenHeight);
    void DrawInventory(int screenWidth, int screenHeight);
    
    // Initialize with some starter blocks
    void GiveStarterItems();
};

// Helper function
const char* BlockTypeToString(BlockType type);
