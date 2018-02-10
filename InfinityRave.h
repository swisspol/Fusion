#include			<Rave.h>
#include			<Rave_System.h>

#ifndef INFINITY_RAVE_H
#define INFINITY_RAVE_H

#define kInfinityVendorID 'Pol '

typedef enum {
	kInfinityQuickDraw				= 1,
	kInfinityFasterDraw				= 2
} TQInfinityDrawingEngine;

typedef enum {
	kInfinityFullBlit				= 1,
	kInfinityVGABlit				= 2,
	kInfinityInterlacedBlit			= 3
} TQInfinityBlitMode;

typedef enum
{
	kInfinityDrawingEngine			= kQATag_EngineSpecific_Minimum,
	kInfinityBlitMode				= kQATag_EngineSpecific_Minimum + 1
} TQInfinityTag;

#endif