#include			"Structures.h"
#include			"Editor Header.h"
#include			"Matrix.h"
#include			"Vector.h"

//CONSTANTES:

#define			kEnterDelay			10

//MACROS:

#define MIN(x,y)				((x) <= (y) ? (x) : (y))

//ROUTINES:

//Utils:

#if 0
pascal OSStatus GetStaticTextText(ControlHandle control, StringPtr text)
{
	Size		actualSize;
	OSStatus	err;
	
	if ( control == nil )
		return paramErr;
		
	if ( text == nil )
		return paramErr;
		
	err = GetControlData( control, 0, kControlStaticTextTextTag, 255, (Ptr)(text + 1), &actualSize );
	if ( err == noErr )
		text[0] = MIN( 255, actualSize );
		
	return err;
}

pascal OSStatus SetStaticTextText(ControlHandle control, ConstStr255Param text, Boolean draw)
{
	OSStatus	err;
	
	if ( control == nil )
		return paramErr;

	err = SetControlData( control, 0, kControlStaticTextTextTag, text[0], (Ptr)(text+1) );
	if ( (err == noErr) && draw )
		DrawOneControl( control );
	
	return err;
}

pascal OSStatus GetEditTextText(ControlHandle control, StringPtr text)
{
	Size		actualSize;
	OSStatus	err;
	
	if ( control == nil )
		return paramErr;
		
	if ( text == nil )
		return paramErr;
		
	err = GetControlData( control, 0, kControlEditTextTextTag, 255, (Ptr)(text + 1), &actualSize );
	if ( err == noErr )
		text[0] = MIN( 255, actualSize );
		
	return err;
}

pascal OSStatus SetEditTextText(ControlHandle control, ConstStr255Param text, Boolean draw)
{
	OSStatus	err;
	
	if ( control == nil )
		return paramErr;

	err = SetControlData( control, 0, kControlEditTextTextTag, text[0], (Ptr)(text+1) );
	if ( (err == noErr) && draw )
		DrawOneControl( control );
	
	return err;
}
#endif

static void FloatToString(float num, Str255 string)
{
	long			d,
					f;
	Str63			s;
	
	//Get decimal part
	if(num >= 0.0) {
		d = num;
		NumToString(d, string);
	}
	else {
		num = -num;
		d = num;
		NumToString(d, string);
		BlockMove(&string[1], &string[2], string[0]);
		string[1] = '-';
		++string[0];
	}
	string[string[0] + 1] = '.';
	++string[0];
	
	//Get fractionnal part
	f = (num - (float) d) * 1000.0;
	NumToString(f, s);
	if(s[0] == 1) {
		s[3] = s[1];
		s[1] = '0';
		s[2] = '0';
		s[0] = 3;
	} else if(s[0] == 2) {
		s[3] = s[2];
		s[2] = s[1];
		s[1] = '0';
		s[0] = 3;
	}
	BlockMove(&s[1], &string[string[0] + 1], s[0]);
	string[0] += s[0];
}

static void StringToFloat(Str255 string, float* num)
{
	long			i,
					n = 0,
					d,
					t;
	float			f;
	Str63			s;
	Boolean		minus = false;
	
	//Check for minus
	if(string[1] == '-') {
		BlockMove(&string[2], &string[1], string[0]);
		--string[0];
		minus = true;
	}
		
	//Find '.'
	for(i = 1; i <= string[0]; ++i)
	if(string[i] == '.') {
		n = i;
		break;
	}
	
	//Extract decimal part
	if(n == 0)
	StringToNum(string, &d);
	else if(n == 1)
	d = 0;
	else {
		BlockMove(&string[1], &s[1], n - 1);
		s[0] = n - 1;
		StringToNum(s, &d);
	}
	
	//Extract fractionnal part
	if(n == 0)
	f = 0.0;
	else {
		BlockMove(&string[n + 1], &s[1], string[0] - n);
		s[0] = string[0] - n;
		StringToNum(s, &t);
		f = (float) t;
		for(i = 0; i < string[0] - n; ++i)
		f /= 10.0;
	}
	
	//Build num
	*num = (float) d + f;
	if(minus)
	*num =-*num;
}

void SetDialogItemFloat(Handle itemHandle, float num)
{
	Str63			text;
	
	FloatToString(num, text);
	SetDialogItemText(itemHandle, text);
}

void GetDialogItemFloat(Handle itemHandle, float* num)
{
	Str63			text;
	
	GetDialogItemText(itemHandle, text);
	StringToFloat(text, num);
}

//Event filter Proc

void Press_Button(DialogPtr theDialog, short itemNum)
{
	Rect				buttonRect;
	short				buttonType;
	Handle				buttonHandle;
	unsigned long		ticks;
	
	GetDialogItem(theDialog, itemNum, &buttonType, &buttonHandle, &buttonRect);
	HiliteControl((ControlHandle) buttonHandle, kControlButtonPart);
	Delay(kEnterDelay, &ticks);
}
	
pascal Boolean EventFilterProc(DialogPtr theDialog, EventRecord *theEvent, DialogItemIndex *itemHit)
{
	GrafPtr				savePort;
	
	if(theEvent->what == updateEvt) {
		GetPort(&savePort);
			
		/*BeginUpdate(mainWin);
			Update_MainWin();
		EndUpdate(mainWin);*/
		
		BeginUpdate(textureWin);
			Update_TextureWin();
		EndUpdate(textureWin);
		
		BeginUpdate(shapeWin);
			Update_ShapeWin();
		EndUpdate(shapeWin);
		
		BeginUpdate(scriptWin);
			Update_ScriptWin();
		EndUpdate(scriptWin);
		
		BeginUpdate(mappingWin);
			Update_MappingWin();
		EndUpdate(mappingWin);
		
		BeginUpdate(sequencerWin);
			Update_SequencerWin();
		EndUpdate(sequencerWin);
		
		SetPort(savePort);
	}
	else if((theEvent->what == keyDown) || (theEvent->what == autoKey)) {
		char theChar = (theEvent->message & charCodeMask);
		short theKey = (theEvent->message & keyCodeMask) >> 8;
	
		if((theKey == 0x24) || (theKey == 0x4C) || (theKey == 0x34)) {
			*itemHit = 1;
			Press_Button(theDialog, 1);
			return true;
		} else if(theKey == 0x35) {
			*itemHit = 2;
			Press_Button(theDialog, 2);
			return true;
		}
		
		if((theChar == '.') && (theEvent->modifiers & cmdKey)) {
			*itemHit = 2;
			Press_Button(theDialog, 2);
			return true;
		}
	}
	
	return false;
}

//Settings dialogs:

void Object_Setting(ObjectPtr object)
{
	short			itemType;
	Rect			aRect;
	Handle			items[10];
	Str31			numShapes,
					numPoints,
					numFaces,
					numTextures;
	long			i,
					count;
	GrafPtr			savePort;
	float			temp;
	UniversalProcPtr		EventFilterRoutine = NewModalFilterProc(EventFilterProc);
	
	GetPort(&savePort);
	theDialog = GetNewDialog(201, nil, (WindowPtr) -1);
	SetPort(theDialog);
	for(i = 0; i < 10; ++i)
	GetDialogItem(theDialog, 3 + i, &itemType, &items[i], &aRect);
	
	SetDialogItemText(items[0], object->name);
	if(object->flags & kFlag_Collision)
	SetControlValue((ControlHandle) items[1], 1);
	SetDialogItemFloat(items[2], object->pos.w.x);
	SetDialogItemFloat(items[3], object->pos.w.y);
	SetDialogItemFloat(items[4], object->pos.w.z);
	SetDialogItemFloat(items[5], RadiansToDegrees(object->rotateX));
	SetDialogItemFloat(items[6], RadiansToDegrees(object->rotateY));
	SetDialogItemFloat(items[7], RadiansToDegrees(object->rotateZ));
	SetDialogItemFloat(items[8], object->scale * 100.0);
	numShapes[0] = 4;
	BlockMove(&object->id, &numShapes[1], 4);
	SetDialogItemText(items[9], numShapes);
	
	NumToString(object->shapeCount, numShapes);
	count = 0;
	for(i = 0; i < object->shapeCount; ++i)
	count += object->shapeList[i]->triangleCount;
	NumToString(count, numFaces);
	count = 0;
	for(i = 0; i < object->shapeCount; ++i)
	count += object->shapeList[i]->pointCount;
	NumToString(count, numPoints);
	count = 0;
	for(i = 0; i < textureCount; ++i)
	count += GetPtrSize((Ptr) textureStorageList[i]) - sizeof(TextureStorage);
	count /= 1024;
	NumToString(count, numTextures);
	ParamText(numShapes, numFaces, numTextures, numPoints);
	
	DrawDialog(theDialog);
	SelectDialogItemText(theDialog, 3, 0, 32000);
	SetDialogDefaultItem(theDialog, 1);
	
	do {
		ModalDialog(EventFilterRoutine, &itemType);
		switch(itemType) {
			
			case 4:
			i = GetControlValue((ControlHandle) items[1]);
			SetControlValue((ControlHandle) items[1], !i);
			break;
			
		}
	} while((itemType != 1) && (itemType != 2));
	
	if(itemType == 2) {
		DisposeDialog(theDialog);
		SetPort(savePort);
		return;
	}
	
	GetDialogItemText(items[0], object->name);
	
	object->flags = 0;
	if(GetControlValue((ControlHandle) items[1]))
	object->flags |= kFlag_Collision;
	
	GetDialogItemFloat(items[2], &object->pos.w.x);
	GetDialogItemFloat(items[3], &object->pos.w.y);
	GetDialogItemFloat(items[4], &object->pos.w.z);
	
	GetDialogItemFloat(items[5], &temp);
	object->rotateX = DegreesToRadians(temp);
	GetDialogItemFloat(items[6], &temp);
	object->rotateY = DegreesToRadians(temp);
	GetDialogItemFloat(items[7], &temp);
	object->rotateZ = DegreesToRadians(temp);
	
	GetDialogItemFloat(items[8], &temp);
	object->scale = temp / 100.0;
	
	GetDialogItemText(items[9], numShapes);
	BlockMove(&numShapes[1], &object->id, 4);
	
	DisposeDialog(theDialog);
	SetPort(savePort);
	Object_UpdateMatrix(object);
}

static long Shape_GetNumFromID(OSType ID)
{
	long				i;
	
	for(i = 0; i < theObject.shapeCount; ++i)
	if(theObject.shapeList[i]->id == ID)
	return i;
	
	return -1;
}

static OSType Shape_GetIDFromNum(long num)
{
	return theObject.shapeList[num]->id;
}

void Shape_Setting(ShapePtr shape, ShapePtr shapeList[])
{
	short			itemType;
	Rect			aRect;
	Handle			items[19];
	Str31			numPoints,
					numTris;
	long			i;
	GrafPtr			savePort;
	float			temp;
	MenuHandle		theMenu;
	long			wasLinked;
	UniversalProcPtr		EventFilterRoutine = NewModalFilterProc(EventFilterProc);
	
	theMenu = GetMenu(202);
	for(i = 0; i < theObject.shapeCount; ++i) {
		InsertMenuItem(theMenu, theObject.shapeList[i]->name, 200);
		if(theObject.shapeList[i] == shape)
		DisableItem(theMenu, i + 3);
	}
	
	GetPort(&savePort);
	theDialog = GetNewDialog(200, nil, (WindowPtr) -1);
	SetPort(theDialog);
	for(i = 0; i < 19; ++i)
	GetDialogItem(theDialog, 3 + i, &itemType, &items[i], &aRect);
	
	SetDialogItemText(items[0], shape->name);
	if(shape->flags & kFlag_RelativePos)
	SetControlValue((ControlHandle) items[1], 1);
	if(shape->flags & kFlag_MayHide)
	SetControlValue((ControlHandle) items[2], 1);
	SetDialogItemFloat(items[3], shape->pos.w.x);
	SetDialogItemFloat(items[4], shape->pos.w.y);
	SetDialogItemFloat(items[5], shape->pos.w.z);
	SetDialogItemFloat(items[6], RadiansToDegrees(shape->rotateX));
	SetDialogItemFloat(items[7], RadiansToDegrees(shape->rotateY));
	SetDialogItemFloat(items[8], RadiansToDegrees(shape->rotateZ));
	SetDialogItemFloat(items[9], shape->scale * 100.0);
	SetControlValue((ControlHandle) items[10], shape->shading + 1);
	if(shape->backfaceCulling)
	SetControlValue((ControlHandle) items[11], 1);
	if(shape->textureFilter == kDefaultTextureFilter)
	SetControlValue((ControlHandle) items[12], 1);
	else
	SetControlValue((ControlHandle) items[12], 3 + shape->textureFilter);
	SetDialogItemFloat(items[13], shape->alpha);
	SetDialogItemFloat(items[14], shape->glow);
	SetDialogItemFloat(items[15], shape->difuse);
	SetDialogItemFloat(items[16], shape->specular);
	
	wasLinked = shape->link;
	if(shape->link == kNoLink)
	SetControlValue((ControlHandle) items[17], 1);
	else
	SetControlValue((ControlHandle) items[17], 3 + Shape_GetNumFromID(shape->link));
	
	numPoints[0] = 4;
	BlockMove(&shape->id, &numPoints[1], 4);
	SetDialogItemText(items[18], numPoints);
	
	NumToString(shape->pointCount, numPoints);
	NumToString(shape->triangleCount, numTris);
	ParamText(numPoints, numTris, nil, nil);
	
	DrawDialog(theDialog);
	SelectDialogItemText(theDialog, 3, 0, 32000);
	SetDialogDefaultItem(theDialog, 1);
	
	do {
		ModalDialog(EventFilterRoutine, &itemType);
		switch(itemType) {
			
			case 4:
			i = GetControlValue((ControlHandle) items[1]);
			SetControlValue((ControlHandle) items[1], !i);
			break;
			
			case 5:
			i = GetControlValue((ControlHandle) items[2]);
			SetControlValue((ControlHandle) items[2], !i);
			break;
			
			case 14:
			i = GetControlValue((ControlHandle) items[11]);
			SetControlValue((ControlHandle) items[11], !i);
			break;
			
		}
	} while((itemType != 1) && (itemType != 2));
	
	if(itemType == 2) {
		DisposeDialog(theDialog);
		ReleaseResource((Handle) theMenu);
		SetPort(savePort);
		return;
	}
	
	GetDialogItemText(items[0], shape->name);
	LSetCell(&shape->name[1], shape->name[0], shapeCell, shapeListHandle);
	
	shape->flags = 0;
	if(GetControlValue((ControlHandle) items[1]))
	shape->flags |= kFlag_RelativePos;
	if(GetControlValue((ControlHandle) items[2]))
	shape->flags |= kFlag_MayHide;
	
	GetDialogItemFloat(items[3], &shape->pos.w.x);
	GetDialogItemFloat(items[4], &shape->pos.w.y);
	GetDialogItemFloat(items[5], &shape->pos.w.z);
	
	GetDialogItemFloat(items[6], &temp);
	shape->rotateX = DegreesToRadians(temp);
	GetDialogItemFloat(items[7], &temp);
	shape->rotateY = DegreesToRadians(temp);
	GetDialogItemFloat(items[8], &temp);
	shape->rotateZ = DegreesToRadians(temp);
	
	GetDialogItemFloat(items[9], &temp);
	shape->scale = temp / 100.0;
	
	shape->shading = GetControlValue((ControlHandle) items[10]) - 1;
	
	if(GetControlValue((ControlHandle) items[11]))
	shape->backfaceCulling = 1;
	else
	shape->backfaceCulling = 0;
	
	if(GetControlValue((ControlHandle) items[12]) == 1)
	shape->textureFilter = kDefaultTextureFilter;
	else
	shape->textureFilter = GetControlValue((ControlHandle) items[12]) - 3;
	
	GetDialogItemFloat(items[13], &shape->alpha);
	GetDialogItemFloat(items[14], &shape->glow);
	GetDialogItemFloat(items[15], &shape->difuse);
	GetDialogItemFloat(items[16], &shape->specular);
	
	i = GetControlValue((ControlHandle) items[17]);
	if(i == 1)
	shape->link = kNoLink;
	else {
		if(wasLinked != Shape_GetIDFromNum(i - 3)) {
			Matrix r1;
			ShapePtr tempShape_0, tempShape_1, tempShape_2;
			Matrix_Clear(&r1);
			
			shape->link = shapeList[i - 3]->id;
			
			tempShape_0 = Shape_GetFromID(shape->link);
			if(tempShape_0->link != kNoLink) {
				tempShape_1 = Shape_GetFromID(tempShape_0->link);
				if(tempShape_1->link != kNoLink) {
					tempShape_2 = Shape_GetFromID(tempShape_1->link);
					Matrix_Cat(&tempShape_2->pos, &r1, &r1);
				}
				Matrix_Cat(&tempShape_1->pos, &r1, &r1);
			}
			Matrix_Cat(&tempShape_0->pos, &r1, &r1);
			
			shape->pos.w.x -= r1.w.x;
			shape->pos.w.y -= r1.w.y;
			shape->pos.w.z -= r1.w.z;
		}
	}
	
	GetDialogItemText(items[18], numPoints);
	BlockMove(&numPoints[1], &shape->id, 4);
	
	DisposeDialog(theDialog);
	SetPort(savePort);
	Shape_UpdateMatrix(shape);
	ReleaseResource((Handle) theMenu);
}

void Script_Setting(ScriptPtr script)
{
	short			itemType;
	Rect			aRect;
	Handle			items[5];
	Str31			numShapes,
					numEvents,
					length;
	long			i,
					count;
	GrafPtr			savePort;
	float			temp;
	UniversalProcPtr		EventFilterRoutine = NewModalFilterProc(EventFilterProc);
	
	GetPort(&savePort);
	theDialog = GetNewDialog(202, nil, (WindowPtr) -1);
	SetPort(theDialog);
	for(i = 0; i < 5; ++i)
	GetDialogItem(theDialog, 3 + i, &itemType, &items[i], &aRect);
	
	SetDialogItemText(items[0], script->name);
	if(script->flags & kFlag_Loop)
	SetControlValue((ControlHandle) items[1], 1);
	if(script->flags & kFlag_ResetOnLoop)
	SetControlValue((ControlHandle) items[2], 1);
	if(script->flags & kFlag_SmoothStart)
	SetControlValue((ControlHandle) items[3], 1);
	numShapes[0] = 4;
	BlockMove(&script->id, &numShapes[1], 4);
	SetDialogItemText(items[4], numShapes);
	
	NumToString(script->animationCount, numShapes);
	count = 0;
	for(i = 0; i < script->animationCount; ++i)
	count += script->animationList[i]->eventCount;
	NumToString(count, numEvents);
	NumToString(script->length, length);
	ParamText(numShapes, numEvents, length, nil);
	
	DrawDialog(theDialog);
	SelectDialogItemText(theDialog, 3, 0, 32000);
	SetDialogDefaultItem(theDialog, 1);
	
	do {
		ModalDialog(EventFilterRoutine, &itemType);
		switch(itemType) {
			
			case 4:
			i = GetControlValue((ControlHandle) items[1]);
			SetControlValue((ControlHandle) items[1], !i);
			break;
			
			case 5:
			i = GetControlValue((ControlHandle) items[2]);
			SetControlValue((ControlHandle) items[2], !i);
			break;
			
			case 6:
			i = GetControlValue((ControlHandle) items[3]);
			SetControlValue((ControlHandle) items[3], !i);
			break;
			
		}
	} while((itemType != 1) && (itemType != 2));
	
	if(itemType == 2) {
		DisposeDialog(theDialog);
		SetPort(savePort);
		return;
	}
	
	GetDialogItemText(items[0], script->name);
	LSetCell(&script->name[1], script->name[0],  scriptCell, scriptListHandle);
	
	script->flags = 0;
	if(GetControlValue((ControlHandle) items[1]))
	script->flags |= kFlag_Loop;
	if(GetControlValue((ControlHandle) items[2]))
	script->flags |= kFlag_ResetOnLoop;
	if(GetControlValue((ControlHandle) items[3]))
	script->flags |= kFlag_SmoothStart;
	
	GetDialogItemText(items[4], numShapes);
	BlockMove(&numShapes[1], &script->id, 4);
	
	DisposeDialog(theDialog);
	SetPort(savePort);
}

void Event_Setting(EventPtr event)
{
	short			itemType;
	Rect			aRect;
	Handle			items[7];
	Str31			time;;
	GrafPtr			savePort;
	float			temp;
	long			i;
	UniversalProcPtr		EventFilterRoutine = NewModalFilterProc(EventFilterProc);
	
	GetPort(&savePort);
	theDialog = GetNewDialog(203, nil, (WindowPtr) -1);
	SetPort(theDialog);
	for(i = 0; i < 7; ++i)
	GetDialogItem(theDialog, 3 + i, &itemType, &items[i], &aRect);
	
	NumToString(event->time, time);
	SetDialogItemText(items[0], time);
	SetDialogItemFloat(items[1], event->position.x);
	SetDialogItemFloat(items[2], event->position.y);
	SetDialogItemFloat(items[3], event->position.z);
	SetDialogItemFloat(items[4], RadiansToDegrees(event->rotateX));
	SetDialogItemFloat(items[5], RadiansToDegrees(event->rotateY));
	SetDialogItemFloat(items[6], RadiansToDegrees(event->rotateZ));
	
	DrawDialog(theDialog);
	SelectDialogItemText(theDialog, 3, 0, 32000);
	SetDialogDefaultItem(theDialog, 1);
	
	do {
		ModalDialog(EventFilterRoutine, &itemType);
	} while((itemType != 1) && (itemType != 2));
	
	if(itemType == 2) {
		DisposeDialog(theDialog);
		SetPort(savePort);
		return;
	}
	
	GetDialogItemText(items[0], time);
	StringToNum(time, &event->time);
	if(event->time < 1)
	event->time = 1;
	if(event->time > kMaxTime * kTimeUnit)
	event->time = kMaxTime * kTimeUnit;
	
	GetDialogItemFloat(items[1], &event->position.x);
	GetDialogItemFloat(items[2], &event->position.y);
	GetDialogItemFloat(items[3], &event->position.z);
	
	GetDialogItemFloat(items[4], &temp);
	event->rotateX = DegreesToRadians(temp);
	GetDialogItemFloat(items[5], &temp);
	event->rotateY = DegreesToRadians(temp);
	GetDialogItemFloat(items[6], &temp);
	event->rotateZ = DegreesToRadians(temp);
	
	DisposeDialog(theDialog);
	SetPort(savePort);
}

/*static Boolean Get_SmoothingAngle(float* angle)
{
	short			itemType;
	Rect			aRect;
	Handle			items[1];
	GrafPtr			savePort;
	
	GetPort(&savePort);
	theDialog = GetNewDialog(400, nil, (WindowPtr) -1);
	SetPort(theDialog);
	//TextFont(3);
	//TextSize(10);
	GetDialogItemAsControl(theDialog, 3, &itemType, &items[0], &aRect);
	
	DrawDialog(theDialog);
	OutLine_Item(theDialog, 4);
	SelectDialogItemText(theDialog, 3, 0, 32000);
	SetDialogDefaultItem(theDialog, 1);
	
	do {
		ModalDialog(EventFilterRoutine, &itemType);
	} while((itemType != 1) && (itemType != 2));
	
	if(itemType == 2) {
		DisposeDialog(theDialog);
		SetPort(savePort);
		return false;
	}
	
	GetDialogItemFloat(items[0], angle);
	
	DisposeDialog(theDialog);
	SetPort(savePort);
	
	return true;
}*/

static short globalTextureNum;

pascal void TextureItemProc(WindowPtr dialog, short theItem)
{
	Rect				iRect,
						textureRect;
	Handle				iHndl;
	short				iType;
	TextureStoragePtr	storage = textureStorageList[globalTextureNum];
	Point				thePoint = {0,0};
	GWorldPtr			oldGWorld;
	GDHandle			oldGDHandle;
	RGBColor			oldColor;

	GetDialogItem(dialog, theItem, &iType, &iHndl, &iRect);
	FrameRect(&iRect);
	InsetRect(&iRect, 1, 1);
	
	GetGWorld(&oldGWorld, &oldGDHandle);
	SetGWorld(textureGWorld, nil);
	Texture_Draw(storage, GetWindowPixMapPtr(textureGWorld), thePoint);
	SetGWorld(oldGWorld, oldGDHandle);
	
	SetRect(&textureRect, 0, 0, storage->width, storage->height);
	GetBackColor(&oldColor);
	BackColor(whiteColor);
	CopyBits(GWBitMapPtr(textureGWorld), GWBitMapPtr(dialog), &textureRect, &iRect, srcCopy, nil);
	RGBBackColor(&oldColor);
}

void Texture_Setting(short textureNum)
{
	short				itemType;
	Rect				aRect;
	Handle				items[4];
	Str31				name,
						width,
						height,
						size;
	long				i;
	GrafPtr				savePort;
	TextureStoragePtr	storage = textureStorageList[textureNum];
	float				xRatio,
						yRatio;
	OSType				oldTextureID = storage->name;
	UniversalProcPtr		EventFilterRoutine = NewModalFilterProc(EventFilterProc),
							TextureItemRoutine = NewUserItemProc(TextureItemProc);
	
	GetPort(&savePort);
	theDialog = GetNewDialog(204, nil, (WindowPtr) -1);
	SetPort(theDialog);
	for(i = 0; i < 4; ++i)
	GetDialogItem(theDialog, 3 + i, &itemType, &items[i], &aRect);
	
	name[0] = 4;
	BlockMove(&storage->name, &name[1], 4);
	SetDialogItemText(items[0], name);
	HiliteControl((ControlHandle) items[1], 255);
	if(storage->flags & kFlag_HasAlpha)
	SetControlValue((ControlHandle) items[1], 1);
	if(storage->flags & kFlag_MipMap)
	SetControlValue((ControlHandle) items[2], 1);
	
	NumToString(storage->width, width);
	NumToString(storage->height, height);
	NumToString((GetPtrSize((Ptr) storage) - sizeof(TextureStorage)) / 1024, size);
	ParamText(width, height, size, nil);
	
	if((storage->width <= aRect.right - aRect.left) && (storage->height <= aRect.bottom - aRect.top)) {
		aRect.right = aRect.left + storage->width + 2;
		aRect.bottom = aRect.top + storage->height + 2;
	}
	else {
		xRatio = (float) storage->width / (float) (aRect.right - aRect.left);
		yRatio = (float) storage->height / (float) (aRect.bottom - aRect.top);
		if(xRatio > yRatio) {
			aRect.bottom = aRect.top + storage->height / xRatio + 2;
			aRect.right = aRect.left + storage->width / xRatio + 2;
		}
		else {
			aRect.bottom = aRect.top + storage->height / yRatio + 2;
			aRect.right = aRect.left + storage->width / yRatio + 2;
		}
	}
	globalTextureNum = textureNum;
	SetDialogItem(theDialog, 6, itemType, (Handle) TextureItemRoutine, &aRect);
	
	DrawDialog(theDialog);
	SelectDialogItemText(theDialog, 3, 0, 32000);
	SetDialogDefaultItem(theDialog, 1);
	
	do {
		ModalDialog(EventFilterRoutine, &itemType);
		switch(itemType) {
			
			case 4:
			i = GetControlValue((ControlHandle) items[1]);
			SetControlValue((ControlHandle) items[1], !i);
			break;
			
			case 5:
			i = GetControlValue((ControlHandle) items[2]);
			SetControlValue((ControlHandle) items[2], !i);
			break;
			
		}
	} while((itemType != 1) && (itemType != 2));
	
	if(itemType == 2) {
		DisposeDialog(theDialog);
		SetPort(savePort);
		return;
	}
	
	GetDialogItemText(items[0], name);
	BlockMove(&name[1], &storage->name, 4);
	if(storage->name != oldTextureID) {
		LSetCell(&storage->name, 4, textureCell, textureListHandle);
		nameList[textureNum] = storage->name;
		for(i = 0; i < theObject.shapeCount; ++i)
		if(theObject.shapeList[i]->texture == oldTextureID)
		theObject.shapeList[i]->texture = storage->name;
	}
	
	storage->flags = 0;
	if(GetControlValue((ControlHandle) items[1]))
	storage->flags |= kFlag_HasAlpha;
	if(GetControlValue((ControlHandle) items[2]))
	storage->flags |= kFlag_MipMap;
	
	DisposeDialog(theDialog);
	SetPort(savePort);
}

void Preferences_Setting(PreferencesPtr prefs)
{
	short				itemType;
	Rect				aRect;
	Handle				items[6];
	long				i;
	GrafPtr				savePort;
	float				temp;
	MenuHandle			theMenu;
	GDHandle			mainDevice = GetMainDevice();
	TQAEngine			*engine;
	TQADevice			raveDevice;
	Str255				name,
						textureRAM;
	RGBColor			theColor;
	Point				where = {-1,-1};
	UniversalProcPtr		EventFilterRoutine = NewModalFilterProc(EventFilterProc);
	
	theMenu = GetMenu(203);
	raveDevice.deviceType = kQADeviceGDevice;
	raveDevice.device.gDevice = mainDevice;
	engine = QADeviceGetFirstEngine(&raveDevice);
	while(engine != NULL)
	{
		QAEngineGestalt(engine, kQAGestalt_ASCIINameLength, &i);
		QAEngineGestalt(engine, kQAGestalt_ASCIIName, &name[1]);
		name[0] = i;
		
		QAEngineGestalt(engine, kQAGestalt_TextureMemory, &i);
		NumToString(i / 1024, textureRAM);
		textureRAM[textureRAM[0] + 1] = 'K';
		textureRAM[textureRAM[0] + 2] = 'b';
		textureRAM[0] += 2;
		name[name[0] + 1] = ' ';
		name[name[0] + 2] = '-';
		name[name[0] + 3] = ' ';
		BlockMove(&textureRAM[1], &name[name[0] + 4], textureRAM[0]);
		name[0] += 3 + textureRAM[0];
		
		InsertMenuItem(theMenu, name, 100);
		engine = QADeviceGetNextEngine(&raveDevice, engine);
	}
	
	GetPort(&savePort);
	theDialog = GetNewDialog(300, nil, (WindowPtr) -1);
	SetPort(theDialog);
	for(i = 0; i < 6; ++i)
	GetDialogItem(theDialog, 3 + i, &itemType, &items[i], &aRect);
	
	SetDialogItemFloat(items[0], prefs->ambient);
	SetDialogItemFloat(items[1], prefs->local);
	if(prefs->flags & kPref_FlagFiltering)
	SetControlValue((ControlHandle) items[2], 1);
	if(prefs->flags & kPref_FlagReduce)
	SetControlValue((ControlHandle) items[3], 1);
	if(prefs->flags & kPref_FlagCompress)
	SetControlValue((ControlHandle) items[4], 1);
	
	if(prefs->raveEngineID == -1)
	SetControlValue((ControlHandle) items[5], 1);
	else
	SetControlValue((ControlHandle) items[5], 3 + prefs->raveEngineID);
	
	DrawDialog(theDialog);
	SelectDialogItemText(theDialog, 3, 0, 32000);
	SetDialogDefaultItem(theDialog, 1);
	
	do {
		ModalDialog(EventFilterRoutine, &itemType);
		switch(itemType) {
			
			case 5:
			i = GetControlValue((ControlHandle) items[2]);
			SetControlValue((ControlHandle) items[2], !i);
			break;
			
			case 6:
			i = GetControlValue((ControlHandle) items[3]);
			SetControlValue((ControlHandle) items[3], !i);
			break;
			
			case 7:
			i = GetControlValue((ControlHandle) items[4]);
			SetControlValue((ControlHandle) items[4], !i);
			break;
			
			case 9:
			theColor.red = Float2UShort(thePrefs.backColorRed);
			theColor.green = Float2UShort(thePrefs.backColorGreen);
			theColor.blue = Float2UShort(thePrefs.backColorBlue);
			if(GetColor(where, "\pNew background color:", &theColor, &theColor)) {
				thePrefs.backColorRed = UShort2Float(theColor.red);
				thePrefs.backColorGreen = UShort2Float(theColor.green);
				thePrefs.backColorBlue = UShort2Float(theColor.blue);
			}
			//DrawDialog(theDialog);
			break;
			
		}
	} while((itemType != 1) && (itemType != 2));
	
	if(itemType == 2) {
		DisposeDialog(theDialog);
		ReleaseResource((Handle) theMenu);
		SetPort(savePort);
		return;
	}
	
	GetDialogItemFloat(items[0], &prefs->ambient);
	if(prefs->ambient < 0.0)
	prefs->ambient = 0.0;
	if(prefs->ambient > 1.0)
	prefs->ambient = 1.0;
	GetDialogItemFloat(items[1], &prefs->local);
	if(prefs->local < 0.0)
	prefs->local = 0.0;
	if(prefs->local > 10.0)
	prefs->local = 10.0;
	
	prefs->flags = 0;
	if(GetControlValue((ControlHandle) items[2]))
	prefs->flags |= kPref_FlagFiltering;
	if(GetControlValue((ControlHandle) items[3]))
	prefs->flags |= kPref_FlagReduce;
	if(GetControlValue((ControlHandle) items[4]))
	prefs->flags |= kPref_FlagCompress;
	
	i = GetControlValue((ControlHandle) items[5]);
	if(i == 1)
	prefs->raveEngineID = -1;
	else
	prefs->raveEngineID = i - 3;
	
	DisposeDialog(theDialog);
	ReleaseResource((Handle) theMenu);
	SetPort(savePort);
}