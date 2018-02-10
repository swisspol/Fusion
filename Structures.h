#include				<RAVE.h>

//CONSTANTES:

#define					kNoTexture				-1
#define					kDefaultTextureFilter	-1
#define					kNoNormals				-1
#define					kNoLink					-1
#define					kNoID					'None'

#define					kBBSize					8
#define					k3DFileCreator			'Fusn'
#define					k3DFileType				'3DOb'
#define					kVariationFileType		'VrOb'

#define					kPi						3.141592653589793

#define					kMaxShapes				200

#define					kMaxAnimations			20
#define					kTimeUnit				100

#define					kFlag_RelativePos		(1 << 0)
#define					kFlag_MayHide			(1 << 1)

#define					kFlag_Collision			(1 << 0)

#define					kFlag_HasAlpha			(1 << 0)
#define					kFlag_MipMap			(1 << 1)

#define					kFlag_Loop				(1 << 0)
#define					kFlag_ResetOnLoop		(1 << 1)
#define					kFlag_SmoothStart		(1 << 2)
#define					kFlag_Running			(1 << 31)

#define					kFile_Version			0x0105
#define					kFile_Info				'info'
#define					kFile_Object			'objc'
#define					kFile_Shape				'shap'
#define					kFile_Points			'pnts'
#define					kFile_Triangles			'trig'
#define					kFile_NormalTable		'nrtb'
#define					kFile_NormalList		'nrls'
#define					kFile_Texture			'txtr'
#define					kFile_CinematicScript	'cnsc'
#define					kFile_CinematicAnimation 'cnan'
#define					kFile_Preview			'prvw'

#define					kFlag_TextureElements	(1 << 0)
#define					kFlag_CinematicElements	(1 << 1)
#define					kFlag_PreviewElement	(1 << 2)
#define					kFlag_Protected			(1 << 31)

#define					kPreviewH				128
#define					kPreviewV				85
#define					kPreviewResID			128

#define					kMaxPatchs				64
#define					kVariationFileVersion	0x0100
#define					kFile_Variation			'vari'

//MACROS:

#define DegreesToRadians(x)	((float)((x) * kPi / 180.0))
#define RadiansToDegrees(x)	((float)((x) * 180.0 / kPi))

//STRUCTURES:

//Data definitions:

typedef struct {
	float			x,
					y,
					z;
} Vector;
typedef Vector* VectorPtr;

typedef struct {
	Vector			point;
	float			u;
	float			v;
	float			c;
} Vertex;
typedef Vertex* VertexPtr;

typedef struct {
	unsigned long	corner[3];
} TriFace;
typedef TriFace* TriFacePtr;

typedef struct {
	Vector			x;
	Vector			y;
	Vector			z;
	Vector			w;
} Matrix;
typedef Matrix* MatrixPtr;

//3D definitions:

typedef struct {
	long			pointCount;
	VertexPtr		pointList;
	long			triangleCount;
	TriFacePtr		triangleList;
	
	short			normalMode; // 0 for pervertex or 1 for flat shading
	long			unused; //normals are required only if object shading is on
	long			normalCount;
	VectorPtr		normalList;
	
	unsigned long	flags;
	Matrix			pos;
	float			rotateX;
	float			rotateY;
	float			rotateZ;
	float			scale;
	long			link;
	
	Vector			boundingBox[kBBSize];
	
	OSType			texture; //shape texture
	float			alpha; //shape transparency
	float			glow; //glow value (0.0 = none) not used if shading is on
	float			difuse;
	float			specular;
	
	unsigned int	backfaceCulling : 1; //remove backfacing triangles
	unsigned int	shading; //shape shading
	short			textureFilter; //-1 = default else standard contants
	
	Str63			name;
	OSType			id;
} Shape;
typedef Shape* ShapePtr;
typedef Shape** ShapeHandle;

typedef struct {
	unsigned long	shapeCount;
	ShapePtr		shapeList[kMaxShapes];
	
	unsigned long	flags;
	Matrix			pos;
	float			rotateX;
	float			rotateY;
	float			rotateZ;
	float			scale;
	
	Vector			boundingBox[kBBSize];
	
	Str63			name;
	OSType			id;
} Object;
typedef Object* ObjectPtr;
typedef Object** ObjectHandle;

typedef struct {
	float			d;	// the distance to the near clipping plane
	float			f;	// the distance to the far clipping plane
	float			h;	// half of the view width at distance d from camera
	
	unsigned int	doubleBuffer : 1;
	unsigned int	noZBuffer : 1;
	unsigned int	alwaysVisible: 1;
	short			antialiasing;
	unsigned int	perspectiveZ : 1;
	short			textureFilter;
	
	Vector			lightVector;
	float			ambient;

	TQAEngine*		engine;
	TQADrawContext*	drawContext;
	
	Boolean		textured;
	unsigned long	verticeCount;
	Ptr				verticeList;
	
	Rect			viewRect;
	float			viewHeight;
	float			viewWidth;
	
	WindowPtr		window;
	long			engineVendorID;
} State;
typedef State* StatePtr;
typedef State** StateHandle;

typedef struct {
	float			roll;
	float			pitch;
	float			yaw;
	Matrix			camera;
} CameraState;
typedef CameraState* CameraStatePtr;
typedef CameraState** CameraStateHandle;

//Texture definitions:

typedef struct {
	OSType			name;
	unsigned long	flags;
	long			width,
					height,
					rowBytes;
	unsigned short data[];
} TextureStorage;
typedef TextureStorage* TextureStoragePtr;
typedef TextureStorage** TextureStorageHandle;

//Cinematic definitions:

typedef struct {
	long			time;
	
	float			rotateX;
	float			rotateY;
	float			rotateZ;
	Vector			position;
} Event;
typedef Event* EventPtr;
typedef Event** EventHandle;

typedef struct {
	OSType			shapeID;
	unsigned long	flags;
	
	float			rotateX_Init;
	float			rotateY_Init;
	float			rotateZ_Init;
	Vector			position_Init;
	
	Boolean		running;
	unsigned long	currentEvent;
	long			startTime,
					endTime;
	float			rotateX;
	float			rotateY;
	float			rotateZ;
	Vector			position;
	float			rotateX_d;
	float			rotateY_d;
	float			rotateZ_d;
	Vector			position_d;
	
	unsigned long	eventCount;
	Event			eventList[];
} CinematicAnimation;
typedef CinematicAnimation Animation;
typedef CinematicAnimation* AnimationPtr;
typedef CinematicAnimation** AnimationHandle;

typedef struct {
	unsigned long	animationCount;
	AnimationPtr	animationList[kMaxAnimations];
	unsigned long	flags;
	long			length;
	
	long			startTime;
	
	Str63			name;
	OSType			id;
} CinematicScript;
typedef CinematicScript Script;
typedef CinematicScript* ScriptPtr;
typedef CinematicScript** ScriptHandle;

//Variation definitions:

typedef struct {
	OSType			previousTexture;
	OSType			newTexture;
} Patch_Definition;
typedef Patch_Definition Patch;
typedef Patch_Definition* PatchPtr;

typedef struct {
	OSType				objectID;
	unsigned long	patchCount;
	Patch_Definition	patchList[kMaxPatchs];
} Variation_Definition;
typedef Variation_Definition Variation;
typedef Variation_Definition* VariationPtr;
typedef Variation_Definition** VariationHandle;

//File definitions:

typedef struct {
	OSType			type;
	long			size;
} Header;
typedef Header* HeaderPtr;
	
typedef struct {
	short			version;
	unsigned long	elementCount[15];
	unsigned long	scriptCount;
	unsigned long	textureCount;
	unsigned long	flags;
} ObjectFileHeader;

typedef struct {
	short			version;
	unsigned char	space[256];
	unsigned long	textureCount;
	unsigned long	flags;
} VariationFileHeader;

