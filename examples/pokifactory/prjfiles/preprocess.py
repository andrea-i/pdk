
import xml.dom.minidom

def padhexa(s):
	return '0x' + s[2:].zfill(2)

def getPalette(path):
	image_file = open(path, "rb")

	bytes = image_file.read()
	colorBytes = bytes[54:(54+48)]
	it = 0
	outVals = []
	for i in range(16):
		b = colorBytes[it]
		it += 1
		g = colorBytes[it]
		it += 1
		r = colorBytes[it]
		it += 1

		outVals.append(padhexa(hex(r)))
		outVals.append(padhexa(hex(g)))
		outVals.append(padhexa(hex(b)))

	image_file.close()

	return outVals

def paletteToCpp(bmpFilename, outFIlename):
	palette = getPalette(bmpFilename, )

	outFile = open(outFIlename, 'w')

	outFile.write('#pragma once\n\n')
	outFile.write('const uint8_t palette[48]={\n')

	for i in range(16):
		outFile.write(palette[3 * i] + ', ')
		outFile.write(palette[(3 * i) + 1] + ', ')
		outFile.write(palette[(3 * i) + 2] + ', \n')

	outFile.write('};')

	outFile.close()

def tiledToCpp(mapName, tiledFilename, outFilename):
	parse = xml.dom.minidom.parse(tiledFilename)
	tilemap = parse.documentElement

	layer = tilemap.getElementsByTagName('layer')[0]
	width = int(layer.getAttribute('width'))
	height = int(layer.getAttribute('height'))
	layerData = layer.getElementsByTagName('data')[0].getElementsByTagName('tile')

	outFile = open(outFilename, 'w')
	outFile.write('#include <stdint.h>\n\n')
	outFile.write('//width: '+ str(width) + ' height: ' + str(height) + '\n')
	outFile.write('const uint16_t ' + mapName + ' [] ={\n')
	outFile.write(str(width) + ', ' + str(height) + ',\n')

	lineStr = ''
	it = 0
	for tileData in layerData:
		tileIdStr = tileData.getAttribute('gid')
		if tileIdStr == '':
			tileId = 0
		else:
			tileId = int(tileIdStr)
		
		lineStr += str(tileId) + ','

		if it == width - 1:
			outFile.write(lineStr + '\n')
			lineStr = ''
			it = 0
		else:
			it += 1

	# add support for flipped tiles
	# const unsigned FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
	# const unsigned FLIPPED_VERTICALLY_FLAG   = 0x40000000;
	# const unsigned FLIPPED_DIAGONALLY_FLAG   = 0x20000000;
	# print(tileId & 0x40000000)

	outFile.write('};\n')
	outFile.close()


paletteToCpp('palette.bmp', '..\\palette.h')
tiledToCpp('backMap', 'backMap.tmx', '..\\backMap.h')
tiledToCpp('bufferMap', 'bufferMap.tmx', '..\\bufferMap.h')

