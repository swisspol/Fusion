#include						<Traps.h>

//VARIABLES:

static RgnHandle			mBarRgn;
static unsigned short	mBarHeight;

//ROUTINES:

static Boolean HasControlStrip()
{
	short			err;
	long			response;
	
	err = Gestalt(gestaltControlStripAttr, &response);
	if(err)
	return false;
	
	return (response & (1 << gestaltControlStripExists));
}

static UniversalProcPtr gControlStripTrapUPP = kUnresolvedCFragSymbolAddress;
static UniversalProcPtr gUnimplementedUPP = kUnresolvedCFragSymbolAddress;

#define _ControlStripDispatch 0xAAF2

pascal void SBShowHideControlStrip(Boolean showIt)
// THREEWORDINLINE(0x303C, 0x0101, 0xAAF2);
//	MOVE.W	#$0101,D0
//	_ControlStripDispatch
{
	if ((Ptr) gUnimplementedUPP == (Ptr) kUnresolvedCFragSymbolAddress)
		gUnimplementedUPP = GetToolboxTrapAddress(_Unimplemented);

	if ((Ptr) gControlStripTrapUPP == (Ptr) kUnresolvedCFragSymbolAddress)
		gControlStripTrapUPP = GetToolboxTrapAddress(_ControlStripDispatch);

	if ((Ptr) gControlStripTrapUPP != (Ptr) gUnimplementedUPP)
	{
		CallUniversalProc(gControlStripTrapUPP,
			kD0DispatchedPascalStackBased |
			DISPATCHED_STACK_ROUTINE_SELECTOR_SIZE(kTwoByteCode) |
			DISPATCHED_STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(Boolean))),
			0x0101,		// selector
			showIt);	// paramter(s)
	}
}
static void SH_ForceUpdate(RgnHandle rgn)
{
	WindowRef		wpFirst = LMGetWindowList();
	
	PaintBehind(wpFirst, rgn);
	CalcVisBehind(wpFirst, rgn);
}

static void GetMBarRgn(RgnHandle mBarRgn)
{
	Rect			mBarRect;

	mBarRect = qd.screenBits.bounds;
	mBarRect.bottom = mBarRect.top + GetMBarHeight();
	RectRgn(mBarRgn, &mBarRect);
}

void Hide_MenuBar()
{
	RgnHandle		GrayRgn = LMGetGrayRgn();
	EventRecord		theEvent;
	long			ticks;
	
	mBarHeight = GetMBarHeight();
	mBarRgn = NewRgn();
	GetMBarRgn(mBarRgn);
	LMSetMBarHeight(0);
	UnionRgn(GrayRgn,mBarRgn,GrayRgn);
	SH_ForceUpdate(mBarRgn);
	
	if(HasControlStrip())
	SBShowHideControlStrip(false);
}

void Show_MenuBar()
{
	RgnHandle		GrayRgn = LMGetGrayRgn();
	EventRecord		theEvent;
	long			ticks;
	
	LMSetMBarHeight(mBarHeight);
	DiffRgn(GrayRgn, mBarRgn, GrayRgn);
	DisposeRgn(mBarRgn);
	DrawMenuBar();
	
	if(HasControlStrip())
	SBShowHideControlStrip(true);
}