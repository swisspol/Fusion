#include			<fp.h>

#include			"Structures.h"
#include			"Editor Header.h"
#include			"Vector.h"
#include			"Matrix.h"

//CONSTANTES:

#define					kNormalMargin				0.001
#define					kPrecision					4

//STRUCTURES:

typedef struct MiniHeader {
	OSType				type;
	unsigned long		size;
};
typedef MiniHeader* MiniHeaderPtr;

typedef struct Matrix4x4 {
	float	value[4][4];
};

//ROUTINES:

ShapePtr Shape_New()
{
	return (ShapePtr) NewPtrClear(sizeof(Shape));
}

void Shape_Dispose(ShapePtr shape)
{
	if(shape == nil)
	return;
	
	if(shape->pointList != nil)
	DisposePtr((Ptr) shape->pointList);
	if(shape->triangleList != nil)
	DisposePtr((Ptr) shape->triangleList);
	if(shape->normalList != nil)
	DisposePtr((Ptr) shape->normalList);
	
	DisposePtr((Ptr) shape);
}

static void Convert_Matrix(Matrix4x4* source, MatrixPtr dest)
{
	Matrix_Clear(dest);
	
	dest->x.x = source->value[0][0];
	dest->x.y = source->value[0][1];
	dest->x.z = source->value[0][2];
	
	dest->y.x = source->value[1][0];
	dest->y.y = source->value[1][1];
	dest->y.z = source->value[1][2];
	
	dest->z.x = source->value[2][0];
	dest->z.y = source->value[2][1];
	dest->z.z = source->value[2][2];
	
	dest->w.x = source->value[3][0];
	dest->w.y = source->value[3][1];
	dest->w.z = source->value[3][2];
}
	
OSErr Shape_Load3DMF(FSSpec* theFile, Boolean attributesBefore)
{
	short				refNum;
	ShapePtr			shape = nil;
	long				bytes,
						i,
						j,
						nbVertices,
						nbFaces,
						nbTris,
						nbMemTris,
						count = 1,
						*indexList,
						id1,
						id2,
						id3;
	MiniHeader			theHeader;
	OSType				type;
	OSErr				theError;
	Matrix4x4			matrix;
	Matrix				theMatrix;
	Boolean			containerHasMatrix = false;
	float				r = 0.5,
						g = 0.5,
						b = 0.5,
						specular = 20.0,
						alpha = 1.0,
						temp;
	Str31				name;
						
	if(!object)
	return -1;
	
	theError = FSpOpenDF(theFile, fsRdPerm, &refNum);
	if(theError)
	return theError;
	SetFPos(refNum, fsFromStart, 0);
	
	bytes = sizeof(OSType);
	FSRead(refNum, &bytes, &type);
	if(type != '3DMF') {
		FSClose(refNum);
		return -43;
	}
	
	//Set cursor
	SetCursor(*waitCursor);
	
	//Skip header
	SetFPos(refNum, fsFromMark, 20);
	
	do {
		bytes = sizeof(MiniHeader);
		theError = FSRead(refNum, &bytes, &theHeader);
		if(theError == noErr)
		switch(theHeader.type) {
		
			case 'cntr': //container begin
			;
			break;
			
			case 'mtrx': //shape matrix
			bytes = sizeof(Matrix4x4);
			FSRead(refNum, &bytes, &matrix);
			Convert_Matrix(&matrix, &theMatrix);
			containerHasMatrix = true;
			break;
			
			case 'kdif': // diffuse color
			bytes = sizeof(float);
			FSRead(refNum, &bytes, &r);
			bytes = sizeof(float);
			FSRead(refNum, &bytes, &g);
			bytes = sizeof(float);
			FSRead(refNum, &bytes, &b);
			
			if(shape == nil || !attributesBefore)
			break;
			for(i = 0; i < shape->pointCount; ++i) {
				shape->pointList[i].u = r;
				shape->pointList[i].v = g;
				shape->pointList[i].c = b;
			}
			break;
			
			case 'cspc': //specular control
			bytes = sizeof(float);
			FSRead(refNum, &bytes, &specular);
			
			if(shape == nil || !attributesBefore)
			break;
			shape->specular = specular;
			break;
			
			case 'kxpr': //transparency color
			bytes = sizeof(float);
			FSRead(refNum, &bytes, &alpha);
			bytes = sizeof(float);
			FSRead(refNum, &bytes, &temp);
			alpha += temp;
			bytes = sizeof(float);
			FSRead(refNum, &bytes, &temp);
			alpha += temp;
			alpha = alpha / 3.0;
			
			if(shape == nil || !attributesBefore)
			break;
			shape->alpha = alpha;
			break;
			
			/*case 'vasl': //vertex attribute list set
			
			break;*/
			
			case 'mesh': //shape mesh
			if(theObject.shapeCount + 1 >= kMaxShapes) {
				SetFPos(refNum, fsFromMark, theHeader.size);
				break;
			}
			theObject.shapeList[theObject.shapeCount] = Shape_New();
			if(theObject.shapeList[theObject.shapeCount] == nil)
			break;
			shape = theObject.shapeList[theObject.shapeCount];
			++theObject.shapeCount;
			
			bytes = sizeof(long);
			FSRead(refNum, &bytes, &nbVertices);
			shape->pointCount = nbVertices;
			shape->pointList = (VertexPtr) NewPtrClear(sizeof(Vertex) * nbVertices);
			if(shape->pointList == nil) {
				DisposePtr((Ptr) shape);
				shape = nil;
				break;
			}
			
			for(i = 0; i < nbVertices; ++i) {
				bytes = sizeof(float);
				FSRead(refNum, &bytes, &shape->pointList[i].point.x);
				bytes = sizeof(float);
				FSRead(refNum, &bytes, &shape->pointList[i].point.y);
				bytes = sizeof(float);
				FSRead(refNum, &bytes, &shape->pointList[i].point.z);
				
				shape->pointList[i].u = r;
				shape->pointList[i].v = g;
				shape->pointList[i].c = b;
			}
			
			bytes = sizeof(long);
			FSRead(refNum, &bytes, &nbFaces);
			//Skip contours
			SetFPos(refNum, fsFromMark, sizeof(long));
			
			nbMemTris = nbFaces;
			shape->triangleList = (TriFace*) NewPtrClear(sizeof(TriFace) * nbMemTris * 10);
			if(shape->triangleList == nil) {
				DisposePtr((Ptr) shape->pointList);
				DisposePtr((Ptr) shape);
				shape = nil;
				break;
			}
			nbTris = 0;
			for(i = 0; i < nbFaces; ++i) {
				bytes = sizeof(long);
				FSRead(refNum, &bytes, &nbVertices);
				if(nbVertices == 3) {
					bytes = sizeof(long);
					FSRead(refNum, &bytes, &shape->triangleList[nbTris].corner[0]);
					bytes = sizeof(long);
					FSRead(refNum, &bytes, &shape->triangleList[nbTris].corner[1]);
					bytes = sizeof(long);
					FSRead(refNum, &bytes, &shape->triangleList[nbTris].corner[2]);
					++nbTris;
				}
				if(nbVertices > 3) {
					indexList = (long*) NewPtr(sizeof(long) * nbVertices);
					for(j = 0; j < nbVertices; ++j) {
						bytes = sizeof(long);
						FSRead(refNum, &bytes, &indexList[j]);
					}
					shape->triangleList[nbTris].corner[0] = indexList[0];
					shape->triangleList[nbTris].corner[1] = indexList[1];
					shape->triangleList[nbTris].corner[2] = indexList[nbVertices - 1];
					++nbTris;
					id1 = 2;
					id2 = nbVertices - 2;
					id3 = nbTris;
					for(j = 3; j < nbVertices; ++j) {
						shape->triangleList[nbTris].corner[0] = shape->triangleList[nbTris - 1].corner[1];
						shape->triangleList[nbTris].corner[1] = shape->triangleList[nbTris - 1].corner[2];
						if(j % 2 == 1) {
							shape->triangleList[nbTris].corner[2] = indexList[id1];
							++id1;
						}
						else {
							shape->triangleList[nbTris].corner[2] = indexList[id2];
							--id2;
						}
						++nbTris;
					}
					for(j = id3; j < nbTris; j += 2) {
						id1 = shape->triangleList[j].corner[0];
						shape->triangleList[j].corner[0] = shape->triangleList[j].corner[1];
						shape->triangleList[j].corner[1] = id1;
					}
					DisposePtr((Ptr) indexList);
				}
			}
			shape->triangleCount = nbTris;
			SetPtrSize((Ptr) shape->triangleList, (sizeof(TriFace) * nbTris));
			
			shape->normalMode = kNoNormals;
			shape->unused = 0;
			shape->normalCount = 0;
			shape->normalList = nil;
			shape->flags = 0;
			Matrix_Clear(&shape->pos);
			if(containerHasMatrix) {
				for(long p = 0; p < shape->pointCount; ++p)
				Matrix_RotateVector(&theMatrix, &shape->pointList[p].point, &shape->pointList[p].point);
				if(containerHasMatrix)
				Matrix_RotateVector(&theMatrix, &theMatrix.w, &theMatrix.w);
				shape->pos.w.x = -theMatrix.w.x;
				shape->pos.w.y = -theMatrix.w.y;
				shape->pos.w.z = -theMatrix.w.z;
			}
			shape->flags |= kFlag_RelativePos;
			shape->rotateX = 0.0;
			shape->rotateY = 0.0;
			shape->rotateZ = 0.0;
			shape->scale = 1.0;
			shape->link = kNoLink;
			Shape_UpdateMatrix(shape);
			shape->texture = kNoTexture;
			shape->alpha = alpha;
			shape->glow = 0.0;
			shape->difuse = 1.0;
			shape->specular = specular;
			shape->backfaceCulling = 1;
			shape->shading = 0;
			shape->textureFilter = kDefaultTextureFilter;
			NumToString(count, theString);
			BlockMove(theFile->name, shape->name, theFile->name[0] + 1);
			shape->name[shape->name[0] + 1] = ' ';
			BlockMove(&theString[1], &shape->name[shape->name[0] + 2], theString[0]);
			shape->name[0] += theString[0] + 1;
			name[1] = name[2] = name[3] = name[4] = '*';
			NumToString(theObject.shapeCount, name);
			BlockMove(&name[1], &shape->id, 4);
			
			theCell.h = 0;
			theCell.v = LAddRow(1, 1000, shapeListHandle);
			LSetCell(&shape->name[1], shape->name[0], theCell, shapeListHandle);
			LSetSelect(false, shapeCell, shapeListHandle);
			shapeCell = theCell;
			LSetSelect(true, shapeCell, shapeListHandle);
			
			Shape_SwitchBF(shape);
			Shape_CalculateBoundingBox(shape);
			Shape_CalculateNormals(shape, 2, 0.0);
			++count;
			break;
			
			case 'tmsh': //shape trimesh
			if(theObject.shapeCount + 1 >= kMaxShapes) {
				SetFPos(refNum, fsFromMark, theHeader.size);
				break;
			}
			theObject.shapeList[theObject.shapeCount] = Shape_New();
			if(theObject.shapeList[theObject.shapeCount] == nil)
			break;
			shape = theObject.shapeList[theObject.shapeCount];
			++theObject.shapeCount;
			
			bytes = sizeof(long);
			FSRead(refNum, &bytes, &nbFaces);
			//Skip numTriangleAttributeTypes
			SetFPos(refNum, fsFromMark, sizeof(long));
			bytes = sizeof(long);
			FSRead(refNum, &bytes, &nbTris); //read numEdges
			//Skip numEdgeAttributeTypes
			SetFPos(refNum, fsFromMark, sizeof(long));
			bytes = sizeof(long);
			FSRead(refNum, &bytes, &nbVertices);
			//Skip numVertexAttributeTypes
			SetFPos(refNum, fsFromMark, sizeof(long));
			
			shape->triangleCount = nbFaces;
			shape->triangleList = (TriFace*) NewPtrClear(sizeof(TriFace) * nbFaces);
			if(shape->triangleList == nil) {
				DisposePtr((Ptr) shape->pointList);
				DisposePtr((Ptr) shape);
				shape = nil;
				break;
			}
			
			if(nbVertices < 256) { //vertex indices are stored on 1 byte
				unsigned char index;
				for(i = 0; i < nbFaces; ++i) {
					bytes = sizeof(unsigned char);
					FSRead(refNum, &bytes, &index);
					shape->triangleList[i].corner[0] = index;
					
					bytes = sizeof(unsigned char);
					FSRead(refNum, &bytes, &index);
					shape->triangleList[i].corner[1] = index;
					
					bytes = sizeof(unsigned char);
					FSRead(refNum, &bytes, &index);
					shape->triangleList[i].corner[2] = index;
				}
			}
			else { //vertex indices are stored on 2 bytes
				unsigned short index;
				for(i = 0; i < nbFaces; ++i) {
					bytes = sizeof(unsigned short);
					FSRead(refNum, &bytes, &index);
					shape->triangleList[i].corner[0] = index;
					
					bytes = sizeof(unsigned short);
					FSRead(refNum, &bytes, &index);
					shape->triangleList[i].corner[1] = index;
					
					bytes = sizeof(unsigned short);
					FSRead(refNum, &bytes, &index);
					shape->triangleList[i].corner[2] = index;
				}
			}
			
			//Skip edges
			SetFPos(refNum, fsFromMark, nbTris * 4 * sizeof(long));
	
			shape->pointCount = nbVertices;
			shape->pointList = (VertexPtr) NewPtrClear(sizeof(Vertex) * nbVertices);
			if(shape->pointList == nil) {
				DisposePtr((Ptr) shape);
				shape = nil;
				break;
			}
			
			for(i = 0; i < nbVertices; ++i) {
				bytes = sizeof(float);
				FSRead(refNum, &bytes, &shape->pointList[i].point.x);
				bytes = sizeof(float);
				FSRead(refNum, &bytes, &shape->pointList[i].point.y);
				bytes = sizeof(float);
				FSRead(refNum, &bytes, &shape->pointList[i].point.z);
	
				shape->pointList[i].u = r;
				shape->pointList[i].v = g;
				shape->pointList[i].c = b;
			}
			
			//Skip bounding box
			SetFPos(refNum, fsFromMark, 2 * (3 * sizeof(float)) + sizeof(long));
			
			shape->normalMode = kNoNormals;
			shape->unused = 0;
			shape->normalCount = 0;
			shape->normalList = nil;
			shape->flags = 0;
			Matrix_Clear(&shape->pos);
			if(containerHasMatrix) {
				for(long p = 0; p < shape->pointCount; ++p)
				Matrix_RotateVector(&theMatrix, &shape->pointList[p].point, &shape->pointList[p].point);
				if(containerHasMatrix)
				Matrix_RotateVector(&theMatrix, &theMatrix.w, &theMatrix.w);
				shape->pos.w.x = -theMatrix.w.x;
				shape->pos.w.y = -theMatrix.w.y;
				shape->pos.w.z = -theMatrix.w.z;
			}
			shape->flags |= kFlag_RelativePos;
			shape->rotateX = 0.0;
			shape->rotateY = 0.0;
			shape->rotateZ = 0.0;
			shape->scale = 1.0;
			shape->link = kNoLink;
			Shape_UpdateMatrix(shape);
			shape->texture = kNoTexture;
			shape->alpha = alpha;
			shape->glow = 0.0;
			shape->difuse = 1.0;
			shape->specular = specular;
			shape->backfaceCulling = 1;
			shape->shading = 0;
			shape->textureFilter = kDefaultTextureFilter;
			NumToString(count, theString);
			BlockMove(theFile->name, shape->name, theFile->name[0] + 1);
			shape->name[shape->name[0] + 1] = ' ';
			BlockMove(&theString[1], &shape->name[shape->name[0] + 2], theString[0]);
			shape->name[0] += theString[0] + 1;
			name[1] = name[2] = name[3] = name[4] = '*';
			NumToString(theObject.shapeCount, name);
			BlockMove(&name[1], &shape->id, 4);
			
			theCell.h = 0;
			theCell.v = LAddRow(1, 1000, shapeListHandle);
			LSetCell(&shape->name[1], shape->name[0], theCell, shapeListHandle);
			LSetSelect(false, shapeCell, shapeListHandle);
			shapeCell = theCell;
			LSetSelect(true, shapeCell, shapeListHandle);
			
			Shape_SwitchBF(shape);
			Shape_CalculateBoundingBox(shape);
			Shape_CalculateNormals(shape, 2, 0.0);
			++count;
			break;
			
			default: // unknown tag -> skip
			SetFPos(refNum, fsFromMark, theHeader.size);
			break;
			
		}
	} while(theError == noErr);
	
	FSClose(refNum);
	
	//Reset cursor
	InitCursor();
	
	return noErr;
}

void Shape_SwitchBF(ShapePtr shape)
{
	long				i;
	TriFacePtr			facePtr;
	unsigned long		temp;
		
	facePtr = shape->triangleList;
	for(i = 0; i < shape->triangleCount; ++i) {
		temp = facePtr->corner[0];
		facePtr->corner[0] = facePtr->corner[1];
		facePtr->corner[1] = temp;
		++facePtr;
	}
}

void Shape_ClearNormals(ShapePtr shape)
{
	if(shape->normalMode == kNoNormals)
	return;
	
	if(shape->normalList != nil)
	DisposePtr((Ptr) shape->normalList);
	
	shape->normalMode = kNoNormals;
	shape->shading = 0;
}
	
#if 1
#define				kTolerance				0.01
#define Value_Between(v, r) ((v >= r - kTolerance) && (v <= r + kTolerance))

static Boolean Compare_Normals(VectorPtr n1, VectorPtr n2)
{
	if(Value_Between(n1->x, n2->x) && Value_Between(n1->y, n2->y) && Value_Between(n1->z, n2->z))
	return true;
	
	return false;
}
#endif
	
static Boolean Normals_Similar(VectorPtr n1, VectorPtr n2, float smoothAngle)
{
	Vector			a,
					b;
	float			angle;
	
	Vector_Normalize(n1, &a);
	Vector_Normalize(n2, &b);
	angle = acos(Vector_DotProduct(&a, &b));
	
	if(angle < DegreesToRadians(smoothAngle))
	return true;
	
	return false;
}

static void CalcTriFaceNormal(const ShapePtr shape, long theTriFace, Vector* normal)
{
	TriFacePtr		tri;
	VertexPtr		points;
	Vector			v1,
					v2;
	
	points = shape->pointList;
	tri = &shape->triangleList[theTriFace];
	
	Vector_Subtract(&points[tri->corner[1]].point, &points[tri->corner[0]].point, &v1);
	Vector_Subtract(&points[tri->corner[2]].point, &points[tri->corner[0]].point, &v2);
	Vector_CrossProduct(&v2, &v1, normal);
	Vector_Normalize(normal, normal);
}

void Shape_CalculateNormals(ShapePtr shape, short mode, float smoothAngle)
{
	long					x,
							y,
							c,
							pn,
							*tempNormalTable,
							tempNormalCount,
							normalID;
	Vector					*tempNormalList,
							n;
	Boolean				*normalUsed;
	TriFace					*tri;
	unsigned int			oldShading;
	GrafPtr					savePort;
	DialogPtr				progressDialog;
	ControlHandle			progressBar;
	Boolean				dialogOn = false;
	
	//Show dialog
	if(shape->pointCount > 1000) {
		GetPort(&savePort);
		progressDialog = GetNewDialog(4000, nil, (WindowPtr) -1);
		GetDialogItemAsControl(progressDialog, 1, &progressBar);
		SetControlMaximum(progressBar, (shape->triangleCount + shape->pointCount) >> 3);
		DrawDialog(progressDialog);
		dialogOn = true;
	}
	
	oldShading = shape->shading;
	Shape_ClearNormals(shape);
	
	switch(mode) {
	
		case 1: // flat shading
		shape->normalCount = shape->triangleCount;
		shape->normalList = (Vector*) NewPtrClear(sizeof(Vector) * shape->triangleCount);
		
		for(x = 0; x < shape->triangleCount; x++)
		CalcTriFaceNormal(shape, x, &shape->normalList[x]);
		break;
	
#if 1
		case 2: // per vertex - auto smooth
		shape->normalCount = shape->pointCount;
		shape->normalList = (Vector*) NewPtrClear(sizeof(Vector) * shape->pointCount);
		
		tempNormalCount = 0;
		tempNormalList = (Vector*) NewPtrClear(sizeof(Vector) * shape->triangleCount);
		tempNormalTable = (long*) NewPtrClear(sizeof(long) * shape->triangleCount);
		normalUsed = (Boolean*) NewPtrClear(sizeof(Boolean) * shape->triangleCount);
		
		for(x = 0; x < shape->triangleCount; x++) {
			CalcTriFaceNormal(shape, x, &n);
			normalID = -1;
			for(y = 0; y < tempNormalCount; ++y)
			if(Compare_Normals(&n, &tempNormalList[y])) {
				normalID = y;
				break;
			}
			
			if(normalID == -1) {
				tempNormalList[tempNormalCount] = n;
				tempNormalTable[x] = tempNormalCount;
				++tempNormalCount;
			}
			else
			tempNormalTable[x] = normalID;
			
			if(dialogOn && !(x % 100))
			SetControlValue(progressBar, x >> 3);
		}
		
		for(x = 0; x < shape->pointCount; x++) {
			Vector_Clear(&n);
			for(y = 0; y < tempNormalCount; ++y)
			normalUsed[y] = false;
			
			for(y = 0; y < shape->triangleCount; ++y) {
				tri = &shape->triangleList[y];
				if((tri->corner[0] == x) || (tri->corner[1] == x) || (tri->corner[2] == x)) {
					if(!normalUsed[tempNormalTable[y]]) {
						Vector_Add(&n, &tempNormalList[tempNormalTable[y]], &n);
						normalUsed[tempNormalTable[y]] = true;
					}
				}
			}
			
			Vector_Normalize(&n, &n);
			shape->normalList[x] = n;
			
			if(dialogOn && !((shape->triangleCount + x) % 100))
			SetControlValue(progressBar, (shape->triangleCount + x) >> 3);
		}
		
		DisposePtr((Ptr) tempNormalList);
		DisposePtr((Ptr) tempNormalTable);
		DisposePtr((Ptr) normalUsed);
		mode = 0; // for setting shape->normalMode
		break;
#else
		case 2: // per vertex - auto smooth
		shape->normalCount = shape->triangleCount * 3;
		shape->normalList = (Vector*) NewPtrClear(sizeof(Vector) * shape->triangleCount * 3);
		
		tempNormalCount = 0;
		tempNormalList = (Vector*) NewPtrClear(sizeof(Vector) * shape->triangleCount);
		tempNormalTable = (long*) NewPtrClear(sizeof(long) * shape->triangleCount);
		normalUsed = (Boolean*) NewPtrClear(sizeof(Boolean) * shape->triangleCount);
		
		for(x = 0; x < shape->triangleCount; x++) {
			CalcTriFaceNormal(shape, x, &n);
			normalID = -1;
			for(y = 0; y < tempNormalCount; ++y)
			if(Normals_Similar(&n, &tempNormalList[y], 1.0)) {
				normalID = y;
				break;
			}
				
			if(normalID == -1) {
				tempNormalList[tempNormalCount] = n;
				tempNormalTable[x] = tempNormalCount;
				++tempNormalCount;
			}
			else
			tempNormalTable[x] = normalID;
		}
		
		for(x = 0; x < shape->triangleCount; x++)
		for(c = 0; c < 3; ++c) {
			pn = shape->triangleList[x].corner[c];
			
			Vector_Clear(&n);
			for(y = 0; y < tempNormalCount; ++y)
			normalUsed[y] = false;
			
			n = tempNormalList[tempNormalTable[x]];
			normalUsed[tempNormalTable[x]] = true;
			
			for(y = 0; y < shape->triangleCount; ++y) {
				tri = &shape->triangleList[y];
				if((tri->corner[0] == pn) || (tri->corner[1] == pn) || (tri->corner[2] == pn)) {
					if(!normalUsed[tempNormalTable[y]]) {
						if(Normals_Similar(&n, &tempNormalList[tempNormalTable[y]], smoothAngle)) {
							Vector_Add(&n, &tempNormalList[tempNormalTable[y]], &n);
							normalUsed[tempNormalTable[y]] = true;
						}
					}
				}
			}
			
			Vector_Normalize(&n, &n);
			shape->normalList[x * 3 + c] = n;
		}
		
		DisposePtr((Ptr) tempNormalList);
		DisposePtr((Ptr) tempNormalTable);
		DisposePtr((Ptr) normalUsed);
		mode = 0; // for setting shape->normalMode
		break;
#endif
		
	}
	
	shape->normalMode = mode;
	if(oldShading != 0)
	shape->shading = oldShading;
	else
	shape->shading = 1;
	
	if(dialogOn) {
		DisposeDialog(progressDialog);
		SetPort(savePort);
	}
}

void Shape_CalculateBoundingBox(ShapePtr shape)
{
	long			i;
	float			minX = 30000.0,
					minY = 30000.0,
					minZ = 30000.0,
					maxX = -30000.0,
					maxY = -30000.0,
					maxZ = -30000.0;
	
	for(i = 0; i < shape->pointCount; ++i) {
		
		if(shape->pointList[i].point.x < minX)
		minX = shape->pointList[i].point.x;
		else if(shape->pointList[i].point.x > maxX)
		maxX = shape->pointList[i].point.x;
		
		if(shape->pointList[i].point.y < minY)
		minY = shape->pointList[i].point.y;
		else if(shape->pointList[i].point.y > maxY)
		maxY = shape->pointList[i].point.y;
		
		if(shape->pointList[i].point.z < minZ)
		minZ = shape->pointList[i].point.z;
		else if(shape->pointList[i].point.z > maxZ)
		maxZ = shape->pointList[i].point.z;
		
	}
	
	shape->boundingBox[0].x = minX;
	shape->boundingBox[0].y = minY;
	shape->boundingBox[0].z = minZ;
	
	shape->boundingBox[1].x = maxX;
	shape->boundingBox[1].y = minY;
	shape->boundingBox[1].z = minZ;
	
	shape->boundingBox[2].x = maxX;
	shape->boundingBox[2].y = maxY;
	shape->boundingBox[2].z = minZ;
	
	shape->boundingBox[3].x = minX;
	shape->boundingBox[3].y = maxY;
	shape->boundingBox[3].z = minZ;
	
	shape->boundingBox[4].x = minX;
	shape->boundingBox[4].y = minY;
	shape->boundingBox[4].z = maxZ;
	
	shape->boundingBox[5].x = maxX;
	shape->boundingBox[5].y = minY;
	shape->boundingBox[5].z = maxZ;
	
	shape->boundingBox[6].x = maxX;
	shape->boundingBox[6].y = maxY;
	shape->boundingBox[6].z = maxZ;
	
	shape->boundingBox[7].x = minX;
	shape->boundingBox[7].y = maxY;
	shape->boundingBox[7].z = maxZ;
}

void Shape_Center(ShapePtr shape)
{
	float			xOffset,
					yOffset,
					zOffset;
	long			i;
	float			minX = 30000.0,
					minY = 30000.0,
					minZ = 30000.0,
					maxX = -30000.0,
					maxY = -30000.0,
					maxZ = -30000.0;
	
	for(i = 0; i < shape->pointCount; ++i) {
		
		if(shape->pointList[i].point.x < minX)
		minX = shape->pointList[i].point.x;
		else if(shape->pointList[i].point.x > maxX)
		maxX = shape->pointList[i].point.x;
		
		if(shape->pointList[i].point.y < minY)
		minY = shape->pointList[i].point.y;
		else if(shape->pointList[i].point.y > maxY)
		maxY = shape->pointList[i].point.y;
		
		if(shape->pointList[i].point.z < minZ)
		minZ = shape->pointList[i].point.z;
		else if(shape->pointList[i].point.z > maxZ)
		maxZ = shape->pointList[i].point.z;
		
	}
	
	xOffset = (minX + maxX) / 2;
	yOffset = (minY + maxY) / 2;
	zOffset = (minZ + maxZ) / 2;
	
	for(i = 0; i < shape->pointCount; ++i) {
		shape->pointList[i].point.x -= xOffset;
		shape->pointList[i].point.y -= yOffset;
		shape->pointList[i].point.z -= zOffset;
	}
	Shape_CalculateBoundingBox(shape);
}

void Shape_Scale(ShapePtr shape, float factor)
{
	long			i;
	
	for(i = 0; i < shape->pointCount; ++i) {
		
		shape->pointList[i].point.x *= factor;
		shape->pointList[i].point.y *= factor;
		shape->pointList[i].point.z *= factor;
		
	}
	Shape_CalculateBoundingBox(shape);
}

void Shape_ScaleBox(ShapePtr shape, float size)
{
	long			i;
	float			minX = 30000.0,
					minY = 30000.0,
					minZ = 30000.0,
					maxX = -30000.0,
					maxY = -30000.0,
					maxZ = -30000.0;
	float			width;
	
	for(i = 0; i < shape->pointCount; ++i) {
		
		if(shape->pointList[i].point.x < minX)
		minX = shape->pointList[i].point.x;
		else if(shape->pointList[i].point.x > maxX)
		maxX = shape->pointList[i].point.x;
		
		if(shape->pointList[i].point.y < minY)
		minY = shape->pointList[i].point.y;
		else if(shape->pointList[i].point.y > maxY)
		maxY = shape->pointList[i].point.y;
		
		if(shape->pointList[i].point.z < minZ)
		minZ = shape->pointList[i].point.z;
		else if(shape->pointList[i].point.z > maxZ)
		maxZ = shape->pointList[i].point.z;
		
	}
	
	width = maxX - minX;
	if((maxY - minY) > width)
	width = maxY - minY;
	if((maxZ - minZ) > width)
	width = maxZ - minZ;
	
	Shape_Scale(shape, size / width);
}

void Shape_UpdateMatrix(ShapePtr shape)
{
	Vector			p;
	Matrix			m;
	
	p = shape->pos.w;
	Matrix_Clear(&shape->pos);
	Matrix_ScaleLocal(&shape->pos, shape->scale, &shape->pos);
	
	Matrix_SetRotateX(shape->rotateX, &m);
	Matrix_RotateByMatrix(&shape->pos, &m);
	
	Matrix_SetRotateY(shape->rotateY, &m);
	Matrix_RotateByMatrix(&shape->pos, &m);
	
	Matrix_SetRotateZ(shape->rotateZ, &m);
	Matrix_RotateByMatrix(&shape->pos, &m);
	
	shape->pos.w = p;
}
	
void Shape_ApplyMatrix(ShapePtr shape)
{
	long				i;
	
	if(!(shape->flags & kFlag_RelativePos))
	return;
	
	for(i = 0; i < shape->pointCount; i++)
	Matrix_TransformVector(&shape->pos, &shape->pointList[i].point, &shape->pointList[i].point);
	
	if(shape->normalMode != kNoNormals)
	for(i = 0; i < shape->normalCount; i++) {
		Matrix_RotateVector(&shape->pos, &shape->normalList[i], &shape->normalList[i]);
		Vector_Normalize(&shape->normalList[i], &shape->normalList[i]);
	}
	
	shape->rotateX = 0.0;
	shape->rotateY = 0.0;
	shape->rotateZ = 0.0;
	shape->scale = 1.0;
	Matrix_Clear(&shape->pos);
	shape->flags &= ~kFlag_RelativePos;
	Shape_CalculateBoundingBox(shape);
}

void Shape_MoveOrigin(ShapePtr shape, float moveX, float moveY, float moveZ)
{
	long				i;
	
	for(i = 0; i < shape->pointCount; i++) {
		shape->pointList[i].point.x -= moveX;
		shape->pointList[i].point.y -= moveY;
		shape->pointList[i].point.z -= moveZ;
	}
	
	for(i = 0; i < kBBSize; i++) {
		shape->boundingBox[i].x -= moveX;
		shape->boundingBox[i].y -= moveY;
		shape->boundingBox[i].z -= moveZ;
	}
	
	shape->pos.w.x += moveX * shape->scale;
	shape->pos.w.y += moveY * shape->scale;
	shape->pos.w.z += moveZ * shape->scale;
}
	
void Shape_ReCenterOrigin(ShapePtr shape)
{
	float			xOffset,
					yOffset,
					zOffset;
	long			i;
	float			minX = 30000.0,
					minY = 30000.0,
					minZ = 30000.0,
					maxX = -30000.0,
					maxY = -30000.0,
					maxZ = -30000.0;
	
	for(i = 0; i < shape->pointCount; ++i) {
		
		if(shape->pointList[i].point.x < minX)
		minX = shape->pointList[i].point.x;
		else if(shape->pointList[i].point.x > maxX)
		maxX = shape->pointList[i].point.x;
		
		if(shape->pointList[i].point.y < minY)
		minY = shape->pointList[i].point.y;
		else if(shape->pointList[i].point.y > maxY)
		maxY = shape->pointList[i].point.y;
		
		if(shape->pointList[i].point.z < minZ)
		minZ = shape->pointList[i].point.z;
		else if(shape->pointList[i].point.z > maxZ)
		maxZ = shape->pointList[i].point.z;
		
	}
	
	xOffset = (minX + maxX) / 2;
	yOffset = (minY + maxY) / 2;
	zOffset = (minZ + maxZ) / 2;
	
	for(i = 0; i < shape->pointCount; ++i) {
		shape->pointList[i].point.x -= xOffset;
		shape->pointList[i].point.y -= yOffset;
		shape->pointList[i].point.z -= zOffset;
	}
	Shape_CalculateBoundingBox(shape);
	
	shape->pos.w.x += xOffset * shape->scale;
	shape->pos.w.y += yOffset * shape->scale;
	shape->pos.w.z += zOffset * shape->scale;
}
	
void Shape_ApplyColor(ShapePtr shape, float red, float green, float blue)
{
	long				i;
	
	for(i = 0; i < shape->pointCount; i++) {
		shape->pointList[i].u = red;
		shape->pointList[i].v = green;
		shape->pointList[i].c = blue;
	}
	
	shape->texture = kNoTexture;
}

void Shape_ApplyTexture(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[], OSType texID, float xScale, float xOffset, float yScale, float yOffset, Boolean noWrapping)
{
	long				x,
						c;
	Vector				v1,
						v2;
	Vector				n;
	TQAVTexture			verts[3];
	TQAVTexture*		v;
	VectorPtr			r;
	VertexPtr			p;
	Matrix				r1;
	VectorPtr			points;
	TriFacePtr			tri;
	unsigned long		pn;
	float				pixelConversion = (state->d / state->h) * (-state->viewWidth / 2),
						iw;
	
	Matrix_Negate(camera, &r1);
	Matrix_Cat(globalPos, &r1, &r1);
	Shape_LinkMatrix(shape, &r1);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &r1, &r1);
	
	points = (Vector*) NewPtr(sizeof(Vector) * shape->pointCount);
	for(x = 0; x < shape->pointCount; x++)
	Matrix_TransformVector(&r1, &shape->pointList[x].point, &points[x]);
	
	for(x = 0; x < shape->triangleCount; x++){
		tri = &shape->triangleList[x];
		
		if(shape->backfaceCulling){
			Vector_Subtract(&points[tri->corner[1]], &points[tri->corner[0]], &v1);
			Vector_Subtract(&points[tri->corner[2]], &points[tri->corner[0]], &v2);
			Vector_CrossProduct(&v2, &v1, &n);
			if(n.z > 0) //if(Vector_DotProduct(&points[tri->corner[0]], &n) > 0)
			continue;
		}
		
		for(c = 0; c < 3; c++){
			pn = tri->corner[c];
			r = &points[pn];
			p = &shape->pointList[pn];
			v = &verts[c];
			
			if(!orthographic) {
				iw = 1.0 / r->z;
				v->x = r->x * iw * pixelConversion + (state->viewWidth/2);
				v->y = r->y * iw * pixelConversion + (state->viewHeight/2);
			}
			else {
				v->x = r->x / orthographicScale * pixelConversion + (state->viewWidth/2);
				v->y = r->y / orthographicScale * pixelConversion + (state->viewHeight/2);
			}
					
			if((v->x > 0.0) && (v->x < state->viewWidth) && (v->y > 0.0) && (v->y < state->viewHeight)) {
				p->u = xScale * v->x / state->viewWidth + xOffset;
				if(noWrapping) {
					if(p->u < 0.0) p->u = 0.0;
					if(p->u > 1.0) p->u = 1.0;
				}
				p->v = 1.0 - (yScale * v->y / state->viewHeight + yOffset);
				if(noWrapping) {
					if(p->v < 0.0) p->v = 0.0;
					if(p->v > 1.0) p->v = 1.0;
				}
				p->c = 0.0;
			}
		}
	}

	DisposePtr((Ptr) points);
	
	shape->texture = texID;
}

long Shape_GetClickedPoints(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[], Point whereMouse, long* pointList)
{
	long				x,
						c,
						i,
						numPoints = 0;
	Vector				v1,
						v2;
	Vector				n;
	TQAVTexture			verts[3];
	TQAVTexture*		v;
	VectorPtr			r;
	VertexPtr			p;
	Matrix				r1;
	VectorPtr			points;
	TriFacePtr			tri;
	unsigned long		pn;
	float				pixelConversion = (state->d / state->h) * (-state->viewWidth / 2),
						iw;
	Point				thePoint;
	Rect				theRect;
	Boolean			found;
	
	theRect.left = whereMouse.h - kPrecision;
	theRect.right = whereMouse.h + kPrecision;
	theRect.top = whereMouse.v - kPrecision;
	theRect.bottom = whereMouse.v + kPrecision;
	
	Matrix_Negate(camera, &r1);
	Matrix_Cat(globalPos, &r1, &r1);
	Shape_LinkMatrix(shape, &r1);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &r1, &r1);
	
	points = (Vector*) NewPtr(sizeof(Vector) * shape->pointCount);
	for(x = 0; x < shape->pointCount; x++)
	Matrix_TransformVector(&r1, &shape->pointList[x].point, &points[x]);
	
	for(x = 0; x < shape->triangleCount; x++){
		tri = &shape->triangleList[x];
		
		Vector_Subtract(&points[tri->corner[1]], &points[tri->corner[0]], &v1);
		Vector_Subtract(&points[tri->corner[2]], &points[tri->corner[0]], &v2);
		Vector_CrossProduct(&v2, &v1, &n);
		if(Vector_DotProduct(&points[tri->corner[0]], &n) > 0)
		continue;
		
		for(c = 0; c < 3; c++){
			pn = tri->corner[c];
			r = &points[pn];
			p = &shape->pointList[pn];
			v = &verts[c];
			
			if(!orthographic) {
				iw = 1.0 / r->z;
				v->x = r->x * iw * pixelConversion + (state->viewWidth/2);
				v->y = r->y * iw * pixelConversion + (state->viewHeight/2);
			}
			else {
				v->x = r->x / orthographicScale * pixelConversion + (state->viewWidth/2);
				v->y = r->y / orthographicScale * pixelConversion + (state->viewHeight/2);
			}
						
			if((v->x > 0.0) && (v->x < state->viewWidth) && (v->y > 0.0) && (v->y < state->viewHeight)) {
				thePoint.h = v->x;
				thePoint.v = v->y;
				if(PtInRect(thePoint, &theRect)) {
					found = false;
					for(i = 0; i < numPoints; ++i)
					if(pn == pointList[i])
					found = true;
					
					if(!found) {
						pointList[numPoints] = pn;
						++numPoints;
					}
				}
			}
		}
	}
	
	DisposePtr((Ptr) points);
	
	return numPoints;
}

void Shape_DrawSelectedPoint(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[], long pointNum)
{
	Matrix				r1;
	Vector				point;
	float				pixelConversion = (state->d / state->h) * (-state->viewWidth / 2),
						iw;
	Point				thePoint;
	Rect				theRect;
	GrafPtr				savePort;
	
	GetPort(&savePort);
	SetPort(mainWin);
	
	Matrix_Negate(camera, &r1);
	Matrix_Cat(globalPos, &r1, &r1);
	Shape_LinkMatrix(shape, &r1);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &r1, &r1);
	
	Matrix_TransformVector(&r1, &shape->pointList[pointNum].point, &point);
	
	if(!orthographic) {
		iw = 1.0 / point.z;
		thePoint.h = point.x * iw * pixelConversion + (state->viewWidth/2);
		thePoint.v = point.y * iw * pixelConversion + (state->viewHeight/2);
	}
	else {
		thePoint.h = point.x / orthographicScale * pixelConversion + (state->viewWidth/2);
		thePoint.v = point.y / orthographicScale * pixelConversion + (state->viewHeight/2);
	}
				
	theRect.left = thePoint.h - kPrecision;
	theRect.right = thePoint.h + kPrecision;
	theRect.top = thePoint.v - kPrecision;
	theRect.bottom = thePoint.v + kPrecision;
		
	ForeColor(yellowColor);
	PaintRect(&theRect);
	
	SetPort(savePort);
}

void Shape_Merge(ShapePtr shape1, ShapePtr shape2)
{
	VertexPtr		newPointList;
	TriFacePtr		newTriangleList;
	long			offsetPoints = shape1->pointCount,
					offsetTriFaces = shape1->triangleCount,
					i;
	
	//Set cursor
	SetCursor(*waitCursor);
	
	//Merge vertices
	newPointList = (VertexPtr) NewPtrClear(sizeof(Vertex) * (shape1->pointCount + shape2->pointCount));
	BlockMove(&shape1->pointList[0], &newPointList[0], sizeof(Vertex) * shape1->pointCount);
	BlockMove(&shape2->pointList[0], &newPointList[shape1->pointCount], sizeof(Vertex) * shape2->pointCount);
	DisposePtr((Ptr) shape1->pointList);
	shape1->pointCount += shape2->pointCount;
	shape1->pointList = newPointList;
	
	//Merge triangles
	newTriangleList = (TriFacePtr) NewPtrClear(sizeof(TriFace) * (shape1->triangleCount + shape2->triangleCount));
	BlockMove(&shape1->triangleList[0], &newTriangleList[0], sizeof(TriFace) * shape1->triangleCount);
	BlockMove(&shape2->triangleList[0], &newTriangleList[shape1->triangleCount], sizeof(TriFace) * shape2->triangleCount);
	DisposePtr((Ptr) shape1->triangleList);
	shape1->triangleCount += shape2->triangleCount;
	shape1->triangleList = newTriangleList;
	for(i = 0; i < shape2->triangleCount; ++i) {
		newTriangleList[offsetTriFaces + i].corner[0] += offsetPoints;
		newTriangleList[offsetTriFaces + i].corner[1] += offsetPoints;
		newTriangleList[offsetTriFaces + i].corner[2] += offsetPoints;
	}
	
	//Merge normals
	if(shape1->normalMode != kNoNormals)
	Shape_CalculateNormals(shape1, 2/*shape1->normalMode*/, 0.0);
	
	Shape_CalculateBoundingBox(shape1);
	
	//Reset cursor
	InitCursor();
}

ShapePtr Shape_Copy(ShapePtr shape)
{
	ShapePtr		copy;
	
	copy = Shape_New();
	BlockMove(shape, copy, sizeof(Shape));
	BlockMove(" - copy", &copy->name[copy->name[0] + 1], 7);
	copy->name[0] += 7;
	
	copy->pointList = (VertexPtr) NewPtr(GetPtrSize((Ptr) shape->pointList));
	BlockMove(shape->pointList, copy->pointList, GetPtrSize((Ptr) shape->pointList));
	
	copy->triangleList = (TriFacePtr) NewPtr(GetPtrSize((Ptr) shape->triangleList));
	BlockMove(shape->triangleList, copy->triangleList, GetPtrSize((Ptr) shape->triangleList));
	
	copy->normalList = (VectorPtr) NewPtr(GetPtrSize((Ptr) shape->normalList));
	BlockMove(shape->normalList, copy->normalList, GetPtrSize((Ptr) shape->normalList));
	
	return copy;
}

ShapePtr Shape_GetFromID(OSType ID)
{
	long				i;
	
	for(i = 0; i < theObject.shapeCount; ++i)
	if(theObject.shapeList[i]->id == ID)
	return theObject.shapeList[i];
	
	return nil;
}

void Shape_LinkMatrix(ShapePtr shape, MatrixPtr m)
{
	if(shape->link == kNoLink)
	return;
	
#if 0
	if(shapeList[shape->link]->link != kNoLink) {
		if(shapeList[shapeList[shape->link]->link]->link != kNoLink)
		Matrix_Cat(&shapeList[shapeList[shapeList[shape->link]->link]->link]->pos, m, m);
		Matrix_Cat(&shapeList[shapeList[shape->link]->link]->pos, m, m);
	}
	Matrix_Cat(&shapeList[shape->link]->pos, m, m);
#else
	ShapePtr tempShape_0, tempShape_1, tempShape_2;
	
	tempShape_0 = Shape_GetFromID(shape->link);
	if(tempShape_0->link != kNoLink) {
		tempShape_1 = Shape_GetFromID(tempShape_0->link);
		if(tempShape_1->link != kNoLink) {
			tempShape_2 = Shape_GetFromID(tempShape_1->link);
			Matrix_Cat(&tempShape_2->pos, m, m);
		}
		Matrix_Cat(&tempShape_1->pos, m, m);
	}
	Matrix_Cat(&tempShape_0->pos, m, m);
#endif
}
	
