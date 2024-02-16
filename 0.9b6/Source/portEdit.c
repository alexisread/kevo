/* Kevo -- a prototype-based object-oriented language *//* (c) Antero Taivalsaari 1991-1993 				  *//* Some parts (c) Antero Taivalsaari 1986-1988 		  *//* portEdit.c: The browser's object editing facilities */#include "global.h"#include "portGlobal.h"/*---------------------------------------------------------------------------*//* Object scrap (ClipList) operations *//* Given a cell, add the corresponding pair to the clip list *//* Since the CUT command causes the original PAIR to be disposed of *//* we make a copy of the pair before assigning it to the clip list */void cellToClipList(browserWindow, cell)WindowPtr browserWindow;Cell 	  cell;{  PAIR* thisPair = cellToPair(browserWindow, cell);	if (thisPair) addToList(ClipList, copyPair(thisPair));}/* Since the clip list contains copies of PAIRs rather than *//* references to the original pairs, the copies must be deleted *//* when the clip list is emptied */void emptyClipList(){  int index;	for (index = 1; index <= ClipList->logicalSize; index++) 		free((PAIR*)fetchFromList(ClipList, index));	ClipObject = NIL;		emptyList(ClipList);}/*---------------------------------------------------------------------------*//* Graphical user interface editing operations *//* Do a 'CUT' operation *//* Remember: this operation can create garbage which is never collected *//*           (xxx implement garbage collector later) */void doCut(){  WindowPtr thisWindow = FrontWindow();  int wKind = getWindowKind(thisWindow);	switch (wKind) {		case PlainWKind:		case TEWKind:			if (theText) TECut(theText);			break;					case BrowserWKind: {			OBJECT* cwd = getBrowserTarget(thisWindow);			PAIR* firstRemovedPairSucc = NIL;			ListHandle iconList = getBrowserIcons(thisWindow);			Cell cell;			Cell firstSelectedCell;			if (whoToModify == DERIVATIVES) {				SysBeep(1);				fprintf(confile, "== This version does not support CUT in the 'derivatives' mode ==\n");  				break;			}	  			/* Get the first selected pair (needed later) */			/* If nothing is selected, return */			firstSelectedCell.h = -1; firstSelectedCell.v = 0;			if (!nextSelectedCell(iconList, &firstSelectedCell)) break;  			/* Create new clone family if needed */  			if (whoToModify == THIS_ONLY) deriveObject(cwd);  			/* Initialize object clipboard information */				emptyClipList();				ClipObject = cwd;				ClipMode = CLIP_CUT;				deleteContext(ClipContext);				ClipContext = copyContext(getContext(cwd));			/* Copy the selected pairs to the clip list (in the backward order) */			/* and delete the pairs from the object */			cell.h = (*iconList)->dataBounds.right; 			cell.v = (*iconList)->dataBounds.bottom-1;			if (prevSelectedCell(iconList, &cell)) {			  PAIR* tempPair;			  				cellToClipList(thisWindow, cell);				LSetSelect(FALSE, cell, iconList);								/* Ensure that we are not trying to cut array slots */				tempPair = cellToPair(thisWindow, cell);				if (!tempPair) goto finalize;								/* Store the successor of the first pair to be cut */				firstRemovedPairSucc = tempPair->sfa;				removeCell(thisWindow, cell);				while (prevSelectedCell(iconList, &cell)) {					cellToClipList(thisWindow, cell);					LSetSelect(FALSE, cell, iconList);										/* Since we are removing properties in the backwards order,					   we must update the successor information (we are trying					   to find the successor of the FIRST pair to be removed).					*/					/* Ensure that we are not cutting array slots */					tempPair = cellToPair(thisWindow, cell);					if (!tempPair) goto finalize;										firstRemovedPairSucc = tempPair->sfa;					removeCell(thisWindow, cell);				}			finalize:				/* Possibly change the object's clone family */				confirmObjectType(cwd, whoToModify, REMOVING_SOMETHING);							/* Rebind the early bound references within the changed object */				/* starting from the successor of the first non-removed pair */				if (firstRemovedPairSucc) rebindContext(firstRemovedPairSucc, ClipContext);							updateBrowser(thisWindow);			}			break;		}		case CloneBrWKind:			/* Editing of clone families is not allowed */			break;	}}/* Do a 'COPY' operation */void doCopy(){  WindowPtr thisWindow = FrontWindow();  int wKind = getWindowKind(thisWindow);	switch (wKind) {		case PlainWKind:		case TEWKind:			if (theText) TECopy(theText);			break;					case BrowserWKind: {			ListHandle iconList = getBrowserIcons(thisWindow);			OBJECT* cwd = getBrowserTarget(thisWindow);			Cell cell;			/* Copy the selected pairs to the clip list */			cell.h = (*iconList)->dataBounds.right; 			cell.v = (*iconList)->dataBounds.bottom-1;			if (prevSelectedCell(iconList, &cell)) {				/* Initialize object clipboard information */				emptyClipList();				ClipObject = cwd;				ClipMode = CLIP_COPY;				deleteContext(ClipContext);				ClipContext = copyContext(getContext(cwd));				cellToClipList(thisWindow, cell);				while (prevSelectedCell(iconList, &cell)) {					cellToClipList(thisWindow, cell);				}			}			/*				Idea: copy the name of the latest selected cell 				to the TE scrap buffer. Implement later? xxx				Suggested code moved to 'oldCode.c'.			*/			break;		}		case CloneBrWKind:			/* Editing of clone families is not allowed */			break;	}}/* Do a 'PASTE' operation */void doPaste(){  WindowPtr thisWindow = FrontWindow();  int wKind = getWindowKind(thisWindow);	switch (wKind) {		case PlainWKind:		case TEWKind:			if (theText) TEPaste(theText); 			if (theTask) {					/* If the window has an associated task */				/* put the pasted text into its text buffer */				char c;				char*	clipBuf = (char*)*TEScrapHandle();				int		clipLen = TEGetScrapLen();				while (clipLen--) {					putToKeyBuffer(theTask, c = *clipBuf++);					if (c == CR || c == LF) putToKeyBuffer(theTask, c);				}				/* Lines must be separated by two returns or linefeeds */				crsToKeyBuffer(theTask);			}			break;					case BrowserWKind: {			OBJECT* cwd = getBrowserTarget(thisWindow);			PAIR* thisPair = NIL;			PAIR* firstPastedPair = NIL;			ListHandle iconList = getBrowserIcons(thisWindow);			int selected = countSelectedCells(iconList);			int index;			/* If the object clipboard is empty, don't do anything */			if (ClipList->logicalSize == 0) break;			if (whoToModify == DERIVATIVES) {				SysBeep(1);				fprintf(confile, "== This version does not support PASTE in the 'derivatives' mode ==\n");  				break;			}				/* If the number of selected cells is > 1, cannot paste */			if (selected > 1) {				SysBeep(1);				fprintf(confile, "== Cannot paste to a browser with more than one selected cell ==\n");				break;			}  			/* Create new clone family if needed */  			if (whoToModify == THIS_ONLY) deriveObject(cwd);  			/* If the number of selected cells is 1, add before the selected cell */			if (selected) {			  Cell cell;				cell.h = -1; cell.v = 0;				(void)nextSelectedCell(iconList, &cell);				/* Ensure that the selected cell is not an anonymous indexed slot */				thisPair = cellToPair(thisWindow, cell);				if (thisPair) {					PAIR* oldPair;					PAIR* newPair;					/* Walk through the object clip list containing the pairs to be pasted */					/* Adding the properties before the requested pair */					/* Recall that pairs are stored in the clip list backwards */					oldPair = (PAIR*)fetchFromList(ClipList, ClipList->logicalSize);					newPair = addBeforePair(thisPair, copyPair(oldPair));					firstPastedPair = newPair;										for (index = ClipList->logicalSize - 1; index >= 1; index--) {						PAIR* oldPair = (PAIR*)fetchFromList(ClipList, index);						PAIR* newPair;						/* Add the slot to the correct place in the object */						newPair = addBeforePair(thisPair, copyPair(oldPair));						/* Local ("instance") variables require special attention */						/* However, no new space for instance variables is needed */						/* if we are cutting and pasting inside the same object */						if (recognizeObject(newPair->ofa) == VAR &&							!(ClipObject == cwd && ClipMode == CLIP_CUT)) 							pasteInstanceVariable(ClipObject, cwd, oldPair, newPair);					}				}			}			/* If the number of selected cells is 0, add to the end of the object */			else {  				OBJECT*  cwd 	 = getBrowserTarget(thisWindow);  				CONTEXT* context = getContext(cwd); 				firstPastedPair = context->latestPair;				/* Walk through the object clip list containing the pairs to be pasted */				for (index = ClipList->logicalSize; index >= 1; index--) {					PAIR* oldPair = (PAIR*)fetchFromList(ClipList, index);					PAIR* newPair;					/* 						Add a new pair to the end of the context using the						information from the old pair. Since 'addPair' initializes						the flags (ffa), we must copy that field manually.					*/ 					newPair = addPair(context, oldPair->nfa, oldPair->ofa);								newPair->ffa = oldPair->ffa;					/* Local ("instance") variables require special attention */					/* However, no new space for instance variables is needed */					/* if we are cutting and pasting inside the same object */					if (recognizeObject(oldPair->ofa) == VAR &&						!(ClipObject == cwd && ClipMode == CLIP_CUT)) 						pasteInstanceVariable(ClipObject, cwd, oldPair, newPair);				}				if (firstPastedPair)					 firstPastedPair = firstPastedPair->sfa;				else firstPastedPair = context->firstPair;			}			/* If the properties have been COPIED (rather than CUT) */			/* We make the donor object a parent of the current (recipient) object */			if (ClipMode == CLIP_COPY && cwd != ClipObject) {				makeParent(cwd, ClipObject);			}			/* Possibly change the object's clone family */			confirmObjectType(cwd, whoToModify, PASTING_SOMETHING);						/* Rebind the early bound references within the changed object */			/* starting from the first added pair */			if (firstPastedPair) rebindContext(firstPastedPair, ClipContext);						updateBrowser(thisWindow);			break;		}		case CloneBrWKind:			/* Editing of clone families is not allowed */			break;	}}/* Do a 'REMOVE' operation *//* Remember: this operation can create garbage which is never collected *//*           (xxx implement garbage collector later) */void doRemove(){  WindowPtr thisWindow = FrontWindow();  int wKind = getWindowKind(thisWindow);	switch (wKind) {		case PlainWKind:		case TEWKind:			if (theText) TEDelete(theText);			break;					case BrowserWKind: {			OBJECT* cwd = getBrowserTarget(thisWindow);			PAIR* firstRemovedPairSucc = NIL;			ListHandle iconList = getBrowserIcons(thisWindow);			Cell cell;			Cell firstSelectedCell;						if (whoToModify == DERIVATIVES) {				SysBeep(1);				fprintf(confile, "== This version does not support REMOVE in the 'derivatives' mode ==\n");  				break;			}	  			/* Get the first selected cell (needed later) */			/* If nothing is selected, return */			firstSelectedCell.h = -1; firstSelectedCell.v = 0;			if (!nextSelectedCell(iconList, &firstSelectedCell)) break;			/* Create a new clone family if needed */			if (whoToModify == THIS_ONLY) deriveObject(cwd);			/* Remove the selected cells from the object */			cell.h = (*iconList)->dataBounds.right; 			cell.v = (*iconList)->dataBounds.bottom-1;			if (prevSelectedCell(iconList, &cell)) {			  PAIR* tempPair;				CONTEXT* oldContext = copyContext(getContext(cwd));				LSetSelect(FALSE, cell, iconList);				/* Ensure that we are not trying to remove array slots */				tempPair = cellToPair(thisWindow, cell);				if (!tempPair) goto finalize;				/* Store the successor of the first pair to be removed */				firstRemovedPairSucc = tempPair->sfa;				removeCell(thisWindow, cell);				while (prevSelectedCell(iconList, &cell)) {					LSetSelect(FALSE, cell, iconList);										/* Since we are removing properties in the backwards order,					   we must update the successor information (we are trying					   to find the successor of the FIRST pair to be removed).					*/					/* Ensure we are not trying to remove array slots */					tempPair = cellToPair(thisWindow, cell);					if (!tempPair) goto finalize;										firstRemovedPairSucc = tempPair->sfa;					removeCell(thisWindow, cell);				}			finalize:				/* Possibly change the object's clone family */				confirmObjectType(cwd, whoToModify, REMOVING_SOMETHING);				/* Rebind the early bound references within the changed object */				/* starting from the successor of the first removed pair */				if (firstRemovedPairSucc) rebindContext(firstRemovedPairSucc, oldContext);							updateBrowser(thisWindow);				deleteContext(oldContext);			}			else {				SysBeep(1);				fprintf(confile, "== Nothing to REMOVE ==\n");			}			break;		}		case CloneBrWKind:			/* Editing of clone families is not allowed */			break;	}}/* Do a 'SELECT ALL' operation */void doSelectAll(){  int wKind = getWindowKind(FrontWindow());	switch (wKind) {		case PlainWKind:		case TEWKind:			if (theText) TESetSelect(0, 32767, theText);			break;					case BrowserWKind: {			ListHandle iconList = getBrowserIcons(FrontWindow()); 			Cell cell;			cell.h = 0; cell.v = 0;			do {				LSetSelect(TRUE, cell, iconList);			} while (LNextCell(TRUE, TRUE, &cell, iconList));			break;		}		case CloneBrWKind:			/* Editing of clone families is not allowed */			break;	}}/* Do a 'SHOW/HIDE' operation */void doShowHide() {  WindowPtr browserWindow = FrontWindow();  OBJECT* cwd = getBrowserTarget(browserWindow);  ListHandle iconList = getBrowserIcons(browserWindow);  Cell cell;	if (whoToModify == DERIVATIVES) {		SysBeep(1);		fprintf(confile, "== This version does not support HIDE/SHOW in the 'derivatives' mode ==\n");  		return;	}		cell.h = -1; cell.v = 0;	if (nextSelectedCell(iconList, &cell)) {		/* Create a new clone family if needed */		if (whoToModify == THIS_ONLY) deriveObject(cwd);		hideShowCell(browserWindow, cell);		while (nextSelectedCell(iconList, &cell)) {			hideShowCell(browserWindow, cell);		}		/* Possibly change the object's clone family */								confirmObjectType(cwd, whoToModify, ENCAPSULATING_SOMETHING);		updateBrowser(browserWindow);	}	else {		SysBeep(1);		fprintf(confile, "== No cells selected ==\n");	}}/* Redefine a method if the user has changed the decompiled definition *//* in the method display window (which is a TextEdit window) *//* The WINFO structure contains additional information about the *//* method to be redefined */void redefineMethod(methodWindow)WindowPtr methodWindow;{  OBJECT* 	cwd = getMethodContext(methodWindow);  PAIR* 	pair = getMethodPair(methodWindow);    TEHandle thisTE 	= getWindowTE(methodWindow);  char** textHandle = (char**)TEGetText(thisTE);  short textLen 	= (*thisTE)->teLength;  	if (browserTask && textLen && strncmp(*textHandle, ":: ", 3)) {  		/* If the beginning of the string has been changed -> redefine */		char* textPtr = *textHandle;				/* Recompilation must take place in the original context. */		/* Since other tasks might be running the currently redefined */		/* operation, we use a critical region to avoid interference */		numberToKeyBuffer(browserTask, (int)cwd);		textToKeyBuffer(browserTask, " CD <|");		crsToKeyBuffer(browserTask);		/* Use the special 'bredef' operation to handle recompilation */		/* Parameters: ( whoToModify pair --  ) */		numberToKeyBuffer(browserTask, (int)pair);		putToKeyBuffer(browserTask, BL);		numberToKeyBuffer(browserTask, whoToModify);		textToKeyBuffer(browserTask, " bredef");		crsToKeyBuffer(browserTask);		/* Put the contents of the window's TE to */		/* the input buffer of the browser task */		while (textLen--) {			char c = *textPtr++;			putToKeyBuffer(browserTask, c); 			if (c == CR) { 				putToKeyBuffer(browserTask, c); 				if (*textPtr == CR) break;	/* Two CR's (empty line) -> ignore rest */ 			} 		}		textToKeyBuffer(browserTask, " NOW |>");		crsToKeyBuffer(browserTask);  	}}  /* This is an internal operation which is needed only in the next operation *//* It clones an existing object (if it is an object) on background using *//* 'browserTask', and assigns it to a given location */void cloneOnBackground(oldValue, newSlot)OBJECT*  oldValue;OBJECT** newSlot;{ 	/* Copy the value to the given location */ 	*newSlot = oldValue;		/* If the value is an object, clone it on background */			if (isContextObject(oldValue) && respondsTo(oldValue, "copy")) {		/* Generated source code: oldValue.clone newSlot ! */		numberToKeyBuffer(browserTask, oldValue);		textToKeyBuffer(browserTask, ".clone ");		numberToKeyBuffer(browserTask, newSlot);		textToKeyBuffer(browserTask, " !");		crsToKeyBuffer(browserTask);	}}/* pasteInstanceVariable(): this operation takes care of necessary things   when local ("instance") variables are pasted to an object.   The things to be done are:   		- allocate space for an extra slot in the recipient   		  (actually, in every member of a clone family).   		- if the offset of the slot differs from the original offset   		  recompile the code (and set the context to be recompiled).   		- copy the value of the original slot to the new one.   		- if the copied value in the slot is an OOP object, then   		  set 'browserTask' to clone the object in the background   		  (since local variables are local -- not shared).*/void pasteInstanceVariable(oldObject, newObject, oldPair, newPair)OBJECT* oldObject;OBJECT* newObject;PAIR* oldPair;PAIR* newPair;{  CONTEXT* newContext = getContext(newObject);  LIST*    cloneFamily = newContext->cloneFamily;  int	   familySize = cloneFamily->logicalSize;  /* The offset of the instance variable in the old object */  int oldOffset = getVARoffset(oldPair);  /* The offset of the instance variable in the new object */  int newOffset = newObject->sfa;  /* The data value of the instance variable in the old object */  OBJECT* oldValue = *getVARslot(oldObject, oldPair);	/* Allocate space for an extra slot in every instance of this object */	/* (in the THIS_ONLY mode, there is only one copy because of '<derive>') */	/* (so we don't have to worry about resizing extra objects) */	resizeFamilyMembers(newObject, newObject->sfa + 1);		/* If offsets do not match, we have to redefine the operation */	/* in the new object. In order to ensure that everything is ok */	/* after doing this, you must call 'rebindContext' immediately */	/* after returning from 'pasteInstanceVariable'. */ 		if (oldOffset != newOffset) {		/* Copy the operation */		/* The operation is a method like this: "(=VAR) offset" */		newPair->ofa = copyObject(oldPair->ofa);				/* When changing the offset, we utilize the fact that 		   the offset is stored like a shared variable. 		*/		*getREFslot(newPair) = (OBJECT*)newOffset;	}	/* 		Copy the value in the old object's variable slot to the new 		data slot in every member in the clone family.				Furthermore, if the original value is an object id,		set 'browserTask' to clone it on the background.	*/	if (familySize > 0) {  		int index;			/* Walk through each object in the new object's clone family */		for (index = 1; index <= familySize; index++) {			OBJECT*  member = (OBJECT*)fetchFromList(cloneFamily, index);			OBJECT** newSlot = getVARslot(member, newPair);			cloneOnBackground(oldValue, newSlot);		}				}	/* 		In the current implementation, it is possible that some objects 		have an empty clone family. For such objects, do the same as above		individually (without going through the clone family).	*/	else {		OBJECT** newSlot = getVARslot(newObject, newPair);		cloneOnBackground(oldValue, newSlot);	}}