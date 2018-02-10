#include			"Structures.h"
#include			"Editor Header.h"

//CONSTANTES:

#define				kPICTOffset				512
#define				kMaskLimit				127

//MACROS:

#define GetWindowPixMapPtr(w) *(((CGrafPtr)(w))->portPixMap)
#define	GWBitMapPtr(w) &(((GrafPtr)(w))->portBits)

//ROUTINES:

TQATexture* Texture_NewFromStorageTexture(TQAEngine* engine, TextureStoragePtr sourceTexture)
{
	TQAError		theQAErr = kQANoErr;
	OSStatus		theErr = noErr;
	TQAImage		image;
	TQATexture		*texture= NULL;
	Rect			pictRect,
					newPictRect;
	GWorldPtr		pictWorld = NULL;
	PixMapHandle	pictPix = NULL;
	CGrafPtr		savePort = NULL;
	GDHandle		saveDevice = NULL;
	Point			destPoint = {0,0};
	
	pictRect.left = 0;
	pictRect.top = 0;
	pictRect.right = sourceTexture->width;
	pictRect.bottom = sourceTexture->height;
		
	GetGWorld(&savePort, &saveDevice);
	theErr = NewGWorld(&pictWorld, 16, &pictRect, NULL, NULL, NULL);
	if(theErr)
	return nil;
	pictPix = GetGWorldPixMap(pictWorld);
	LockPixels(pictPix);
	
	SetGWorld(pictWorld, NULL);
	Texture_Draw(sourceTexture, GetWindowPixMapPtr(pictWorld), destPoint);
	if(thePrefs.flags & kPref_FlagReduce) {
		newPictRect.left = 0;
		newPictRect.top = 0;
		newPictRect.right = pictRect.right / 2;
		newPictRect.bottom = pictRect.bottom / 2;
		CopyBits(GWBitMapPtr(pictWorld), GWBitMapPtr(pictWorld), &pictRect, &newPictRect, srcCopy, nil);
		pictRect = newPictRect;
	}
	SetGWorld(savePort, saveDevice);

	image.width = pictRect.right;
	image.height = pictRect.bottom;
	image.rowBytes = (**pictPix).rowBytes & 0x3FFF;
	image.pixmap = GetPixBaseAddr(pictPix);
	
	if(thePrefs.flags & kPref_FlagCompress) {
		if(sourceTexture->flags & kFlag_HasAlpha)
		theQAErr = QATextureNew(engine, kQATexture_HighCompression, kQAPixel_ARGB16, &image, &texture);
		else
		theQAErr = QATextureNew(engine, kQATexture_HighCompression, kQAPixel_RGB16, &image, &texture);
	}
	else {
		if(sourceTexture->flags & kFlag_HasAlpha)
		theQAErr = QATextureNew(engine, kQATexture_None, kQAPixel_ARGB16, &image, &texture);
		else
		theQAErr = QATextureNew(engine, kQATexture_None, kQAPixel_RGB16, &image, &texture);
	}
	if(theQAErr == kQANoErr)
	theQAErr = QATextureDetach(engine, texture); 
	if(theQAErr == kQANoErr)
	goto cleanup2;
	
	if(texture != nil)
	QATextureDelete(engine, texture);
	texture = nil;
	
cleanup2:
	DisposeGWorld(pictWorld);
	
	return texture;
}
	
TextureStoragePtr Texture_NewStorageFromPictWithAlpha(PicHandle picture, OSType name)
{
	OSStatus			theErr = noErr;
	TQAImage			image;
	Rect				pictRect;
	GWorldPtr			pictWorld = NULL,
						bigWorld = NULL;
	PixMapHandle		pictPix = NULL,
						bigPix;
	CGrafPtr			savePort = NULL;
	GDHandle			saveDevice = NULL;
	TextureStoragePtr	textureStorage = nil;
	long				width,
						height;
	unsigned char		*maskPtr;
	unsigned short	*pixelPtr;
	Ptr					maskStartAddress,
						picStartAddress;
	long				maskRowBytes,
						picRowBytes;
					
	if(*picture == nil)
	return nil;
	pictRect = (**picture).picFrame;
	OffsetRect(&pictRect, -pictRect.left, -pictRect.top);
		
	GetGWorld(&savePort, &saveDevice);
	theErr = NewGWorld(&bigWorld, 32, &pictRect, NULL, NULL, NULL);
	if(theErr)
	return nil;
	theErr = NewGWorld(&pictWorld, 16, &pictRect, NULL, NULL, NULL);
	if(theErr)
	return nil;
	pictPix = GetGWorldPixMap(pictWorld);
	LockPixels(pictPix);
	bigPix = GetGWorldPixMap(bigWorld);
	LockPixels(bigPix);
	SetGWorld(bigWorld, NULL);
	DrawPicture(picture, &pictRect);
	SetGWorld(pictWorld, NULL);
	DrawPicture(picture, &pictRect);
	SetGWorld(savePort, saveDevice);

	picStartAddress = GetPixBaseAddr(pictPix);
	maskStartAddress = GetPixBaseAddr(bigPix);
	picRowBytes = (**pictPix).rowBytes & 0x3FFF;
	maskRowBytes = (**bigPix).rowBytes & 0x3FFF;
	height = pictRect.bottom;
	do {
		width = pictRect.right;
		pixelPtr = (unsigned short*) picStartAddress;
		maskPtr = (unsigned char*) maskStartAddress;
		do {
			if(*maskPtr > kMaskLimit)
			*pixelPtr |= 32768;
			++pixelPtr;
			maskPtr += 4;
		} while(--width);
		picStartAddress += picRowBytes;
		maskStartAddress += maskRowBytes;
	} while(--height);
	
	image.width = pictRect.right;
	image.height = pictRect.bottom;
	image.rowBytes = (**pictPix).rowBytes & 0x3FFF;
	image.pixmap = GetPixBaseAddr(pictPix);
	
	textureStorage = (TextureStoragePtr) NewPtr(sizeof(TextureStorage) + image.height * image.rowBytes);
	if(textureStorage == nil) {
		DisposeGWorld(pictWorld);
		DisposeGWorld(bigWorld);
		return nil;
	}
	textureStorage->name = name;
	textureStorage->flags = kFlag_HasAlpha;
	textureStorage->width = image.width;
	textureStorage->height = image.height;
	textureStorage->rowBytes = image.rowBytes;
	BlockMove(image.pixmap, &textureStorage->data[0], image.height * image.rowBytes);
	
	DisposeGWorld(pictWorld);
	DisposeGWorld(bigWorld);
	
	return textureStorage;
}

TextureStoragePtr Texture_NewStorageFromPict(PicHandle picture, OSType name)
{
	OSStatus			theErr = noErr;
	TQAImage			image;
	Rect				pictRect;
	GWorldPtr			pictWorld = NULL;
	PixMapHandle		pictPix = NULL;
	CGrafPtr			savePort = NULL;
	GDHandle			saveDevice = NULL;
	TextureStoragePtr	textureStorage = nil;
	
	if(*picture == nil)
	return nil;
	pictRect = (**picture).picFrame;
	OffsetRect(&pictRect, -pictRect.left, -pictRect.top);
		
	GetGWorld(&savePort, &saveDevice);
	theErr = NewGWorld(&pictWorld, 16, &pictRect, NULL, NULL, NULL);
	if(theErr)
	return nil;
	pictPix = GetGWorldPixMap(pictWorld);
	LockPixels(pictPix);
	SetGWorld(pictWorld, NULL);
	DrawPicture(picture, &pictRect);
	SetGWorld(savePort, saveDevice);

	image.width = pictRect.right;
	image.height = pictRect.bottom;
	image.rowBytes = (**pictPix).rowBytes & 0x3FFF;
	image.pixmap = GetPixBaseAddr(pictPix);
	
	textureStorage = (TextureStoragePtr) NewPtr(sizeof(TextureStorage) + image.height * image.rowBytes);
	if(textureStorage == nil) {
		DisposeGWorld(pictWorld);
		return nil;
	}
	textureStorage->name = name;
	textureStorage->flags = 0;
	textureStorage->width = image.width;
	textureStorage->height = image.height;
	textureStorage->rowBytes = image.rowBytes;
	BlockMove(image.pixmap, &textureStorage->data[0], image.height * image.rowBytes);
	
	DisposeGWorld(pictWorld);
	
	return textureStorage;
}

OSErr Texture_NewFromFile(TQAEngine* engine, FSSpec* theFile, Boolean useAlpha)
{
	short				fileID;
	long				picSize;
	OSErr				theError;
	PicHandle			picture;
	OSType				name;
	
	if(textureCount == kMaxShapes - 1)
	return -1;
	
	theError = FSpOpenDF(theFile, fsRdPerm, &fileID);
	if(theError)
	return theError;
	GetEOF(fileID, &picSize);
	picSize -= kPICTOffset;
	picture = (PicHandle) NewHandle(picSize);
	if(picture == nil)
	return MemError();
	HLock((Handle) picture);
	SetFPos(fileID, fsFromStart, kPICTOffset);
	FSRead(fileID, &picSize, *picture);
	FSClose(fileID);
	
	BlockMove(&theFile->name[1], &name, 4);
	
	if(useAlpha)
	textureStorageList[textureCount] = Texture_NewStorageFromPictWithAlpha(picture, name);
	else
	textureStorageList[textureCount] = Texture_NewStorageFromPict(picture, name);
	HUnlock((Handle) picture);
	DisposeHandle((Handle) picture);
	if(textureStorageList[textureCount] == nil)
	return -4;
	
	textureList[textureCount] = Texture_NewFromStorageTexture(engine, textureStorageList[textureCount]);
	if(textureList[textureCount] == nil)
	return -3;
	nameList[textureCount] = name;
	++textureCount;
	
	theCell.h = 0;
	theCell.v = LAddRow(1, 1000, textureListHandle);
	LSetCell(&theFile->name[1], 4, theCell, textureListHandle);
	LSetSelect(false, textureCell, textureListHandle);
	textureCell = theCell;
	LSetSelect(true, textureCell, textureListHandle);
	
	return noErr;
}

void Texture_Draw(TextureStoragePtr texturePtr, PixMapPtr destPixMap, Point destPoint)
{
	Ptr					sourceBaseAddress,
						destBaseAddress;
	long				height = texturePtr->height,
						sourceRowBytes,
						destRowBytes;
	
	sourceRowBytes = texturePtr->rowBytes;
	destRowBytes = destPixMap->rowBytes & 0x3FFF;
	sourceBaseAddress = (Ptr) &texturePtr->data[0];
	destBaseAddress = destPixMap->baseAddr + destPoint.v * destRowBytes + destPoint.h * 2;
	do {
		BlockMove(sourceBaseAddress, destBaseAddress, 2 * texturePtr->width);
		sourceBaseAddress += sourceRowBytes;
		destBaseAddress += destRowBytes;
	} while(--height);
}