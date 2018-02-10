#include			<ATIRave.h>
#include			<Appearance.h>

#include			"Structures.h"
#include			"Editor Header.h"
#include			"Matrix.h"
#include			"Keys.h"

//CONSTANTES:

#define				rotatePerKey			0.01
#define				movePerKey				0.2
#define				kMove					0.05
#define				kMoveOrigin				0.02 / theObject.shapeList[shapeCell.v]->scale
#define				kRotate					1.0
#define				kRotateFast				3.0

#define				kSuspendResumeEvent		0x01000000
#define				kResumeEvent			0x00000001

#define				kColorDarkGrey			50000
#define				kColorLightGrey			57000
#define				kColorVeryDarkGrey		40000

#define				kInfinityPlugInType			'shlb'

//PROTOTYPES:

extern void Hide_MenuBar();
extern void Show_MenuBar();
extern pascal Boolean EventFilterProc(DialogPtr theDialog, EventRecord *theEvent, DialogItemIndex *itemHit);

//VARIABLES:

WindowPtr			mainWin,
					whichWin,
					mappingWin;
DialogPtr			textureWin,
					shapeWin,
					scriptWin,
					sequencerWin,
					whichDialog,
					theDialog;
GWorldPtr			textureGWorld;
ControlHandle		sequencerScroll,
					timeScroll;
MenuHandle			menu[6],
					texturePopUpMenu,
					shapePopUpMenu,
					scriptPopUpMenu,
					sequencerPopUpMenu;
Pattern				offPattern;
					
EventRecord			event;
Boolean			run = true,
					isForeGround = true,
					filled = true,
					normals = false,
					mappingVisible = false,
					shapeVisible = true,
					textureVisible = true,
					scriptVisible = true,
					sequencerVisible = false,
					selectedOnly = false,
					fullViewAvailable = false,
					fullView = false;
short				special = 3;
char				theChar;
short				whereClick;
Rect				dragRect = (**LMGetGrayRgn()).rgnBBox;
Str63				theString;
ListHandle			textureListHandle,
					shapeListHandle,
					scriptListHandle;
Cell				theCell,
					shapeCell = {0,0},
					textureCell = {0,0},
					scriptCell = {0,0},
					oldShapeCell = {-1,-1};
short				animationCell = -1,
					firstAnimation = 0;
CursHandle			keyCursor,
					waitCursor;
RGBColor			veryDarkGrey = {kColorVeryDarkGrey, kColorVeryDarkGrey, kColorVeryDarkGrey},
					darkGrey = {kColorDarkGrey, kColorDarkGrey, kColorDarkGrey},
					lightGrey = {kColorLightGrey, kColorLightGrey, kColorLightGrey};
long				lastClick,
					currentTime = 0,
					firstTime = 0;
Preferences			thePrefs;
				
short				mode = 0,
					curView = 1,
					curTexWidth,
					curTexHeight;
Object				theObject;
Boolean			object = false,
					zoom = false,
					orthographic = false;
float				orthographicScale = 16.0;
CameraState			localCamera,
					topCamera,
					frontCamera,
					rightCanera;
CameraState*		currentCamera;
StatePtr			localState,
					topState,
					frontState,
					rightState,
					globalState;
float				objectRotateX = 0.0,
					objectRotateY = 0.0,
					objectRotateZ = 0.0;
long				selectedPoints = 0,
					selectedPoint[50];

UniversalProcPtr	ODOC_Handler_Routine,
					QUIT_Handler_Routine;

Rect				popUpRect = {0, 158 - 16, 16, 156},
					popUpRect2 = {0, 702, 16, 716},
					viewRect = {16,0,220, 156 - 15},
					viewRect2 = {16, 0, 128, kWitdh},
					nameRect = {16, 0, 128, kNameSize},
					timeRect = {16, kNameSize, 128, kWitdh};
					
long				textureCount = 0;
TQATexture*			textureList[kMaxShapes];
TextureStoragePtr	textureStorageList[kMaxShapes];
OSType				nameList[kMaxShapes];

long				scriptCount;
ScriptPtr			scriptList[kMaxScripts],
					currentScript = nil;
UnsignedWide		lastFrameTime;
PicHandle			previewPic = nil;

//FONCTIONS:

//UTILITAIRES:

static void Show_AboutDialog(short num)
{
	short		itemHit;
	
	theDialog = GetNewDialog(1000 + num, nil, (WindowPtr) -1);
	DrawDialog(theDialog);
	while(!Button())
	;
	FlushEvents(everyEvent, 0);
	DisposeDialog(theDialog);
}
	
void Do_Error(OSErr theError, short explanationTextID)
{
	AlertStdAlertParamRec	params;
	short					outItemHit,
							ID;
	Str31					errorNum;
	Str255					text1,
							text2;
	UniversalProcPtr		EventFilterRoutine = NewModalFilterProc(EventFilterProc);
	
	params.movable = true;
	params.helpButton = false;
	params.filterProc = EventFilterRoutine;
	params.defaultText = (StringPtr) kAlertDefaultOKText;
	params.cancelText = nil;
	params.otherText = nil;
	params.defaultButton = kAlertStdAlertOKButton;
	params.cancelButton = 0;
	params.position = kWindowDefaultPosition;
	
	GetIndString(text1, 200, 1);
	NumToString(theError, errorNum);
	BlockMove(&errorNum[1], &text1[text1[0] + 1], errorNum[0]);
	text1[0] += errorNum[0];
	ID = explanationTextID / 100;
	GetIndString(text2, ID * 100, explanationTextID - ID * 100);
	StandardAlert(kAlertStopAlert, text1, text2, &params, &outItemHit);
}
	
PicHandle Take_VRAMCapture(GDHandle device, Rect* copyRect, Rect* finalRect)
{
	PicHandle			thePic = nil;
	GWorldPtr			oldGWorld;
	GDHandle			oldGDHandle;
	GWorldPtr			tempGWorld;
	Rect				tempRect;
	PixMapPtr			sourcePixMap,
						destPixMap;
	Ptr					sourceBaseAddress,
						destBaseAddress;
	long				height,
						sourceRowBytes,
						destRowBytes;
	
	//Create temp GWorld
	GetGWorld(&oldGWorld, &oldGDHandle);
	tempRect = *copyRect;
	OffsetRect(&tempRect, -tempRect.left, -tempRect.top);
	sourcePixMap = *((**device).gdPMap);
	if(NewGWorld(&tempGWorld, sourcePixMap->pixelSize, &tempRect, NULL, NULL, NULL))
	return nil;
	SetGWorld(tempGWorld, nil);
	LockPixels(GetGWorldPixMap(tempGWorld));
	BackColor(whiteColor);
	ForeColor(blackColor);
	destPixMap = *(tempGWorld->portPixMap);
	
	//Copy VRAM data
	sourceRowBytes = sourcePixMap->rowBytes & 0x3FFF;
	destRowBytes = destPixMap->rowBytes & 0x3FFF;
	sourceBaseAddress = sourcePixMap->baseAddr + copyRect->top * sourceRowBytes + copyRect->left * sourcePixMap->pixelSize / 8;
	destBaseAddress = destPixMap->baseAddr;
	height = copyRect->bottom - copyRect->top;
	do {
		BlockMove(sourceBaseAddress, destBaseAddress, (copyRect->right - copyRect->left) * sourcePixMap->pixelSize / 8);
		sourceBaseAddress += sourceRowBytes;
		destBaseAddress += destRowBytes;
	} while(--height);
	
	//Resize picture
	if(finalRect->top != -1)
	CopyBits(GWBitMapPtr(tempGWorld), GWBitMapPtr(tempGWorld), &tempRect, finalRect, srcCopy, nil);
	else
	*finalRect = tempRect;
	
	//Create picture
	thePic = OpenPicture(&tempRect);
	CopyBits(GWBitMapPtr(tempGWorld), GWBitMapPtr(tempGWorld), finalRect, finalRect, srcCopy, nil);
	ClosePicture();
	(**thePic).picFrame = *finalRect;
	
	//Clean up
	UnlockPixels(GetGWorldPixMap(tempGWorld));
	DisposeGWorld(tempGWorld);
	SetGWorld(oldGWorld, oldGDHandle);
	
	return thePic;
}

static OSErr Take_ScreenShot()
{
	Rect					copyRect,
							finalRect;
	PicHandle				pic;
	StandardFileReply		theReply;
	OSErr					theError;
	short					destFileID;
	long					bytes;
	
	SetRect(&copyRect, 32, 32, 637, 437);
	SetRect(&finalRect, -1, -1, -1, -1);
	pic = Take_VRAMCapture(GetMainDevice(), &copyRect, &finalRect);
	if(!pic)
	return -1;
	
	StandardPutFile("\pName of the PICT file:", "\pScreenshot", &theReply);
	if(!theReply.sfGood) {
		DisposeHandle((Handle) pic);
		return noErr;
	}
	if(theReply.sfReplacing) {
		theError = FSpDelete(&theReply.sfFile);
		if(theError) {
			DisposeHandle((Handle) pic);
			return theError;
		}
	}
	
	theError = FSpCreate(&theReply.sfFile, 'ttxt', 'PICT', smSystemScript);
	if(theError)
	return theError;
	
	theError = FSpOpenDF(&theReply.sfFile, fsRdWrPerm, &destFileID);
	if(theError)
	return theError;
	SetEOF(destFileID, 512);
	SetFPos(destFileID, fsFromStart, 512);
	bytes = GetHandleSize((Handle) pic);
	HLock((Handle) pic);
	FSWrite(destFileID, &bytes, *pic);
	HUnlock((Handle) pic);
	
	FSClose(destFileID);
	DisposeHandle((Handle) pic);
	
	return noErr;
}

//DRAWING:

void Check_ShapeMapping()
{
	long				i;
	GrafPtr				savePort;
	
	if(!object || (theObject.shapeCount == 0)) {
		mappingVisible = false;
		ShowHide(mappingWin, false);
		return;
	}
	
	if(theObject.shapeList[shapeCell.v]->texture != kNoTexture) {
		mappingVisible = true;
		for(i = 0; i < textureCount; ++i)
		if(theObject.shapeList[shapeCell.v]->texture == nameList[i]) {
			curTexWidth = textureStorageList[i]->width;
			curTexHeight = textureStorageList[i]->height;
			if(zoom)
			SizeWindow(mappingWin, curTexWidth * 2, curTexHeight * 2, true);
			else
			SizeWindow(mappingWin, curTexWidth, curTexHeight, true);
		}
		ShowHide(mappingWin, true);
		GetPort(&savePort);
		SetPort(mappingWin);
		Update_MappingWin();
		SetPort(savePort);
	}
	else {
		mappingVisible = false;
		ShowHide(mappingWin, false);
	}
}

//GESTION EVENEMENTS:

static void Switch_FullView()
{
	GDHandle		mainDevice = GetMainDevice();
	Rect*			screenRect = &(**mainDevice).gdRect;
	WindowPtr		fullWindow;
	
	if(!fullView) {
		QASetInt(globalState->drawContext, (TQATagInt) kATIScaleBlit, true);
		QASetInt(globalState->drawContext, (TQATagInt) kATIScaleLeft, 0);
		QASetInt(globalState->drawContext, (TQATagInt) kATIScaleRight, screenRect->right);
		QASetInt(globalState->drawContext, (TQATagInt) kATIScaleTop, 0);
		QASetInt(globalState->drawContext, (TQATagInt) kATIScaleBottom, screenRect->bottom);
		Hide_MenuBar();
		HideCursor();
	}
	else {
		QASetInt(globalState->drawContext, (TQATagInt) kATIScaleBlit, false);
		fullWindow = NewWindow(nil, screenRect, "\p", true, 0, (WindowPtr) -1, false, 0);
		DisposeWindow(fullWindow);
		Show_MenuBar();
		ShowCursor();
	}
	fullView = !fullView;
}
	
static void Do_Menu(long result)
{
	short			theItem = LoWord(result),
					theMenu = HiWord(result);
	OSErr			theError;
	
	switch(theMenu) {
	
		case appleID:
		if(theItem == 1)
		Show_AboutDialog(0);
		else if(theItem == 2)
		Show_AboutDialog(1);
		else {
			GetMenuItemText(menu[appleM], theItem, theString);
			OpenDeskAcc(theString);
		}
		break;
		
		case fileID:
		switch(theItem) {
		
			case 1:
			theError = Object_New();
			if(theError) {
				if(theError != -1)
				Do_Error(theError, 0);
				break;
			}
			DisableItem(menu[1], 1);
			DisableItem(menu[1], 2);
			EnableItem(menu[1], 3);
			EnableItem(menu[1], 5);
			EnableItem(menu[1], 7);
			EnableItem(texturePopUpMenu, 0);
			EnableItem(shapePopUpMenu, 0);
			EnableItem(scriptPopUpMenu, 0);
			DisableItem(shapePopUpMenu, 2);
			DisableItem(shapePopUpMenu, 3);
			DisableItem(shapePopUpMenu, 4);
			DisableItem(shapePopUpMenu, 6);
			DisableItem(shapePopUpMenu, 7);
			DisableItem(shapePopUpMenu, 9);
			DisableItem(shapePopUpMenu, 10);
			DisableItem(shapePopUpMenu, 11);
			DisableItem(shapePopUpMenu, 13);
			DisableItem(shapePopUpMenu, 14);
			DisableItem(shapePopUpMenu, 16);
			DisableItem(shapePopUpMenu, 17);
			Check_ShapeMapping();
			break;
			
			case 2:
			theError = Object_Open(false, false);
			if(theError) {
				//if(theError != -1)
				//Do_Error(theError, 0);
				break;
			}
			DisableItem(menu[1], 1);
			DisableItem(menu[1], 2);
			EnableItem(menu[1], 3);
			EnableItem(menu[1], 3);
			EnableItem(menu[1], 5);
			EnableItem(menu[1], 7);
			EnableItem(texturePopUpMenu, 0);
			EnableItem(shapePopUpMenu, 0);
			EnableItem(scriptPopUpMenu, 0);
			Check_ShapeMapping();
			break;
			
			case 3:
			if(fullView)
			Switch_FullView();
			theError = Object_Close();
			if(theError) {
				if(theError != -1)
				Do_Error(theError, 0);
				break;
			}
			EnableItem(menu[1], 1);
			EnableItem(menu[1], 2);
			DisableItem(menu[1], 3);
			DisableItem(menu[1], 5);
			DisableItem(menu[1], 7);
			DisableItem(texturePopUpMenu, 0);
			DisableItem(shapePopUpMenu, 0);
			DisableItem(scriptPopUpMenu, 0);
			Check_ShapeMapping();
			if(sequencerVisible) {
				ShowHide(sequencerWin, false);
				sequencerVisible = false;
			}
			break;
			
			case 5:
			if(fullView)
			Switch_FullView();
			theError = Object_Save();
			if(theError) {
				if(theError != -1)
				Do_Error(theError, 104);
				break;
			}
			break;
			
			case 7:
			if(event.modifiers & optionKey)
			theError = Object_Open(true, true);
			else
			theError = Object_Open(true, false);
			if(theError) {
				//if(theError != -1)
				//Do_Error(theError, 0);
				break;
			}
			EnableItem(shapePopUpMenu, 2);
			EnableItem(shapePopUpMenu, 3);
			EnableItem(shapePopUpMenu, 4);
			EnableItem(shapePopUpMenu, 6);
			EnableItem(shapePopUpMenu, 7);
			EnableItem(shapePopUpMenu, 9);
			EnableItem(shapePopUpMenu, 10);
			EnableItem(shapePopUpMenu, 11);
			EnableItem(shapePopUpMenu, 13);
			EnableItem(shapePopUpMenu, 14);
			EnableItem(shapePopUpMenu, 16);
			EnableItem(shapePopUpMenu, 17);
			Check_ShapeMapping();
			break;
			
			case 9:
			if(fullView)
			Switch_FullView();
			run = false;
			break;
		
		}
		break;
		
		case editID:
		switch(theItem) {
		
			case 8:
			Preferences_Setting(&thePrefs);
			break;
			
		}
		break;
		
		case renderID:
		switch(theItem) {
		
			case 1:
			orthographic = false;
			CheckItem(menu[2], 1, true);
			CheckItem(menu[2], 2, false);
			break;
			
			case 2:
			orthographic = true;
			CheckItem(menu[2], 1, false);
			CheckItem(menu[2], 2, true);
			break;
			
			case 3:
			orthographicScale /= 2;
			break;
			
			case 4:
			orthographicScale *= 2;
			break;
			
			case 6:
			filled = true;
			CheckItem(menu[2], 6, true);
			CheckItem(menu[2], 7, false);
			break;
			
			case 7:
			filled = false;
			CheckItem(menu[2], 6, false);
			CheckItem(menu[2], 7, true);
			break;
			
			case 9:
			normals = !normals;
			CheckItem(menu[2], 9, normals);
			break;
			
			case 10:
			selectedOnly = !selectedOnly;
			CheckItem(menu[2], 10, selectedOnly);
			break;
			
		}
		break;
		
		case viewID:
		switch(theItem) {
		
			case 1:
			if(fullView)
			break;
			mode = 0;
			CheckItem(menu[3], 1, true);
			CheckItem(menu[3], 3, false);
			CheckItem(menu[3], 4, false);
			CheckItem(menu[3], 5, false);
			CheckItem(menu[3], 6, false);
			EraseRect(&mainWin->portRect);
			DisableItem(menu[3], 8);
			break;
			
			case 3:
			mode = 1;
			curView = 1;
			CheckItem(menu[3], 1, false);
			CheckItem(menu[3], 3, true);
			CheckItem(menu[3], 4, false);
			CheckItem(menu[3], 5, false);
			CheckItem(menu[3], 6, false);
			currentCamera = &topCamera;
			if(fullViewAvailable)
			EnableItem(menu[3], 8);
			break;
			
			case 4:
			mode = 1;
			curView = 2;
			CheckItem(menu[3], 1, false);
			CheckItem(menu[3], 3, false);
			CheckItem(menu[3], 4, true);
			CheckItem(menu[3], 5, false);
			CheckItem(menu[3], 6, false);
			currentCamera = &frontCamera;
			if(fullViewAvailable)
			EnableItem(menu[3], 8);
			break;
			
			case 5:
			mode = 1;
			curView = 3;
			CheckItem(menu[3], 1, false);
			CheckItem(menu[3], 3, false);
			CheckItem(menu[3], 4, false);
			CheckItem(menu[3], 5, true);
			CheckItem(menu[3], 6, false);
			currentCamera = &rightCamera;
			if(fullViewAvailable)
			EnableItem(menu[3], 8);
			break;
			
			case 6:
			mode = 1;
			curView = 4;
			CheckItem(menu[3], 1, false);
			CheckItem(menu[3], 3, false);
			CheckItem(menu[3], 4, false);
			CheckItem(menu[3], 5, false);
			CheckItem(menu[3], 6, true);
			currentCamera = &localCamera;
			if(fullViewAvailable)
			EnableItem(menu[3], 8);
			break;
			
			case 8:
			if(object)
			Switch_FullView();
			break;
			
			case 10:
			Take_ScreenShot();
			break;
			
			case 11:
			Rect			copyRect,
							finalRect;
	
			if(previewPic != nil)
			DisposeHandle((Handle) previewPic);
			SetRect(&copyRect, 33, 33, 636, 436);
			SetRect(&finalRect, 0, 0, kPreviewH, kPreviewV);
			previewPic = Take_VRAMCapture(GetMainDevice(), &copyRect, &finalRect);
			break;
		
		}
		break;
		
		case windowID:
		switch(theItem) {
		
			case 1:
			shapeVisible = !shapeVisible;
			ShowHide(shapeWin, shapeVisible);
			CheckItem(menu[4], 1, shapeVisible);
			break;
			
			case 2:
			textureVisible = !textureVisible;
			ShowHide(textureWin, textureVisible);
			CheckItem(menu[4], 2, textureVisible);
			break;
			
			case 3:
			scriptVisible = !scriptVisible;
			ShowHide(scriptWin, scriptVisible);
			CheckItem(menu[4], 3, scriptVisible);
			break;
		
		}
		break;
		
	}
	
	HiliteMenu(0);		
}

static void Do_MouseDown(EventRecord theEvent)
{
	whereClick = FindWindow(theEvent.where, &whichWin);
	
	switch(whereClick) {
		
		case inDrag:
		dragRect = (**LMGetGrayRgn()).rgnBBox;
		DragWindow(whichWin, theEvent.where, &dragRect);
		break;
		
		case inMenuBar:
		Do_Menu(MenuSelect(theEvent.where));
		break;
		
		case inGoAway:
		if(!TrackGoAway(whichWin, theEvent.where))
		break;
		if(whichWin == sequencerWin) {
			ShowHide(sequencerWin, false);
			sequencerVisible = false;
			if(currentScript->flags & kFlag_Running)
			Script_Stop(currentScript);
			currentScript = nil;
		}
		if(whichWin == shapeWin) {
			shapeVisible = !shapeVisible;
			ShowHide(shapeWin, shapeVisible);
			CheckItem(menu[4], 1, shapeVisible);
		}
		if(whichWin == textureWin) {
			textureVisible = !textureVisible;
			ShowHide(textureWin, textureVisible);
			CheckItem(menu[4], 2, textureVisible);
		}
		if(whichWin == scriptWin) {
			scriptVisible = !scriptVisible;
			ShowHide(scriptWin, scriptVisible);
			CheckItem(menu[4], 3, scriptVisible);
		}
		break;
		
		case inZoomIn:
		case inZoomOut:
		if(!TrackBox(whichWin, theEvent.where, whereClick))
		break;
		zoom = !zoom;
		Check_ShapeMapping();
		break;
		
		case inContent:
		SetPort(whichWin);
		GlobalToLocal(&theEvent.where);
		if(whichWin == mainWin)
		Do_MainWin(theEvent.where);
		if(whichWin == textureWin)
		Do_TextureWin(theEvent.where);
		if(whichWin == shapeWin)
		Do_ShapeWin(theEvent.where);
		if(whichWin == scriptWin)
		Do_ScriptWin(theEvent.where);
		if(whichWin == mappingWin)
		Do_MappingWin(theEvent.where);
		if(whichWin == sequencerWin)
		Do_SequencerWin(theEvent.where);
		lastClick = theEvent.when;
		break;
		
	}
}

static void PerFrameKeyCheck_Camera()
{
	KeyMap			theKeys;
	int				command,
					option,
					control;

	GetKeys(theKeys);

	command = IsKeyDown(theKeys, keyCommand);
	option = IsKeyDown(theKeys, keyOption);
	control = IsKeyDown(theKeys, keyControl);
	
	if(IsKeyDown(theKeys, keyArrowLeft)) {
		if(option)
		objectRotateZ -= DegreesToRadians(kRotateFast);
		else if(command)
		localCamera.camera.w.x += movePerKey;
		else
		objectRotateY += DegreesToRadians(kRotateFast);
	}
	
	if(IsKeyDown(theKeys, keyArrowRight)) {
		if(option)
		objectRotateZ += DegreesToRadians(kRotateFast);
		else if(command)
		localCamera.camera.w.x -= movePerKey;
		else
		objectRotateY -= DegreesToRadians(kRotateFast);
	}
	
	if(IsKeyDown(theKeys, keyArrowUp)) {
		if(option)
		localCamera.camera.w.z += movePerKey;
		else if(command)
		localCamera.camera.w.y += movePerKey;
		else
		objectRotateX += DegreesToRadians(kRotateFast);
	}

	if(IsKeyDown(theKeys, keyArrowDown)) {
		if(option)
		localCamera.camera.w.z -= movePerKey;
		else if(command)
		localCamera.camera.w.y -= movePerKey;
		else
		objectRotateX -= DegreesToRadians(kRotateFast);
	}
}

static void PerFrameKeyCheck_Top()
{
	KeyMap			theKeys;
	int				command,
					option,
					control;

	GetKeys(theKeys);

	command = IsKeyDown(theKeys, keyCommand);
	option = IsKeyDown(theKeys, keyOption);
	control = IsKeyDown(theKeys, keyControl);
	
	if(IsKeyDown(theKeys, keyArrowLeft)) {
		if(command)
		topCamera.camera.w.x += movePerKey;
		else if(option) {
			theObject.shapeList[shapeCell.v]->rotateY += DegreesToRadians(kRotate);
			Shape_UpdateMatrix(theObject.shapeList[shapeCell.v]);
		} else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], kMoveOrigin, 0.0, 0.0);
		else
		theObject.shapeList[shapeCell.v]->pos.w.x += kMove;
	}

	if(IsKeyDown(theKeys, keyArrowRight)) {
		if(command)
		topCamera.camera.w.x -= movePerKey;
		else if(option) {
			theObject.shapeList[shapeCell.v]->rotateY -= DegreesToRadians(kRotate);
			Shape_UpdateMatrix(theObject.shapeList[shapeCell.v]);
		} else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], -kMoveOrigin, 0.0, 0.0);
		else
		theObject.shapeList[shapeCell.v]->pos.w.x -= kMove;
	}

	if(IsKeyDown(theKeys, keyArrowUp)) {
		if(command)
		topCamera.camera.w.z += movePerKey;
		else if(option)
		topCamera.camera.w.y -= movePerKey;
		else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], 0.0, 0.0, kMoveOrigin);
		else
		theObject.shapeList[shapeCell.v]->pos.w.z += kMove;
	}

	if(IsKeyDown(theKeys, keyArrowDown)) {
		if(command)
		topCamera.camera.w.z -= movePerKey;
		else if(option)
		topCamera.camera.w.y += movePerKey;
		else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], 0.0, 0.0, -kMoveOrigin);
		else
		theObject.shapeList[shapeCell.v]->pos.w.z -= kMove;
	}
}

static void PerFrameKeyCheck_Front()
{
	KeyMap			theKeys;
	int				command,
					option,
					control;

	GetKeys(theKeys);

	command = IsKeyDown(theKeys, keyCommand);
	option = IsKeyDown(theKeys, keyOption);
	control = IsKeyDown(theKeys, keyControl);
	
	if(IsKeyDown(theKeys, keyArrowLeft)) {
		if(command)
		frontCamera.camera.w.x += movePerKey;
		else if(option) {
			theObject.shapeList[shapeCell.v]->rotateZ -= DegreesToRadians(kRotate);
			Shape_UpdateMatrix(theObject.shapeList[shapeCell.v]);
		} else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], kMoveOrigin, 0.0, 0.0);
		else
		theObject.shapeList[shapeCell.v]->pos.w.x += kMove;
	}

	if(IsKeyDown(theKeys, keyArrowRight)) {
		if(command)
		frontCamera.camera.w.x -= movePerKey;
		else if(option) {
			theObject.shapeList[shapeCell.v]->rotateZ += DegreesToRadians(kRotate);
			Shape_UpdateMatrix(theObject.shapeList[shapeCell.v]);
		} else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], -kMoveOrigin, 0.0, 0.0);
		else
		theObject.shapeList[shapeCell.v]->pos.w.x -= kMove;
	}

	if(IsKeyDown(theKeys, keyArrowUp)) {
		if(command)
		frontCamera.camera.w.y += movePerKey;
		else if(option)
		frontCamera.camera.w.z += movePerKey;
		else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], 0.0, kMoveOrigin, 0.0);
		else
		theObject.shapeList[shapeCell.v]->pos.w.y += kMove;
	}

	if(IsKeyDown(theKeys, keyArrowDown)) {
		if(command)
		frontCamera.camera.w.y -= movePerKey;
		else if(option)
		frontCamera.camera.w.z -= movePerKey;
		else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], 0.0, -kMoveOrigin, 0.0);
		else
		theObject.shapeList[shapeCell.v]->pos.w.y -= kMove;
	}
}

static void PerFrameKeyCheck_Right()
{
	KeyMap			theKeys;
	int				command,
					option,
					control;

	GetKeys(theKeys);

	command = IsKeyDown(theKeys, keyCommand);
	option = IsKeyDown(theKeys, keyOption);
	control = IsKeyDown(theKeys, keyControl);
	
	if(IsKeyDown(theKeys, keyArrowLeft)) {
		if(command)
		rightCamera.camera.w.z -= movePerKey;
		else if(option) {
			theObject.shapeList[shapeCell.v]->rotateX -= DegreesToRadians(kRotate);
			Shape_UpdateMatrix(theObject.shapeList[shapeCell.v]);
		} else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], 0.0, 0.0, -kMoveOrigin);
		else
		theObject.shapeList[shapeCell.v]->pos.w.z -= kMove;
	}

	if(IsKeyDown(theKeys, keyArrowRight)) {
		if(command)
		rightCamera.camera.w.z += movePerKey;
		else if(option) {
			theObject.shapeList[shapeCell.v]->rotateX += DegreesToRadians(kRotate);
			Shape_UpdateMatrix(theObject.shapeList[shapeCell.v]);
		} else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], 0.0, 0.0, kMoveOrigin);
		else
		theObject.shapeList[shapeCell.v]->pos.w.z += kMove;
	}

	if(IsKeyDown(theKeys, keyArrowUp)) {
		if(command)
		rightCamera.camera.w.y += movePerKey;
		else if(option)
		rightCamera.camera.w.x += movePerKey;
		else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], 0.0, kMoveOrigin, 0.0);
		else
		theObject.shapeList[shapeCell.v]->pos.w.y += kMove;
	}

	if(IsKeyDown(theKeys, keyArrowDown)) {
		if(command)
		rightCamera.camera.w.y -= movePerKey;
		else if(option)
		rightCamera.camera.w.x -= movePerKey;
		else if(control)
		Shape_MoveOrigin(theObject.shapeList[shapeCell.v], 0.0, -kMoveOrigin, 0.0);
		else
		theObject.shapeList[shapeCell.v]->pos.w.y -= kMove;
	}
}
	
static void Event_Loop()
{
	Rect				frameRateRect = {130,0,145,50};
	GrafPtr				savePort;
	UnsignedWide		frameTime;
	float				framesPerSecond;
	Str31				frameRate;
	
	while(run) {
		
		if(WaitNextEvent(everyEvent, &event, (long) 0, 0))
		switch(event.what) {
			
			case mouseDown:
			Do_MouseDown(event);
			break;
			
			case keyDown:
			case autoKey:
			theChar = (event.message & charCodeMask);
			if(event.modifiers & cmdKey)
			Do_Menu(MenuKey(theChar));
			break;
			
			case updateEvt:
			GetPort(&savePort);
			
			//SetGWorld((CGrafPtr) mainWin, nil);
			BeginUpdate(mainWin);
				Update_MainWin();
			EndUpdate(mainWin);
			
			//SetGWorld((CGrafPtr) textureWin, nil);
			BeginUpdate(textureWin);
				Update_TextureWin();
			EndUpdate(textureWin);
			
			//SetGWorld((CGrafPtr) shapeWin, nil);
			BeginUpdate(shapeWin);
				Update_ShapeWin();
			EndUpdate(shapeWin);
			
			//SetGWorld((CGrafPtr) scriptWin, nil);
			BeginUpdate(scriptWin);
				Update_ScriptWin();
			EndUpdate(scriptWin);
			
			//SetGWorld((CGrafPtr) mappingWin, nil);
			BeginUpdate(mappingWin);
				Update_MappingWin();
			EndUpdate(mappingWin);
			
			//SetGWorld((CGrafPtr) sequencerWin, nil);
			BeginUpdate(sequencerWin);
				Update_SequencerWin();
			EndUpdate(sequencerWin);
			
			SetPort(savePort);
			break;
			
			case osEvt:
			if(event.message & kSuspendResumeEvent) {
				if(event.message & kResumeEvent) {
					HiliteWindow(mainWin, true);
					if(shapeVisible)
					ShowHide(shapeWin, true);
					if(textureVisible)
					ShowHide(textureWin, true);
					if(scriptVisible)
					ShowHide(scriptWin, true);
					if(mappingVisible)
					ShowHide(mappingWin, true);
					if(sequencerVisible)
					ShowHide(sequencerWin, true);
					isForeGround = true;
				}
				else {
					if(fullView)
					Switch_FullView();
					EraseRect(&mainWin->portRect);
					HiliteWindow(mainWin, false);
					if(shapeVisible)
					ShowHide(shapeWin, false);
					if(textureVisible)
					ShowHide(textureWin, false);
					if(scriptVisible)
					ShowHide(scriptWin, false);
					if(mappingVisible)
					ShowHide(mappingWin, false);
					if(sequencerVisible)
					ShowHide(sequencerWin, false);
					InitCursor();
					isForeGround = false;
				}
			}
			break;
			
			case kHighLevelEvent:
			if((OSType) event.message == typeAppleEvent)
			AEProcessAppleEvent(&event);
			break;
		
		}
		
		if(sequencerVisible && (currentScript->flags & kFlag_Running)) {
			Script_Running(currentScript);
#if 1
			Microseconds(&frameTime);
			framesPerSecond = 1000000.0 / (frameTime.lo - lastFrameTime.lo);
			NumToString(100 * framesPerSecond, frameRate);
			frameRate[frameRate[0] + 1] = frameRate[frameRate[0]];
			frameRate[frameRate[0]] = frameRate[frameRate[0] - 1];
			frameRate[frameRate[0] - 1] = '.';
			++frameRate[0];
			if(sequencerVisible) {
				GetPort(&savePort);
				SetPort(sequencerWin);
				EraseRect(&frameRateRect);
				MoveTo(4,140);
				DrawString(frameRate);
				SetPort(savePort);
			}
			lastFrameTime = frameTime;
#endif
			//Update_TimePointer();
		}
		
		if(isForeGround) {
			if(object && theObject.shapeCount)
			switch(curView) {
				case 1: PerFrameKeyCheck_Top(); break;
				case 2: PerFrameKeyCheck_Front(); break;
				case 3: PerFrameKeyCheck_Right(); break;
				case 4: PerFrameKeyCheck_Camera(); break;
			}
			Update_MainWin();
		}
	}
}

//MAIN:
					
pascal OSErr ODOC_Handler(AppleEvent* theAppleEvent, AppleEvent* reply, long handlerRefCon)
{	
	AEDescList		docList;
	AEKeyword		keyword;
	DescType		returnedType;
	FSSpec			theFSSpec;
	Size			actualSize;
	FInfo			fileInfo;
	long			nbItems;
	int				i;
	OSErr			theError;
	
	theError = AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &docList);
	if(theError)
	return theError;
	theError = AECountItems(&docList, &nbItems);
	if(theError)
	return theError;
	
	for(i = 1; i <= nbItems; ++i) {
		theError = AEGetNthPtr(&docList, i, typeFSS, &keyword, &returnedType, (Ptr) &theFSSpec, sizeof(FSSpec), &actualSize);
		if(theError)
		return theError;
		
		theError = FSpGetFInfo(&theFSSpec, &fileInfo);
		if(theError)
		return theError;
		
		if((fileInfo.fdType == k3DFileType) && !object) {
			theError = Object_Load(&theFSSpec);
			if(theError) {
				//Do_Error(theError, 0);
				break;
			}
			DisableItem(menu[1], 1);
			DisableItem(menu[1], 2);
			EnableItem(menu[1], 3);
			EnableItem(menu[1], 3);
			EnableItem(menu[1], 5);
			EnableItem(menu[1], 7);
			EnableItem(texturePopUpMenu, 0);
			EnableItem(shapePopUpMenu, 0);
			EnableItem(scriptPopUpMenu, 0);
			Check_ShapeMapping();
		}
	}
	
	AEDisposeDesc(&docList);
	
	return nil;
}

pascal OSErr QUIT_Handler(AppleEvent* theAppleEvent, AppleEvent* reply, long handlerRefCon)
{
	run = false;

	return nil;
}

static Point GetWindowPosition(WindowPtr window)
{
	GrafPtr				savePort;
	Point				thePoint = {0,0};
	
	GetPort(&savePort);
	SetPort(window);
	LocalToGlobal(&thePoint);
	SetPort(savePort);
	
	return thePoint;
}
	
static Boolean Init_Appeareance()
{
	OSErr					theError;
	long					response;
	
	//Do we have Appeareance Manager installed?
	theError = Gestalt(gestaltAppearanceAttr,&response);
	if((theError == noErr) && (BitTst(&response,31 - gestaltAppearanceExists))) {
    	theError = RegisterAppearanceClient();
    	if(theError)
    	return false;
    	else
    	return true;
	}
	
	return false;
}
	
static Boolean Rave_Available()
{
	AlertStdAlertParamRec	params;
	short					outItemHit;
	Str255					text1,
							text2;
	
	if(QADrawContextNew == kUnresolvedCFragSymbolAddress) {
		params.movable = true;
		params.helpButton = false;
		params.filterProc = nil;
		params.defaultText = "\pQuit";
		params.cancelText = nil;
		params.otherText = nil;
		params.defaultButton = kAlertStdAlertOKButton;
		params.cancelButton = 0;
		params.position = kWindowDefaultPosition;
		
		GetIndString(text1, 200, 2);
		GetIndString(text2, 200, 3);
		StandardAlert(kAlertStopAlert, text1, text2, &params, &outItemHit);
		
		return false;
	}
	
	return true;
}

static Load_InfinitySoftwareEngine(FSSpec* theFile)
{
	CFragConnectionID		connID;
	Ptr						mainAddress;
	Str255					errorMessage;
	OSErr					theError;
	ProcPtr				InitRoutine;
	CFragSymbolClass		theClass;
	
	theError = GetDiskFragment(theFile, 0, 0, nil, kPrivateCFragCopy, &connID, &mainAddress, errorMessage);
	if(theError)
	return theError;
	
	theError = FindSymbol(connID, "\pRavePlugIn_InitFunction__Fv", (Ptr*) &InitRoutine, &theClass);
	if(theError)
	return theError;
	
	(*InitRoutine)();
	
	return noErr;
}
	
OSErr ScanForFolder(short volumeID, long parentFolderID, Str63 folderName, long* folderID)
{
	short			idx;
	CInfoPBRec		cipbr;
	HFileInfo		*fpb = (HFileInfo*) &cipbr;
	DirInfo			*dpb = (DirInfo*) &cipbr;
	Str63			theString;
	
	fpb->ioVRefNum = volumeID;
	fpb->ioNamePtr = theString;
	for(idx = 1; true; ++idx) {
		fpb->ioDirID = parentFolderID;
		fpb->ioFDirIndex = idx;
		if(PBGetCatInfo(&cipbr, false))
		break;
		
		if(fpb->ioFlAttrib & 16 && EqualString(theString, folderName, false, false)) {
			*folderID = dpb->ioDrDirID;
			return noErr;
		}
	}
	
	return -1; //Folder not found
}

static OSErr Get_EnginesFolder(FSSpec* theFolder)
{
	HGetVol(nil, &theFolder->vRefNum, &theFolder->parID);
	GetIndString(theFolder->name, 129, 1);
	
	return ScanForFolder(theFolder->vRefNum, theFolder->parID, theFolder->name, &theFolder->parID);
}

static void Scan_InfinitySoftwareEngine()
{
	CInfoPBRec				cipbr;
	HFileInfo				*fpb = (HFileInfo*) &cipbr;
	DirInfo					*dpb = (DirInfo*) &cipbr;
	FSSpec					theFolder,
							theFile;
	short					idx;
	OSErr					theError;
	
	theError = Get_EnginesFolder(&theFolder);
	if(theError) {
		//Do_Error(theError, 207);
		return;
	}
	theFile.vRefNum = theFolder.vRefNum;
	theFile.parID = theFolder.parID;
	fpb->ioVRefNum = theFolder.vRefNum;
	fpb->ioNamePtr = theFile.name;
	for(idx = 1; true; ++idx) {
		fpb->ioDirID = theFolder.parID;
		fpb->ioFDirIndex = idx;
		if(PBGetCatInfo(&cipbr, false))
		break;
		
		if(fpb->ioFlFndrInfo.fdType == kInfinityPlugInType)
		Load_InfinitySoftwareEngine(&theFile);
	}
}
	
void main()
{
	OSErr				theError;
	Rect				maxTextureRect = {0,0,512,512};
	GWorldPtr			oldGWorld;
	GDHandle			oldGDHandle;
	
	//Init ToolBox
	InitToolbox();
	if(!Init_Appeareance()) {
		Alert(128, nil);
		ExitToShell();
	}
	
	//Check for Rave
	if(!Rave_Available())
	ExitToShell();
	
	//Load Infinity Software Rave Engines
	Scan_InfinitySoftwareEngine();
	
#if 1
	//Display splash screen
	Show_AboutDialog(0);
#endif
	
	//Setup
	Preference_Read();
	Init_Menus();
	Init_MainWindow();
	Init_TextureWindow();
	Init_ShapeWindow();
	Init_ScriptWindow();
	Init_MappingWindow();
	Init_SequencerWindow();
	SelectWindow(mainWin);
	BringToFront(mappingWin);
	BringToFront(sequencerWin);
	BringToFront(textureWin);
	BringToFront(shapeWin);
	BringToFront(scriptWin);
	viewRect.right += 15;
	
	keyCursor = GetCursor(128);
	DetachResource((Handle) keyCursor);
	HLock((Handle) keyCursor);
	waitCursor = GetCursor(watchCursor);
	DetachResource((Handle) waitCursor);
	HLock((Handle) waitCursor);
	GetIndPattern(&offPattern, sysPatListID, 28);
	
	GetGWorld(&oldGWorld, &oldGDHandle);
	theError = NewGWorld(&textureGWorld, 16, &maxTextureRect, NULL, NULL, NULL);
	if(theError) {
		Do_Error(theError, 204);
		ExitToShell();
	}
	SetGWorld(textureGWorld, nil);
	LockPixels(GetGWorldPixMap(textureGWorld));
	BackColor(whiteColor);
	ForeColor(blackColor);
	SetGWorld(oldGWorld, oldGDHandle);
	
	ODOC_Handler_Routine = NewAEEventHandlerProc(ODOC_Handler);
	QUIT_Handler_Routine = NewAEEventHandlerProc(QUIT_Handler);
	AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, ODOC_Handler_Routine, 0, false);
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, QUIT_Handler_Routine, 0, false);
	
	theError = Init_Engine();
	if(theError) {
		Do_Error(theError, 205);
		ExitToShell();
	}
	
	if(Clock_Install())
	ExitToShell();
	
	Event_Loop();
	
	Object_Close();
	
	QADrawContextDelete(localState->drawContext);
	DisposePtr((Ptr) localState);
	QADrawContextDelete(topState->drawContext);
	DisposePtr((Ptr) topState);
	QADrawContextDelete(frontState->drawContext);
	DisposePtr((Ptr) frontState);
	QADrawContextDelete(rightState->drawContext);
	DisposePtr((Ptr) rightState);
	QADrawContextDelete(globalState->drawContext);
	DisposePtr((Ptr) globalState);
	
	Clock_Dispose();
	AERemoveEventHandler(kCoreEventClass, kAEOpenDocuments, ODOC_Handler_Routine, false);
	AERemoveEventHandler(kCoreEventClass, kAEQuitApplication, QUIT_Handler_Routine, false);
	UnlockPixels(GetGWorldPixMap(textureGWorld));
	DisposeGWorld(textureGWorld);
	LDispose(textureListHandle);
	LDispose(shapeListHandle);
	LDispose(scriptListHandle);
	
	thePrefs.windowPosition[0] = GetWindowPosition(shapeWin);
	thePrefs.windowPosition[1] = GetWindowPosition(textureWin);
	thePrefs.windowPosition[2] = GetWindowPosition(scriptWin);
	thePrefs.windowPosition[3] = GetWindowPosition(mappingWin);
	thePrefs.windowPosition[4] = GetWindowPosition(sequencerWin);
	
	DisposeWindow(mainWin);
	DisposeWindow(mappingWin);
	DisposeDialog(textureWin);
	DisposeDialog(shapeWin);
	DisposeDialog(scriptWin);
	DisposeDialog(sequencerWin);
	SetGWorld(oldGWorld, oldGDHandle);
	
	DisposeHandle((Handle) keyCursor);
	DisposeHandle((Handle) waitCursor);
	
	Preference_Write();
	//UnregisterAppearanceClient();
	ExitToShell();
}