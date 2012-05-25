/***********************************************************************
 *
 Copyright � 1998, Basswood Associates, www.basswood.com

 Freeware.  You may do whatever you like with this, as long as you don't sue me.
 
 Ugly lawyer stuff:   
 BASSWOOD ASSOCIATES MAKES NO REPRESENTATIONS OR WARRANTIES THAT THE SOFTWARE IS
 FREE OF ERRORS OR THAT THE SOFTWARE IS SUITABLE FOR YOUR USE.  THE SOFTWARE IS
 PROVIDED ON AN "AS IS" BASIS.  BASSWOOD MAKES NO WARRANTIES, TERMS OR CONDITIONS, 
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
 * FILE:     8queens.h
 * AUTHOR:   Tom Shields, Dec 15, 1998
 * $Id: 8queens.h,v 1.2 1999/01/06 11:09:47 ts Exp $
 *
 * DECLARER: 8queens
 *
 * DESCRIPTION: Solve the 8 Queens problem
 *    
 *
 **********************************************************************/
#ifndef __8QUEENS_H__
#define __8QUEENS_H__


/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator            '8qns'
#define appVersionNum              0x02
#define appPrefID                  0x00


// Define the minimum OS version we support
#define ourMinVersion   sysMakeROMVersion(2,0,0,sysROMStageRelease,0)

// maximum board size
#define maxRows         8
#define maxCols         8

#define bitmapWidth     16
#define bitmapHeight    16

#define gridOriginX     1
#define gridOriginY     17

#define statusOriginX   2
#define statusOriginY   147
#define statusWidth     130
#define statusHeight    12

/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/
typedef Boolean     GridType[maxRows][maxCols];
typedef Int         OpenGridType[maxRows];
typedef DWord       PackedGridType;

// prefs stuff
#define E8queensPrefSignature   '8qns'

typedef struct {
  DWord             signature;  // signature for pref resource validation
  GridType          grid;       // saved grid info
  Int               numPlaced;  // number of queens in the grid
  Boolean           whiteUpperLeft;  // is the upper left square white?
} E8queensPrefType;

#endif  // __8QUEENS_H__
