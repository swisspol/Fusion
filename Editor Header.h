//CONSTANTES:

#define				kMaxScripts						20
#define				kMaxTime						10
#define				kEditorTimeUnit					(kTimeUnit / 10)

#define				kWitdh							700
#define				kNameSize						200
#define				kLineHeight						18
#define				kNbLines						6
#define				kSpaceLineHeight				1

#define				kPref_Version					0x0102
#define				kPref_FlagFiltering				(1 << 0)
#define				kPref_FlagReduce				(1 << 1)
#define				kPref_FlagCompress				(1 << 2)

enum				{appleID = 1000, fileID, editID, renderID, viewID, windowID};
enum				{appleM = 0, fileM, renderM, viewM, windowM, editM};

//MACROS:

#define GetWindowPixMapPtr(w) *(((CGrafPtr)(w))->portPixMap)
#define	GWBitMapPtr(w) &(((GrafPtr)(w))->portBits)

#define UShort2Float(v) ((float) v / 65535.0)
#define Float2UShort(v) ((unsigned short) (v * 65535.0))

//STRUCTURES:

typedef struct Preferences_Definition
{
	unsigned long	version;
	unsigned long	flags;
	float			ambient;
	float			local;
	long			raveEngineID;
	Point			windowPosition[10];
	float			backColorRed,
					backColorGreen,
					backColorBlue;
};
typedef Preferences_Definition Preferences;
typedef Preferences_Definition* PreferencesPtr;

extern "C" {
	
//VARIABLES:

WindowPtr			mainWin,
					mappingWin,
					whichWin;
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
Boolean			run,
					isForeGround,
					filled,
					normals,
					mappingVisible,
					shapeVisible,
					textureVisible,
					scriptVisible,
					sequencerVisible,
					selectedOnly,
					fullView,
					fullViewAvailable;
short				special;
char				theChar;
short				whereClick,
					theKey;
Rect				dragRect;
Str63				theString;
GWorldPtr			oldGWorld;
GDHandle			oldGDHandle;
ListHandle			textureListHandle,
					shapeListHandle,
					scriptListHandle;
Cell				theCell,
					shapeCell,
					textureCell,
					scriptCell,
					oldShapeCell;
short				animationCell,
					firstAnimation;
CursHandle			keyCursor,
					waitCursor;	
RGBColor			veryDarkGrey,
					darkGrey,
					lightGrey;
long				lastClick,
					currentTime,
					firstTime;				
Preferences			thePrefs;

short				mode,
					curView,
					curTexWidth,
					curTexHeight;
Object				theObject;
Boolean			object,
					zoom,
					orthographic;
float				orthographicScale;

CameraState			localCamera,
					topCamera,
					frontCamera,
					rightCamera;
CameraState*		currentCamera;
StatePtr			localState,
					topState,
					frontState,
					rightState,
					globalState;
Rect				popUpRect,
					popUpRect2,
					viewRect,
					viewRect2,
					nameRect,
					timeRect;
					
long				textureCount;
TQATexture*			textureList[kMaxShapes];
TextureStoragePtr	textureStorageList[kMaxShapes];
OSType				nameList[kMaxShapes];

long				scriptCount;
ScriptPtr			scriptList[kMaxScripts],
					currentScript;

unsigned long		clockTime;
UnsignedWide		lastFrameTime;
PicHandle			previewPic;

float				objectRotateX,
					objectRotateY,
					objectRotateZ;
long				selectedPoints,
					selectedPoint[50];
		
//ROUTINES:

//File main.cp
PicHandle Take_VRAMCapture(GDHandle device, Rect* copyRect, Rect* finalRect);
void Do_Error(OSErr theError, short explanationTextID);
void Check_ShapeMapping();

//File Init.cp
void InitToolbox();
void Init_Menus();
void Init_MainWindow();
void Init_TextureWindow();
void Init_ShapeWindow();
void Init_ScriptWindow();
void Init_SequencerWindow();
void Init_MappingWindow();
OSErr Init_Engine();

//File: Object.cp
OSErr Object_New();
OSErr Object_Open(Boolean merge, Boolean applyObjectMatrix);
OSErr Object_Load(FSSpec* theFile);
OSErr Object_Merge(FSSpec* theFile, Boolean applyObjectMatrix);
OSErr Object_Close();
OSErr Object_Save();
void Object_UpdateMatrix(ObjectPtr object);
void Object_ApplyMatrix(ObjectPtr object);
void Object_CalculateBoundingBox(ObjectPtr object);

//File: Shape.cp
ShapePtr Shape_New();
void Shape_Dispose(ShapePtr shape);
OSErr Shape_Load3DMF(FSSpec* theFile, Boolean attributesBefore);
void Shape_SwitchBF(ShapePtr shape);
void Shape_ClearNormals(ShapePtr shape);
void Shape_CalculateNormals(ShapePtr shape, short mode, float smoothAngle);
void Shape_CalculateBoundingBox(ShapePtr shape);
void Shape_Center(ShapePtr shape);
void Shape_ReCenterOrigin(ShapePtr shape);
void Shape_Scale(ShapePtr shape, float factor);
void Shape_ScaleBox(ShapePtr shape, float size);
void Shape_UpdateMatrix(ShapePtr shape);
void Shape_ApplyMatrix(ShapePtr shape);
void Shape_MoveOrigin(ShapePtr shape, float moveX, float moveY, float moveZ);
void Shape_ApplyColor(ShapePtr shape, float red, float green, float blue);
void Shape_ApplyTexture(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[], OSType texID, float xScale, float xOffset, float yScale, float yOffset, Boolean noWrapping);
long Shape_GetClickedPoints(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[], Point whereMouse, long* pointList);
void Shape_DrawSelectedPoint(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[]);
void Shape_Merge(ShapePtr shape1, ShapePtr shape2);
ShapePtr Shape_Copy(ShapePtr shape);
ShapePtr Shape_GetFromID(OSType ID);
void Shape_LinkMatrix(ShapePtr shape, MatrixPtr m);

//File: Rendering.cp
void Render(StatePtr state, MatrixPtr camera);

//File: Texture.cp
TQATexture* Texture_NewFromStorageTexture(TQAEngine* engine, TextureStoragePtr sourceTexture);
TextureStoragePtr Texture_NewStorageFromPictWithAlpha(PicHandle picture, OSType name);
TextureStoragePtr Texture_NewStorageFromPict(PicHandle picture, OSType name);
OSErr Texture_NewFromFile(TQAEngine* engine, FSSpec* theFile, Boolean useAlpha);
void Texture_Draw(TextureStoragePtr texturePtr, PixMapPtr destPixMap, Point destPoint);

//File: Cinematic.cp
ScriptPtr Script_New();
void Script_Dispose(ScriptPtr script);
void Script_NewAnimation(ScriptPtr script, OSType shapeID);
void Script_DisposeAnimation(ScriptPtr script, long animationID);
void Script_AnimationAddEvent(ScriptPtr script, long animationID, long time);
void Script_AnimationDeleteEvent(ScriptPtr script, long animationID, long eventID);
void Script_AnimationSetEvent(ScriptPtr script, long animationID, long eventID);
void Script_AnimationAddStartStatusEvents(ScriptPtr script, long time);
void Script_UpdateLength(ScriptPtr script);
void Script_Run(ScriptPtr script);
void Script_Stop(ScriptPtr script);
void Script_Reset(ScriptPtr script);
void Script_Running(ScriptPtr script);

//File: Clock.cp
OSErr Clock_Install();
void Clock_Dispose();

//File: Preferences.cp
void Preference_Read();
void Preference_Write();

//File: Settings.cp
void SetDialogItemFloat(Handle itemHandle, float num);
void GetDialogItemFloat(Handle itemHandle, float* num);
void Object_Setting(ObjectPtr object);
void Shape_Setting(ShapePtr shape, ShapePtr shapeList[]);
void Script_Setting(ScriptPtr script);
void Event_Setting(EventPtr event);
void Texture_Setting(short textureNum);
void Preferences_Setting(PreferencesPtr prefs);

//File: Windows.cp
void Do_MainWin(Point whereMouse);
void Do_TextureWin(Point whereMouse);
void Do_MappingWin(Point whereMouse);
void Do_ShapeWin(Point whereMouse);
void Do_ScriptWin(Point whereMouse);
void Do_SequencerWin(Point whereMouse);
void Update_MainWin();
void Update_TextureWin();
void Update_ShapeWin();
void Update_ScriptWin();
void Update_MappingWin();
void Update_SequencerWin();

}