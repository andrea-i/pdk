#pragma once

#include <stdint.h>

namespace pdk{

//mandatory  compile-time settings
// #define PDK_BUFFER_W 0
// #define PDK_BUFFER_H 0
// #define PDK_MAX_BACK_SWAP_TILES 0

// game
void init();
bool shouldUpdate();
void setFrameRateLimit(uint8_t fps);
void setPalette(const uint8_t* p);
bool buttonPressed(uint8_t button);
uint32_t getFPS();

// background
void setBackMap(const uint16_t* map, const uint8_t*  img);

// background tiles swap
void setBackSwapImage(const uint8_t*  img);
void setBackSwapTilesNum(uint8_t num);
void setBackSwapTile(uint8_t dynTileArrayId, uint8_t screenTileX, uint8_t screenTileY, uint16_t dynTileImageTileId);

// buffer
void swapPaletteIndex(uint8_t paletteId, uint8_t swappedPaletteId);
void setBufferArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void setBufferMap(const uint16_t* map, const uint8_t* img);
void clearBuffer(uint8_t paletteColorId);
void paintBufferTiles(uint8_t bufferTileX, uint8_t bufferTileY, uint8_t bufferMapRegionX, uint8_t bufferMapRegionY, uint8_t bufferMapRegionWidth, uint8_t bufferMapRegionHeight);
void paintBufferSprite(uint8_t transparentPaletteId, uint8_t bufferX, uint8_t bufferY, uint8_t bufferMapRegionX, uint8_t bufferMapRegionY, uint8_t bufferMapRegionWidth, uint8_t bufferMapRegionHeight);
void paintBufferSpriteFlipX(uint8_t transparentPaletteId, uint8_t bufferX, uint8_t bufferY, uint8_t bufferMapRegionX, uint8_t bufferMapRegionY, uint8_t bufferMapRegionWidth, uint8_t bufferMapRegionHeight);
bool random(uint8_t min, uint8_t max);
}// end pdk