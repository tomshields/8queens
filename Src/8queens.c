/***********************************************************************
 *
 Copyright © 1998, Basswood Associates, www.basswood.com

 Freeware.  You may do whatever you like with this, as long as you don't sue me.
 
 Ugly lawyer stuff:   
 BASSWOOD ASSOCIATES MAKES NO REPRESENTATIONS OR WARRANTIES THAT THE SOFTWARE IS
 FREE OF ERRORS OR THAT THE SOFTWARE IS SUITABLE FOR YOUR USE.  THE SOFTWARE IS
 PROVIDED ON AN "AS IS" BASIS.  3COM MAKES NO WARRANTIES, TERMS OR CONDITIONS, 
 EXPRESS OR IMPLIED, EITHER IN FACT OR BY OPERATION OF LAW, STATUTORY OR OTHERWISE,
 INCLUDING WARRANTIES, TERMS, OR CONDITIONS OF MERCHANTABILITY, FITNESS FOR A
 PARTICULAR PURPOSE, AND SATISFACTORY QUALITY.

 TO THE FULL EXTENT ALLOWED BY LAW, BASSWOOD ASSOCIATES ALSO EXCLUDES FOR ITSELF AND
 ITS SUPPLIERS ANY LIABILITY, WHETHER BASED IN CONTRACT OR TORT (INCLUDING 
 NEGLIGENCE), FOR DIRECT, INCIDENTAL, CONSEQUENTIAL, INDIRECT, SPECIAL, OR PUNITIVE 
 DAMAGES OF ANY KIND, OR FOR LOSS OF REVENUE OR PROFITS, LOSS OF BUSINESS, LOSS OF
 INFORMATION OR DATA, OR OTHER FINANCIAL LOSS ARISING OUT OF OR IN CONNECTION WITH
 THIS SOFTWARE, EVEN IF BASSWOOD ASSOCIATES HAS BEEN ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGES.

 *************************************************************************
 *
 * PROJECT:  8queens
 * FILE:     8queens.c
 * AUTHOR:   Tom Shields, Dec 15, 1998
 *
 * DECLARER: 8queens
 *
 * DESCRIPTION: Solve the 8 Queens problem
 *    
 *
 **********************************************************************/
#include <Pilot.h>
#include <SysEvtMgr.h>
#include <KeyMgr.h>
#include "8queens.h"
#include "8queensRsc.h"



/***********************************************************************
 *
 *   Entry Points
 *
 ***********************************************************************/


/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
static GridType         curGrid, blinkGrid;
static OpenGridType     findGrid;
Boolean                 someReversed;

static WinHandle  OffscreenGridWinH = 0;
static WinHandle  OffscreenBitmapWinH = 0;

typedef enum gType {
  queenwhiteGraphic = 0,
  queenwhiterevGraphic,
  queenblackGraphic,
  queenblackrevGraphic,
  allwhiteGraphic,
  allblackGraphic,
  lastGraphic
} GraphicType;

static Int        GraphicsTable[] = {
  queenwhiteBitmap,
  queenwhiterevBitmap,
  queenblackBitmap,
  queenblackrevBitmap,
  allwhiteBitmap,
  allblackBitmap,
};

static E8queensPrefType Prefs;


/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/


/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version is meet your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *                             
 ***********************************************************************/
static Err RomVersionCompatible (DWord requiredVersion, Word launchFlags)
{
  DWord romVersion;

  // this is boilerplate...
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
  if (romVersion < requiredVersion) {
    if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
        (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) {
      FrmAlert (RomIncompatibleAlert);
      if (romVersion < 0x03000000)
        AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
    }
    return (sysErrRomIncompatible);
  }
  return 0;
}


/***********************************************************************
 *
 * FUNCTION:  DrawBitmap
 *
 * DESCRIPTION: Convenience routine to draw a bitmap at specified location
 *
 * PARAMETERS:  winHandle -- handle of window to draw to (0 for current window)
 *              resID     -- bitmap resource id
 *              x, y      -- bitmap origin relative to current window
 *
 * RETURNED:  nothing.
 *
 ***********************************************************************/
static void DrawBitmap(WinHandle winHandle, Int resID, Short x, Short y)
{
  Handle    resH;
  BitmapPtr resP;
  WinHandle currDrawWindow;
  
  // If passed a non-null window handle, set it up as the draw window, saving
  // the current draw window
  if (winHandle)
    currDrawWindow = WinSetDrawWindow(winHandle);
    
  resH = DmGetResource(bitmapRsc, resID);
  ErrFatalDisplayIf(!resH, "no bitmap");
  resP = MemHandleLock(resH);
  WinDrawBitmap(resP, x, y);
  MemPtrUnlock(resP);
  DmReleaseResource(resH);
  
  // Restore the current draw window
  if (winHandle)
    WinSetDrawWindow(currDrawWindow);
}


/***********************************************************************
 *
 * FUNCTION:     DrawCell
 *
 * DESCRIPTION: Draw a grid cell
 *
 * PARAMETERS:  dstWinH -- destination window handle (0 for current window)
 *              row   -- cell row (0-based)
 *              col   -- cell column (0-based)
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void DrawCell(WinHandle dstWinH, Int row, Int col)
{
  Int       bitmapID = 0;
  Int       bitmapIndex = 0;
  Int       x, y;

  // Figure out where it goes
  x = gridOriginX + col * bitmapWidth;
  y = gridOriginY + row * bitmapHeight;

  // Determine which bitmap to use
  if (blinkGrid[row][col]) {
    if (row+col & 1)
      bitmapIndex = queenwhiterevGraphic;
    else
      bitmapIndex = queenblackrevGraphic;
  } else if (curGrid[row][col]) {
    if (row+col & 1)
      bitmapIndex = queenwhiteGraphic;
    else
      bitmapIndex = queenblackGraphic;
  } else {
    if (row+col & 1)
      bitmapIndex = allwhiteGraphic;
    else
      bitmapIndex = allblackGraphic;
  }
  
  // Draw the bitmap
  if (OffscreenBitmapWinH) {
    RectangleType srcRect;
    srcRect.topLeft.x = bitmapWidth * bitmapIndex;
    srcRect.topLeft.y = 0;
    srcRect.extent.x = bitmapWidth;
    srcRect.extent.y = bitmapHeight;
    WinCopyRectangle(OffscreenBitmapWinH, dstWinH, &srcRect, x, y, scrCopy);
  } else {
    bitmapID = GraphicsTable[bitmapIndex];
    DrawBitmap(dstWinH, bitmapID, x, y);
  }
}


/***********************************************************************
 *
 * FUNCTION:     DrawGrid
 *
 * DESCRIPTION: Draw the grid
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void DrawGrid(void)
{
  Int           i, j;
  RectangleType srcRect;
  
  //
  // Draw the grid -- If the off-screen window exists,
  // the drawing will happen on the off-screen window, and then copied
  // to the on-screen window for performance
  // If, on the other hand, the off-screen window was not created, the drawing
  // will take place directly on the on-screen window 
  //

  for ( i=0; i < maxRows; i++ )
    for ( j=0; j < maxCols; j++ )
      DrawCell(OffscreenGridWinH, i, j);

  // Draw the line around the outside
  // assumes gridOriginX and gridOriginY are > 0
  srcRect.topLeft.x = gridOriginX - 1;
  srcRect.topLeft.y = gridOriginY - 1;
  srcRect.extent.x =  maxCols * bitmapWidth + 2;
  srcRect.extent.y =  maxRows * bitmapHeight + 2;
  WinDrawRectangle(&srcRect, 0);
  
  // Do the copy if off-screen
  if (OffscreenGridWinH) {
    srcRect.topLeft.x = gridOriginX;
    srcRect.topLeft.y = gridOriginY;
    srcRect.extent.x =  maxCols * bitmapWidth;
    srcRect.extent.y =  maxRows * bitmapHeight;
    WinCopyRectangle(OffscreenGridWinH, 0, &srcRect,
                      gridOriginX, gridOriginY, scrCopy );
  }
}


/***********************************************************************
 *
 * FUNCTION:     RotateLeft
 *
 * DESCRIPTION:  Rotate the grid counterclockwise
 *               Assumes the board is square
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void RotateLeft(void)
{
  Int           i, j;
  Boolean       temp;
  // optimization
  Int           maxIndex = maxRows - 1;

  // assumes square board, so only use maxRows
  for (i=0; i < maxRows/2; i++) {
    for (j=i; j < maxIndex-i; j++) {
      temp = curGrid[i][j];
      curGrid[i][j] = curGrid[j][maxIndex-i];
      curGrid[j][maxIndex-i] = curGrid[maxIndex-i][maxIndex-j];
      curGrid[maxIndex-i][maxIndex-j] = curGrid[maxIndex-j][i];
      curGrid[maxIndex-j][i] = temp;
    }
  } 
}


/***********************************************************************
 *
 * FUNCTION:     RotateRight
 *
 * DESCRIPTION:  Rotate the grid clockwise
 *               Assumes the board is square
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void RotateRight(void)
{
  Int           i, j;
  Boolean       temp;
  // optimization
  Int           maxIndex = maxRows - 1;

  // assumes square board, so only use maxRows
  for (i=0; i < maxRows/2; i++) {
    for (j=i; j < maxIndex-i; j++) {
      temp = curGrid[i][j];
      curGrid[i][j] = curGrid[maxIndex-j][i];
      curGrid[maxIndex-j][i] = curGrid[maxIndex-i][maxIndex-j];
      curGrid[maxIndex-i][maxIndex-j] = curGrid[j][maxIndex-i];
      curGrid[j][maxIndex-i] = temp;
    }
  } 
}


/***********************************************************************
 *
 * FUNCTION:     FlipVert
 *
 * DESCRIPTION:  Flip the grid vertically, around a horizontal axis
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void FlipVert(void)
{
  Int           i, j;
  Boolean       temp;

  for (j=0; j < maxCols; j++) {
    for (i=0; i < maxRows/2; i++) {
      temp = curGrid[i][j];
      curGrid[i][j] = curGrid[maxRows-1-i][j];
      curGrid[maxRows-1-i][j] = temp;
    }
  } 
}


/***********************************************************************
 *
 * FUNCTION:     FlipHoriz
 *
 * DESCRIPTION:  Flip the grid horizontally, around a vertical axis
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void FlipHoriz(void)
{
  Int           i, j;
  Boolean       temp;

  for (i=0; i < maxRows; i++) {
    for (j=0; j < maxCols/2; j++) {
      temp = curGrid[i][j];
      curGrid[i][j] = curGrid[i][maxCols-1-j];
      curGrid[i][maxCols-1-j] = temp;
    }
  } 
}


/***********************************************************************
 *
 * FUNCTION:     CheckSolution
 *
 * DESCRIPTION:  Check the solution, putting up a congratulations dialog
 *               if solved, or reversing the queens that can attack
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void CheckSolution(void)
{
  Int           i, j, count;

  // first check rows for attacks
  for (i=0; i < maxRows; i++) {
    count = 0;
    for (j=0; j < maxCols; j++) {
      if (curGrid[i][j]) count++;
    }
    if (count > 1) {
      for (j=0; j < maxCols; j++)
        blinkGrid[i][j] = curGrid[i][j];
    }
  }

  // then cols
  for (j=0; j < maxCols; j++) {
    count = 0;
    for (i=0; i < maxRows; i++) {
      if (curGrid[i][j]) count++;
    }
    if (count > 1) {
      for (i=0; i < maxRows; i++)
        blinkGrid[i][j] = curGrid[i][j];
    }
  } 

  // check diags
  for (j=0; j < maxCols; j++) {
    // left to right, through top row
    count = 0;
    for (i=0; i < maxRows-j; i++) {
      if (curGrid[i][i+j]) count++;
    }
    if (count > 1) {
      for (i=0; i < maxRows-j; i++)
        blinkGrid[i][i+j] = curGrid[i][i+j];
    }
    // right to left, through top row
    count = 0;
    for (i=0; i <= j; i++) {
      if (curGrid[i][j-i]) count++;
    }
    if (count > 1) {
      for (i=0; i <= j; i++)
        blinkGrid[i][j-i] = curGrid[i][j-i];
    }
    // left to right, through bottom row
    count = 0;
    for (i=0; i <= j; i++) {
      if (curGrid[maxRows-1-i][j-i]) count++;
    }
    if (count > 1) {
      for (i=0; i <= j; i++)
        blinkGrid[maxRows-1-i][j-i] = curGrid[maxRows-1-i][j-i];
    }
    // right to left, through bottom row
    count = 0;
    for (i=0; i < maxRows-j; i++) {
      if (curGrid[maxRows-1-i][i+j]) count++;
    }
    if (count > 1) {
      for (i=0; i < maxRows-j; i++)
        blinkGrid[maxRows-1-i][i+j] = curGrid[maxRows-1-i][i+j];
    }
  } 

  // see if we found any attacks, and count the queens
  count = 0;
  for (j=0; j < maxCols; j++) {
    for (i=0; i < maxRows; i++) {
      if (blinkGrid[i][j])
        someReversed = true;
      if (curGrid[i][j])
        count++;
    }
  }

  if (someReversed)
    DrawGrid();
  else if (count == maxRows)
    FrmAlert(SuccessAlert);
}


/***********************************************************************
 *
 * FUNCTION:     FindCheck
 *
 * DESCRIPTION:  Check the findGrid for attacks
 *
 * PARAMETERS:  none
 *
 * RETURNED:  Boolean -- true if no attacks found
 *
 ***********************************************************************/
static Boolean FindCheck(void)
{
  Int i, j;
  for (i=0; i < maxRows; i++) {
    if (findGrid[i] == maxCols)
      continue;
    for (j=i+1; j < maxCols; j++) {
      if (findGrid[j] == maxCols)
        continue;
      if (findGrid[i] == findGrid[j])
        return false;
      if (i+findGrid[i] == j+findGrid[j])
        return false;
      if (i-findGrid[i] == j-findGrid[j])
        return false;
    }
  }
  return true;
}


/***********************************************************************
 *
 * FUNCTION:     FindRecurse
 *
 * DESCRIPTION:  Find a candidate queen in the given row, and recurse
 *
 * PARAMETERS:  row  -- the row to find candidates for
 *
 * RETURNED:  Boolean -- true if a solution is found
 *
 ***********************************************************************/
static Boolean FindRecurse(Int row)
{
  Int col;

  // check to see if we're done
  if (row >= maxRows)
    return true;

  // if there's already a queen in this row, keep spinning
  if (findGrid[row] < maxCols)
    return FindRecurse(row+1);

  // otherwise, try to find a candidate
  for (col=0; col < maxCols; col++) {
    findGrid[row] = col;
    if (FindCheck()) {
      if (FindRecurse(row+1))
        return true;
    }
  }
  findGrid[row] = maxCols;

  // didn't find any
  return false;
}


/***********************************************************************
 *
 * FUNCTION:     FindSolution
 *
 * DESCRIPTION:  Find a solution, given some queens already placed
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void FindSolution(void)
{
  Int           i, j;
  Boolean       found;

  CheckSolution();
  if (someReversed)
    return;

  // build findGrid
  for (i=0; i < maxRows; i++) {
    findGrid[i] = maxCols;
    for (j=0; j < maxCols; j++) {
      if (curGrid[i][j])
        findGrid[i] = j;
    }
  }

  for (i=0; i < maxRows && findGrid[i] < maxCols; i++) ;
  if (i == maxRows) // puzzle already completed
    return;

  found = FindRecurse(i);
  if (found) {
    for (i=0; i < maxRows; i++)
      curGrid[i][findGrid[i]] = true;
    DrawGrid();
  } else {
    FrmAlert(NoSolnsAlert);
  }
}


/***********************************************************************
 *
 * FUNCTION:     HandlePenDown
 *
 * DESCRIPTION: Handles a pen down;
 *
 * PARAMETERS:  penX      -- display-relative x position
 *              penY      -- display-relative y position
 *
 * RETURNED:  true if handled; false if not
 *
 ***********************************************************************/
static Boolean HandlePenDown(Int penX, Int penY)
{
  UInt         row, col;
  SWord        x = (SWord)penX;
  SWord        y = (SWord)penY;
  
  // Map display relative coordinates to window-relative
  WinDisplayToWindowPt(&x, &y);

  // Convert to grid-relative coordinates
  x -= gridOriginX;
  y -= gridOriginY;
  
  // Is it on the grid?
  if ((x < 0) || (x >= maxCols * bitmapWidth) ||
      (y < 0) || (y >= maxRows * bitmapHeight)) {
    return false;
  }
  
  // Get the piece position
  col = x / bitmapWidth;
  row = y / bitmapHeight;

  // turn on or off as appropriate
  curGrid[row][col] = !curGrid[row][col];

  // If we had some reversed queens, redraw the board
  if (someReversed) {
    MemSet(blinkGrid, sizeof(blinkGrid), 0);
    someReversed = false;
    DrawGrid();
  } else 
    DrawCell(0, row, col);
  
  return true;
}


/***********************************************************************
 *
 * FUNCTION:    MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    true if the command was handled
 *
 ***********************************************************************/
static Boolean MainFormDoCommand (Word command)
{
  Boolean   handled = false;
  FormPtr   frm;

  MenuEraseStatus(MenuGetActiveMenu());

  switch (command)
    {
    case MainOptionsHelp:
      FrmHelp(SolveHelpString);
      handled = true;
      break;
      
    case MainOptionsAbout8Queens:
      frm = FrmInitForm(AboutForm);
      FrmDoDialog(frm);
      FrmDeleteForm(frm);
      handled = true;
      break;
    }
    
  return handled;
}


/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Main View"
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has been handled and should not be passed
 *              to a higher level handler.
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent (EventPtr event)
{
  Boolean handled = false;

  if (event->eType == ctlSelectEvent) {
    switch (event->data.ctlSelect.controlID) {

      // Main form - buttons along the bottom
      case MainCheckButton:
        CheckSolution();
        handled = true;
        break;
      case MainFindButton:
        FindSolution();
        handled = true;
        break;
      case MainBrowseButton:
        // TODO
        handled = true;
        break;

      // buttons along the side, both forms
      case MainRotLeftButton:
        RotateLeft();
        DrawGrid();
        handled = true;
        break;
      case MainRotRightButton:
        RotateRight();
        DrawGrid();
        handled = true;
        break;
      case MainFlipVertButton:
        FlipVert();
        DrawGrid();
        handled = true;
        break;
      case MainFlipHorizButton:
        FlipHoriz();
        DrawGrid();
        handled = true;
        break;
      case MainClearButton:
        MemSet(curGrid, sizeof(curGrid), 0);
        DrawGrid();
        handled = true;
        break;
       
      default:
        break;
    }
  }

  else if (event->eType == penDownEvent) {
    if ((KeyCurrentState() & (keyBitPageUp | keyBitPageDown)) &&
        (FrmPointInTitle(FrmGetActiveForm(), event->screenX, event->screenY))) {
      FrmAlert(EggAlert);
      handled = true;
    } else {
      handled = HandlePenDown(event->screenX, event->screenY);
    }
  }
  
  else if (event->eType == menuEvent) {
    handled = MainFormDoCommand(event->data.menu.itemID);
  }

  else if ((event->eType == frmUpdateEvent) || (event->eType == frmOpenEvent)) {
    FrmDrawForm(FrmGetActiveForm());
    DrawGrid();
    handled = true;
  }
    
  return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has been handled and should not be passed
 *              to a higher level handler.
 *
 ***********************************************************************/
static Boolean AppHandleEvent( EventPtr eventP)
{
  Word formId;
  FormPtr frmP;


  if (eventP->eType == frmLoadEvent)
    {
    // Load the form resource.
    formId = eventP->data.frmLoad.formID;
    frmP = FrmInitForm(formId);
    FrmSetActiveForm(frmP);

    // Set the event handler for the form.  The handler of the currently
    // active form is called by FrmHandleEvent each time is receives an
    // event.
    switch (formId)
      {
      case MainForm:
        FrmSetEventHandler(frmP, MainFormHandleEvent);
        break;

      default:
        ErrNonFatalDisplayIf(true, "Invalid Form Load Event");
        break;

      }
    return true;
    }
  
  return false;
}


/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void AppEventLoop(void)
{
  Word error;
  EventType event;

  do {
    EvtGetEvent(&event, evtWaitForever);
    
    if (!SysHandleEvent(&event))
      if (!MenuHandleEvent(0, &event, &error))
        if (!AppHandleEvent(&event))
          FrmDispatchEvent(&event);

  } while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:     LoadGrid
 *
 * DESCRIPTION: Load saved grid from app preferences.
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void LoadGrid(void)
{
  Boolean found;
  
  // Try loading a saved grid
  found = PrefGetAppPreferencesV10(appFileCreator, appVersionNum, &Prefs,
                                    sizeof(Prefs));
  if (found && Prefs.signature == E8queensPrefSignature) {
    MemMove(curGrid, Prefs.grid, sizeof(curGrid));
  } else {
    MemSet(curGrid, sizeof(curGrid), 0);
  }
  MemSet(blinkGrid, sizeof(blinkGrid), 0);
}



/***********************************************************************
 *
 * FUNCTION:     SaveGrid
 *
 * DESCRIPTION: Save grid in app preferences.
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void SaveGrid(void)
{
  Prefs.signature = E8queensPrefSignature;
  MemMove(Prefs.grid, curGrid, sizeof(curGrid));

  // Save our preferences to the Preferences database
  PrefSetAppPreferencesV10(appFileCreator, appVersionNum, &Prefs, sizeof(Prefs));
}


/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION: Initialize application.
 *
 *            Create and initialize offscreen window for drawing optimization
 *            Load grid from preferences
 *
 * PARAMETERS:   none
 *
 * RETURNED:     0 on success
 *
 ***********************************************************************/
static Word StartApplication (void)
{
  Word  error;

  LoadGrid();
  
  // Initialize offscreen bitmap window once
  // (used for drawing optimization - this significantly speeds up mass redraws
  // such as during during initial draw, grid restoraration, and screen update)
  if (!OffscreenBitmapWinH) {
    OffscreenBitmapWinH = WinCreateOffscreenWindow(bitmapWidth*lastGraphic,
                                               bitmapHeight, screenFormat, &error);
    if (OffscreenBitmapWinH) {
      Int i;
      for (i=0; i < lastGraphic; i++) {
        DrawBitmap(OffscreenBitmapWinH, GraphicsTable[i], i*bitmapWidth, 0);
      }
    }
  }

  // this window is a little bigger than it needs to be so I don't have to mess
  // with origin calculations, and for the border - ugly, but easy
  if (!OffscreenGridWinH) {
    OffscreenGridWinH = WinCreateOffscreenWindow (gridOriginX+maxCols*bitmapWidth+1,
                          gridOriginY+maxRows*bitmapHeight+1, screenFormat, &error);
  }

  return 0;
}


/***********************************************************************
 *
 * FUNCTION:  StopApplication
 *
 * DESCRIPTION: Save the current state of the application, close all
 *            forms, and deletes the offscreen window.
 *
 * PARAMETERS:  none
 *
 * RETURNED:  nothing
 *
 ***********************************************************************/
static void StopApplication (void)
{
  SaveGrid();

  FrmCloseAllForms();
  
  // Free the offscreen window
  if (OffscreenBitmapWinH) {
    WinDeleteWindow(OffscreenBitmapWinH, false);
    OffscreenBitmapWinH = 0;
  }

  // Free the offscreen window
  if (OffscreenGridWinH) {
    WinDeleteWindow(OffscreenGridWinH, false);
    OffscreenGridWinH = 0;
  }
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    0
 *
 ***********************************************************************/
 
DWord PilotMain (Word cmd, Ptr cmdPBP, Word launchFlags)
{
  Word error;
  
  error = RomVersionCompatible (ourMinVersion, launchFlags);
  if (error) return (error);


  if (cmd == sysAppLaunchCmdNormalLaunch) {
    error = StartApplication();
    if (error) return (error);

    FrmGotoForm(MainForm);

    AppEventLoop();
    StopApplication();
  }

  return 0;
}
