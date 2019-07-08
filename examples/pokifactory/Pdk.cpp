
#include "Pdk.h"

#include <PokittoCore.h>

namespace { // private

#define PDK_TILES_W 27
#define PDK_TILES_H 22

struct TiledRenderInfo {
	uint8_t __attribute__ ((aligned)) buffer[(PDK_BUFFER_W/2)*PDK_BUFFER_H]; 
	uint16_t palette[16];
	uint8_t paletteIndexer[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

	// buffer
	uint8_t bufferStartX = 0;
	uint8_t bufferStartY = 0;
	uint8_t bufferWidth = 0;
	uint8_t bufferHeight = 0;
	uint16_t bufferSize = (PDK_BUFFER_W / 2) * PDK_BUFFER_H;
	uint8_t bufferHalfWidth = PDK_BUFFER_W / 2;
	const uint16_t* bufferMap = 0;
	const uint8_t* bufferTilesImage = 0;
	uint8_t bufferImageHalfWidth = 0;
	uint8_t bufferImageTilesW = 0;
	uint8_t screenMarginOffset = 0;
	bool bufferUpdated = false;

	// background
	const uint16_t* backMap = 0;
	const uint8_t* backTilesImage = 0;
	bool backMapUpdated = false;

	// back swap tiles
	const uint8_t* backSwapTilesImage = 0;
	uint8_t numBackSwapTiles = 0;
	uint32_t __attribute__ ((aligned)) backSwapTiles[PDK_MAX_BACK_SWAP_TILES];
	bool backSwapTilesUpdated = false;

	// time data
	uint8_t timePerFrame = 0;
	uint32_t nextFrameMillis = 0;
	uint32_t frameEndMicros = 0;
	//fps vars
	uint32_t fps_frameCount = 0;
	uint32_t fps_refreshtime = 0;
	uint32_t fps_counter = 0;
};

TiledRenderInfo renderInfo;

static inline void setup_data_16(uint16_t data)
{
	// SET_MASK_P2;
	LPC_GPIO_PORT->MPIN[2] = (data<<3); // write bits to port
	// CLR_MASK_P2;
}

inline void write_command_16(uint16_t data)
{
   // CLR_CS; // select lcd
   CLR_CD; // clear CD = command
   SET_RD; // RD high, do not read
   setup_data_16(data); // function that inputs the data into the relevant bus lines
   CLR_WR_SLOW;  // WR low
   SET_WR;  // WR low, then high = write strobe
   // SET_CS; // de-select lcd
}

inline void write_data_16(uint16_t data)
{
   // CLR_CS;
   SET_CD;
   SET_RD;
   setup_data_16(data);
   CLR_WR;
   SET_WR;
   // SET_CS;
}


void drawBackMap(){
	uint8_t imgHalfWidth = renderInfo.backTilesImage[0] / 2;

	for(uint8_t tileY = 0; tileY < PDK_TILES_H; ++tileY){
		uint16_t lcdY = (8 * tileY);

		for(uint8_t tileX = 0; tileX < PDK_TILES_W; ++tileX){
			uint16_t lcdX = (8 * tileX);
				
			uint16_t mapIt = ((tileY * PDK_TILES_W) + tileX) + 2;
			uint16_t mapTileId = renderInfo.backMap[mapIt];
			if(mapTileId > 0){// it's a back tile, otherwise it's buffer area
				mapTileId -= 1;//map tiles are stored with base index at 1, bring back to 0
				
				uint8_t mapTileX = mapTileId % PDK_TILES_W;
				uint8_t mapTileY = mapTileId / PDK_TILES_W;

				for(uint8_t yOffset = 0; yOffset < 8; ++yOffset){
					write_command_16(0x21); write_data_16(lcdX);// move to x lcd coord
					write_command_16(0x20); write_data_16(lcdY + yOffset);// move to y lcd coord
					write_command(0x22); // ready to write pixels column

					for(uint8_t xOffset = 0; xOffset < 4; ++xOffset){
						uint16_t imgPixelIt = 
							(((8 * mapTileY) + yOffset) * imgHalfWidth) + // y coord
							(4 * mapTileX) + xOffset; // x coord

						uint8_t paletteIds = renderInfo.backTilesImage[imgPixelIt + 2];

						uint8_t paletteId = paletteIds >> 4;
						uint16_t color = renderInfo.palette[paletteId];
						write_data_16(color);

						paletteId = paletteIds & (0x0F);
						color = renderInfo.palette[paletteId];
						write_data_16(color);
					}
				}
			}
		}
	}

	renderInfo.backMapUpdated = false;
}

void drawBackSwapTiles(){
	uint8_t imgHalfWidth = renderInfo.backSwapTilesImage[0] / 2;
	uint8_t imgTilesW = renderInfo.backSwapTilesImage[0] / 8;

	for(uint16_t arratIt = 0; arratIt < renderInfo.numBackSwapTiles; ++arratIt){
		uint32_t combined = renderInfo.backSwapTiles[arratIt];
		if(combined == 0){
			continue;
		}

		combined -= 1;

		uint16_t screenTileId = (uint16_t) (combined >> 16);
		uint16_t dynTileId = (uint16_t) (combined & 0x0000FFFFuL);

		uint8_t screenTileX = screenTileId % PDK_TILES_W;
		uint8_t screenTileY = screenTileId / PDK_TILES_W;

		uint8_t imgTileX = dynTileId % imgTilesW;
		uint8_t imgTileY = dynTileId / imgTilesW;

		for(uint8_t yOffset = 0; yOffset < 8; ++yOffset){
			write_command_16(0x21); write_data_16(8 * screenTileX);// move to x lcd coord
			write_command_16(0x20); write_data_16((8 * screenTileY) + yOffset);// move to y lcd coord
			write_command(0x22); // ready to write row of pixels 

			uint16_t imgPixelIt = 
					(((8 * imgTileY) + yOffset) * imgHalfWidth) + // y coord
					(4 * imgTileX); // x coord

			for(uint8_t xOffset = 0; xOffset < 4; ++xOffset){
				uint8_t paletteIds = renderInfo.backSwapTilesImage[imgPixelIt + 2];

				uint8_t paletteId = paletteIds >> 4;
				uint16_t color = renderInfo.palette[paletteId];
				write_data_16(color);

				paletteId = paletteIds & (0x0F);
				color = renderInfo.palette[paletteId];
				write_data_16(color);

				++imgPixelIt;
			}
		}
	}

	renderInfo.backSwapTilesUpdated = false;
}

void drawBuffer(){
	for(uint8_t bufferY = 0; bufferY < renderInfo.bufferHeight; ++bufferY){
		write_command_16(0x20); 
		write_data_16(renderInfo.bufferStartY + bufferY);// move to y lcd coord

		write_command_16(0x21); write_data_16(renderInfo.bufferStartX);// move to x lcd coord
		write_command(0x22); // ready to write row of pixels

		uint16_t bufferIt = bufferY * renderInfo.bufferHalfWidth;

		for(uint8_t bufferTileX = 0; bufferTileX < renderInfo.bufferHalfWidth; ++bufferTileX){
			uint8_t paletteIds = renderInfo.buffer[bufferIt];
			uint8_t paletteId = paletteIds >> 4;
			uint16_t color = renderInfo.palette[paletteId];
			
			// optimized version of write_data_16(color);
			SET_CD;
			SET_RD;  
			setup_data_16(color);
			
			paletteId = paletteIds & (0x0F);
			color = renderInfo.palette[paletteId];

			// optimized version of write_data_16(color);
			CLR_WR;
			SET_WR;
			setup_data_16(color);
			CLR_WR;
			SET_WR;

			++bufferIt;
		}
	}

	renderInfo.bufferUpdated = false;
}

void updateDisplay(){
	if(renderInfo.backMapUpdated){
		drawBackMap();
	}
	else if(renderInfo.backSwapTilesUpdated){
		drawBackSwapTiles();
	}

	if(renderInfo.bufferUpdated){
		drawBuffer();
	}
}

inline void paintTileSlow(uint8_t transparentPaletteId, bool flipX, bool flipY, uint8_t bufferX, uint8_t bufferY, uint8_t bufferMapTileX, uint8_t bufferMapTileY){
	uint8_t bufferModX = bufferX % 2;
	
	for(int tilePixelXit = 0; tilePixelXit < 4; ++tilePixelXit){
		for(int tilePixelYit = 0; tilePixelYit < 8; ++tilePixelYit){
			uint8_t pixel2;
			uint8_t pixel1;

			if(!flipX){
				uint16_t bufferImagePixelIt =
					((tilePixelYit + (8*bufferMapTileY)) * renderInfo.bufferImageHalfWidth) +
					(tilePixelXit + (4*bufferMapTileX));
				
				uint8_t bufferImagePixels = renderInfo.bufferTilesImage[bufferImagePixelIt + 2];

				pixel1 = bufferImagePixels >> 4;
				pixel2 = bufferImagePixels & (0x0F);
			}
			else{
				uint16_t bufferImagePixelIt =
					((tilePixelYit + (8*bufferMapTileY)) * renderInfo.bufferImageHalfWidth) +
					(4-tilePixelXit + (4*bufferMapTileX))-2;

				uint8_t bufferImagePixels = renderInfo.bufferTilesImage[bufferImagePixelIt + 2];

				pixel1 = bufferImagePixels & (0x0F);
				pixel2 = bufferImagePixels >> 4;
			}
			
			uint16_t bufferPixelIt = 
				((bufferY + tilePixelYit) * renderInfo.bufferHalfWidth) + 
				((bufferX/2) + tilePixelXit);

			if(bufferModX == 0){
				uint8_t bufferPixels = renderInfo.buffer[bufferPixelIt];
				uint8_t bufferPixel1 = bufferPixels >> 4;
				uint8_t bufferPixel2 = bufferPixels & (0x0F);

				if(pixel1 == transparentPaletteId){
					pixel1 = bufferPixel1;
				}
				if(pixel2 == transparentPaletteId){
					pixel2 = bufferPixel2;
				}

				bufferPixels = (bufferPixels&0x0F) | (pixel1<<4);
				bufferPixels = (bufferPixels&0xF0) | (pixel2);
				
				renderInfo.buffer[bufferPixelIt] = bufferPixels;
			}
			else{ // it means we are drawing half between two cells of the buffer (which contains two pixels per horizontal cell)
				uint8_t bufferPixelsLeft = renderInfo.buffer[bufferPixelIt];
				uint8_t bufferPixelsRight = renderInfo.buffer[bufferPixelIt + 1];

				uint8_t bufferPixel1 = bufferPixelsLeft >> 4;
				uint8_t bufferPixel2 = bufferPixelsRight & (0x0F);

				if(pixel1 == transparentPaletteId){
					pixel1 = bufferPixel1;
				}
				if(pixel2 == transparentPaletteId){
					pixel2 = bufferPixel2;
				}

				renderInfo.buffer[bufferPixelIt + 1] = (bufferPixelsRight&0x0F) | (pixel2<<4);
				renderInfo.buffer[bufferPixelIt] = (bufferPixelsLeft&0xF0) | (pixel1);
			}
		}
	}
}

void paintBufferTilesSlow(
	uint8_t bufferTileX, uint8_t bufferTileY, 
	uint8_t bufferMapRegionX, uint8_t bufferMapRegionY, 
	uint8_t bufferMapRegionWidth, uint8_t bufferMapRegionHeight){

	for(uint8_t tileXit = 0; tileXit < bufferMapRegionWidth; ++tileXit){
		for(uint8_t tileYit = 0; tileYit < bufferMapRegionHeight; ++tileYit){
			uint16_t bufferMapIt = 
				((bufferMapRegionY+tileYit) * renderInfo.bufferImageTilesW) + 
				(bufferMapRegionX+tileXit);

			uint16_t bufferMapTileId = renderInfo.bufferMap[bufferMapIt + 2] - 1;

			uint8_t bufferMapTileX = bufferMapTileId % renderInfo.bufferImageTilesW;
			uint8_t bufferMapTileY = bufferMapTileId / renderInfo.bufferImageTilesW;

			paintTileSlow(16, false, false,//no transparency or flips
				8 * (bufferTileX + tileXit), 
				8 * (bufferTileY + tileYit), 
				bufferMapTileX, bufferMapTileY);
		}
	}
}

void paintBufferSpriteSlow(
	uint8_t transparentPaletteId, bool flipX, bool flipY,
	uint8_t bufferX, uint8_t bufferY, 
	uint8_t bufferMapRegionX, uint8_t bufferMapRegionY, 
	uint8_t bufferMapRegionWidth, uint8_t bufferMapRegionHeight){

	for(uint8_t tileXit = 0; tileXit < bufferMapRegionWidth; ++tileXit){
		for(uint8_t tileYit = 0; tileYit < bufferMapRegionHeight; ++tileYit){
			uint16_t bufferMapIt = 
				((bufferMapRegionY+tileYit) * renderInfo.bufferImageTilesW) + 
				(bufferMapRegionX+tileXit);

			if(flipX){
				bufferMapIt = 
					((bufferMapRegionY+tileYit) * renderInfo.bufferImageTilesW) + 
					((bufferMapRegionX+bufferMapRegionWidth-1) - tileXit);
			}

			uint16_t bufferMapTileId = renderInfo.bufferMap[bufferMapIt + 2] - 1;

			uint8_t bufferMapTileX = bufferMapTileId % renderInfo.bufferImageTilesW;
			uint8_t bufferMapTileY = bufferMapTileId / renderInfo.bufferImageTilesW;

			paintTileSlow(transparentPaletteId, flipX, flipY,
				bufferX + (8 * tileXit), 
				bufferY + (8 * tileYit), 
				bufferMapTileX, bufferMapTileY);
		}
	}
}

} // end private namespace


void pdk::init(){
	Pokitto::Core::initClock();
	Pokitto::Core::initGPIO();
	Pokitto::Core::initButtons();
	Pokitto::Core::initRandom();
	Pokitto::lcdInit();

	write_command(0x03); write_data(0x1038); //realy only need to call this once
}

void pdk::setBackMap(const uint16_t* map, const uint8_t*  img){
	renderInfo.backMap = map;
	renderInfo.backTilesImage = img;
	renderInfo.backMapUpdated = true;
}

void pdk::setBufferMap(const uint16_t* map, const uint8_t* img){
	renderInfo.bufferMap = map;
	renderInfo.bufferTilesImage = img;

	renderInfo.bufferImageHalfWidth = renderInfo.bufferTilesImage[0] / 2;
	renderInfo.bufferImageTilesW = renderInfo.bufferTilesImage[0] / 8;

	renderInfo.bufferUpdated = true;
}

void pdk::setBackSwapImage(const uint8_t*  img){
	renderInfo.backSwapTilesImage = img;
	renderInfo.backSwapTilesUpdated= true;
}

void pdk::setPalette(const uint8_t* p) {
	for (int i=0; i < 16; i++){
		uint16_t color;
		color = p[(3 * i)+2]>>3;// B
		color |= ((p[(3 * i)+1] >> 2) << 5);// G
		color |= ((p[3 * i] >> 3) << 11);// R

		renderInfo.palette[i] = color;
	}
}

void pdk::setBufferArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height){
	renderInfo.bufferStartX = x;
	renderInfo.bufferStartY = y;
	renderInfo.bufferWidth = width;
	renderInfo.bufferHeight = height;

	renderInfo.bufferHalfWidth = width / 2;
	renderInfo.bufferSize = renderInfo.bufferHalfWidth * height;

	renderInfo.bufferUpdated = true;
}

void pdk::setFrameRateLimit(uint8_t fps) {
	renderInfo.timePerFrame = 1000 / fps;
}

uint32_t pdk::getFPS(){
    return renderInfo.fps_counter;
}

bool pdk::shouldUpdate(){
	uint32_t now = Pokitto::Core::getTime();

	if ((((renderInfo.nextFrameMillis - now)) > renderInfo.timePerFrame) && renderInfo.frameEndMicros == 1) { //if time to render a new frame is reached and the frame end has ran once
		renderInfo.nextFrameMillis = now + renderInfo.timePerFrame;

		renderInfo.frameEndMicros = 0;
		Pokitto::Core::buttons.update();

		#ifdef PDK_SHOWFPS
		// fps counter to be enabled at compile time
		static uint32_t fpsInterval_ms = 1000*3;
		renderInfo.fps_frameCount++;
		if (now > renderInfo.fps_refreshtime) {
		    renderInfo.fps_counter = (1000*renderInfo.fps_frameCount) / (now - renderInfo.fps_refreshtime + fpsInterval_ms);
		    renderInfo.fps_refreshtime = now + fpsInterval_ms;
		    renderInfo.fps_frameCount = 0;
		}
		#endif

		return true;
	}
	else{
		if(renderInfo.frameEndMicros == 0){ //runs once at the end of the frame
			updateDisplay();

			renderInfo.frameEndMicros = 1; //jonne
		}

		return false;
	}

	return true;
}

void pdk::setBackSwapTilesNum(uint8_t num){
	if(num > PDK_MAX_BACK_SWAP_TILES){
		num = PDK_MAX_BACK_SWAP_TILES;
	}

	renderInfo.numBackSwapTiles = num;
}

void pdk::setBackSwapTile(uint8_t dynTileArrayId, uint8_t screenTileX, uint8_t screenTileY, uint16_t dynTileId){
	uint16_t screenTileId = (PDK_TILES_W * screenTileY) + screenTileX;

	uint32_t combined = (((uint32_t)screenTileId) << 16) | ((uint32_t)dynTileId);
	renderInfo.backSwapTiles[dynTileArrayId] = combined + 1;

	renderInfo.backSwapTilesUpdated = true;
}

void pdk::clearBuffer(uint8_t paletteColorId){
	for(uint16_t i = 0; i < renderInfo.bufferSize; ++i){
		uint8_t pixel = 0;
		pixel = (pixel&0xF0) | (paletteColorId);
		pixel = (pixel&0x0F) | (paletteColorId<<4);
		
		renderInfo.buffer[i] = pixel;
	}

	renderInfo.bufferUpdated = true;
}

void pdk::paintBufferTiles(uint8_t bufferTileX, uint8_t bufferTileY, uint8_t bufferMapRegionX, uint8_t bufferMapRegionY, uint8_t bufferMapRegionWidth, uint8_t bufferMapRegionHeight){
	#ifdef PDK_SLOW_READABLE_CODE
	paintBufferTilesSlow(
		bufferTileX, bufferTileY, 
		bufferMapRegionX, bufferMapRegionY, bufferMapRegionWidth, bufferMapRegionHeight);
	#else // optimized code, annoying to read
	uint8_t mapWidth = renderInfo.bufferMap[0];

	uint16_t bufferMapIt = (bufferMapRegionY * mapWidth) + bufferMapRegionX;

	uint16_t verticalOffset1 = (bufferMapRegionHeight * mapWidth) - 1;
	uint16_t verticalOffset2 = ((renderInfo.bufferHalfWidth * 8) * bufferMapRegionHeight) - 4;

	uint16_t bufferPixelIt = ((8 * bufferTileY) * renderInfo.bufferHalfWidth) + ((8 * bufferTileX) / 2);
	uint8_t tileY = 0;

	for(uint16_t tileIt = 0; tileIt < bufferMapRegionWidth*bufferMapRegionHeight; ++tileIt){
		uint16_t tileId = renderInfo.bufferMap[bufferMapIt + 2] - 1;
		bufferMapIt += mapWidth; // move 1 from bufferMapRegionY

		uint8_t mapTileX = tileId % renderInfo.bufferImageTilesW;
		uint8_t mapTileY = tileId / renderInfo.bufferImageTilesW;

		uint8_t pixelX = 0;
		uint16_t tilePixelIt = ((8 * mapTileY) * renderInfo.bufferImageHalfWidth) + (4 * mapTileX);
		
		for(int pixelIt = 0; pixelIt < 32; ++pixelIt){
			uint8_t pixels = renderInfo.bufferTilesImage[tilePixelIt + 2];

			// re-index palette to allow palette swapping
			uint8_t pixel1 = pixels >> 4;
			uint8_t pixel2 = pixels & (0x0F);
			pixel1 = renderInfo.paletteIndexer[pixel1];
			pixel2 = renderInfo.paletteIndexer[pixel2];
			pixels = (pixels&0x0F) | (pixel1<<4);
			pixels = (pixels&0xF0) | (pixel2);

			renderInfo.buffer[bufferPixelIt] = pixels;

			++tilePixelIt;
			++bufferPixelIt;

			++pixelX;
			if(pixelX == 4){
				pixelX = 0;

				tilePixelIt += renderInfo.bufferImageHalfWidth - 4;
				bufferPixelIt += renderInfo.bufferHalfWidth - 4;
			}
		}

		++tileY;
		if(tileY == bufferMapRegionHeight){
			bufferMapIt -= verticalOffset1; // back to bufferMapRegionY + move 1 from bufferMapRegionX
			
			bufferPixelIt -= verticalOffset2;
			tileY = 0;
		}
	}
	#endif
	
	renderInfo.bufferUpdated = true;
}

void pdk::paintBufferSprite(
	uint8_t transparentPaletteId, 
	uint8_t bufferX, uint8_t bufferY, 
	uint8_t bufferMapRegionX, uint8_t bufferMapRegionY, uint8_t bufferMapRegionWidth, uint8_t bufferMapRegionHeight){

	#ifdef PDK_SLOW_READABLE_CODE
	bool flippedX = false;
	bool flippedY = false;
	paintBufferSpriteSlow(
		transparentPaletteId, flippedX, flippedY,
		bufferX, bufferY, 
		bufferMapRegionX, bufferMapRegionY,
		bufferMapRegionWidth, bufferMapRegionHeight);
	#else
	uint8_t mapWidth = renderInfo.bufferMap[0];
	
	uint16_t bufferMapIt = (bufferMapRegionY * mapWidth) + bufferMapRegionX;

	uint16_t verticalOffset1 = (bufferMapRegionHeight * mapWidth) - 1;
	uint16_t verticalOffset2 = ((renderInfo.bufferHalfWidth * 8) * bufferMapRegionHeight) - 4;

	uint8_t bufferModX = bufferX % 2;
	uint8_t bufferModY = bufferY % 2;
	uint16_t bufferPixelIt = ((bufferY + bufferModY) * renderInfo.bufferHalfWidth) + (bufferX / 2);
	uint8_t tileY = 0;

	for(uint16_t tileIt = 0; tileIt < bufferMapRegionWidth*bufferMapRegionHeight; ++tileIt){
		uint16_t tileId = renderInfo.bufferMap[bufferMapIt + 2] - 1;
		bufferMapIt += mapWidth; // move 1 from bufferMapRegionY

		uint8_t mapTileX = tileId % renderInfo.bufferImageTilesW;
		uint8_t mapTileY = tileId / renderInfo.bufferImageTilesW;

		uint8_t pixelX = 0;
		uint16_t tilePixelIt = ((8 * mapTileY) * renderInfo.bufferImageHalfWidth) + (4 * mapTileX);
		
		for(int pixelIt = 0; pixelIt < 32; ++pixelIt){
			uint8_t pixels = renderInfo.bufferTilesImage[tilePixelIt + 2];
			uint8_t pixel1 = pixels >> 4;
			uint8_t pixel2 = pixels & (0x0F);

			// re-index palette to allow palette swapping
			pixel1 = renderInfo.paletteIndexer[pixel1];
			pixel2 = renderInfo.paletteIndexer[pixel2];

			if(bufferModX == 0){
				uint8_t bufferPixels = renderInfo.buffer[bufferPixelIt];
				uint8_t bufferPixel1 = bufferPixels >> 4;
				uint8_t bufferPixel2 = bufferPixels & (0x0F);

				if(pixel1 == transparentPaletteId){
					pixel1 = bufferPixel1;
				}
				if(pixel2 == transparentPaletteId){
					pixel2 = bufferPixel2;
				}

				bufferPixels = (bufferPixels&0x0F) | (pixel1<<4);
				bufferPixels = (bufferPixels&0xF0) | (pixel2);
				
				renderInfo.buffer[bufferPixelIt] = bufferPixels;
			}
			else{ // it means we are drawing half between two cells of the buffer (which contains two pixels per horizontal cell, bummer)
				uint8_t bufferPixelsLeft = renderInfo.buffer[bufferPixelIt];
				uint8_t bufferPixelsRight = renderInfo.buffer[bufferPixelIt + 1];

				uint8_t bufferPixel1 = bufferPixelsLeft >> 4;
				uint8_t bufferPixel2 = bufferPixelsRight & (0x0F);

				if(pixel1 == transparentPaletteId){
					pixel1 = bufferPixel1;
				}
				if(pixel2 == transparentPaletteId){
					pixel2 = bufferPixel2;
				}

				renderInfo.buffer[bufferPixelIt + 1] = (bufferPixelsRight&0x0F) | (pixel2<<4);
				renderInfo.buffer[bufferPixelIt] = (bufferPixelsLeft&0xF0) | (pixel1);
			}

			++tilePixelIt;
			++bufferPixelIt;

			++pixelX;
			if(pixelX == 4){
				pixelX = 0;

				tilePixelIt += renderInfo.bufferImageHalfWidth - 4;
				bufferPixelIt += renderInfo.bufferHalfWidth - 4;
			}
		}

		++tileY;
		if(tileY == bufferMapRegionHeight){
			bufferMapIt -= verticalOffset1; // back to bufferMapRegionY + move 1 from bufferMapRegionX
			
			bufferPixelIt -= verticalOffset2;
			tileY = 0;
		}
	}
	#endif

	renderInfo.bufferUpdated = true;
}

void pdk::paintBufferSpriteFlipX(
	uint8_t transparentPaletteId, 
	uint8_t bufferX, uint8_t bufferY, 
	uint8_t bufferMapRegionX, uint8_t bufferMapRegionY, uint8_t bufferMapRegionWidth, uint8_t bufferMapRegionHeight){

	#ifdef PDK_SLOW_READABLE_CODE
	bool flippedX = true;
	bool flippedY = false;
	paintBufferSpriteSlow(
		transparentPaletteId, flippedX, flippedY,
		bufferX, bufferY, 
		bufferMapRegionX, bufferMapRegionY,
		bufferMapRegionWidth, bufferMapRegionHeight);
	#else
	uint8_t mapWidth = renderInfo.bufferMap[0];

	uint16_t bufferMapIt = (bufferMapRegionY * mapWidth) + (bufferMapRegionX + (bufferMapRegionWidth-1));

	uint16_t verticalOffset1 = (bufferMapRegionHeight * mapWidth)+1;
	uint16_t verticalOffset2 = ((renderInfo.bufferHalfWidth * 8) * bufferMapRegionHeight) - 4;

	uint8_t bufferModX = bufferX % 2;
	uint8_t bufferModY = bufferY % 2;
	uint16_t bufferPixelIt = ((bufferY + bufferModY) * renderInfo.bufferHalfWidth) + (bufferX / 2);
	uint8_t tileY = 0;

	for(uint16_t tileIt = 0; tileIt < bufferMapRegionWidth*bufferMapRegionHeight; ++tileIt){
		uint16_t tileId = renderInfo.bufferMap[bufferMapIt + 2] - 1;
		bufferMapIt += mapWidth; // move 1 from bufferMapRegionY

		uint8_t mapTileX = tileId % renderInfo.bufferImageTilesW;
		uint8_t mapTileY = tileId / renderInfo.bufferImageTilesW;

		uint8_t pixelX = 0;
		uint16_t tilePixelIt = ((8 * mapTileY) * renderInfo.bufferImageHalfWidth) + ((4 * mapTileX) + 3);
		
		for(int pixelIt = 0; pixelIt < 32; ++pixelIt){
			uint8_t pixels = renderInfo.bufferTilesImage[tilePixelIt + 2];
			uint8_t pixel1 = pixels >> 4;
			uint8_t pixel2 = pixels & (0x0F);

			// re-index palette to allow palette swapping
			pixel1 = renderInfo.paletteIndexer[pixel1];
			pixel2 = renderInfo.paletteIndexer[pixel2];

			if(bufferModX == 0){
				uint8_t bufferPixels = renderInfo.buffer[bufferPixelIt];
				uint8_t bufferPixel1 = bufferPixels >> 4;
				uint8_t bufferPixel2 = bufferPixels & (0x0F);

				if(pixel1 == transparentPaletteId){
					pixel1 = bufferPixel1;
				}
				if(pixel2 == transparentPaletteId){
					pixel2 = bufferPixel2;
				}

				bufferPixels = (bufferPixels&0x0F) | (pixel2<<4);
				bufferPixels = (bufferPixels&0xF0) | (pixel1);
				
				renderInfo.buffer[bufferPixelIt] = bufferPixels;
			}
			else{ // it means we are drawing half between two cells of the buffer (which contains two pixels per horizontal cell, bummer)
				uint8_t bufferPixelsLeft = renderInfo.buffer[bufferPixelIt];
				uint8_t bufferPixelsRight = renderInfo.buffer[bufferPixelIt + 1];

				uint8_t bufferPixel1 = bufferPixelsLeft >> 4;
				uint8_t bufferPixel2 = bufferPixelsRight & (0x0F);

				if(pixel1 == transparentPaletteId){
					pixel1 = bufferPixel2;
				}
				if(pixel2 == transparentPaletteId){
					pixel2 = bufferPixel1;
				}

				renderInfo.buffer[bufferPixelIt + 1] = (bufferPixelsRight&0x0F) | (pixel1<<4);
				renderInfo.buffer[bufferPixelIt] = (bufferPixelsLeft&0xF0) | (pixel2);
			}

			--tilePixelIt;
			++bufferPixelIt;

			++pixelX;
			if(pixelX == 4){
				pixelX = 0;

				tilePixelIt += renderInfo.bufferImageHalfWidth + 4;
				bufferPixelIt += renderInfo.bufferHalfWidth - 4;
			}
		}

		++tileY;
		if(tileY == bufferMapRegionHeight){
			bufferMapIt -= verticalOffset1; // back to bufferMapRegionY + move 1 from bufferMapRegionX
			
			bufferPixelIt -= verticalOffset2;
			tileY = 0;
		}
	}
	#endif

	renderInfo.bufferUpdated = true;
}

void pdk::swapPaletteIndex(uint8_t paletteId, uint8_t swappedPaletteId){
	renderInfo.paletteIndexer[paletteId] = swappedPaletteId;
}

bool pdk::buttonPressed(uint8_t button){
	return Pokitto::heldStates[button];
}

bool pdk::random(uint8_t min, uint8_t max){
	return rand() % (max-min) + max;
}
