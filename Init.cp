#include			<QD3DAcceleration.h>

#include			"Structures.h"
#include			"Editor Header.h"
#include			"Matrix.h"

#include			<Rave_System.h>
#include			"InfinityRave.h"

//CONSTANTES:

#define					kOffset						32

#define					kPrecision					4
#define					kBorder						2

//MACROS:

#define Color16(r,g,b) (b + (g << 5) + (r << 10))
#define Color32(r,g,b) (b + (g << 8) + (r << 16))

//ROUTINES:

void InitToolbox()
{
	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0L);
	InitCursor();
	MaxApplZone();
	
	MoreMasters();
	MoreMasters();
	MoreMasters();
	MoreMasters();
}

void Init_Menus()
{
	menu[appleM] = GetMenu(appleID);
	AppendResMenu(menu[appleM], 'DRVR');
	menu[fileM] = GetMenu(fileID);
	menu[renderM] = GetMenu(renderID);
	menu[viewM] = GetMenu(viewID);
	menu[windowM] = GetMenu(windowID);
	menu[editM] = GetMenu(editID);
	
	InsertMenu(menu[0], 0);
	InsertMenu(menu[1], 0);
	InsertMenu(menu[5], 0);
	InsertMenu(menu[2], 0);
	InsertMenu(menu[3], 0);
	InsertMenu(menu[4], 0);
	
	texturePopUpMenu = GetMenu(128);
	InsertMenu(texturePopUpMenu, -1);
	shapePopUpMenu = GetMenu(129);
	InsertMenu(shapePopUpMenu, -1);
	scriptPopUpMenu = GetMenu(130);
	InsertMenu(scriptPopUpMenu, -1);
	sequencerPopUpMenu = GetMenu(131);
	InsertMenu(sequencerPopUpMenu, -1);
	
	DrawMenuBar();
	
	DisableItem(menu[1], 3);
	DisableItem(menu[1], 5);
	DisableItem(menu[1], 7);
	CheckItem(menu[2], 1, true);
	CheckItem(menu[2], 6, true);
	CheckItem(menu[3], 1, true);
	DisableItem(menu[3], 8);
	CheckItem(menu[4], 1, true);
	CheckItem(menu[4], 2, true);
	CheckItem(menu[4], 3, true);
	DisableItem(texturePopUpMenu, 0);
	DisableItem(shapePopUpMenu, 0);
	DisableItem(scriptPopUpMenu, 0);
}
	
void Init_MainWindow()
{
	mainWin = GetNewCWindow(128, nil, (WindowPtr) -1);
	SetGWorld((CGrafPtr) mainWin, nil);
	BackColor(blackColor);
	ForeColor(whiteColor);
	TextFont(3);
	TextSize(18);
	TextFace(0);
}

void Init_TextureWindow()
{
	Rect			listBounds = {0,0,0,1};
	Point			cellSize = {0,0};
	
	textureWin = GetNewDialog(129, nil, (WindowPtr) -1);
	MoveWindow(textureWin, thePrefs.windowPosition[1].h, thePrefs.windowPosition[1].v, false);
	SetGWorld((CGrafPtr) textureWin, nil);
	BackColor(whiteColor);
	ForeColor(blackColor);
	TextFont(3);
	TextSize(10);
	TextFace(1);
	
	textureListHandle = LNew(&viewRect, &listBounds, cellSize, 0, textureWin, true, false, false, true);
}

void Init_ShapeWindow()
{
	Rect			listBounds = {0,0,0,1};
	Point			cellSize = {0,0};
	
	shapeWin = GetNewDialog(130, nil, (WindowPtr) -1);
	MoveWindow(shapeWin, thePrefs.windowPosition[0].h, thePrefs.windowPosition[0].v, false);
	SetGWorld((CGrafPtr) shapeWin, nil);
	BackColor(whiteColor);
	ForeColor(blackColor);
	TextFont(3);
	TextSize(10);
	TextFace(1);
	
	shapeListHandle = LNew(&viewRect, &listBounds, cellSize, 0, shapeWin, true, false, false, true);
}

void Init_ScriptWindow()
{
	Rect			listBounds = {0,0,0,1};
	Point			cellSize = {0,0};
	
	scriptWin = GetNewDialog(131, nil, (WindowPtr) -1);
	MoveWindow(scriptWin, thePrefs.windowPosition[2].h, thePrefs.windowPosition[2].v, false);
	SetGWorld((CGrafPtr) scriptWin, nil);
	BackColor(whiteColor);
	ForeColor(blackColor);
	TextFont(3);
	TextSize(10);
	TextFace(1);
	
	scriptListHandle = LNew(&viewRect, &listBounds, cellSize, 0, scriptWin, true, false, false, true);
}

void Init_SequencerWindow()
{
	sequencerWin = GetNewDialog(128, nil, (WindowPtr) -1);
	MoveWindow(sequencerWin, thePrefs.windowPosition[4].h, thePrefs.windowPosition[4].v, false);
	SetGWorld((CGrafPtr) sequencerWin, nil);
	BackColor(whiteColor);
	ForeColor(blackColor);
	TextFont(3);
	TextSize(10);
	TextFace(1);
	
	GetDialogItemAsControl(sequencerWin, 2, &sequencerScroll);
	SetControlMinimum(sequencerScroll, 0);
	SetControlMaximum(sequencerScroll, 0);
	SetControlValue(sequencerScroll, 0);
	
	GetDialogItemAsControl(sequencerWin, 3, &timeScroll);
	SetControlMinimum(timeScroll, 0);
	SetControlMaximum(timeScroll, 90);
	SetControlValue(timeScroll, 0);
}

void Init_MappingWindow()
{
	mappingWin = GetNewCWindow(129, nil, (WindowPtr) -1);
	MoveWindow(mappingWin, thePrefs.windowPosition[3].h, thePrefs.windowPosition[3].v, false);
	SetGWorld((CGrafPtr) mappingWin, nil);
	BackColor(whiteColor);
	ForeColor(blackColor);
}

static void Buffer_DrawSelectedPoint32(const TQADevice* buffer, StatePtr state, ShapePtr shape, MatrixPtr transformationMatrix, long pointNum)
{
	Vector				point;
	float				pixelConversion = (state->d / state->h) * (-state->viewWidth / 2),
						iw;
	Point				thePoint;
	TQADeviceMemory*	memoryDevice;
	GDHandle			gDevice;
	PixMapPtr			destPixMap;
	Ptr					destBaseAddress,
						baseAddress;
	long				destRowBytes,
						height,
						width;
	unsigned long*	pixel;
	
	Matrix_TransformVector(transformationMatrix, &shape->pointList[pointNum].point, &point);
	
	if(!orthographic) {
		iw = 1.0 / point.z;
		thePoint.h = point.x * iw * pixelConversion + (state->viewWidth/2);
		thePoint.v = point.y * iw * pixelConversion + (state->viewHeight/2);
	}
	else {
		thePoint.h = point.x / orthographicScale * pixelConversion + (state->viewWidth/2);
		thePoint.v = point.y / orthographicScale * pixelConversion + (state->viewHeight/2);
	}
				
	if(buffer->deviceType == kQADeviceMemory) {
		memoryDevice = (TQADeviceMemory*) &buffer->device.memoryDevice;
		
		destRowBytes = memoryDevice->rowBytes;
		destBaseAddress = (Ptr) memoryDevice->baseAddr;
		
		if((thePoint.h < kPrecision) || (thePoint.h > memoryDevice->width - kPrecision))
		return;
		if((thePoint.v < kPrecision) || (thePoint.v > memoryDevice->height - kPrecision))
		return;
	}
	else if(buffer->deviceType == kQADeviceGDevice) {
		gDevice = buffer->device.gDevice;
		destPixMap = *(**gDevice).gdPMap;
		
		destRowBytes = destPixMap->rowBytes & 0x3FFF;
		destBaseAddress = destPixMap->baseAddr;
		
		if((thePoint.h < kPrecision) || (thePoint.h > destPixMap->bounds.right - kPrecision))
		return;
		if((thePoint.v < kPrecision) || (thePoint.v > destPixMap->bounds.bottom - kPrecision))
		return;
	}
	
	baseAddress = destBaseAddress + (thePoint.v - kPrecision) * destRowBytes + (thePoint.h - kPrecision) * 4;
	height = kPrecision * 2;
	do {
		width = kPrecision * 2;
		pixel = (unsigned long*) baseAddress;
		do {
			*pixel = Color32(255, 255, 0);
			++pixel;
		} while(--width);
		baseAddress += destRowBytes;
	} while(--height);
	
	baseAddress = destBaseAddress + (thePoint.v - kPrecision + kBorder) * destRowBytes + (thePoint.h - kPrecision + kBorder) * 4;
	height = (kPrecision  - kBorder) * 2;
	do {
		width = (kPrecision  - kBorder) * 2;
		pixel = (unsigned long*) baseAddress;
		do {
			*pixel = 0x00000000;
			++pixel;
		} while(--width);
		baseAddress += destRowBytes;
	} while(--height);
}

static void Buffer_DrawSelectedPoint16(const TQADevice* buffer, StatePtr state, ShapePtr shape, MatrixPtr transformationMatrix, long pointNum)
{
	Vector				point;
	float				pixelConversion = (state->d / state->h) * (-state->viewWidth / 2),
						iw;
	Point				thePoint;
	TQADeviceMemory*	memoryDevice;
	GDHandle			gDevice;
	PixMapPtr			destPixMap;
	Ptr					destBaseAddress,
						baseAddress;
	long				destRowBytes,
						height,
						width;
	unsigned short*	pixel;
	
	Matrix_TransformVector(transformationMatrix, &shape->pointList[pointNum].point, &point);
	
	if(!orthographic) {
		iw = 1.0 / point.z;
		thePoint.h = point.x * iw * pixelConversion + (state->viewWidth/2);
		thePoint.v = point.y * iw * pixelConversion + (state->viewHeight/2);
	}
	else {
		thePoint.h = point.x / orthographicScale * pixelConversion + (state->viewWidth/2);
		thePoint.v = point.y / orthographicScale * pixelConversion + (state->viewHeight/2);
	}
				
	if(buffer->deviceType == kQADeviceMemory) {
		memoryDevice = (TQADeviceMemory*) &buffer->device.memoryDevice;
		
		destRowBytes = memoryDevice->rowBytes;
		destBaseAddress = (Ptr) memoryDevice->baseAddr;
		
		if((thePoint.h < kPrecision) || (thePoint.h > memoryDevice->width - kPrecision))
		return;
		if((thePoint.v < kPrecision) || (thePoint.v > memoryDevice->height - kPrecision))
		return;
	}
	else if(buffer->deviceType == kQADeviceGDevice) {
		gDevice = buffer->device.gDevice;
		destPixMap = *(**gDevice).gdPMap;
		
		destRowBytes = destPixMap->rowBytes & 0x3FFF;
		destBaseAddress = destPixMap->baseAddr;
		
		if((thePoint.h < kPrecision) || (thePoint.h > destPixMap->bounds.right - kPrecision))
		return;
		if((thePoint.v < kPrecision) || (thePoint.v > destPixMap->bounds.bottom - kPrecision))
		return;
	}
	
	baseAddress = destBaseAddress + (thePoint.v - kPrecision) * destRowBytes + (thePoint.h - kPrecision) * 2;
	height = kPrecision * 2;
	do {
		width = kPrecision * 2;
		pixel = (unsigned short*) baseAddress;
		do {
			*pixel = Color16(31, 31, 0);
			++pixel;
		} while(--width);
		baseAddress += destRowBytes;
	} while(--height);
	
	baseAddress = destBaseAddress + (thePoint.v - kPrecision + kBorder) * destRowBytes + (thePoint.h - kPrecision + kBorder) * 2;
	height = (kPrecision  - kBorder) * 2;
	do {
		width = (kPrecision  - kBorder) * 2;
		pixel = (unsigned short*) baseAddress;
		do {
			*pixel = 0x0000;
			++pixel;
		} while(--width);
		baseAddress += destRowBytes;
	} while(--height);
}

static void Buffer_CompositingMethod(const TQADrawContext* drawContext, const TQADevice* buffer, const TQARect* dirtyRect, void* refCon)
{
	long				i;
	ShapePtr*			shapeList = &theObject.shapeList[0],
						shape = theObject.shapeList[shapeCell.v];
	Matrix				r1;
	MatrixPtr			camera = &currentCamera->camera;
	short				depth;
	TQADeviceMemory*	memoryDevice;
	GDHandle			gDevice;
	PixMapPtr			destPixMap;
	
	if(!object || (!theObject.shapeCount))
	return;
	
	Matrix_Negate(camera, &r1);
	Matrix_Cat(&theObject.pos, &r1, &r1);
	Shape_LinkMatrix(shape, &r1);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &r1, &r1);
	
	if(buffer->deviceType == kQADeviceMemory) {
		memoryDevice = (TQADeviceMemory*) &buffer->device.memoryDevice;
		if((memoryDevice->pixelType == kQAPixel_RGB16) || (memoryDevice->pixelType == kQAPixel_ARGB16))
		depth = 16;
		else if((memoryDevice->pixelType == kQAPixel_RGB32) || (memoryDevice->pixelType == kQAPixel_ARGB32))
		depth = 32;
		else
		return;
	}
	else if(buffer->deviceType == kQADeviceGDevice) {
		gDevice = buffer->device.gDevice;
		destPixMap = *(**gDevice).gdPMap;
		if(destPixMap->pixelSize == 16)
		depth = 16;
		else if(destPixMap->pixelSize == 32)
		depth = 32;
		else
		return;
	}
	
	if(depth == 16) {
		for(i = 0; i < selectedPoints; ++i)
		Buffer_DrawSelectedPoint16(buffer, globalState, theObject.shapeList[shapeCell.v], &r1, selectedPoint[i]);
	}
	else {
		for(i = 0; i < selectedPoints; ++i)
		Buffer_DrawSelectedPoint32(buffer, globalState, theObject.shapeList[shapeCell.v], &r1, selectedPoint[i]);
	}
}

static OSErr SetupDrawContext(GDHandle device, State *state, Rect bounds, Point offset)
{
	TQAError		err;
	Rect			deviceRect;
	TQADevice		qaDevice;
	TQARect 		qaBoundsRect;
	unsigned long flags;
	QDErr			e;
	TQANoticeMethod	noticeMethod;
	
	deviceRect = (**device).gdRect;
	state->window = nil;
	state->viewRect = bounds;
	state->viewHeight = bounds.bottom;
	state->viewWidth = bounds.right;
	state->engineVendorID = 0;

	qaDevice.deviceType = kQADeviceGDevice;
	qaDevice.device.gDevice = device;
	qaBoundsRect.left = bounds.left + kOffset + offset.h;
	qaBoundsRect.right = bounds.right + kOffset + offset.h;
	qaBoundsRect.top = bounds.top + kOffset + offset.v;
	qaBoundsRect.bottom = bounds.bottom + kOffset + offset.v;
	
	flags = 0;
	if(state->doubleBuffer)
	flags += kQAContext_DoubleBuffer;
	if(state->noZBuffer)
	flags += kQAContext_NoZBuffer;
	
	err = QADrawContextNew(&qaDevice, &qaBoundsRect, nil, state->engine, flags, &state->drawContext);
	if(err)
	return err;
	
	QASetInt(state->drawContext, kQATag_ZFunction, state->alwaysVisible ? kQAZFunction_True : kQAZFunction_LT);
	QASetInt(state->drawContext, kQATag_Antialias, state->antialiasing);
	QASetInt(state->drawContext, kQATag_PerspectiveZ, state->perspectiveZ ? kQAPerspectiveZ_On : kQAPerspectiveZ_Off);
	QASetInt(state->drawContext, kQATag_TextureFilter, state->textureFilter);
	QASetFloat(state->drawContext, kQATag_ColorBG_a, 1.0);
	QASetFloat(state->drawContext, kQATag_ColorBG_r, thePrefs.backColorRed);
	QASetFloat(state->drawContext, kQATag_ColorBG_g, thePrefs.backColorGreen);
	QASetFloat(state->drawContext, kQATag_ColorBG_b, thePrefs.backColorBlue);
	
	QASetInt(state->drawContext, kQATag_Blend, kQABlend_Interpolate);
	
#if 1
	noticeMethod.bufferNoticeMethod = (TQABufferNoticeMethod) Buffer_CompositingMethod;
	QASetNoticeMethod(state->drawContext, kQAMethod_BufferComposite, noticeMethod, nil);
#endif

	return noErr;
}

OSErr Init_Engine()
{
	Matrix				m;
	TQAEngine*			theEngine;
	OSErr				theError;
	Rect				destRect;
	Point				destPoint;
	Str255				string;
	GDHandle			mainDevice = GetMainDevice();
	TQADevice			raveDevice;
	long				i;
	
	raveDevice.deviceType = kQADeviceGDevice;
	raveDevice.device.gDevice = mainDevice;
	QAEngineEnable(kQAVendor_Apple, kQAEngine_AppleHW); //Enable Apple accelerator card
	theEngine = QADeviceGetFirstEngine(&raveDevice);
	if(theEngine == nil)
	return -2;
	if(thePrefs.raveEngineID != -1) {
		for(i = 0; i < thePrefs.raveEngineID; ++i)
		if(theEngine != nil)
		theEngine = QADeviceGetNextEngine(&raveDevice, theEngine);
		
		if(theEngine == nil) {
			theEngine = QADeviceGetFirstEngine(&raveDevice);
			thePrefs.raveEngineID = -1;
		}
		if(theEngine == nil)
		return -2;
	}
	
	QAEngineGestalt(theEngine, kQAGestalt_VendorID, &i);
	if(i == kQAVendor_ATI)
	fullViewAvailable = true;
	
	Matrix_Clear(&topCamera.camera);
	Matrix_SetRotateX(DegreesToRadians(90.0), &topCamera.camera);
	topCamera.camera.w.y = 20.0;
	
	Matrix_Clear(&frontCamera.camera);
	frontCamera.camera.w.z = -20.0;
	
	Matrix_Clear(&rightCamera.camera);
	Matrix_SetRotateY(DegreesToRadians(90.0), &rightCamera.camera);
	rightCamera.camera.w.x = -20.0;
	
	Matrix_Clear(&localCamera.camera);
	localCamera.camera.w.z = -20.0;
	
	localState = (State*) NewPtrClear(sizeof(State));
	if(localState == nil)
	return MemError();
	
	localState->d = 0.06;
	localState->f = 50.0;
	localState->h = localState->d * 0.2;
	localState->doubleBuffer = 1;
	localState->noZBuffer = 0;
	localState->alwaysVisible = 0;
	localState->perspectiveZ = 1;
	if(thePrefs.flags & kPref_FlagFiltering)
	localState->textureFilter = 1; //0->2
	else
	localState->textureFilter = 0;
	localState->antialiasing = 0; //0->3
		
	localState->lightVector.x = 0.0;
	localState->lightVector.y = thePrefs.local;
	localState->lightVector.z = 0.0;
	localState->ambient = thePrefs.ambient;
	
	localState->verticeList = nil;
	
	Matrix_SetRotateX(DegreesToRadians(-30.0), &m);
	Matrix_TransformVector(&m, &localState->lightVector, &localState->lightVector);
	Matrix_SetRotateZ(DegreesToRadians(30.0), &m);
	Matrix_TransformVector(&m, &localState->lightVector, &localState->lightVector);
	
	localState->engine = theEngine;
	
	SetRect(&destRect, 0, 0, 300, 200);
	destPoint.h = 304;
	destPoint.v = 1;
	theError = SetupDrawContext(mainDevice, localState, destRect, destPoint);
	if(theError)
	return theError;
	
	topState = (State*) NewPtrClear(sizeof(State));
	if(topState == nil)
	return MemError();
	BlockMove(localState, topState, sizeof(State));
	destPoint.h = 1;
	destPoint.v = 1;
	theError = SetupDrawContext(mainDevice, topState, destRect, destPoint);
	if(theError)
	return theError;
	
	frontState = (State*) NewPtrClear(sizeof(State));
	if(frontState == nil)
	return MemError();
	BlockMove(localState, frontState, sizeof(State));
	destPoint.h = 1;
	destPoint.v = 204;
	theError = SetupDrawContext(mainDevice, frontState, destRect, destPoint);
	if(theError)
	return theError;
	
	rightState = (State*) NewPtrClear(sizeof(State));
	if(rightState == nil)
	return MemError();
	BlockMove(localState, rightState, sizeof(State));
	destPoint.h = 304;
	destPoint.v = 204;
	theError = SetupDrawContext(mainDevice, rightState, destRect, destPoint);
	if(theError)
	return theError;
	
	globalState = (State*) NewPtrClear(sizeof(State));
	if(globalState == nil)
	return MemError();
	BlockMove(localState, globalState, sizeof(State));
	SetRect(&destRect, 0, 0, 603, 403);
	destPoint.h = 1;
	destPoint.v = 1;
	theError = SetupDrawContext(mainDevice, globalState, destRect, destPoint);
	if(theError)
	return theError;
	
	return noErr;
}