#include "inventory.h"
#include <cstring>

BlockItem::BlockItem(BlockType type, int qty, const std::string& itemName, Color color)
    : blockType(type), count(qty), name(itemName), displayColor(color) {}

Inventory::Inventory() : selectedSlot(0), isOpen(false) {
    hotbar.resize(HOTBAR_SIZE);
    backpack.resize(INVENTORY_SIZE);
}

void Inventory::SetSelectedSlot(int slot) {
    if (slot >= 0 && slot < HOTBAR_SIZE) {
        selectedSlot = slot;
    }
}

void Inventory::ScrollSelection(int direction) {
    selectedSlot += direction;
    if (selectedSlot < 0) selectedSlot = HOTBAR_SIZE - 1;
    if (selectedSlot >= HOTBAR_SIZE) selectedSlot = 0;
}

InventorySlot& Inventory::GetSelectedItem() {
    return hotbar[selectedSlot];
}

bool Inventory::AddBlock(BlockType type, int count) {
    BlockProperties props = GetBlockProperties(type);
    
    // First try to stack with existing same block type
    for (auto& slot : hotbar) {
        if (slot.type == ItemType::BLOCK && slot.block.blockType == type) {
            slot.block.count += count;
            return true;
        }
    }
    
    for (auto& slot : backpack) {
        if (slot.type == ItemType::BLOCK && slot.block.blockType == type) {
            slot.block.count += count;
            return true;
        }
    }
    
    // Find empty slot in hotbar first
    for (auto& slot : hotbar) {
        if (slot.IsEmpty()) {
            slot.type = ItemType::BLOCK;
            slot.block = BlockItem(type, count, BlockTypeToString(type), props.color);
            return true;
        }
    }
    
    // Then backpack
    for (auto& slot : backpack) {
        if (slot.IsEmpty()) {
            slot.type = ItemType::BLOCK;
            slot.block = BlockItem(type, count, BlockTypeToString(type), props.color);
            return true;
        }
    }
    
    return false; // Inventory full
}

bool Inventory::RemoveBlock(int slot, int count) {
    if (slot < 0 || slot >= HOTBAR_SIZE) return false;
    
    if (hotbar[slot].type == ItemType::BLOCK && hotbar[slot].block.count >= count) {
        hotbar[slot].block.count -= count;
        if (hotbar[slot].block.count <= 0) {
            hotbar[slot].Clear();
        }
        return true;
    }
    return false;
}

void Inventory::GiveStarterItems() {
    AddBlock(BlockType::GRASS, 64);
    AddBlock(BlockType::DIRT, 64);
    AddBlock(BlockType::STONE, 64);
    AddBlock(BlockType::WOOD, 32);
    AddBlock(BlockType::BRICK, 32);
    AddBlock(BlockType::GLASS, 16);
    AddBlock(BlockType::SAND, 32);
}

void Inventory::DrawHotbar(int screenWidth, int screenHeight) {
    const int slotSize = 50;
    const int slotSpacing = 5;
    const int totalWidth = HOTBAR_SIZE * (slotSize + slotSpacing) - slotSpacing;
    const int startX = (screenWidth - totalWidth) / 2;
    const int startY = screenHeight - slotSize - 20;
    
    // Draw hotbar background
    DrawRectangle(startX - 10, startY - 10, totalWidth + 20, slotSize + 20, {0, 0, 0, 200});
    
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        int x = startX + i * (slotSize + slotSpacing);
        int y = startY;
        
        // Slot background
        Color slotColor = (i == selectedSlot) ? GOLD : DARKGRAY;
        DrawRectangle(x, y, slotSize, slotSize, slotColor);
        DrawRectangleLines(x, y, slotSize, slotSize, BLACK);
        
        // Draw item if present
        if (!hotbar[i].IsEmpty() && hotbar[i].type == ItemType::BLOCK) {
            // Draw colored square representing block
            DrawRectangle(x + 5, y + 5, slotSize - 10, slotSize - 10, hotbar[i].block.displayColor);
            DrawRectangleLines(x + 5, y + 5, slotSize - 10, slotSize - 10, BLACK);
            
            // Draw count
            if (hotbar[i].block.count > 1) {
                DrawText(std::to_string(hotbar[i].block.count).c_str(), x + 8, y + slotSize - 18, 12, WHITE);
            }
        }
        
        // Draw slot number
        DrawText(std::to_string(i + 1).c_str(), x + 2, y + 2, 10, LIGHTGRAY);
    }
}

void Inventory::DrawInventory(int screenWidth, int screenHeight) {
    if (!isOpen) return;
    
    const int slotSize = 50;
    const int slotSpacing = 5;
    const int cols = 9;
    
    // Inventory panel dimensions
    int panelWidth = cols * (slotSize + slotSpacing) + 40;
    int panelHeight = 8 * (slotSize + slotSpacing) + 60;
    int panelX = (screenWidth - panelWidth) / 2;
    int panelY = (screenHeight - panelHeight) / 2;
    
    // Draw panel background
    DrawRectangle(panelX, panelY, panelWidth, panelHeight, {0, 0, 0, 240});
    DrawRectangleLines(panelX, panelY, panelWidth, panelHeight, WHITE);
    
    // Title
    DrawText("Inventory", panelX + 20, panelY + 10, 20, WHITE);
    
    // Draw hotbar at top of inventory
    int hotbarY = panelY + 40;
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        int x = panelX + 20 + i * (slotSize + slotSpacing);
        
        Color slotColor = (i == selectedSlot) ? GOLD : DARKGRAY;
        DrawRectangle(x, hotbarY, slotSize, slotSize, slotColor);
        DrawRectangleLines(x, hotbarY, slotSize, slotSize, BLACK);
        
        if (!hotbar[i].IsEmpty() && hotbar[i].type == ItemType::BLOCK) {
            DrawRectangle(x + 5, hotbarY + 5, slotSize - 10, slotSize - 10, hotbar[i].block.displayColor);
            if (hotbar[i].block.count > 1) {
                DrawText(std::to_string(hotbar[i].block.count).c_str(), x + 8, hotbarY + slotSize - 18, 12, WHITE);
            }
        }
    }
    
    // Draw backpack below
    int backpackY = hotbarY + slotSize + 20;
    DrawText("Storage", panelX + 20, backpackY - 20, 16, LIGHTGRAY);
    
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        int row = i / cols;
        int col = i % cols;
        int x = panelX + 20 + col * (slotSize + slotSpacing);
        int y = backpackY + row * (slotSize + slotSpacing);
        
        DrawRectangle(x, y, slotSize, slotSize, DARKGRAY);
        DrawRectangleLines(x, y, slotSize, slotSize, BLACK);
        
        if (!backpack[i].IsEmpty() && backpack[i].type == ItemType::BLOCK) {
            DrawRectangle(x + 5, y + 5, slotSize - 10, slotSize - 10, backpack[i].block.displayColor);
            if (backpack[i].block.count > 1) {
                DrawText(std::to_string(backpack[i].block.count).c_str(), x + 8, y + slotSize - 18, 12, WHITE);
            }
        }
    }
}

// Helper function
const char* BlockTypeToString(BlockType type) {
    switch (type) {
        case BlockType::GRASS: return "Grass";
        case BlockType::DIRT: return "Dirt";
        case BlockType::STONE: return "Stone";
        case BlockType::WOOD: return "Wood";
        case BlockType::SAND: return "Sand";
        case BlockType::WATER: return "Water";
        case BlockType::LEAVES: return "Leaves";
        case BlockType::BRICK: return "Brick";
        case BlockType::METAL: return "Metal";
        case BlockType::GLASS: return "Glass";
        default: return "Unknown";
    }
}
