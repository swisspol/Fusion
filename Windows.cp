#include			"Structures.h"
#include			"Editor Header.h"
#include			"Matrix.h"
#include			"Vector.h"
#include			"Keys.h"

//CONSTANTES:

#define				kTextureScaleStep		0.001
#define				kTextureMoveStep		0.001

//ROUTINES:

void Do_MainWin(Point whereMouse)
{
	Rect				theRect;
	GrafPtr				savePort;
	long				i,
						j,
						k,
						pointList[10],
						numPoints;
	Boolean			found;
	short				oldCurView;
	
	if(!object)
	return;
	
	if(mode) {
		if(currentCamera == &localCamera) {
			theObject.rotateX += objectRotateX;
			theObject.rotateY += objectRotateY;
			theObject.rotateZ += objectRotateZ;
			Object_UpdateMatrix(&theObject);
			
			numPoints = Shape_GetClickedPoints(globalState, theObject.shapeList[shapeCell.v], &theObject.pos, &currentCamera->camera, 
				theObject.shapeList, whereMouse, pointList);
			
			theObject.rotateX -= objectRotateX;
			theObject.rotateY -= objectRotateY;
			theObject.rotateZ -= objectRotateZ;
			Object_UpdateMatrix(&theObject);
		}
		else
		numPoints = Shape_GetClickedPoints(globalState, theObject.shapeList[shapeCell.v], &theObject.pos, &currentCamera->camera, 
			theObject.shapeList, whereMouse, pointList);
			
		if(numPoints == 0)
		selectedPoints = 0;
		else {
			for(i = 0; i < numPoints; ++i) {
				found = false;
				for(j = 0; j < selectedPoints; ++j)
				if(pointList[i] == selectedPoint[j]) {
					for(k = j + 1; k < selectedPoints; ++k)
					selectedPoint[k - 1] = selectedPoint[k];
					--selectedPoints;
					found = true;
				}
				
				if(!found) {
					selectedPoint[selectedPoints] = pointList[i];
					++selectedPoints;
				}
			}
		}
		
		GetPort(&savePort);
		Update_MappingWin();
		SetPort(savePort);
		return;
	}
	
	oldCurView = curView;
	SetRect(&theRect, 0, 0, 302, 202);
	if(PtInRect(whereMouse, &theRect)) {
		curView = 1;
		goto Refresh;
	}
	SetRect(&theRect, 0, 203, 302, 405);
	if(PtInRect(whereMouse, &theRect)) {
		curView = 2;
		goto Refresh;
	}
	SetRect(&theRect, 303, 203, 605, 405);
	if(PtInRect(whereMouse, &theRect)) {
		curView = 3;
		goto Refresh;
	}
	curView = 4;
	
	Refresh:
	if(oldCurView != curView)
	EraseRect(&mainWin->portRect);
}

static void Apply_Texture()
{
	float			xScale = 1.0,
					xOffset = 0.0,
					yScale = 1.0,
					yOffset = 0.0;
	KeyMap			theKeys;
	int				command,
					option,
					control;
	GrafPtr			savePort;
	Boolean		noWrapping = true;
				
	GetPort(&savePort);
	SetCursor(*keyCursor);
	if(event.modifiers & optionKey)
	noWrapping = false;
	while(!Button()) {
		
		Shape_ApplyTexture(globalState, theObject.shapeList[shapeCell.v], &theObject.pos, &currentCamera->camera, 
			theObject.shapeList, nameList[textureCell.v], xScale, xOffset, yScale, yOffset, noWrapping);
		Update_MainWin();
		
		GetKeys(theKeys);
		command = IsKeyDown(theKeys, keyCommand);
		option = IsKeyDown(theKeys, keyOption);
		control = IsKeyDown(theKeys, keyControl);
		
		if(IsKeyDown(theKeys, keyArrowLeft)) {
			if(option)
			xScale += kTextureScaleStep;
			else
			xOffset += kTextureMoveStep;
		}
		
		if(IsKeyDown(theKeys, keyArrowRight)) {
			if(option)
			xScale -= kTextureScaleStep;
			else
			xOffset -= kTextureMoveStep;
		}
		
		if(IsKeyDown(theKeys, keyArrowUp)) {
			if(command) {
				xScale -= kTextureScaleStep;
				yScale -= kTextureScaleStep;
			}
			else if(option)
			yScale -= kTextureScaleStep;
			else
			yOffset += kTextureMoveStep;
		}
	
		if(IsKeyDown(theKeys, keyArrowDown)) {
			if(command) {
				xScale += kTextureScaleStep;
				yScale += kTextureScaleStep;
			}
			else if(option)
			yScale += kTextureScaleStep;
			else
			yOffset -= kTextureMoveStep;
		}
		
	}
	InitCursor();
	SetPort(savePort);
}

void Do_TextureWin(Point whereMouse)
{
	long				menuResult,
						i;
	short				theItem;
	OSErr				theError;
	float				red, green, blue;
	RGBColor			theColor = {0x8888, 0x8888, 0x8888};
	Point				where = {-1,-1};
	SFTypeList			theTypeList;
	StandardFileReply	theReply;
	Boolean			doubleClick;
	VertexPtr			vextex;
	
	if(PtInRect(whereMouse, &popUpRect)) {
		InvertRect(&popUpRect);
		LocalToGlobal((Point*) (&popUpRect.top));
		
		if(theObject.shapeCount) {
			if(textureCount && mode && (currentCamera != &localCamera))
			EnableItem(texturePopUpMenu, 4);
			else
			DisableItem(texturePopUpMenu, 4);
			EnableItem(texturePopUpMenu, 7);
			if(selectedPoints && (theObject.shapeList[shapeCell.v]->texture == kNoTexture))
			EnableItem(texturePopUpMenu, 8);
			else
			DisableItem(texturePopUpMenu, 8);
		}
		else {
			DisableItem(texturePopUpMenu, 4);
			DisableItem(texturePopUpMenu, 7);
			DisableItem(texturePopUpMenu, 8);
		}
		
		if(textureCount == 0) {
			DisableItem(texturePopUpMenu, 2);
			DisableItem(texturePopUpMenu, 5);
		}
		else {
			EnableItem(texturePopUpMenu, 2);
			EnableItem(texturePopUpMenu, 5);
		}
		
		menuResult = PopUpMenuSelect(texturePopUpMenu, popUpRect.top + 16, popUpRect.left, 1);
		
		if(menuResult) {
			theItem = LoWord(menuResult);
			switch(theItem) {
			
				case 1:
				theTypeList[0] = 'PICT';
				StandardGetFilePreview(nil, 1, theTypeList, &theReply);
				if(!theReply.sfGood)
				break;
				if(event.modifiers & optionKey)
				theError = Texture_NewFromFile(localState->engine, &theReply.sfFile, true);
				else
				theError = Texture_NewFromFile(localState->engine, &theReply.sfFile, false);
				if(theError)
				Do_Error(theError, 206);
				break;
				
				case 2:
				DisposePtr((Ptr) textureStorageList[textureCell.v]);
				QATextureDelete(localState->engine, textureList[textureCell.v]);
				for(i = textureCell.v + 1; i < textureCount; ++i) {
					textureStorageList[i - 1] = textureStorageList[i];
					textureList[i - 1] = textureList[i];
					nameList[i - 1] = nameList[i];
				}
				--textureCount;
				LDelRow(1, textureCell.v, textureListHandle);
				if(textureCell.v > textureCount - 1)
				textureCell.v = textureCount - 1;
				LSetSelect(true, textureCell, textureListHandle);
				break;
				
				case 4:
				Apply_Texture();
				Check_ShapeMapping();
				break;
				
				case 5:
				Texture_Setting(textureCell.v);
				break;
				
				case 7:
				if(GetColor(where, "\pNew shape color:", &theColor, &theColor)) {
					red = UShort2Float(theColor.red);
					green = UShort2Float(theColor.green);
					blue = UShort2Float(theColor.blue);
					Shape_ApplyColor(theObject.shapeList[shapeCell.v], red, green, blue);
					Check_ShapeMapping();
				}
				break;
				
				case 8:
				if(selectedPoints == 1) {
					vextex = &theObject.shapeList[shapeCell.v]->pointList[selectedPoint[0]];
					theColor.red = Float2UShort(vextex->u);
					theColor.green = Float2UShort(vextex->v);
					theColor.blue = Float2UShort(vextex->c);
				}
				if(GetColor(where, "\pNew vertex color:", &theColor, &theColor)) {
					red = UShort2Float(theColor.red);
					green = UShort2Float(theColor.green);
					blue = UShort2Float(theColor.blue);
					for(i = 0; i < selectedPoints; ++i) {
						vextex = &theObject.shapeList[shapeCell.v]->pointList[selectedPoint[i]];
						vextex->u = red;
						vextex->v = green;
						vextex->c = blue;
					}
				}
				break;
			
			}
		}
		
		GlobalToLocal((Point*) (&popUpRect.top));
		InvertRect(&popUpRect);
		Update_TextureWin();
	}
	else if(PtInRect(whereMouse, &viewRect)) {
		doubleClick = LClick(whereMouse, nil, textureListHandle);
		textureCell.h = 0;
		textureCell.v = 0;
		if(!LGetSelect(true, &textureCell, textureListHandle)) {
			textureCell.v = textureCount - 1;
			LSetSelect(true, textureCell, textureListHandle);
		}
		if(doubleClick)
		Texture_Setting(textureCell.v);
	}
}

#define kPrecision 2

void Do_MappingWin(Point whereMouse)
{
	long			i,
					j;
	Point			thePoint;
	Rect			theRect;
	KeyMap			theKeys;
	GrafPtr			savePort;
	int				shift;
	
	if(selectedPoints == 0)
	return;
	
	for(i = 0; i < selectedPoints; ++i) {
		thePoint.h = theObject.shapeList[shapeCell.v]->pointList[selectedPoint[i]].u * (float) curTexWidth;
		thePoint.v = curTexHeight - theObject.shapeList[shapeCell.v]->pointList[selectedPoint[i]].v * (float) curTexHeight;
		if(zoom) {
			thePoint.h *= 2;
			thePoint.v *= 2;
		}
		
		theRect.left = thePoint.h - kPrecision;
		theRect.right = thePoint.h + kPrecision;
		theRect.top = thePoint.v - kPrecision;
		theRect.bottom = thePoint.v + kPrecision;
		if(PtInRect(whereMouse, &theRect)) {
			while(Button())
			;
			SetCursor(*keyCursor);
			while(!Button()) {
				GetKeys(theKeys);
				shift = IsKeyDown(theKeys, keyShift);
				
				if(IsKeyDown(theKeys, keyArrowLeft)) {
					if(shift)
					for(j = 0; j < selectedPoints; ++j)
					theObject.shapeList[shapeCell.v]->pointList[selectedPoint[j]].u -= kTextureMoveStep;
					else
					theObject.shapeList[shapeCell.v]->pointList[selectedPoint[i]].u -= kTextureMoveStep;
				}
				if(IsKeyDown(theKeys, keyArrowRight)) {
					if(shift)
					for(j = 0; j < selectedPoints; ++j)
					theObject.shapeList[shapeCell.v]->pointList[selectedPoint[j]].u += kTextureMoveStep;
					else
					theObject.shapeList[shapeCell.v]->pointList[selectedPoint[i]].u += kTextureMoveStep;
				}
				if(IsKeyDown(theKeys, keyArrowUp)) {
					if(shift)
					for(j = 0; j < selectedPoints; ++j)
					theObject.shapeList[shapeCell.v]->pointList[selectedPoint[j]].v += kTextureMoveStep;
					else
					theObject.shapeList[shapeCell.v]->pointList[selectedPoint[i]].v += kTextureMoveStep;
				}
				if(IsKeyDown(theKeys, keyArrowDown)) {
					if(shift)
					for(j = 0; j < selectedPoints; ++j)
					theObject.shapeList[shapeCell.v]->pointList[selectedPoint[j]].v -= kTextureMoveStep;
					else
					theObject.shapeList[shapeCell.v]->pointList[selectedPoint[i]].v -= kTextureMoveStep;
				}
				
				Update_MappingWin();
				if(filled)
				Update_MainWin();
			}
			InitCursor();
			return;
		}
	}
}

void Do_ShapeWin(Point whereMouse)
{
	long				menuResult,
						i;
	SFTypeList			theTypeList;
	StandardFileReply	theReply;
	short				theItem;
	OSErr				theError;
	Matrix				m;
	Boolean			doubleClick;
	float				angle;
		
	if(PtInRect(whereMouse, &popUpRect)) {
		InvertRect(&popUpRect);
		LocalToGlobal((Point*) (&popUpRect.top));
		
		if(theObject.shapeCount) {
			if(theObject.shapeCount == 1) {
				DisableItem(shapePopUpMenu, 2);
				DisableItem(shapePopUpMenu, 3);
			}
			else {
				EnableItem(shapePopUpMenu, 2);
				if((oldShapeCell.v == -1) || (oldShapeCell.v == shapeCell.v))
				DisableItem(shapePopUpMenu, 3);
				else
				EnableItem(shapePopUpMenu, 3);
			}
			
			if(theObject.shapeList[shapeCell.v]->normalMode == kNoNormals)
			DisableItem(shapePopUpMenu, 7);
			else
			EnableItem(shapePopUpMenu, 7);
			
			if(theObject.shapeList[shapeCell.v]->flags & kFlag_RelativePos)
			EnableItem(shapePopUpMenu, 13);
			else
			DisableItem(shapePopUpMenu, 13);
		}
		menuResult = PopUpMenuSelect(shapePopUpMenu, popUpRect.top + 16, popUpRect.left, 1);
		
		if(menuResult) {
			theItem = LoWord(menuResult);
			switch(theItem) {
			
				case 1:
				theTypeList[0] = '3DMF';
				StandardGetFile(nil, 1, theTypeList, &theReply);
				if(!theReply.sfGood)
				break;
				if(event.modifiers & optionKey)
				theError = Shape_Load3DMF(&theReply.sfFile, false);
				else
				theError = Shape_Load3DMF(&theReply.sfFile, true);
				if(theError) {
					if(theError != -1)
					Do_Error(theError, 105);
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
				
				case 2:
				Shape_Dispose(theObject.shapeList[shapeCell.v]);
				for(i = shapeCell.v + 1; i < theObject.shapeCount; ++i)
				theObject.shapeList[i - 1] = theObject.shapeList[i];
				--theObject.shapeCount;
				LDelRow(1, shapeCell.v, shapeListHandle);
				if(shapeCell.v > theObject.shapeCount - 1)
				shapeCell.v = theObject.shapeCount - 1;
				LSetSelect(true, shapeCell, shapeListHandle);
				break;
				
				case 3:
				Shape_Merge(theObject.shapeList[oldShapeCell.v], theObject.shapeList[shapeCell.v]);
				++theObject.shapeList[oldShapeCell.v]->name[0];
				theObject.shapeList[oldShapeCell.v]->name[theObject.shapeList[oldShapeCell.v]->name[0]] = '*';
				LSetCell(&theObject.shapeList[oldShapeCell.v]->name[1], theObject.shapeList[oldShapeCell.v]->name[0], oldShapeCell, shapeListHandle);
				
				Shape_Dispose(theObject.shapeList[shapeCell.v]);
				for(i = shapeCell.v + 1; i < theObject.shapeCount; ++i)
				theObject.shapeList[i - 1] = theObject.shapeList[i];
				--theObject.shapeCount;
				LDelRow(1, shapeCell.v, shapeListHandle);
				if(shapeCell.v > theObject.shapeCount - 1)
				shapeCell.v = theObject.shapeCount - 1;
				LSetSelect(true, shapeCell, shapeListHandle);
				break;
				
				case 4:
				theObject.shapeList[theObject.shapeCount] = Shape_Copy(theObject.shapeList[shapeCell.v]);
				LAddRow(1, 1000, shapeListHandle);
				theCell.h = 0;
				theCell.v = theObject.shapeCount;
				LSetCell(&theObject.shapeList[theObject.shapeCount]->name[1], theObject.shapeList[theObject.shapeCount]->name[0], theCell, shapeListHandle);
				++theObject.shapeCount;
				LSetSelect(false, shapeCell, shapeListHandle);
				shapeCell.v = theObject.shapeCount - 1;
				LSetSelect(true, shapeCell, shapeListHandle);
				break;
				
				case 6:
				SetCursor(*waitCursor);
				if(event.modifiers & optionKey) {
					for(i = 0; i < theObject.shapeCount; ++i)
					Shape_CalculateNormals(theObject.shapeList[i], 2, 0.0);
				}
				else
				Shape_CalculateNormals(theObject.shapeList[shapeCell.v], 2, 0.0);
				InitCursor();
				break;
				
				case 7:
				if(event.modifiers & optionKey) {
					for(i = 0; i < theObject.shapeCount; ++i)
					Shape_ClearNormals(theObject.shapeList[i]);
				}
				else
				Shape_ClearNormals(theObject.shapeList[shapeCell.v]);
				break;
				
				case 9:
				SetCursor(*waitCursor);
				if(event.modifiers & optionKey) {
					for(i = 0; i < theObject.shapeCount; ++i) {
						Shape_ClearNormals(theObject.shapeList[i]);
						Shape_SwitchBF(theObject.shapeList[i]);
						Shape_CalculateNormals(theObject.shapeList[i], 2, 0.0);
					}
				}
				else {
					Shape_ClearNormals(theObject.shapeList[shapeCell.v]);
					Shape_SwitchBF(theObject.shapeList[shapeCell.v]);
					Shape_CalculateNormals(theObject.shapeList[shapeCell.v], 2, 0.0);
				}
				InitCursor();
				break;
				
				case 10:
				if(event.modifiers & optionKey) {
					for(i = 0; i < theObject.shapeCount; ++i)
					Shape_Center(theObject.shapeList[i]);
				}
				else
				Shape_Center(theObject.shapeList[shapeCell.v]);
				break;
				
				case 11:
				if(event.modifiers & optionKey) {
					for(i = 0; i < theObject.shapeCount; ++i)
					Shape_ReCenterOrigin(theObject.shapeList[i]);
				}
				else
				Shape_ReCenterOrigin(theObject.shapeList[shapeCell.v]);
				break;
				
				case 13:
				if(event.modifiers & optionKey) {
					for(i = 0; i < theObject.shapeCount; ++i)
					Shape_ApplyMatrix(theObject.shapeList[i]);
				}
				else
				Shape_ApplyMatrix(theObject.shapeList[shapeCell.v]);
				break;
				
				case 14:
				Object_ApplyMatrix(&theObject);
				break;
				
				case 16:
				Shape_Setting(theObject.shapeList[shapeCell.v], theObject.shapeList);
				break;
				
				case 17:
				Object_Setting(&theObject);
				break;
			
			}
		}
		
		GlobalToLocal((Point*) (&popUpRect.top));
		InvertRect(&popUpRect);
		Update_ShapeWin();
	}
	else if(PtInRect(whereMouse, &viewRect)) {
		doubleClick = LClick(whereMouse, nil /*event.modifiers*/, shapeListHandle);
		oldShapeCell = shapeCell;
		shapeCell.h = 0;
		shapeCell.v = 0;
		if(!LGetSelect(true, &shapeCell, shapeListHandle)) {
			shapeCell.v = theObject.shapeCount - 1;
			LSetSelect(true, shapeCell, shapeListHandle);
		}
		if(doubleClick)
		Shape_Setting(theObject.shapeList[shapeCell.v], theObject.shapeList);
		selectedPoints = 0;
		Check_ShapeMapping();
	}
}

static void Script_Edit(ScriptPtr script)
{
	Str255				title = "\pSequencer - ";
	GrafPtr				savePort;
	
	GetPort(&savePort);
	BlockMove(&script->name[1], &title[title[0] + 1], script->name[0]);
	title[0] += script->name[0];
	SetWTitle(sequencerWin, title);
	
	ShowHide(sequencerWin, true);
	sequencerVisible = true;
	currentScript = script;
	
	if(script->animationCount)
	animationCell = 0;
	else
	animationCell = -1;
	firstAnimation = 0;
	currentTime = 0;
	firstTime = 0;
	
	Update_SequencerWin();
	SetPort(savePort);
}

void Do_ScriptWin(Point whereMouse)
{
	long				menuResult;
	short				theItem;
	OSErr				theError;
	Boolean			doubleClick;
	long				i;
	Str255				title = "\pSequencer - ";
		
	if(PtInRect(whereMouse, &popUpRect)) {
		InvertRect(&popUpRect);
		LocalToGlobal((Point*) (&popUpRect.top));
		
		if(theObject.shapeCount)
		EnableItem(scriptPopUpMenu, 1);
		else
		DisableItem(scriptPopUpMenu, 1);
		if(scriptCount) {
			EnableItem(scriptPopUpMenu, 2);
			EnableItem(scriptPopUpMenu, 4);
			EnableItem(scriptPopUpMenu, 5);
		}
		else {
			DisableItem(scriptPopUpMenu, 2);
			DisableItem(scriptPopUpMenu, 4);
			DisableItem(scriptPopUpMenu, 5);
		}
		
		menuResult = PopUpMenuSelect(scriptPopUpMenu, popUpRect.top + 16, popUpRect.left, 1);
		
		if(menuResult) {
			theItem = LoWord(menuResult);
			switch(theItem) {
			
				case 1:
				if(scriptCount == kMaxScripts - 1)
				break;
				scriptList[scriptCount] = Script_New();
				theCell.h = 0;
				theCell.v = LAddRow(1, 1000, scriptListHandle);
				LSetCell(&scriptList[scriptCount]->name[1], scriptList[scriptCount]->name[0], theCell, scriptListHandle);
				LSetSelect(false, scriptCell, scriptListHandle);
				scriptCell = theCell;
				LSetSelect(true, scriptCell, scriptListHandle);
				++scriptCount;
				Script_Edit(scriptList[scriptCell.v]);
				break;
				
				case 2:
				if(scriptList[scriptCell.v] == currentScript) {
					ShowHide(sequencerWin, false);
					sequencerVisible = false;
					currentScript = nil;
				}
				Script_Dispose(scriptList[scriptCell.v]);
				for(i = scriptCell.v + 1; i < scriptCount; ++i)
				scriptList[i - 1] = scriptList[i];
				--scriptCount;
				LDelRow(1, scriptCell.v, scriptListHandle);
				if(scriptCell.v > scriptCount - 1)
				scriptCell.v = scriptCount - 1;
				LSetSelect(true, scriptCell, scriptListHandle);
				break;
				
				case 4:
				Script_Edit(scriptList[scriptCell.v]);
				break;
				
				case 5:
				Script_Setting(scriptList[scriptCell.v]);
				if(sequencerVisible && (scriptList[scriptCell.v] == currentScript)) {
					BlockMove(&currentScript->name[1], &title[title[0] + 1], currentScript->name[0]);
					title[0] += currentScript->name[0];
					SetWTitle(sequencerWin, title);
					Update_SequencerWin();
				}
				break;
			
			}
		}
		
		GlobalToLocal((Point*) (&popUpRect.top));
		InvertRect(&popUpRect);
		Update_ScriptWin();
	}
	else if(PtInRect(whereMouse, &viewRect)) {
		doubleClick = LClick(whereMouse, nil /*event.modifiers*/, scriptListHandle);
		scriptCell.h = 0;
		scriptCell.v = 0;
		if(!LGetSelect(true, &scriptCell, scriptListHandle)) {
			scriptCell.v = scriptCount - 1;
			LSetSelect(true, scriptCell, scriptListHandle);
		}
		if(doubleClick)
		Script_Edit(scriptList[scriptCell.v]);
	}
}

static void ShowHide_SelectedAnimation()
{
	Rect			tempRect;
	
	tempRect.top = 16 + (animationCell - firstAnimation) * (kLineHeight + 1);
	tempRect.bottom = tempRect.top + kLineHeight;
	tempRect.left = 0;
	tempRect.right = kWitdh; //kNameSize;
	
#if 0
	RGBBackColor(&darkGrey);
	LMSetHiliteMode(LMGetHiliteMode() & hiliteBit);
	InvertRect(&tempRect);
	BackColor(whiteColor);
#else
	InvertRect(&tempRect);
#endif
}

void Do_SequencerWin(Point whereMouse)
{
	long				menuResult,
						i;
	short				theItem,
						partCode;
	ControlHandle		whichControl;
	Boolean			doubleClick = event.when - lastClick < GetDblTime();
	Rect				frameRateRect = {130,0,145,100};
	
	partCode = FindControl(whereMouse, sequencerWin, &whichControl);
	if(partCode) {
		if(whichControl == sequencerScroll) {
			TrackControl(sequencerScroll, whereMouse, nil);
			switch(partCode) {
				
				case kControlDownButtonPart:
				if(firstAnimation < currentScript->animationCount - 6)
				++firstAnimation;
				break;
				
				case kControlUpButtonPart:
				if(firstAnimation > 0)
				--firstAnimation;
				break;
				
				case kControlPageDownPart:
				firstAnimation = currentScript->animationCount - 6;
				break;
				
				case kControlPageUpPart:
				firstAnimation = 0;
				break;
				
				default:
				firstAnimation = GetControlValue(sequencerScroll);
				break;
			}
			Update_SequencerWin();
		}
		if(whichControl == timeScroll) {
			TrackControl(timeScroll, whereMouse, nil);
			switch(partCode) {
				
				case kControlDownButtonPart:
				if(firstTime < 90)
				++firstTime;
				break;
				
				case kControlUpButtonPart:
				if(firstTime > 0)
				--firstTime;
				break;
				
				case kControlPageDownPart:
				firstTime = 90;
				break;
				
				case kControlPageUpPart:
				firstTime = 0;
				break;
				
				default:
				firstTime = GetControlValue(timeScroll);
				break;
			}
			Update_SequencerWin();
		}
	}
	
	if(PtInRect(whereMouse, &popUpRect2)) {
		InvertRect(&popUpRect2);
		LocalToGlobal((Point*) (&popUpRect2.top));
		
		if(currentScript->animationCount) {
			if(currentScript->flags & kFlag_Running) {
				DisableItem(sequencerPopUpMenu, 1);
				EnableItem(sequencerPopUpMenu, 2);
				DisableItem(sequencerPopUpMenu, 3);
				DisableItem(sequencerPopUpMenu, 5);
				DisableItem(sequencerPopUpMenu, 6);
				DisableItem(sequencerPopUpMenu, 8);
			}
			else {
				EnableItem(sequencerPopUpMenu, 1);
				DisableItem(sequencerPopUpMenu, 2);
				EnableItem(sequencerPopUpMenu, 3);
				EnableItem(sequencerPopUpMenu, 5);
				EnableItem(sequencerPopUpMenu, 6);
				EnableItem(sequencerPopUpMenu, 8);
			}
		}
		else {
			DisableItem(sequencerPopUpMenu, 1);
			DisableItem(sequencerPopUpMenu, 2);
			DisableItem(sequencerPopUpMenu, 3);
			DisableItem(sequencerPopUpMenu, 6);
			DisableItem(sequencerPopUpMenu, 8);
		}
		
		menuResult = PopUpMenuSelect(sequencerPopUpMenu, popUpRect2.top + 16, popUpRect2.left, 1);
		
		if(menuResult) {
			theItem = LoWord(menuResult);
			switch(theItem) {
			
				case 1:
				Script_Run(currentScript);
				Microseconds(&lastFrameTime);
				MoveTo(50,140);
				DrawString("\pFPS");
				break;
				
				case 2:
				Script_Stop(currentScript);
				EraseRect(&frameRateRect);
				break;
				
				case 3:
				Script_Reset(currentScript);
				break;
				
				case 5:
				animationCell = currentScript->animationCount;
				if(animationCell - firstAnimation == 6)
				++firstAnimation;
				Script_NewAnimation(currentScript, shapeCell.v);
				break;
				
				case 6:
				Script_DisposeAnimation(currentScript, animationCell);
				if(currentScript->animationCount == 0)
				animationCell = -1;
				else if(animationCell > currentScript->animationCount - 1)
				animationCell = currentScript->animationCount - 1;
				if(firstAnimation && (currentScript->animationCount - firstAnimation < 6))
				--firstAnimation;
				Script_UpdateLength(currentScript);
				break;
				
				case 8:
				Script_AnimationAddStartStatusEvents(currentScript, currentTime);
				Script_UpdateLength(currentScript);
				break;
			
			}
		}
		
		GlobalToLocal((Point*) (&popUpRect2.top));
		InvertRect(&popUpRect2);
		Update_SequencerWin();
	}
	else {
		if(PtInRect(whereMouse, &viewRect2) && currentScript->animationCount) {
			ShowHide_SelectedAnimation();
			animationCell = firstAnimation + (whereMouse.v - 16) / (kLineHeight + 1);
			if(animationCell < 0)
			animationCell = 0;
			if(animationCell > currentScript->animationCount - 1)
			animationCell = currentScript->animationCount - 1;
			ShowHide_SelectedAnimation();
		}
		if(PtInRect(whereMouse, &nameRect) && doubleClick && !(currentScript->flags & kFlag_Running)) {
			/*for(i = 0; i < currentScript->animationList[animationCell]->eventCount; ++i)
			if(currentScript->animationList[animationCell]->eventList[i].time == currentTime) {
				Script_AnimationDeleteEvent(currentScript, animationCell, i);
				Script_AnimationAddEvent(currentScript, animationCell, currentTime);
			}*/
			Script_AnimationAddEvent(currentScript, animationCell, currentTime);
			Script_UpdateLength(currentScript);
			Update_SequencerWin();
		} else if(PtInRect(whereMouse, &timeRect)) {
			currentTime = (whereMouse.h - kNameSize) / 5 + firstTime * kEditorTimeUnit;
			if(/*doubleClick &&*/ !(currentScript->flags & kFlag_Running) && currentScript->animationCount) {
				for(i = 0; i < currentScript->animationList[animationCell]->eventCount; ++i)
				if(currentScript->animationList[animationCell]->eventList[i].time == currentTime) {
					if(event.modifiers & optionKey)
					Script_AnimationDeleteEvent(currentScript, animationCell, i);
					else if(event.modifiers & controlKey)
					Script_AnimationSetEvent(currentScript, animationCell, i);
					else
					Event_Setting(&currentScript->animationList[animationCell]->eventList[i]);
					Script_UpdateLength(currentScript);
				}
			}
			Update_SequencerWin();
		}
	}
}

void Update_MainWin()
{
	Rect			theRect;
	long			i;
	
	SetPort(mainWin);
	if(!isForeGround) {
		EraseRect(&mainWin->portRect);
		return;
	}
	
	if(object) {
		if(mode) {
			if(currentCamera == &localCamera) {
				theObject.rotateX += objectRotateX;
				theObject.rotateY += objectRotateY;
				theObject.rotateZ += objectRotateZ;
				Object_UpdateMatrix(&theObject);
				Render(globalState, &localCamera.camera);
#if 0
				for(i = 0; i < selectedPoints; ++i)
				Shape_DrawSelectedPoint(globalState, theObject.shapeList[shapeCell.v], &theObject.pos, &currentCamera->camera, theObject.shapeList, selectedPoint[i]);
#endif				
				theObject.rotateX -= objectRotateX;
				theObject.rotateY -= objectRotateY;
				theObject.rotateZ -= objectRotateZ;
				Object_UpdateMatrix(&theObject);
			}
			else {
				Render(globalState, &currentCamera->camera);
#if 0
				for(i = 0; i < selectedPoints; ++i)
				Shape_DrawSelectedPoint(globalState, theObject.shapeList[shapeCell.v], &theObject.pos, &currentCamera->camera, theObject.shapeList, selectedPoint[i]);
#endif
			}
			
			if(!fullView) {
				SetRect(&theRect, 0, 0, 605, 405);
				ForeColor(redColor);
				FrameRect(&theRect);
			}
		}
		else {
			theObject.rotateX += objectRotateX;
			theObject.rotateY += objectRotateY;
			theObject.rotateZ += objectRotateZ;
			Object_UpdateMatrix(&theObject);
			Render(localState, &localCamera.camera);
			
			theObject.rotateX -= objectRotateX;
			theObject.rotateY -= objectRotateY;
			theObject.rotateZ -= objectRotateZ;
			Object_UpdateMatrix(&theObject);
			Render(topState, &topCamera.camera);
			Render(frontState, &frontCamera.camera);
			Render(rightState, &rightCamera.camera);
			
			ForeColor(whiteColor);
			MoveTo(302,0);
			LineTo(302,405);
			MoveTo(0,202);
			LineTo(605,202);
			
			if(curView == 1)
			SetRect(&theRect, 0, 0, 302, 202);
			else if(curView == 2)
			SetRect(&theRect, 0, 203, 302, 405);
			else if(curView == 3)
			SetRect(&theRect, 303, 203, 605, 405);
			else SetRect(&theRect, 303, 0, 605, 202);
			ForeColor(redColor);
			FrameRect(&theRect);
		}
	}
	else
	EraseRect(&mainWin->portRect);
}

void Update_TextureWin()
{
	if(!textureVisible)
	return;
	
	SetPort(textureWin);
	DrawDialog(textureWin);
	MoveTo(6,11);
	DrawString("\pTexture operations");
	LUpdate((*textureListHandle)->port->visRgn, textureListHandle);
}

void Update_ShapeWin()
{
	if(!shapeVisible)
	return;
	
	SetPort(shapeWin);
	DrawDialog(shapeWin);
	MoveTo(6,11);
	DrawString("\pShape operations");
	LUpdate((*shapeListHandle)->port->visRgn, shapeListHandle);
}

void Update_ScriptWin()
{
	if(!scriptVisible)
	return;
	
	SetPort(scriptWin);
	DrawDialog(scriptWin);
	MoveTo(6,11);
	DrawString("\pScript operations");
	LUpdate((*scriptListHandle)->port->visRgn, scriptListHandle);
}
	
void Update_MappingWin()
{
	long				i,
						j;
	Point				thePoint = {0,0};
	Rect				zoomRect,
						textureRect;
	GWorldPtr			oldGWorld;
	GDHandle			oldGDHandle;

	if(!mappingVisible)
	return;
	
	GetGWorld(&oldGWorld, &oldGDHandle);
	for(i = 0; i < textureCount; ++i)
	if(theObject.shapeList[shapeCell.v]->texture == nameList[i]) {
		SetGWorld(textureGWorld, nil);
		Texture_Draw(textureStorageList[i], GetWindowPixMapPtr(textureGWorld), thePoint);
		for(j = 0; j < selectedPoints; ++j) {
			thePoint.h = theObject.shapeList[shapeCell.v]->pointList[selectedPoint[j]].u * (float) curTexWidth;
			thePoint.v = curTexHeight - theObject.shapeList[shapeCell.v]->pointList[selectedPoint[j]].v * (float) curTexHeight;
			
			ForeColor(blackColor);
			PenSize(4,4);
			MoveTo(thePoint.h - 2, thePoint.v - 2);
			Line(0,0);
			ForeColor(yellowColor);
			PenSize(2,2);
			MoveTo(thePoint.h - 1, thePoint.v - 1);
			Line(0,0);
		}
		
		SetGWorld(oldGWorld, oldGDHandle);
		SetPort(mappingWin);
		SetRect(&textureRect, 0, 0, curTexWidth, curTexHeight);
		if(zoom)
		SetRect(&zoomRect, 0, 0, curTexWidth * 2, curTexHeight * 2);
		else
		SetRect(&zoomRect, 0, 0, curTexWidth, curTexHeight);
		CopyBits(GWBitMapPtr(textureGWorld), GWBitMapPtr(mappingWin), &textureRect, &zoomRect, srcCopy, nil);
		return;
	}
}

static void Update_TimePointer()
{
	Rect			tempRect;
	long			time = clockTime - currentScript->startTime;
	GrafPtr			savePort;
	
	GetPort(&savePort);
	SetPort(sequencerWin);
	tempRect.top = 16;
	tempRect.bottom = 113;
	tempRect.left = kNameSize + (time - firstTime * kEditorTimeUnit) * 5;
	tempRect.right = tempRect.left + 1;
	
	if((time >= firstTime * kEditorTimeUnit) && (time - firstTime * kEditorTimeUnit < kTimeUnit))
	InvertRect(&tempRect);
	SetPort(savePort);
}

void Update_SequencerWin()
{
	Rect				theRect = {16, 0,129,kWitdh};
	long				i,
						j,
						max;
	short				pos = 16;
	Str31				time;
	EventPtr			timeEvent;
	GrafPtr				savePort;
	
	if(!sequencerVisible)
	return;
	
	//Draw window
	GetPort(&savePort);
	SetPort(sequencerWin);
	BackColor(whiteColor);
	ForeColor(blackColor);
	DrawDialog(sequencerWin);
	MoveTo(6,11);
	DrawString("\pSequencer operations");
	
	//Update scrollbars
	SetControlMaximum(sequencerScroll, currentScript->animationCount - 6);
	SetControlValue(sequencerScroll, firstAnimation);
	SetControlValue(timeScroll, firstTime);
	UpdateControls(sequencerWin, sequencerWin->visRgn);
	
	//Draw background
	RGBForeColor(&lightGrey);
	PaintRect(&theRect);
	theRect.right = kNameSize;
	RGBForeColor(&darkGrey);
	PaintRect(&theRect);
	ForeColor(whiteColor);
	for(i = 0; i < kNbLines - 1; ++i) {
		pos += kLineHeight;
		MoveTo(0, pos);
		LineTo(kWitdh - 1, pos);
		++pos;
	}
	
	//Draw timeline
	TextSize(8);
	TextFace(0);
	ForeColor(blackColor);
	for(i = 0; i < kTimeUnit; ++i) {
		MoveTo(kNameSize + i * 5, 12);
		Line(0, 3);
		if((firstTime * kEditorTimeUnit + i) % 5 == 0)
		Line(0, -5);
		if((firstTime * kEditorTimeUnit + i) % 10 == 0) {
			Line(0, -3);
			MoveTo(kNameSize + i * 5 + 1, 8);
			NumToString(firstTime + i / kEditorTimeUnit, time);
			DrawString(time);
		}
	}
	TextSize(10);
	TextFace(1);
	
	//Draw animations
	max = 6;
	if(firstAnimation + max > currentScript->animationCount)
	max = currentScript->animationCount - firstAnimation;
	PenSize(3,3);
	for(i = 0; i < max; ++i) {
		//Draw name
		ForeColor(blackColor);
		MoveTo(10, 26 + i * (kLineHeight + 1) + 2);
		DrawString(Shape_GetFromID(currentScript->animationList[firstAnimation + i]->shapeID)->name);
		
		//Draw events
		ForeColor(blueColor);
		timeEvent = currentScript->animationList[firstAnimation + i]->eventList;
		for(j = 0; j < currentScript->animationList[firstAnimation + i]->eventCount; ++j) {
			if((timeEvent->time > firstTime * kEditorTimeUnit) && (timeEvent->time < firstTime * kEditorTimeUnit + kTimeUnit)) {
				MoveTo(kNameSize + (timeEvent->time - firstTime * kEditorTimeUnit) * 5 - 1, 18 + i * (kLineHeight + 1));
				Line(0, kLineHeight - 7);
			}
			++timeEvent;
		}
	}
	PenSize(1,1);
	
	//Draw time pointer
	if((currentTime >= firstTime * kEditorTimeUnit) && (currentTime - firstTime * kEditorTimeUnit < kTimeUnit)) {
		ForeColor(redColor);
		MoveTo(kNameSize + (currentTime - firstTime * kEditorTimeUnit) * 5, 0);
		Line(0, 129);
	}
	
	//Draw out-of-length
	RGBForeColor(&veryDarkGrey);
	if(currentScript->length - firstTime * kEditorTimeUnit > 0) {
		theRect.left = kNameSize + (currentScript->length - firstTime * kEditorTimeUnit) * 5;
		theRect.right = kWitdh;
		PenMode(patOr);
		PenPat(&offPattern);
		PaintRect(&theRect);
		PenPat(&qd.black);
		PenMode(patCopy);
		
		ForeColor(blueColor);
		PenSize(3,3);
		MoveTo(theRect.left, 0);
		Line(0, 12);
		if(currentScript->flags & kFlag_Loop) {
			Line(-8, 0);
			Line(4,-4);
		}
		PenSize(1,1);
	} else if(currentScript->length <= firstTime * kEditorTimeUnit) {
		theRect.left = kNameSize;
		theRect.right = kWitdh;
		PenMode(patOr);
		PenPat(&offPattern);
		PaintRect(&theRect);
		PenPat(&qd.black);
		PenMode(patCopy);
	}
	
	//Draw selected animation
	if((animationCell != -1) && (animationCell - firstAnimation >= 0) && (animationCell - firstAnimation < 6))
	ShowHide_SelectedAnimation();
	
	//Restore standard colors
	BackColor(whiteColor);
	ForeColor(blackColor);
	SetPort(savePort);
}