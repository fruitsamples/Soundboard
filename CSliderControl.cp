/*** File:        CSliderControl.cp** Written by:  Tim Nufire**** Copyright � 1990-1995 Apple Computer, Inc.** All rights reserved. *//*****************************************************************************//* This is a custom slider ported by me from Kibitz to PowerPlant.			 *//*****************************************************************************/enum {	kCapHeight = 11,	kThumbHeight = 9,	kThumbOffset = 11,	kSliderWidth = 20};#include "CSliderControl.h"#ifndef __RESOURCES__#include <Resources.h>#endif#ifndef __TOOLUTILS__#include <ToolUtils.h>#endif#ifndef __MEMORY__#include <Memory.h>#endif#ifndef __OSUTILS__#include <OSUtils.h>#endif#ifndef __CONTROLS__#include <Controls.h>#endif#ifndef __GWLAYERS__#include "GWLayers.h"#endif/*****************************************************************************/#pragma options align=mac68ktypedef struct cdefRsrcJMP {	long	jsrInst;	long	moveInst;	short	jmpInst;	long	jmpAddress;} cdefRsrcJMP;typedef cdefRsrcJMP *cdefRsrcJMPPtr, **cdefRsrcJMPHndl;typedef struct thumbCntlParams {	Rect	limitRect;	Rect	slopRect;	short	axis;} thumbCntlParams;#pragma options align=reset/*****************************************************************************//* SetSliderJmpAddress() 													 *//*																			 *//* This routine patches a dummy slider CDEF so that the slider code can 	 *//* reside inside our object. This be called once during application startup  *//* BEFORE any slider objects are created.									 */	/*****************************************************************************/void CSliderControl::SetSliderJmpAddress(){	cdefRsrcJMPHndl	cdefRsrc;	static	ControlDefUPP	sliderCtlUPP = nil;	cdefRsrc = (cdefRsrcJMPHndl)GetResource('CDEF', (sliderCntlProc / 16));	if (!sliderCtlUPP)		sliderCtlUPP = NewControlDefProc(CSliderControl::SliderCtl);	(*cdefRsrc)->jmpAddress = (long)sliderCtlUPP;		/* Make sure that instruction caches don't kill us. */	#ifndef powerc		FlushInstructionCache(); 	#endif}/******************************************************************************* SliderCtl(short varCode, ControlHandle ctl, short msg, long parm)		 **																			 ** This is a custom slider originally from Kibitz. It expects contrlRfCon to** contain a pointer to an CSliderControl object and will call that objects** member functions UpdateSlider() and TrackSlider() to handle drawing and ** tracking respectively.**** There is a custom cdef for this code.  All it does is jump to this code.** In addition to being a nice way to objectize the slider, there is another** really good reason for this which is that it is possible for the cdef to ** be re-entered! When the user clicks on a slider, the control manager locks ** down the cdef, and then calls it. When the custom cdef returns to the control ** manager, the cdef is unlocked. This all seems very reasonable. However, if ** the code that tracks the slider updates another slider you have a problem.** This is because the control manager handles the update by calling the cdef.  ** Of course, it locks it down, and then when the control manager is returned to, ** it unlocks it.  BUT WAIT!!  We are still tracking the slider which caused the ** update in the first place.  This is true.  It is also true that the cdef is now ** UNLOCKED!  It isn't a good idea to rts to code that has moved.  This is why ** the cdef jumps to the code in the application.  We never return to the ** (potentially unlocked) cdef.  We  return straight to the control manager.  ** Ugly problem, huh?*****************************************************************************/pascal long CSliderControl::SliderCtl(short /*varCode*/, ControlHandle ctl, short msg, long parm){	CSliderControl	*thisSlider;	Rect			viewRect;	thumbCntlParams	*tcp;		thisSlider = (CSliderControl *)(*ctl)->contrlRfCon;	viewRect = (*ctl)->contrlRect;	if (thisSlider) switch (msg) {		case initCntl:		case dispCntl:		case posCntl:			break;		case drawCntl:			thisSlider->UpdateSlider();			break;		case testCntl:			if ((*ctl)->contrlHilite != 255			 && (*ctl)->contrlMax			 && PtInRect(*(Point *)&parm, &viewRect)) {				thisSlider->mLastClick = *((Point *)&parm);								/* Everything is the thumb.  The "thumb" routine will figure				** out what part it is.  Since this is a very specific control,				** we can get away with this simplification. */				return(kControlIndicatorPart);			}			break;		case calcCRgns:			parm &= 0x00FFFFFF; /* 24-bit memory manager is in use */			/* fall through to calcCntlRgn */		case calcCntlRgn:		case calcThumbRgn:			RectRgn((RgnHandle)parm, &viewRect);			break;		case thumbCntl:			tcp = (thumbCntlParams *)parm;			tcp->limitRect = viewRect;			InsetRect(&viewRect, -64, -64);			tcp->slopRect = viewRect;			tcp->axis = 2;			break;		case dragCntl:			thisSlider->TrackSlider(thisSlider->mLastClick);			return(true);		case autoTrack:			break;		default:			break;	}	return(0);}// ---------------------------------------------------------------------------//		� CreateFromCNTL [static]// ---------------------------------------------------------------------------//	Create a StdControl or SliderControl from a CNTL resource/*LStdControl*LStdControl::CreateFromCNTL(	ResIDT		inCNTLid,	MessageT	inValueMessage,	ResIDT		inTextTraitsID,	LView		*inSuperView){	LStdControl	*theStdControl = nil;	::HidePen();	inSuperView->FocusDraw();		if (inTextTraitsID != 0) {		// Control does not use System font		UTextTraits::SetPortTextTraits(inTextTraitsID);	}		ControlHandle	macControlH = ::GetNewControl(inCNTLid,												inSuperView->GetMacPort());	::ShowPen();	ThrowIfNil_(macControlH);		SCNTLResource	*resP = *(SCNTLResourceH) ::GetResource('CNTL', inCNTLid);		SPaneInfo	thePaneInfo;	thePaneInfo.paneID = inCNTLid;	thePaneInfo.left = resP->bounds.left;	thePaneInfo.top = resP->bounds.top;	thePaneInfo.width = resP->bounds.right - thePaneInfo.left;	thePaneInfo.height = resP->bounds.bottom - thePaneInfo.top;	thePaneInfo.visible = (resP->visible != 0);	thePaneInfo.enabled = true;	thePaneInfo.bindings.left =		thePaneInfo.bindings.top =		thePaneInfo.bindings.right =		thePaneInfo.bindings.bottom = false;	thePaneInfo.userCon = 0;	thePaneInfo.superView = inSuperView;										// Mask off useWFont variation code	Int16	controlKind =			(resP->procID & ~((Uint16) kControlUsesOwningWindowsFontVariant));	switch (controlKind) {			case pushButProc:			theStdControl = new LStdButton(thePaneInfo, inValueMessage,									inTextTraitsID, macControlH);			break;					case checkBoxProc:			theStdControl = new LStdCheckBox(thePaneInfo, inValueMessage,									resP->value, inTextTraitsID, macControlH);			break;					case radioButProc:			theStdControl = new LStdRadioButton(thePaneInfo, inValueMessage,									resP->value, inTextTraitsID, macControlH);			break;					case popupMenuProc:			theStdControl = new LStdPopupMenu(thePaneInfo, inValueMessage,									::GetControlMaximum(macControlH),									inTextTraitsID, macControlH);			break;					case sliderCntlProc:			theStdControl = new CSliderControl(thePaneInfo, inValueMessage,									::GetControlValue(macControlH),									::GetControlMinimum(macControlH),									::GetControlMaximum(macControlH),									controlKind, inTextTraitsID,									macControlH);			break;					default:			theStdControl = new LStdControl(thePaneInfo, inValueMessage,									::GetControlValue(macControlH),									::GetControlMinimum(macControlH),									::GetControlMaximum(macControlH),									controlKind, inTextTraitsID,									macControlH);			break;	}	return theStdControl;}*/// ---------------------------------------------------------------------------//		� CreateSliderControlStream [static]// ---------------------------------------------------------------------------//	Create a new SliderControl from the data in a Stream////	Current port must be the Window into which to install the controlCSliderControl*CSliderControl::CreateSliderControlStream(	LStream	*inStream){	return (new CSliderControl(inStream));}// ---------------------------------------------------------------------------//		� InitSlider// ---------------------------------------------------------------------------//	Initialization routine.voidCSliderControl::InitSlider(void){	if (mMacControlH) {		SetSliderRefCon(GetControlReference(mMacControlH));		SetControlReference(mMacControlH, (long)this);	}}// ---------------------------------------------------------------------------//		� CSliderControl(const CSliderControl&)// ---------------------------------------------------------------------------//	Copy ConstructorCSliderControl::CSliderControl(	const CSliderControl &inOriginal)		: LStdControl(inOriginal){	InitSlider();}// ---------------------------------------------------------------------------//		� CSliderControl(Int16)// ---------------------------------------------------------------------------//	Construct a SliderControl for a particular kind of Toolbox Control////	NOTE: On entry, the current Port must be Window into which to//	install the Control.CSliderControl::CSliderControl( Int16 inControlKind) : LStdControl(inControlKind){	InitSlider();}// ---------------------------------------------------------------------------//		� CSliderControl// ---------------------------------------------------------------------------//	Construct SliderControl from input parametersCSliderControl::CSliderControl(	const SPaneInfo	&inPaneInfo,	MessageT		inValueMessage,	Int32			inValue,	Int32			inMinValue,	Int32			inMaxValue,	Int16			inControlKind,	ResIDT			inTextTraitsID,	Str255			inTitle,	Int32			inMacRefCon)		: LStdControl(inPaneInfo, inValueMessage, inValue,						inMinValue, inMaxValue, inControlKind, 						inTextTraitsID, inTitle, inMacRefCon){	InitSlider();}// ---------------------------------------------------------------------------//		� CSliderControl// ---------------------------------------------------------------------------//	Construct from input parameters and an existing ControlHandleCSliderControl::CSliderControl(	const SPaneInfo	&inPaneInfo,	MessageT		inValueMessage,	Int32			inValue,	Int32			inMinValue,	Int32			inMaxValue,	Int16			inControlKind,	ResIDT			inTextTraitsID,	ControlHandle	inMacControlH)		: LStdControl(inPaneInfo, inValueMessage, inValue,						inMinValue, inMaxValue, inControlKind,						inTextTraitsID, inMacControlH){	InitSlider();}// ---------------------------------------------------------------------------//		� CSliderControl(LStream*)// ---------------------------------------------------------------------------//	Construct SliderControl from a data streamCSliderControl::CSliderControl(	LStream	*inStream)		: LStdControl(inStream){	InitSlider();}// ---------------------------------------------------------------------------//		� ~CSliderControl// ---------------------------------------------------------------------------//	DestructorCSliderControl::~CSliderControl(){}/*****************************************************************************/void	CSliderControl::AdjustSlider(Int32 val, Int32 max){	GrafPtr	oldPort = UQDGlobals::GetCurrentPort();	SetPort((**mMacControlH).contrlOwner);	/* Change the slider value and show the result. */	SetMaxValue(max);	SetValue(val);	UpdateSlider();	SetPort(oldPort);}void	CSliderControl::UpdateSlider(short hiliteCap){	Rect		ctlRect, workRect, sliderRect;	Boolean		active;	short		i, j;	RgnHandle	origClipRgn, clipRgn, workRgn;	CIconHandle	icons[7];	/* We use color icons here for the various slider parts.  This is so that we	** can take advantage of the depth of monitors.  I use icons here so that I	** can do a single plot of an icon if the delta of the thumb is -12 to 12.	** (The thumb is in the center of an icon, and 12 pixels above and below the	** icon, I have slider bar.  Use RedEdit to check it out.)  This technique	** gives a very smooth appearance when the thumb slides.  There is no flash.	** For deltas greater than +-12, I redraw the slider without the thumb, and	** then draw the thumb in the new position.  Since the thumb is moving a lot	** anyway, this doesn't show up as a flicker.  There is no overlap in the	** old and new positions for a big delta. */	FocusDraw();	ctlRect = (*mMacControlH)->contrlRect;	for (i = 0; i < 7; i++) icons[i] = ReadCIcon(i + rSliderBase);	origClipRgn = NewRgn();	GetClip(origClipRgn);	clipRgn = NewRgn();	for (i = 0; i < 2; i++) {		/* Draw the arrow parts first. */		j = i;		if (hiliteCap == i) j += 5;		workRect = ctlRect;		if (!i)			workRect.bottom = workRect.top + kCapHeight;		else			workRect.top = workRect.bottom - kCapHeight;		RectRgn(clipRgn, &workRect);		SetClip(clipRgn);			/* Clip out the area outside the arrow part. */		SliderDrawCIcon(icons[j], workRect.left, workRect.top);			/* Draw the arrow part. */	}	ctlRect.top    += kCapHeight;	ctlRect.bottom -= kCapHeight;	RectRgn(clipRgn, &ctlRect);	SetClip(clipRgn);		/* Clip out everything except the slider bar area. */	active  = true; /* ((*mMacControlH)->contrlOwner == FrontWindow()); */	if ((*mMacControlH)->contrlHilite == 255) active = false;	if (!(*mMacControlH)->contrlMax) active = false;	if (active) {		/* If control active, draw the thumb. */		sliderRect = CalcSliderRect();		SliderDrawCIcon(icons[3],					   sliderRect.left, sliderRect.top - kThumbOffset);		workRgn = NewRgn();		RectRgn(workRgn, &sliderRect);		DiffRgn(clipRgn, workRgn, clipRgn);		SetClip(clipRgn);		DisposeRgn(workRgn);			/* Now that the thumb is drawn, protect it by clipping it out. */	}	for (i = ctlRect.top; i < ctlRect.bottom; i += 32)		SliderDrawCIcon(icons[2], ctlRect.left, i);			/* Draw the slider bar portion. */	/* It is now completely drawn.  Clean up and get out. */	SetClip(origClipRgn);	DisposeRgn(clipRgn);	DisposeRgn(origClipRgn);	for (i = 0; i < 7; i++) KillCIcon(icons[i]);}/*****************************************************************************/void	CSliderControl::SliderDrawCIcon(CIconHandle iconHndl, short hloc, short vloc){	Rect	iconRect;	iconRect.right  = (iconRect.left = hloc) + 32;	iconRect.bottom = (iconRect.top  = vloc) + 32;	DrawCIcon(iconHndl, iconRect);}/*****************************************************************************/Rect	CSliderControl::CalcSliderRect(){	Rect	ctlRect, sliderRect;	short	max, val;	long	calc;	ctlRect = (*mMacControlH)->contrlRect;	ctlRect.top    += kCapHeight;	ctlRect.bottom -= kCapHeight;	max = (*mMacControlH)->contrlMax;	val = (*mMacControlH)->contrlValue;	calc = ctlRect.bottom - ctlRect.top - kThumbHeight;	calc *= val;	if (max) calc /= max;	sliderRect.top    = ctlRect.top + calc,	sliderRect.left   = ctlRect.left;	sliderRect.bottom = sliderRect.top + kThumbHeight;	sliderRect.right  = ctlRect.right;	return(sliderRect);}/*****************************************************************************/short	CSliderControl::CalcSliderValue(Point mouseLoc){	Rect	ctlRect;	long	max, val;	ctlRect = (*mMacControlH)->contrlRect;	ctlRect.top    += kCapHeight;	ctlRect.bottom -= kCapHeight;	max = (*mMacControlH)->contrlMax;		val = (mouseLoc.v - ctlRect.top - kThumbHeight/2) * max;	val /= ctlRect.bottom - ctlRect.top - kThumbHeight;	if (val < 0) 	val = 0;	if (val > max)	val = max;		return(val);}/*****************************************************************************/void	CSliderControl::TrackAndRun(Point *mouseLoc){	EventRecord	macEvent;	// Give the movies some time (both in our app and outside)	::WaitNextEvent (0, &macEvent, 0, 0);		// Repeaters get time after every event	LPeriodical::DevoteTimeToRepeaters(macEvent);//	SystemTask();//	MoviesTask(0, 0);	FocusDraw();	*mouseLoc = macEvent.where;	GlobalToLocal(mouseLoc);}/*****************************************************************************/void	CSliderControl::TrackSlider(Point origMouseLoc){	CIconHandle	icons[7];	GrafPtr		oldPort = UQDGlobals::GetCurrentPort();	Rect		ctlRect, sliderRange, slopRect, sliderRect, capRect, pgRect;	RgnHandle	origClipRgn, clipRgn, workRgn;	short		i, max, val, ovloc, voffset, vloc, delta, hiliteCap;	Boolean		hiliteOn;	long		origTick, calc;	Point		lastMouseLoc, mouseLoc;	/* Get everything we need set up. */	FocusDraw();	origTick = TickCount();	for (i = 2; i < 5; i++) icons[i] = ReadCIcon(i + rSliderBase);	SetPort((**mMacControlH).contrlOwner);	ctlRect = (*mMacControlH)->contrlRect;	origClipRgn = NewRgn();	GetClip(origClipRgn);	clipRgn = NewRgn();	workRgn = NewRgn();	max = (*mMacControlH)->contrlMax;	pgRect = ctlRect;	pgRect.top    += kCapHeight;	pgRect.bottom -= kCapHeight - 1;	/* That ought to be enough setup. */	if (PtInRect(origMouseLoc, &pgRect)) {		/* If in the slide area... */		sliderRect = CalcSliderRect();				if (!PtInRect(origMouseLoc, &sliderRect)) {	/* If they are not on the thumb... */					/* Move the thumb to them... */			(*mMacControlH)->contrlValue = val = CalcSliderValue(origMouseLoc);			UpdateSlider();			SliderAction(val);						/* Update starting slider rect... */			sliderRect = CalcSliderRect();		}			RectRgn(clipRgn, &pgRect);		SetClip(clipRgn);		sliderRange = pgRect;					/* Calc area thumb can move. */		sliderRange.bottom -= kThumbHeight + 1;	/* Count height of thumb against range. */		slopRect = sliderRange;					/* Give the user some slop. */		InsetRect(&slopRect, -20, -20);		lastMouseLoc = origMouseLoc;		voffset = lastMouseLoc.v - sliderRect.top;		ovloc   = lastMouseLoc.v - voffset;			while (StillDown()) {			TrackAndRun(&mouseLoc);			SetClip(clipRgn);			if (!EqualPt(mouseLoc, lastMouseLoc)) {		/* The mouse has moved. */				if (!PtInRect(mouseLoc, &slopRect)) mouseLoc = lastMouseLoc;					/* Outside slopRect, so snap back to the last good position. */				vloc = mouseLoc.v - voffset;				if (vloc < sliderRange.top)    vloc = sliderRange.top;				if (vloc > sliderRange.bottom) vloc = sliderRange.bottom;				delta = vloc - ovloc;					/* The delta tells us how much the thumb moved. */				if (					(delta < -((32 - kThumbHeight) / 2)) ||					(delta >  ((32 - kThumbHeight) / 2))				) {					for (i = pgRect.top; i < pgRect.bottom; i += 32)						SliderDrawCIcon(icons[2], pgRect.left, i);							/* The thumb moved too far for a single plot to cover							** up the old position, so clear the old thumb. */				}				calc  = max + 1;		/* Force below math to be with longs. */				calc *= (vloc - sliderRange.top);					/* We use max + 1 because there is one more game					** move position than moves in the game.  This is					** because we can position in front of the first move,					** as well as after the last move. */				calc /= (sliderRange.bottom - sliderRange.top);				val = calc;				if (val > max) val = max;				if (val < 0)   val = 0;				if (delta)					SliderDrawCIcon(icons[3], pgRect.left, vloc - kThumbOffset);										/* The thumb is now updated. */				SetClip(origClipRgn);				SliderAction(val);					/* We set the clipRgn back to the original so the board					** can update.  (Pretty boring if it doesn't). */				SetClip(clipRgn);				FocusDraw();					/* Back to our normally scheduled program... */				lastMouseLoc = mouseLoc;				(*mMacControlH)->contrlValue = val;				ovloc = vloc;			}		}	}	else {		/* We missed the thumb.  See if we hit an arrow part... */		delta = hiliteOn = 0;		capRect = ctlRect;		capRect.bottom = ctlRect.top + kCapHeight;		if (max > 1000)			delta = max/1000;		else delta = 1;		if (PtInRect(origMouseLoc, &capRect)) {			delta = -delta;			hiliteCap = 0;		}		else {			capRect = ctlRect;			capRect.top = ctlRect.bottom - kCapHeight + 1;			if (PtInRect(origMouseLoc, &capRect)) {				hiliteCap = 1;			}		}		if (delta) {	/* We hit an arrow, and there is a change to do... */			do {				TrackAndRun(&mouseLoc);				SetClip(clipRgn);				if (PtInRect(mouseLoc, &capRect)) {		/* Still in arrow... */					val = (*mMacControlH)->contrlValue + delta;					if (val < 0) val = 0;					if (val > max) val = max;										if (val != (*mMacControlH)->contrlValue) {	/* Still scrolling... */						hiliteOn = true;						(*mMacControlH)->contrlValue = val;						UpdateSlider(hiliteCap);						SetClip(origClipRgn);						SliderAction(val);					}					else {		/* Scrolled as far as we can go, so unhilite arrow. */						if (hiliteOn) {							UpdateSlider();							hiliteOn = false;						}					}				}				else {		/* Outside arrow, so unhilite it. */					if (hiliteOn) {						UpdateSlider();						hiliteOn = false;					}				}				while ((StillDown()) && (origTick + 20 > TickCount())) {};					/* Don't go too fast. */			} while (StillDown());		}	}	SetClip(origClipRgn);	DisposeRgn(workRgn);	DisposeRgn(clipRgn);	DisposeRgn(origClipRgn);	UpdateSlider();		/* Snap the slider to a move position.  The user may have let go of the		** slider at a position that doesn't map exactly to the game position. */	for (i = 2; i < 5; i++) KillCIcon(icons[i]);	SetPort(oldPort);}/*****************************************************************************/void	CSliderControl::SliderAction(short /*newPos*/){}