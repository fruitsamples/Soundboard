// ===========================================================================//	CQuickTimeWindow.cp			�1995 Apple Computer, Inc. All rights reserved.// ===========================================================================#ifdef PowerPlant_PCH#include PowerPlant_PCH#endif#include "CQuickTimeWindow.h"#ifndef __STANDARDFILE__#include <StandardFile.h>#endifconst CommandT cmd_StartMovie 	= 13101;const CommandT cmd_StopMovie 	= 13102;// ---------------------------------------------------------------------------//		� CreateQuickTimeWindow// ---------------------------------------------------------------------------//	Return a newly created Window object initialized from a PPob resourceCQuickTimeWindow*CQuickTimeWindow::CreateQuickTimeWindow(	ResIDT inWindowID,	LCommander *inSuperCommander,	Boolean inShow,	Movie inMovie){	Str63 movieTitle;	CQuickTimeWindow *theWindow = nil;		if (!inMovie)		inMovie = GetMovieFromFile(movieTitle);	else *movieTitle = 0;			if (inMovie)		theWindow = (CQuickTimeWindow*)LWindow::CreateWindow(inWindowID, inSuperCommander);		if (theWindow) {		theWindow->DisplayMovie(inMovie, movieTitle);				if (inShow) theWindow->Show();	}	else if (inMovie)		::DisposeMovie(inMovie);			return theWindow;}// ---------------------------------------------------------------------------//		� CreateQuickTimeWindowStream// ---------------------------------------------------------------------------//	Return a newly created Window object initialized with data from a StreamCQuickTimeWindow*CQuickTimeWindow::CreateQuickTimeWindowStream(	LStream	*inStream){	return (new CQuickTimeWindow(inStream));}// ---------------------------------------------------------------------------//		� CQuickTimeWindow(LStream*)// ---------------------------------------------------------------------------//	Construct Window from the data in a streamCQuickTimeWindow::CQuickTimeWindow(	LStream	*inStream) : LWindow(inStream){	mMovieController = nil;	mMovie = nil;}// ---------------------------------------------------------------------------//		� ~CQuickTimeWindow()// ---------------------------------------------------------------------------//	Destructor for Window from the data in a streamCQuickTimeWindow::~CQuickTimeWindow(){	if (mMovieController) {		::StopMovie(mMovie);		::MCMovieChanged(mMovieController, mMovie);		::DisposeMovieController(mMovieController);	}		if (mMovie)		::DisposeMovie(mMovie);}// ---------------------------------------------------------------------------//		� GetMovieFromFile(Str63 movieTitle)// ---------------------------------------------------------------------------//	Get a QuickTime movie from a file via StandardGetFilePreviewMovieCQuickTimeWindow::GetMovieFromFile(Str63 movieTitle){	Movie				theMovie = nil;	SFTypeList			fileTypes;	StandardFileReply	reply;	fileTypes[0] = MovieFileType;	::StandardGetFilePreview(nil, 1, fileTypes, &reply);		if (reply.sfGood) {		OSErr	err;		Int16	movieRefNum;		err = ::OpenMovieFile(&reply.sfFile, &movieRefNum, fsRdPerm);				if (!err) {			Int16	actualResID = DoTheRightThing;			Boolean	wasChanged;			::NewMovieFromFile(&theMovie, movieRefNum, &actualResID,										nil, newMovieActive, &wasChanged);													::CloseMovieFile(movieRefNum);		}	}		if (theMovie)		::BlockMove(reply.sfFile.name, movieTitle, sizeof(Str63));			return theMovie;}// ---------------------------------------------------------------------------//		� AttemptQuit(long inSaveOption)// ---------------------------------------------------------------------------//	User is trying to quit, close our windowBooleanCQuickTimeWindow::AttemptQuit(	long	inSaveOption){	if (LWindow::AttemptQuit(inSaveOption)) {		DoClose();		return true;	}	else return false;}// ---------------------------------------------------------------------------//		� DrawSelf()// ---------------------------------------------------------------------------//	Draw the movie controlervoidCQuickTimeWindow::DrawSelf(){	LWindow::DrawSelf();	if (mMovieController)		::MCDraw(mMovieController, GetMacPort());}// ---------------------------------------------------------------------------//		� DisplayMovie(Movie inMovie, Str63 inMovieTitle)// ---------------------------------------------------------------------------//	Display movie, resize the window, and set window titlevoidCQuickTimeWindow::DisplayMovie(Movie inMovie, Str63 inMovieTitle){	Rect	movieBounds;	if (inMovie) {		if (mMovieController) {			::StopMovie(mMovie);			::MCMovieChanged(mMovieController, mMovie);			::DisposeMovieController(mMovieController);		}				if (mMovie)			::DisposeMovie(mMovie);			mMovie = inMovie;			::GetMovieBox(inMovie, &movieBounds);		::OffsetRect(&movieBounds, -movieBounds.left, -movieBounds.top);		::SetMovieBox(inMovie, &movieBounds);		::SetMovieGWorld(inMovie, (CGrafPtr) GetMacPort(), nil);				CalcLocalFrameRect(movieBounds);		mMovieController = ::NewMovieController(inMovie, &movieBounds, mcTopLeftMovie);				if (mMovieController) {			Boolean enableKeys = true;						::MCDoAction(mMovieController, mcActionSetKeysEnabled, &enableKeys);			if (movieBounds.bottom)			{				movieBounds.top = 48;				movieBounds.bottom = 870;			}			movieBounds.left = 48;			movieBounds.right = 1152;						::MCDoAction(mMovieController, mcActionSetGrowBoxBounds, &movieBounds);			::MCGetControllerBoundsRect(mMovieController, &movieBounds);					PortToGlobalPoint(topLeft(movieBounds));			PortToGlobalPoint(botRight(movieBounds));			DoSetBounds(movieBounds);						if (*inMovieTitle)				SetDescriptor(inMovieTitle);				StartRepeating();		} 		else StopRepeating();	}}// ---------------------------------------------------------------------------//		� ClickInContent// ---------------------------------------------------------------------------//	Respond to a click in the content region of a WindowvoidCQuickTimeWindow::ClickInContent(	const EventRecord	&inMacEvent){	ComponentResult handled;		if (mMovieController) {		Rect	newMovieBounds, origMovieBounds;			::MCGetControllerBoundsRect(mMovieController, &origMovieBounds);				FocusDraw();		handled = ::MCIsPlayerEvent(mMovieController, &inMacEvent);				::MCGetControllerBoundsRect(mMovieController, &newMovieBounds);		if ((origMovieBounds.right != newMovieBounds.right) ||		    (origMovieBounds.bottom != newMovieBounds.bottom))		{			PortToGlobalPoint(topLeft(newMovieBounds));			PortToGlobalPoint(botRight(newMovieBounds));			DoSetBounds(newMovieBounds);		}	}	else handled = 0;	if (!handled)		LWindow::ClickInContent(inMacEvent);}// ---------------------------------------------------------------------------//		� SpendTime(const EventRecord	&inMacEvent)// ---------------------------------------------------------------------------//	Give the QuickTime movie some timevoidCQuickTimeWindow::SpendTime(	const EventRecord	&inMacEvent){	if (mMovieController) {		EventRecord		macEvent;				FocusDraw();				if (inMacEvent.what == mouseDown) {					// mouseDown events in this window are handled in ClickInContent() above. 			// This was done because when the movie control is given a  mouseDown 			// event AFTER the window has moved (click in drag bar, for example) it 			// gets confused.			//			// Calling OSEventAvail with a zero event mask will always			// pass back a null event. Since all we want to do is give			// the movie player some time, this is exactly what is needed.						::OSEventAvail(0, &macEvent);			::MCIsPlayerEvent(mMovieController, &macEvent);		} 		else ::MCIsPlayerEvent(mMovieController, &inMacEvent);	}}// ---------------------------------------------------------------------------//		� ObeyCommand// ---------------------------------------------------------------------------//	Respond to commandsBooleanCQuickTimeWindow::ObeyCommand(	CommandT	inCommand,	void		*ioParam){	Boolean	cmdHandled = true;		switch (inCommand) {			// +++ Add cases here for the commands you handle		//		Remember to add same cases to FindCommandStatus below		//		to enable/disable the menu items for the commands				case cmd_StartMovie:			if (mMovieController) {				::StartMovie(mMovie);				::MCMovieChanged(mMovieController, mMovie);			}			break;		case cmd_StopMovie:			if (mMovieController) {				::StopMovie(mMovie);				::MCMovieChanged(mMovieController, mMovie);			}			break;					default:			cmdHandled = LWindow::ObeyCommand(inCommand, ioParam);			break;	}		return cmdHandled;}// ---------------------------------------------------------------------------//		� FindCommandStatus// ---------------------------------------------------------------------------//	Pass back status of a (menu) commandvoidCQuickTimeWindow::FindCommandStatus(	CommandT	inCommand,	Boolean		&outEnabled,	Boolean		&outUsesMark,	Char16		&outMark,	Str255		outName){	switch (inCommand) {			// +++ Add cases here for the commands you handle.		//		//		Set outEnabled to TRUE for commands that can be executed at		//		this time.		//		//		If the associated menu items can have check marks, set		//		outUsesMark and outMark accordingly.		//		//		Set outName to change the name of the menu item		case cmd_StartMovie:		case cmd_StopMovie:			outEnabled = (mMovieController != nil);			break;		default:			LWindow::FindCommandStatus(inCommand, outEnabled, outUsesMark,								outMark, outName);			break;	}}// ---------------------------------------------------------------------------//		� HandleKeyPress// ---------------------------------------------------------------------------//	Space toggles the movie on and off.BooleanCQuickTimeWindow::HandleKeyPress(	const EventRecord	&inKeyEvent){	Boolean	keyHandled = false;	Char16	theChar = inKeyEvent.message & charCodeMask;			// Process space key		/* Movie controler handles this	if (theChar == 0x20)	{		long mcFlags;				::MCGetControllerInfo(mMovieController, &mcFlags);			LCommander::GetTarget()->ProcessCommand((mcFlags & mcInfoIsPlaying) ? cmd_StopMovie : cmd_StartMovie);		keyHandled = true;	}	*/		return keyHandled;}