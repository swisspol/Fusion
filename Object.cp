#include			"Structures.h"
#include			"Editor Header.h"
#include			"Matrix.h"
#include			"Vector.h"

//PREPROCESSOR FLAGS:

#define				SAVE_PREVIEW				1

//STRUCTURES:

//Old object for file version 0x101 and 0x0100
typedef struct {
	unsigned long	shapeCount;
	ShapePtr		shapeList[20];
	
	unsigned long	flags;
	Matrix			pos;
	float			rotateX;
	float			rotateY;
	float			rotateZ;
	float			scale;
	
	Str63			name;
	
	Vector			boundingBox[8];
} OLD_Object;

//ROUTINES:

OSErr Object_New()
{
	long				i;
	OSErr				theError;
	
	if(object) {
		theError = Object_Close();
		if(theError)
		return theError;
	}
		
	theObject.shapeCount = 0;
	for(i = 0; i < kMaxShapes; ++i)
	theObject.shapeList[i] = nil;
	theObject.flags = 0;
	Matrix_Clear(&theObject.pos);
	theObject.rotateX = 0.0;
	theObject.rotateY = 0.0;
	theObject.rotateZ = 0.0;
	theObject.scale = 1.0;
	for(i = 0; i < kBBSize; ++i) {
		theObject.boundingBox[i].x = 0.0;
		theObject.boundingBox[i].y = 0.0;
		theObject.boundingBox[i].z = 0.0;
	}
	BlockMove("\pUntitled", theObject.name, 9);
	theObject.id = kNoID;
	
	object = true;
	
	return noErr;
}

PicHandle Object_ExtractPreview(FSSpec* theFile)
{
	PicHandle			thePic = nil;
	short				oldResFile = CurResFile();
	short				destFileID;
	
	destFileID = FSpOpenResFile(theFile, fsRdPerm);
	if(destFileID == -1)
	return nil;
	UseResFile(destFileID);
	thePic = (PicHandle) Get1Resource('PICT', kPreviewResID);
	if(thePic != nil)
	DetachResource((Handle) thePic);
	CloseResFile(destFileID);
	UseResFile(oldResFile);
	
	return thePic;
}
	
pascal short Hook(short item, DialogPtr dialog, FSSpec* theFile)
{
	FInfo			theInfo;
	PicHandle		thePic;
	Rect			destRect = {41,20,126,148};
	
	if(GetWRefCon(WindowPtr(dialog)) != sfMainDialogRefCon)
	return sfHookNullEvent;
	
	if(item == sfHookNullEvent) {
		if(!FSpGetFInfo(theFile, &theInfo)) {
			if(theInfo.fdType == k3DFileType) {
				thePic = Object_ExtractPreview(theFile);
				if(thePic != nil) {
					//BackColor(whiteColor);
					//ForeColor(blackColor);
					DrawPicture(thePic, &destRect);
					DisposeHandle((Handle) thePic);
				}
				else
				EraseRect(&destRect);
			}
			else
			EraseRect(&destRect);
		}
		else
		EraseRect(&destRect);
		InsetRect(&destRect, -1, -1);
		FrameRect(&destRect);
	}
	
	return item;
}

OSErr Object_Open(Boolean merge, Boolean applyObjectMatrix)
{
	StandardFileReply		theReply;
	SFTypeList				theTypeList;
	Point					where = {-1,-1};
	UniversalProcPtr		HookRoutine = NewDlgHookYDProc(Hook);
	
	theTypeList[0] = k3DFileType;
	CustomGetFile(nil, 1, theTypeList, &theReply, 3000, where, HookRoutine, nil, nil, nil, &theReply.sfFile);
	if(!theReply.sfGood)
	return -1;
	
	if(merge)
	return Object_Merge(&theReply.sfFile, applyObjectMatrix);
	
	return Object_Load(&theReply.sfFile);
}

OSErr Object_Load(FSSpec* theFile)
{
	OSErr				theError;
	long				i,
						j,
						bytes;
	short				destFileID;
	Header				theHeader;
	ObjectFileHeader	theFileHeader;
	OLD_Object			oldObject;
	Str63				name;
			
	//Load data
	theError = FSpOpenDF(theFile, fsRdWrPerm, &destFileID);
	if(theError)
	return theError;
	SetFPos(destFileID, fsFromStart, 0);
	
	//Read file header
	bytes = sizeof(ObjectFileHeader);
	FSRead(destFileID, &bytes, &theFileHeader);
	
	//Read object data
	bytes = sizeof(Header);
	FSRead(destFileID, &bytes, &theHeader);
	bytes = theHeader.size;
	
	if(theFileHeader.version > kFile_Version) {
		Do_Error(-5, 101);
		FSClose(destFileID);
		return -5;
	}
	
	if(theFileHeader.flags & kFlag_Protected) {
		Do_Error(-6, 102);
		FSClose(destFileID);
		return -6;
	}
	
	if(theFileHeader.version < 0x0102) {
		FSRead(destFileID, &bytes, &oldObject);
		theObject.shapeCount = oldObject.shapeCount;
		theObject.flags = oldObject.flags;
		theObject.pos = oldObject.pos;
		theObject.rotateX = oldObject.rotateX;
		theObject.rotateY = oldObject.rotateY;
		theObject.rotateZ = oldObject.rotateZ;
		theObject.scale = oldObject.scale;
		BlockMove(oldObject.boundingBox, theObject.boundingBox, sizeof(Vector) * kBBSize);
		BlockMove(oldObject.name, theObject.name, sizeof(Str63));
	}
	else
	FSRead(destFileID, &bytes, &theObject);

	//Set cursor
	SetCursor(*waitCursor);
	
	//Read shapes
	theCell.h = 0;
	for(i = 0; i < theObject.shapeCount; ++i) {
		//Read shape data
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		theObject.shapeList[i] = (ShapePtr) NewPtr(sizeof(Shape));
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, theObject.shapeList[i]);
	
		//Create shape ID
		if(theFileHeader.version < 0x0105) {
			name[1] = name[2] = name[3] = name[4] = '*';
			NumToString(i, name);
			BlockMove(&name[1], &theObject.shapeList[i]->id, 4);
		}
		
		//Read points
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		theObject.shapeList[i]->pointList = (VertexPtr) NewPtr(theHeader.size);
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, theObject.shapeList[i]->pointList);
		
		//Read triangles
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		theObject.shapeList[i]->triangleList = (TriFacePtr) NewPtr(theHeader.size);
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, theObject.shapeList[i]->triangleList);
		
		//Read normals
		if(theObject.shapeList[i]->normalMode != kNoNormals) {
			//Read normal table
			if(theFileHeader.version < 0x0103) {
				//Skip normal table
				bytes = sizeof(Header);
				FSRead(destFileID, &bytes, &theHeader);
				SetFPos(destFileID, fsFromMark, theHeader.size);
				
				//Skip normal list
				bytes = sizeof(Header);
				FSRead(destFileID, &bytes, &theHeader);
				SetFPos(destFileID, fsFromMark, theHeader.size);
				
				//Recalculate normals
				theObject.shapeList[i]->normalMode = kNoNormals;
				Shape_CalculateNormals(theObject.shapeList[i], 2, 0.0);
			}
			else if((theFileHeader.version == 0x0103) || (theFileHeader.version == 0x0104)) {
				//Skip normal list
				bytes = sizeof(Header);
				FSRead(destFileID, &bytes, &theHeader);
				SetFPos(destFileID, fsFromMark, theHeader.size);
				
				//Recalculate normals
				theObject.shapeList[i]->normalMode = kNoNormals;
				Shape_CalculateNormals(theObject.shapeList[i], 2, 0.0);
			}
			else {
				//Read normal list
				bytes = sizeof(Header);
				FSRead(destFileID, &bytes, &theHeader);
				theObject.shapeList[i]->normalList = (VectorPtr) NewPtr(theHeader.size);
				bytes = theHeader.size;
				FSRead(destFileID, &bytes, theObject.shapeList[i]->normalList);
			}
		}
		else {
			theObject.shapeList[i]->unused = 0;
			theObject.shapeList[i]->normalList = nil;
		}
		
		theCell.v = LAddRow(1, 1000, shapeListHandle);
		LSetCell(&theObject.shapeList[i]->name[1], theObject.shapeList[i]->name[0], theCell, shapeListHandle);
		LSetSelect(false, shapeCell, shapeListHandle);
		shapeCell = theCell;
		LSetSelect(true, shapeCell, shapeListHandle);
	}
	
	//Correct shape linking
	if(theFileHeader.version < 0x0105)
	for(i = 0; i < theObject.shapeCount; ++i) {
		if(theObject.shapeList[i]->link != kNoLink)
		theObject.shapeList[i]->link = theObject.shapeList[theObject.shapeList[i]->link]->id;
	}
	
	//Load textures
	theCell.h = 0;
	//if(theFileHeader.flags & kFlag_TextureElements)
	for(i = 0; i < theFileHeader.textureCount; ++i) {
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		textureStorageList[textureCount] = (TextureStoragePtr) NewPtr(theHeader.size);
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, textureStorageList[textureCount]);
		
		textureList[textureCount] = Texture_NewFromStorageTexture(localState->engine, textureStorageList[textureCount]);
		if(textureList[textureCount] == nil)
		return -4;
		BlockMove(&textureStorageList[textureCount]->name, &nameList[textureCount], 4);
		++textureCount;
		
		theCell.v = LAddRow(1, 1000, textureListHandle);
		LSetCell(&textureStorageList[textureCount - 1]->name, 4, theCell, textureListHandle);
		LSetSelect(false, textureCell, textureListHandle);
		textureCell = theCell;
		LSetSelect(true, textureCell, textureListHandle);
	}
	
	//Load cinematic
	theCell.h = 0;
	if((theFileHeader.flags & kFlag_CinematicElements))
	for(i = 0; i < theFileHeader.scriptCount; ++i) {
		//Read script
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		scriptList[scriptCount] = (ScriptPtr) NewPtr(sizeof(Script));
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, scriptList[scriptCount]);
		
		//Read animations
		for(j = 0; j < scriptList[scriptCount]->animationCount; ++j) {
			bytes = sizeof(Header);
			FSRead(destFileID, &bytes, &theHeader);
			scriptList[scriptCount]->animationList[j] = (AnimationPtr) NewPtr(theHeader.size);
			bytes = theHeader.size;
			FSRead(destFileID, &bytes, scriptList[scriptCount]->animationList[j]);
		}
		++scriptCount;
		
		theCell.h = 0;
		theCell.v = LAddRow(1, 1000, scriptListHandle);
		LSetCell(&scriptList[scriptCount - 1]->name[1], scriptList[scriptCount - 1]->name[0], theCell, scriptListHandle);
		LSetSelect(false, scriptCell, scriptListHandle);
		scriptCell = theCell;
		LSetSelect(true, scriptCell, scriptListHandle);
	}
	
	//Correct cinematic
	if(theFileHeader.version < 0x0105)
	for(i = 0; i < theFileHeader.scriptCount; ++i) {
		for(j = 0; j < scriptList[i]->animationCount; ++j)
		scriptList[i]->animationList[j]->shapeID = theObject.shapeList[scriptList[i]->animationList[j]->shapeID]->id;
	}
	
	FSClose(destFileID);
	object = true;
	
	if(theFileHeader.version == 0x0100)
	Object_CalculateBoundingBox(&theObject);
	
	if(theFileHeader.flags & kFlag_PreviewElement)
	previewPic = Object_ExtractPreview(theFile);
	
	//Reset cursor
	InitCursor();
	
	if(theFileHeader.version < kFile_Version)
	Do_Error(-7, 103);
	
	return noErr;
}

OSErr Object_Merge(FSSpec* theFile, Boolean applyObjectMatrix)
{
	OSErr				theError;
	long				i,
						j,
						bytes;
	short				destFileID;
	Header				theHeader;
	ObjectFileHeader	theFileHeader;
	Object				tempObject;
	
	//Load data
	theError = FSpOpenDF(theFile, fsRdWrPerm, &destFileID);
	if(theError)
	return theError;
	SetFPos(destFileID, fsFromStart, 0);
	
	//Read file header
	bytes = sizeof(ObjectFileHeader);
	FSRead(destFileID, &bytes, &theFileHeader);
	
	//Read object data
	bytes = sizeof(Header);
	FSRead(destFileID, &bytes, &theHeader);
	bytes = theHeader.size;
	
	if((theFileHeader.version != kFile_Version)) {
		Do_Error(-5, 101);
		FSClose(destFileID);
		return -5;
	}
	
	if(theFileHeader.flags & kFlag_Protected) {
		Do_Error(-6, 102);
		FSClose(destFileID);
		return -6;
	}
	
	//Set cursor
	SetCursor(*waitCursor);
	
	FSRead(destFileID, &bytes, &tempObject);
	
	//Read shapes
	theCell.h = 0;
	for(i = 0; i < tempObject.shapeCount; ++i) {
		//Read shape data
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		tempObject.shapeList[i] = (ShapePtr) NewPtr(sizeof(Shape));
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, tempObject.shapeList[i]);
	
		//Read points
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		tempObject.shapeList[i]->pointList = (VertexPtr) NewPtr(theHeader.size);
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, tempObject.shapeList[i]->pointList);
		
		//Read triangles
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		tempObject.shapeList[i]->triangleList = (TriFacePtr) NewPtr(theHeader.size);
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, tempObject.shapeList[i]->triangleList);
		
		//Read normals
		if(tempObject.shapeList[i]->normalMode != kNoNormals) {
			//Read normal list
			bytes = sizeof(Header);
			FSRead(destFileID, &bytes, &theHeader);
			tempObject.shapeList[i]->normalList = (VectorPtr) NewPtr(theHeader.size);
			bytes = theHeader.size;
			FSRead(destFileID, &bytes, tempObject.shapeList[i]->normalList);
		}
		else {
			tempObject.shapeList[i]->unused = 0;
			tempObject.shapeList[i]->normalList = nil;
		}
		
		theCell.v = LAddRow(1, 1000, shapeListHandle);
		LSetCell(&tempObject.shapeList[i]->name[1], tempObject.shapeList[i]->name[0], theCell, shapeListHandle);
		LSetSelect(false, shapeCell, shapeListHandle);
		shapeCell = theCell;
		LSetSelect(true, shapeCell, shapeListHandle);
	}
	
	//Load textures
	theCell.h = 0;
	//if(theFileHeader.flags & kFlag_TextureElements)
	for(i = 0; i < theFileHeader.textureCount; ++i) {
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		textureStorageList[textureCount] = (TextureStoragePtr) NewPtr(theHeader.size);
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, textureStorageList[textureCount]);
		
		textureList[textureCount] = Texture_NewFromStorageTexture(localState->engine, textureStorageList[textureCount]);
		if(textureList[textureCount] == nil)
		return -4;
		BlockMove(&textureStorageList[textureCount]->name, &nameList[textureCount], 4);
		++textureCount;
		
		theCell.v = LAddRow(1, 1000, textureListHandle);
		LSetCell(&textureStorageList[textureCount - 1]->name, 4, theCell, textureListHandle);
		LSetSelect(false, textureCell, textureListHandle);
		textureCell = theCell;
		LSetSelect(true, textureCell, textureListHandle);
	}
	
	//Load cinematic
	theCell.h = 0;
	if(theFileHeader.flags & kFlag_CinematicElements)
	for(i = 0; i < theFileHeader.scriptCount; ++i) {
		//Read script
		bytes = sizeof(Header);
		FSRead(destFileID, &bytes, &theHeader);
		scriptList[scriptCount] = (ScriptPtr) NewPtr(sizeof(Script));
		bytes = theHeader.size;
		FSRead(destFileID, &bytes, scriptList[scriptCount]);
		
		//Read animations
		for(j = 0; j < scriptList[scriptCount]->animationCount; ++j) {
			bytes = sizeof(Header);
			FSRead(destFileID, &bytes, &theHeader);
			scriptList[scriptCount]->animationList[j] = (AnimationPtr) NewPtr(theHeader.size);
			bytes = theHeader.size;
			FSRead(destFileID, &bytes, scriptList[scriptCount]->animationList[j]);
		}
		++scriptCount;
		
		theCell.h = 0;
		theCell.v = LAddRow(1, 1000, scriptListHandle);
		LSetCell(&scriptList[scriptCount - 1]->name[1], scriptList[scriptCount - 1]->name[0], theCell, scriptListHandle);
		LSetSelect(false, scriptCell, scriptListHandle);
		scriptCell = theCell;
		LSetSelect(true, scriptCell, scriptListHandle);
	}
	
	FSClose(destFileID);
	
	//Apply object's matrix
	if(applyObjectMatrix)
	Object_ApplyMatrix(&tempObject);
	
	//Merge the 2 objects
	for(i = 0; i < tempObject.shapeCount; ++i) {
		theObject.shapeList[theObject.shapeCount] = tempObject.shapeList[i];
		++theObject.shapeCount;
	}
	Object_CalculateBoundingBox(&theObject);
	
	//Reset cursor
	InitCursor();
	
	return noErr;
}

OSErr Object_Close()
{
	OSErr			theError;
	long			i;
	
	theError = Object_Save();
	if((theError != noErr) && (theError != -1))
	return theError;
	
	object = false;
	
	for(i = 0; i < theObject.shapeCount; ++i)
	Shape_Dispose(theObject.shapeList[i]);
	LDelRow(0, 0, shapeListHandle);
	shapeCell.h = 0;
	shapeCell.v = 0;
	oldShapeCell.h = -1;
	oldShapeCell.v = -1;
	
	for(i = 0; i < textureCount; ++i) {
		DisposePtr((Ptr) textureStorageList[i]);
		QATextureDelete(localState->engine, textureList[i]);
	}
	textureCount = 0;
	LDelRow(0, 0, textureListHandle);
	textureCell.h = 0;
	textureCell.v = 0;
	
	for(i = 0; i < scriptCount; ++i)
	Script_Dispose(scriptList[i]);
	scriptCount = 0;
	LDelRow(0, 0, scriptListHandle);
	scriptCell.h = 0;
	scriptCell.v = 0;
	
	selectedPoints = 0;
	
	if(previewPic != nil)
	DisposeHandle((Handle) previewPic);
	previewPic = nil;
	
	return noErr;
}

OSErr Object_Save()
{
	StandardFileReply		theReply;
	OSErr					theError;
	Point					where = {-1,-1};
	short					destFileID;
	long					i,
							j,
							n;
	Handle					dataHandle;
	Str31					name;
	long					bytes;
	Header					theHeader;		
	ObjectFileHeader		theFileHeader;
	Boolean				protect = false;
	
	if(!object || (theObject.shapeCount == 0))
	return -1;
	
	if(currentScript != nil)
		if(currentScript->flags & kFlag_Running)
		Script_Stop(currentScript);
	
#if SAVE_PREVIEW
	Rect			copyRect,
					finalRect;
	
	if(previewPic == nil) {
		SetRect(&copyRect, 33, 33, 636, 436);
		SetRect(&finalRect, 0, 0, kPreviewH, kPreviewV);
		previewPic = Take_VRAMCapture(GetMainDevice(), &copyRect, &finalRect);
	}
#endif
	
	if(event.modifiers & optionKey)
	protect = true;
	
	StandardPutFile("\pName of the object file:", theObject.name, &theReply);
	if(!theReply.sfGood)
	return -1;
	if(theReply.sfReplacing) {
		theError = FSpDelete(&theReply.sfFile);
		if(theError)
		return theError;
	}
		
	FSpCreateResFile(&theReply.sfFile, k3DFileCreator, k3DFileType, smSystemScript);
	if(ResError())
	return ResError();
	
	//Set cursor
	SetCursor(*waitCursor);
	
	//Update object bounding box
	Object_CalculateBoundingBox(&theObject);
	
	//Save data
	theError = FSpOpenDF(&theReply.sfFile, fsRdWrPerm, &destFileID);
	if(theError)
	return theError;
	SetFPos(destFileID, fsFromStart, 0);
	
	//Write file header
	theFileHeader.version = kFile_Version;
	for(i = 0; i < 15; ++i)
	theFileHeader.elementCount[i] = 0;
	theFileHeader.scriptCount = scriptCount;
	theFileHeader.textureCount = textureCount;
	theFileHeader.flags = 0;
	if(textureCount != 0)
	theFileHeader.flags |= kFlag_TextureElements;
	if(scriptCount != 0)
	theFileHeader.flags |= kFlag_CinematicElements;
#if SAVE_PREVIEW
	if(previewPic != nil)
	theFileHeader.flags |= kFlag_PreviewElement;
#endif
	if(protect)
	theFileHeader.flags |= kFlag_Protected;
	bytes = sizeof(ObjectFileHeader);
	FSWrite(destFileID, &bytes, &theFileHeader);
	
	//Write object data
	theHeader.type = kFile_Object;
	theHeader.size = sizeof(Object);
	bytes = sizeof(Header);
	FSWrite(destFileID, &bytes, &theHeader);
	bytes = sizeof(Object);
	FSWrite(destFileID, &bytes, &theObject);

	//Write shapes
	for(i = 0; i < theObject.shapeCount; ++i) {
		//Write shape data
		theHeader.type = kFile_Shape;
		theHeader.size = sizeof(Shape);
		bytes = sizeof(Header);
		FSWrite(destFileID, &bytes, &theHeader);
		bytes = sizeof(Shape);
		FSWrite(destFileID, &bytes, theObject.shapeList[i]);
	
		//Write points
		theHeader.type = kFile_Points;
		theHeader.size = GetPtrSize((Ptr) theObject.shapeList[i]->pointList);
		bytes = sizeof(Header);
		FSWrite(destFileID, &bytes, &theHeader);
		bytes = GetPtrSize((Ptr) theObject.shapeList[i]->pointList);
		FSWrite(destFileID, &bytes, theObject.shapeList[i]->pointList);
		
		//Write triangles
		theHeader.type = kFile_Triangles;
		theHeader.size = GetPtrSize((Ptr) theObject.shapeList[i]->triangleList);
		bytes = sizeof(Header);
		FSWrite(destFileID, &bytes, &theHeader);
		bytes = GetPtrSize((Ptr) theObject.shapeList[i]->triangleList);
		FSWrite(destFileID, &bytes, theObject.shapeList[i]->triangleList);
		
		//Write normals
		if(theObject.shapeList[i]->normalMode != kNoNormals) {
			//Write normal list
			theHeader.type = kFile_NormalList;
			theHeader.size = GetPtrSize((Ptr) theObject.shapeList[i]->normalList);
			bytes = sizeof(Header);
			FSWrite(destFileID, &bytes, &theHeader);
			bytes = GetPtrSize((Ptr) theObject.shapeList[i]->normalList);
			FSWrite(destFileID, &bytes, theObject.shapeList[i]->normalList);
		}
	}
		
	//Save textures
	for(i = 0; i < textureCount; ++i) {
		theHeader.type = kFile_Texture;
		theHeader.size = GetPtrSize((Ptr) textureStorageList[i]);
		bytes = sizeof(Header);
		FSWrite(destFileID, &bytes, &theHeader);
		bytes = GetPtrSize((Ptr) textureStorageList[i]);
		FSWrite(destFileID, &bytes, textureStorageList[i]);
	}
	
	//Save cinematic
	for(i = 0; i < scriptCount; ++i) {
		//Write script
		theHeader.type = kFile_CinematicScript;
		theHeader.size = sizeof(Script);
		bytes = sizeof(Header);
		FSWrite(destFileID, &bytes, &theHeader);
		bytes = sizeof(Script);
		FSWrite(destFileID, &bytes, scriptList[i]);
		
		//Write animations
		for(j = 0; j < scriptList[i]->animationCount; ++j) {
			theHeader.type = kFile_CinematicAnimation;
			theHeader.size = GetPtrSize((Ptr) scriptList[i]->animationList[j]);
			bytes = sizeof(Header);
			FSWrite(destFileID, &bytes, &theHeader);
			bytes = GetPtrSize((Ptr) scriptList[i]->animationList[j]);
			FSWrite(destFileID, &bytes, scriptList[i]->animationList[j]);
		}
	}
	
	FSClose(destFileID);
	
	//Write preview
#if SAVE_PREVIEW
	short			oldResFile = CurResFile();
	
	if(previewPic == nil)
	return noErr;
	
	destFileID = FSpOpenResFile(&theReply.sfFile, fsRdWrPerm);
	if(destFileID == -1)
	return ResError();
	UseResFile(destFileID);
	
	AddResource((Handle) previewPic, 'PICT', kPreviewResID, "\pNone");
	WriteResource((Handle) previewPic);
	DetachResource((Handle) previewPic);
	
	CloseResFile(destFileID);
	UseResFile(oldResFile);
#endif

	//Reset cursor
	InitCursor();
	
	return noErr;
}

void Object_ApplyMatrix(ObjectPtr object)
{
	long				i,
						j;
	ShapePtr			shape;
	
	for(i = 0; i < object->shapeCount; ++i) {
		shape = object->shapeList[i];
		for(j = 0; j < shape->pointCount; j++)
		Matrix_TransformVector(&object->pos, &shape->pointList[j].point, &shape->pointList[j].point);
		if(shape->normalMode != kNoNormals)
		for(j = 0; j < shape->normalCount; j++) {
			Matrix_RotateVector(&object->pos, &shape->normalList[j], &shape->normalList[j]);
			Vector_Normalize(&shape->normalList[j], &shape->normalList[j]);
		}
		if(shape->flags & kFlag_RelativePos)
		Matrix_RotateVector(&object->pos, &shape->pos.w, &shape->pos.w);
		Shape_CalculateBoundingBox(shape);
	}
	
	object->rotateX = 0.0;
	object->rotateY = 0.0;
	object->rotateZ = 0.0;
	object->scale = 1.0;
	Matrix_Clear(&object->pos);
	
	Object_CalculateBoundingBox(object);
}

void Object_UpdateMatrix(ObjectPtr object)
{
	Vector			p;
	Matrix			m;
	
	p = object->pos.w;
	Matrix_Clear(&object->pos);
	Matrix_ScaleLocal(&object->pos, object->scale, &object->pos);
	
	Matrix_SetRotateX(object->rotateX, &m);
	Matrix_RotateByMatrix(&object->pos, &m);
	
	Matrix_SetRotateY(object->rotateY, &m);
	Matrix_RotateByMatrix(&object->pos, &m);
	
	Matrix_SetRotateZ(object->rotateZ, &m);
	Matrix_RotateByMatrix(&object->pos, &m);
	
	object->pos.w = p;
}

void Object_CalculateBoundingBox(ObjectPtr object)
{
	long			i,
					j;
	float			minX = 30000.0,
					minY = 30000.0,
					minZ = 30000.0,
					maxX = -30000.0,
					maxY = -30000.0,
					maxZ = -30000.0;
	ShapePtr		shape;
	Vector			point;
	Matrix			r1;
	
	for(j = 0; j < object->shapeCount; ++j) {
		shape = object->shapeList[j];
		
		r1 = object->pos;
		Shape_LinkMatrix(shape, &r1);
		if(shape->flags & kFlag_RelativePos)
		Matrix_Cat(&shape->pos, &r1, &r1);
		
		for(i = 0; i < shape->pointCount; ++i) {
			Matrix_TransformVector(&r1, &shape->pointList[i].point, &point);
			
			if(point.x < minX)
			minX = point.x;
			else if(point.x > maxX)
			maxX = point.x;
			
			if(point.y < minY)
			minY = point.y;
			else if(point.y > maxY)
			maxY = point.y;
			
			if(point.z < minZ)
			minZ = point.z;
			else if(point.z > maxZ)
			maxZ = point.z;
			
		}
	}
	
	object->boundingBox[0].x = minX;
	object->boundingBox[0].y = minY;
	object->boundingBox[0].z = minZ;
	
	object->boundingBox[1].x = maxX;
	object->boundingBox[1].y = minY;
	object->boundingBox[1].z = minZ;
	
	object->boundingBox[2].x = maxX;
	object->boundingBox[2].y = maxY;
	object->boundingBox[2].z = minZ;
	
	object->boundingBox[3].x = minX;
	object->boundingBox[3].y = maxY;
	object->boundingBox[3].z = minZ;
	
	object->boundingBox[4].x = minX;
	object->boundingBox[4].y = minY;
	object->boundingBox[4].z = maxZ;
	
	object->boundingBox[5].x = maxX;
	object->boundingBox[5].y = minY;
	object->boundingBox[5].z = maxZ;
	
	object->boundingBox[6].x = maxX;
	object->boundingBox[6].y = maxY;
	object->boundingBox[6].z = maxZ;
	
	object->boundingBox[7].x = minX;
	object->boundingBox[7].y = maxY;
	object->boundingBox[7].z = maxZ;
}