#include "Pdk.h"
#include <cstdio>
#include "palette.h"
#include "backTiles.h"
#include "backMap.h"
#include "bufferTiles.h"
#include "bufferMap.h"

bool gFlip = 0;
bool gDrawGhosting = false;
uint8_t gRPlayerPos = 0;

void drawRDudeUp(){
	// draw legs
	pdk::paintBufferSprite(15, 130, 88, 1, 14, 2, 1);

	// draw torso animated
	if(gFlip){
		if(gDrawGhosting){
			pdk::swapPaletteIndex(1, 14);
			pdk::paintBufferSprite(15, 119, 72, 0, 12, 3, 2);// ghosting
		}
		pdk::swapPaletteIndex(1, 1);
		pdk::paintBufferSpriteFlipX(15, 133, 72, 0, 12, 3, 2);
	}
	else{
		if(gDrawGhosting){
			pdk::swapPaletteIndex(1, 14);
			pdk::paintBufferSpriteFlipX(15, 134, 72, 0, 12, 3, 2);// ghosting
		}
		pdk::swapPaletteIndex(1, 1);
		pdk::paintBufferSprite(15, 119, 72, 0, 12, 3, 2);
	}
}

void drawRDudeDown(){
	// draw legs
	pdk::paintBufferSprite(15, 130, 47, 4, 14, 2, 1);

	// draw torso animated
	if(gFlip){
		if(gDrawGhosting){
			pdk::swapPaletteIndex(1, 14);
			pdk::paintBufferSprite(15, 129, 32, 7, 13, 2, 2);
			pdk::paintBufferSprite(15, 121, 24, 6, 12, 2, 1);
		}
		pdk::swapPaletteIndex(1, 1);
		pdk::paintBufferSprite(15, 122, 32, 3, 12, 3, 2);
		pdk::paintBufferSprite(15, 122, 48, 3, 14, 1, 1);
	}
	else{
		if(gDrawGhosting){
			pdk::swapPaletteIndex(1, 14);
			pdk::paintBufferSprite(15, 122, 32, 3, 12, 3, 2);
			pdk::paintBufferSprite(15, 122, 48, 3, 14, 1, 1);
		}
		pdk::swapPaletteIndex(1, 1);
		pdk::paintBufferSprite(15, 129, 32, 7, 13, 2, 2);
		pdk::paintBufferSprite(15, 121, 24, 6, 12, 2, 1);
	}
}

#define BTN_UP      3
#define BTN_RIGHT   2
#define BTN_DOWN    1
#define BTN_LEFT    0
#define BTN_A           4
#define BTN_B           5
#define BTN_C           6

int main(){
	pdk::init();
	pdk::setPalette(palette);
	pdk::setFrameRateLimit(30);

	pdk::setBackMap(backMap, backTiles); // fills the background and leave untouched

	pdk::setBufferMap(bufferMap, bufferTiles);
	pdk::setBufferArea(16, 24, 176, 96);

	uint8_t animTick = 0;

	while(true){
		if(pdk::shouldUpdate()){ 
			if(pdk::buttonPressed(BTN_UP)){
				gRPlayerPos = 1;
			}
			else if(pdk::buttonPressed(BTN_DOWN)){
				gRPlayerPos = 0;
			}

			pdk::paintBufferTiles(0, 0, 0, 0, 22, 12);// draw buffer background tiles
			
			if(gRPlayerPos == 1){
				drawRDudeUp();
			}
			else{
				drawRDudeDown();
			}
			
			animTick = (animTick + 1) % 10;
			if(animTick == 0){
				gFlip = !gFlip;
				gDrawGhosting = true;
			}

			if(animTick == 2){
				gDrawGhosting = false;
			}
		}
	}

	return 0;
}
