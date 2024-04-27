
/*
* ColorizingDMD: Program to edit cROM colorized roms
* Programmed in plain C with Visual Studio 2022 by Zedrummer, 2022
* 
* Linked to the project Visual Pinball Engine for Unity and, as such, is licensed under the GNU General Public License v3.0 https://github.com/freezy/VisualPinball.Engine/blob/master/LICENSE
* 
* Uses OpenGL Immediate Mode for display in 2D in window as this is by far fast enough to make it works at 100+ FPS even on a low end computer with any dedicated GPU
* 
*/

#pragma warning( disable : 6308)

#pragma region Includes

#include "framework.h"
#include "ColorizingDMD.h"
#include <strsafe.h>
#include <gdiplus.h>
using namespace Gdiplus;
#include <CommCtrl.h>
#include "resource.h"
#include <shlwapi.h>
#include "cRom.h"
#include <tchar.h>
#include <direct.h>
#include "OGL_Immediate_2D.h"
#include <windowsx.h>
#include <math.h>
#include <shlobj_core.h>
#include <crtdbg.h>
#include "LiteZip.h"
#include <opencv2/opencv.hpp>
using namespace cv;
//#include "gifski.h"
#include "Gif.h"
#include "dmddevice.h"
#include "crc32.h"
#include <wincodec.h>
#include "serumdll.h"

#pragma endregion Includes

#pragma region Global_Variables

#define MAJOR_VERSION 3
#define MINOR_VERSION 0
#define PATCH_VERSION 4

static TCHAR szWindowClass[] = _T("ColorizingDMD");
static TCHAR szWindowClass2[] = _T("ChildWin");
static TCHAR szWindowClass3[] = _T("Image");

#define IMAGE_MASK_OPACITY 200
#define IMAGE_ZOOM_TEXMUL 4

HINSTANCE hInst;                                // current instance
// vars with all the HWND 
HWND hWnd = NULL, hwTB = NULL, hwTB2 = NULL, hwTB3 = NULL, hwTB4 = NULL;
HWND hPal = NULL;
HWND hPal2 = NULL;
HWND hPal3 = NULL;
HWND hMovSec = NULL, hColSet = NULL, hSprites = NULL, hImages = NULL, hStatus = NULL, hStatus2 = NULL, hStatus3 = NULL, hBStatus = NULL;
HWND hBG = NULL;
HWND hConsole = NULL;
HWND hFrame32 = NULL, hFrame64 = NULL;

UINT ColSetMode = 0, preColRot = 0, acColRot = 0;
GLFWwindow* glfwframe, * glfwframestrip, * glfwsprites, * glfwspritestrip, * glfwimages, * glfwBG, * glfwBGstrip; // GLFW windows
bool fDone = false;
HACCEL hAccelTable;
// vars set to redraw toolbars
bool Update_Toolbar = false;
bool Update_Toolbar2 = false;
bool Update_Toolbar4 = false;

// gl window dimensions
UINT ScrWframe = 500, ScrHframe = 200;
UINT ScrWsprite = 500, ScrHsprite = 200;
UINT ScrWBG = 500, ScrHBG = 200;

// variables for changing color frames
UINT8 SelFrameColor = 0;
DWORD timeSelFrame = 0;

UINT Edit_Mode = 0;
bool isLoadedProject = false; // is there a project loaded?
HIMAGELIST g_hImageList = NULL, g_hImageListD = NULL;
bool Night_Mode = false;

bool NewProj;
char Dir_Dumps[MAX_PATH], Dir_Images[MAX_PATH], Dir_Serum[MAX_PATH], Dir_GIFs[MAX_PATH], Dir_VP[MAX_PATH]; // different paths we need to keep

// the 2 main structures of our project
cRom_struct MycRom = { "",0,0,0,0,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL };
cRP_struct MycRP = { "",{FALSE},{0},0,0,{0},FALSE,0,FALSE };

COLORREF PrevColors[16]={0};
UINT16 originalcolors[16];
UINT acPalette = 0;
// vars for colors selected
UINT noColSel = 0;
UINT noSprSel = 0;
UINT noColMod = 0;

// vars for things drawn over
UINT8 Draw_Extra_Surface[256 * 64];
UINT8 Draw_Extra_Surface2[MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];

UINT Color_Pipette = 0; // are we picking the color?

// vars for zoom
float frame_zoom = 1;
float BG_zoom = 1;
int offset_BG_x = 0, offset_BG_y = 0; // offset in pixels of the top left corner of the frame in the client area
float sprite_zoom = 1;
int offset_frame_x = 0, offset_frame_y = 0;
int offset_sprite_x = 0, offset_sprite_y = 0;

// vars for gl textures
UINT TxCircleFr = (UINT)-1;
UINT TxCircleSpr = (UINT)-1;
UINT TxCircleBG = (UINT)-1;
UINT TxFrameStrip[2] = { (UINT)-1, (UINT)-1 };
UINT TxSpriteStrip[2] = { (UINT)-1, (UINT)-1 };
UINT TxBGStrip[2] = { (UINT)-1, (UINT)-1 };
UINT TxChiffres, TxcRom;
UINT TxImage = (UINT)-1;
UINT TxSelImage = (UINT)-1;

// vars for image import window
UINT8* pSelImage = NULL;
UINT width_image, height_image;
UINT XSelection = 0, YSelection = 0, WSelection = 0, HSelection = 0;
UINT XiSelection = 0, YiSelection = 0, WiSelection = 0, HiSelection = 0;
UINT NSelection = 0;
UINT NiSelection = 0;
float SelRatio = 1;
bool image_zoom_srce = false, image_zoom_dest = false;
int crop_reduction = 0, crop_reductioni = 0, crop_reductionf = 0;
int initcropwidth = 120, initcropheight = 0;
int crop_offsetx = 0, crop_offsety = 0;
int crop_sizeW = 0, crop_sizeH = 0;
int crop_iOsizeW = 0, crop_iOsizeH = 0;
int crop_fOsizeW = 0, crop_fOsizeH = 0;
int crop_ioffsetx = 0, crop_ioffsety = 0, crop_foffsetx = 0, crop_foffsety = 0;
int crop_width = 0, crop_height = 0;
int image_posx = 0, image_posy = 0;
int image_sizeW = 10, image_sizeH = 10;
bool image_mouseLpressed = false, image_mouseRpressed = false;
bool image_source_format_video = false;
cv::VideoCapture image_video_cap;
long image_video_frame_rate = 60;
UINT8 image_video_hour, image_video_minute, image_video_second, image_video_frame;
UINT8 image_video_nhours, image_video_nminutes, image_video_nseconds, image_video_nframes;
bool image_loaded = false;
cv::Mat image_mat, image_org_mat;
int image_brightness = 0, image_contrast = 0, image_blur = 0;
char image_path[MAX_PATH];

// vars for textures applied to the filmstrips
UINT acFSText = 0;
UINT acSSText = 0;
UINT acBSText = 0;

UINT8 Raw_Digit_Def[RAW_DIGIT_W * RAW_DIGIT_H * 11]; // buffer for raw numbers + 'x'

// pointers to memory to draw filmstrips
UINT8* pFrameStrip = NULL;
UINT8* pSpriteStrip = NULL;
UINT8* pBGStrip = NULL;
UINT SliderWidth, PosSlider;
UINT SliderWidth2, PosSlider2;
UINT SliderWidth4, PosSlider4;

// vars for client size of gl windows
UINT ScrW, ScrH;
UINT ScrW2, ScrH2;
UINT ScrW3, ScrH3;
UINT ScrW4, ScrH4;

// vars for monitor resolution
int MonWidth = 1920;
int MonHeight = 1080;

// vars for position in the film strips
int PreFrameInStrip = 0;
UINT NFrameToDraw = 0;
UINT FS_LMargin = 0;
int PreSpriteInStrip = 0;
UINT NSpriteToDraw = 0;
UINT SS_LMargin = 0;
int PreBGInStrip = 0;
UINT NBGToDraw = 0;
UINT BS_LMargin = 0;

// vars for current displayed elements
UINT acFrame = 0, prevFrame = 0;
UINT acSprite = 0, prevSprite=0;
UINT16 acBG = 0;
UINT acDetSprite = 0;

// vars for selections in the filmstrips
unsigned int nSelFrames = 1;
unsigned int SelFrames[MAX_SEL_FRAMES] = { 0 };
unsigned int nSelSprites = 1;
unsigned int SelSprites[MAX_SEL_FRAMES] = { 0 };
const UINT8 SelColor[3] = { 100,150,255 };
const UINT8 acColor[3] = { 255,255,255 };
const UINT8 SameColor[3] = { 0,180,0 };
const UINT8 UnselColor[3] = { 255,50,50 };
const UINT8 SectionColor[3] = { 255,50,255 };
const UINT8 Common_Color[3] = { 255,255,255 };

bool MultiSelectionActionWarning = false;
bool isDrawAllSel = false;
bool isMaskAllSel = false;

DWORD timeLPress = 0, timeRPress = 0, timeUPress = 0, timeDPress = 0; // timers for repetition
UINT32 Mouse_Mode = 0;
bool isDel_Mode; // are we deleting from draw?
// vars for position of the mouse
int MouseIniPosx, MouseIniPosy;
int MouseFinPosx, MouseFinPosy;
int MouseFrSliderlx;
int MouseSpSliderlx;
int MouseBGSliderlx;
bool MouseFrSliderLPressed = false;
bool MouseSpSliderLPressed = false;
bool MouseBGSliderLPressed = false;

char temporaryDir[MAX_PATH]; // temporary path to save action files

// vars for undo/redo
#define UNDO_REDO_BUFFER_SIZE (512*1024*1024)
UINT8* UndoSaveN;
int UndoAvailableN = 0;
UINT UndoActionN[MAX_UNDO];
UINT8 UndoToDisk[MAX_UNDO];
UINT UndoLen[MAX_UNDO];
bool UndoUsedNumber[256];
UINT acUndoPos = 0;
UINT8* RedoSaveN;
int RedoAvailableN = 0;
UINT RedoActionN[MAX_UNDO];
UINT8 RedoToDisk[MAX_UNDO];
UINT RedoLen[MAX_UNDO];
bool RedoUsedNumber[256];
UINT acRedoPos = 0;

bool isAddToMask = true; // true if the selection is add to mask

HANDLE hStdout; // for log window

bool DrawCommonPoints = true; // Do we display the common points of the selected frames (in original frame mode)?

HWND hCBList = NULL; // the combobox list to subclass for right click

#define MAX_SAME_FRAMES 10000
int nSameFrames = 0;
int SameFrames[MAX_SAME_FRAMES];

// vars to know if we redraw the filmstrips
bool UpdateFSneeded = false;
bool UpdateSSneeded = false;
bool UpdateBSneeded = false;

// sprite drawing mode 
UINT8 Sprite_Mode = 0;
bool SpriteFill_Mode = false;

UINT8 acDynaSet = 0; // current dynamic color set used

UINT8 mselcol;

// Vars for copy and paste
UINT8 Copy_Content = 0;
UINT8 Copy_Mask[256 * 64] = { 0 };
UINT8 Copy_iMask[256 * 64] = { 0 };
UINT16 Copy_ColN[256 * 64];
UINT8 Copy_Colo[256 * 64];
UINT8 Copy_Dyna[256 * 64];
UINT8 Paste_Content = 0;
UINT Paste_Width, Paste_Height;
UINT8 Paste_Mask[256 * 64];
UINT16 Paste_ColN[256 * 64];
UINT8 Paste_Colo[256 * 64];
UINT8 Paste_Dyna[256 * 64];
UINT8 Common_Mask[256 * 64];
int Copy_From_Frame = -1;
int Copy_From_DynaMask = -1;
bool Copy_Mode = false;
bool Copy_Available = false;
int paste_offsetx, paste_offsety;
bool Paste_Mode = false;
int Paste_Mirror = 0;

HCURSOR hcColPick, hcArrow, hcPaste; // pick color cursor, standard arrow cursor, paste cursor
GLFWcursor *glfwpastecur, *glfwdropcur, *glfwarrowcur; // glfw cursors

// vars for managing colors in color panels
bool Start_Gradient = false;
UINT Ini_Gradient_Color, Fin_Gradient_Color, Pal_Gradient_Color = 0;
UINT16 Gradient_Colors[64];
UINT Gradient_Length = 0;
bool Start_Gradient2 = false;
UINT Draw_Grad_Ini = 0, Draw_Grad_Fin = 63, Draw_Grad_Palette = 0;
bool Draw_Grad_Opposite = false;
UINT16 Gradient_Colors2[64];
UINT Gradient_Length2 = 0;
bool Start_Col_Exchange = false;
UINT Pre_Col_Pos = 0;
UINT Start_Imported_Col_Exchange = 0;
int Import_Color_Frame = 0;
UINT Ini_Import_Color, Fin_Import_Color;

// vars for permanent pushed buttons
bool Ident_Pushed = false;
bool BBIdent_Pushed = false;
bool Common_Pushed = false;
bool Zoom_Pushed = false;
bool Zoom_Pushed_Sprite = false;

// vars for filter while importing txt frames
bool filter_time = false, filter_allmask = false, filter_color = false;
int filter_length = 15, filter_ncolor = 16;

int statusBarHeight;

bool BlocPause = true;

bool AllSameFramesUpdated = false;

// vars for extra resolution switches
bool ExtraResFClicked = false;
bool ExtraResSClicked = false;
bool ExtraResBClicked = false;
bool nEditExtraResolutionF = false;
bool nEditExtraResolutionS = false;
bool nEditExtraResolutionB = false;

float ColPick_H, ColPick_S, ColPick_V; // current position in the color picker tool
UINT8 ColPick_R8, ColPick_G8, ColPick_B8;

// these variables are used to check if the functions Predraw_XXX must be called
UINT prevAcFrame = (UINT)-1;
bool prevEditExtraResolutionF = true;
UINT16 prevAcBG = (UINT16)-1;
bool prevEditExtraResolutionB = true;

HBRUSH hActiveBrush = CreateSolidBrush(RGB(255, 0, 0)), hInactiveBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

// vars to choose which graphic filters to use to resize
int FrameResizeFilter = cv::INTER_NEAREST;
int SpriteResizeFilter = cv::INTER_NEAREST;
int BGResizeFilter = cv::INTER_NEAREST;
int ImgResizeFilter = cv::INTER_NEAREST;

#pragma endregion Global_Variables

#pragma region Undo_Tools

void UpdateURCounts(void)
{
    char tbuf[256];
    _itoa_s(UndoAvailableN, tbuf, 256, 10);
    SetDlgItemTextA(hwTB, IDC_UCOUNT, tbuf);
    SetDlgItemTextA(hwTB2, IDC_UCOUNT, tbuf);
    SetDlgItemTextA(hwTB4, IDC_UCOUNT, tbuf);
    _itoa_s(RedoAvailableN, tbuf, 256, 10);
    SetDlgItemTextA(hwTB, IDC_RCOUNT, tbuf);
    SetDlgItemTextA(hwTB2, IDC_RCOUNT, tbuf);
    SetDlgItemTextA(hwTB4, IDC_RCOUNT, tbuf);
}

void deleteFilesWithPattern(const char* pattern)
{
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    char searchPath[MAX_PATH];

    sprintf_s(searchPath, MAX_PATH, "%s", pattern);

    hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do
    {
        DeleteFileA(findData.cFileName);
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
}

void ResetUndoService()
{
    char tbuf[MAX_PATH];
    strcpy_s(tbuf, MAX_PATH, "%s*.undo");
    deleteFilesWithPattern(tbuf);
    strcpy_s(tbuf, MAX_PATH, "%s*.redo");
    deleteFilesWithPattern(tbuf);
    memset(UndoToDisk, 255, 256);
    memset(RedoToDisk, 255, 256);
    memset(UndoUsedNumber, false, 256);
    memset(RedoUsedNumber, false, 256);
    UndoAvailableN = 0;
    acUndoPos = 0;
    acRedoPos = 0;
    RedoAvailableN = 0;
    EnableWindow(GetDlgItem(hwTB, IDC_UNDO), FALSE);
    EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), FALSE);
    EnableWindow(GetDlgItem(hwTB4, IDC_UNDO), FALSE);
    EnableWindow(GetDlgItem(hwTB, IDC_REDO), FALSE);
    EnableWindow(GetDlgItem(hwTB2, IDC_REDO), FALSE);
    EnableWindow(GetDlgItem(hwTB4, IDC_REDO), FALSE);
}

HANDLE CreateSaveFile(bool isUndo, UINT8 noFile)
{
    char tpFile[MAX_PATH];
    if (isUndo) sprintf_s(tpFile, MAX_PATH, "%ssaveaction%03i.undo", temporaryDir, noFile);
    else sprintf_s(tpFile, MAX_PATH, "%ssaveaction%03i.redo", temporaryDir, noFile);
    // Attempt to delete the file
    HANDLE hFile = CreateFileA(tpFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        cprintf(true, "Unable to create the save action file, the undo content is going to be reset to avoid problems");
        ResetUndoService();
    }
    return hFile;
}

BOOL WriteSaveFile(HANDLE hFile, void* pbuffer, UINT buflen)
{
    if (hFile == INVALID_HANDLE_VALUE) return false;

    // Write to file
    DWORD written = 0;
    BOOL resw = WriteFile(hFile, pbuffer, buflen, &written, NULL);
    if (!resw || written!=buflen)
    {
        char fName[MAX_PATH];
        GetFinalPathNameByHandleA(hFile, fName, MAX_PATH, FILE_NAME_NORMALIZED);
        CloseHandle(hFile);
        DeleteFileA(fName);
        cprintf(true, "Unable to write in the save action file, the undo content is going to be reset to avoid problems");
        ResetUndoService();
    }
    return true;
}

void CloseSaveFile(HANDLE hFile, bool isUndo, UINT8 noFile, UINT ActionType)
{
    if (isUndo)
    {
        UndoToDisk[UndoAvailableN] = noFile;
        UndoActionN[UndoAvailableN] = ActionType;
        UndoLen[UndoAvailableN] = 0;
        UndoUsedNumber[noFile] = true;
        UndoAvailableN++;
    }
    else
    {
        RedoToDisk[RedoAvailableN] = noFile;
        RedoActionN[RedoAvailableN] = ActionType;
        RedoLen[RedoAvailableN] = 0;
        RedoUsedNumber[noFile] = true;
        RedoAvailableN++;
    }
    CloseHandle(hFile);
}

HANDLE OpenSaveFile(bool isUndo, UINT8 noFile)
{
    char tpFile[MAX_PATH];
    if (isUndo) sprintf_s(tpFile, MAX_PATH, "%ssaveaction%03i.undo", temporaryDir, noFile);
    else sprintf_s(tpFile, MAX_PATH, "%ssaveaction%03i.redo", temporaryDir, noFile);
    // Attempt to delete the file
    HANDLE hFile = CreateFileA(tpFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        cprintf(true, "Unable to open the save action file, the undo content is going to be reset to avoid problems");
        ResetUndoService();
    }
    return hFile;
}

BOOL ReadSaveFile(HANDLE hFile, void* pbuffer, UINT buflen)
{
    if (hFile == INVALID_HANDLE_VALUE) return false;

    // Write to file
    DWORD written = 0;
    BOOL resw = ReadFile(hFile, pbuffer, buflen, &written, NULL);
    if (!resw || written != buflen)
    {
        char fName[MAX_PATH];
        GetFinalPathNameByHandleA(hFile, fName, MAX_PATH, FILE_NAME_NORMALIZED);
        CloseHandle(hFile);
        DeleteFileA(fName);
        cprintf(true, "Unable to read from the save action file, the undo content is going to be reset to avoid problems");
        ResetUndoService();
    }
    return true;
}


UINT8 GetFreeSaveFileNumber(bool isUndo)
{
    UINT ti = 0;
    while (ti < 255)
    {
        if (isUndo && !UndoUsedNumber[ti]) return ti;
        else if (!isUndo && !RedoUsedNumber[ti]) return ti;
        ti++;
    }
    cprintf(true, "Unable to get a free slot to save action, the undo content is going to be reset to avoid problems");
    ResetUndoService();
    return ti;
}

UINT8 GetLastDiskSaveNumber(bool isUndo)
{
    if (isUndo)
    {
        if (UndoAvailableN == 0) return 255;
        return UndoToDisk[UndoAvailableN - 1];
    }
    else
    {
        if (RedoAvailableN == 0) return 255;
        return RedoToDisk[RedoAvailableN - 1];
    }
}

void DeleteSaveFile(bool isUndo, UINT8 noFile)
{
    char tpFile[MAX_PATH];
    if (isUndo)
    {
        sprintf_s(tpFile, MAX_PATH, "%ssaveaction%03i.undo", temporaryDir, noFile);
        UndoUsedNumber[noFile] = false;
    }
    else
    {
        sprintf_s(tpFile, MAX_PATH, "%ssaveaction%03i.redo", temporaryDir, noFile);
        RedoUsedNumber[noFile] = false;
    }
    DeleteFileA(tpFile);
}

void DeleteSaveFiles()
{
    for (int ti = 0; ti < 256; ti++)
    {
        if (UndoUsedNumber[ti]) DeleteSaveFile(true, ti);
        if (RedoUsedNumber[ti]) DeleteSaveFile(false, ti);
    }
}

void EraseFirstSavedActions(bool isUndo, UINT spaceNeeded)
{
    if (spaceNeeded > UNDO_REDO_BUFFER_SIZE) return;
    UINT* pAc;
    UINT8* pBuffer;
    UINT* acPos;
    UINT* allLen;
    UINT8* toDisk;
    int* pAvailable;
    if (isUndo)
    {
        pAc = UndoActionN;
        pBuffer = UndoSaveN;
        pAvailable = &UndoAvailableN;
        allLen = UndoLen;
        acPos = &acUndoPos;
        toDisk = UndoToDisk;
    }
    else
    {
        pAc = RedoActionN;
        pBuffer = RedoSaveN;
        pAvailable = &RedoAvailableN;
        allLen = RedoLen;
        acPos = &acRedoPos;
        toDisk = RedoToDisk;
    }
    UINT remmem = UNDO_REDO_BUFFER_SIZE - *acPos;
    while (remmem < spaceNeeded && (*pAvailable) > 0)
    {
        if (toDisk[0] == 255) // not saved on disk
            memcpy(pBuffer, &pBuffer[allLen[0]], *acPos - allLen[0]);
        else
            DeleteSaveFile(isUndo, toDisk[0]);
        *acPos -= allLen[0];
        remmem = UNDO_REDO_BUFFER_SIZE - *acPos;
        for (int ti = 0; ti < (*pAvailable) - 1; ti++)
        {
            pAc[ti] = pAc[ti + 1];
            allLen[ti] = allLen[ti + 1];
            toDisk[ti] = toDisk[ti + 1];
        }
        (*pAvailable)--;
    }
}
void EraseFirstSavedAction(bool isUndo)
{
    UINT* pAc;
    UINT8* pBuffer;
    UINT* acPos;
    UINT* allLen;
    UINT8* toDisk;
    int* pAvailable;
    if (isUndo)
    {
        pAc = UndoActionN;
        pBuffer = UndoSaveN;
        pAvailable = &UndoAvailableN;
        allLen = UndoLen;
        acPos = &acUndoPos;
        toDisk = UndoToDisk;
    }
    else
    {
        pAc = RedoActionN;
        pBuffer = RedoSaveN;
        pAvailable = &RedoAvailableN;
        allLen = RedoLen;
        acPos = &acRedoPos;
        toDisk = RedoToDisk;
    }
    if (toDisk[0] == 255) // not saved on disk
        memcpy(pBuffer, &pBuffer[allLen[0]], *acPos - allLen[0]);
    else
        DeleteSaveFile(isUndo, toDisk[0]);
    *acPos -= allLen[0];
    for (int ti = 0; ti < (*pAvailable) - 1; ti++)
    {
        pAc[ti] = pAc[ti + 1];
        allLen[ti] = allLen[ti + 1];
        toDisk[ti] = toDisk[ti + 1];
    }
    (*pAvailable)--;
}
UINT8* SaveGetBuffer(bool isUndo, UINT spaceNeeded)
{
    if (spaceNeeded > UNDO_REDO_BUFFER_SIZE) return NULL;
    if (isUndo)
    {
        if (UndoAvailableN == MAX_UNDO) EraseFirstSavedAction(true);
        if (spaceNeeded > UNDO_REDO_BUFFER_SIZE - acUndoPos)
            EraseFirstSavedActions(true, spaceNeeded);
        return &UndoSaveN[acUndoPos];
    }
    else
    {
        if (RedoAvailableN == MAX_UNDO) EraseFirstSavedAction(false);
        if (spaceNeeded > UNDO_REDO_BUFFER_SIZE - acRedoPos)
            EraseFirstSavedActions(false, spaceNeeded);
        return &RedoSaveN[acRedoPos];
    }
}
void SaveSetAction(bool isUndo, UINT action,UINT spaceNeeded)
{
    if (isUndo)
    {
        UndoActionN[UndoAvailableN] = action;
        UndoLen[UndoAvailableN] = spaceNeeded;
        UndoToDisk[UndoAvailableN] = 255;
        acUndoPos += spaceNeeded;
        UndoAvailableN++;
        SendMessage(hwTB, TB_ENABLEBUTTON, BM_UNDO, MAKELONG(1, 0));
    }
    else
    {
        RedoActionN[RedoAvailableN] = action;
        RedoLen[RedoAvailableN] = spaceNeeded;
        RedoToDisk[RedoAvailableN] = 255;
        acRedoPos += spaceNeeded;
        RedoAvailableN++;
        SendMessage(hwTB, TB_ENABLEBUTTON, BM_REDO, MAKELONG(1, 0));
    }
}


UINT8* RecoverGetBuffer(bool isUndo)
{
    if (isUndo)
    {
        if (acUndoPos == 0) return NULL;
        return &UndoSaveN[acUndoPos - UndoLen[UndoAvailableN - 1]];
    }
    else
    {
        if (acRedoPos == 0) return NULL;
        return &RedoSaveN[acRedoPos - RedoLen[RedoAvailableN - 1]];
    }
}
void RecoverAdjustAction(bool isUndo)
{
    if (isUndo)
    {
        UndoAvailableN--;
        acUndoPos -= UndoLen[UndoAvailableN];
        if (UndoAvailableN == 0) SendMessage(hwTB, TB_ENABLEBUTTON, BM_UNDO, MAKELONG(0, 0));
    }
    else
    {
        RedoAvailableN--;
        acRedoPos -= RedoLen[RedoAvailableN];
        if (RedoAvailableN == 0) SendMessage(hwTB, TB_ENABLEBUTTON, BM_REDO, MAKELONG(0, 0));
    }
}




// SA_DRAW:

UINT CalcSizeDrawFrames()
{
    return (nSelFrames * sizeof(UINT16) * (MycRom.fWidth * MycRom.fHeight + MycRom.fWidthX * MycRom.fHeightX));
}

void SaveDrawFrames(bool isUndo)
{
    UINT spaceNeeded = CalcSizeDrawFrames();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(pBuffer, (UINT8*)&MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
        pBuffer += MycRom.fWidth * MycRom.fHeight * sizeof(UINT16);
        memcpy(pBuffer, (UINT8*)&MycRom.cFramesX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
        pBuffer += MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16);
    }
    SaveSetAction(isUndo, SA_DRAW, spaceNeeded);
}

void RecoverDrawFrames(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy((UINT8*)&MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
        pBuffer += MycRom.fWidth * MycRom.fHeight * sizeof(UINT16);
        memcpy((UINT8*)&MycRom.cFramesX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], pBuffer, MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
        pBuffer += MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16);
    }
    InitColorRotation();
    RecoverAdjustAction(isUndo);
}

// SA_FULLDRAW:

UINT CalcSizeFullDrawFrames()
{
    return (nSelFrames * ((sizeof(UINT16) + 1) * (MycRom.fWidth * MycRom.fHeight + MycRom.fWidthX * MycRom.fHeightX)) +
        nSelFrames * (2 * sizeof(UINT16) * MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN));
}

void SaveFullDrawFrames(bool isUndo)
{
    UINT spaceNeeded = CalcSizeFullDrawFrames();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(pBuffer, &MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
        pBuffer += MycRom.fWidth * MycRom.fHeight * sizeof(UINT16);
        memcpy(pBuffer, &MycRom.cFramesX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
        pBuffer += MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16);
        memcpy(pBuffer, &MycRom.Dyna4Cols[SelFrames[ti] * MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN], MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN * sizeof(UINT16));
        pBuffer += MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN * sizeof(UINT16);
        memcpy(pBuffer, &MycRom.Dyna4ColsX[SelFrames[ti] * MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN], MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN * sizeof(UINT16));
        pBuffer += MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN * sizeof(UINT16);
        memcpy(pBuffer, &MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
        pBuffer += MycRom.fWidth * MycRom.fHeight;
        memcpy(pBuffer, &MycRom.DynaMasksX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX);
        pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    }
    SaveSetAction(isUndo, SA_FULLDRAW, spaceNeeded);
}

void RecoverFullDrawFrames(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy((UINT8*)&MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
        pBuffer += MycRom.fWidth * MycRom.fHeight * sizeof(UINT16);
        memcpy((UINT8*)&MycRom.cFramesX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], pBuffer, MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
        pBuffer += MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16);
        memcpy(&MycRom.Dyna4Cols[SelFrames[ti] * MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN], pBuffer, MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN * sizeof(UINT16));
        pBuffer += MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN * sizeof(UINT16);
        memcpy(&MycRom.Dyna4ColsX[SelFrames[ti] * MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN], pBuffer, MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN * sizeof(UINT16));
        pBuffer += MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN * sizeof(UINT16);
        memcpy(&MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight);
        pBuffer += MycRom.fWidth * MycRom.fHeight;
        memcpy(&MycRom.DynaMasksX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], pBuffer, MycRom.fWidthX * MycRom.fHeightX);
        pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    }
    InitColorRotation();
    RecoverAdjustAction(isUndo);
}

// SA_DRAWPAL:

UINT CalcSizeDrawPalFrames()
{
    return (sizeof(UINT16) * N_PALETTES * 64 + nSelFrames * sizeof(UINT16) * (MycRom.fWidth * MycRom.fHeight + MycRom.fWidthX * MycRom.fHeightX));
}

void SaveDrawPalFrames(bool isUndo)
{
    UINT spaceNeeded = CalcSizeDrawPalFrames();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(pBuffer, (UINT8*)&MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
        pBuffer += MycRom.fWidth * MycRom.fHeight * sizeof(UINT16);
        memcpy(pBuffer, (UINT8*)&MycRom.cFramesX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
        pBuffer += MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16);
    }
    memcpy(pBuffer, MycRP.Palette, sizeof(UINT16) * N_PALETTES * 64);
    SaveSetAction(isUndo, SA_DRAWPAL, spaceNeeded);
}

void RecoverDrawPalFrames(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy((UINT8*)&MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
        pBuffer += MycRom.fWidth * MycRom.fHeight * sizeof(UINT16);
        memcpy((UINT8*)&MycRom.cFramesX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], pBuffer, MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
        pBuffer += MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16);
    }
    memcpy(MycRP.Palette, pBuffer, sizeof(UINT16) * N_PALETTES * 64);
    RecoverAdjustAction(isUndo);
}


UINT CalcSizeSelection()
{
    return (2 * sizeof(UINT) + nSelFrames * sizeof(UINT));
}

void SaveSelection(bool isUndo)
{
    UINT8* pBuffer;
    if (isUndo && (UndoAvailableN > 1) && (UndoActionN[UndoAvailableN - 2] == SA_SELECTION) && (UndoActionN[UndoAvailableN - 1] == SA_SELECTION))
    {
        UndoAvailableN--;
        acUndoPos -= UndoLen[UndoAvailableN];
    }
    UINT spaceNeeded = CalcSizeSelection();
    pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    *((UINT*)pBuffer) = nSelFrames;
    pBuffer += sizeof(UINT);
    *((UINT*)pBuffer) = acFrame;
    pBuffer += sizeof(UINT);
    memcpy(pBuffer, SelFrames, nSelFrames * sizeof(UINT));
    SaveSetAction(isUndo, SA_SELECTION, spaceNeeded);
}

void RecoverSelection(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    nSelFrames = *((UINT*)pBuffer);
    pBuffer += sizeof(UINT);
    acFrame = *((UINT*)pBuffer);
    pBuffer += sizeof(UINT);
    memcpy(SelFrames, pBuffer, nSelFrames * sizeof(UINT));
    InitColorRotation();
    if (acFrame >= PreFrameInStrip + NFrameToDraw) PreFrameInStrip = acFrame - NFrameToDraw + 1;
    if ((int)acFrame < PreFrameInStrip) PreFrameInStrip = acFrame;
    SetMultiWarningF();
    UpdateNewacFrame();

    RecoverAdjustAction(isUndo);
}

// SA_DYNACOLOR:

UINT CalcSizeDynaColor()
{
    return (1 + nSelFrames * 2 * sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
}

void SaveDynaColor(bool isUndo)
{
    UINT spaceNeeded = CalcSizeDynaColor();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    *pBuffer = acDynaSet;
    pBuffer++;
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(pBuffer, (UINT8*)&MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        pBuffer += sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors;
        memcpy(pBuffer, (UINT8*)&MycRom.Dyna4ColsX[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        pBuffer += sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors;
    }
    SaveSetAction(isUndo, SA_DYNACOLOR, spaceNeeded);
}

void RecoverDynaColor(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    acDynaSet = *pBuffer;
    pBuffer++;
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy((UINT8*)&MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], pBuffer, sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        pBuffer += sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors;
        memcpy((UINT8*)&MycRom.Dyna4ColsX[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], pBuffer, sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        pBuffer += sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors;
    }
    RecoverAdjustAction(isUndo);
    InvalidateRect(hwTB, NULL, TRUE);
}

// SA_DYNAALL:

UINT CalcSizeDynaAll()
{
    return (1 + nSelFrames * (2 * sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + MycRom.fWidth * MycRom.fHeight + MycRom.fWidthX * MycRom.fHeightX));
}

void SaveDynaAll(bool isUndo)
{
    UINT spaceNeeded = CalcSizeDynaAll();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    *pBuffer = acDynaSet;
    pBuffer++;
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(pBuffer, (UINT8*)&MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        pBuffer += sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors;
        memcpy(pBuffer, (UINT8*)&MycRom.Dyna4ColsX[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        pBuffer += sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors;
        memcpy(pBuffer, &MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
        pBuffer += MycRom.fWidth * MycRom.fHeight;
        memcpy(pBuffer, &MycRom.DynaMasksX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX);
        pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    }
    SaveSetAction(isUndo, SA_DYNAALL, spaceNeeded);
}

void RecoverDynaAll(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    acDynaSet = *pBuffer;
    pBuffer++;
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy((UINT8*)&MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], pBuffer, sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        pBuffer += sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors;
        memcpy((UINT8*)&MycRom.Dyna4ColsX[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], pBuffer, sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        pBuffer += sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors;
        memcpy(&MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight);
        pBuffer += MycRom.fWidth * MycRom.fHeight;
        memcpy(&MycRom.DynaMasksX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], pBuffer, MycRom.fWidthX * MycRom.fHeightX);
        pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    }
    RecoverAdjustAction(isUndo);
    InvalidateRect(hwTB, NULL, TRUE);
}

// SA_COMPMASK:

UINT CalcSizeCompMask()
{
    return (MycRom.fWidth * MycRom.fHeight);
}

void SaveCompMask(bool isUndo)
{
    UINT spaceNeeded = CalcSizeCompMask();
    UINT8 ti = MycRom.CompMaskID[acFrame];
    if (ti == 255) return;
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    memcpy(pBuffer, &MycRom.CompMasks[ti * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
    SaveSetAction(isUndo, SA_COMPMASK, spaceNeeded);
}

void RecoverCompMask(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    UINT8 ti = MycRom.CompMaskID[acFrame];
    memcpy(&MycRom.CompMasks[ti * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight);
    RecoverAdjustAction(isUndo);
}

// SA_DURATION:

UINT CalcSizeDuration()
{
    return (nSelFrames * sizeof(UINT));
}

void SaveDuration(bool isUndo)
{
    UINT spaceNeeded = CalcSizeDuration();
    UINT* pBuffer = (UINT*)SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelFrames; ti++) pBuffer[ti] = MycRP.FrameDuration[SelFrames[ti]];
    SaveSetAction(isUndo, SA_DURATION, spaceNeeded);
}

void RecoverDuration(bool isUndo)
{
    UINT* pBuffer = (UINT*)RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) MycRP.FrameDuration[SelFrames[ti]] = pBuffer[ti];
    RecoverAdjustAction(isUndo);
}

// SA_PALETTE:

UINT CalcSizePalette()
{
    return (sizeof(UINT16) * N_PALETTES * 64);
}

void SavePalette(bool isUndo)
{
    UINT spaceNeeded = CalcSizePalette();
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    memcpy(pBuffer, MycRP.Palette, sizeof(UINT16) * N_PALETTES * 64);
    SaveSetAction(isUndo, SA_PALETTE, spaceNeeded);
}

void RecoverPalette(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(MycRP.Palette, pBuffer, sizeof(UINT16) * N_PALETTES * 64);
    RecoverAdjustAction(isUndo);
}


// SA_MASKID:

UINT CalcSizeMaskID()
{
    return (nSelFrames);
}

void SaveMaskID(bool isUndo)
{
    UINT spaceNeeded = CalcSizeMaskID();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelFrames; ti++) pBuffer[ti] = MycRom.CompMaskID[SelFrames[ti]];
    SaveSetAction(isUndo, SA_MASKID, spaceNeeded);
}

void RecoverMaskID(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) MycRom.CompMaskID[SelFrames[ti]] = pBuffer[ti];
    UpdateMaskList();
    CheckSameFrames();
    RecoverAdjustAction(isUndo);
}

// SA_ISFRAMEX:

UINT CalcSizeIsFrameX()
{
    return (nSelFrames);
}

void SaveIsFrameX(bool isUndo)
{
    UINT spaceNeeded = CalcSizeIsFrameX();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelFrames; ti++) pBuffer[ti] = MycRom.isExtraFrame[SelFrames[ti]];
    SaveSetAction(isUndo, SA_ISFRAMEX, spaceNeeded);
}

void RecoverIsFrameX(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) MycRom.isExtraFrame[SelFrames[ti]] = pBuffer[ti];
    UpdateMaskList();
    CheckSameFrames();
    UpdateFSneeded = true;
    if (ExtraResFClicked && MycRom.isExtraFrame[acFrame] > 0)
    {
        CheckDlgButton(hwTB, IDC_EXTRARES, BST_CHECKED);
        nEditExtraResolutionF = true;
    }
    else
    {
        CheckDlgButton(hwTB, IDC_EXTRARES, BST_UNCHECKED);
        nEditExtraResolutionF = false;
    }
    Calc_Resize_Frame();
    InvalidateRect(GetDlgItem(hwTB, IDC_DISPEXTRA), NULL, TRUE);
    RecoverAdjustAction(isUndo);
}

// SA_ISSPRITEX:

UINT CalcSizeIsSpriteX()
{
    return (nSelSprites);
}

void SaveIsSpriteX(bool isUndo)
{
    UINT spaceNeeded = CalcSizeIsSpriteX();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelSprites; ti++) pBuffer[ti] = MycRom.isExtraSprite[SelSprites[ti]];
    SaveSetAction(isUndo, SA_ISSPRITEX, spaceNeeded);
}

void RecoverIsSpriteX(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) MycRom.isExtraSprite[SelSprites[ti]] = pBuffer[ti];
    UpdateMaskList();
    CheckSameFrames();
    UpdateSSneeded = true;
    if (ExtraResSClicked && MycRom.isExtraSprite[acSprite] > 0)
    {
        CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED);
        nEditExtraResolutionS = true;
    }
    else
    {
        CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
        nEditExtraResolutionS = false;
    }
    Calc_Resize_Sprite();
    InvalidateRect(GetDlgItem(hwTB2, IDC_DISPEXTRA), NULL, TRUE);
    RecoverAdjustAction(isUndo);
}

// SA_ISBGX:

UINT CalcSizeIsBackgroundX()
{
    return (1);
}

void SaveIsBackgroundX(bool isUndo)
{
    UINT spaceNeeded = CalcSizeIsBackgroundX();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    pBuffer[0] = MycRom.isExtraBackground[acBG];
    SaveSetAction(isUndo, SA_ISBGX, spaceNeeded);
}

void RecoverIsBackgroundX(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    MycRom.isExtraBackground[acBG] = pBuffer[0];
    UpdateMaskList();
    CheckSameFrames();
    UpdateBSneeded = true;
    if (ExtraResBClicked && MycRom.isExtraBackground[acBG] > 0)
    {
        CheckDlgButton(hwTB4, IDC_EXTRARES, BST_CHECKED);
        nEditExtraResolutionB = true;
    }
    else
    {
        CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
        nEditExtraResolutionB = false;
    }
    Calc_Resize_BG();
    InvalidateRect(GetDlgItem(hwTB4, IDC_DISPEXTRA), NULL, TRUE);
    RecoverAdjustAction(isUndo);
}

// SA_SHAPEMODE:

UINT CalcSizeShapeMode()
{
    return (nSelFrames);
}

void SaveShapeMode(bool isUndo)
{
    UINT spaceNeeded = CalcSizeShapeMode();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelFrames; ti++) pBuffer[ti] = MycRom.ShapeCompMode[SelFrames[ti]];
    SaveSetAction(isUndo, SA_SHAPEMODE, spaceNeeded);
}

void RecoverShapeMode(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) MycRom.ShapeCompMode[SelFrames[ti]] = pBuffer[ti];
    if (MycRom.ShapeCompMode[acFrame] == TRUE) Button_SetCheck(GetDlgItem(hwTB, IDC_SHAPEMODE), BST_CHECKED); else Button_SetCheck(GetDlgItem(hwTB, IDC_SHAPEMODE), BST_UNCHECKED);
    CheckSameFrames();
    RecoverAdjustAction(isUndo);
}

// SA_COLSETS:

UINT CalcSizeColorSets()
{
    return (sizeof(UINT16) * MAX_COL_SETS * 16 + MAX_COL_SETS * 64 + MAX_COL_SETS * sizeof(BOOL));
}

void SaveColorSets(bool isUndo)
{
    UINT spaceNeeded = CalcSizeColorSets();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    memcpy(pBuffer, MycRP.ColSets, sizeof(UINT16) * MAX_COL_SETS * 16); // copy the col sets
    memcpy(&pBuffer[sizeof(UINT16) * MAX_COL_SETS * 16], MycRP.nameColSet, MAX_COL_SETS * 64); // copy their names
    memcpy(&pBuffer[MAX_COL_SETS * (sizeof(UINT16) * 16 + 64)], MycRP.activeColSet, MAX_COL_SETS * sizeof(BOOL)); // copy the active one
    SaveSetAction(isUndo, SA_COLSETS, spaceNeeded);
}

void RecoverColorSets(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(MycRP.ColSets, pBuffer, sizeof(UINT16) * MAX_COL_SETS * 16);
    memcpy(MycRP.nameColSet, &pBuffer[sizeof(UINT16) * MAX_COL_SETS * 16], MAX_COL_SETS * 64);
    memcpy(MycRP.activeColSet, &pBuffer[MAX_COL_SETS * (sizeof(UINT16) * 16 + 64)], MAX_COL_SETS * sizeof(BOOL));
    RecoverAdjustAction(isUndo);
}

// 

// SA_COPYMASK

UINT CalcSizeCopyMask()
{
    return (10 * 256 * 64 + 2 * sizeof(UINT) + 5 * sizeof(int) + 3 * sizeof(bool));
}

void SaveCopyMask(bool isUndo)
{
    UINT spaceNeeded = CalcSizeCopyMask();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    memcpy(pBuffer, Copy_Mask, 256 * 64);
    pBuffer += 256 * 64;
    memcpy(pBuffer, Copy_ColN, 256 * 64 * sizeof(UINT16));
    pBuffer += 256 * 64 * sizeof(UINT16);
    memcpy(pBuffer, Copy_Colo, 256 * 64);
    pBuffer += 256 * 64;
    memcpy(pBuffer, Copy_Dyna, 256 * 64);
    pBuffer += 256 * 64;
    *(UINT*)pBuffer = Paste_Width;
    pBuffer += sizeof(UINT);
    *(UINT*)pBuffer = Paste_Height;
    pBuffer += sizeof(UINT);
    memcpy(pBuffer, Paste_Mask, 256 * 64);
    pBuffer += 256 * 64;
    memcpy(pBuffer, Paste_ColN, 256 * 64 * sizeof(UINT16));
    pBuffer += 256 * 64 * sizeof(UINT16);
    memcpy(pBuffer, Paste_Colo, 256 * 64);
    pBuffer += 256 * 64;
    memcpy(pBuffer, Paste_Dyna, 256 * 64);
    pBuffer += 256 * 64;
    *(int*)pBuffer = Copy_From_Frame;
    pBuffer += sizeof(int);
    *(int*)pBuffer = Copy_From_DynaMask;
    pBuffer += sizeof(int);
    *(bool*)pBuffer = Copy_Mode;
    pBuffer += sizeof(bool);
    *(bool*)pBuffer = Copy_Available;
    pBuffer += sizeof(bool);
    *(int*)pBuffer = paste_offsetx;
    pBuffer += sizeof(int);
    *(int*)pBuffer = paste_offsety;
    pBuffer += sizeof(int);
    /**(int*)pBuffer = paste_centerx;
    pBuffer += sizeof(int);
    *(int*)pBuffer = paste_centery;
    pBuffer += sizeof(int);*/
    *(bool*)pBuffer = Paste_Mode;
    pBuffer += sizeof(bool);
    *(int*)pBuffer = Paste_Mirror;
    pBuffer += sizeof(int);
    SaveSetAction(isUndo, SA_COPYMASK, spaceNeeded);
}

void RecoverCopyMask(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(Copy_Mask, pBuffer, 256 * 64);
    pBuffer += 256 * 64;
    memcpy(Copy_ColN, pBuffer, 256 * 64 * sizeof(UINT16));
    pBuffer += 256 * 64 * sizeof(UINT16);
    memcpy(Copy_Colo, pBuffer, 256 * 64);
    pBuffer += 256 * 64;
    memcpy(Copy_Dyna, pBuffer, 256 * 64);
    pBuffer += 256 * 64;
    Paste_Width = *(UINT*)pBuffer;
    pBuffer += sizeof(UINT);
    Paste_Height = *(UINT*)pBuffer;
    pBuffer += sizeof(UINT);
    memcpy(Paste_Mask, pBuffer, 256 * 64);
    pBuffer += 256 * 64;
    memcpy(Paste_ColN, pBuffer, 256 * 64 * sizeof(UINT16));
    pBuffer += 256 * 64 * sizeof(UINT16);
    memcpy(Paste_Colo, pBuffer, 256 * 64);
    pBuffer += 256 * 64;
    memcpy(Paste_Dyna, pBuffer, 256 * 64);
    pBuffer += 256 * 64;
    Copy_From_Frame = *(int*)pBuffer;
    pBuffer += sizeof(int);
    Copy_From_DynaMask = *(int*)pBuffer;
    pBuffer += sizeof(int);
    Copy_Mode = *(bool*)pBuffer;
    pBuffer += sizeof(bool);
    Copy_Available = *(bool*)pBuffer;
    pBuffer += sizeof(bool);
    paste_offsetx = *(int*)pBuffer;
    pBuffer += sizeof(int);
    paste_offsety = *(int*)pBuffer;
    pBuffer += sizeof(int);
    /*paste_centerx = *(int*)pBuffer;
    pBuffer += sizeof(int);
    paste_centery = *(int*)pBuffer;
    pBuffer += sizeof(int);*/
    Paste_Mode = *(bool*)pBuffer;
    pBuffer += sizeof(bool);
    Paste_Mirror = *(int*)pBuffer;
    pBuffer += sizeof(int);
    GetSelectionSize();
    RecoverAdjustAction(isUndo);
}

// SA_DYNAMASK:

UINT CalcSizeDynaMask()
{
    return (nSelFrames * (MycRom.fWidth * MycRom.fHeight + MycRom.fWidthX * MycRom.fHeightX));
}

void SaveDynaMask(bool isUndo)
{
    UINT spaceNeeded = CalcSizeDynaMask();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT32 ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(pBuffer, &MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
        pBuffer += MycRom.fWidth * MycRom.fHeight;
        memcpy(pBuffer, &MycRom.DynaMasksX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX);
        pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    }
    SaveSetAction(isUndo, SA_DYNAMASK, spaceNeeded);
}

void RecoverDynaMask(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT32 ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(&MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight);
        pBuffer += MycRom.fWidth * MycRom.fHeight;
        memcpy(&MycRom.DynaMasksX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], pBuffer, MycRom.fWidthX * MycRom.fHeightX);
        pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    }
    RecoverAdjustAction(isUndo);
}

/*
void SaveSpriteCol(bool isUndo)
{
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    memcpy(pBuffer, &MycRP.Sprite_Edit_Colors[acSprite * 16], 16);
    SaveSetAction(isUndo, SA_SPRITECOLOR);
}

void RecoverSpriteCol(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(&MycRP.Sprite_Edit_Colors[acSprite * 16], pBuffer, 16);
    RecoverAdjustAction(isUndo);
}
*/

// SA_FRAMESPRITES:

UINT CalcSizeFrameSprites()
{
    return (nSelFrames * MAX_SPRITES_PER_FRAME * (1 + sizeof(UINT16) * 4));
}

void SaveFrameSprites(bool isUndo)
{
    UINT spaceNeeded = CalcSizeFrameSprites();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(pBuffer, &MycRom.FrameSprites[SelFrames[ti] * MAX_SPRITES_PER_FRAME], MAX_SPRITES_PER_FRAME);
        pBuffer += MAX_SPRITES_PER_FRAME;
        memcpy(pBuffer, &MycRom.FrameSpriteBB[SelFrames[ti] * MAX_SPRITES_PER_FRAME * 4], MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16));
        pBuffer += MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16);
    }
    SaveSetAction(isUndo, SA_FRAMESPRITES, spaceNeeded);
}

void RecoverFrameSprites(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(&MycRom.FrameSprites[SelFrames[ti] * MAX_SPRITES_PER_FRAME], pBuffer, MAX_SPRITES_PER_FRAME);
        pBuffer += MAX_SPRITES_PER_FRAME;
        memcpy(&MycRom.FrameSpriteBB[SelFrames[ti] * MAX_SPRITES_PER_FRAME * 4], pBuffer, MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16));
        pBuffer += MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16);
    }
    RecoverAdjustAction(isUndo);
}

// SA_ACSPRITE:

UINT CalcSizeAcSprite()
{
    return (2 * sizeof(UINT) + nSelSprites * sizeof(int));
}

void SaveAcSprite(bool isUndo)
{
    UINT8* pBuffer;
    if (isUndo && (UndoAvailableN > 1) && (UndoActionN[UndoAvailableN - 2] == SA_ACSPRITE) && (UndoActionN[UndoAvailableN - 1] == SA_ACSPRITE))
    {
        UndoAvailableN--;
        acUndoPos -= UndoLen[UndoAvailableN];
    }
    UINT spaceNeeded = CalcSizeAcSprite();
    pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    *((UINT*)pBuffer) = nSelSprites;
    *((UINT*)(&pBuffer[sizeof(UINT)])) = acSprite;
    memcpy(&pBuffer[2 * sizeof(UINT)], SelSprites, nSelSprites * sizeof(int));
    SaveSetAction(isUndo, SA_ACSPRITE, spaceNeeded);
}

void RecoverAcSprite(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    nSelSprites = *((UINT*)pBuffer);
    acSprite = *((UINT*)(&pBuffer[sizeof(UINT)]));
    if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
    memcpy(SelSprites, &pBuffer[2 * sizeof(UINT)], nSelSprites * sizeof(int));
    RecoverAdjustAction(isUndo);
}


UINT CalcSizeSprite()
{
    return (1 + MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * 6 + 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16) + MAX_SPRITE_DETECT_AREAS * sizeof(UINT16) + MAX_SPRITE_DETECT_AREAS * sizeof(UINT32));
}

void SaveSprite(bool isUndo)
{
    UINT spaceNeeded = CalcSizeSprite();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    *pBuffer = MycRom.isExtraSprite[acSprite];
    pBuffer++;
    memcpy(pBuffer, &MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT;
    memcpy(pBuffer, &MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16);
    memcpy(pBuffer, &MycRom.SpriteMaskX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT;
    memcpy(pBuffer, &MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16);
    memcpy(pBuffer, &MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS], 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    pBuffer += 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16);
    memcpy(pBuffer, &MycRom.SpriteDetDwordPos[acSprite * MAX_SPRITE_DETECT_AREAS], MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    pBuffer += MAX_SPRITE_DETECT_AREAS * sizeof(UINT16);
    memcpy(pBuffer, &MycRom.SpriteDetDwords[acSprite * MAX_SPRITE_DETECT_AREAS], MAX_SPRITE_DETECT_AREAS * sizeof(UINT32));
    pBuffer += MAX_SPRITE_DETECT_AREAS * sizeof(UINT32);
    SaveSetAction(isUndo, SA_SPRITE, spaceNeeded);
}

void RecoverSprite(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    MycRom.isExtraSprite[acSprite] = *pBuffer;
    pBuffer++;
    memcpy(&MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], pBuffer, MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT;
    memcpy(&MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], pBuffer, MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16);
    memcpy(&MycRom.SpriteMaskX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], pBuffer, MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT;
    memcpy(&MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], pBuffer, MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16);
    memcpy(&MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS], pBuffer, 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    pBuffer += 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16);
    memcpy(&MycRom.SpriteDetDwordPos[acSprite * MAX_SPRITE_DETECT_AREAS], pBuffer, MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    pBuffer += MAX_SPRITE_DETECT_AREAS * sizeof(UINT16);
    memcpy(&MycRom.SpriteDetDwords[acSprite * MAX_SPRITE_DETECT_AREAS], pBuffer, MAX_SPRITE_DETECT_AREAS * sizeof(UINT32));
    pBuffer += MAX_SPRITE_DETECT_AREAS * sizeof(UINT32);
    RecoverAdjustAction(isUndo);
}

// SA_SPRITEDETAREAS:

UINT CalcSizeSpriteDetAreas()
{
    return (4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16) + MAX_SPRITE_DETECT_AREAS * sizeof(UINT16) + MAX_SPRITE_DETECT_AREAS * sizeof(UINT32));
}

void SaveSpriteDetAreas(bool isUndo)
{
    UINT spaceNeeded = CalcSizeSpriteDetAreas();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    memcpy(pBuffer, &MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS], 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    pBuffer += 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16);
    memcpy(pBuffer, &MycRom.SpriteDetDwordPos[acSprite * MAX_SPRITE_DETECT_AREAS], MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    pBuffer += MAX_SPRITE_DETECT_AREAS * sizeof(UINT16);
    memcpy(pBuffer, &MycRom.SpriteDetDwords[acSprite * MAX_SPRITE_DETECT_AREAS], MAX_SPRITE_DETECT_AREAS * sizeof(UINT32));
    pBuffer += MAX_SPRITE_DETECT_AREAS * sizeof(UINT32);
    SaveSetAction(isUndo, SA_SPRITEDETAREAS, spaceNeeded);
}

void RecoverSpriteDetAreas(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(&MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS], pBuffer, 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    pBuffer += 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16);
    memcpy(&MycRom.SpriteDetDwordPos[acSprite * MAX_SPRITE_DETECT_AREAS], pBuffer, MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    pBuffer += MAX_SPRITE_DETECT_AREAS * sizeof(UINT16);
    memcpy(&MycRom.SpriteDetDwords[acSprite * MAX_SPRITE_DETECT_AREAS], pBuffer, MAX_SPRITE_DETECT_AREAS * sizeof(UINT32));
    pBuffer += MAX_SPRITE_DETECT_AREAS * sizeof(UINT32);
    RecoverAdjustAction(isUndo);
}

// SA_SPRITEDRAW:

UINT CalcSizeDrawSprite()
{
    return (2 * nSelSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
}

void SaveDrawSprite(bool isUndo)
{
    UINT spaceNeeded = CalcSizeDrawSprite();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelSprites; ti++)
    {
        memcpy(pBuffer, &MycRom.SpriteColored[SelSprites[ti] * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
        pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16);
        memcpy(pBuffer, &MycRom.SpriteColoredX[SelSprites[ti] * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
        pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16);
    }
    SaveSetAction(isUndo, SA_SPRITEDRAW, spaceNeeded);
}

void RecoverDrawSprite(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelSprites; ti++)
    {
        memcpy(&MycRom.SpriteColored[SelSprites[ti] * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], pBuffer, MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
        pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16);
        memcpy(&MycRom.SpriteColoredX[SelSprites[ti] * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], pBuffer, MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
        pBuffer += MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16);
    }
    UpdateSSneeded = true;
    RecoverAdjustAction(isUndo);
}

// SA_COLROT:

UINT CalcSizeColRot()
{
    return (nSelFrames * 2 * sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN);
}

void SaveColRot(bool isUndo)
{
    UINT spaceNeeded = CalcSizeColRot();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT32 ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(pBuffer, &MycRom.ColorRotations[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN);
        pBuffer += sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN;
        memcpy(pBuffer, &MycRom.ColorRotationsX[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN);
        pBuffer += sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN;
    }
    SaveSetAction(isUndo, SA_COLROT, spaceNeeded);
}

void RecoverColRot(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT32 ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(&MycRom.ColorRotations[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], pBuffer, sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN);
        pBuffer += sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN;
        memcpy(&MycRom.ColorRotationsX[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], pBuffer, sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN);
        pBuffer += sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN;
    }
    InitColorRotation();
    RecoverAdjustAction(isUndo);
}

// SA_SPRITES: Save ALL the sprites, use SA_SPRITE for just 1

void SaveSprites(bool isUndo)
{
    UINT noSave = GetFreeSaveFileNumber(isUndo);
    if (noSave == 255) return;
    HANDLE hSave = CreateSaveFile(isUndo, noSave);
    if (hSave == INVALID_HANDLE_VALUE) return;
    WriteSaveFile(hSave, &MycRom.nSprites, sizeof(UINT));
    WriteSaveFile(hSave, MycRom.isExtraSprite, MycRom.nSprites);
    WriteSaveFile(hSave, MycRom.SpriteOriginal, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    WriteSaveFile(hSave, MycRom.SpriteColored, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    WriteSaveFile(hSave, MycRom.SpriteMaskX, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    WriteSaveFile(hSave, MycRom.SpriteColoredX, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    WriteSaveFile(hSave, MycRom.SpriteDetAreas, sizeof(UINT16) * MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS);
    WriteSaveFile(hSave, MycRom.SpriteDetDwordPos, sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_DETECT_AREAS);
    WriteSaveFile(hSave, MycRom.SpriteDetDwords, sizeof(UINT32) * MycRom.nSprites * MAX_SPRITE_DETECT_AREAS);
    WriteSaveFile(hSave, MycRom.FrameSprites, MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    WriteSaveFile(hSave, MycRom.FrameSpriteBB, 4 * sizeof(UINT16) * MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    WriteSaveFile(hSave, MycRP.Sprite_Names, 255 * SIZE_SECTION_NAMES);
    WriteSaveFile(hSave, MycRP.Sprite_Col_From_Frame, 255 * sizeof(UINT));
    WriteSaveFile(hSave, MycRP.SpriteRect, 2 * 4 * 255);
    WriteSaveFile(hSave, MycRP.SpriteRectMirror, sizeof(BOOL) * 2 * 255);
    WriteSaveFile(hSave, &acSprite, sizeof(UINT));
    CloseSaveFile(hSave, isUndo, noSave, SA_SPRITES);
}

void RecoverSprites(bool isUndo)
{
    UINT noSave = GetLastDiskSaveNumber(isUndo);
    if (noSave == 255) return;
    HANDLE hSave = OpenSaveFile(isUndo, noSave);
    if (hSave == INVALID_HANDLE_VALUE) return;
    UINT tnSprites = MycRom.nSprites;
    ReadSaveFile(hSave, &MycRom.nSprites, sizeof(UINT));
    if (tnSprites != MycRom.nSprites)
    {
        MycRom.isExtraSprite = (UINT8*)realloc(MycRom.isExtraSprite, MycRom.nSprites);
        MycRom.SpriteOriginal = (UINT8*)realloc(MycRom.SpriteOriginal, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
        MycRom.SpriteColored = (UINT16*)realloc(MycRom.SpriteColored, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
        MycRom.SpriteMaskX = (UINT8*)realloc(MycRom.SpriteMaskX, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
        MycRom.SpriteColoredX = (UINT16*)realloc(MycRom.SpriteColoredX, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
        MycRom.SpriteDetAreas = (UINT16*)realloc(MycRom.SpriteDetAreas, sizeof(UINT16) * MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS);
        MycRom.SpriteDetDwordPos = (UINT16*)realloc(MycRom.SpriteDetDwordPos, sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_DETECT_AREAS);
        MycRom.SpriteDetDwords = (UINT32*)realloc(MycRom.SpriteDetDwords, sizeof(UINT32) * MycRom.nSprites * MAX_SPRITE_DETECT_AREAS);
        MycRom.FrameSprites = (UINT8*)realloc(MycRom.FrameSprites, MycRom.nFrames * MAX_SPRITES_PER_FRAME);
        MycRom.FrameSpriteBB = (UINT16*)realloc(MycRom.FrameSpriteBB, 4 * sizeof(UINT16) * MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    }
    ReadSaveFile(hSave, MycRom.isExtraSprite, MycRom.nSprites);
    ReadSaveFile(hSave, MycRom.SpriteOriginal, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    ReadSaveFile(hSave, MycRom.SpriteColored, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    ReadSaveFile(hSave, MycRom.SpriteMaskX, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    ReadSaveFile(hSave, MycRom.SpriteColoredX, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    ReadSaveFile(hSave, MycRom.SpriteDetAreas, sizeof(UINT16) * MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS);
    ReadSaveFile(hSave, MycRom.SpriteDetDwordPos, sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_DETECT_AREAS);
    ReadSaveFile(hSave, MycRom.SpriteDetDwords, sizeof(UINT32) * MycRom.nSprites * MAX_SPRITE_DETECT_AREAS);
    ReadSaveFile(hSave, MycRom.FrameSprites, MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    ReadSaveFile(hSave, MycRom.FrameSpriteBB, 4 * sizeof(UINT16) * MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    ReadSaveFile(hSave, MycRP.Sprite_Names, 255 * SIZE_SECTION_NAMES);
    ReadSaveFile(hSave, MycRP.Sprite_Col_From_Frame, 255 * sizeof(UINT));
    ReadSaveFile(hSave, MycRP.SpriteRect, sizeof(UINT16) * 4 * 255);
    ReadSaveFile(hSave, MycRP.SpriteRectMirror, sizeof(BOOL) * 2 * 255);
    ReadSaveFile(hSave, &acSprite, sizeof(UINT));
    CloseHandle(hSave);
    RecoverAdjustAction(isUndo);
}

// SA_FRAMES: Save ALL the frames

void SaveFrames(bool isUndo)
{
    UINT noSave = GetFreeSaveFileNumber(isUndo);
    if (noSave == 255) return;
    HANDLE hSave = CreateSaveFile(isUndo, noSave);
    if (hSave == INVALID_HANDLE_VALUE) return;
    WriteSaveFile(hSave, &MycRom.nFrames, sizeof(UINT));
    WriteSaveFile(hSave, MycRom.HashCode, MycRom.nFrames * sizeof(UINT));
    WriteSaveFile(hSave, MycRom.CompMaskID, MycRom.nFrames);
    WriteSaveFile(hSave, MycRom.ShapeCompMode, MycRom.nFrames);
    WriteSaveFile(hSave, MycRom.isExtraFrame, MycRom.nFrames);
    WriteSaveFile(hSave, MycRom.cFrames, sizeof(UINT16) * MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    WriteSaveFile(hSave, MycRom.cFramesX, sizeof(UINT16) * MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    WriteSaveFile(hSave, MycRom.DynaMasks, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    WriteSaveFile(hSave, MycRom.DynaMasksX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    WriteSaveFile(hSave, MycRom.Dyna4Cols, sizeof(UINT16) * MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
    WriteSaveFile(hSave, MycRom.Dyna4ColsX, sizeof(UINT16) * MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
    WriteSaveFile(hSave, MycRom.FrameSprites, MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    WriteSaveFile(hSave, MycRom.FrameSpriteBB, 4 * sizeof(UINT16) * MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    WriteSaveFile(hSave, MycRom.ColorRotations, sizeof(UINT16) * MycRom.nFrames * MAX_COLOR_ROTATIONN*MAX_LENGTH_COLOR_ROTATION);
    WriteSaveFile(hSave, MycRom.ColorRotationsX, sizeof(UINT16) * MycRom.nFrames * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
    WriteSaveFile(hSave, MycRom.TriggerID, MycRom.nFrames * sizeof(UINT));
    WriteSaveFile(hSave, MycRom.BackgroundID, MycRom.nFrames * sizeof(UINT16));
    WriteSaveFile(hSave, MycRom.BackgroundMask, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    WriteSaveFile(hSave, MycRom.BackgroundMaskX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);

    WriteSaveFile(hSave, MycRP.oFrames, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    WriteSaveFile(hSave, MycRP.Sprite_Col_From_Frame, 255 * sizeof(UINT));
    WriteSaveFile(hSave, MycRP.FrameDuration, sizeof(UINT) * MycRom.nFrames);
    WriteSaveFile(hSave, &acFrame, sizeof(UINT));
    CloseSaveFile(hSave, isUndo, noSave, SA_FRAMES);
}

void RecoverFrames(bool isUndo)
{
    UINT noSave = GetLastDiskSaveNumber(isUndo);
    if (noSave == 255) return;
    HANDLE hSave = OpenSaveFile(isUndo, noSave);
    if (hSave == INVALID_HANDLE_VALUE) return;
    UINT tnFrames = MycRom.nFrames;
    ReadSaveFile(hSave, &MycRom.nFrames, sizeof(UINT));
    if (tnFrames != MycRom.nFrames)
    {
        MycRom.HashCode = (UINT*)realloc(MycRom.HashCode, sizeof(UINT) * MycRom.nFrames);
        MycRom.CompMaskID = (UINT8*)realloc(MycRom.CompMaskID, MycRom.nFrames);
        MycRom.ShapeCompMode = (UINT8*)realloc(MycRom.ShapeCompMode, MycRom.nFrames);
        MycRom.isExtraFrame = (UINT8*)realloc(MycRom.isExtraFrame, MycRom.nFrames);
        MycRom.cFrames = (UINT16*)realloc(MycRom.cFrames, sizeof(UINT16) * MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
        MycRom.cFramesX = (UINT16*)realloc(MycRom.cFramesX, sizeof(UINT16) * MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
        MycRom.DynaMasks = (UINT8*)realloc(MycRom.DynaMasks, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
        MycRom.DynaMasksX = (UINT8*)realloc(MycRom.DynaMasksX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
        MycRom.Dyna4Cols = (UINT16*)realloc(MycRom.Dyna4Cols, sizeof(UINT16) * MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        MycRom.Dyna4ColsX = (UINT16*)realloc(MycRom.Dyna4ColsX, sizeof(UINT16) * MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        MycRom.FrameSprites = (UINT8*)realloc(MycRom.FrameSprites, MycRom.nFrames * MAX_SPRITES_PER_FRAME);
        MycRom.FrameSpriteBB = (UINT16*)realloc(MycRom.FrameSpriteBB, 4 * sizeof(UINT16) * MycRom.nFrames * MAX_SPRITES_PER_FRAME);
        MycRom.ColorRotations = (UINT16*)realloc(MycRom.ColorRotations, sizeof(UINT16) * MycRom.nFrames * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
        MycRom.ColorRotationsX = (UINT16*)realloc(MycRom.ColorRotationsX, sizeof(UINT16) * MycRom.nFrames * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
        MycRom.TriggerID = (UINT*)realloc(MycRom.TriggerID, sizeof(UINT) * MycRom.nFrames);
        MycRom.BackgroundID = (UINT16*)realloc(MycRom.BackgroundID, sizeof(UINT16) * MycRom.nFrames);
        MycRom.BackgroundMask = (UINT8*)realloc(MycRom.BackgroundMask, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
        MycRom.BackgroundMaskX = (UINT8*)realloc(MycRom.BackgroundMaskX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
        MycRP.oFrames = (UINT8*)realloc(MycRP.oFrames, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
        MycRP.FrameDuration = (UINT*)realloc(MycRP.FrameDuration, sizeof(UINT) * MycRom.nFrames);
    }
    ReadSaveFile(hSave, MycRom.HashCode, MycRom.nFrames * sizeof(UINT));
    ReadSaveFile(hSave, MycRom.CompMaskID, MycRom.nFrames);
    ReadSaveFile(hSave, MycRom.ShapeCompMode, MycRom.nFrames);
    ReadSaveFile(hSave, MycRom.isExtraFrame, MycRom.nFrames);
    ReadSaveFile(hSave, MycRom.cFrames, sizeof(UINT16) * MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    ReadSaveFile(hSave, MycRom.cFramesX, sizeof(UINT16) * MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    ReadSaveFile(hSave, MycRom.DynaMasks, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    ReadSaveFile(hSave, MycRom.DynaMasksX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    ReadSaveFile(hSave, MycRom.Dyna4Cols, sizeof(UINT16) * MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
    ReadSaveFile(hSave, MycRom.Dyna4ColsX, sizeof(UINT16) * MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
    ReadSaveFile(hSave, MycRom.FrameSprites, MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    ReadSaveFile(hSave, MycRom.FrameSpriteBB, 4 * 2 * MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    ReadSaveFile(hSave, MycRom.ColorRotations, sizeof(UINT16) * MycRom.nFrames * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
    ReadSaveFile(hSave, MycRom.ColorRotationsX, sizeof(UINT16) * MycRom.nFrames * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
    ReadSaveFile(hSave, MycRom.TriggerID, MycRom.nFrames * sizeof(UINT));
    ReadSaveFile(hSave, MycRom.BackgroundID, MycRom.nFrames * sizeof(UINT16));
    ReadSaveFile(hSave, MycRom.BackgroundMask, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    ReadSaveFile(hSave, MycRom.BackgroundMaskX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);

    ReadSaveFile(hSave, MycRP.oFrames, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    ReadSaveFile(hSave, MycRP.Sprite_Col_From_Frame, 255 * sizeof(UINT));
    ReadSaveFile(hSave, MycRP.FrameDuration, sizeof(UINT) * MycRom.nFrames);
    ReadSaveFile(hSave, &acFrame, sizeof(UINT));
    CloseHandle(hSave);
    RecoverAdjustAction(isUndo);
}

// SA_SECTIONS:

UINT CalcSizeSections()
{
    return (sizeof(UINT32) + MAX_SECTIONS * SIZE_SECTION_NAMES + sizeof(UINT32) * MAX_SECTIONS);
}

void SaveSections(bool isUndo)
{
    UINT spaceNeeded = CalcSizeSections();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    *((UINT32*)pBuffer) = MycRP.nSections;
    memcpy(&pBuffer[4], MycRP.Section_Names, MAX_SECTIONS * SIZE_SECTION_NAMES);
    memcpy(&pBuffer[4 + MAX_SECTIONS * SIZE_SECTION_NAMES], MycRP.Section_Firsts, sizeof(UINT32) * MAX_SECTIONS);
    SaveSetAction(isUndo, SA_SECTIONS, spaceNeeded);
}

void RecoverSections(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    MycRP.nSections = *((UINT32*)pBuffer);
    memcpy(MycRP.Section_Names, &pBuffer[4], MAX_SECTIONS * SIZE_SECTION_NAMES);
    memcpy(MycRP.Section_Firsts, &pBuffer[4 + MAX_SECTIONS * SIZE_SECTION_NAMES], sizeof(UINT32) * MAX_SECTIONS);
    RecoverAdjustAction(isUndo);
    UpdateSectionList();
}

// SA_SETBACKGROUND:

UINT CalcSizeSetBackground()
{
    return (nSelFrames * (sizeof(UINT16) + MycRom.fWidth * MycRom.fHeight + MycRom.fWidthX * MycRom.fHeightX));
}

void SaveSetBackground(bool isUndo)
{
    UINT spaceNeeded = CalcSizeSetBackground();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT32 ti = 0; ti < nSelFrames; ti++)
    {
        *((UINT16*)pBuffer) = MycRom.BackgroundID[SelFrames[ti]];
        pBuffer += sizeof(UINT16);
        memcpy(pBuffer, &MycRom.BackgroundMask[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
        pBuffer += MycRom.fWidth * MycRom.fHeight;
        memcpy(pBuffer, &MycRom.BackgroundMaskX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX);
        pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    }
    SaveSetAction(isUndo, SA_SETBACKGROUND, spaceNeeded);
}

void RecoverSetBackground(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT32 ti = 0; ti < nSelFrames; ti++)
    {
        MycRom.BackgroundID[SelFrames[ti]] = *((UINT16*)pBuffer);
        pBuffer += sizeof(UINT16);
        memcpy(&MycRom.BackgroundMask[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight);
        pBuffer += MycRom.fWidth * MycRom.fHeight;
        memcpy(&MycRom.BackgroundMaskX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], pBuffer, MycRom.fWidthX * MycRom.fHeightX);
        pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    }
    RecoverAdjustAction(isUndo);
}

// SA_COPYBACKGROUND:

UINT CalcSizeCopyBackground()
{
    return (3 * MycRom.fWidth * MycRom.fHeight + 3 * MycRom.fWidthX * MycRom.fHeightX + 1);
}

void SaveCopyBackground(bool isUndo)
{
    UINT spaceNeeded = CalcSizeCopyBackground();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    memcpy(pBuffer, &MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
    pBuffer += MycRom.fWidth * MycRom.fHeight;
    memcpy(pBuffer, &MycRom.DynaMasksX[acFrame * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX);
    pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    memcpy(pBuffer, &MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    pBuffer += MycRom.fWidth * MycRom.fHeight * sizeof(UINT16);
    memcpy(pBuffer, &MycRom.cFramesX[acFrame * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    pBuffer += MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16);
    memcpy(pBuffer, &MycRom.isExtraFrame[acFrame], 1);
    pBuffer += 1;
    SaveSetAction(isUndo, SA_COPYBACKGROUND, spaceNeeded);
}

void RecoverCopyBackground(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(&MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight);
    pBuffer += MycRom.fWidth * MycRom.fHeight;
    memcpy(&MycRom.DynaMasksX[acFrame * MycRom.fWidthX * MycRom.fHeightX], pBuffer, MycRom.fWidthX * MycRom.fHeightX);
    pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    memcpy(&MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight);
    pBuffer += MycRom.fWidth * MycRom.fHeight;
    memcpy(&MycRom.cFramesX[acFrame * MycRom.fWidthX * MycRom.fHeightX], pBuffer, MycRom.fWidthX * MycRom.fHeightX);
    pBuffer += MycRom.fWidthX * MycRom.fHeightX;
    memcpy(&MycRom.isExtraFrame[acFrame], pBuffer, 1);
    pBuffer += 1;
    RecoverAdjustAction(isUndo);
}

// SA_DELBACKGROUND: Save all the backgrounds

void SaveDelBackground(bool isUndo)
{
    UINT noSave = GetFreeSaveFileNumber(isUndo);
    if (noSave == 255) return;
    HANDLE hSave = CreateSaveFile(isUndo, noSave);
    if (hSave == INVALID_HANDLE_VALUE) return;
    WriteSaveFile(hSave, &MycRom.nBackgrounds, sizeof(UINT));
    WriteSaveFile(hSave, MycRom.isExtraBackground, MycRom.nBackgrounds);
    WriteSaveFile(hSave, MycRom.BackgroundFrames, MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    WriteSaveFile(hSave, MycRom.BackgroundFramesX, MycRom.nBackgrounds * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    WriteSaveFile(hSave, MycRom.BackgroundMask, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    WriteSaveFile(hSave, MycRom.BackgroundMaskX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    WriteSaveFile(hSave, MycRom.BackgroundID, MycRom.nFrames * sizeof(UINT16));
    WriteSaveFile(hSave, &acBG, sizeof(UINT));
    CloseSaveFile(hSave, isUndo, noSave, SA_DELBACKGROUND);
}

void RecoverDelBackground(bool isUndo)
{
    UINT noSave = GetLastDiskSaveNumber(isUndo);
    if (noSave == 255) return;
    HANDLE hSave = OpenSaveFile(isUndo, noSave);
    if (hSave == INVALID_HANDLE_VALUE) return;
    UINT tnBG = MycRom.nBackgrounds;
    ReadSaveFile(hSave, &MycRom.nBackgrounds, sizeof(UINT));
    if (tnBG != MycRom.nBackgrounds)
    {
        MycRom.isExtraBackground = (UINT8*)realloc(MycRom.isExtraBackground, MycRom.nBackgrounds);
        MycRom.BackgroundFrames = (UINT16*)realloc(MycRom.BackgroundFrames, MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
        MycRom.BackgroundFramesX = (UINT16*)realloc(MycRom.BackgroundFramesX, MycRom.nBackgrounds * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    }
    ReadSaveFile(hSave, MycRom.isExtraBackground, MycRom.nBackgrounds);
    ReadSaveFile(hSave, MycRom.BackgroundFrames, MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    ReadSaveFile(hSave, MycRom.BackgroundFramesX, MycRom.nBackgrounds * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    ReadSaveFile(hSave, MycRom.BackgroundMask, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    ReadSaveFile(hSave, MycRom.BackgroundMaskX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    ReadSaveFile(hSave, MycRom.BackgroundID, MycRom.nFrames * sizeof(UINT16));
    ReadSaveFile(hSave, &acBG, sizeof(UINT));
    CloseHandle(hSave);
    RecoverAdjustAction(isUndo);
}

// SA_ADDBACKGROUND: Save all the backgrounds

void SaveAddBackground(bool isUndo)
{
    UINT noSave = GetFreeSaveFileNumber(isUndo);
    if (noSave == 255) return;
    HANDLE hSave = CreateSaveFile(isUndo, noSave);
    if (hSave == INVALID_HANDLE_VALUE) return;
    WriteSaveFile(hSave, &MycRom.nBackgrounds, sizeof(UINT));
    WriteSaveFile(hSave, MycRom.isExtraBackground, MycRom.nBackgrounds);
    WriteSaveFile(hSave, MycRom.BackgroundFrames, MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    WriteSaveFile(hSave, MycRom.BackgroundFramesX, MycRom.nBackgrounds * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    WriteSaveFile(hSave, MycRom.BackgroundMask, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    WriteSaveFile(hSave, MycRom.BackgroundMaskX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    WriteSaveFile(hSave, MycRom.BackgroundID, MycRom.nFrames * sizeof(UINT16));
    WriteSaveFile(hSave, &acBG, sizeof(UINT));
    CloseSaveFile(hSave, isUndo, noSave, SA_ADDBACKGROUND);
}

void RecoverAddBackground(bool isUndo)
{
    RecoverDelBackground(isUndo);
}

//SA_SELBACKGROUND:

void SaveSelBackground(bool isUndo)
{
    UINT8* pBuffer;
    if (isUndo && (UndoAvailableN > 1) && (UndoActionN[UndoAvailableN - 2] == SA_SELBACKGROUND) && (UndoActionN[UndoAvailableN - 1] == SA_SELBACKGROUND))
    {
        UndoAvailableN--;
        acUndoPos -= UndoLen[UndoAvailableN];
    }
    pBuffer = SaveGetBuffer(isUndo, sizeof(UINT16));
    *((UINT16*)pBuffer) = acBG;
    SaveSetAction(isUndo, SA_SELBACKGROUND, sizeof(UINT16));
}

void RecoverSelBackground(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    acBG = *((UINT16*)pBuffer);
    if (MycRom.isExtraBackground && MycRom.isExtraBackground[acBG] > 0) CheckDlgButton(hwTB4, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
    UpdateBackgroundList();
    RecoverAdjustAction(isUndo);
}

// SA_DELFRAMEBG:

UINT CalcSizeFrameBG()
{
    return (nSelFrames * sizeof(UINT16));
}

void SaveDelFrameBG(bool isUndo)
{
    UINT spaceNeeded = CalcSizeFrameBG();
    UINT16* pBuffer = (UINT16*)SaveGetBuffer(isUndo, spaceNeeded);
    for (UINT ti = 0; ti < nSelFrames; ti++) pBuffer[ti] = MycRom.BackgroundID[SelFrames[ti]];
    SaveSetAction(isUndo, SA_DELFRAMEBG, spaceNeeded);
}

void RecoverDelFrameBG(bool isUndo)
{
    UINT16* pBuffer = (UINT16*)RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) MycRom.BackgroundID[SelFrames[ti]] = pBuffer[ti];
    RecoverAdjustAction(isUndo);
}

// SA_REIMPORTBACKGROUND:

UINT CalcSizeReimportBG()
{
    return (sizeof(UINT16) * (MycRom.fWidth * MycRom.fHeight + MycRom.fWidthX * MycRom.fHeightX) + 1);
}

void SaveReimportBG(bool isUndo)
{
    UINT spaceNeeded = CalcSizeReimportBG();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    memcpy(pBuffer, &MycRom.BackgroundFrames[acBG * MycRom.fWidth * MycRom.fHeight], sizeof(UINT16) * MycRom.fWidth * MycRom.fHeight);
    pBuffer += sizeof(UINT16) * MycRom.fWidth * MycRom.fHeight;
    memcpy(pBuffer, &MycRom.BackgroundFramesX[acBG * MycRom.fWidthX * MycRom.fHeightX], sizeof(UINT16) * MycRom.fWidthX * MycRom.fHeightX);
    pBuffer += sizeof(UINT16) * MycRom.fWidthX * MycRom.fHeightX;
    memcpy(pBuffer, &MycRom.isExtraBackground[acBG], 1);
    pBuffer++;
    SaveSetAction(isUndo, SA_REIMPORTBACKGROUND, spaceNeeded);
}

void RecoverReimportBG(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(&MycRom.BackgroundFrames[acBG * MycRom.fWidth * MycRom.fHeight], pBuffer, sizeof(UINT16) * MycRom.fWidth * MycRom.fHeight);
    pBuffer += sizeof(UINT16) * MycRom.fWidth * MycRom.fHeight;
    memcpy(&MycRom.BackgroundFramesX[acBG * MycRom.fWidthX * MycRom.fHeightX], pBuffer, sizeof(UINT16) * MycRom.fWidthX * MycRom.fHeightX);
    pBuffer += sizeof(UINT16) * MycRom.fWidthX * MycRom.fHeightX;
    memcpy(&MycRom.isExtraBackground[acBG], pBuffer, 1);
    pBuffer++;
    RecoverAdjustAction(isUndo);
}

// SA_ACCOLOR:

UINT CalcAcColors()
{
    return (16 * sizeof(UINT16) + sizeof(UINT));
}

void SaveAcColors(bool isUndo)
{
    UINT spaceNeeded = CalcAcColors();
    UINT8* pBuffer = SaveGetBuffer(isUndo, spaceNeeded);
    *(UINT*)pBuffer = noColSel;
    pBuffer += sizeof(UINT);
    memcpy(pBuffer, MycRP.acEditColorsS, 16 * sizeof(UINT16));
    pBuffer += 16 * sizeof(UINT16);
    SaveSetAction(isUndo, SA_ACCOLORS, spaceNeeded);
}

void RecoverAcColors(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    noColSel = *(UINT*)pBuffer;
    pBuffer += sizeof(UINT);
    for (int ti = 0; ti < 16; ti++)
    {
        MycRP.acEditColorsS[ti] = *(UINT16*)pBuffer;
        pBuffer += sizeof(UINT16);
    }
    for (int ti = 0; ti < 16; ti++)
    {
        InvalidateRect(GetDlgItem(hwTB, IDC_COL1 + ti), NULL, TRUE);
        InvalidateRect(GetDlgItem(hwTB2, IDC_COL1 + ti), NULL, TRUE);
    }
    RecoverAdjustAction(isUndo);
}

// SA_TRIGGERID:

void SaveTriggerID(bool isUndo)
{
    UINT8* pBuffer = SaveGetBuffer(isUndo, sizeof(UINT));
    *(UINT*)pBuffer = MycRom.TriggerID[acFrame];
    SaveSetAction(isUndo, SA_TRIGGERID, sizeof(UINT));
}

void RecoverTriggerID(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    MycRom.TriggerID[acFrame] = *(UINT*)pBuffer;
    RecoverAdjustAction(isUndo);
}


void SaveAction(bool isUndo, int action)
{
    if (isUndo)
    {
        RedoAvailableN = 0;
        acRedoPos = 0;
    }
    switch (action)
    {
    case SA_DRAW: // save before drawing
        SaveDrawFrames(isUndo);
        break;
    case SA_SPRITEDRAW: // save before drawing
        SaveDrawSprite(isUndo);
        break;
    case SA_FULLDRAW: // save before drawing + dynamic (mask, sets)
        SaveFullDrawFrames(isUndo);
        break;
    case SA_DRAWPAL: // save before drawing
        SaveDrawPalFrames(isUndo);
        break;
    case SA_SELECTION: // save before changing selection
        SaveSelection(isUndo);
        break;
    case SA_COMPMASK: // save before changing comp mask
       SaveCompMask(isUndo);
        break;
    case SA_PALETTE: // save before changing palette
        SavePalette(isUndo);
        break;
    //case SA_MOVCOMPPATTERN: // save before changing moving comparison pattern
 //       SaveMovCompPattern(isUndo);
    //case SA_EDITCOLOR:
    //    SaveEditColor(isUndo);
    //    break;
    case SA_DYNACOLOR:
        SaveDynaColor(isUndo);
        break;
    case SA_DYNAALL:
        SaveDynaAll(isUndo);
        break;
    case SA_MASKID:
        SaveMaskID(isUndo);
        break;
    case SA_SHAPEMODE:
        SaveShapeMode(isUndo);
        break;
    case SA_COLSETS:
        SaveColorSets(isUndo);
        break;
    case SA_COPYMASK:
        SaveCopyMask(isUndo);
        break;
    case SA_DYNAMASK:
        SaveDynaMask(isUndo);
        break;
    //case SA_SPRITECOLOR:
    //    SaveSpriteCol(isUndo);
    //    break;
    case SA_ACSPRITE:
        SaveAcSprite(isUndo);
        break;
    case SA_SPRITE:
        SaveSprite(isUndo);
        break;
    case SA_SPRITES:
        SaveSprites(isUndo);
        break;
    case SA_FRAMESPRITES:
        SaveFrameSprites(isUndo);
        break;
    case SA_SECTIONS:
        SaveSections(isUndo);
        break;
    case SA_COLROT:
        SaveColRot(isUndo);
        break;
    case SA_DURATION:
        SaveDuration(isUndo);
        break;
    case SA_SETBACKGROUND:
        SaveSetBackground(isUndo);
        break;
    case SA_COPYBACKGROUND:
        SaveCopyBackground(isUndo);
        break;
    case SA_DELBACKGROUND:
        SaveDelBackground(isUndo);
        break;
    case SA_ADDBACKGROUND:
        SaveAddBackground(isUndo);
        break;
    case SA_SELBACKGROUND:
        SaveSelBackground(isUndo);
        break;
    case SA_DELFRAMEBG:
        SaveDelFrameBG(isUndo);
        break;
    case SA_REIMPORTBACKGROUND:
        SaveReimportBG(isUndo);
        break;
    case SA_FRAMES:
        SaveFrames(isUndo);
        break;
    case SA_ISFRAMEX:
        SaveIsFrameX(isUndo);
        break;
    case SA_ISSPRITEX:
        SaveIsSpriteX(isUndo);
        break;
    case SA_ISBGX:
        SaveIsBackgroundX(isUndo);
        break;
    case SA_ACCOLORS:
        SaveAcColors(isUndo);
        break;
    case SA_TRIGGERID:
        SaveTriggerID(isUndo);
        break;
    case SA_SPRITEDETAREAS:
        SaveSpriteDetAreas(isUndo);
        break;
    }
    /*UpdateFSneeded = true;
    UpdateSSneeded = true;
    UpdateBSneeded = true;*/
    UpdateURCounts();
    if (UndoAvailableN > 0)
    {
        EnableWindow(GetDlgItem(hwTB, IDC_UNDO), TRUE);
        EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), TRUE);
        EnableWindow(GetDlgItem(hwTB4, IDC_UNDO), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwTB, IDC_UNDO), FALSE);
        EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), FALSE);
        EnableWindow(GetDlgItem(hwTB4, IDC_UNDO), FALSE);
    }
    if (RedoAvailableN > 0)
    {
        EnableWindow(GetDlgItem(hwTB, IDC_REDO), TRUE);
        EnableWindow(GetDlgItem(hwTB2, IDC_REDO), TRUE);
        EnableWindow(GetDlgItem(hwTB4, IDC_REDO), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwTB, IDC_REDO), FALSE);
        EnableWindow(GetDlgItem(hwTB2, IDC_REDO), FALSE);
        EnableWindow(GetDlgItem(hwTB4, IDC_REDO), FALSE);
    }
}

void RecoverAction(bool isUndo)
{
    int action;
    if (isUndo)
    {
        if (UndoAvailableN == 0) return;
        action = UndoActionN[UndoAvailableN - 1];
    }
    else
    {
        if (RedoAvailableN == 0) return;
        action = RedoActionN[RedoAvailableN - 1];
    }
    switch (action)
    {
    case SA_DRAW: // save before drawing
        SaveDrawFrames(!isUndo);
        RecoverDrawFrames(isUndo);
        break;
    case SA_SPRITEDRAW: // save before drawing
        SaveDrawSprite(!isUndo);
        RecoverDrawSprite(isUndo);
        break;
    case SA_FULLDRAW: // save before drawing + dynamic (mask, sets)
        SaveFullDrawFrames(!isUndo);
        RecoverFullDrawFrames(isUndo);
        break;
    case SA_DRAWPAL: // save before drawing
        SaveDrawPalFrames(!isUndo);
        RecoverDrawPalFrames(isUndo);
        break;
    case SA_SELECTION: // save before changing selection
        SaveSelection(!isUndo);
        RecoverSelection(isUndo);
        break;
    case SA_COMPMASK: // save before changing comp mask
        SaveCompMask(!isUndo);
        RecoverCompMask(isUndo);
        break;
    case SA_PALETTE: // save before changing palette
        SavePalette(!isUndo);
        RecoverPalette(isUndo);
        break;
    case SA_DYNACOLOR:
        SaveDynaColor(!isUndo);
        RecoverDynaColor(isUndo);
        for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
        break;
    case SA_DYNAALL:
        SaveDynaAll(!isUndo);
        RecoverDynaAll(isUndo);
        for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
        break;
    case SA_MASKID:
        SaveMaskID(!isUndo);
        RecoverMaskID(isUndo);
        break;
    case SA_SHAPEMODE:
        SaveShapeMode(!isUndo);
        RecoverShapeMode(isUndo);
        break;
    case SA_COLSETS:
        SaveColorSets(!isUndo);
        RecoverColorSets(isUndo);
        break;
    case SA_COPYMASK:
        SaveCopyMask(!isUndo);
        RecoverCopyMask(isUndo);
        break;
    case SA_DYNAMASK:
        SaveDynaMask(!isUndo);
        RecoverDynaMask(isUndo);
        break;
    case SA_ACSPRITE:
        SaveAcSprite(!isUndo);
        RecoverAcSprite(isUndo);
        break;
    case SA_SPRITE:
        SaveSprite(!isUndo);
        RecoverSprite(isUndo);
        break;
    case SA_FRAMESPRITES:
        SaveFrameSprites(!isUndo);
        RecoverFrameSprites(isUndo);
        break;
    case SA_SPRITES:
        SaveSprites(!isUndo);
        RecoverSprites(isUndo);
        break;
    case SA_SECTIONS:
        SaveSections(!isUndo);
        RecoverSections(isUndo);
        break;
    case SA_COLROT:
        SaveColRot(!isUndo);
        RecoverColRot(isUndo);
        break;
    case SA_DURATION:
        SaveDuration(!isUndo);
        RecoverDuration(isUndo);
        break;
    case SA_SETBACKGROUND:
        SaveSetBackground(!isUndo);
        RecoverSetBackground(isUndo);
        break;
    case SA_COPYBACKGROUND:
        SaveCopyBackground(!isUndo);
        RecoverCopyBackground(isUndo);
        break;
    case SA_DELBACKGROUND:
        SaveDelBackground(!isUndo);
        RecoverDelBackground(isUndo);
        break;
    case SA_ADDBACKGROUND:
        SaveAddBackground(!isUndo);
        RecoverAddBackground(isUndo);
        break;
    case SA_SELBACKGROUND:
        SaveSelBackground(!isUndo);
        RecoverSelBackground(isUndo);
        break;
    case SA_DELFRAMEBG:
        SaveDelFrameBG(!isUndo);
        RecoverDelFrameBG(isUndo);
        break;
    case SA_REIMPORTBACKGROUND:
        SaveReimportBG(!isUndo);
        RecoverReimportBG(isUndo);
        break;
    case SA_FRAMES:
        SaveFrames(!isUndo);
        RecoverFrames(isUndo);
        break;
    case SA_ISFRAMEX:
        SaveIsFrameX(!isUndo);
        RecoverIsFrameX(isUndo);
        break;
    case SA_ISSPRITEX:
        SaveIsSpriteX(!isUndo);
        RecoverIsSpriteX(isUndo);
        break;
    case SA_ISBGX:
        SaveIsBackgroundX(!isUndo);
        RecoverIsBackgroundX(isUndo);
        break;
    case SA_ACCOLORS:
        SaveAcColors(!isUndo);
        RecoverAcColors(isUndo);
        break;
    case SA_TRIGGERID:
        SaveTriggerID(!isUndo);
        RecoverTriggerID(isUndo);
        break;
    case SA_SPRITEDETAREAS:
        SaveSpriteDetAreas(!isUndo);
        RecoverSpriteDetAreas(isUndo);
        break;
    }
    UpdateFSneeded = true;
    UpdateSSneeded = true;
    UpdateBSneeded = true;
    UpdateURCounts();
    if (UndoAvailableN > 0)
    {
        EnableWindow(GetDlgItem(hwTB, IDC_UNDO), TRUE);
        EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), TRUE);
        EnableWindow(GetDlgItem(hwTB4, IDC_UNDO), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwTB, IDC_UNDO), FALSE);
        EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), FALSE);
        EnableWindow(GetDlgItem(hwTB4, IDC_UNDO), FALSE);
    }
    if (RedoAvailableN > 0)
    {
        EnableWindow(GetDlgItem(hwTB, IDC_REDO), TRUE);
        EnableWindow(GetDlgItem(hwTB2, IDC_REDO), TRUE);
        EnableWindow(GetDlgItem(hwTB4, IDC_REDO), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwTB, IDC_REDO), FALSE);
        EnableWindow(GetDlgItem(hwTB2, IDC_REDO), FALSE);
        EnableWindow(GetDlgItem(hwTB4, IDC_REDO), FALSE);
    }
}

#pragma endregion Undo_Tools

#pragma region Debug_Tools

void cprintf(bool isFlash, const char* format,...) // write to the console
{
    char tbuf[5000];
    va_list argptr;
    va_start(argptr, format);
    vsprintf_s(tbuf,1024, format, argptr);
    va_end(argptr);
    char tbuf2[512];
    SYSTEMTIME lt;
    GetLocalTime(&lt);
    sprintf_s(tbuf2, 512, "%02d:%02d: %s\n\r", lt.wHour,lt.wMinute, tbuf);
    WriteFile(hStdout, tbuf2, (DWORD)strlen(tbuf2), NULL, NULL);
    if (isFlash) FlashWindow(hConsole, TRUE);
}

void AffLastError(char* lpszFunction)
{
    char* lpMsgBuf;
    char* lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMsgBuf,
        0, NULL);

    lpDisplayBuf = (char*)LocalAlloc(LMEM_ZEROINIT,
        (strlen((LPCSTR)lpMsgBuf) + strlen((LPCSTR)lpszFunction) + 40) * sizeof(char));
    StringCchPrintfA(lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(char),
        "%s failed with error %d: %s",
        lpszFunction, dw, lpMsgBuf);
    cprintf(true, lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

#pragma endregion Debug_Tools

#pragma region Memory_Tools

void Del_Buffer_Element(UINT8* pBuf, UINT* pnElt, UINT noElt, UINT Elt_Size)
{
    if (noElt >= (*pnElt)) return;
    if ((*pnElt) == 1)
    {
        free(pBuf);
        pBuf = NULL;
        (*pnElt) = 0;
        return;
    }
    if (noElt < ((*pnElt) - 1)) memcpy(&pBuf[noElt * Elt_Size], &pBuf[(noElt + 1) * Elt_Size], ((*pnElt) - 1 - noElt) * Elt_Size);
    pBuf = (UINT8*)realloc(pBuf, ((*pnElt) - 1) * Elt_Size);
    (*pnElt)--;
}

#pragma endregion Memory_Tools

#pragma region Color_Tools

void hsv_to_rgb888(float h, float s, float v, UINT8* r, UINT8* g, UINT8* b)
{
    int i;
    float f, p, q, t;

    if (s == 0)
    {
        *r = *g = *b = (int)(v * 255);
        return;
    }
    h = (float)fmod(h, 360.0);
    if (h < 0) {
        h += 360.0;
    }
    s = (float)fmin(fmax(s, 0.0), 100.0);
    v = (float)fmin(fmax(v, 0.0), 100.0);
    s /= 100.0;
    v /= 100.0;

    h /= 60;
    i = (int)h;
    f = h - i;
    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));

    switch (i) {
    case 0:
        *r = (UINT8)(v * 255);
        *g = (UINT8)(t * 255);
        *b = (UINT8)(p * 255);
        break;
    case 1:
        *r = (UINT8)(q * 255);
        *g = (UINT8)(v * 255);
        *b = (UINT8)(p * 255);
        break;
    case 2:
        *r = (UINT8)(p * 255);
        *g = (UINT8)(v * 255);
        *b = (UINT8)(t * 255);
        break;
    case 3:
        *r = (UINT8)(p * 255);
        *g = (UINT8)(q * 255);
        *b = (UINT8)(v * 255);
        break;
    case 4:
        *r = (UINT8)(t * 255);
        *g = (UINT8)(p * 255);
        *b = (UINT8)(v * 255);
        break;
    default:
        *r = (UINT8)(v * 255);
        *g = (UINT8)(p * 255);
        *b = (UINT8)(q * 255);
        break;
    }
}
void rgb888_to_hex(UINT8 r, UINT8 g, UINT8 b, char* pHex)
{
    UINT val = (((UINT)r) << 16) + (((UINT)g) << 8) + (UINT)b;
    sprintf_s(pHex, 16, "%06X", val);
}
void rgb888_to_hsv(UINT8 r, UINT8 g, UINT8 b, float* h, float* s, float* v)
{
    float red = (float)r / 255;
    float green = (float)g / 255;
    float blue = (float)b / 255;

    float cmax = red > green ? (red > blue ? red : blue) : (green > blue ? green : blue);
    float cmin = red < green ? (red < blue ? red : blue) : (green < blue ? green : blue);
    float delta = cmax - cmin;

    if (delta == 0)
        *h = 0;
    else if (cmax == red)
        *h = 60 * ((green - blue) / delta);
    else if (cmax == green)
        *h = 60 * ((blue - red) / delta + 2);
    else if (cmax == blue)
        *h = 60 * ((red - green) / delta + 4);

    if (*h < 0)
        *h += 360;

    *s = (cmax == 0 ? 0 : delta / cmax) * 100;
    *v = cmax * 100;
}
UINT16 rgb565togray565(UINT16 color)
{
    UINT16 R = (color & 0b1111100000000000) >> 10;
    UINT16 G = (color & 0b0000011111100000) >> 5;
    UINT16 B = (color & 0b0000000000011111) << 1;
    UINT16 graylevel = (UINT16)(0.21f * (float)R + 0.72f * (float)G + 0.07f * (float)B);
    B = R = graylevel >> 1;
    G = graylevel;
    return ((R << 11) + (G << 5) + B);
}
void CopyDynaCol(UINT fromframe, UINT toframe)
{
    for (UINT ti = 0; ti < MAX_DYNA_SETS_PER_FRAMEN; ti++)
    {
        for (UINT tj = 0; tj < MycRom.noColors; tj++)
        {
            MycRom.Dyna4Cols[toframe * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + ti * MycRom.noColors + tj] = MycRom.Dyna4Cols[fromframe * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + ti * MycRom.noColors + tj];
            if (MycRom.isExtraFrame[fromframe])
                MycRom.Dyna4ColsX[toframe * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + ti * MycRom.noColors + tj] = MycRom.Dyna4ColsX[fromframe * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + ti * MycRom.noColors + tj];
        }
    }
}

void Choose_Color_Palette(int butid)
{
    RECT rc;
    GetWindowRect(GetDlgItem(hwTB, butid), &rc);
    if (hPal) DestroyWindow(hPal);
    if (hPal2) DestroyWindow(hPal2);
    if (hPal3) DestroyWindow(hPal3);
    hPal2 = hPal3 = NULL;
    if (butid < IDC_DYNACOL1) noColMod = butid - IDC_COL1; else noColMod = butid - IDC_DYNACOL1 + 16;
    int sh = 160 + MARGIN_PALETTE_X + MARGIN_PALETTE_Y;
    if (MycRP.isImported >0 ) sh += MARGIN_PALETTE_X + 160;
    hPal = CreateWindowEx(0, L"Palette", L"", WS_POPUP, rc.left, rc.top, 160 + 2 * MARGIN_PALETTE_X, sh, NULL, NULL, hInst, NULL);       // Parent window
    if (!hPal)
    {
        AffLastError((char*)"Create Pal Window");
        return;
    }
    ShowWindow(hPal, true);
}

void Choose_Color_Palette2(int butid)
{
    RECT rc;
    GetWindowRect(GetDlgItem(hwTB2, butid), &rc);
    if (hPal) DestroyWindow(hPal);
    if (hPal2) DestroyWindow(hPal2);
    if (hPal3) DestroyWindow(hPal3);
    hPal = hPal3 = NULL;
    noColMod = butid - IDC_COL1;
    hPal2 = CreateWindowEx(0, L"Palette", L"", WS_POPUP, rc.left, rc.top, 160 + 2 * MARGIN_PALETTE_X, 160 + MARGIN_PALETTE_X + MARGIN_PALETTE_Y, NULL, NULL, hInst, NULL);       // Parent window.
    if (!hPal2)
    {
        AffLastError((char*)"Create Pal Window");
        return;
    }
    ShowWindow(hPal2, true);
}

void Choose_Color_Palette3(void)
{
    RECT rc;
    GetWindowRect(GetDlgItem(hwTB, IDC_COLROT), &rc);
    if (hPal) DestroyWindow(hPal);
    if (hPal2) DestroyWindow(hPal2);
    if (hPal3) DestroyWindow(hPal3);
    hPal2 = hPal = NULL;
    hPal3 = CreateWindowEx(0, L"Palette", L"", WS_POPUP, rc.left, rc.top, 160 + 2 * MARGIN_PALETTE_X, 160 + MARGIN_PALETTE_X + MARGIN_PALETTE_Y, NULL, NULL, hInst, NULL);       // Parent window.
    if (!hPal3)
    {
        AffLastError((char*)"Create Pal Window");
        return;
    }
    ShowWindow(hPal3, true);
}
void DrawBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int width, int height)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    BitBlt(hdc, x, y, width, height, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);
}
int ncolpickthumbPos = 0;
void UpdateHSValues(HWND hwD)
{
    char tbuf[32];
    sprintf_s(tbuf, 32, "%u", (UINT)(roundf(ColPick_H)));
    SetDlgItemTextA(hwD, IDC_H, tbuf);
    sprintf_s(tbuf, 32, "%u", (UINT)(roundf(ColPick_S)));
    SetDlgItemTextA(hwD, IDC_S, tbuf);
}
void UpdateVValues(HWND hwD)
{
    char tbuf[32];
    sprintf_s(tbuf, 32, "%u", (UINT)(roundf(ColPick_V)));
    SetDlgItemTextA(hwD, IDC_V, tbuf);

}
void UpdateRGBValues(HWND hwD)
{
    char tbuf[32];
    sprintf_s(tbuf, 32, "%u", (UINT)ColPick_R8);
    SetDlgItemTextA(hwD, IDC_R8, tbuf);
    sprintf_s(tbuf, 32, "%u", (UINT)ColPick_G8);
    SetDlgItemTextA(hwD, IDC_G8, tbuf);
    sprintf_s(tbuf, 32, "%u", (UINT)ColPick_B8);
    SetDlgItemTextA(hwD, IDC_B8, tbuf);
    sprintf_s(tbuf, 32, "%u", (UINT)ColPick_R8>>3);
    SetDlgItemTextA(hwD, IDC_R5, tbuf);
    sprintf_s(tbuf, 32, "%u", (UINT)ColPick_G8>>2);
    SetDlgItemTextA(hwD, IDC_G6, tbuf);
    sprintf_s(tbuf, 32, "%u", (UINT)ColPick_B8>>3);
    SetDlgItemTextA(hwD, IDC_B5, tbuf);
    rgb888_to_hex(ColPick_R8, ColPick_G8, ColPick_B8, tbuf);
    SetDlgItemTextA(hwD, IDC_RGB888, tbuf);
    sprintf_s(tbuf, 32, "%05u", (UINT)rgb888_to_rgb565(ColPick_R8, ColPick_G8, ColPick_B8));
    SetDlgItemTextA(hwD, IDC_RGB565, tbuf);
}
void CalcColFromHSV(void)
{
    hsv_to_rgb888(ColPick_H, ColPick_S, ColPick_V, &ColPick_R8, &ColPick_G8, &ColPick_B8);
}
void CalcColFromRGB888(void)
{
    rgb888_to_hsv(ColPick_R8, ColPick_G8, ColPick_B8, &ColPick_H, &ColPick_S, &ColPick_V);
}
UINT8 hexCharToUint8(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    else return -1;
}
UINT8 hexStringToUint8(char* hexString)
{
    UINT8 result = 0;
    result += (hexCharToUint8(hexString[0]) << 4);
    result += hexCharToUint8(hexString[1]);
    return result;
}

HWND hColPick = NULL;
LRESULT CALLBACK RGB888Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CHAR:
        {
            if (!isxdigit((char)wParam) && wParam != 0x03 && wParam != 0x16 && wParam != 0x18 && wParam != 0x08 && wParam != 0x09 && wParam != 0x0D)
                return 0;
            break;
        }
        case WM_SETFOCUS:
        {
            SendMessage(hwnd, EM_SETSEL, 0, -1);
            return DefSubclassProc(hwnd, msg, wParam, lParam);
            break;
        }
        case WM_PASTE:
        {
            // Filter pasted text
            if (!IsClipboardFormatAvailable(CF_TEXT)) break;

            if (!OpenClipboard(hwnd)) break;

            HANDLE hClipboardData = GetClipboardData(CF_TEXT);
            if (hClipboardData == nullptr)
            {
                CloseClipboard();
                break;
            }

            char* pszClipboardText = static_cast<char*>(GlobalLock(hClipboardData));
            if (pszClipboardText == nullptr)
            {
                CloseClipboard();
                break;
            }

            char tbuf[8];
            int j = 0;
            for (size_t i = 0; i < strlen(pszClipboardText); ++i)
            {
                if (isxdigit(pszClipboardText[i]) ||
                    pszClipboardText[i] == '\b' || // Backspace
                    pszClipboardText[i] == '\t' || // Tab
                    pszClipboardText[i] == '\x16' || // Ctrl+V (Paste)
                    pszClipboardText[i] == '\x18' || // Ctrl+X (Cut)
                    pszClipboardText[i] == '\x03' || // Ctrl+C (Copy)
                    pszClipboardText[i] == '\x7F')
                {
                    tbuf[j] = pszClipboardText[i];
                    j = j + 1;
                    if (j == 6) break;
                }
            }
            tbuf[j] = 0;
            SetWindowTextA(hwnd, tbuf);

            GlobalUnlock(hClipboardData);
            CloseClipboard();
        }
        case WM_KILLFOCUS:
        {
            char tbuf[16], tbuf2[16];
            GetWindowTextA(hwnd, tbuf, 16);
            int sl = (int)strlen(tbuf);
            while (sl < 6)
            {
                strcpy_s(tbuf2, 16, tbuf);
                sprintf_s(tbuf, 16, "0%s", tbuf2);
                sl = (int)strlen(tbuf);
            }
            SetWindowTextA(hwnd, tbuf);
            ColPick_R8 = hexStringToUint8(tbuf);
            ColPick_G8 = hexStringToUint8(&tbuf[2]);
            ColPick_B8 = hexStringToUint8(&tbuf[4]);
            rgb888_to_hsv(ColPick_R8, ColPick_G8, ColPick_B8, &ColPick_H, &ColPick_S, &ColPick_V);
            UpdateHSValues(hColPick);
            UpdateVValues(hColPick);
            UpdateRGBValues(hColPick);
            InvalidateRect(hColPick, NULL, FALSE);
            ncolpickthumbPos = (int)(500 - 500 * ColPick_V / 100.0f);
            SendMessage(GetDlgItem(hColPick, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
            break;
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK RGB8Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KILLFOCUS)
    {
        char tbuf[16];
        GetWindowTextA(hwnd, tbuf, 16);
        UINT val = atoi(tbuf);
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        _itoa_s(val, tbuf, 16, 10);
        SetWindowTextA(hwnd, tbuf);
        if (hwnd == GetDlgItem(hColPick, IDC_R8)) ColPick_R8 = (UINT8)val;
        else if (hwnd == GetDlgItem(hColPick, IDC_G8)) ColPick_G8 = (UINT8)val;
        else if (hwnd == GetDlgItem(hColPick, IDC_B8)) ColPick_B8 = (UINT8)val;
        else return DefSubclassProc(hwnd, msg, wParam, lParam);
        rgb888_to_hsv(ColPick_R8, ColPick_G8, ColPick_B8, &ColPick_H, &ColPick_S, &ColPick_V);
        UpdateHSValues(hColPick);
        UpdateVValues(hColPick);
        UpdateRGBValues(hColPick);
        InvalidateRect(hColPick, NULL, FALSE);
        ncolpickthumbPos = (int)(500 - 500 * ColPick_V / 100.0f);
        SendMessage(GetDlgItem(hColPick, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
    }
    else if (msg == WM_SETFOCUS)
    {
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK RGB565Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KILLFOCUS)
    {
        char tbuf[16];
        GetWindowTextA(hwnd, tbuf, 10);
        UINT val = atoi(tbuf);
        if (val < 0) val = 0;
        if (val > 65535) val = 65535;
        _itoa_s(val, tbuf, 16, 10);
        SetWindowTextA(hwnd, tbuf);
        rgb565_to_rgb888((UINT16)val, &ColPick_R8, &ColPick_G8, &ColPick_B8);
        rgb888_to_hsv(ColPick_R8, ColPick_G8, ColPick_B8, &ColPick_H, &ColPick_S, &ColPick_V);
        UpdateHSValues(hColPick);
        UpdateVValues(hColPick);
        UpdateRGBValues(hColPick);
        InvalidateRect(hColPick, NULL, FALSE);
        ncolpickthumbPos = (int)(500 - 500 * ColPick_V / 100.0f);
        SendMessage(GetDlgItem(hColPick, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
    }
    else if (msg==WM_PASTE)
    {
        if (IsClipboardFormatAvailable(CF_TEXT))
        {
            if (OpenClipboard(hwnd))
            {
                HANDLE hClipboardData = GetClipboardData(CF_TEXT);
                if (hClipboardData != nullptr)
                {
                    char* pszClipboardText = static_cast<char*>(GlobalLock(hClipboardData));
                    if (pszClipboardText != nullptr)
                    {
                        char tbuf[16];
                        int j = 0;
                        for (size_t i = 0; i < strlen(pszClipboardText); ++i)
                        {
                            if (isdigit(pszClipboardText[i]) ||
                                pszClipboardText[i] == '\b' || // Backspace
                                pszClipboardText[i] == '\t' || // Tab
                                pszClipboardText[i] == '\x16' || // Ctrl+V (Paste)
                                pszClipboardText[i] == '\x18' || // Ctrl+X (Cut)
                                pszClipboardText[i] == '\x03' || // Ctrl+C (Copy)
                                pszClipboardText[i] == '\x7F')
                            {
                                tbuf[j] = pszClipboardText[i];
                                j = j + 1;
                                if (j == 6) break;
                            }
                        }
                        tbuf[j] = 0;
                        SetWindowTextA(hwnd, tbuf);
                        GlobalUnlock(hClipboardData);
                        GetWindowTextA(hwnd, tbuf, 10);
                        UINT val = atoi(tbuf);
                        if (val < 0) val = 0;
                        if (val > 65535) val = 65535;
                        _itoa_s(val, tbuf, 16, 10);
                        SetWindowTextA(hwnd, tbuf);
                        rgb565_to_rgb888((UINT16)val, &ColPick_R8, &ColPick_G8, &ColPick_B8);
                        rgb888_to_hsv(ColPick_R8, ColPick_G8, ColPick_B8, &ColPick_H, &ColPick_S, &ColPick_V);
                        UpdateHSValues(hColPick);
                        UpdateVValues(hColPick);
                        UpdateRGBValues(hColPick);
                        InvalidateRect(hColPick, NULL, FALSE);
                        ncolpickthumbPos = (int)(500 - 500 * ColPick_V / 100.0f);
                        SendMessage(GetDlgItem(hColPick, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
                    }
                }
                CloseClipboard();
            }
        }
    }
    else if (msg == WM_SETFOCUS)
    {
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK RGB5Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KILLFOCUS)
    {
        char tbuf[16];
        GetWindowTextA(hwnd, tbuf, 16);
        UINT val32 = atoi(tbuf);
        UINT8 val;
        if (val32 < 0) val32 = 0;
        if (val32 > 63) val32 = 63;
        if ((hwnd == GetDlgItem(hColPick, IDC_R5) || hwnd == GetDlgItem(hColPick, IDC_B5)) && val32 > 31) val32 = 31;
        val = (UINT8)val32;
        _itoa_s(val32, tbuf, 16, 10);
        SetWindowTextA(hwnd, tbuf);

        if (hwnd == GetDlgItem(hColPick, IDC_R5)) ColPick_R8 = val << 3 | val >> 2; // bit shift + copy rgb565 most significant bits to rgb888 less significant bits (see rgb565_to_rgb888() functions)
        else if (hwnd == GetDlgItem(hColPick, IDC_G6)) ColPick_G8 = val << 2 | val >> 4; // bit shift + copy rgb565 most significant bits to rgb888 less significant bits (see rgb565_to_rgb888() functions)
        else if (hwnd == GetDlgItem(hColPick, IDC_B5)) ColPick_B8 = val << 3 | val >> 2; // bit shift + copy rgb565 most significant bits to rgb888 less significant bits (see rgb565_to_rgb888() functions)
        else return DefSubclassProc(hwnd, msg, wParam, lParam);
        rgb888_to_hsv(ColPick_R8, ColPick_G8, ColPick_B8, &ColPick_H, &ColPick_S, &ColPick_V);
        UpdateHSValues(hColPick);
        UpdateVValues(hColPick);
        UpdateRGBValues(hColPick);
        InvalidateRect(hColPick, NULL, FALSE);
        ncolpickthumbPos = (int)(500 - 500 * ColPick_V / 100.0f);
        SendMessage(GetDlgItem(hColPick, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
    }
    else if (msg == WM_SETFOCUS)
    {
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK HSVProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KILLFOCUS)
    {
        char tbuf[16];
        GetWindowTextA(hwnd, tbuf, 16);
        UINT val = atoi(tbuf);
        if (val < 0) val = 0;
        if (val > 360) val = 360;
        if ((hwnd == GetDlgItem(hColPick, IDC_S) || hwnd == GetDlgItem(hColPick, IDC_V)) && val > 100)
        {
            val = 100;
        }
        if (hwnd == GetDlgItem(hColPick, IDC_S)) ColPick_S = (float)val;
        else if (hwnd == GetDlgItem(hColPick, IDC_V)) ColPick_V = (float)val;
        else ColPick_H = (float)val;
        _itoa_s(val, tbuf, 16, 10);
        SetWindowTextA(hwnd, tbuf);
        hsv_to_rgb888(ColPick_H, ColPick_S, ColPick_V, &ColPick_R8, &ColPick_G8, &ColPick_B8);
        UpdateHSValues(hColPick);
        UpdateVValues(hColPick);
        UpdateRGBValues(hColPick);
        InvalidateRect(hColPick, NULL, FALSE);
        ncolpickthumbPos = (int)(500 - 500 * ColPick_V / 100.0f);
        SendMessage(GetDlgItem(hColPick, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
    }
    else if (msg == WM_SETFOCUS)
    {
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

HBITMAP hColPickBitmap = NULL;
bool iscolpickbpressed = false;
int ColPickReturnValue = 0;
UINT16 IniPickCol;
UINT16 previousPickedColors[8] = { 0,0,0,0,0,0,0,0 };
LRESULT CALLBACK ColPickProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            {
                MoveWindow(GetDlgItem(hwndDlg, IDC_COLORPICK), 20, 20, 512, 512, TRUE);
                if (!hColPickBitmap)
                {
                    char path[MAX_PATH];
                    GetModuleFileNameA(NULL, path, MAX_PATH);
                    PathRemoveFileSpecA(path);
                    strcat_s(path, MAX_PATH, "\\textures\\color_picker_image.bmp");
                    hColPickBitmap = (HBITMAP)LoadImageA(NULL, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
                    if (hColPickBitmap == NULL) AffLastError((char*)"LoadImage");
                }
                hColPick = hwndDlg;
                HWND slider = GetDlgItem(hwndDlg, IDC_LUMIN);
                SendMessage(slider, TBM_SETRANGE, TRUE, MAKELONG(0, 500));
                SendMessage(slider, TBM_SETTICFREQ, 50, 0);
                rgb565_to_rgb888(IniPickCol, &ColPick_R8, &ColPick_G8, &ColPick_B8);
                CalcColFromRGB888();
                UpdateHSValues(hwndDlg);
                UpdateVValues(hwndDlg);
                UpdateRGBValues(hwndDlg);
                iscolpickbpressed = false;
                SendMessage(GetDlgItem(hwndDlg, IDC_RGB888), EM_SETLIMITTEXT, 6, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_RGB888), (SUBCLASSPROC)RGB888Proc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_R8), EM_SETLIMITTEXT, 3, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_R8), (SUBCLASSPROC)RGB8Proc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_G8), EM_SETLIMITTEXT, 3, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_G8), (SUBCLASSPROC)RGB8Proc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_B8), EM_SETLIMITTEXT, 3, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_B8), (SUBCLASSPROC)RGB8Proc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RGB565), EM_SETLIMITTEXT, 5, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_RGB565), (SUBCLASSPROC)RGB565Proc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_R5), EM_SETLIMITTEXT, 2, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_R5), (SUBCLASSPROC)RGB5Proc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_G6), EM_SETLIMITTEXT, 2, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_G6), (SUBCLASSPROC)RGB5Proc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_B5), EM_SETLIMITTEXT, 2, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_B5), (SUBCLASSPROC)RGB5Proc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_H), EM_SETLIMITTEXT, 3, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_H), (SUBCLASSPROC)HSVProc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_S), EM_SETLIMITTEXT, 3, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_S), (SUBCLASSPROC)HSVProc, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_V), EM_SETLIMITTEXT, 3, 0);
                SetWindowSubclass(GetDlgItem(hwndDlg, IDC_V), (SUBCLASSPROC)HSVProc, 0, 0);
                InvalidateRect(hColPick, NULL, FALSE);
                ncolpickthumbPos = (int)(500 - 500 * ColPick_V / 100.0f);
                SendMessage(GetDlgItem(hColPick, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
                ColPickReturnValue = -1;
                return TRUE;
            }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    ColPickReturnValue = (int)rgb888_to_rgb565(ColPick_R8, ColPick_G8, ColPick_B8);
                    for (int ti = 0; ti < 8; ti++)
                    {
                        if (previousPickedColors[ti] == ColPickReturnValue)
                        {
                            for (int tj = ti; tj < 7; tj++) previousPickedColors[tj] = previousPickedColors[tj + 1];
                        }
                    }
                    for (int ti = 7; ti > 0; ti--) previousPickedColors[ti] = previousPickedColors[ti - 1];
                    previousPickedColors[0] = ColPickReturnValue;
                    EndDialog(hwndDlg, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                {
                    ColPickReturnValue = -1;
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
                }
                case IDC_OLDCOL1:
                case IDC_OLDCOL2:
                case IDC_OLDCOL3:
                case IDC_OLDCOL4:
                case IDC_OLDCOL5:
                case IDC_OLDCOL6:
                case IDC_OLDCOL7:
                case IDC_OLDCOL8:
                {
                    rgb565_to_rgb888(previousPickedColors[LOWORD(wParam) - IDC_OLDCOL1], &ColPick_R8, &ColPick_G8, &ColPick_B8);
                    CalcColFromRGB888();
                    UpdateHSValues(hwndDlg);
                    UpdateVValues(hwndDlg);
                    UpdateRGBValues(hwndDlg);
                    ncolpickthumbPos = (int)(500 - 500 * ColPick_V / 100.0f);
                    SendMessage(GetDlgItem(hColPick, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
                    InvalidateRect(hColPick, NULL, FALSE);
                    return TRUE;
                }
            }
            return TRUE;
        }
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwndDlg, &ps);
                HWND hControl = GetDlgItem(hwndDlg, IDC_COLORPICK);
                RECT rectb;
                GetWindowRect(hControl, &rectb);
                ScreenToClient(hwndDlg, (LPPOINT)&rectb.left);
                ScreenToClient(hwndDlg, (LPPOINT)&rectb.right);
                int controlWidth = rectb.right - rectb.left;
                int controlHeight = rectb.bottom - rectb.top;
                DrawBitmap(hdc, hColPickBitmap, rectb.left, rectb.top, controlWidth, controlHeight);
                int crossSize = 20;
                int halfCrossSize = crossSize / 2;
                int xc = (int)(ColPick_H / 360.0f * 512) + rectb.left;
                int yc = 512 - (int)(ColPick_S / 100.0f * 512) + rectb.top;
                IntersectClipRect(hdc, rectb.left, rectb.top, rectb.right, rectb.bottom);
                HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255)); // White pen
                SelectObject(hdc, hPen);
                MoveToEx(hdc, xc - halfCrossSize, yc, NULL);
                LineTo(hdc, xc + halfCrossSize, yc); 
                MoveToEx(hdc, xc, yc - halfCrossSize, NULL);
                LineTo(hdc, xc, yc + halfCrossSize);
                DeleteObject(hPen);
                SelectClipRgn(hdc, NULL);
                RECT rectc;
                hControl = GetDlgItem(hwndDlg, IDC_COLORR);
                GetWindowRect(hControl, &rectb);
                ScreenToClient(hwndDlg, (LPPOINT)&rectb.left);
                ScreenToClient(hwndDlg, (LPPOINT)&rectb.right);
                LONG halfw = (rectb.right - rectb.left) / 2;
                LONG thirdh = (rectb.bottom - rectb.top) / 3;
                HBRUSH hBrush = CreateSolidBrush(RGB(ColPick_R8, ColPick_G8 ,ColPick_B8));
                rectc.top = rectb.top;
                rectc.left = rectb.left;
                rectc.bottom = rectb.bottom;
                rectc.right = rectb.left + halfw;
                FillRect(hdc, &rectc, hBrush);
                DeleteObject(hBrush);
                UINT16 rgb565 = rgb888_to_rgb565(ColPick_R8, ColPick_G8, ColPick_B8);
                UINT8 rgb888[3];
                rgb565_to_rgb888(rgb565, rgb888);
                hBrush = CreateSolidBrush(RGB(rgb888[0], rgb888[1], rgb888[2]));
                rectc.left += halfw;
                rectc.right = rectb.right;
                FillRect(hdc, &rectc, hBrush);
                DeleteObject(hBrush);
                DeleteObject(hPen);
                hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
                SelectObject(hdc, hPen);
                MoveToEx(hdc, rectc.left, rectc.top, NULL);
                LineTo(hdc, rectc.left, rectc.top + thirdh);
                MoveToEx(hdc, rectc.left, rectc.top + 2 * thirdh, NULL);
                LineTo(hdc, rectc.left, rectc.bottom);
                DeleteObject(hPen);
                EndPaint(hwndDlg, &ps);
                return TRUE;
            }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
            UINT tpm;
            if ((lpdis->CtlID >= IDC_OLDCOL1) && (lpdis->CtlID <= IDC_OLDCOL8))
            {
                if (lpdis->CtlID) tpm = lpdis->CtlID - IDC_OLDCOL1;
                HBRUSH bg = CreateSolidBrush(RGB565_to_RGB888(previousPickedColors[tpm]));
                RECT rc = lpdis->rcItem;
                /*if (tpm == noColSel)
                {
                    HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
                    FillRect(lpdis->hDC, &rc, br);
                    DeleteObject(br);
                }
                rc.left += 2;
                rc.right -= 2;
                rc.top += 2;
                rc.bottom -= 2;*/
                FillRect(lpdis->hDC, &rc, bg);
                DeleteObject(bg);
            }
            return TRUE;
        }
        case WM_CLOSE:
            {
                EndDialog(hwndDlg, 0);
                return TRUE;
            }
        case WM_LBUTTONUP:
            {
                iscolpickbpressed = false;
                return TRUE;
            }
        case WM_LBUTTONDOWN:
            {
                int mouseX = GET_X_LPARAM(lParam);
                int mouseY = GET_Y_LPARAM(lParam);
                HWND hWndControl = GetDlgItem(hwndDlg, IDC_COLORPICK);
                RECT rectb;
                GetWindowRect(hWndControl, &rectb);
                ScreenToClient(hwndDlg, (LPPOINT)&rectb.left);
                ScreenToClient(hwndDlg, (LPPOINT)&rectb.right);
                int relativeX = mouseX - rectb.left;
                int relativeY = mouseY - rectb.top;
                if (relativeX >= 0 && relativeY >= 0 && relativeX <= 512 && relativeY <= 512)
                {
                    iscolpickbpressed = true;
                    ColPick_H = ((float)relativeX / 512.0f * 360);
                    ColPick_S = 100 - ((float)relativeY / 512.0f * 100);
                    UpdateHSValues(hwndDlg);
                    InvalidateRect(hwndDlg, NULL, FALSE);
                    CalcColFromHSV();
                    UpdateRGBValues(hwndDlg);
                    return TRUE;
                }
            }
        case WM_MOUSEMOVE:
            {
                if (!iscolpickbpressed) return TRUE;
                int mouseX = GET_X_LPARAM(lParam);
                int mouseY = GET_Y_LPARAM(lParam);
                HWND hWndControl = GetDlgItem(hwndDlg, IDC_COLORPICK);
                RECT rectb;
                GetWindowRect(hWndControl, &rectb);
                ScreenToClient(hwndDlg, (LPPOINT)&rectb.left);
                ScreenToClient(hwndDlg, (LPPOINT)&rectb.right);
                int relativeX = mouseX - rectb.left;
                int relativeY = mouseY - rectb.top;
                if (relativeX < 0) relativeX = 0;
                else if (relativeX > 512) relativeX = 512;
                if (relativeY < 0) relativeY = 0;
                else if (relativeY > 512) relativeY = 512;
                ColPick_H = ((float)relativeX / 512.0f * 360);
                ColPick_S = 100 - ((float)relativeY / 512.0f * 100);
                UpdateHSValues(hwndDlg);
                CalcColFromHSV();
                UpdateRGBValues(hwndDlg);
                InvalidateRect(hwndDlg, NULL, FALSE);
                return TRUE;
            }
        case WM_VSCROLL:
            {
                int nScrollCode = LOWORD(wParam);
                switch (nScrollCode)
                {
                    case SB_PAGEDOWN:
                    {
                        ncolpickthumbPos += 50;
                        if (ncolpickthumbPos > 500) ncolpickthumbPos = 500;
                        SendMessage(GetDlgItem(hwndDlg, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
                        ColPick_V = 100 - (float)ncolpickthumbPos / 500.0f * 100;
                        UpdateHSValues(hwndDlg);
                        UpdateVValues(hwndDlg);
                        CalcColFromHSV();
                        UpdateRGBValues(hwndDlg);
                        InvalidateRect(hwndDlg, NULL, FALSE);
                        return TRUE;
                    }
                    case SB_PAGEUP:
                    {
                        ncolpickthumbPos -= 50;
                        if (ncolpickthumbPos < 0) ncolpickthumbPos = 0;
                        SendMessage(GetDlgItem(hwndDlg, IDC_LUMIN), TBM_SETPOS, TRUE, ncolpickthumbPos);
                        ColPick_V = 100 - (float)ncolpickthumbPos / 500.0f * 100;
                        UpdateHSValues(hwndDlg);
                        UpdateVValues(hwndDlg);
                        CalcColFromHSV();
                        UpdateRGBValues(hwndDlg);
                        InvalidateRect(hwndDlg, NULL, FALSE);
                        return TRUE;
                    }
                    case SB_THUMBPOSITION: 
                    case SB_THUMBTRACK:
                    {
                        ncolpickthumbPos = HIWORD(wParam);    
                        ColPick_V = 100 - (float)ncolpickthumbPos / 500.0f * 100;
                        UpdateHSValues(hwndDlg);
                        UpdateVValues(hwndDlg);
                        CalcColFromHSV();
                        UpdateRGBValues(hwndDlg);
                        InvalidateRect(hwndDlg, NULL, FALSE);
                    }
                }
                return TRUE;
            }
    }
    return FALSE;
}
bool ColorPicker(UINT16* pRGB,LONG xm,LONG ym, HWND hparent)
{
    IniPickCol = *pRGB;
    DialogBoxA(hInst, MAKEINTRESOURCEA(IDD_COLORPICKER), hparent, ColPickProc);
    if (ColPickReturnValue >= 0)
    {
        SaveAction(true, SA_PALETTE);
        *pRGB = (UINT16)ColPickReturnValue;
        SetCursorPos(xm, ym);
        int butid;
        if (noColMod < 16) butid = IDC_COL1 + noColMod; else butid = IDC_DYNACOL1 + noColMod - 16;
        if (hparent==hPal) Choose_Color_Palette(butid);
        else Choose_Color_Palette2(butid);
        //InvalidateRect(GetDlgItem(hwTB3, IDC_COLORS), NULL, FALSE);
        return true;
    }
    SetCursorPos(xm, ym);
    int butid;
    if (noColMod < 16) butid = IDC_COL1 + noColMod; else butid = IDC_DYNACOL1 + noColMod - 16;
    if (hparent == hPal) Choose_Color_Palette(butid);
    else Choose_Color_Palette2(butid);
    //InvalidateRect(GetDlgItem(hwTB3, IDC_COLORS), NULL, FALSE);
    return false;
}
void ResizeDynaMask(UINT8* pdImage, UINT dwidth, UINT dheight, UINT8* psImage, bool shrink)
{
    for (UINT tj = 0; tj < dheight; tj++)
    {
        for (UINT ti = 0; ti < dwidth; ti++)
        {
            UINT8* finpt = &pdImage[tj * dwidth + ti];
            if (shrink)
            {
                UINT8* tpt = &psImage[(tj * 2) * dwidth * 2 + ti * 2];
                if (tpt[0] == tpt[1] || tpt[0] == tpt[dwidth * 2] || tpt[0] == tpt[dwidth * 2 + 1]) *finpt = tpt[0];
                else if (tpt[1] == tpt[dwidth * 2] || tpt[1] == tpt[dwidth * 2 + 1]) *finpt = tpt[1];
                else if (tpt[dwidth * 2] == tpt[dwidth * 2 + 1]) *finpt = tpt[dwidth * 2];
                else *finpt = tpt[0];
            }
            else *finpt = psImage[(tj / 2) * dwidth / 2 + ti / 2];
        }
    }
}
void ResizeBGMask(UINT8* pdImage, UINT dwidth, UINT dheight, UINT8* psImage, bool shrink)
{
    for (UINT tj = 0; tj < dheight; tj++)
    {
        for (UINT ti = 0; ti < dwidth; ti++)
        {
            UINT8* finpt = &pdImage[tj * dwidth + ti];
            if (shrink)
            {
                UINT8* tpt = &psImage[(tj * 2) * dwidth * 2 + ti * 2];
                if (tpt[0] == tpt[1] || tpt[0] == tpt[dwidth * 2] || tpt[0] == tpt[dwidth * 2 + 1]) *finpt = tpt[0];
                else if (tpt[1] == tpt[dwidth * 2] || tpt[1] == tpt[dwidth * 2 + 1]) *finpt = tpt[1];
                else if (tpt[dwidth * 2] == tpt[dwidth * 2 + 1]) *finpt = tpt[dwidth * 2];
                else *finpt = tpt[0];
            }
            else
                *finpt = psImage[(tj / 2) * dwidth / 2 + ti / 2];
        }
    }
}
void ResizeRGB565Image(UINT16* pdImage, UINT dwidth, UINT dheight, UINT16* psImage, UINT swidth, UINT sheight, int filter)
{
    cv::Mat imageMat(sheight, swidth, CV_8UC3);
    UINT8 rgb888[3];
    for (UINT y = 0; y < sheight; ++y)
    {
        for (UINT x = 0; x < swidth; ++x)
        {
            rgb565_to_rgb888(psImage[y * swidth + x], rgb888);
            imageMat.at<cv::Vec3b>(y, x) = cv::Vec3b(rgb888[2], rgb888[1], rgb888[0]); // OpenCV uses BGR ordering, so we need to swap R and B channels
        }
    }
    cv::Mat destmat;
    cv::resize(imageMat, destmat, cv::Size(dwidth, dheight), 0, 0, filter);
    for (UINT y = 0; y < dheight; ++y)
    {
        for (UINT x = 0; x < dwidth; ++x)
        {
            cv::Vec3b pixel = destmat.at<cv::Vec3b>(y, x);
            pdImage[y * dwidth + x] = rgb888_to_rgb565(pixel[2], pixel[1], pixel[0]);
        }
    }
}
//ResizeRGB565Sprite(&MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT]);
//ResizeSpriteMask(&MycRom.SpriteMaskX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], MycRom.fHeight == 64);
void ResizeRGB565Sprite(UINT16* pdSprite, UINT8* pdSprMask, UINT16* psSprite, UINT8* psSprMask, bool shrink, int filter)
{
    int ssprw=0, ssprh=0;
    for (int tj = 0; tj < MAX_SPRITE_HEIGHT; tj++)
    {
        for (int ti = 0; ti < MAX_SPRITE_WIDTH; ti++)
        {
            if (psSprMask[tj * MAX_SPRITE_WIDTH + ti] < 255)
            {
                if (tj > ssprh) ssprh = tj;
                if (ti > ssprw) ssprw = ti;
            }
        }
    }
    ssprh++;
    ssprw++;
    cv::Mat imageMat(ssprh, ssprw, CV_8UC3);
    UINT8 rgb888[3];
    for (int y = 0; y < ssprh; ++y)
    {
        for (int x = 0; x < ssprw; ++x)
        {
            rgb565_to_rgb888(psSprite[y * MAX_SPRITE_WIDTH + x], rgb888);
            imageMat.at<cv::Vec3b>(y, x) = cv::Vec3b(rgb888[2], rgb888[1], rgb888[0]);
        }
    }
    int dsprw, dsprh;
    if (shrink)
    {
        dsprw = ssprw / 2;
        dsprh = ssprh / 2;
    }
    else
    {
        dsprw = min(ssprw * 2, MAX_SPRITE_WIDTH);
        dsprh = min(ssprh * 2, MAX_SPRITE_HEIGHT);
    }
    cv::Mat destmat;
    cv::resize(imageMat, destmat, cv::Size(dsprw, dsprh), 0, 0, filter);
    for (int y = 0; y < dsprh; ++y)
    {
        for (int x = 0; x < dsprw; ++x)
        {
            cv::Vec3b pixel = destmat.at<cv::Vec3b>(y, x);
            pdSprite[y * MAX_SPRITE_WIDTH + x] = rgb888_to_rgb565(pixel[2], pixel[1], pixel[0]);
            UINT8* finmsk = &pdSprMask[y * MAX_SPRITE_WIDTH + x];
            if (shrink)
            {
                UINT8* tmsk = &psSprMask[y * 2 * MAX_SPRITE_WIDTH + x * 2];
                if (tmsk[0] == tmsk[1] || tmsk[0] == tmsk[MAX_SPRITE_WIDTH] || tmsk[0] == tmsk[MAX_SPRITE_WIDTH + 1]) *finmsk = tmsk[0];
                else if (tmsk[1] == tmsk[MAX_SPRITE_WIDTH] || tmsk[1] == tmsk[MAX_SPRITE_WIDTH + 1]) *finmsk = tmsk[1];
                else if (tmsk[MAX_SPRITE_WIDTH] == tmsk[MAX_SPRITE_WIDTH + 1]) *finmsk = tmsk[MAX_SPRITE_WIDTH];
                else *finmsk = tmsk[0];
            }
            else *finmsk = psSprMask[y / 2 * MAX_SPRITE_WIDTH + x / 2];
        }
    }
}


UINT8 draw_color[4],under_draw_color[4];
void SetRenderDrawColor(UINT8 R, UINT8 G, UINT8 B, UINT8 A)
{
    draw_color[0] = R;
    draw_color[1] = G;
    draw_color[2] = B;
    draw_color[3] = A;
}
void SetRenderDrawColorv(UINT8* col3, UINT8 A)
{
    draw_color[0] = col3[0];
    draw_color[1] = col3[1];
    draw_color[2] = col3[2];
    draw_color[3] = A;
}
void SetRenderDrawColor565(UINT16 RGB565, UINT8 A)
{
    draw_color[0] = (UINT8)((RGB565 & 0b1111100000000000) >> 8);
    draw_color[1] = (UINT8)((RGB565 & 0b11111100000) >> 3);
    draw_color[2] = (UINT8)((RGB565 & 0b11111) << 3);
    draw_color[3] = A;
}
void Init_oFrame_Palette(UINT16* pPal)
{
    for (unsigned int ti = 0; ti < MycRom.noColors; ti++)
    {
        originalcolors[ti] = pPal[ti];
    }
}
void Init_cFrame_Palette2(void)
{
    for (unsigned int ti = 0; ti < MycRom.noColors; ti++)
    {
        MycRP.acEditColorsS[ti] = MycRP.Palette[ti] = originalcolors[ti] = 
            (UINT16)(31.0 * (float)ti / (float)(MycRom.noColors - 1)) << 11 |
            (UINT16)(31.0 * (float)ti / (float)(MycRom.noColors - 1)) << 5 |
            (UINT16)(0.0 * (float)ti / (float)(MycRom.noColors - 1));
    }
}

void FullGrayConvert(void)
{
    UndoAvailableN = 0;
    acUndoPos = 0;
    RedoAvailableN = 0;
    acRedoPos = 0;
    int tmaxw = max((int)MycRom.fWidth, (int)MycRom.fWidthX);
    int tmaxh = max((int)MycRom.fHeight, (int)MycRom.fHeightX);
    int tminw = min((int)MycRom.fWidth, (int)MycRom.fWidthX);
    int tminh = min((int)MycRom.fHeight, (int)MycRom.fHeightX);
    UINT16* tfr32, * tfr64, * tbg32, * tbg64;
    if ((int)MycRom.fHeight == tmaxh)
    {
        tfr64 = MycRom.cFrames;
        tfr32 = MycRom.cFramesX;
        tbg64 = MycRom.BackgroundFrames;
        tbg32 = MycRom.BackgroundFramesX;
    }
    else
    {
        tfr64 = MycRom.cFramesX;
        tfr32 = MycRom.cFrames;
        tbg64 = MycRom.BackgroundFramesX;
        tbg32 = MycRom.BackgroundFrames;
    }
    for (int ti = 0; ti < max(max((int)MycRom.nFrames, (int)MycRom.nBackgrounds), (int)MycRom.nSprites); ti++)
    {
        int posmaxfr = ti * tmaxw * tmaxh;
        int posminfr = ti * tminw * tminh;
        for (int tj = 0; tj < tmaxh; tj++)
        {
            for (int tk = 0; tk < tmaxw; tk++)
            {
                int posmaxpix = posmaxfr + tj * tmaxw + tk;
                int posminpix = posminfr + (tj / 2) * tminw + (tk / 2);
                if (tj % 2 == 0 && tk % 2 == 0)
                {
                    if (ti < (int)MycRom.nFrames) tfr32[posminpix] = rgb565togray565(tfr32[posminpix]);
                    if (ti < (int)MycRom.nBackgrounds) tbg32[posminpix] = rgb565togray565(tbg32[posminpix]);
                }
                if (ti < (int)MycRom.nFrames) tfr64[posmaxpix] = rgb565togray565(tfr64[posmaxpix]);
                if (ti < (int)MycRom.nBackgrounds) tbg64[posmaxpix] = rgb565togray565(tbg64[posmaxpix]);
            }
        }
        if (ti < (int)MycRom.nFrames)
        {
            for (int tj = 0; tj < MAX_DYNA_SETS_PER_FRAMEN * (int)MycRom.noColors; tj++)
            {
                MycRom.Dyna4Cols[ti * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + tj] = rgb565togray565(MycRom.Dyna4Cols[ti * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + tj]);
                MycRom.Dyna4ColsX[ti * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + tj] = rgb565togray565(MycRom.Dyna4ColsX[ti * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + tj]);
            }
            for (int tj = 2; tj < MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION; tj++)
            {
                MycRom.ColorRotations[ti * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION + tj] = rgb565togray565(MycRom.ColorRotations[ti * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION + tj]);
                MycRom.ColorRotationsX[ti * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION + tj] = rgb565togray565(MycRom.ColorRotationsX[ti * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION + tj]);
            }
        }
        if (ti < (int)MycRom.nSprites)
        {
            for (int tj = 0; tj < MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT; tj++)
            {
                MycRom.SpriteColored[ti * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + tj] = rgb565togray565(MycRom.SpriteColored[ti * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + tj]);
                MycRom.SpriteColoredX[ti * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + tj] = rgb565togray565(MycRom.SpriteColoredX[ti * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + tj]);
            }
        }
    }
    UpdateBSneeded = true;
    UpdateFSneeded = true;
    UpdateSSneeded = true;
}

UINT16 Brightness(UINT16 color, float coef)
{
    int Rin = (int)color >> 11;
    int Gin = (int)(color >> 5) & 0b111111;
    int Bin = (int)(color & 0b11111);
    int Rout = (int)((float)Rin * coef);
    int Gout = (int)((float)Gin * coef);
    int Bout = (int)((float)Bin * coef);
    if (coef > 1)
    {
        if (Rin == Rout) Rout++;
        if (Rout > 0b11111) Rout = 0b11111;
        if (Gin == Gout) Gout++;
        if (Gout > 0b111111) Gout = 0b111111;
        if (Bin == Bout) Bout++;
        if (Bout > 0b11111) Bout = 0b11111;
    }
    else if (coef < 1)
    {
        if (Rin == Rout) Rout--;
        if (Rout < 0) Rout = 0;
        if (Gin == Gout) Gout--;
        if (Gout < 0) Gout = 0;
        if (Bin == Bout) Bout--;
        if (Bout < 0) Bout = 0;
    }
    return (UINT16)(((UINT16)Rout << 11) + ((UINT16)Gout << 5) + (UINT16)Bout);
}
void ApplyBrightness(float coef)
{
    UINT fw, fh;
    UINT16* pfr;
    for (UINT tk = 0; tk < nSelFrames; tk++)
    {
        if (nEditExtraResolutionF)
        {
            if (MycRom.isExtraFrame[SelFrames[tk]] == 0) continue;
            fw = MycRom.fWidthX;
            fh = MycRom.fHeightX;
            pfr = &MycRom.cFramesX[SelFrames[tk]*fw*fh];
        }
        else
        {
            fw = MycRom.fWidth;
            fh = MycRom.fHeight;
            pfr = &MycRom.cFrames[SelFrames[tk] * fw * fh];
        }
        for (UINT tj = 0; tj < fh; tj++)
        {
            for (UINT ti = 0; ti < fw; ti++)
            {
                if (Copy_Mask[tj * fw + ti] > 0)
                    pfr[tj * fw + ti] = Brightness(pfr[tj * fw + ti], coef);
            }
        }
    }
}

#pragma endregion Color_Tools

#pragma region Editor_Tools

UINT nSameFramesPerMask[2 * (MAX_MASKS + 1)] = { 0 };

void Check_SameFrames_Masks(BOOL shapemode)
{
    nSameFramesPerMask[shapemode * MAX_MASKS] = CheckSameFrames(255, shapemode);
    for (UINT ti = 0; ti < MycRom.nCompMasks; ti++)
    {
        nSameFramesPerMask[shapemode * (MAX_MASKS + 1) + ti + 1] = CheckSameFrames(ti, shapemode);
    }
}

void Check_SameFrames_Masks_All(void)
{
    //BestMask = 0;
    Check_SameFrames_Masks(FALSE);
    Check_SameFrames_Masks(TRUE);
    /*for (UINT ti = 0; ti < MycRom.nCompMasks; ti++)
    {
        if (nSameFramesPerMask[ti] < nSameFramesPerMask[BestMask]) BestMask = ti;
        if (nSameFramesPerMask[ti + MAX_MASKS + 1] < nSameFramesPerMask[BestMask]) BestMask = ti + MAX_MASKS + 1;
    }
    char tbuf[256];
    if (BestMask == 0) strcpy_s(tbuf, 256, "Best No mask");
    else if (BestMask == 0) strcpy_s(tbuf, 256, "Best No mask (S)");
    else if (BestMask >= MAX_MASKS + 1) sprintf_s(tbuf, 256, "Best Mask #%i (S)");
    else sprintf_s(tbuf, 256, "Best Mask #%i");
    SetDlgItemTextA(hwTB, IDC_LISTSAMEFR, tbuf);*/
}

void Check_Commons()
{
    memset(Common_Mask, 1, 256 * 64);
    if (nSelFrames <= 1) return;
    for (UINT tj = 0; tj < MycRom.fHeight; tj++)
    {
        for (UINT ti = 0; ti < MycRom.fWidth; ti++)
        {
            for (UINT tk = 1; tk < nSelFrames; tk++)
            {
                if (MycRP.oFrames[SelFrames[0] * MycRom.fWidth * MycRom.fHeight + tj * MycRom.fWidth + ti] != MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + tj * MycRom.fWidth + ti])
                {
                    Common_Mask[tj * MycRom.fWidth + ti] = 0;
                    break;
                }
            }
        }
    }
}

void Delete_Frame(UINT32 nofr)
{
    if (nofr < MycRom.nFrames - 1)
    {
        int nfrdecal = MycRom.nFrames - nofr - 1;
        memcpy(&MycRom.HashCode[nofr], &MycRom.HashCode[nofr + 1], sizeof(UINT32) * nfrdecal);
        memcpy(&MycRom.CompMaskID[nofr], &MycRom.CompMaskID[nofr + 1], nfrdecal);
        memcpy(&MycRom.ShapeCompMode[nofr], &MycRom.ShapeCompMode[nofr + 1], nfrdecal);
        memcpy(&MycRom.isExtraFrame[nofr], &MycRom.isExtraFrame[nofr + 1], nfrdecal);
        UINT32 toffd = nofr * MycRom.fWidth * MycRom.fHeight;
        UINT32 toffs = (nofr + 1) * MycRom.fWidth * MycRom.fHeight;
        UINT32 toffdX = nofr * MycRom.fWidthX * MycRom.fHeightX;
        UINT32 toffsX = (nofr + 1) * MycRom.fWidthX * MycRom.fHeightX;
        memcpy(&MycRP.oFrames[toffd], &MycRP.oFrames[toffs], nfrdecal * MycRom.fWidth * MycRom.fHeight);
        memcpy(&MycRom.cFrames[toffd], &MycRom.cFrames[toffs], nfrdecal * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
        memcpy(&MycRom.cFramesX[toffdX], &MycRom.cFramesX[toffsX], nfrdecal * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
        memcpy(&MycRom.DynaMasks[toffd], &MycRom.DynaMasks[toffs], nfrdecal * MycRom.fWidth * MycRom.fHeight);
        memcpy(&MycRom.DynaMasksX[toffdX], &MycRom.DynaMasksX[toffsX], nfrdecal * MycRom.fWidthX * MycRom.fHeightX);
        memcpy(&MycRom.Dyna4Cols[nofr * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], &MycRom.Dyna4Cols[(nofr + 1) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], sizeof(UINT16) * nfrdecal * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        memcpy(&MycRom.Dyna4ColsX[nofr * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], &MycRom.Dyna4ColsX[(nofr + 1) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], sizeof(UINT16) * nfrdecal * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        memcpy(&MycRom.FrameSprites[nofr * MAX_SPRITES_PER_FRAME], &MycRom.FrameSprites[(nofr + 1) * MAX_SPRITES_PER_FRAME], MAX_SPRITES_PER_FRAME * nfrdecal);
        memcpy(&MycRom.FrameSpriteBB[nofr * MAX_SPRITES_PER_FRAME * 4], &MycRom.FrameSpriteBB[(nofr + 1) * MAX_SPRITES_PER_FRAME * 4], MAX_SPRITES_PER_FRAME * nfrdecal * 4 * sizeof(UINT16));
        memcpy(&MycRom.ColorRotations[nofr * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION], &MycRom.ColorRotations[(nofr + 1) * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION], sizeof(UINT16) * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * nfrdecal);
        memcpy(&MycRom.ColorRotationsX[nofr * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION], &MycRom.ColorRotationsX[(nofr + 1) * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION], sizeof(UINT16) * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * nfrdecal);
        memcpy(&MycRom.TriggerID[nofr], &MycRom.TriggerID[nofr + 1], sizeof(UINT32) * nfrdecal);
        memcpy(&MycRP.FrameDuration[nofr], &MycRP.FrameDuration[nofr + 1], sizeof(UINT32) * nfrdecal);
        memcpy(&MycRom.BackgroundID[nofr], &MycRom.BackgroundID[nofr + 1], sizeof(UINT16) * nfrdecal);
        memcpy(&MycRom.BackgroundMask[toffd], &MycRom.BackgroundMask[toffs], nfrdecal * MycRom.fWidth * MycRom.fHeight);
        memcpy(&MycRom.BackgroundMaskX[toffdX], &MycRom.BackgroundMaskX[toffsX], nfrdecal * MycRom.fWidthX * MycRom.fHeightX);

    }
    for (UINT32 ti = 0; ti < nSelFrames; ti++)
    {
        if (SelFrames[ti] > nofr) SelFrames[ti]--;
    }
    Del_Selection_Frame(nofr);
    for (int ti = 0; ti < nSameFrames; ti++)
    {
        if (SameFrames[ti] > (int)nofr) SameFrames[ti]--;
    }
    Del_Same_Frame(nofr);
    for (int ti = 0; ti < (int)MycRP.nSections; ti++)
    {
        if (MycRP.Section_Firsts[ti] > (int)nofr) MycRP.Section_Firsts[ti]--;
    }
    Del_Section_Frame(nofr);
    for (UINT ti = 0; ti < MycRom.nSprites; ti++)
    {
        if (MycRP.Sprite_Col_From_Frame[ti] > nofr) MycRP.Sprite_Col_From_Frame[ti]--;
        else if (MycRP.Sprite_Col_From_Frame[ti] == nofr) MycRP.SpriteRect[ti * 4] = 0xffff;
    }
    MycRom.nFrames--;
    if ((acFrame > nofr) && (acFrame > 0))
    {
        acFrame--;
        InitColorRotation();
        if (!isFrameSelected2(acFrame))
        {
            nSelFrames = 1;
            SetMultiWarningF();
            SelFrames[0] = acFrame;
        }
        UpdateNewacFrame();
        UpdateFSneeded = true;
    }
    if (acFrame >= MycRom.nFrames) acFrame = MycRom.nFrames - 1;
    if ((PreFrameInStrip > (int)nofr) && (PreFrameInStrip > 0)) PreFrameInStrip--;
    if (PreFrameInStrip >= (int)MycRom.nFrames) PreFrameInStrip = MycRom.nFrames - 1;
    MycRom.HashCode = (UINT32*)realloc(MycRom.HashCode, MycRom.nFrames * sizeof(UINT32));
    MycRom.CompMaskID = (UINT8*)realloc(MycRom.CompMaskID, MycRom.nFrames);
    MycRom.isExtraFrame = (UINT8*)realloc(MycRom.isExtraFrame, MycRom.nFrames);
    MycRP.oFrames = (UINT8*)realloc(MycRP.oFrames, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.cFrames = (UINT16*)realloc(MycRom.cFrames, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    MycRom.cFramesX = (UINT16*)realloc(MycRom.cFramesX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    MycRom.DynaMasks = (UINT8*)realloc(MycRom.DynaMasks, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.DynaMasksX = (UINT8*)realloc(MycRom.DynaMasksX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    MycRom.Dyna4Cols = (UINT16*)realloc(MycRom.Dyna4Cols, sizeof(UINT16) * MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
    MycRom.Dyna4ColsX = (UINT16*)realloc(MycRom.Dyna4ColsX, sizeof(UINT16) * MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
    MycRom.FrameSprites = (UINT8*)realloc(MycRom.FrameSprites, MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    MycRom.FrameSpriteBB = (UINT16*)realloc(MycRom.FrameSpriteBB, MycRom.nFrames * MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16));
    MycRom.ColorRotations = (UINT16*)realloc(MycRom.ColorRotations, MycRom.nFrames * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    MycRom.ColorRotationsX = (UINT16*)realloc(MycRom.ColorRotationsX, MycRom.nFrames * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    MycRom.TriggerID = (UINT32*)realloc(MycRom.TriggerID, MycRom.nFrames * sizeof(UINT32));
    MycRP.FrameDuration = (UINT32*)realloc(MycRP.FrameDuration, MycRom.nFrames * sizeof(UINT32));
    MycRom.BackgroundID = (UINT16*)realloc(MycRom.BackgroundID, MycRom.nFrames * sizeof(UINT16));
    MycRom.BackgroundMask = (UINT8*)realloc(MycRom.BackgroundMask, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.BackgroundMaskX = (UINT8*)realloc(MycRom.BackgroundMaskX, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    UpdateFSneeded = true;
}

DWORD lastrotTime[MAX_COLOR_ROTATIONN];
UINT8 acrotShift[MAX_COLOR_ROTATIONN];

void InitColorRotation(void)
{
    if (MycRom.name[0] == 0) return;
    DWORD actime = timeGetTime();
    prevAcFrame = (UINT)-1;
    for (UINT ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
    {
        lastrotTime[ti] = actime;
        acrotShift[ti] = 0;
    }
    InitColorRotation2();
}

UINT16 RotationsInBG[256 * 64][2];
DWORD lastrotTimeBG[MAX_COLOR_ROTATIONN];
UINT8 acrotShiftBG[MAX_COLOR_ROTATIONN];

void InitColorRotation2(void)
{
    if (MycRom.name[0] == 0) return;
    if (MycRom.nBackgrounds == 0) return;
    prevAcBG = (UINT16)-1;
    DWORD actime = timeGetTime();
    for (UINT ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
    {
        lastrotTimeBG[ti] = actime;
        acrotShiftBG[ti] = 0;
    }
}

void CheckSameFrames(void)
{
    UINT8* pmsk;
    if (MycRom.CompMaskID[acFrame] == 255) pmsk = NULL;
    else pmsk = &MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight];
    nSameFrames = 0;
    SendMessageA(GetDlgItem(hwTB, IDC_SAMEFRAMELIST), CB_RESETCONTENT, 0, 0);
    UINT8* pfrm = &MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight];
    bool isshape = MycRom.ShapeCompMode[acFrame];
    char tbuf[10];
    for (UINT32 tk = 0; tk < MycRom.nFrames; tk++)
    {
        if (tk == acFrame) continue;
        bool samefr = true;
        UINT8* pfrm2 = &MycRP.oFrames[tk * MycRom.fWidth * MycRom.fHeight];
        for (UINT32 ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
        {
            if (pmsk)
            {
                if (pmsk[ti] > 0) continue;
            }
            if (!isshape)
            {
                if (pfrm[ti] != pfrm2[ti])
                {
                    samefr = false;
                    break;
                }
            }
            else
            {
                if (((pfrm[ti] == 0) && (pfrm2[ti] > 0)) || ((pfrm[ti] > 0) && (pfrm2[ti] == 0)))
                {
                    samefr = false;
                    break;
                }
            }
        }
        if (!samefr) continue;
        if (nSameFrames < MAX_SAME_FRAMES)
        {
            _itoa_s(tk, tbuf, 9, 10);
            SendMessageA(GetDlgItem(hwTB, IDC_SAMEFRAMELIST), CB_ADDSTRING, 0, (LPARAM)tbuf);
            SameFrames[nSameFrames] = tk;
            nSameFrames++;
        }
    }
    if (nSameFrames == MAX_SAME_FRAMES) sprintf_s(tbuf, 9, ">=%i", MAX_SAME_FRAMES);
    else _itoa_s(nSameFrames, tbuf, 9, 10);
    SetDlgItemTextA(hwTB, IDC_SAMEFRAME, tbuf);
}

UINT CheckSameFrames(UINT8 nomsk, BOOL shapemode)
{
    UINT nsmfr = 0;
    UINT8* pmsk;
    if (nomsk == 255) pmsk = NULL;
    else pmsk = &MycRom.CompMasks[nomsk * MycRom.fWidth * MycRom.fHeight];
    nSameFrames = 0;
    UINT8* pfrm = &MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight];
    for (UINT32 tk = 0; tk < MycRom.nFrames; tk++)
    {
        if (tk == acFrame) continue;
        bool samefr = true;
        UINT8* pfrm2 = &MycRP.oFrames[tk * MycRom.fWidth * MycRom.fHeight];
        for (UINT32 ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
        {
            if (pmsk)
            {
                if (pmsk[ti] > 0) continue;
            }
            if (!shapemode)
            {
                if (pfrm[ti] != pfrm2[ti])
                {
                    samefr = false;
                    break;
                }
            }
            else
            {
                if (((pfrm[ti] == 0) && (pfrm2[ti] > 0)) || ((pfrm[ti] > 0) && (pfrm2[ti] == 0)))
                {
                    samefr = false;
                    break;
                }
            }
        }
        if (samefr) nsmfr++;
    }
    return nsmfr;
}

void Add_Surface_To_Mask(UINT8* Surface, bool isDel)
{
    if (MycRom.name[0] == 0) return;
    if (MycRom.CompMaskID[acFrame] == 255) return;
    for (UINT32 ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        if (Surface[ti] > 0)
        {
            if (!isDel)
                MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight + ti] = 1;
            else
                MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight + ti] = 0;
        }
    }
    CheckSameFrames();
}

void Add_Surface_To_Copy(UINT8* Surface, bool isDel)
{
    if (MycRom.name[0] == 0) return;
    UINT16* pcpyc;
    UINT8* pdynm;
    UINT fw, fh;
    if (nEditExtraResolutionF && Edit_Mode == 1)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pcpyc = &MycRom.cFramesX[acFrame * fw * fh];
        pdynm = &MycRom.DynaMasksX[acFrame * fw * fh];
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pcpyc = &MycRom.cFrames[acFrame * fw * fh];
        pdynm = &MycRom.DynaMasks[acFrame * fw * fh];
    }
    for (UINT32 ti = 0; ti < fw * fh; ti++)
    {
        if (Surface[ti] > 0)
        {
            if (!isDel) Copy_Mask[ti] = 1; else Copy_Mask[ti] = 0;
        }
        Copy_ColN[ti] = pcpyc[ti];
        Copy_Dyna[ti] = pdynm[ti];
    }
    for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
        Copy_Colo[ti] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
    GetSelectionSize();
}

void Add_Surface_To_Dyna(UINT8* Surface, bool isDel)
{
    if (MycRom.name[0] == 0) return;
    UINT8* pdyn;
    UINT16* pfra;
    UINT fw, fh;
    for (UINT tk = 0; tk < nSelFrames; tk++)
    {
        if (ExtraResFClicked)
        {
            if (MycRom.isExtraFrame[SelFrames[tk]] == 0) return;
            fw = MycRom.fWidthX;
            fh = MycRom.fHeightX;
            pdyn = &MycRom.DynaMasksX[SelFrames[tk] * fw * fh];
            pfra = &MycRom.cFramesX[SelFrames[tk] * fw * fh];
        }
        else
        {
            fw = MycRom.fWidth;
            fh = MycRom.fHeight;
            pdyn = &MycRom.DynaMasks[SelFrames[tk] * fw * fh];
            pfra = &MycRom.cFrames[SelFrames[tk] * fw * fh];
        }
        for (UINT tj = 0; tj < fh; tj++)
        {
            for (UINT ti = 0; ti < fw; ti++)
            {
                UINT i = ti, j = tj;
                if (ExtraResFClicked)
                {
                    if (fh == 64)
                    {
                        i = ti / 2;
                        j = tj / 2;
                    }
                    else
                    {
                        i = ti * 2;
                        j = tj * 2;
                    }
                }
                if (Surface[tj * fw + ti] > 0)
                {
                    if (!isDel) pdyn[tj * fw + ti] = acDynaSet;
                    else
                    {
                        if (pdyn[tj * fw + ti] < 255)
                            pfra[tj * fw + ti] = MycRP.Palette[MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + j * MycRom.fWidth + i]];
                        pdyn[tj * fw + ti] = 255;
                    }
                }
            }
        }
    }
}
void FreeCopyMasks(void)
{
    Copy_Content = 0;
    for (int tj = 0; tj < 64; tj++)
    {
        for (int ti = 0; ti < 256; ti++)
        {
            Draw_Extra_Surface[tj * 256 + ti] = Copy_Mask[tj * 256 + ti] = Paste_Mask[tj * 256 + ti] = 0;
        }
    }
    for (int tj = 0; tj < MAX_SPRITE_HEIGHT; tj++)
    {
        for (int ti = 0; ti < MAX_SPRITE_WIDTH; ti++)
        {
            Draw_Extra_Surface2[tj * 256 + ti] = 0;
        }
    }
    Copy_From_Frame = -1;
    Copy_From_DynaMask = -1;
    GetSelectionSize();
}

void Deactivate_Draw_All_Sel(void)
{
    isDrawAllSel = false;
    isMaskAllSel = false;
    SendMessage(hwTB, TB_CHECKBUTTON, BM_DRAWALL, MAKELONG(0, 0));
}

void InitVariables(void)
{
    Edit_Mode = 0;
    for (UINT ti = 0; ti < 15; ti++) PrevColors[ti] = 0;
    for (UINT ti = 0; ti < 15; ti++)
    {
        MycRP.Palette[ti] = originalcolors[ti];
        MycRP.acEditColorsS[ti] = originalcolors[ti];
    }
    noColSel = 0;
    PreFrameInStrip = 0;
    acFrame = 0;
    prevAcFrame = (UINT)-1;
    PreSpriteInStrip = 0;
    acSprite = 0;
    PreBGInStrip = 0;
    acBG = 0;
    CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
    CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
    prevAcBG = (UINT16)-1;
    InitColorRotation();
    prevFrame = 0;
    nSelFrames = 1;
    SetMultiWarningF();
    SetMultiWarningS();
    SelFrames[0] = 0;
    MultiSelectionActionWarning = false;
    isDrawAllSel = false;
    isMaskAllSel = false;
    ResetUndoService();
    FreeCopyMasks();
    UndoAvailableN = 0;
    RedoAvailableN = 0;
}

void UpdateNewacFrame(void)
{
    if (MycRom.name[0] == 0) return;
    int ti = Which_Section(acFrame);
    SetDlgItemTextA(hwTB, IDC_SECTIONNAME, &MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
    if (Edit_Mode == 0)
    {
        SendMessage(GetDlgItem(hwTB, IDC_SECTIONLIST), CB_SETCURSEL, ti + 1, 0);
        UINT8 puc = MycRom.CompMaskID[acFrame];
        puc++;
        SendMessage(GetDlgItem(hwTB, IDC_MASKLIST), CB_SETCURSEL, (WPARAM)puc, 0);
        if (MycRom.ShapeCompMode[acFrame]) SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_SETCHECK, BST_CHECKED, 0);
        else SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_SETCHECK, BST_UNCHECKED, 0);
        CheckSameFrames();
    }
    else
    {
        UpdateFrameSpriteList();
        SendMessage(GetDlgItem(hwTB, IDC_SECTIONLIST2), CB_SETCURSEL, ti + 1, 0);
        if (MycRom.isExtraFrame[acFrame] > 0) CheckDlgButton(hwTB, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB, IDC_EXTRARES, BST_UNCHECKED);
    }
    if (ExtraResFClicked && MycRom.isExtraFrame[acFrame] > 0) nEditExtraResolutionF = true; else nEditExtraResolutionF = false;
    InvalidateRect(hwTB, NULL, FALSE);
    Calc_Resize_Frame();
    if (MycRom.isExtraFrame[acFrame] > 0) CheckDlgButton(hwTB, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB, IDC_EXTRARES, BST_UNCHECKED);
    UpdateFrameBG();
}

int isFrameSelected(UINT noFr)
{
    for (unsigned int ti = 0; ti < nSelFrames; ti++)
    {
        if (SelFrames[ti] == noFr) return (int)ti;
    }
    return -1;
}
int isSpriteSelected(UINT nospr)
{
    if (acSprite == nospr && nSelSprites > 0) return -2;
    for (unsigned int ti = 0; ti < nSelSprites; ti++)
    {
        if (SelSprites[ti] == nospr) return (int)ti;
    }
    return -1;
}
int isFrameSelected3(UINT noFr)
{
    if ((acFrame == noFr) && (nSelFrames > 0)) return -2;
    for (unsigned int ti = 0; ti < nSelFrames; ti++)
    {
        if (SelFrames[ti] == noFr) return (int)ti;
    }
    return -1;
}

bool isFrameSelected2(UINT noFr)
{
    for (unsigned int ti = 0; ti < nSelFrames; ti++)
    {
        if (SelFrames[ti] == noFr) return true;
    }
    return false;
}

HBITMAP hMultiBitmapF = NULL, hMultiBitmapS = NULL;
void SetMultiWarningF(void)
{
    if (hMultiBitmapF) DeleteObject(hMultiBitmapF);
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    if (nSelFrames > 1) strcat_s(path, MAX_PATH, "\\icons\\MultiSelection.bmp");
    else strcat_s(path, MAX_PATH, "\\icons\\SingleSelection.bmp");
    hMultiBitmapF = (HBITMAP)LoadImageA(NULL, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    SendMessageW(GetDlgItem(hwTB, IDC_MULTIF), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hMultiBitmapF);
}
void SetMultiWarningS()
{
    if (hMultiBitmapS) DeleteObject(hMultiBitmapS);
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    if (nSelSprites > 1) strcat_s(path, MAX_PATH, "\\icons\\MultiSelection.bmp");
    else strcat_s(path, MAX_PATH, "\\icons\\SingleSelection.bmp");
    hMultiBitmapS = (HBITMAP)LoadImageA(NULL, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    SendMessageW(GetDlgItem(hwTB2, IDC_MULTIF), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hMultiBitmapS);
}
UINT8 ValuePlus2x2(UINT8* pbuf, UINT lineW)
{
    if (*pbuf == pbuf[1] || *pbuf == pbuf[lineW] || *pbuf == pbuf[lineW + 1]) return *pbuf;
    else if (pbuf[1] == pbuf[lineW] || pbuf[1] == pbuf[lineW + 1]) return pbuf[1];
    else if (pbuf[lineW] == pbuf[lineW + 1]) return pbuf[lineW];
    return *pbuf;
}

void Add_Selection_Frame(UINT nofr)
{
    if (nSelFrames == MAX_SEL_FRAMES)
    {
        acFrame = SelFrames[nSelFrames - 1];
        InitColorRotation();
        return;
    }
    if (isFrameSelected(nofr) == -1)
    {
        SelFrames[nSelFrames] = nofr;
        nSelFrames++;
    }
    SetMultiWarningF();
}

void Del_Selection_Frame(UINT nofr)
{
    int possel = isFrameSelected(nofr);
    if (possel == -1) return;
    if (possel < (int)nSelFrames - 1)
    {
        for (UINT ti = possel; ti < nSelFrames - 1; ti++) SelFrames[ti] = SelFrames[ti + 1];
    }
    nSelFrames--;
    SetMultiWarningF();
}

void Del_Selection_Sprite(UINT nospr)
{
    int possel = isSpriteSelected(nospr);
    if (possel == -1) return;
    if (possel < (int)nSelSprites - 1)
    {
        for (UINT ti = possel; ti < nSelSprites - 1; ti++) SelSprites[ti] = SelSprites[ti + 1];
    }
    nSelSprites--;
}

int isSameFrame(UINT noFr)
{
    for (int ti = 0; ti < nSameFrames; ti++)
    {
        if (SameFrames[ti] == noFr) return ti;
    }
    return -1;
}

void Del_Same_Frame(UINT nofr)
{
    int possame = isSameFrame(nofr);
    if (possame == -1) return;
    if (possame < (int)nSameFrames - 1)
    {
        for (int ti = possame; ti < nSameFrames - 1; ti++) SameFrames[ti] = SameFrames[ti + 1];
    }
    nSameFrames--;
}

int is_Section_First(UINT nofr)
{
    for (UINT ti = 0; ti < MycRP.nSections; ti++)
    {
        if (MycRP.Section_Firsts[ti] == nofr) return (int)ti;
    }
    return -1;
}

int Which_Section(UINT nofr)
{
    int tres = -1, tfirst = -1;
    for (int ti = 0; ti < (int)MycRP.nSections; ti++)
    {
        if (((int)MycRP.Section_Firsts[ti] > tfirst) && (MycRP.Section_Firsts[ti] <= nofr))
        {
            tfirst = (int)MycRP.Section_Firsts[ti];
            tres = ti;
        }
    }
    return tres;
}

void Delete_Section(int nosec)
{
    for (int ti = nosec; ti < (int)MycRP.nSections - 1; ti++)
    {
        MycRP.Section_Firsts[ti] = MycRP.Section_Firsts[ti+1];
        for (int tj = 0; tj < SIZE_SECTION_NAMES; tj++) MycRP.Section_Names[ti * SIZE_SECTION_NAMES + tj] = MycRP.Section_Names[(ti + 1) * SIZE_SECTION_NAMES + tj];
    }
    MycRP.nSections--;
}

void Delete_Sprite(int nospr)
{
    if (MycRom.nSprites == 0) return;
    if (nospr < (int)MycRom.nSprites - 1)
    {
        memmove(&MycRom.isExtraSprite[nospr], &MycRom.isExtraSprite[nospr + 1], MycRom.nSprites - 1 - nospr);
        memmove(&MycRom.SpriteOriginal[nospr * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteOriginal[(nospr + 1) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
        memmove(&MycRom.SpriteColored[nospr * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteColored[(nospr + 1) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
        memmove(&MycRom.SpriteMaskX[nospr * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteMaskX[(nospr + 1) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
        memmove(&MycRom.SpriteColoredX[nospr * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteColoredX[(nospr + 1) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
        memmove(&MycRP.Sprite_Col_From_Frame[nospr], &MycRP.Sprite_Col_From_Frame[nospr + 1], sizeof(UINT32) * (MycRom.nSprites - 1 - nospr));
        memmove(&MycRP.Sprite_Names[SIZE_SECTION_NAMES * nospr], &MycRP.Sprite_Names[SIZE_SECTION_NAMES * (nospr + 1)], (MycRom.nSprites - 1 - nospr) * SIZE_SECTION_NAMES);
        memmove(&MycRom.SpriteDetDwordPos[nospr * MAX_SPRITE_DETECT_AREAS], &MycRom.SpriteDetDwordPos[(nospr + 1) * MAX_SPRITE_DETECT_AREAS], sizeof(UINT16) * (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_DETECT_AREAS);
        memmove(&MycRom.SpriteDetDwords[nospr * MAX_SPRITE_DETECT_AREAS], &MycRom.SpriteDetDwords[(nospr + 1) * MAX_SPRITE_DETECT_AREAS], sizeof(UINT32) * (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_DETECT_AREAS);
        memmove(&MycRom.SpriteDetAreas[nospr * MAX_SPRITE_DETECT_AREAS*4], &MycRom.SpriteDetAreas[(nospr + 1) * MAX_SPRITE_DETECT_AREAS*4], sizeof(UINT16) * (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_DETECT_AREAS*4);
        memmove(&MycRP.SpriteRect[nospr * 4], &MycRP.SpriteRect[(nospr + 1) * 4], sizeof(UINT16) * 4 * (MycRom.nSprites - 1 - nospr));
        memmove(&MycRP.SpriteRectMirror[nospr * 2], &MycRP.SpriteRectMirror[(nospr + 1) * 2], sizeof(BOOL) * 2 * (MycRom.nSprites - 1 - nospr));
    }
    /*for (UINT32 ti = 0; ti < nSelSprites; ti++)
    {
        if ((int)SelSprites[ti] > nospr) SelSprites[ti]--;
    }*/
    Del_Selection_Sprite(nospr);
    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
    {
        for (UINT tj = 0; tj < MAX_SPRITES_PER_FRAME; tj++)
        {
            if (MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj] == nospr)
            {
                if (tj < MAX_SPRITES_PER_FRAME - 1)
                {
                    memmove(&MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj], &MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj + 1],
                        MAX_SPRITES_PER_FRAME - 1 - tj);
                    memmove(&MycRom.FrameSpriteBB[ti * MAX_SPRITES_PER_FRAME * 4 + tj * 4], &MycRom.FrameSpriteBB[ti * MAX_SPRITES_PER_FRAME * 4 + (tj + 1) * 4],
                        (MAX_SPRITES_PER_FRAME - 1 - tj) * 4 * sizeof(UINT16));
                }
                MycRom.FrameSprites[(ti + 1) * MAX_SPRITES_PER_FRAME - 1] = 255;
            }
        }
        for (UINT tj = 0; tj < MAX_SPRITES_PER_FRAME; tj++)
        {
            if (MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj] > nospr && MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj] != 255) MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj]--;
        }
    }
    MycRom.nSprites--;
    if (acSprite >= MycRom.nSprites) acSprite = MycRom.nSprites - 1;
    if ((PreSpriteInStrip > (int)nospr) && (PreSpriteInStrip > 0)) PreSpriteInStrip--;
    if (PreSpriteInStrip >= (int)MycRom.nSprites) PreSpriteInStrip = MycRom.nSprites - 1;
    if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
    MycRom.isExtraSprite = (UINT8*)realloc(MycRom.isExtraSprite, MycRom.nSprites);
    MycRom.SpriteOriginal = (UINT8*)realloc(MycRom.SpriteOriginal, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    MycRom.SpriteColored = (UINT16*)realloc(MycRom.SpriteColored, sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    MycRom.SpriteMaskX = (UINT8*)realloc(MycRom.SpriteMaskX, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    MycRom.SpriteColoredX = (UINT16*)realloc(MycRom.SpriteColoredX, sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    MycRom.SpriteDetDwordPos = (UINT16*)realloc(MycRom.SpriteDetDwordPos, sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_DETECT_AREAS);
    MycRom.SpriteDetDwords = (UINT32*)realloc(MycRom.SpriteDetDwords, sizeof(UINT32) * MycRom.nSprites * MAX_SPRITE_DETECT_AREAS);
    MycRom.SpriteDetAreas = (UINT16*)realloc(MycRom.SpriteDetAreas, sizeof(UINT16) * 4 * MycRom.nSprites * MAX_SPRITE_DETECT_AREAS);
}

int Duplicate_Section_Name(char* name)
{
    if (strcmp(name, "- None -") == 0) return 30000;
    for (UINT ti = 0; ti < MycRP.nSections; ti++)
    {
        if (strcmp(name, &MycRP.Section_Names[ti * SIZE_SECTION_NAMES]) == 0) return ti;
    }
    return -1;
}

int Duplicate_Sprite_Name(char* name)
{
    for (UINT ti = 0; ti < MycRom.nSprites; ti++)
    {
        if (strcmp(name, &MycRP.Sprite_Names[ti * SIZE_SECTION_NAMES]) == 0) return ti;
    }
    return -1;
}

void Del_Section_Frame(UINT nofr)
{
    int tsec = is_Section_First(nofr);
    if ((tsec > -1) && (is_Section_First(nofr + 1) > -1)) Delete_Section(tsec);
}

void Get_Mask_Pos(UINT x, UINT y, UINT* poffset, UINT8* pMask)
{
    *poffset = y * MycRom.fWidth / 8 + x / 8;
    *pMask = 0x80 >> (x % 8);
}

#pragma endregion Editor_Tools

#pragma region Window_Tools_And_Drawings

void Draw_Rectangle(GLFWwindow* glfwin, int ix, int iy, int fx, int fy)
{
    glfwMakeContextCurrent(glfwin);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_LOOP);
    glColor4ubv(draw_color);
    glVertex2i(ix, iy);
    glVertex2i(fx, iy);
    glVertex2i(fx, fy);
    glVertex2i(ix, fy);
    glEnd();
}

void Draw_Fill_Rectangle_Text(GLFWwindow* glfwin, int ix, int iy, int fx, int fy, UINT textID, float tx0, float tx1, float ty0, float ty1)
{
    glfwMakeContextCurrent(glfwin);
    glColor4ub(255, 255, 255, 255);
    glBindTexture(GL_TEXTURE_2D, textID);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBegin(GL_TRIANGLES);
    glTexCoord2f(tx0, ty0);
    glVertex2i(ix, iy);
    glTexCoord2f(tx1, ty0);
    glVertex2i(fx, iy);
    glTexCoord2f(tx0, ty1);
    glVertex2i(ix, fy);
    glTexCoord2f(tx0, ty1);
    glVertex2i(ix, fy);
    glTexCoord2f(tx1, ty0);
    glVertex2i(fx, iy);
    glTexCoord2f(tx1, ty1);
    glVertex2i(fx, fy);
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void Display_Avancement(float avanct, int step, int nsteps)
{
    float avancement = (step + avanct) / nsteps;
    Draw_Fill_Rectangle_Text(glfwframe, 0, 0, (int)(ScrWframe * avancement), ScrHframe, TxcRom, 0, avancement, 0, 1);
    gl33_SwapBuffers(glfwframe, false);
}

void Draw_Raw_Digit(UINT8 digit, UINT x, UINT y, UINT8* pbuf, UINT width, UINT height)
{
    UINT8* pdig = &Raw_Digit_Def[digit * RAW_DIGIT_W];
    UINT8* pdig2;
    const UINT dwid = 11 * RAW_DIGIT_W;
    UINT mx = x + RAW_DIGIT_W;
    if (mx > width) mx = width;
    UINT my = y + RAW_DIGIT_H;
    if (my > height) my = height;
    for (UINT tj = y; tj < my; tj++)
    {
        pdig2 = pdig;
        for (UINT ti = x; ti < mx; ti++)
        {
            float lval = (float)(255-(*pdig)) / 255.0f;
            UINT val = (UINT)((1.0f - lval) * pbuf[ti * 4 + tj * 4 * width] + lval * draw_color[0]);
            if (val > 255) pbuf[ti * 4 + tj * 4 * width] = 255; else pbuf[ti * 4 + tj * 4 * width] = val;
            val = (UINT)((1.0f - lval) * pbuf[ti * 4 + tj * 4 * width + 1] + lval * draw_color[1]);
            if (val > 255) pbuf[ti * 4 + tj * 4 * width + 1] = 255; else pbuf[ti * 4 + tj * 4 * width + 1] = val;
            val = (UINT)((1.0f - lval) * pbuf[ti * 4 + tj * 4 * width + 2] + lval * draw_color[2]);
            if (val > 255) pbuf[ti * 4 + tj * 4 * width + 2] = 255; else pbuf[ti * 4 + tj * 4 * width + 2] = val;
            pbuf[ti * 4 + tj * 4 * width + 3] = 255;
            pdig++;
        }
        pdig = pdig2 + dwid;
    }
}

void Draw_Raw_Number(UINT number, UINT x, UINT y, UINT8* pbuf, UINT width, UINT height)
{
    UINT div = 1000000000;
    bool started = false;
    UINT num = number;
    UINT tx = x;
    while (div > 0)
    {
        if (started || (num / div > 0) || (div == 1))
        {
            started = true;
            UINT8 digit = (UINT8)(num / div);
            Draw_Raw_Digit(digit, tx, y, pbuf, width, height);
            num = num - div * digit;
            tx += RAW_DIGIT_W;
        }
        div = div / 10;
    }
}

void Draw_Digit(UINT8 digit, UINT x, UINT y, float zoom)
{
    glColor4ubv(draw_color);
    glTexCoord2f(digit / 10.0f, 0);
    glVertex2i(x, y);
    glTexCoord2f((digit + 1) / 10.0f, 0);
    glVertex2i(x + (int)(zoom * DIGIT_TEXTURE_W), y);
    glTexCoord2f((digit + 1) / 10.0f, 1);
    glVertex2i(x + (int)(zoom * DIGIT_TEXTURE_W), y + (int)(zoom * DIGIT_TEXTURE_H));
    glTexCoord2f(digit / 10.0f, 1);
    glVertex2i(x, y + (int)(zoom * DIGIT_TEXTURE_H));
    glTexCoord2f(digit / 10.0f, 0);
    glVertex2i(x, y);
    glTexCoord2f((digit + 1) / 10.0f, 1);
    glVertex2i(x + (int)(zoom * DIGIT_TEXTURE_W), y + (int)(zoom * DIGIT_TEXTURE_H));
}

void ConvertSurfaceToFrame(UINT8* surface, bool isDel)
{
    UINT fw, fh;
    UINT16* pfr;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pfr = MycRom.cFramesX;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pfr = MycRom.cFrames;
    }
    for (UINT tj = 0; tj < fh; tj++)
    {
        for (UINT ti = 0; ti < fw; ti++)
        {
            if (surface[tj * fw + ti] != 0)
            {
                UINT16 col = MycRP.acEditColorsS[noColSel];
                for (UINT tk = 0; tk < nSelFrames; tk++)
                {
                    if (isDel)
                    {
                        if (!nEditExtraResolutionF)
                            col = originalcolors[MycRP.oFrames[SelFrames[tk] * fw * fh + tj * fw + ti]];
                        else
                        {
                            if (fh == 64)
                                col = originalcolors[MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + tj / 2 * MycRom.fWidth + ti / 2]];
                            else
                                col = originalcolors[MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + tj * 2 * MycRom.fWidth + ti * 2]];
                        }
                    }
                    else if (MycRP.DrawColMode == 1)
                    {
                        if (!nEditExtraResolutionF)
                            col = MycRP.acEditColorsS[MycRP.oFrames[SelFrames[tk] * fw * fh + tj * fw + ti]];
                        else
                        {
                            if (fh == 64)
                                col = MycRP.acEditColorsS[MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + tj / 2 * MycRom.fWidth + ti / 2]];
                            else
                                col = MycRP.acEditColorsS[MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + tj * 2 * MycRom.fWidth + ti * 2]];
                        }
                    }
                    pfr[SelFrames[tk] * fh * fw + tj * fw + ti] = col;
                }
            }
        }
    }
}
void ConvertSurfaceToDetection(UINT8* surface)
{
    UINT16 xmin = 0xffff, xmax = 0, ymin = 0xffff, ymax = 0;
    for (UINT tj = 0; tj < MAX_SPRITE_HEIGHT; tj++)
    {
        for (UINT ti = 0; ti < MAX_SPRITE_WIDTH; ti++)
        {
            if (surface[ti + tj * MAX_SPRITE_WIDTH] == TRUE)
            {
                if (xmin > ti) xmin = ti;
                if (ymin > tj) ymin = tj;
                if (xmax < ti) xmax = ti;
                if (ymax < tj) ymax = tj;
            }
        }
    }
    xmax = xmax - xmin + 1;
    ymax = ymax - ymin + 1;
    MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4] = xmin;
    MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4 + 1] = ymin;
    MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4 + 2] = xmax;
    MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4 + 3] = ymax;
}
void ConvertSurfaceToSprite(UINT8* surface)
{
    UINT16* pspr;
    if (nEditExtraResolutionS) pspr = MycRom.SpriteColoredX;
    else pspr = MycRom.SpriteColored;
    for (UINT ti = 0; ti < MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT; ti++)
    {
        UINT16 col = MycRP.acEditColorsS[noSprSel];
        if (surface[ti] == TRUE)
        {
            if ((!nEditExtraResolutionS && MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + ti] < 255) ||
                (nEditExtraResolutionS && MycRom.SpriteMaskX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + ti] < 255))
            {
                pspr[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + ti] = col;
            }
        }
    }
}

void ConvertCopyToGradient(int xd, int yd, int xf, int yf)
{
    float pscal[256 * 64];
    /*float psmin = 1000000;
    float psmax = -1000000;*/
    UINT fw, fh;
    UINT16* pfra;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pfra = MycRom.cFramesX;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pfra = MycRom.cFrames;
    }
    float vx = (float)xf - (float)xd;
    float vy = (float)yf - (float)yd;
    float vnorm = (float)sqrt(vx * vx + vy * vy);
    vx /= vnorm;
    vy /= vnorm;
    for (int tj = 0; tj < (int)fh; tj++)
    {
        for (int ti = 0; ti < (int)fw; ti++)
        {
            pscal[tj * fw + ti] = (float)(ti - xd) / vnorm * vx + (float)(tj - yd) / vnorm * vy;
        }
    }
    float range = (float)(max(Draw_Grad_Fin, Draw_Grad_Ini) - min(Draw_Grad_Fin, Draw_Grad_Ini) + 1);
    for (UINT ti = 0; ti < fw * fh; ti++)
    {
        if (Copy_Mask[ti] > 0)
        {
            for (UINT tj = 0; tj < nSelFrames; tj++)
            {
                UINT16 fcol;
                if (pscal[ti] <= 0) fcol = min(Draw_Grad_Fin, Draw_Grad_Ini);
                else if (pscal[ti] >= 1) fcol = max(Draw_Grad_Fin, Draw_Grad_Ini);
                else if ((pscal[ti] >= 0) && (pscal[ti] <= 1)) fcol = min(Draw_Grad_Ini, Draw_Grad_Fin) + (UINT16)((pscal[ti] * range));
                if (Draw_Grad_Opposite) fcol = min(Draw_Grad_Ini, Draw_Grad_Fin) + max(Draw_Grad_Fin, Draw_Grad_Ini) - fcol;
                pfra[SelFrames[tj] * fw * fh + ti] = MycRP.Palette[fcol];
            }
        }
    }
}

float distance(int cx, int cy, int x, int y)
{
    float vx = (float)cx - (float)x;
    float vy = (float)cy - (float)y;
    return (float)sqrt(vx * vx + vy * vy);
}

float distanceellipse(int cx, int cy, int x, int y, float ratio)
{
    float vx = (float)cx - (float)x;
    float vy = (float)cy - (float)y;
    return (float)sqrt(vx * vx + ratio * ratio * vy * vy);
}

void ConvertCopyToRadialGradient(int xd, int yd, int xf, int yf)
{
    UINT fw, fh;
    UINT16* pfra;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pfra = MycRom.cFramesX;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pfra = MycRom.cFrames;
    }
    float vnorm = distance(xf, yf, xd, yd);
    float range = (float)(max(Draw_Grad_Fin, Draw_Grad_Ini) - min(Draw_Grad_Fin, Draw_Grad_Ini) + 1);
    for (int tj = 0; tj < (int)fh; tj++)
    {
        for (int ti = 0; ti < (int)fw; ti++)
        {
            if (Copy_Mask[tj * fw + ti] > 0)
            {
                float rayon = distance(xd, yd, ti, tj);

                if (rayon >= vnorm)
                {
                    for (unsigned int tk = 0; tk < nSelFrames; tk++)
                    {
                        if (Draw_Grad_Opposite)
                            pfra[SelFrames[tk] * fw * fh + tj * fw + ti] = MycRP.Palette[min(Draw_Grad_Fin, Draw_Grad_Ini)];
                        else
                            pfra[SelFrames[tk] * fw * fh + tj * fw + ti] = MycRP.Palette[max(Draw_Grad_Fin, Draw_Grad_Ini)];
                    }
                }
                else
                {
                    for (unsigned int tk = 0; tk < nSelFrames; tk++) 
                    {
                        if (Draw_Grad_Opposite)
                            pfra[SelFrames[tk] * fw * fh + tj * fw + ti] = MycRP.Palette[max(Draw_Grad_Ini, Draw_Grad_Fin) - (int)((rayon / vnorm) * range)];
                        else
                            pfra[SelFrames[tk] * fw * fh + tj * fw + ti] = MycRP.Palette[min(Draw_Grad_Ini, Draw_Grad_Fin) + (int)((rayon / vnorm) * range)];
                    }
                }
            }
        }
    }
}

void ConvertCopyToEllipseRadialGradient(int xd, int yd, int xf, int yf)
{
    UINT fw, fh;
    UINT16* pfra;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pfra = MycRom.cFramesX;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pfra = MycRom.cFrames;
    }
    float vnorm = (float)abs(xf - xd);
    float ratio = vnorm / (float)abs(yf - yd);
    float range = (float)(max(Draw_Grad_Fin, Draw_Grad_Ini) - min(Draw_Grad_Fin, Draw_Grad_Ini) + 1);
    for (int tj = 0; tj < (int)fh; tj++)
    {
        for (int ti = 0; ti < (int)fw; ti++)
        {
            if (Copy_Mask[tj * fw + ti] > 0)
            {
                float rayon = distanceellipse(xd, yd, ti, tj, ratio);

                if (rayon >= vnorm)
                {
                    for (unsigned int tk = 0; tk < nSelFrames; tk++)
                    {
                        if (Draw_Grad_Opposite)
                            pfra[SelFrames[tk] * fw * fh + tj * fw + ti] = MycRP.Palette[min(Draw_Grad_Fin, Draw_Grad_Ini)];
                        else
                            pfra[SelFrames[tk] * fw * fh + tj * fw + ti] = MycRP.Palette[max(Draw_Grad_Fin, Draw_Grad_Ini)];
                    }
                }
                else
                {
                    for (unsigned int tk = 0; tk < nSelFrames; tk++)
                    {
                        if (Draw_Grad_Opposite)
                            pfra[SelFrames[tk] * fw * fh + tj * fw + ti] = MycRP.Palette[max(Draw_Grad_Ini, Draw_Grad_Fin) - (int)((rayon / vnorm) * range)];
                        else
                            pfra[SelFrames[tk] * fw * fh + tj * fw + ti] = MycRP.Palette[min(Draw_Grad_Ini, Draw_Grad_Fin) + (int)((rayon / vnorm) * range)];
                    }
                }
            }
        }
    }
}
void rgb565_to_rgb888(uint16_t rgb565, uint8_t* r, uint8_t* g, uint8_t* b)
{
    *r = ((rgb565 >> 8) & 0xF8) | ((rgb565 >> 13) & 0x07);
    *g = ((rgb565 >> 3) & 0xFC) | ((rgb565 >> 9) & 0x03);
    *b = ((rgb565 << 3) & 0xF8) | ((rgb565 >> 2) & 0x07);
}
void rgb565_to_rgb888(uint16_t rgb565, uint8_t* rgb888)
{
    rgb888[0] = ((rgb565 >> 8) & 0xF8) | ((rgb565 >> 13) & 0x07);
    rgb888[1] = ((rgb565 >> 3) & 0xFC) | ((rgb565 >> 9) & 0x03);
    rgb888[2] = ((rgb565 << 3) & 0xF8) | ((rgb565 >> 2) & 0x07);
}
COLORREF RGB565_to_RGB888(UINT16 rgb565) 
{
    uint8_t r8 = ((rgb565 >> 8) & 0xF8) | ((rgb565 >> 13) & 0x07);
    uint8_t g8 = ((rgb565 >> 3) & 0xFC) | ((rgb565 >> 9) & 0x03);
    uint8_t b8 = ((rgb565 << 3) & 0xF8) | ((rgb565 >> 2) & 0x07);
    COLORREF rgb888 = (b8 << 16) | (g8 << 8) | r8;
    return rgb888;
}
uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t r5 = r >> 3;
    uint8_t g6 = g >> 2;
    uint8_t b5 = b >> 3;

    uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;

    return rgb565;
}

void SetRGB888ColorFromRGB565(UINT8* pDst, UINT16 rgb565)
{
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;
    pDst[0] = r5 << 3;
    pDst[1] = g6 << 2;
    pDst[2] = b5 << 3;
}

void EmptyExtraSurface(void)
{
    memset(Draw_Extra_Surface, 0, 256 * 64);
}

void EmptyExtraSurface2(void)
{
    memset(Draw_Extra_Surface2, 0, MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
}

void putpixel(int x, int y, UINT8* surface, UINT8 color, bool coloronly, UINT16* frame)
{
    UINT fw, fh;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
    }
    if ((x < 0) || (x >= (int)fw) || (y < 0) || (y >= (int)fh)) return;
    if (coloronly)
    {
        if (frame[y * fw + x] == 0) return;
    }
    surface[y * fw + x] = color;
}

void putpixel2(int x, int y, UINT8* surface, UINT8 color)
{
    if ((x < 0) || (x >= (int)MAX_SPRITE_WIDTH) || (y < 0) || (y >= (int)MAX_SPRITE_HEIGHT)) return;
    surface[y * MAX_SPRITE_WIDTH + x] = color;
}

void drawline(int x, int y, int x2, int y2, UINT8* surface, UINT8 color, bool coloronly, UINT16* frame)
{
    int w = x2 - x;
    int h = y2 - y;
    int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
    if (w < 0) dx1 = -1; else if (w > 0) dx1 = 1;
    if (h < 0) dy1 = -1; else if (h > 0) dy1 = 1;
    if (w < 0) dx2 = -1; else if (w > 0) dx2 = 1;

    int longest = abs(w);
    int shortest = abs(h);

    if (!(longest > shortest))
    {
        longest = abs(h);
        shortest = abs(w);
        if (h < 0) dy2 = -1;
        else if (h > 0) dy2 = 1;
        dx2 = 0;
    }
    int numerator = longest >> 1;
    for (int i = 0; i <= longest; i++)
    {
        putpixel(x, y, surface, color, coloronly, frame);
        numerator += shortest;
        if (!(numerator < longest))
        {
            numerator -= longest;
            x += dx1;
            y += dy1;
        }
        else {
            x += dx2;
            y += dy2;
        }
    }
}

void drawline2(int x, int y, int x2, int y2, UINT8* surface, UINT8 color)
{
    int w = x2 - x;
    int h = y2 - y;
    int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
    if (w < 0) dx1 = -1; else if (w > 0) dx1 = 1;
    if (h < 0) dy1 = -1; else if (h > 0) dy1 = 1;
    if (w < 0) dx2 = -1; else if (w > 0) dx2 = 1;

    int longest = abs(w);
    int shortest = abs(h);

    if (!(longest > shortest))
    {
        longest = abs(h);
        shortest = abs(w);
        if (h < 0) dy2 = -1;
        else if (h > 0) dy2 = 1;
        dx2 = 0;
    }
    int numerator = longest >> 1;
    for (int i = 0; i <= longest; i++)
    {
        putpixel2(x, y, surface, color);
        numerator += shortest;
        if (!(numerator < longest))
        {
            numerator -= longest;
            x += dx1;
            y += dy1;
        }
        else {
            x += dx2;
            y += dy2;
        }
    }
}

void drawAllOctantsF(int xc, int yc, int xp, int yp, UINT8* surface, UINT8 color, bool coloronly, UINT16* frame)
{
    for (int x = 0; x <= xp; x++)
    {
        for (int y = 0; y <= yp; y++)
        {
            putpixel(xc + x, yc + y, surface, color, coloronly, frame);
            putpixel(xc - x, yc + y, surface, color, coloronly, frame);
            putpixel(xc + x, yc - y, surface, color, coloronly, frame);
            putpixel(xc - x, yc - y, surface, color, coloronly, frame);
            putpixel(xc + y, yc + x, surface, color, coloronly, frame);
            putpixel(xc - y, yc + x, surface, color, coloronly, frame);
            putpixel(xc + y, yc - x, surface, color, coloronly, frame);
            putpixel(xc - y, yc - x, surface, color, coloronly, frame);
        }
    }
}

void drawAllQuadrantsF(int xc, int yc, int xp, int yp, UINT8* surface, UINT8 color, bool coloronly, UINT16* frame)
{
    for (int x = 0; x <= xp; x++)
    {
        for (int y = 0; y <= yp; y++)
        {
            putpixel(xc + x, yc + y, surface, color, coloronly, frame);
            putpixel(xc - x, yc + y, surface, color, coloronly, frame);
            putpixel(xc + x, yc - y, surface, color, coloronly, frame);
            putpixel(xc - x, yc - y, surface, color, coloronly, frame);
        }
    }
}

void drawAllOctants(int xc, int yc, int x, int y, UINT8* surface, UINT8 color, bool coloronly, UINT16* frame)
{
    putpixel(xc + x, yc + y, surface, color, coloronly, frame);
    putpixel(xc - x, yc + y, surface, color, coloronly, frame);
    putpixel(xc + x, yc - y, surface, color, coloronly, frame);
    putpixel(xc - x, yc - y, surface, color, coloronly, frame);
    putpixel(xc + y, yc + x, surface, color, coloronly, frame);
    putpixel(xc - y, yc + x, surface, color, coloronly, frame);
    putpixel(xc + y, yc - x, surface, color, coloronly, frame);
    putpixel(xc - y, yc - x, surface, color, coloronly, frame);
}

void drawAllQuadrants(int xc, int yc, int x, int y, UINT8* surface, UINT8 color, bool coloronly, UINT16* frame)
{
    putpixel(xc + x, yc + y, surface, color, coloronly, frame);
    putpixel(xc - x, yc + y, surface, color, coloronly, frame);
    putpixel(xc + x, yc - y, surface, color, coloronly, frame);
    putpixel(xc - x, yc - y, surface, color, coloronly, frame);
}


void drawAllOctantsF2(int xc, int yc, int xp, int yp, UINT8* surface, UINT8 color)
{
    for (int x = 0; x <= xp; x++)
    {
        for (int y = 0; y <= yp; y++)
        {
            putpixel2(xc + x, yc + y, surface, color);
            putpixel2(xc - x, yc + y, surface, color);
            putpixel2(xc + x, yc - y, surface, color);
            putpixel2(xc - x, yc - y, surface, color);
            putpixel2(xc + y, yc + x, surface, color);
            putpixel2(xc - y, yc + x, surface, color);
            putpixel2(xc + y, yc - x, surface, color);
            putpixel2(xc - y, yc - x, surface, color);
        }
    }
}

void drawAllOctants2(int xc, int yc, int x, int y, UINT8* surface, UINT8 color)
{
    putpixel2(xc + x, yc + y, surface, color);
    putpixel2(xc - x, yc + y, surface, color);
    putpixel2(xc + x, yc - y, surface, color);
    putpixel2(xc - x, yc - y, surface, color);
    putpixel2(xc + y, yc + x, surface, color);
    putpixel2(xc - y, yc + x, surface, color);
    putpixel2(xc + y, yc - x, surface, color);
    putpixel2(xc - y, yc - x, surface, color);
}

void drawellipse(int xc, int yc, int rx, int ry, UINT8* surface, UINT8 color, BOOL filled, bool coloronly, UINT16* frame)
{
        float dx, dy, d1, d2, x, y;
        x = 0;
        y = (float)ry;

        d1 = (ry * ry)
            - (rx * rx * ry)
            + (0.25f * rx * rx);
        dx = 2 * ry * ry * x;
        dy = 2 * rx * rx * y;

        while (dx < dy)
        {

            if (!filled) drawAllQuadrants(xc, yc, (int)x, (int)y, surface, color, coloronly, frame); else drawAllQuadrantsF(xc, yc, (int)x, (int)y, surface, color, coloronly, frame);

            if (d1 < 0)
            {
                x++;
                dx = dx + (2 * ry * ry);
                d1 = d1 + dx + (ry * ry);
            }
            else
            {
                x++;
                y--;
                dx = dx + (2 * ry * ry);
                dy = dy - (2 * rx * rx);
                d1 = d1 + dx - dy + (ry * ry);
            }
        }

        d2 = ((ry * ry) * ((x + 0.5f) * (x + 0.5f)))
            + ((rx * rx) * ((y - 1) * (y - 1)))
            - (rx * rx * ry * ry);

        while (y >= 0)
        {

            if (!filled) drawAllQuadrants(xc, yc, (int)x, (int)y, surface, color, coloronly, frame); else drawAllQuadrantsF(xc, yc, (int)x, (int)y, surface, color, coloronly, frame);

            if (d2 > 0)
            {
                y--;
                dy = dy - (2 * rx * rx);
                d2 = d2 + (rx * rx) - dy;
            }
            else {
                y--;
                x++;
                dx = dx + (2 * ry * ry);
                dy = dy - (2 * rx * rx);
                d2 = d2 + dx - dy + (rx * rx);
            }
        }
}

void drawcircle(int xc, int yc, int r, UINT8* surface, UINT8 color, BOOL filled, bool coloronly, UINT16* frame)
{
    int x = 0, y = r;
    int d = 3 - 2 * r;
    if (!filled) drawAllOctants(xc, yc, x, y, surface, color, coloronly, frame); else drawAllOctantsF(xc, yc, x, y, surface, color, coloronly, frame);
    while (y >= x)
    {
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
            d = d + 4 * x + 6;
        if (!filled) drawAllOctants(xc, yc, x, y, surface, color, coloronly, frame); else drawAllOctantsF(xc, yc, x, y, surface, color, coloronly, frame);
    }
}

void drawcircle2(int xc, int yc, int r, UINT8* surface, UINT8 color, BOOL filled)
{
    int x = 0, y = r;
    int d = 3 - 2 * r;
    if (!filled) drawAllOctants2(xc, yc, x, y, surface, color); else drawAllOctantsF2(xc, yc, x, y, surface, color);
    while (y >= x)
    {
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
            d = d + 4 * x + 6;
        if (!filled) drawAllOctants2(xc, yc, x, y, surface, color); else drawAllOctantsF2(xc, yc, x, y, surface, color);
    }
}

void drawrectangle(int xd, int yd, int xf, int yf, UINT8* surface, UINT8 color, BOOL isfilled, bool coloronly, UINT16* frame)
{
    UINT fw, fh;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
    }
    int x = xd, y = yd, x2 = xf, y2 = yf;
    if (x < 0) x = 0;
    if (x2 < 0) x2 = 0;
    if (y < 0) y = 0;
    if (y2 < 0) y2 = 0;
    if (x >= (int)fw) x = fw - 1;
    if (x2 >= (int)fw) x2 = fw - 1;
    if (y >= (int)fh) y = fh - 1;
    if (y2 >= (int)fh) y2 = fh - 1;
    for (int tj = min(x, x2); tj <= max(x, x2); tj++)
    {
        for (int ti = min(y, y2); ti <= max(y, y2); ti++)
        {
            if ((tj != x) && (tj != x2) && (ti != y) && (ti != y2) && (!isfilled)) continue;
            if (coloronly)
            {
                if (frame[ti * fw + tj] == 0) continue;
            }
            surface[ti * fw + tj] = color;
        }
    }
}

void drawrectangle2(int xd, int yd, int xf, int yf, UINT8* surface, UINT8 color, BOOL isfilled)
{
    int x = xd, y = yd, x2 = xf, y2 = yf;
    if (x < 0) x = 0;
    if (x2 < 0) x2 = 0;
    if (y < 0) y = 0;
    if (y2 < 0) y2 = 0;
    if (x >= (int)MAX_SPRITE_WIDTH) x = MAX_SPRITE_WIDTH - 1;
    if (x2 >= (int)MAX_SPRITE_WIDTH) x2 = MAX_SPRITE_WIDTH - 1;
    if (y >= (int)MAX_SPRITE_HEIGHT) y = MAX_SPRITE_HEIGHT - 1;
    if (y2 >= (int)MAX_SPRITE_HEIGHT) y2 = MAX_SPRITE_HEIGHT - 1;
    for (int tj = min(x, x2); tj <= max(x, x2); tj++)
    {
        for (int ti = min(y, y2); ti <= max(y, y2); ti++)
        {
            if ((tj != x) && (tj != x2) && (ti != y) && (ti != y2) && (!isfilled)) continue;
            surface[ti * MAX_SPRITE_WIDTH + tj] = color;
        }
    }
}

void floodfill(int x, int y, UINT8* surface, UINT16 scolor, UINT8 sdyn, UINT8 dcolor, UINT fw, UINT fh, UINT8* pdyn, UINT16* pfra)
{
    if ((x < 0) || (x >= (int)fw) || (y < 0) || (y >= (int)fh)) return;
    if (pdyn[y * fw + x] != sdyn) return;
    if ((sdyn == 255) && (pfra[y * fw + x] != scolor)) return;
    if (surface[y * fw + x] == dcolor) return;
    surface[y * fw + x] = dcolor;
    floodfill(x + 1, y, surface, scolor, sdyn, dcolor, fw, fh, pdyn, pfra);
    floodfill(x - 1, y, surface, scolor, sdyn, dcolor, fw, fh, pdyn, pfra);
    floodfill(x, y + 1, surface, scolor, sdyn, dcolor, fw, fh, pdyn, pfra);
    floodfill(x, y - 1, surface, scolor, sdyn, dcolor, fw, fh, pdyn, pfra);
}

void floodfill2(int x, int y, UINT8* surface, UINT8 scolor, UINT8 dcolor)
{
    if ((x < 0) || (x >= (int)MycRom.fWidth) || (y < 0) || (y >= (int)MycRom.fHeight)) return;
    if (MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + y * MycRom.fWidth + x] != scolor) return;
    if (surface[y * MycRom.fWidth + x] == dcolor) return;
    surface[y * MycRom.fWidth + x] = dcolor;
    floodfill2(x + 1, y, surface, scolor, dcolor);
    floodfill2(x - 1, y, surface, scolor, dcolor);
    floodfill2(x, y + 1, surface, scolor, dcolor);
    floodfill2(x, y - 1, surface, scolor, dcolor);
}

void floodfill3(int x, int y, UINT8* surface, UINT16 scolor, UINT8 dcolor)
{
    UINT16* pspr;
    if (nEditExtraResolutionS) pspr = &MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
    else pspr = &MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
    if ((x < 0) || (x >= (int)MAX_SPRITE_WIDTH) || (y < 0) || (y >= (int)MAX_SPRITE_HEIGHT)) return;
    if ((!nEditExtraResolutionS && MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + y * MAX_SPRITE_WIDTH + x] == 255)||
        (nEditExtraResolutionS && MycRom.SpriteMaskX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + y * MAX_SPRITE_WIDTH + x] == 255)) return;
    if (pspr[y * MAX_SPRITE_WIDTH + x] != scolor) return;
    if (surface[y * MAX_SPRITE_WIDTH + x] == dcolor) return;
    surface[y * MAX_SPRITE_WIDTH + x] = dcolor;
    floodfill3(x + 1, y, surface, scolor, dcolor);
    floodfill3(x - 1, y, surface, scolor, dcolor);
    floodfill3(x, y + 1, surface, scolor, dcolor);
    floodfill3(x, y - 1, surface, scolor, dcolor);
}

void drawfill(int x, int y, UINT8* surface, UINT8 color)
{
    UINT fw, fh;
    UINT8* pdyn;
    UINT16* pfra;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pdyn = &MycRom.DynaMasksX[acFrame * fw * fh];
        pfra = &MycRom.cFramesX[acFrame * fw * fh];
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pdyn = &MycRom.DynaMasks[acFrame * fw * fh];
        pfra = &MycRom.cFrames[acFrame * fw * fh];
    }
    UINT16 searchcol = pfra[y * fw + x];
    UINT8 searchdyn = pdyn[y * fw + x];
    floodfill(x, y, surface, searchcol, searchdyn, color, fw, fh, pdyn, pfra);
}

void drawfill2(int x, int y, UINT8* surface, UINT8 color)
{
    UINT8 searchcol = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + y * MycRom.fWidth + x];
    floodfill2(x, y, surface, searchcol, color);
}

void drawfill3(int x, int y, UINT8* surface, UINT8 color)
{
    UINT16* pspr;
    if (nEditExtraResolutionS) pspr = &MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
    else pspr = &MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
    if (MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + y * MAX_SPRITE_WIDTH + x] == 255) return;
    UINT16 searchcol = pspr[y * MAX_SPRITE_WIDTH + x];
    floodfill3(x, y, surface, searchcol, color);
}

void Draw_Line(float x0, float y0, float x1, float y1)
{
    glVertex2i((int)x0, (int)y0);
    glVertex2i((int)x1, (int)y1);
}

void Draw_Over_From_Surface(UINT8* Surface, UINT8 val, float zoom, int ofx,int ofy, bool checkfalse, bool invert)
{
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor4ubv(draw_color);

    UINT fw, fh;
    if (Edit_Mode == 1 && nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
    }
    for (UINT tj = ofy; tj < fh; tj++)
    {
        for (UINT ti = ofx; ti < fw; ti++)
        {
            UINT tk = tj * fw + ti;
            if (((Surface[tk] != val) && (!invert)) || ((Surface[tk] == val) && (invert)))
            {
                if ((tj - ofy) == 0) Draw_Line((ti - ofx) * zoom, 0, ((ti - ofx) + 1) * zoom, 0);
                else if (((Surface[tk - fw] == val) && (!invert)) || ((Surface[tk - fw] != val) && (invert))) Draw_Line((ti - ofx) * zoom, (tj - ofy) * zoom, ((ti - ofx) + 1) * zoom, (tj - ofy) * zoom);

                if ((tj - ofy) == fh - 1) Draw_Line((ti - ofx) * zoom, fh * zoom, ((ti - ofx) + 1) * zoom, fh * zoom - 1);
                else if (((Surface[tk + fw] == val) && (!invert)) || ((Surface[tk + fw] != val) && (invert))) Draw_Line((ti - ofx) * zoom, ((tj - ofy) + 1) * zoom, ((ti - ofx) + 1) * zoom, ((tj - ofy) + 1) * zoom);

                if ((ti - ofx) == 0) Draw_Line(0, (tj - ofy) * zoom, 0, ((tj - ofy) + 1) * zoom - 1);
                else if (((Surface[tk - 1] == val) && (!invert)) || ((Surface[tk - 1] != val) && (invert))) Draw_Line((ti - ofx) * zoom, (tj - ofy) * zoom, (ti - ofx) * zoom, ((tj - ofy) + 1) * zoom);

                if ((ti - ofx) == fw - 1) Draw_Line(fw * zoom, (tj - ofy) * zoom, fw * zoom, ((tj - ofy) + 1) * zoom);
                else if (((Surface[tk + 1] == val) && (!invert)) || ((Surface[tk + 1] != val) && (invert))) Draw_Line(((ti - ofx) + 1) * zoom, (tj - ofy) * zoom, ((ti - ofx) + 1) * zoom, ((tj - ofy) + 1) * zoom);
            }
            else if (checkfalse)
            {
                Draw_Line((ti - ofx) * zoom, (tj - ofy) * zoom, ((ti - ofx) + 1) * zoom, ((tj - ofy) + 1) * zoom);
                Draw_Line((ti - ofx) * zoom, ((tj - ofy) + 1) * zoom, ((ti - ofx) + 1) * zoom, (tj - ofy) * zoom);
            }
        }
    }
    glEnd();
}

void Draw_Over_From_Rectangle(UINT16* Recta, float zoom, int ofx, int ofy)
{
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor4ubv(draw_color);

    for (UINT16 tj = Recta[1]; tj <= Recta[3]; tj++)
    {
        for (UINT16 ti = Recta[0]; ti <= Recta[2]; ti++)
        {
            if (tj == Recta[1]) Draw_Line((ti - ofx) * zoom, (tj - ofy) * zoom, ((ti - ofx) + 1) * zoom - 1, (tj - ofy) * zoom);

            if (tj == Recta[3]) Draw_Line((ti - ofx) * zoom, ((tj - ofy) + 1) * zoom - 1, ((ti - ofx) + 1) * zoom - 1, ((tj - ofy) + 1) * zoom - 1);

            if (ti == Recta[0]) Draw_Line((ti - ofx) * zoom, (tj - ofy) * zoom, (ti - ofx) * zoom, ((tj - ofy) + 1) * zoom - 1);

            if (ti == Recta[2]) Draw_Line(((ti - ofx) + 1) * zoom - 1, (tj - ofy) * zoom, ((ti - ofx) + 1) * zoom - 1, ((tj - ofy) + 1) * zoom - 1);

            Draw_Line((ti - ofx) * zoom, (tj - ofy) * zoom, ((ti - ofx) + 1) * zoom, ((tj - ofy) + 1) * zoom);
            Draw_Line((ti - ofx) * zoom, ((tj - ofy) + 1) * zoom, ((ti - ofx) + 1) * zoom, (tj - ofy) * zoom);
        }
    }
    glEnd();
}

void Draw_Over_From_Surface2(UINT8* Surface, UINT8 val, float zoom, int ofx, int ofy, bool checkfalse, bool invert)
{
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor4ubv(draw_color);
    for (UINT tj = 0; tj < MAX_SPRITE_HEIGHT; tj++)
    {
        for (UINT ti = 0; ti < MAX_SPRITE_WIDTH; ti++)
        {
            UINT tk = tj * MAX_SPRITE_WIDTH + ti;
            if (((Surface[tk] != val) && (!invert)) || ((Surface[tk] == val) && (invert)))
            {
                if (tj == 0) Draw_Line((ti - ofx) * zoom, 0, ((ti - ofx) + 1) * zoom, 0);
                else if (((Surface[tk - MAX_SPRITE_WIDTH] == val) && (!invert)) || ((Surface[tk - MAX_SPRITE_WIDTH] != val) && (invert))) Draw_Line((ti - ofx) * zoom, (tj - ofy) * zoom, ((ti - ofx) + 1) * zoom, (tj - ofy) * zoom);

                if (tj == MAX_SPRITE_HEIGHT - 1) Draw_Line((ti - ofx) * zoom, MAX_SPRITE_HEIGHT * zoom, ((ti - ofx) + 1) * zoom, MAX_SPRITE_HEIGHT * zoom - 1);
                else if (((Surface[tk + MAX_SPRITE_WIDTH] == val) && (!invert)) || ((Surface[tk + MAX_SPRITE_WIDTH] != val) && (invert))) Draw_Line((ti - ofx) * zoom, ((tj - ofy) + 1) * zoom, ((ti - ofx) + 1) * zoom, ((tj - ofy) + 1) * zoom);

                if (ti == 0) Draw_Line(0, (tj - ofy) * zoom, 0, ((tj - ofy) + 1) * zoom - 1);
                else if (((Surface[tk - 1] == val) && (!invert)) || ((Surface[tk - 1] != val) && (invert))) Draw_Line((ti - ofx) * zoom, (tj - ofy) * zoom, (ti - ofx) * zoom, ((tj - ofy) + 1) * zoom);

                if (ti == MAX_SPRITE_WIDTH - 1) Draw_Line(MAX_SPRITE_WIDTH * zoom, (tj - ofy) * zoom, MAX_SPRITE_WIDTH * zoom, ((tj - ofy) + 1) * zoom);
                else if (((Surface[tk + 1] == val) && (!invert)) || ((Surface[tk + 1] != val) && (invert))) Draw_Line(((ti - ofx) + 1) * zoom, (tj - ofy) * zoom, ((ti - ofx) + 1) * zoom, ((tj - ofy) + 1) * zoom);
            }
            else if (checkfalse)
            {
                Draw_Line((ti - ofx) * zoom, (tj - ofy) * zoom, ((ti - ofx) + 1) * zoom, ((tj - ofy) + 1) * zoom);
                Draw_Line((ti - ofx) * zoom, ((tj - ofy) + 1) * zoom, ((ti - ofx) + 1) * zoom, (tj - ofy) * zoom);
            }
        }
    }
    glEnd();
}

void Draw_Paste_Over(GLFWwindow* glfwin,UINT x,UINT y,float zoom)
{
    UINT TxCircle;
    glEnable(GL_BLEND);
    if (glfwin == glfwframe) TxCircle = TxCircleFr;
    else if (glfwin == glfwsprites) TxCircle = TxCircleSpr;
    else TxCircle = TxCircleBG;
    glBindTexture(GL_TEXTURE_2D, TxCircle);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    UINT fw, fh;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
    }
    else 
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
    }
    for (int tj = 0; tj < (int)Paste_Height; tj++)
    {
        for (int ti = 0; ti < (int)Paste_Width; ti++)
        {
            int i = ti;
            int j = tj;
            if (Paste_Mirror & 1) i = Paste_Width - 1 - i;
            if (Paste_Mirror & 2) j = Paste_Height - 1 - j;
            if (Paste_Mask[i + j * Paste_Width] == 0) continue;
            //if ((ti + ofx< 0) || (ti + ofx >= (int)fw) || (tj + ofy < 0) || (tj + ofy >= (int)fh)) continue;
            int cff = Copy_From_Frame;
            if (cff == -1) cff = acFrame;
            SetRenderDrawColor565(Paste_ColN[i + j * Paste_Width], mselcol);
            RenderDrawPoint(glfwin, x + (ti + paste_offsetx) * zoom, y + (tj + paste_offsety) * zoom, zoom);
        }
    }
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    SetRenderDrawColor(255, 255, 255, 255);
    Draw_Rectangle(glfwin, (int)(x + (paste_offsetx) * zoom), (int)(y + (paste_offsety) * zoom), (int)(x + (Paste_Width + paste_offsetx) * zoom), (int)(y + (Paste_Height + paste_offsety) * zoom));
}

void Draw_Number(UINT number, UINT x, UINT y, float zoom)
{
    UINT div = 1000000000;
    bool started = false;
    UINT num=number;
    UINT tx = x;
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxChiffres);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    while (div > 0)
    {
        if (started || (num / div > 0) || (div == 1))
        {
            started = true;
            UINT8 digit = (UINT8)(num / div);
            Draw_Digit(digit, tx, y, zoom);
            num = num - div * digit;
            tx += (UINT)(zoom * DIGIT_TEXTURE_W);
        }
        div = div / 10;
    }
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void SetViewport(GLFWwindow* glfwin)
{
    int Resx, Resy;
    glfwMakeContextCurrent(glfwin);
    glfwGetFramebufferSize(glfwin, &Resx, &Resy);
    glViewport(0, 0, Resx, Resy);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, Resx, Resy, 0, -2, 2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Calc_Resize_Frame(void)
{
    glfwMakeContextCurrent(glfwframe);
    RECT winrect;
    GetClientRect(hWnd, &winrect);
    ScrW = winrect.right;
    ScrH = winrect.bottom;
    NFrameToDraw = (int)((float)(ScrW - FRAME_STRIP_W_MARGIN) / (float)(256 + FRAME_STRIP_W_MARGIN)); // Calculate the number of frames to display in the strip
    FS_LMargin = (ScrW - (NFrameToDraw * (FRAME_STRIP_W_MARGIN + 256) + FRAME_STRIP_W_MARGIN)) / 2; // calculate the left and right margin in the strip
    if (MycRom.name[0])
    {
        if (MycRom.fWidth == 192)
        {
            NFrameToDraw = (int)((float)(ScrW - FRAME_STRIP_W_MARGIN) / (float)(192 + FRAME_STRIP_W_MARGIN)); // Calculate the number of frames to display in the strip
            FS_LMargin = (ScrW - (NFrameToDraw * (FRAME_STRIP_W_MARGIN + 192) + FRAME_STRIP_W_MARGIN)) / 2; // calculate the left and right margin in the strip
        }
    }
    int thei = winrect.bottom - (TOOLBAR_HEIGHT + 20) - FRAME_STRIP_HEIGHT - statusBarHeight - 20;
    int twid = winrect.right;
    int mul = 4;
    if (MycRom.name[0])
    {
        if (MycRom.fWidth == 192) mul = 3;
    }
    if (((float)twid / (float)thei) > mul) twid = thei * mul; else
    {
        thei = twid / mul;
        twid = thei * mul;
    }
    if (MycRom.name[0])
    {
        UINT fh, fw;
        if (nEditExtraResolutionF && Edit_Mode == 1)
        {
            fh = MycRom.fHeightX;
            fw = MycRom.fWidthX;
        }
        else
        {
            fh = MycRom.fHeight;
            fw = MycRom.fWidth;
        }
        frame_zoom = ((float)thei / (float)fh);
        twid = (UINT)(frame_zoom * fw);
        thei = (UINT)(frame_zoom * fh);
    }
    else
    {
        frame_zoom = ((float)thei / (float)32);
        twid = (UINT)(frame_zoom * 128);
        thei = (UINT)(frame_zoom * 32);
    }
    glfwSetWindowSize(glfwframe, twid, thei);
    ScrWframe = twid;
    ScrHframe = thei;
    int offsx = (winrect.right - twid) / 2;
    int offsy = TOOLBAR_HEIGHT + 10 + (winrect.bottom - (TOOLBAR_HEIGHT + 20) - thei - FRAME_STRIP_HEIGHT) / 2;
    glfwSetWindowPos(glfwframe, offsx, offsy);
    SetViewport(glfwframe);
    glfwMakeContextCurrent(glfwframestrip);
    glfwSetWindowSize(glfwframestrip, ScrW, FRAME_STRIP_HEIGHT);
    glfwSetWindowPos(glfwframestrip, 0, ScrH - FRAME_STRIP_HEIGHT - statusBarHeight);
    SetViewport(glfwframestrip);
}

void Calc_Resize_Sprite(void)
{
    glfwMakeContextCurrent(glfwsprites);
    RECT winrect;
    GetClientRect(hSprites, &winrect);
    ScrW2 = winrect.right;
    ScrH2 = winrect.bottom;
    NSpriteToDraw = (int)((float)(ScrW2 - FRAME_STRIP_W_MARGIN) / (float)(MAX_SPRITE_WIDTH + FRAME_STRIP_W_MARGIN)); // Calculate the number of frames to display in the strip
    SS_LMargin = (ScrW2 - (NSpriteToDraw * (FRAME_STRIP_W_MARGIN + MAX_SPRITE_WIDTH) + FRAME_STRIP_W_MARGIN)) / 2; // calculate the left and right margin in the strip
    int thei = winrect.bottom - (TOOLBAR_HEIGHT + 20) - FRAME_STRIP_HEIGHT2 - statusBarHeight - 20;
    int twid = winrect.right;
    float mul = 4;
    if (((float)twid / (float)thei) > mul) twid = (int)(thei * mul); else
    {
        thei = (int)(twid / mul);
        twid = (int)(thei * mul);
    }
    sprite_zoom = ((float)thei / (float)MAX_SPRITE_HEIGHT);
    twid = (int)(sprite_zoom * MAX_SPRITE_WIDTH);
    thei = (int)(sprite_zoom * MAX_SPRITE_HEIGHT);
    glfwSetWindowSize(glfwsprites, twid, thei);
    ScrWsprite = twid;
    ScrHsprite = thei;
    int offsx = (winrect.right - twid) / 2;
    int offsy = TOOLBAR_HEIGHT + 10 + (winrect.bottom - (TOOLBAR_HEIGHT + 20) - thei - FRAME_STRIP_HEIGHT2) / 2;
    glfwSetWindowPos(glfwsprites, offsx, offsy);
    SetViewport(glfwsprites);
    glfwMakeContextCurrent(glfwspritestrip);
    glfwSetWindowSize(glfwspritestrip, ScrW2, FRAME_STRIP_HEIGHT2);
    glfwSetWindowPos(glfwspritestrip, 0, ScrH2 - FRAME_STRIP_HEIGHT2 - statusBarHeight);
    SetViewport(glfwspritestrip);
}

void Calc_Resize_Image(void)
{
    glfwMakeContextCurrent(glfwimages);
    RECT winrect;
    GetClientRect(hImages, &winrect);
    ScrW3 = winrect.right;
    ScrH3 = winrect.bottom - (TOOLBAR_HEIGHT + 10) - statusBarHeight;
    glfwSetWindowSize(glfwimages, ScrW3, ScrH3) ;
    glfwSetWindowPos(glfwimages, 0, TOOLBAR_HEIGHT + 10);
    SetViewport(glfwimages);
}


unsigned char RGBMask[3] = { 255,255,255 };

void MaskCommonPoints(UINT8* surface)
{
    memset(surface, 1, MycRom.fWidth * MycRom.fHeight);
    if (nSelFrames < 2) return;
    for (UINT tj = 0; tj < MycRom.fWidth * MycRom.fHeight; tj++)
    {
        for (UINT ti = 1; ti < nSelFrames; ti++)
        {
            if (MycRP.oFrames[SelFrames[0] * MycRom.fWidth * MycRom.fHeight + tj] != MycRP.oFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight + tj])
            {
                surface[tj] = 0;
                break;
            }
        }
    }
}

void RenderDrawPointClip(GLFWwindow* glfwin, float x, float y, unsigned int xmax, unsigned int ymax, float zoom)
{
    if ((x > xmax) || (y > ymax)) return;
    if ((x + (int)zoom - 1 <= xmax) && (y + (int)zoom - 1 <= ymax))
    {
        glColor4ubv(draw_color);
        glTexCoord2f(0, 0);
        glVertex2f(x, y);
        glTexCoord2f(1, 0);
        glVertex2f(x + zoom - 1, y);
        glTexCoord2f(1, 1);
        glVertex2f(x + zoom - 1, y + zoom - 1);
        glTexCoord2f(0, 1);
        glVertex2f(x, y + zoom - 1);
        glTexCoord2f(0, 0);
        glVertex2f(x, y);
        glTexCoord2f(1, 1);
        glVertex2f(x + zoom - 1, y + zoom - 1);
        return;
    }
    float tx = x + zoom - 1;
    float ty = y + zoom - 1;
    float ttx = 1, tty = 1;
    if (tx > xmax)
    {
        tx = (float)xmax;
        ttx = (float)(xmax - x) / zoom;
    }
    if (ty > ymax)
    {
        ty = (float)ymax;
        tty = (float)(ymax - y) / zoom;
    }
    glColor4ubv(draw_color);
    glTexCoord2f(0, 0);
    glVertex2f(x, y);
    glTexCoord2f(ttx, 0);
    glVertex2f(tx, y);
    glTexCoord2f(ttx, tty);
    glVertex2f(tx, ty);
    glTexCoord2f(0, tty);
    glVertex2f(x, ty);
    glTexCoord2f(0, 0);
    glVertex2f(x, y);
    glTexCoord2f(ttx, tty);
    glVertex2f(tx, ty);
}

void RenderDrawPoint(GLFWwindow* glfwin, float x, float y, float zoom)
{
    glColor4ubv(draw_color);
    glTexCoord2f(0, 0);
    glVertex2f(x, y);
    glTexCoord2f(1, 0);
    glVertex2f(x + zoom - 1, y);
    glTexCoord2f(1, 1);
    glVertex2f(x + zoom - 1, y + zoom - 1);
    glTexCoord2f(0, 1);
    glVertex2f(x, y + zoom - 1);
    glTexCoord2f(0, 0);
    glVertex2f(x, y);
    glTexCoord2f(1, 1);
    glVertex2f(x + zoom - 1, y + zoom - 1);
}

UINT16 RotationsInFrame[256 * 64][2];
bool isColorInRotation(UINT16 nocol, UINT nofr, bool isextra, UINT8* pnorot, UINT16* pnorotcol)
{
    UINT16* prot;
    *pnorot = 0xff;
    *pnorotcol = 0xffff;
    if (isextra && MycRom.isExtraFrame[nofr] == 0) return false;
    for (UINT8 ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
    {
        if (isextra) prot = &MycRom.ColorRotationsX[(nofr * MAX_COLOR_ROTATIONN + ti) * MAX_LENGTH_COLOR_ROTATION];
        else prot = &MycRom.ColorRotations[(nofr * MAX_COLOR_ROTATIONN + ti) * MAX_LENGTH_COLOR_ROTATION];
        if (*prot == 0) continue;
        for (UINT16 tj = 2; tj < (*prot) + 2; tj++)
        {
            if (prot[tj] == nocol)
            {
                *pnorot = ti;
                *pnorotcol = tj - 2; 
                return true;
            }
        }
    }
    return false;
}
void CheckNewRotation(int x, int y, UINT16 col)
{
    UINT8 norot;
    UINT16 norotcol;
    UINT fw;
    if (nEditExtraResolutionF) fw = MycRom.fWidthX; else fw = MycRom.fWidth;
    if (isColorInRotation(col, acFrame, nEditExtraResolutionF, &norot, &norotcol))
    {
        RotationsInFrame[y * fw + x][0] = (UINT16)norot;
        RotationsInFrame[y * fw + x][1] = norotcol;
    }
    else RotationsInFrame[y * fw + x][0] = 0xffff;
}

void Predraw_Frame_For_Rotations(unsigned int nofr)
{
    if (MycRom.name[0] == 0 || MycRom.nFrames == 0) return;
    UINT fw, fh;
    UINT16* pfr;
    UINT8* pBGm = NULL;
    UINT16* pBG = NULL;
    UINT16 BGID = MycRom.BackgroundID[acFrame];
    UINT8* pDYNs;
    UINT16* pDYNc = NULL;
    DWORD lt = timeGetTime();
    for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
    {
        lastrotTime[ti] = lt;
        acrotShift[ti] = 0;
    }
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pfr = &MycRom.cFramesX[nofr * fw * fh];
        if (BGID < MycRom.nBackgrounds && MycRom.isExtraBackground[BGID] > 0)
        {
            pBG = &MycRom.BackgroundFramesX[BGID * fw * fh];
            pBGm = &MycRom.BackgroundMaskX[nofr * fw * fh];
        }
        pDYNs = &MycRom.DynaMasksX[nofr * fw * fh];
        pDYNc = &MycRom.Dyna4ColsX[nofr * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors];
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pfr = &MycRom.cFrames[nofr * fw * fh];
        if (BGID < MycRom.nBackgrounds)
        {
            pBG = &MycRom.BackgroundFrames[BGID * fw * fh];
            pBGm = &MycRom.BackgroundMask[nofr * fw * fh];
        }
        pDYNs = &MycRom.DynaMasks[nofr * fw * fh];
        pDYNc = &MycRom.Dyna4Cols[nofr * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors];
    }
    for (UINT tj = 0; tj < fh; tj++)
    {
        for (UINT ti = 0; ti < fw; ti++)
        {
            int tk = ti, tl = tj;
            if (nEditExtraResolutionF && fh == 64)
            {
                tk /= 2;
                tl /= 2;
            }
            else if (nEditExtraResolutionF)
            {
                tk *= 2;
                tl *= 2;
            }
            UINT16 finalcol;
            UINT coltype;
            if (pBG != NULL && (MycRP.oFrames[nofr * MycRom.fWidth * MycRom.fHeight + tl * MycRom.fWidth + tk] == 0) &&
                (pBGm[tj * fw + ti] > 0))
            {
                //SetRenderDrawColor565(pBG[tj * fw + ti], 255);
                finalcol = pBG[tj * fw + ti];
                coltype = 0;
            }
            else
            {
                UINT8 nodynaset = pDYNs[tj * fw + ti];
                if (nodynaset == 255) ///SetRenderDrawColor565(pfr[tj * fw + ti], 255);
                {
                    finalcol = pfr[tj * fw + ti];
                    coltype = 1;
                }
                else //SetRenderDrawColor565(pDYNc[nodynaset * MycRom.noColors + MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + tl * MycRom.fWidth + tk]], 255);
                {
                    finalcol = pDYNc[nodynaset * MycRom.noColors + MycRP.oFrames[nofr * MycRom.fWidth * MycRom.fHeight + tl * MycRom.fWidth + tk]];
                    coltype = 2;
                }
            }
            UINT8 norot;
            UINT16 norotcol;
            if ((coltype != 2) && isColorInRotation(finalcol, nofr, nEditExtraResolutionF, &norot, &norotcol))
            {
                RotationsInFrame[tj * fw + ti][0] = (UINT16)norot;
                RotationsInFrame[tj * fw + ti][1] = norotcol;
            }
            else RotationsInFrame[tj * fw + ti][0] = 0xffff;
        }
    }
}
void CheckAcFrameChanged(void)
{
    if (acFrame == prevAcFrame && prevEditExtraResolutionF == nEditExtraResolutionF) return;
    prevAcFrame = acFrame;
    prevEditExtraResolutionF = nEditExtraResolutionF;
    Predraw_Frame_For_Rotations(acFrame);
    Predraw_BG_For_Rotations(acBG);
}
void Draw_Frame(GLFWwindow* glfwin, float zoom, unsigned int ofx, unsigned int ofy, unsigned int nofr, unsigned int x, unsigned int y, bool original)
{
    if (nofr >= MycRom.nFrames)
    {
        cprintf(true, "Unknown frame requested in Draw_Frame");
        return;
    }
    CheckAcFrameChanged();
    glfwMakeContextCurrent(glfwin);
    glEnable(GL_BLEND);
    if (!original && ExtraResFClicked && MycRom.isExtraFrame[acFrame] == 0)
    {
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLES);
        glColor4f(1, 0, 0, 1);
        glVertex2i(10, 0);
        glVertex2i(ScrWframe, ScrHframe - 10);
        glVertex2i(ScrWframe - 10, ScrHframe);
        glVertex2i(10, 0);
        glVertex2i(ScrWframe - 10, ScrHframe);
        glVertex2i(0, 10);
        glVertex2i(ScrWframe - 10, 0);
        glVertex2i(ScrWframe, 10);
        glVertex2i(10, ScrHframe);
        glVertex2i(ScrWframe - 10, 0);
        glVertex2i(10, ScrHframe);
        glVertex2i(0, ScrHframe - 10);
        glEnd();
        return;
    }
    glBindTexture(GL_TEXTURE_2D, TxCircleFr);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    UINT16* pFrame, * pcolrot, (*pBG) = NULL, *pDYNc;
    UINT fw, fh;
    UINT8 (*pBGm) = NULL, * pDYNs;
    UINT16 BGID = MycRom.BackgroundID[nofr];
    if (nEditExtraResolutionF && !original)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pFrame = &MycRom.cFramesX[nofr * fw * fh];
        pcolrot = &MycRom.ColorRotationsX[nofr * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
        if (BGID < MycRom.nBackgrounds && MycRom.isExtraBackground[BGID] > 0)
        {
            pBG = &MycRom.BackgroundFramesX[BGID * fw * fh];
            pBGm = &MycRom.BackgroundMaskX[nofr * fw * fh];
        }
        pDYNs = &MycRom.DynaMasksX[nofr * fw * fh];
        pDYNc = &MycRom.Dyna4ColsX[nofr * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors];
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pFrame = &MycRom.cFrames[nofr * fw * fh];
        pcolrot = &MycRom.ColorRotations[nofr * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
        if (BGID < MycRom.nBackgrounds)
        {
            pBG = &MycRom.BackgroundFrames[BGID * fw * fh];
            pBGm = &MycRom.BackgroundMask[nofr * fw * fh];
        }
        pDYNs = &MycRom.DynaMasks[nofr * fw * fh];
        pDYNc = &MycRom.Dyna4Cols[nofr * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors];
    }
    UINT8* pFrameo = &MycRP.oFrames[nofr * MycRom.fWidth * MycRom.fHeight];
    if (!original)
    {
        DWORD actime = timeGetTime();
        for (UINT ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
        {
            if (pcolrot[ti * MAX_LENGTH_COLOR_ROTATION] > 0)
            {
                if (actime >= lastrotTime[ti] + (DWORD)pcolrot[ti * MAX_LENGTH_COLOR_ROTATION + 1])
                {
                    lastrotTime[ti] = actime;
                    acrotShift[ti]++;
                    if (acrotShift[ti] >= (UINT8)pcolrot[ti * MAX_LENGTH_COLOR_ROTATION]) acrotShift[ti] = 0;
                }
            }
        }
    }
    for (unsigned int tj = ofy; tj < fh; tj++)
    {
        for (unsigned int ti = ofx; ti < fw; ti++)
        {
            if (!original)
            {
                int tk = ti, tl = tj;
                if (nEditExtraResolutionF && fh == 64)
                {
                    tk /= 2;
                    tl /= 2;
                }
                else if (nEditExtraResolutionF)
                {
                    tk *= 2;
                    tl *= 2;
                }
                UINT16 norot = RotationsInFrame[tj * fw + ti][0];
                if (norot < 0xffff)
                {
                    UINT16 nocolinrot = RotationsInFrame[tj * fw + ti][1];
                    UINT16 collength = pcolrot[norot * MAX_LENGTH_COLOR_ROTATION];
                    SetRenderDrawColor565(pcolrot[norot * MAX_LENGTH_COLOR_ROTATION +
                        (nocolinrot + acrotShift[norot]) % collength + 2], 255);
                }
                else if (pBG != NULL && (MycRP.oFrames[nofr * MycRom.fWidth * MycRom.fHeight + tl * MycRom.fWidth + tk] == 0) &&
                    (pBGm[tj * fw + ti] > 0))
                    SetRenderDrawColor565(pBG[tj * fw + ti], 255);
                else
                {
                    UINT8 nodynaset = pDYNs[tj * fw + ti];
                    if (nodynaset == 255) SetRenderDrawColor565(pFrame[tj * fw + ti], 255);
                    else
                        SetRenderDrawColor565(pDYNc[nodynaset * MycRom.noColors + MycRP.oFrames[nofr * MycRom.fWidth * MycRom.fHeight + tl * MycRom.fWidth + tk]], 255);
                }
            }
            else
            {
                UINT16 tcol = originalcolors[pFrameo[ti + tj * MycRom.fWidth]];
                if ((Common_Pushed) && (Common_Mask[tj * MycRom.fWidth + ti] == 0))
                {
                    unsigned char red, green, blue;
                    rgb565_to_rgb888(tcol, &red, &green, &blue);
                    red = (unsigned char)(red * (float)mselcol / 255.0f + Common_Color[0] * (float)(255 - mselcol) / 255.0f);
                    green = (unsigned char)(green * (float)mselcol / 255.0f + Common_Color[1] * (float)(255 - mselcol) / 255.0f);
                    blue = (unsigned char)(blue * (float)mselcol / 255.0f + Common_Color[2] * (float)(255 - mselcol) / 255.0f);
                    tcol = rgb888_to_rgb565(red, green, blue);
                }
                SetRenderDrawColor565(tcol, 255);
            }
            RenderDrawPointClip(glfwin, x + (ti - ofx) * zoom, y + (tj - ofy) * zoom, ScrWframe - 1, ScrHframe - 1, zoom);
        }
    }
    glEnd();
    if ((original) && (MycRom.CompMaskID[acFrame] < 255))
    {
        SetRenderDrawColor(255, 0, 255, 255);
        Draw_Over_From_Surface(&MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight], 0, zoom, 0, 0, true, true);
    }
    else if (!original)
    {
        if (Ident_Pushed)
        {
            SetRenderDrawColor(mselcol, 0, mselcol, 255);
            Draw_Over_From_Surface(pDYNs, acDynaSet, zoom, ofx,ofy, true, false);
        }
        int sprsel = (int)SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_GETCURSEL, 0, 0);
        if (BBIdent_Pushed && (sprsel != CB_ERR))
        {
            SetRenderDrawColor(255 - mselcol, 255 - mselcol, 0, 255 - mselcol);
            if (!nEditExtraResolutionF) Draw_Over_From_Rectangle(&MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + sprsel) * 4], zoom,ofx,ofy);
            else
            {
                UINT16 sprBB[4];
                if (fh == 64)
                {
                    sprBB[0] = MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + sprsel) * 4] * 2;
                    sprBB[1] = MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + sprsel) * 4 + 1] * 2;
                    sprBB[2] = MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + sprsel) * 4 + 2] * 2 + 1;
                    sprBB[3] = MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + sprsel) * 4 + 3] * 2 + 1;
                }
                else
                {
                    sprBB[0] = MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + sprsel) * 4] / 2;
                    sprBB[1] = MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + sprsel) * 4 + 1] / 2;
                    sprBB[2] = MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + sprsel) * 4 + 2] / 2;
                    sprBB[3] = MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + sprsel) * 4 + 3] / 2;
                }
                Draw_Over_From_Rectangle(sprBB, zoom, ofx, ofy);
            }
        }
        //else
        {
            SetRenderDrawColor(mselcol, 0, mselcol, mselcol);
            Draw_Over_From_Surface(pDYNs, 255, zoom, ofx, ofy, false, true);
            if (Paste_Mode) Draw_Paste_Over(glfwin, x, y, zoom);
            else
            {
                SetRenderDrawColor(0, mselcol, mselcol, mselcol);
                Draw_Over_From_Surface(Copy_Mask, 0, zoom, ofx, ofy, true, true);
            }
        }
    }
}
void Draw_Frame_Strip(void)
{
    float RTexCoord = (float)ScrW / (float)MonWidth;
    glfwMakeContextCurrent(glfwframestrip);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxFrameStrip[acFSText]);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    glColor4f(1, 1, 1, 1);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 0);
    glVertex2i(ScrW, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW, FRAME_STRIP_HEIGHT);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW, FRAME_STRIP_HEIGHT);
    glTexCoord2f(0, 1);
    glVertex2i(0, FRAME_STRIP_HEIGHT);
    glEnd();
}

void Draw_Sprite_Strip(void)
{
    float RTexCoord = (float)ScrW2 / (float)MonWidth;
    glfwMakeContextCurrent(glfwspritestrip);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[acSSText]);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    glColor4f(1, 1, 1, 1);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 0);
    glVertex2i(ScrW2, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW2, FRAME_STRIP_HEIGHT2);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW2, FRAME_STRIP_HEIGHT2);
    glTexCoord2f(0, 1);
    glVertex2i(0, FRAME_STRIP_HEIGHT2);
    glEnd();
}

void CalcPosSlider(void)
{
    float div = (float)MycRom.nFrames / (float)SliderWidth;
    PosSlider = (int)((float)PreFrameInStrip / div);
}
void Get_Frame_Strip_Line_Color(UINT pos)
{
    UINT corframe = (int)((float)(pos - FRAME_STRIP_SLIDER_MARGIN) * (float)MycRom.nFrames / (float)SliderWidth);
    UINT cornextframe = (int)((float)(pos - FRAME_STRIP_SLIDER_MARGIN + 1) * (float)MycRom.nFrames / (float)SliderWidth);
    if (cornextframe == corframe) cornextframe++;
    bool issel = false, isac = false;
    for (UINT ti = corframe; ti < cornextframe; ti++)
    {
        int iFS = isFrameSelected3(ti);
        if (iFS == -2) isac = true;
        if (iFS > -1) issel = true;
    }
    if (isac == true)
    {
        draw_color[0] = acColor[0];
        draw_color[1] = acColor[1];
        draw_color[2] = acColor[2];
    }
    else if (issel == true)
    {
        draw_color[0] = SelColor[0];
        draw_color[1] = SelColor[1];
        draw_color[2] = SelColor[2];
    }
    else
    {
        draw_color[0] = UnselColor[0];
        draw_color[1] = UnselColor[1];
        draw_color[2] = UnselColor[2];
    }
    issel = false;
    for (UINT ti = corframe; ti < cornextframe; ti++)
    {
        if (isSameFrame(ti) > -1) issel = true;
    }
    if (issel == true)
    {
        under_draw_color[0] = SameColor[0];
        under_draw_color[1] = SameColor[1];
        under_draw_color[2] = SameColor[2];
    }
    else
    {
        under_draw_color[0] = UnselColor[0];
        under_draw_color[1] = UnselColor[1];
        under_draw_color[2] = UnselColor[2];
    }
}

void Frame_Strip_Update(void)
{
    if (pFrameStrip) memset(pFrameStrip, 0, MonWidth * FRAME_STRIP_HEIGHT * 4);
    if (MycRom.name[0] == 0) return;
    UINT8* pstrip, * psmem, * psmem2, * pmask, * pfro, * pBGm;
    UINT16* pfr, *pBG, *pdyn;
    const UINT addrow = ScrW * 4;
    pstrip = pFrameStrip + (FS_LMargin + FRAME_STRIP_W_MARGIN) * 4 + FRAME_STRIP_H_MARGIN * addrow;
    UINT8 frFrameColor[3] = { 255,255,255 };
    int fwid = 256;
    if (MycRom.name[0])
    {
        if (MycRom.fWidth == 192) fwid = 192;
    }
    for (int ti = 0; ti < (int)NFrameToDraw; ti++)
    {
        if ((PreFrameInStrip + ti < 0) || (PreFrameInStrip + ti >= (int)MycRom.nFrames))
        {
            pstrip += (fwid + FRAME_STRIP_W_MARGIN) * 4;
            continue;
        }
        if (PreFrameInStrip + ti == acFrame)
        {
            frFrameColor[0] = acColor[0];
            frFrameColor[1] = acColor[1];
            frFrameColor[2] = acColor[2];
        }
        else if (isFrameSelected(PreFrameInStrip + ti) >= 0)
        {
            frFrameColor[0] = SelColor[0];
            frFrameColor[1] = SelColor[1];
            frFrameColor[2] = SelColor[2];
        }
        else
        {
            frFrameColor[0] = UnselColor[0];
            frFrameColor[1] = UnselColor[1];
            frFrameColor[2] = UnselColor[2];
        }
        for (int tj = -1; tj < fwid + 1; tj++)
        {
            *(pstrip + tj * 4 + 64 * addrow) = *(pstrip - addrow + tj * 4) = frFrameColor[0];
            *(pstrip + tj * 4 + 64 * addrow + 1) = *(pstrip - addrow + tj * 4 + 1) = frFrameColor[1];
            *(pstrip + tj * 4 + 64 * addrow + 2) = *(pstrip - addrow + tj * 4 + 2) = frFrameColor[2];
            *(pstrip + tj * 4 + 64 * addrow + 3) = *(pstrip - addrow + tj * 4 + 3) = 255;
        }
        for (int tj = 0; tj < 64; tj++)
        {
            *(pstrip + tj * addrow + fwid * 4) = *(pstrip - 4 + tj * addrow) = frFrameColor[0];
            *(pstrip + tj * addrow + fwid * 4 + 1) = *(pstrip - 4 + tj * addrow + 1) = frFrameColor[1];
            *(pstrip + tj * addrow + fwid * 4 + 2) = *(pstrip - 4 + tj * addrow + 2) = frFrameColor[2];
            *(pstrip + tj * addrow + fwid * 4 + 3) = *(pstrip - 4 + tj * addrow + 3) = 255;
        }
        pfro = &MycRP.oFrames[(PreFrameInStrip + ti) * MycRom.fWidth * MycRom.fHeight];
        UINT BGID = MycRom.BackgroundID[PreFrameInStrip + ti];
        UINT fw, fh;
        pBG = NULL;
        pBGm = NULL;
        bool drawextramode;
        if (ExtraResFClicked && Edit_Mode == 1 && MycRom.isExtraFrame[PreFrameInStrip + ti] > 0)
        {
            fw = MycRom.fWidthX;
            fh = MycRom.fHeightX;
            pfr = &MycRom.cFramesX[(PreFrameInStrip + ti) * fw * fh];
            if (BGID < MycRom.nBackgrounds && MycRom.isExtraBackground[BGID] > 0)
            {
                pBG = &MycRom.BackgroundFramesX[BGID * fw * fh];
                pBGm = &MycRom.BackgroundMaskX[(PreFrameInStrip + ti) * fw * fh];
            }
            pmask = &MycRom.DynaMasksX[(PreFrameInStrip + ti) * fw * fh];
            pdyn = &MycRom.Dyna4ColsX[(PreFrameInStrip + ti) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors];
            drawextramode = true;
        }
        else
        {
            fw = MycRom.fWidth;
            fh = MycRom.fHeight;
            pfr = &MycRom.cFrames[(PreFrameInStrip + ti) * fw * fh];
            if (BGID < MycRom.nBackgrounds)
            {
                pBG = &MycRom.BackgroundFrames[BGID * fw * fh];
                pBGm = &MycRom.BackgroundMask[(PreFrameInStrip + ti) * fw * fh];
            }
            pmask = &MycRom.DynaMasks[(PreFrameInStrip + ti) * fw * fh];
            pdyn = &MycRom.Dyna4Cols[(PreFrameInStrip + ti) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors];
            drawextramode = false;
        }
        bool doublepixsize = true;
        if (fh == 64) doublepixsize = false;
        psmem = pstrip;
        // let's go
        for (UINT tj = 0; tj < fh; tj++)
        {
            psmem2 = pstrip;
            for (UINT tk = 0; tk < fw; tk++)
            {
                int tm = tk, tl = tj;
                if (drawextramode && fh == 64)
                {
                    tm /= 2;
                    tl /= 2;
                }
                else if (drawextramode)
                {
                    tm *= 2;
                    tl *= 2;
                }
                if ((Edit_Mode == 1) && (pfro[tl * MycRom.fWidth + tm] == 0) && (pBG != NULL) && (pBGm[tj * fw + tk] > 0)) // display colorized frame from background
                    SetRGB888ColorFromRGB565(pstrip, pBG[tj * fw + tk]);
                else if ((Edit_Mode == 1) && (pmask[tj * fw + tk] != 255))
                    SetRGB888ColorFromRGB565(pstrip,
                        MycRom.Dyna4Cols[((PreFrameInStrip + ti) * MAX_DYNA_SETS_PER_FRAMEN + pmask[tj * fw + tk]) * MycRom.noColors + pfro[tl * MycRom.fWidth + tm]]);
                else if (Edit_Mode == 1)
                    SetRGB888ColorFromRGB565(pstrip, pfr[tj * fw + tk]);
                else
                    SetRGB888ColorFromRGB565(pstrip, originalcolors[pfro[tl * MycRom.fWidth + tm]]);
                pstrip[3] = 255;
                if (doublepixsize)
                {
                    pstrip[addrow + 4] = pstrip[addrow] = pstrip[4] = pstrip[0];
                    pstrip[addrow + 5] = pstrip[addrow + 1] = pstrip[5] = pstrip[1];
                    pstrip[addrow + 6] = pstrip[addrow + 2] = pstrip[6] = pstrip[2];
                    pstrip[addrow + 7] = pstrip[addrow + 3] = pstrip[7] = 255;
                    pstrip += 4;
                }
                pstrip += 4;
            }
            pstrip = psmem2 + addrow;
            if (doublepixsize) pstrip += addrow;
        }
        pstrip = psmem + (fwid + FRAME_STRIP_W_MARGIN) * 4;
    }
    int presect = -1, acsect;
    for (UINT ti = 0; ti < NFrameToDraw; ti++)
    {
        if (isSameFrame(PreFrameInStrip + ti) == -1) SetRenderDrawColor(128, 128, 128, 255); else SetRenderDrawColorv((UINT8*)SameColor, 255);
        if (PreFrameInStrip + ti < 0) continue;
        if (PreFrameInStrip + ti >= MycRom.nFrames) continue;
        Draw_Raw_Number(PreFrameInStrip + ti, FS_LMargin + FRAME_STRIP_W_MARGIN + ti * (fwid + FRAME_STRIP_W_MARGIN) + 15, FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pFrameStrip, ScrW, FRAME_STRIP_H_MARGIN);
        SetRenderDrawColor(255, 255, 0, 255);
        Draw_Raw_Number(MycRP.FrameDuration[PreFrameInStrip + ti], FS_LMargin + FRAME_STRIP_W_MARGIN + ti * (fwid + FRAME_STRIP_W_MARGIN) + fwid - 5 * RAW_DIGIT_W - 1, FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pFrameStrip, ScrW, FRAME_STRIP_H_MARGIN);
        acsect = Which_Section(PreFrameInStrip + ti);
        if (acsect != presect)
        {
            SetRenderDrawColorv((UINT8*)SectionColor, 255);
            Draw_Raw_Number(acsect + 1, FS_LMargin+ FRAME_STRIP_W_MARGIN + ti * (fwid + FRAME_STRIP_W_MARGIN) + 100, FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pFrameStrip, ScrW, FRAME_STRIP_H_MARGIN);
        }
        if (MycRom.isExtraFrame[PreFrameInStrip + ti] > 0)
        {
            SetRenderDrawColor(80, 80, 255, 255);
            Draw_Raw_Digit(10, FS_LMargin + FRAME_STRIP_W_MARGIN + ti * (fwid + FRAME_STRIP_W_MARGIN), FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pFrameStrip, ScrW, FRAME_STRIP_H_MARGIN);
        }
        presect = acsect;
    }
    for (UINT ti = addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 5); ti < addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 7); ti++)
    {
        pFrameStrip[ti] = 50;
    }
    for (UINT ti = FRAME_STRIP_SLIDER_MARGIN; ti < ScrW - FRAME_STRIP_SLIDER_MARGIN; ti++)
    {
        Get_Frame_Strip_Line_Color(ti);
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti] = draw_color[0];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 1] = draw_color[1];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 2] = draw_color[2];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 3] = 255;
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti] = draw_color[0];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 1] = draw_color[1];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 2] = draw_color[2];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 3] = 255;
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti] = under_draw_color[0];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 1] = under_draw_color[1];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 2] = under_draw_color[2];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 3] = 255;
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti] = under_draw_color[0];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 1] = under_draw_color[1];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 2] = under_draw_color[2];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 3] = 255;
        //if (isSameFrame(PreFrameInStrip + ti) == -1) SetRenderDrawColor(255, 255, 255, 255); else SetRenderDrawColor(0, 255, 0, 255);
    }
    SliderWidth = ScrW - 2 * FRAME_STRIP_SLIDER_MARGIN;
    CalcPosSlider();
    for (int ti = -5; ti <= 6; ti++)
    {
        for (int tj = 0; tj <= 2; tj++)
        {
            int offset = (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + ti) * addrow + (FRAME_STRIP_SLIDER_MARGIN + tj + PosSlider) * 4;
            pFrameStrip[offset] = 255;
            pFrameStrip[offset + 1] = 255;
            pFrameStrip[offset + 2] = 255;
            pFrameStrip[offset + 3] = 255;
        }
    }
    SetRenderDrawColor(255, 255, 255, 255);
    glfwMakeContextCurrent(glfwframestrip);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, TxFrameStrip[!(acFSText)]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ScrW, FRAME_STRIP_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pFrameStrip);
    acFSText = !(acFSText);
}

void CalcPosSlider2(void)
{
    float div = (float)MycRom.nSprites / (float)SliderWidth2;
    PosSlider2 = (int)((float)PreSpriteInStrip / div);
}
void Get_Sprite_Strip_Line_Color(UINT pos)
{
    UINT corsprite = (int)((float)(pos - FRAME_STRIP_SLIDER_MARGIN) * (float)MycRom.nSprites / (float)SliderWidth2);
    UINT cornextsprite = (int)((float)(pos - FRAME_STRIP_SLIDER_MARGIN + 1) * (float)MycRom.nSprites / (float)SliderWidth2);
    if (cornextsprite == corsprite) cornextsprite++;
    bool issel = false, isac = false;
    for (UINT ti = corsprite; ti < cornextsprite; ti++)
    {
        int iSS = isSpriteSelected(ti);

        if (iSS == -2) isac = true;
        if (iSS > -1) issel = true;
    }
    if (isac == true)
    {
        draw_color[0] = acColor[0];
        draw_color[1] = acColor[1];
        draw_color[2] = acColor[2];
    }
    else if (issel == true)
    {
        draw_color[0] = SelColor[0];
        draw_color[1] = SelColor[1];
        draw_color[2] = SelColor[2];
    }
    else
    {
        draw_color[0] = UnselColor[0];
        draw_color[1] = UnselColor[1];
        draw_color[2] = UnselColor[2];
    }
}
void Sprite_Strip_Update(void)
{
    glfwMakeContextCurrent(glfwspritestrip);
    if (pSpriteStrip) memset(pSpriteStrip, 0, MonWidth * FRAME_STRIP_HEIGHT2 * 4);
    if (MycRom.name[0] == 0) return;
    if (MycRom.nSprites == 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[!(acSSText)]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ScrW2, FRAME_STRIP_HEIGHT2, GL_RGBA, GL_UNSIGNED_BYTE, pSpriteStrip); //RGBA with 4 bytes alignment for efficiency
        acSSText = !(acSSText);
        return;
    }
    UINT8* pspro;
    UINT16* psprc;
    int addrow = ScrW2 * 4;
    UINT8* pstrip = pSpriteStrip + (SS_LMargin + FRAME_STRIP_W_MARGIN) * 4 + FRAME_STRIP_H_MARGIN * addrow;
    UINT8 frFrameColor[3] = { 255,255,255 };
    int fwid = MAX_SPRITE_WIDTH;
    for (int ti = 0; ti < (int)NSpriteToDraw; ti++)
    {
        if (ExtraResSClicked && MycRom.isExtraSprite[PreSpriteInStrip + ti])
        {
            psprc = &MycRom.SpriteColoredX[(PreSpriteInStrip + ti) * MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH];
            pspro = &MycRom.SpriteMaskX[(PreSpriteInStrip + ti) * MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH];
        }
        else
        {
            psprc = &MycRom.SpriteColored[(PreSpriteInStrip + ti) * MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH];
            pspro = &MycRom.SpriteOriginal[(PreSpriteInStrip + ti) * MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH];
        }
        if ((PreSpriteInStrip + ti < 0) || (PreSpriteInStrip + ti >= (int)MycRom.nSprites))
        {
            pstrip += (fwid + FRAME_STRIP_W_MARGIN) * 4;
            continue;
        }
        if (PreSpriteInStrip + ti == acSprite)
        {
            frFrameColor[0] = acColor[0];
            frFrameColor[1] = acColor[1];
            frFrameColor[2] = acColor[2];
        }
        else if (isSpriteSelected(PreSpriteInStrip + ti) >= 0)
        {
            frFrameColor[0] = SelColor[0];
            frFrameColor[1] = SelColor[1];
            frFrameColor[2] = SelColor[2];
        }
        else
        {
            frFrameColor[0] = UnselColor[0];
            frFrameColor[1] = UnselColor[1];
            frFrameColor[2] = UnselColor[2];
        }
        for (int tj = -1; tj < fwid + 1; tj++)
        {
            *(pstrip + tj * 4 + MAX_SPRITE_HEIGHT * addrow) = *(pstrip - addrow + tj * 4) = frFrameColor[0];
            *(pstrip + tj * 4 + MAX_SPRITE_HEIGHT * addrow + 1) = *(pstrip - addrow + tj * 4 + 1) = frFrameColor[1];
            *(pstrip + tj * 4 + MAX_SPRITE_HEIGHT * addrow + 2) = *(pstrip - addrow + tj * 4 + 2) = frFrameColor[2];
            *(pstrip + tj * 4 + MAX_SPRITE_HEIGHT * addrow + 3) = *(pstrip - addrow + tj * 4 + 3) = 255;
        }
        for (int tj = 0; tj < MAX_SPRITE_HEIGHT; tj++)
        {
            *(pstrip + tj * addrow + fwid * 4) = *(pstrip - 4 + tj * addrow) = frFrameColor[0];
            *(pstrip + tj * addrow + fwid * 4 + 1) = *(pstrip - 4 + tj * addrow + 1) = frFrameColor[1];
            *(pstrip + tj * addrow + fwid * 4 + 2) = *(pstrip - 4 + tj * addrow + 2) = frFrameColor[2];
            *(pstrip + tj * addrow + fwid * 4 + 3) = *(pstrip - 4 + tj * addrow + 3) = 255;
        }
        for (UINT tj = 0; tj < MAX_SPRITE_HEIGHT; tj++)
        {
            for (UINT tk = 0; tk < MAX_SPRITE_WIDTH; tk++)
            {
                if (pspro[tk + tj * MAX_SPRITE_WIDTH] < 255)
                {
                    SetRGB888ColorFromRGB565(&pstrip[tj * addrow + tk * 4], psprc[tk + tj * MAX_SPRITE_WIDTH]);
                    pstrip[tj * addrow + tk * 4 + 3] = 255;
                }
            }
        }
        pstrip += (fwid + FRAME_STRIP_W_MARGIN) * 4;
    }
    for (UINT ti = 0; ti < NSpriteToDraw; ti++)
    {
        SetRenderDrawColor(128, 128, 128, 255);
        if (PreSpriteInStrip + ti < 0) continue;
        if (PreSpriteInStrip + ti >= MycRom.nSprites) continue;
        Draw_Raw_Number(PreSpriteInStrip + ti, BS_LMargin + FRAME_STRIP_W_MARGIN + ti * (fwid + FRAME_STRIP_W_MARGIN) + 15, FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pSpriteStrip, ScrW2, FRAME_STRIP_H_MARGIN);
        if (MycRom.isExtraSprite[PreSpriteInStrip + ti] > 0)
        {
            SetRenderDrawColor(80, 80, 255, 255);
            Draw_Raw_Digit(10, BS_LMargin + FRAME_STRIP_W_MARGIN + ti * (fwid + FRAME_STRIP_W_MARGIN), FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pSpriteStrip, ScrW2, FRAME_STRIP_H_MARGIN);
        }
    }
    for (int ti = addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 5); ti < addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 7); ti++)
    {
        pSpriteStrip[ti] = 50;
    }
    for (UINT ti = FRAME_STRIP_SLIDER_MARGIN; ti < ScrW2 - FRAME_STRIP_SLIDER_MARGIN; ti++)
    {
        Get_Sprite_Strip_Line_Color(ti);
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti] = draw_color[0];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 1] = draw_color[1];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 2] = draw_color[2];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 3] = 255;
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti] = draw_color[0];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 1] = draw_color[1];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 2] = draw_color[2];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 3] = 255;
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti] = draw_color[0];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 1] = draw_color[1];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 2] = draw_color[2];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 3] = 255;
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti] = draw_color[0];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 1] = draw_color[1];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 2] = draw_color[2];
        pSpriteStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 3] = 255;
    }
    SliderWidth2 = ScrW2 - 2 * FRAME_STRIP_SLIDER_MARGIN;
    CalcPosSlider2();
    for (int ti = -5; ti <= 6; ti++)
    {
        for (int tj = 0; tj <= 2; tj++)
        {
            int offset = (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + ti) * addrow + (FRAME_STRIP_SLIDER_MARGIN + tj + PosSlider2) * 4;
            pSpriteStrip[offset] = 255;
            pSpriteStrip[offset + 1] = 255;
            pSpriteStrip[offset + 2] = 255;
            pSpriteStrip[offset + 3] = 255;
        }
    }
    SetRenderDrawColor(255, 255, 255, 255);
    //glfwMakeContextCurrent(glfwspritestrip);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[!(acSSText)]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ScrW2, FRAME_STRIP_HEIGHT2, GL_RGBA, GL_UNSIGNED_BYTE, pSpriteStrip); //RGBA with 4 bytes alignment for efficiency
    acSSText = !(acSSText);
}

void CopyRows(UINT sfr, UINT sfrrow, UINT dfr, UINT dfrrow, UINT nrows)
{
    UINT16* pdfr, * psfr;
    UINT8* pddyn, * psdyn;
    UINT fw, fh;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pdfr = &MycRom.cFramesX[dfr * fw * fh];
        psfr = &MycRom.cFramesX[sfr * fw * fh];
        pddyn = &MycRom.DynaMasksX[dfr * fw * fh];
        psdyn = &MycRom.DynaMasksX[sfr * fw * fh];
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pdfr = &MycRom.cFrames[dfr * fw * fh];
        psfr = &MycRom.cFrames[sfr * fw * fh];
        pddyn = &MycRom.DynaMasks[dfr * fw * fh];
        psdyn = &MycRom.DynaMasks[sfr * fw * fh];
    }
    memcpy_s(&pdfr[dfrrow * fw], nrows * fw * sizeof(UINT16), &psfr[sfrrow * fw], nrows * fw * sizeof(UINT16));
    memcpy_s(&pddyn[dfrrow * fw], nrows * fw, &psdyn[sfrrow * fw], nrows * fw);
}
void CopyCols(UINT sfr, UINT sfrcol, UINT dfr, UINT dfrcol, UINT ncols)
{
    UINT fw, fh;
    UINT16* ps, * pd;
    UINT8* psd, * pdd;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        ps = &MycRom.cFramesX[sfr * fw * fh + sfrcol];
        pd = &MycRom.cFramesX[dfr * fw * fh + dfrcol];
        psd = &MycRom.DynaMasksX[sfr * fw * fh + sfrcol];
        pdd = &MycRom.DynaMasksX[dfr * fw * fh + dfrcol];
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        ps= &MycRom.cFrames[sfr * fw * fh + sfrcol];
        pd = &MycRom.cFrames[dfr * fw * fh + dfrcol];
        psd = &MycRom.DynaMasks[sfr * fw * fh + sfrcol];
        pdd = &MycRom.DynaMasks[dfr * fw * fh + dfrcol];
    }
    for (UINT ti = 0; ti < fh; ti++)
    {
        memcpy_s(pd, ncols * sizeof(UINT16), ps, ncols * sizeof(UINT16));
        memcpy_s(pdd, ncols, psd, ncols);
        ps += fw;
        pd += fw;
        psd += fw;
        pdd += fw;
    }
}

void AutoFillScrolling(void)
{
    UINT firstfr = acFrame, lastfr = acFrame;
    bool frfound = true;
    while (frfound)
    {
        frfound = false;
        for (UINT ti = 0; ti < nSelFrames; ti++)
        {
            if ((firstfr > 0) && (SelFrames[ti] == firstfr - 1))
            {
                firstfr--;
                frfound = true;
            }
            if ((lastfr < MycRom.nFrames - 1) && (SelFrames[ti] == lastfr + 1))
            {
                lastfr++;
                frfound = true;
            }
        }
    }
    UINT32 rowCRC32firstfr[64], rowCRC32lastfr[64], rowCRC32acfr[64];
    UINT32 colCRC32firstfr[256], colCRC32lastfr[256], colCRC32acfr[256];
    for (UINT ti = 0; ti < MycRom.fHeight; ti++)
    {
        rowCRC32firstfr[ti] = crc32_fast(&MycRP.oFrames[firstfr * MycRom.fWidth * MycRom.fHeight + ti * MycRom.fWidth], MycRom.fWidth, FALSE);
        rowCRC32lastfr[ti] = crc32_fast(&MycRP.oFrames[lastfr * MycRom.fWidth * MycRom.fHeight + ti * MycRom.fWidth], MycRom.fWidth, FALSE);
    }
    for (UINT ti = 0; ti < MycRom.fWidth; ti++)
    {
        colCRC32firstfr[ti] = crc32_fast_step(&MycRP.oFrames[firstfr * MycRom.fWidth * MycRom.fHeight + ti], MycRom.fWidth, MycRom.fHeight, FALSE);
        colCRC32lastfr[ti] = crc32_fast_step(&MycRP.oFrames[lastfr * MycRom.fWidth * MycRom.fHeight + ti], MycRom.fWidth, MycRom.fHeight, FALSE);
    }
    UINT noreffr = firstfr;
    UINT32* prow = rowCRC32firstfr;
    UINT32* pcol = colCRC32firstfr;
    for (UINT tl = 0; tl < 2; tl++)
    {
        for (UINT tk = firstfr + 1; tk < lastfr; tk++)
        {
            for (UINT ti = 0; ti < MycRom.fHeight; ti++)
            {
                rowCRC32acfr[ti] = crc32_fast(&MycRP.oFrames[tk * MycRom.fWidth * MycRom.fHeight + ti * MycRom.fWidth], MycRom.fWidth, FALSE);
            }
            for (UINT ti = 0; ti < MycRom.fWidth; ti++)
            {
                colCRC32acfr[ti] = crc32_fast_step(&MycRP.oFrames[tk * MycRom.fWidth * MycRom.fHeight + ti], MycRom.fWidth, MycRom.fHeight, FALSE);
            }
            bool allgood;
            for (UINT ti = 1; ti < MycRom.fHeight; ti++)
            {
                allgood = true;
                if (rowCRC32acfr[0] == prow[ti])
                {
                    for (UINT tj = ti + 1; tj < MycRom.fHeight; tj++)
                    {
                        if (rowCRC32acfr[tj - ti] != prow[tj])
                        {
                            allgood = false;
                            break;
                        }
                    }
                }
                else allgood = false;
                if (allgood)
                {
                    if (!nEditExtraResolutionF) CopyRows(noreffr, ti, tk, 0, MycRom.fHeight - ti); // if we don't edit the extra resolution, easy
                    else
                    {
                        if (MycRom.fHeight == 64)
                        {
                            if (ti % 2 == 1) CopyRows(noreffr, ti / 2, tk, 0, MycRom.fHeightX - ti / 2);
                        }
                        else
                            CopyRows(noreffr, ti * 2, tk, 0, MycRom.fHeightX - ti * 2);
                    }
                    break;
                }
            }
            if (allgood) continue;
            for (UINT ti = 1; ti < MycRom.fHeight; ti++)
            {
                allgood = true;
                if (prow[0] == rowCRC32acfr[ti])
                {
                    for (UINT tj = ti + 1; tj < MycRom.fHeight; tj++)
                    {
                        if (prow[tj - ti] != rowCRC32acfr[tj])
                        {
                            allgood = false;
                            break;
                        }
                    }
                }
                else allgood = false;
                if (allgood)
                {
                    if (!nEditExtraResolutionF) CopyRows(noreffr, 0, tk, ti, MycRom.fHeight - ti); // if we don't edit the extra resolution, easy
                    else
                    {
                        if (MycRom.fHeight == 64)
                        {
                            if (ti % 2 == 1) CopyRows(noreffr, 0, tk, ti / 2, MycRom.fHeightX - ti / 2);
                        }
                        else
                            CopyRows(noreffr, 0, tk, ti * 2, MycRom.fHeightX - ti * 2);
                    }
                    break;
                }
            }
            if (allgood) continue;
            for (UINT ti = 1; ti < MycRom.fWidth; ti++)
            {
                allgood = true;
                if (colCRC32acfr[0] == pcol[ti])
                {
                    for (UINT tj = ti + 1; tj < MycRom.fWidth; tj++)
                    {
                        if (colCRC32acfr[tj - ti] != pcol[tj])
                        {
                            allgood = false;
                            break;
                        }
                    }
                }
                else allgood = false;
                if (allgood)
                {
                    if (!nEditExtraResolutionF) CopyCols(noreffr, ti, tk, 0, MycRom.fWidth - ti); // if we don't edit the extra resolution, easy
                    else
                    {
                        if (MycRom.fHeight == 64)
                        {
                            if (ti % 2 == 1) CopyCols(noreffr, ti / 2, tk, 0, MycRom.fWidthX - ti / 2);
                        }
                        else
                            CopyCols(noreffr, ti * 2, tk, 0, MycRom.fWidthX - ti * 2);
                    }
                    break;
                }
            }
            if (allgood) continue;
            for (UINT ti = 1; ti < MycRom.fWidth; ti++)
            {
                allgood = true;
                if (pcol[0] == colCRC32acfr[ti])
                {
                    for (UINT tj = ti + 1; tj < MycRom.fWidth; tj++)
                    {
                        if (pcol[tj - ti] != colCRC32acfr[tj])
                        {
                            allgood = false;
                            break;
                        }
                    }
                }
                else allgood = false;
                if (allgood)
                {
                    if (!nEditExtraResolutionF) CopyCols(noreffr, 0, tk, ti, MycRom.fWidth - ti); // if we don't edit the extra resolution, easy
                    else
                    {
                        if (MycRom.fHeight == 64)
                        {
                            if (ti % 2 == 1) CopyCols(noreffr, 0, tk, ti / 2, MycRom.fWidthX - ti / 2);
                        }
                        else
                            CopyCols(noreffr, 0, tk, ti * 2, MycRom.fWidthX - ti * 2);
                    }
                    break;
                }
            }
        }
        noreffr = lastfr;
        prow = rowCRC32lastfr;
        pcol = colCRC32lastfr;
    }
}


#define MUL_SIZE_GIF 5
void DrawImagePix(UINT8* pimage, UINT16* protation, UINT pixnb, UINT width, UINT16 pcol, UINT sizepix, UINT16 colrot, UINT16 nocolrot)
{
    UINT8* pPix = &pimage[((pixnb / width) * sizepix * width * sizepix + (pixnb % width) * sizepix) * 3];
    UINT16* pRot = NULL;
    if (protation) pRot = &protation[((pixnb / width) * sizepix * width * sizepix + (pixnb % width) * sizepix) * 2];
    for (UINT ti = 0; ti < sizepix - 1; ti++)
    {
        for (UINT tj = 0; tj < sizepix - 1; tj++)
        {
            if (((ti != 0) || (tj != 0)) && ((ti != 0) || (tj != sizepix - 2)) && ((ti != sizepix - 2) || (tj != 0)) && ((ti != sizepix - 2) || (tj != sizepix - 2)))
            {
                rgb565_to_rgb888(pcol, &pPix[(tj * sizepix * width + ti) * 3]);
                if (protation)
                {
                    pRot[(tj * sizepix * width + ti) * 2] = colrot;
                    pRot[(tj * sizepix * width + ti) * 2 + 1] = nocolrot;
                }
            }
        }
    }

}

#define GIF_FRAME_SEPARATION 15
bool CreateGIF(char* GIFname, UINT8* pimages, UINT8* pimagesX, UINT16* protationm,UINT16* protationmX, UINT8* poimages, unsigned int* pdurations, UINT16* pcrotations, UINT16* pcrotationsX, int nimages, bool isextra)
{
    int height = MycRom.fHeight * MUL_SIZE_GIF;
    if (pimagesX && isextra) height += GIF_FRAME_SEPARATION + MycRom.fHeightX * MUL_SIZE_GIF;
    if (poimages) height += GIF_FRAME_SEPARATION + MycRom.fHeight * MUL_SIZE_GIF;
    int width;
    if (isextra)
    {
        width = max((int)MycRom.fWidth, (int)MycRom.fWidthX) * MUL_SIZE_GIF;
    }
    else width = (int)MycRom.fWidth * MUL_SIZE_GIF;
    int offsety = 0, offsetyX = MycRom.fHeight * MUL_SIZE_GIF + GIF_FRAME_SEPARATION;
    int offsetox = (width - MycRom.fWidth * MUL_SIZE_GIF) / 2;
    int offsetxX = (width - MycRom.fWidthX * MUL_SIZE_GIF) / 2;
    if (poimages)
    {
        offsety = MycRom.fHeight * MUL_SIZE_GIF + GIF_FRAME_SEPARATION;
        offsetyX = 2 * MycRom.fHeight * MUL_SIZE_GIF + 2 * GIF_FRAME_SEPARATION;
    }
    UINT8* pGIF = (UINT8*)malloc(width * height * 3);
    if (!pGIF) return false;
    GifskiSettings gifset;
    gifset.quality = 100;
    gifset.height = height;
    gifset.width = width;
    gifset.repeat = 0;
    gifset.fast = false;
    gifski* g = gifski_new2(&gifset);
    gifski_set_file_output2(g, GIFname);
    long acduration = 0;
    int tnofr = 0;
    for (int ti = 0; ti < nimages; ti++)
    {
        int nrot = 0, nrotX = 0;
        UINT16* pacrot = &pcrotations[ti * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
        UINT16* pacrotX = &pcrotationsX[ti * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
        UINT16* protm = &protationm[ti * MycRom.fHeight * MUL_SIZE_GIF * MycRom.fWidth * MUL_SIZE_GIF * 2];
        UINT16* protmX = &protationmX[ti * MycRom.fHeightX * MUL_SIZE_GIF * MycRom.fWidthX * MUL_SIZE_GIF * 2];
        UINT32 nextrot[MAX_COLOR_ROTATIONN];
        UINT32 acshift[MAX_COLOR_ROTATIONN];
        UINT32 nextrotX[MAX_COLOR_ROTATIONN];
        UINT32 acshiftX[MAX_COLOR_ROTATIONN];
        bool isrotactive[MAX_COLOR_ROTATIONN];
        bool isrotactiveX[MAX_COLOR_ROTATIONN];
        for (int tj = 0; tj < MAX_COLOR_ROTATIONN; tj++)
        {
            isrotactive[tj] = pacrot[tj * MAX_LENGTH_COLOR_ROTATION] > 0;
            if (isrotactive[tj]) nrot++;
            isrotactiveX[tj] = pimagesX && pacrotX[tj * MAX_LENGTH_COLOR_ROTATION] > 0;
            if (isrotactiveX[tj]) nrotX++;
        }
        if (poimages)
        {
            for (UINT tj = 0; tj < MycRom.fHeight * MUL_SIZE_GIF; tj++)
                memcpy(&pGIF[(tj * width + offsetox) * 3], &poimages[ti * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * MUL_SIZE_GIF*3 + tj * MycRom.fWidth * MUL_SIZE_GIF * 3], MycRom.fWidth * MUL_SIZE_GIF*3);
        }
        if (nrot == 0 && nrotX == 0)
        {
            for (UINT tj = 0; tj < MycRom.fHeight * MUL_SIZE_GIF; tj++)
                memcpy(&pGIF[((tj + offsety) * width + offsetox) * 3], &pimages[ti * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * MUL_SIZE_GIF*3 + tj * MycRom.fWidth * MUL_SIZE_GIF*3], MycRom.fWidth * MUL_SIZE_GIF*3);
            if (pimagesX)
            {
                for (UINT tj = 0; tj < MycRom.fHeightX * MUL_SIZE_GIF; tj++)
                    memcpy(&pGIF[((tj + offsetyX) * width + offsetxX) * 3], &pimagesX[ti * MycRom.fWidthX * MUL_SIZE_GIF * MycRom.fHeightX * MUL_SIZE_GIF*3 + tj * MycRom.fWidthX * MUL_SIZE_GIF * 3], MycRom.fWidthX * MUL_SIZE_GIF * 3);
            }
            gifski_add_frame_rgb2(g, tnofr, width, width * 3, height , pGIF, (double)acduration / 1000.0);
            tnofr++;
            acduration += pdurations[ti];
        }
        else
        {
            memset(acshift, 0, sizeof(UINT32) * MAX_COLOR_ROTATIONN);
            if (pimagesX) memset(acshiftX, 0, sizeof(UINT32) * MAX_COLOR_ROTATIONN);
            for (int tj = 0; tj < MAX_COLOR_ROTATIONN; tj++)
            {
                if (isrotactive[tj]) nextrot[tj] = pacrot[tj * MAX_LENGTH_COLOR_ROTATION + 1]; else nextrot[tj] = 0xffffffff;
                if (isrotactiveX[tj]) nextrotX[tj] = pacrotX[tj * MAX_LENGTH_COLOR_ROTATION + 1]; else nextrotX[tj] = 0xffffffff;
            }
            UINT32 nextrota = 0xfffffffe;
            for (int tk = 0; tk < MAX_COLOR_ROTATIONN; tk++)
            {
                if (nextrot[tk] < nextrota) nextrota = nextrot[tk];
                if (nextrotX[tk] < nextrota) nextrota = nextrotX[tk];
            }
            for (UINT tj = 0; tj < MycRom.fHeight * MUL_SIZE_GIF; tj++)
                memcpy(&pGIF[((tj + offsety) * width + offsetox) * 3], &pimages[ti * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * MUL_SIZE_GIF * 3 + tj * MycRom.fWidth * MUL_SIZE_GIF * 3], MycRom.fWidth * MUL_SIZE_GIF * 3);
            if (pimagesX)
            {
                for (UINT tj = 0; tj < MycRom.fHeightX * MUL_SIZE_GIF; tj++)
                    memcpy(&pGIF[((tj + offsetyX) * width + offsetxX) * 3], &pimagesX[ti * MycRom.fWidthX * MUL_SIZE_GIF * MycRom.fHeightX * MUL_SIZE_GIF * 3 + tj * MycRom.fWidthX * MUL_SIZE_GIF * 3], MycRom.fWidthX * MUL_SIZE_GIF * 3);
            }
            bool oneshot = false;
            if (pdurations[ti] < nextrota)
            {
                oneshot = true;
            }
            gifski_add_frame_rgb2(g, tnofr, width, width * 3, height, pGIF, (double)acduration / 1000.0);
            UINT32 oldrota = nextrota;
            tnofr++;
            if (oneshot) continue;
            for (int tk = 0; tk < MAX_COLOR_ROTATIONN; tk++)
            {
                if (nextrot[tk] == nextrota)
                {
                    nextrot[tk] += pacrot[tk * MAX_LENGTH_COLOR_ROTATION + 1];
                    acshift[tk]++;
                    if (acshift[tk] == pacrot[tk * MAX_LENGTH_COLOR_ROTATION]) acshift[tk] = 0;
                }
                if (nextrotX[tk] == nextrota)
                {
                    nextrotX[tk] += pacrotX[tk * MAX_LENGTH_COLOR_ROTATION + 1];
                    acshiftX[tk]++;
                    if (acshiftX[tk] == pacrotX[tk * MAX_LENGTH_COLOR_ROTATION]) acshiftX[tk] = 0;
                }
            }
            while (1 == 1)
            {
                UINT32 nextrota = 0xfffffffe;
                for (int tk = 0; tk < MAX_COLOR_ROTATIONN; tk++)
                {
                    if (nextrot[tk] < nextrota) nextrota = nextrot[tk];
                    if (nextrotX[tk] < nextrota) nextrota = nextrotX[tk];
                }
                for (UINT tj = 0; tj < MycRom.fHeight * MUL_SIZE_GIF; tj++)
                {
                    for (UINT tk = 0; tk < MycRom.fWidth * MUL_SIZE_GIF; tk++)
                    {
                        UINT16 colrot = protm[(tj * MycRom.fWidth * MUL_SIZE_GIF + tk) * 2];
                        UINT16 nocolrot = protm[(tj * MycRom.fWidth * MUL_SIZE_GIF + tk) * 2 + 1];
                        if (colrot != 0xffff)
                        {
                            rgb565_to_rgb888(pacrot[colrot * MAX_LENGTH_COLOR_ROTATION + 2 + (acshift[colrot] + nocolrot) % pacrot[colrot * MAX_LENGTH_COLOR_ROTATION]],
                                &pGIF[((tj + offsety) * width + offsetox + tk) * 3]);
                        }
                        else
                            memcpy(&pGIF[((tj + offsety) * width + offsetox + tk) * 3], &pimages[ti * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * MUL_SIZE_GIF * 3 + (tj * MycRom.fWidth * MUL_SIZE_GIF + tk) * 3], 3);
                    }
                }
                if (isextra)
                {
                    for (UINT tj = 0; tj < MycRom.fHeightX * MUL_SIZE_GIF; tj++)
                    {
                        for (UINT tk = 0; tk < MycRom.fWidthX * MUL_SIZE_GIF; tk++)
                        {
                            UINT16 colrot = protmX[(tj * MycRom.fWidthX * MUL_SIZE_GIF + tk) * 2];
                            UINT16 nocolrot = protmX[(tj * MycRom.fWidthX * MUL_SIZE_GIF + tk) * 2 + 1];
                            if (colrot != 0xffff)
                            {
                                rgb565_to_rgb888(pacrotX[colrot * MAX_LENGTH_COLOR_ROTATION + 2 + (acshiftX[colrot] + nocolrot) % pacrotX[colrot * MAX_LENGTH_COLOR_ROTATION]],
                                    &pGIF[((tj + offsetyX) * width + offsetxX + tk) * 3]);
                            }
                            else
                                memcpy(&pGIF[((tj + offsetyX) * width + offsetxX + tk) * 3], &pimagesX[ti * MycRom.fWidthX * MUL_SIZE_GIF * MycRom.fHeightX * MUL_SIZE_GIF * 3 + (tj * MycRom.fWidthX * MUL_SIZE_GIF + tk) * 3], 3);
                        }
                    }
                }
                if (nextrota >= pdurations[ti])
                {
                    gifski_add_frame_rgb2(g, tnofr, width, width * 3, height, pGIF, (double)(acduration + oldrota) / 1000.0);
                    acduration += pdurations[ti];
                    tnofr++;
                    break;
                }
                gifski_add_frame_rgb2(g, tnofr, width, width * 3, height, pGIF, (double)(acduration + oldrota) / 1000.0);
                tnofr++;
                oldrota = nextrota;
                for (int tk = 0; tk < MAX_COLOR_ROTATIONN; tk++)
                {
                    if (nextrot[tk] == nextrota)
                    {
                        nextrot[tk] += pacrot[tk * MAX_LENGTH_COLOR_ROTATION + 1];
                        acshift[tk]++;
                        if (acshift[tk] == pacrot[tk * MAX_LENGTH_COLOR_ROTATION]) acshift[tk] = 0;
                    }
                    if (nextrotX[tk] == nextrota)
                    {
                        nextrotX[tk] += pacrotX[tk * MAX_LENGTH_COLOR_ROTATION + 1];
                        acshiftX[tk]++;
                        if (acshiftX[tk] == pacrotX[tk * MAX_LENGTH_COLOR_ROTATION]) acshiftX[tk] = 0;
                    }
                }
            }
        }
    }
    free(pGIF);
    gifski_finish2(g);
    MessageBoxA(hWnd, "GIF file generated", "Success", MB_OK);
    return true;
}

bool CheckExtraFrameAvailable(UINT frID)
{
    if (MycRom.isExtraFrame[frID] == 0) return false;
    if (MycRom.BackgroundID[frID] < 0xffff && MycRom.isExtraBackground[MycRom.BackgroundID[frID]] == 0) return false;
    for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
    {
        if (MycRom.FrameSprites[frID * MAX_SPRITES_PER_FRAME + ti] < 255 && MycRom.isExtraSprite[MycRom.FrameSprites[frID * MAX_SPRITES_PER_FRAME + ti]] == 0) return false;
    }
    return true;
}

void SaveAnimatedGif(void)
{
    if (MycRom.name[0] == 0) return;
    UINT firstfr = acFrame, lastfr = acFrame;
    bool frfound = true;
    bool isextra = CheckExtraFrameAvailable(acFrame);
    while (frfound)
    {
        frfound = false;
        for (UINT ti = 0; ti < nSelFrames; ti++)
        {
            if ((firstfr > 0) && (SelFrames[ti] == firstfr - 1))
            {
                firstfr--;
                if (CheckExtraFrameAvailable(firstfr)) isextra = true;
                frfound = true;
            }
            if ((lastfr < MycRom.nFrames - 1) && (SelFrames[ti] == lastfr + 1))
            {
                lastfr++;
                if (CheckExtraFrameAvailable(lastfr)) isextra = true;
                frfound = true;
            }
        }
    }

    UINT32 nimages = lastfr - firstfr + 1;
    UINT16* pmaskrot = (UINT16*)malloc(nimages * MUL_SIZE_GIF * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * 2 * sizeof(UINT16));
    UINT8* pimages = (UINT8*)malloc(nimages * MUL_SIZE_GIF * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * 3);
    UINT16* pmaskrotX = NULL;
    UINT8* pimagesX = NULL;
    if (isextra)
    {
        pmaskrotX = (UINT16*)malloc(nimages * MUL_SIZE_GIF * MycRom.fWidthX * MUL_SIZE_GIF * MycRom.fHeightX * 2 * sizeof(UINT16));
        pimagesX = (UINT8*)malloc(nimages * MUL_SIZE_GIF * MycRom.fWidthX * MUL_SIZE_GIF * MycRom.fHeightX * 3);
    }
    UINT8* poimages = NULL;
    bool imok = true;
    if (GetKeyState(VK_SHIFT) & 0x8000)
    {
        poimages = (UINT8*)malloc(nimages * MUL_SIZE_GIF * max((int)MycRom.fWidth, (int)MycRom.fWidthX) * MUL_SIZE_GIF * MycRom.fHeight * 3);
        if (!poimages) imok = false;
    }
    UINT32* pdurations = (UINT32*)malloc(nimages * sizeof(UINT32));
    if ((!pimages) || (isextra && !pimagesX) || (!pmaskrot) || (isextra && !pmaskrotX) || (!pdurations) || (!imok))
    {
        if (pmaskrot) free(pmaskrot);
        if (pmaskrotX) free(pmaskrotX);
        if (pimages) free(pimages);
        if (pimagesX) free(pimages);
        if (poimages) free(poimages);
        if (pdurations) free(pdurations);
        MessageBoxA(hWnd, "Could not get memory to create images from frames", "Failed", MB_OK);
        return;
    }
    memset(pimages, 0, nimages * MUL_SIZE_GIF * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * 3);
    memset(pmaskrot, 0xff, nimages * MUL_SIZE_GIF * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * 2 * sizeof(UINT16));
    if (isextra)
    {
        memset(pimagesX, 0, nimages * MUL_SIZE_GIF * MycRom.fWidthX * MUL_SIZE_GIF * MycRom.fHeightX * 3);
        memset(pmaskrotX, 0xff, nimages * MUL_SIZE_GIF * MycRom.fWidthX * MUL_SIZE_GIF * MycRom.fHeightX * 2 * sizeof(UINT16));
    }
    if (poimages) memset(poimages, 0, nimages * MUL_SIZE_GIF * max((int)MycRom.fWidth, (int)MycRom.fWidthX) * MUL_SIZE_GIF * MycRom.fHeight * sizeof(UINT16));
    for (UINT ti = firstfr; ti <= lastfr; ti++)
    {
        UINT16 BGID = MycRom.BackgroundID[ti];
        for (UINT tl = 0; tl < MycRom.fHeight; tl++)
        {
            for (UINT tk = 0; tk < MycRom.fWidth; tk++)
            {
                UINT tj = tl * MycRom.fWidth + tk;
                UINT16 pcol;
                UINT8 colrot8 = 0xff;
                UINT16 colrot = 0xffff;
                UINT16 nocolrot = 0xffff;
                if ((BGID < MycRom.nBackgrounds) && (MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight + tj] == 0) &&
                    (MycRom.BackgroundMask[ti * MycRom.fWidth * MycRom.fHeight + tj] > 0))
                {
                    pcol = MycRom.BackgroundFrames[BGID * MycRom.fWidth * MycRom.fHeight + tj];
                    isColorInRotation(pcol, ti, false, &colrot8, &nocolrot);
                    if (colrot8 == 0xff) colrot = 0xffff;
                    else colrot = colrot8;
                }
                else if (MycRom.DynaMasks[ti * MycRom.fWidth * MycRom.fHeight + tj] == 255)
                {
                    pcol = MycRom.cFrames[ti * MycRom.fWidth * MycRom.fHeight + tj];
                    isColorInRotation(pcol, ti, false, &colrot8, &nocolrot);
                    if (colrot8 == 0xff) colrot = 0xffff;
                    else colrot = colrot8;
                }
                else
                    pcol = MycRom.Dyna4Cols[ti * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + MycRom.DynaMasks[ti * MycRom.fWidth * MycRom.fHeight + tj] * MycRom.noColors + MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight + tj]];
                DrawImagePix(&pimages[(ti - firstfr) * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * MUL_SIZE_GIF * 3], &pmaskrot[(ti - firstfr) * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * MUL_SIZE_GIF * 2],
                    tj, MycRom.fWidth, pcol, MUL_SIZE_GIF, colrot, nocolrot);
                if (poimages)
                {
                    pcol = originalcolors[MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight + tj]];
                    DrawImagePix(&poimages[(ti - firstfr) * MycRom.fWidth * MUL_SIZE_GIF * MycRom.fHeight * MUL_SIZE_GIF * 3], NULL, tj, MycRom.fWidth, pcol, MUL_SIZE_GIF, 0xffff, 0xffff);
                }
            }
        }
        if (isextra)
        {
            for (UINT tl = 0; tl < MycRom.fHeightX; tl++)
            {
                for (UINT tk = 0; tk < MycRom.fWidthX; tk++)
                {
                    UINT16 pcol;
                    UINT8 colrot8 = 0xff;
                    UINT16 colrot = 0xffff;
                    UINT16 nocolrot = 0xffff;
                    UINT tj = tl * MycRom.fWidthX + tk;
                    UINT tm;
                    if (MycRom.fHeightX == 32) tm = tl * 2 * MycRom.fWidth + tk * 2;
                    else tm = tl / 2 * MycRom.fWidth + tk / 2;
                    if (CheckExtraFrameAvailable(ti))
                    {
                        if ((BGID < MycRom.nBackgrounds) && (MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight + tm] == 0) &&
                            (MycRom.BackgroundMaskX[ti * MycRom.fWidthX * MycRom.fHeightX + tj] > 0))
                        {
                            pcol = MycRom.BackgroundFramesX[BGID * MycRom.fWidthX * MycRom.fHeightX + tj];
                            isColorInRotation(pcol, ti, true, &colrot8, &nocolrot);
                            if (colrot8 == 0xff) colrot = 0xffff;
                            else colrot = colrot8;
                        }
                        else if (MycRom.DynaMasksX[ti * MycRom.fWidthX * MycRom.fHeightX + tj] == 255)
                        {
                            pcol = MycRom.cFramesX[ti * MycRom.fWidthX * MycRom.fHeightX + tj];
                            isColorInRotation(pcol, ti, true, &colrot8, &nocolrot);
                            if (colrot8 == 0xff) colrot = 0xffff;
                            else colrot = colrot8;
                        }
                        else
                            pcol = MycRom.Dyna4ColsX[ti * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + MycRom.DynaMasksX[ti * MycRom.fWidthX * MycRom.fHeightX + tj] * MycRom.noColors + MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight + tm]];
                    }
                    else
                    {
                        pcol = 0;
                        colrot = nocolrot = 0xffff;
                    }
                    DrawImagePix(&pimagesX[(ti - firstfr) * MycRom.fWidthX * MUL_SIZE_GIF * MycRom.fHeightX * MUL_SIZE_GIF * 3], &pmaskrotX[(ti - firstfr) * MycRom.fWidthX * MUL_SIZE_GIF * MycRom.fHeightX * MUL_SIZE_GIF * 2],
                        tj, MycRom.fWidthX, pcol, MUL_SIZE_GIF, colrot, nocolrot);
                }
            }
        }
        pdurations[ti - firstfr] = MycRP.FrameDuration[ti];
    }
    char szFile[260];
    strcpy_s(szFile, 260, Dir_GIFs);
    strcat_s(szFile, 260, "*.gif");
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrTitle = "Saving Animated GIF...";
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Animated Image (.gif)\0*.GIF\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = Dir_GIFs;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn) == TRUE)
    {
        strcpy_s(Dir_GIFs, 260, ofn.lpstrFile);
        int i = (int)strlen(Dir_GIFs) - 1;
        while ((i > 0) && (Dir_GIFs[i] != '\\')) i--;
        Dir_GIFs[i + 1] = 0;
        SavePaths();
    }
    else
    {
        cprintf(true, "The GIF file was not saved");
        return; 
    }
    size_t sln = strlen(ofn.lpstrFile);
    if (((ofn.lpstrFile[sln - 1] != 'f') && (ofn.lpstrFile[sln - 1] != 'F')) || ((ofn.lpstrFile[sln - 2] != 'i') && (ofn.lpstrFile[sln - 2] != 'I')) || ((ofn.lpstrFile[sln - 3] != 'g') && (ofn.lpstrFile[sln - 3] != 'G')) || (ofn.lpstrFile[sln - 4] != '.'))
        strcat_s(ofn.lpstrFile, MAX_PATH, ".gif");
    CreateGIF(ofn.lpstrFile, pimages, pimagesX, pmaskrot, pmaskrotX, poimages, pdurations, &MycRom.ColorRotations[firstfr * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], &MycRom.ColorRotationsX[firstfr * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], nimages, isextra);
    free(pmaskrot);
    free(pmaskrotX);
    free(pimages);
    free(pimagesX);
    if (poimages) free(poimages);
    free(pdurations);
}

bool isTesterfr32, isTesterfr64;
UINT8* ptesteroldfr;
UINT16* ptesternewfr32, * ptesternewfr64;
UINT8* ptesterbuffer32, * ptesterbuffer64;
#define MUL_TESTER_PIXEL_SIZE 5

LRESULT CALLBACK FrameWndProc(HWND hwDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CLOSE:
        return 0;
    case WM_PAINT:
    {
        if (!ptesterbuffer32 || !ptesterbuffer64) return TRUE;
        PAINTSTRUCT ps;
        BITMAPINFO bitmapInfo = { 0 };
        UINT fw, fh;
        int imageWidth, imageHeight;
        if ((hwDlg == hFrame32 && MycRom.fHeight == 32) || (hwDlg == hFrame64 && MycRom.fHeight == 64))
        {
            fw = MycRom.fWidth;
            fh = MycRom.fHeight;
        }
        else
        {
            fw = MycRom.fWidthX;
            fh = MycRom.fHeightX;
        }
        imageWidth = MUL_TESTER_PIXEL_SIZE * fw;
        imageHeight = MUL_TESTER_PIXEL_SIZE * fh;
        RECT clientRect;
        GetClientRect(hwDlg, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int posx = (clientWidth - imageWidth) / 2;
        UINT8* ptnf;
        if (hwDlg == hFrame32) ptnf = ptesterbuffer32; else ptnf = ptesterbuffer64;
        memset(ptnf, 0, imageWidth * imageHeight * 4);
        if ((hwDlg == hFrame32 && ptesternewfr32 && isTesterfr32) || (hwDlg == hFrame64 && ptesternewfr64 && isTesterfr64))
        {
            for (UINT tj = 0; tj < fh; tj++)
            {
                for (UINT ti = 0; ti < fw; ti++)
                {
                    UINT8 pixcol[4];
                    if (fh == 32) rgb565_to_rgb888(ptesternewfr32[tj * fw + ti], pixcol);
                    else rgb565_to_rgb888(ptesternewfr64[tj * fw + ti], pixcol);
                    pixcol[3] = pixcol[0];
                    pixcol[0] = pixcol[2];
                    pixcol[2] = pixcol[3];
                    pixcol[3] = 255;
                    for (int tl = 0; tl < MUL_TESTER_PIXEL_SIZE; tl++)
                    {
                        for (int tk = 0; tk < MUL_TESTER_PIXEL_SIZE; tk++)
                        {
                            if ((tl > 0 && tk > 0 && tl < MUL_TESTER_PIXEL_SIZE - 2 && tk < MUL_TESTER_PIXEL_SIZE - 2) ||
                                ((tl == 0 || tl == MUL_TESTER_PIXEL_SIZE - 2) && tk > 0 && tk < MUL_TESTER_PIXEL_SIZE - 2) ||
                                ((tk == 0 || tk == MUL_TESTER_PIXEL_SIZE - 2) && tl > 0 && tl < MUL_TESTER_PIXEL_SIZE - 2))
                            {
                                memcpy(&ptnf[((tj * MUL_TESTER_PIXEL_SIZE + tl) * fw * MUL_TESTER_PIXEL_SIZE + ti * MUL_TESTER_PIXEL_SIZE + tk) * 4], pixcol, 4);
                            }
                        }
                    }
                }
            }
        }

        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = imageWidth;
        bitmapInfo.bmiHeader.biHeight = -imageHeight;
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;        
        bitmapInfo.bmiHeader.biCompression = BI_RGB;

        HDC hdc = BeginPaint(hwDlg, &ps);


        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, imageWidth, imageHeight);
        SelectObject(memDC, memBitmap);

        SetDIBitsToDevice(memDC, 0, 0, imageWidth, imageHeight, 0, 0, 0, imageHeight, ptnf, &bitmapInfo, DIB_RGB_COLORS);

        BitBlt(hdc, posx, 0, imageWidth, imageHeight, memDC, 0, 0, SRCCOPY);

        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hwDlg, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwDlg, message, wParam, lParam);
    }
    return 0;
}
UINT testerframeID;
UINT16* pTesterFrame32, * pTesterFrame64;
unsigned char TesterOriginalFrame[512 * 128 * 4];

void SendFrameToTester2(unsigned int nofr, UINT8* pframes)
{
    int pixsize = 128 / MycRom.fHeight;
    int offsetx = 0;
    if (MycRom.fWidth == 192) offsetx = 64;
    for (int tj = 0; tj < 128; tj++)
    {
        for (int ti = 0; ti < 512; ti++)
        {
            if ((ti < offsetx) || (ti >= 512 - offsetx)) memset(&TesterOriginalFrame[(tj * 512 + ti) * 4], 0, 4);
            else
            {
                float coefcol = (float)pframes[nofr * MycRom.fWidth * MycRom.fHeight+ (ti - offsetx) / pixsize + tj / pixsize * MycRom.fWidth] / (float)MycRom.noColors;
                TesterOriginalFrame[(tj * 512 + ti) * 4 + 2] = (unsigned char)(255 * coefcol);
                TesterOriginalFrame[(tj * 512 + ti) * 4 + 1] = (unsigned char)(127 * coefcol);
                TesterOriginalFrame[(tj * 512 + ti) * 4] = 0;
                TesterOriginalFrame[(tj * 512 + ti) * 4 + 3] = 255;
            }
        }
    }
    ColorizeAFrame(&pframes[nofr * MycRom.fWidth * MycRom.fHeight], &ptesteroldfr, &ptesternewfr32, &ptesternewfr64, &testerframeID, &isTesterfr32, &isTesterfr64);
    InvalidateRect(hFrame32, NULL, FALSE);
    InvalidateRect(hFrame64, NULL, FALSE);
    //if (nocolors == 4) DmdDev_Render_4_Shades(width, height, &pframes[nofr * width * height]);
    //else DmdDev_Render_16_Shades(width, height, &pframes[nofr * width * height]);
}
UINT8* pTesterFrames;
UINT* pTesterTimecodes;
UINT nTesterFrames;
bool TesterSelectionMode;
float Testerplayspeed = 1;
int TesterPlay = 0;
int Tester_Posx = 0, Tester_Posy = 0;
#define MAX_TESTER_TICKS 30
void SetTesterRangeAndTicKs(HWND slider,UINT nFrames)
{
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELONG(0, nFrames - 1));
    SendMessage(slider, TBM_SETPOS, TRUE, 0);
    UINT ticfreq;
    if (nFrames < MAX_TESTER_TICKS) ticfreq = nFrames;
    else ticfreq = nFrames / (MAX_TESTER_TICKS - 1);
    SendMessage(slider, TBM_SETTICFREQ, ticfreq, 0);
}


bool CreateFrameWindows(HWND hParent)
{
    RECT rc;
    if (MycRom.fHeight == 32)
    {
        rc = { 0, 0, (LONG)MycRom.fWidth * MUL_TESTER_PIXEL_SIZE, (LONG)MycRom.fHeight * MUL_TESTER_PIXEL_SIZE };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
    }
    else
    {
        rc = { 0, 0, (LONG)MycRom.fWidthX * MUL_TESTER_PIXEL_SIZE, (LONG)MycRom.fHeightX * MUL_TESTER_PIXEL_SIZE };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
    }

    hFrame32 = CreateWindowEx(0, L"FrameTester", L"Frame 32P", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, hParent, NULL, hInst, NULL);

    if (!hFrame32) return false;

    ShowWindow(hFrame32, SW_SHOW);
    UpdateWindow(hFrame32);

    if (MycRom.fHeight == 64)
    {
        rc = { 0, 0, (LONG)MycRom.fWidth * MUL_TESTER_PIXEL_SIZE, (LONG)MycRom.fHeight * MUL_TESTER_PIXEL_SIZE };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
    }
    else
    {
        rc = { 0, 0, (LONG)MycRom.fWidthX * MUL_TESTER_PIXEL_SIZE, (LONG)MycRom.fHeightX * MUL_TESTER_PIXEL_SIZE };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
    }
    hFrame64 = CreateWindowEx(0, L"FrameTester", L"Frame 64P", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, hParent, NULL, hInst, NULL);

    if (!hFrame64)
    {
        DestroyWindow(hFrame32);
        return false;
    }
    ShowWindow(hFrame64, SW_SHOW);
    UpdateWindow(hFrame64);

    ptesterbuffer64 = ptesterbuffer32 = NULL;
    if (MycRom.fHeight == 32)
    {
        ptesterbuffer32 = (UINT8*)malloc(4 * MycRom.fWidth * MUL_TESTER_PIXEL_SIZE * MycRom.fHeight * MUL_TESTER_PIXEL_SIZE);
        ptesterbuffer64 = (UINT8*)malloc(4 * MycRom.fWidthX * MUL_TESTER_PIXEL_SIZE * MycRom.fHeightX * MUL_TESTER_PIXEL_SIZE);
    }
    else
    {
        ptesterbuffer64 = (UINT8*)malloc(4 * MycRom.fWidth * MUL_TESTER_PIXEL_SIZE * MycRom.fHeight * MUL_TESTER_PIXEL_SIZE);
        ptesterbuffer32 = (UINT8*)malloc(4 * MycRom.fWidthX * MUL_TESTER_PIXEL_SIZE * MycRom.fHeightX * MUL_TESTER_PIXEL_SIZE);
    }
    if (!ptesterbuffer32 || !ptesterbuffer64)
    {
        if (ptesterbuffer32)
        {
            free(ptesterbuffer32);
            ptesterbuffer32 = NULL;
        }
        if (ptesterbuffer64)
        {
            free(ptesterbuffer64);
            ptesterbuffer64 = NULL;
        }
        DestroyWindow(hFrame32);
        DestroyWindow(hFrame64);
        return false;
    }
    return true;
}

UINT TesterDuration;
bool TesterLoop=false;
bool TesterShow32P = true;
bool TesterShow64P = true;

void GetIDFrameText(UINT nofr, char* cFrameID)
{
    if (nofr == 65535) strcpy_s(cFrameID, 64, "No Frame identified");
    else sprintf_s(cFrameID, 64, "Frame identified: %i", nofr);
}

LRESULT CALLBACK Tester_Proc(HWND hwDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HWND hChFr = GetDlgItem(hwDlg, IDC_CHOOSEFRAME);
    char cFrameID[64];
    switch (Msg)
    {
        case WM_INITDIALOG:
        {
            ptesterbuffer32 = ptesterbuffer64 = NULL;
            SetDlgItemTextA(hwDlg, IDC_INFO, "Navigate through the selected frames:");
            SetTesterRangeAndTicKs(hChFr, nSelFrames);
            SendFrameToTester2(SelFrames[0], MycRP.oFrames);
            char tbuf[128];
            GetIDFrameText(testerframeID, cFrameID);
            sprintf_s(tbuf, 128, "Frame Tester                   Current frame: %i / Total: %i       %s", 0, nSelFrames, cFrameID);
            SetWindowTextA(hwDlg, tbuf);
            InvalidateRect(hwDlg, NULL, FALSE);
            TesterSelectionMode = true;
            Testerplayspeed = 1;
            TesterPlay = 0;
            SendMessage(GetDlgItem(hwDlg, IDC_SPD1), BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(GetDlgItem(hwDlg, IDC_SPD2), BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hwDlg, IDC_SPD4), BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hwDlg, IDC_SPD8), BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hwDlg, IDC_SPD16), BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hwDlg, IDC_SPD32), BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hwDlg, IDC_SPDHALF), BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hwDlg, IDC_SPDQUARTER), BM_SETCHECK, BST_UNCHECKED, 0);
            SetWindowPos(hwDlg, 0, Tester_Posx, Tester_Posy, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
            if (TesterLoop == false) SendMessage(GetDlgItem(hwDlg, IDC_LOOP), BM_SETCHECK, BST_UNCHECKED, 0);
            else SendMessage(GetDlgItem(hwDlg, IDC_LOOP), BM_SETCHECK, BST_CHECKED, 0);
            if (!CreateFrameWindows(hwDlg))
            {
                MessageBoxA(hwDlg, "Couldn't create the windows for tester", "Failed", MB_OK);
                DestroyWindow(hwDlg);
                return TRUE;
            }
            if (TesterShow32P)
            {
                CheckDlgButton(hwDlg, IDC_FRAME32, BST_CHECKED);
                ShowWindow(hFrame32, SW_SHOW);
            }
            else
            {
                CheckDlgButton(hwDlg, IDC_FRAME32, BST_UNCHECKED);
                ShowWindow(hFrame32, SW_HIDE);
            }
            if (TesterShow64P)
            {
                CheckDlgButton(hwDlg, IDC_FRAME64, BST_CHECKED);
                ShowWindow(hFrame64, SW_SHOW);
            }
            else
            {
                CheckDlgButton(hwDlg, IDC_FRAME64, BST_UNCHECKED);
                ShowWindow(hFrame64, SW_HIDE);
            }
            return TRUE;
        }
        case WM_TIMER:
        {
            if (timeGetTime() >= TesterDuration)
            {
                KillTimer(hwDlg, 1);
                int frpos = (int)SendMessage(hChFr, TBM_GETPOS, TRUE, 0);
                if (TesterPlay == 1)
                {
                    frpos++;
                    if (TesterSelectionMode)
                    {
                        if (frpos == nSelFrames)
                        {
                            if (TesterLoop) frpos = 0;
                            else
                            {
                                TesterPlay = 0;
                                return TRUE;
                            }
                        }
                        TesterDuration = timeGetTime() + (int)(MycRP.FrameDuration[SelFrames[frpos]] / Testerplayspeed);
                    }
                    else
                    {
                        if (frpos == nTesterFrames)
                        {
                            if (TesterLoop) frpos = 0;
                            else
                            {
                                TesterPlay = 0;
                                return TRUE;
                            }
                        }
                        TesterDuration = timeGetTime() + (int)(pTesterTimecodes[frpos] / Testerplayspeed);
                    }
                }
                else
                {
                    frpos--;
                    if (TesterSelectionMode)
                    {
                        if (frpos == -1)
                        {
                            if (TesterLoop) frpos = nSelFrames - 1;
                            else
                            {
                                TesterPlay = 0;
                                return TRUE;
                            }
                        }
                        TesterDuration = timeGetTime() + (int)(MycRP.FrameDuration[SelFrames[frpos]] / Testerplayspeed);
                    }
                    else
                    {
                        if (frpos == -1)
                        {
                            if (TesterLoop) frpos = nTesterFrames - 1;
                            else
                            {
                                TesterPlay = 0;
                                return TRUE;
                            }
                        }
                        TesterDuration = timeGetTime() + (int)(pTesterTimecodes[frpos] / Testerplayspeed);
                    }
                }
                SendMessage(hChFr, TBM_SETPOS, TRUE, frpos);
                SetTimer(hwDlg, 1, 16, NULL);
                SendMessage(hwDlg, WM_HSCROLL, 0, 0);
            }
            //else
            {
                /*int frpos = (int)SendMessage(hChFr, TBM_GETPOS, TRUE, 0);
                if (TesterSelectionMode) SendFrameToTester2(SelFrames[frpos], MycRP.oFrames);
                else SendFrameToTester2(frpos, pTesterFrames);*/
                //ColorizeAFrame(&pframes[nofr * MycRom.fWidth * MycRom.fHeight], &ptesteroldfr, &ptesternewfr32, &ptesternewfr64, &testerframeID);
                if (ColorRotateAFrame(&ptesteroldfr, &ptesternewfr32, NULL, &ptesternewfr64, NULL))
                {
                    InvalidateRect(hFrame32, NULL, FALSE);
                    InvalidateRect(hFrame64, NULL, FALSE);
                }
            }
            return TRUE;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BITMAPINFO bitmapInfo = { 0 };
            int imageWidth = 512;
            int imageHeight = 128;

            RECT clientRect;
            GetClientRect(hwDlg, &clientRect);
            int clientWidth = clientRect.right - clientRect.left;
            int posx = (clientWidth - imageWidth) / 2;

            bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmapInfo.bmiHeader.biWidth = imageWidth;    
            bitmapInfo.bmiHeader.biHeight = -imageHeight;
            bitmapInfo.bmiHeader.biPlanes = 1;
            bitmapInfo.bmiHeader.biBitCount = 32;        
            bitmapInfo.bmiHeader.biCompression = BI_RGB; 

            HDC hdc = BeginPaint(hwDlg, &ps);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, imageWidth, imageHeight);
            SelectObject(memDC, memBitmap);

            SetDIBitsToDevice(memDC, 0, 0, imageWidth, imageHeight, 0, 0, 0, imageHeight, TesterOriginalFrame, &bitmapInfo, DIB_RGB_COLORS);

            BitBlt(hdc, posx, 100, imageWidth, imageHeight, memDC, 0, 0, SRCCOPY);

            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hwDlg, &ps);
            return 0;
        }
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDC_LOOP:
                {
                    if (SendMessage(GetDlgItem(hwDlg, IDC_LOOP), BM_GETCHECK, 0, 0) == BST_CHECKED) TesterLoop = true; else TesterLoop = false;
                    return TRUE;
                }
                case IDCANCEL:
                {
                    KillTimer(hwDlg, 1);
                    WINDOWPLACEMENT wp;
                    wp.length = sizeof(WINDOWPLACEMENT);
                    GetWindowPlacement(hwDlg, &wp);
                    EndDialog(hwDlg, 0);
                    Tester_Posx = wp.rcNormalPosition.left;
                    Tester_Posy = wp.rcNormalPosition.top;
                    SaveWindowPosition();
                    free(ptesterbuffer32);
                    free(ptesterbuffer64);
                    DestroyWindow(hFrame32);
                    DestroyWindow(hFrame64);
                    return TRUE;
                }
                case IDC_SPD1:
                {
                    Testerplayspeed = 1;
                    return TRUE;
                }
                case IDC_SPD2:
                {
                    Testerplayspeed = 2;
                    return TRUE;
                }
                case IDC_SPD4:
                {
                    Testerplayspeed = 4;
                    return TRUE;
                }
                case IDC_SPD8:
                {
                    Testerplayspeed = 8;
                    return TRUE;
                }
                case IDC_SPD16:
                {
                    Testerplayspeed = 16;
                    return TRUE;
                }
                case IDC_SPD32:
                {
                    Testerplayspeed = 32;
                    return TRUE;
                }
                case IDC_SPDHALF:
                {
                    Testerplayspeed = 0.5f;
                    return TRUE;
                }
                case IDC_SPDQUARTER:
                {
                    Testerplayspeed = 0.25f;
                    return TRUE;
                }
                case IDC_REWIND:
                {
                    if (TesterPlay)
                    {
                        TesterPlay = 0;
                        KillTimer(hwDlg, 1);
                    }
                    else
                    {
                        TesterPlay = 2;
                        int frpos = (int)SendMessage(hChFr, TBM_GETPOS, TRUE, 0);
                        if (TesterSelectionMode) TesterDuration = timeGetTime() + (int)(MycRP.FrameDuration[SelFrames[frpos]] / Testerplayspeed);
                        else TesterDuration = timeGetTime() + (int)(pTesterTimecodes[frpos] / Testerplayspeed);
                        SetTimer(hwDlg, 1, 16, NULL);
                    }
                    return TRUE;
                }
                case IDC_PLAYPAUSE:
                {
                    if (TesterPlay)
                    {
                        TesterPlay = false;
                        KillTimer(hwDlg, 1);
                    }
                    else
                    {
                        TesterPlay = true;
                        int frpos = (int)SendMessage(hChFr, TBM_GETPOS, TRUE, 0);
                        if (TesterSelectionMode) TesterDuration = timeGetTime() + (int)(MycRP.FrameDuration[SelFrames[frpos]] / Testerplayspeed);
                        else TesterDuration = timeGetTime() + (int)(pTesterTimecodes[frpos] / Testerplayspeed);
                        SetTimer(hwDlg, 1, 16, NULL);
                    }
                    return TRUE;
                }
                case IDC_LOADDUMP:
                {
                    KillTimer(hwDlg, 1);
                    OPENFILENAMEA ofn;
                    char szFile[260];
                    strcpy_s(szFile, 260, Dir_Dumps);
                    strcat_s(szFile, 260, "*.txt");

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.lpstrTitle = "Choose the dump file to test";
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Text (.txt)\0*.TXT\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrInitialDir = Dir_Dumps;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                    if (GetOpenFileNameA(&ofn) == TRUE)
                    {
                        strcpy_s(Dir_Dumps, 260, ofn.lpstrFile);
                        int i = (int)strlen(Dir_Dumps) - 1;
                        while ((i > 0) && (Dir_Dumps[i] != '\\')) i--;
                        Dir_Dumps[i + 1] = 0;
                        NewProj = true;
                        SavePaths();
                        if (pTesterFrames)
                        {
                            free(pTesterFrames);
                            pTesterFrames = NULL;
                        }
                        if (pTesterTimecodes)
                        {
                            free(pTesterTimecodes);
                            pTesterTimecodes = NULL;
                        }
                        SetDlgItemTextA(hwDlg, IDC_INFO, "Please wait while loading...");
                        const char* pres = ImportDump(ofn.lpstrFile, MycRom.name, &pTesterFrames, &pTesterTimecodes, &nTesterFrames, MycRom.noColors, MycRom.fWidth, MycRom.fHeight);
                        if (pres[0] != 0)
                        {
                            cprintf(true, pres);
                            MessageBoxA(hwDlg, "Error while loading the dump file", "Error", MB_OK);
                            return TRUE;
                        }
                        SetDlgItemTextA(hwDlg, IDC_INFO, "Navigate through the dumped frames:");
                        TesterSelectionMode = false;
                        SendFrameToTester2(0, pTesterFrames);
                        char tbuf[128];
                        GetIDFrameText(testerframeID, cFrameID);
                        sprintf_s(tbuf, 128, "Frame Tester                   Current frame: %i / Total: %i       %s", 0, nTesterFrames, cFrameID);
                        SetWindowTextA(hwDlg, tbuf);
                        InvalidateRect(hwDlg, NULL, FALSE);
                        SetTesterRangeAndTicKs(hChFr, nTesterFrames);
                    }
                    return TRUE;
                }
                case IDC_FRAME32:
                {
                    if (Button_GetCheck(GetDlgItem(hwDlg, IDC_FRAME32)) == BST_CHECKED) TesterShow32P = true;
                    else TesterShow32P = false;
                    if (TesterShow32P)
                    {
                        CheckDlgButton(hwDlg, IDC_FRAME32, BST_CHECKED);
                        ShowWindow(hFrame32, SW_SHOW);
                    }
                    else
                    {
                        CheckDlgButton(hwDlg, IDC_FRAME32, BST_UNCHECKED);
                        ShowWindow(hFrame32, SW_HIDE);
                    }
                    return TRUE;
                }
                case IDC_FRAME64:
                {
                    if (Button_GetCheck(GetDlgItem(hwDlg, IDC_FRAME64)) == BST_CHECKED) TesterShow64P = true;
                    else TesterShow64P = false;
                    if (TesterShow64P)
                    {
                        CheckDlgButton(hwDlg, IDC_FRAME64, BST_CHECKED);
                        ShowWindow(hFrame64, SW_SHOW);
                    }
                    else
                    {
                        CheckDlgButton(hwDlg, IDC_FRAME64, BST_UNCHECKED);
                        ShowWindow(hFrame64, SW_HIDE);
                    }
                    return TRUE;
                }
            }
        }
        /*case WM_CLOSE:
        {
            KillTimer(hwDlg, 1);
            WINDOWPLACEMENT wp;
            wp.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(hwDlg, &wp);
            Tester_Posx = wp.rcNormalPosition.left;
            Tester_Posy = wp.rcNormalPosition.top;
            SaveWindowPosition();
            //DestroyWindow(hFrame32);
            //DestroyWindow(hFrame64);
            if (pTesterFrames)
            {
                free(pTesterFrames);
                pTesterFrames = NULL;
            }
            if (pTesterTimecodes)
            {
                free(pTesterTimecodes);
                pTesterTimecodes = NULL;
            }
            return 0;
        }*/
        case WM_HSCROLL:
        {
            char tbuf[128];
            int frpos = (int)SendMessage(hChFr, TBM_GETPOS, TRUE, 0);
            if (TesterSelectionMode)
            {
                SendFrameToTester2(SelFrames[frpos], MycRP.oFrames);
                InvalidateRect(hwDlg, NULL, FALSE);
                GetIDFrameText(testerframeID, cFrameID);
                sprintf_s(tbuf, 128, "Frame Tester                   Current frame: %i / Total: %i       %s", frpos, nSelFrames, cFrameID);
            }
            else
            {
                SendFrameToTester2(frpos, pTesterFrames);
                InvalidateRect(hwDlg, NULL, FALSE);
                GetIDFrameText(testerframeID, cFrameID);
                sprintf_s(tbuf, 128, "Frame Tester                   Current frame: %i / Total: %i       %s", frpos, nTesterFrames, cFrameID);
            }
            SetWindowTextA(hwDlg, tbuf);
            return TRUE;
        }
    }
    return FALSE;
}
void TestSelectedFrames(void)
{
    if (MycRom.name[0] == 0) return;
    if (Dir_VP[0] == 0)
    {
        char szDir[MAX_PATH];
        BROWSEINFOA bInfo;
        bInfo.hwndOwner = hWnd;
        bInfo.pidlRoot = NULL;
        bInfo.pszDisplayName = szDir;
        bInfo.lpszTitle = "Please, select the VPinMame directory";
        bInfo.ulFlags = 0;
        bInfo.lpfn = NULL;
        bInfo.lParam = 0;
        bInfo.iImage = -1;

        LPITEMIDLIST lpItem = SHBrowseForFolderA(&bInfo);
        if (lpItem != NULL)
        {
            SHGetPathFromIDListA(lpItem, Dir_VP);
            if ((Dir_VP[strlen(Dir_VP) - 1] != '/') && (Dir_VP[strlen(Dir_VP) - 1] != '\\')) strcat_s(Dir_VP, MAX_PATH, "\\");
            CoTaskMemFree(lpItem);
        }
    }
    if (Dir_VP[0] == 0) return;
    SavePaths();
    char tbuf[MAX_PATH];
    sprintf_s(tbuf, MAX_PATH, "%saltcolor\\", Dir_VP);
    if (!PathFileExistsA(tbuf))
    {
        if (!CreateDirectoryA(tbuf, NULL))
        {
            MessageBoxA(hWnd, "Didn't succeed to create the altcolor directory", "Failed", MB_OK);
            return;
        }
    }
    strcat_s(tbuf, MAX_PATH, MycRom.name);
    strcat_s(tbuf, MAX_PATH, "\\");
    if (!PathFileExistsA(tbuf))
    {
        if (!CreateDirectoryA(tbuf, NULL))
        {
            MessageBoxA(hWnd, "Didn't succeed to create the altcolor\\rom_name directory", "Failed", MB_OK);
            return;
        }
    }
    Save_cRom(false, true, tbuf);
    /*char tbuf2[MAX_PATH];
    strcat_s(tbuf, MAX_PATH, MycRom.name);
    strcat_s(tbuf, MAX_PATH, ".cRZ");
    sprintf_s(tbuf2, MAX_PATH, "%s%s.cRZ", Dir_Serum, MycRom.name);
    if (!CopyFileA(tbuf2, tbuf, FALSE))
    {
        MessageBoxA(hWnd, "Can't copy the cRZ file in the altcolor\\rom_name directory", "Failed", MB_OK);
        return;
    }*/
    //const char* pidmd = InitDmdDevice(Dir_VP, MycRom.name);
    if (!InitLibSerum(Dir_VP, MycRom.name))
    {
        MessageBoxA(hWnd, "Couldn't start Serum64.dll v2.0.0+ from the VPinMame folder you chose, thanks to choose another one and try again", "Failed", MB_OK);
        Dir_VP[0] = 0;
        SavePaths();
        return;
    }
    DialogBoxA(hInst,MAKEINTRESOURCEA(IDD_TESTER), hWnd, Tester_Proc);
    //StopDmdDevice();
    StopLibSerum();
}
void ConvertMaskToSourceReso(UINT8* pDst, UINT8* pSrc)
{
    if (!nEditExtraResolutionF)
    {
        memcpy(pDst, pSrc, MycRom.fWidth * MycRom.fHeight);
    }
    else if (MycRom.fHeight == 64)
    {
        for (UINT tj = 0; tj < MycRom.fHeight; tj++)
        {
            for (UINT ti = 0; ti < MycRom.fWidth; ti++)
                pDst[tj * MycRom.fWidth + ti] = pSrc[(tj / 2) * MycRom.fWidthX + ti / 2];
        }
    }
    else
    {
        for (UINT tj = 0; tj < MycRom.fHeight; tj++)
        {
            for (UINT ti = 0; ti < MycRom.fWidth; ti++)
            {
                pDst[tj * MycRom.fWidth + ti] = pSrc[(tj * 2) * MycRom.fWidthX + ti * 2];
            }
        }
    }
}
void AutoCopy(void)
{
    if (MycRom.name[0] == 0) return;
    if ((WSelection == 0) || (HSelection == 0)) return;
    UINT* pselcrc32 = (UINT*)malloc(sizeof(UINT) * MycRom.nFrames);
    if (!pselcrc32)
    {
        cprintf(true, "Auto copy didn't get the memory for CRC32 calculations");
        return;
    }
    UINT8 Copy_Mask_ORG[256*64];
    ConvertMaskToSourceReso(Copy_Mask_ORG, Copy_Mask);
    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
        pselcrc32[ti] = crc32_fast_mask(&MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight], Copy_Mask_ORG, MycRom.fWidth * MycRom.fHeight);
    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
    {
        if (ti == acFrame) continue;
        if (pselcrc32[acFrame] == pselcrc32[ti])
        {
            if (!nEditExtraResolutionF)
            {
                for (UINT tj = 0; tj < MycRom.fWidth * MycRom.fHeight; tj++)
                {
                    if (Copy_Mask_ORG[tj] != 0)
                    {
                        MycRom.cFrames[ti * MycRom.fWidth * MycRom.fHeight + tj] = Copy_ColN[tj];
                        MycRom.DynaMasks[ti * MycRom.fWidth * MycRom.fHeight + tj] = Copy_Dyna[tj];
                    }
                }
            }
            else
            {
                for (UINT tj = 0; tj < MycRom.fWidthX * MycRom.fHeightX; tj++)
                {
                    if (Copy_Mask_ORG[tj] != 0)
                    {
                        MycRom.cFrames[ti * MycRom.fWidthX * MycRom.fHeightX + tj] = Copy_ColN[tj];
                        MycRom.DynaMasks[ti * MycRom.fWidthX * MycRom.fHeightX + tj] = Copy_Dyna[tj];
                    }
                }
            }
        }
    }
    free(pselcrc32);
    MessageBoxA(hWnd, "Auto Copy completed", "Info", MB_OK);
}

void Draw_Sprite(float zoom, unsigned int ofx, unsigned int ofy)
{
    if ((MycRom.name[0] == 0) || (MycRom.nSprites == 0)) return;
    glfwMakeContextCurrent(glfwsprites);
    glDisable(GL_BLEND);
    if (ExtraResSClicked && MycRom.isExtraSprite[acSprite] == 0)
    {
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLES);
        glColor4f(1, 0, 0, 1);
        glVertex2i(10, 0);
        glVertex2i(ScrWsprite, ScrHsprite - 10);
        glVertex2i(ScrWsprite - 10, ScrHsprite);
        glVertex2i(10, 0);
        glVertex2i(ScrWsprite - 10, ScrHsprite);
        glVertex2i(0, 10);
        glVertex2i(ScrWsprite - 10, 0);
        glVertex2i(ScrWsprite, 10);
        glVertex2i(10, ScrHsprite);
        glVertex2i(ScrWsprite - 10, 0);
        glVertex2i(10, ScrHsprite);
        glVertex2i(0, ScrHsprite - 10);
        glEnd();
        return;
    }
    glBindTexture(GL_TEXTURE_2D, TxCircleSpr);
    float sprite_zoom2 = zoom;// sprite_zoom* sprite_zoom_mul;
    UINT16* pSpr;
    UINT8* pSpro;
    if (nEditExtraResolutionS)
    {
        pSpr = &MycRom.SpriteColoredX[acSprite * MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH];
        pSpro = &MycRom.SpriteMaskX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
    }
    else
    {
        pSpr = &MycRom.SpriteColored[acSprite * MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH];
        pSpro = &MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
    }
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    for (UINT tj = ofy; tj < MAX_SPRITE_HEIGHT; tj++)
    {
        for (UINT ti = ofx; ti < MAX_SPRITE_WIDTH; ti++)
        {
            if (pSpro[ti + tj * MAX_SPRITE_WIDTH] < 255)
            {
                SetRenderDrawColor565(pSpr[ti + tj * MAX_SPRITE_WIDTH], 255);
                RenderDrawPointClip(glfwsprites, (ti - ofx) * zoom, (tj - ofy) * zoom, ScrWsprite - 1, ScrHsprite - 1, zoom);
            }
        }
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor4ub(mselcol, mselcol, mselcol, 255);
    for (UINT tj = ofy; tj < MAX_SPRITE_HEIGHT; tj++)
    {
        for (UINT ti = ofx; ti < MAX_SPRITE_WIDTH; ti++)
        {
            if (pSpro[ti + tj * MAX_SPRITE_WIDTH] == 255)
            {
                glVertex2f((ti - ofx) * sprite_zoom2, (tj - ofy) * sprite_zoom2);
                glVertex2f((ti - ofx + 1) * sprite_zoom2, (tj - ofy + 1) * sprite_zoom2);
            }
        }
    }
    if (!ExtraResSClicked)
    {
        UINT acdetspr = (UINT)SendMessage(GetDlgItem(hwTB2, IDC_DETSPR), CB_GETCURSEL, 0, 0);
        UINT16* psda = &MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acdetspr * 4];
        glColor4ub(255, 255, 255, 255);
        if ((*psda) != 0xffff)
        {
            for (UINT16 tj = psda[1]; tj < psda[1] + psda[3]; tj++)
            {
                for (UINT16 ti = psda[0]; ti < psda[0] + psda[2]; ti++)
                {
                    Draw_Line((float)((ti - ofx) * sprite_zoom2), (float)((tj - ofy) * sprite_zoom2), (float)((ti - ofx + 1) * sprite_zoom2 + 1), (float)((tj - ofy + 1) * sprite_zoom2 + 1));
                    Draw_Line((float)((ti - ofx) * sprite_zoom2), (float)((tj - ofy + 1) * sprite_zoom2 + 1), (float)((ti - ofx + 1) * sprite_zoom2 + 1), (float)((tj - ofy) * sprite_zoom2));
                }
            }
        }
    }
    glEnd();
}


void UpdateSelImage(void)
{
    glfwMakeContextCurrent(glfwimages);
    glBindTexture(GL_TEXTURE_2D, TxSelImage);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WiSelection * IMAGE_ZOOM_TEXMUL, HiSelection * IMAGE_ZOOM_TEXMUL, GL_RGBA, GL_UNSIGNED_BYTE, pSelImage);
}

void UpdateCropSize(void)
{
    if (initcropwidth > 0)
    {
        crop_iOsizeW = image_sizeW - crop_reductioni;
        crop_iOsizeH = (UINT)(crop_iOsizeW / SelRatio);
        crop_fOsizeW = image_sizeW - crop_reductionf;
        crop_fOsizeH = (UINT)(crop_fOsizeW / SelRatio);
        crop_sizeW = image_sizeW - crop_reduction;
        crop_sizeH = (UINT)(crop_sizeW / SelRatio);
    }
    else
    {
        crop_iOsizeH = image_sizeH - crop_reductioni;
        crop_iOsizeW = (UINT)(crop_iOsizeH * SelRatio);
        crop_fOsizeH = image_sizeH - crop_reductionf;
        crop_fOsizeW = (UINT)(crop_fOsizeH * SelRatio);
        crop_sizeH = image_sizeH - crop_reduction;
        crop_sizeW = (UINT)(crop_sizeH * SelRatio);
    }
}
void UpdateSelBuffer(void)
{
    UINT fw;
    //if (nEditExtraResolutionF) fw = MycRom.fWidthX; else fw = MycRom.fWidth;
    if (MycRom.fHeight == 64) fw = MycRom.fWidth; else fw = MycRom.fWidthX;
    for (int tj = 0; tj < (int)HiSelection; tj++)
    {
        for (int ti = 0; ti < (int)WiSelection; ti++)
        {
            if ((Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOFRAME)) == BST_CHECKED && Copy_iMask[(YiSelection + tj) * fw + XiSelection + ti] > 0) ||
                Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
            {
                for (int tk = 0; tk < IMAGE_ZOOM_TEXMUL; tk++)
                {
                    for (int tl = 0; tl < IMAGE_ZOOM_TEXMUL; tl++)
                    {
                        pSelImage[(ti * IMAGE_ZOOM_TEXMUL + tl + (tj * IMAGE_ZOOM_TEXMUL + tk) * WiSelection * IMAGE_ZOOM_TEXMUL) * 4 + 3] = 0;
                    }
                }
            }
            else
                int tfd = 54;
        }
    }
}

void CalcImageCropFrame(void)
{
    crop_offsetx = 0;
    crop_offsety = 0;
    for (int ti = 0; ti < 256 * IMAGE_ZOOM_TEXMUL * 64 * IMAGE_ZOOM_TEXMUL; ti++) pSelImage[ti * 4 + 3] = IMAGE_MASK_OPACITY;
    if (TxImage >= 0)
    {
        SelRatio = (float)WiSelection / (float)HiSelection;
        if (SelRatio > (float)width_image / (float)height_image)
        {
            initcropwidth = width_image;
            initcropheight = 0;
        }
        else
        {
            initcropwidth = 0;
            initcropheight = height_image;
        }
        if (NiSelection > 0)
        {
            UpdateSelBuffer();
            UpdateSelImage();
        }
    }
}
void GetSelectionSize(void)
{
    UINT xmax = 0, ymax = 0;
    UINT fw, fh;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        Copy_Content = 2;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        Copy_Content = 1;
    }
    XSelection = fw;
    YSelection = fh;
    NSelection = 0;
    for (UINT tj = 0; tj < fh; tj++)
    {
        for (UINT ti = 0; ti < fw; ti++)
        {
            if (Copy_Mask[tj * fw + ti] > 0)
            {
                NSelection++;
                if (ti < XSelection) XSelection = ti;
                if (ti > xmax) xmax = ti;
                if (tj < YSelection) YSelection = tj;
                if (tj > ymax) ymax = tj;
            }
        }
    }
    WSelection = xmax - XSelection + 1;
    HSelection = ymax - YSelection + 1;
    if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
    {
        if (MycRom.fHeightX == 64)
        {
            XiSelection = YiSelection = 0;
            WiSelection = MycRom.fWidthX;
            HiSelection = MycRom.fHeightX;
            NiSelection = WiSelection * HiSelection;
        }
        else
        {
            XiSelection = YiSelection = 0;
            WiSelection = MycRom.fWidth;
            HiSelection = MycRom.fHeight;
            NiSelection = WiSelection * HiSelection;
        }
    }
    else
    {
        int maxwidth = max((int)MycRom.fWidth, (int)MycRom.fWidthX);
        int minwidth = min((int)MycRom.fWidth, (int)MycRom.fWidthX);
        if ((nEditExtraResolutionF && MycRom.fHeight == 64) || (!nEditExtraResolutionF && MycRom.fHeightX == 64))
        {
            XiSelection = XSelection * 2;
            YiSelection = YSelection * 2;
            WiSelection = WSelection * 2;
            HiSelection = HSelection * 2;
            NiSelection = NSelection * 2;
            for (int tj = 0; tj < 64; tj++)
            {
                for (int ti = 0; ti < maxwidth; ti++)
                {
                    Copy_iMask[tj * maxwidth + ti] = Copy_iMask[tj * maxwidth + ti + 1] =
                        Copy_iMask[(tj + 1) * maxwidth + ti] = Copy_iMask[(tj + 1) * maxwidth + ti + 1] =
                        Copy_Mask[tj / 2 * minwidth + ti / 2];
                }
            }
        }
        else
        {
            XiSelection = XSelection;
            YiSelection = YSelection;
            WiSelection = WSelection;
            HiSelection = HSelection;
            NiSelection = NSelection;
            memcpy(Copy_iMask, Copy_Mask, maxwidth * 64);
            if (WiSelection % 2 > 0)
            {
                if (XiSelection > 0) XiSelection--;
                WiSelection++;
            }
            if (HiSelection % 2 > 0)
            {
                if (YiSelection > 0) YiSelection--;
                HiSelection++;
            }
        }
    }
    
    crop_reduction = 0;
    crop_reductioni = 0;
    crop_reductionf = 0;
    image_zoom_srce = false;
    image_zoom_dest = false;
    CalcImageCropFrame();
    UpdateCropSize();
}

#define MAX_RECTAS_CALC 12

typedef struct
{
    int nrects;
    int x[4];
    int w[4];
    int y[4];
    int h[4];
}linerect_struct;
linerect_struct myrectas[MAX_RECTAS_CALC];
int nlrectas = 0, nrectas[MAX_RECTAS_CALC] = { 0,0,0,0,0,0,0 };

void CalcRectas(int lasty, int acy, bool* recton)
{
    if (nlrectas == MAX_RECTAS_CALC - 1) return;
    UINT8* pfill = (UINT8*)malloc(image_sizeW+1);
    if (!pfill) return;
    memset(pfill, 0, image_sizeW);
    nrectas[nlrectas] = 0;
    if (recton[0]) memset(&pfill[crop_offsetx], 1, min(crop_sizeW, image_sizeW - crop_offsetx));
    if (recton[1]) memset(&pfill[crop_ioffsetx], 1, min(crop_iOsizeW, image_sizeW - crop_ioffsetx));
    if (recton[2]) memset(&pfill[crop_foffsetx], 1, min(crop_fOsizeW, image_sizeW - crop_foffsetx));
    int initi = 0;
    while ((pfill[initi] > 0) && (initi < (int)image_sizeW)) initi++;
    if (initi == image_sizeW)
    {
        free(pfill);
        return;
    }
    for (int ti = initi + 1; (ti < (int)image_sizeW) && (nrectas[nlrectas] < 4); ti++)
    {
        if ((pfill[ti] > 0) || (ti == image_sizeW - 1))
        {
            myrectas[nlrectas].x[nrectas[nlrectas]] = initi;
            if (ti != image_sizeW - 1) myrectas[nlrectas].w[nrectas[nlrectas]] = ti - initi;
            else myrectas[nlrectas].w[nrectas[nlrectas]] = ti - initi + 1;
            myrectas[nlrectas].y[nrectas[nlrectas]] = lasty;
            myrectas[nlrectas].h[nrectas[nlrectas]] = acy - lasty + 2;
            nrectas[nlrectas]++;
            while ((ti < (int)image_sizeW) && (pfill[ti] > 0)) ti++;
            initi = ti;
        }
    }
    nlrectas++;
    free(pfill);
}

void OrderRectas(void)
{
    UINT yrecta[3 * 2] = { (UINT)crop_offsety,(UINT)(crop_offsety + crop_sizeH),(UINT)crop_ioffsety,(UINT)(crop_ioffsety + crop_iOsizeH),(UINT)crop_foffsety,(UINT)(crop_foffsety + crop_fOsizeH) };
    bool recton[3] = { false,false,false };
    bool rectnotpassed[3] = { true,true,true };
    if (!image_zoom_srce)
    {
        yrecta[2] = 10000;
        yrecta[3] = 10004;
        rectnotpassed[1] = false;
    }
    if (!image_zoom_dest)
    {
        yrecta[4] = 10000;
        yrecta[5] = 10004;
        rectnotpassed[2] = false;
    }
    int ti = 0, lastti = 0;
    nlrectas = 0;
    while ((ti < (int)image_sizeH) && ((rectnotpassed[0] || rectnotpassed[1] || rectnotpassed[2])))
    {
        for (UINT tj = 0; tj < 3 * 2; tj++)
        {
            if (ti == yrecta[tj])
            {
                if (ti > lastti)
                {
                    CalcRectas(lastti, ti-1, recton);
                    lastti = ti;
                }
                break;
            }
        }
        for (UINT tj = 0; tj < 3 * 2; tj++)
        {
            if (ti == yrecta[tj])
            {
                if (tj == 0) recton[0] = true;
                else if (tj == 1)
                {
                    recton[0] = false;
                    rectnotpassed[0] = false;
                }
                else if (tj == 2) recton[1] = true;
                else if (tj == 3)
                {
                    recton[1] = false;
                    rectnotpassed[1] = false;
                }
                else if (tj == 4) recton[2] = true;
                else if (tj == 5)
                {
                    recton[2] = false;
                    rectnotpassed[2] = false;
                }
            }
        }
        ti++;
    }
    if (ti <= (int)image_sizeH)
    {
        CalcRectas(lastti, image_sizeH - 1, recton);
    }
}
void Draw_Image(void)
{
    glfwMakeContextCurrent(glfwimages);
    glColor4f(1, 1, 1, 1);

    if (TxImage == (UINT)-1) return;
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxImage);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0, 0);
    glVertex2i(image_posx, image_posy);
    glTexCoord2f(1, 0);
    glVertex2i(image_posx + image_sizeW - 1, image_posy);
    glTexCoord2f(1, 1);
    glVertex2i(image_posx + image_sizeW - 1, image_posy + image_sizeH - 1);
    glTexCoord2f(0, 0);
    glVertex2i(image_posx, image_posy);
    glTexCoord2f(1, 1);
    glVertex2i(image_posx + image_sizeW - 1, image_posy + image_sizeH - 1);
    glTexCoord2f(0, 1);
    glVertex2i(image_posx, image_posy + image_sizeH - 1);
    glEnd();

    if (NiSelection == 0) return;
    if (TxSelImage == (UINT)-1) return;
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxSelImage);
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0, 0);
    glVertex2i(image_posx + crop_offsetx, image_posy + crop_offsety);
    glTexCoord2f((float)WiSelection / 256.0f, 0);
    glVertex2i(image_posx + crop_offsetx + crop_sizeW - 1, image_posy + crop_offsety);
    glTexCoord2f((float)WiSelection / 256.0f, (float)HiSelection / 64.0f);
    glVertex2i(image_posx + crop_offsetx + crop_sizeW-1, image_posy + crop_offsety + crop_sizeH-1);
    glTexCoord2f(0, 0);
    glVertex2i(image_posx + crop_offsetx, image_posy + crop_offsety);
    glTexCoord2f((float)WiSelection / 256.0f, (float)HiSelection / 64.0f);
    glVertex2i(image_posx + crop_offsetx + crop_sizeW-1, image_posy + crop_offsety + crop_sizeH-1);
    glTexCoord2f(0, (float)HiSelection / 64.0f);
    glVertex2i(image_posx + crop_offsetx, image_posy + crop_offsety + crop_sizeH-1);
    if (image_zoom_srce)
    {
        glTexCoord2f(0, 0);
        glVertex2i(image_posx + crop_ioffsetx, image_posy + crop_ioffsety);
        glTexCoord2f((float)WiSelection / 256.0f, 0);
        glVertex2i(image_posx + crop_ioffsetx + crop_iOsizeW-1, image_posy + crop_ioffsety);
        glTexCoord2f((float)WiSelection / 256.0f, (float)HiSelection / 64.0f);
        glVertex2i(image_posx + crop_ioffsetx + crop_iOsizeW-1, image_posy + crop_ioffsety + crop_iOsizeH-1);
        glTexCoord2f(0, 0);
        glVertex2i(image_posx + crop_ioffsetx, image_posy + crop_ioffsety);
        glTexCoord2f((float)WiSelection / 256.0f, (float)HiSelection / 64.0f);
        glVertex2i(image_posx + crop_ioffsetx + crop_iOsizeW-1, image_posy + crop_ioffsety + crop_iOsizeH-1);
        glTexCoord2f(0, (float)HiSelection / 64.0f);
        glVertex2i(image_posx + crop_ioffsetx, image_posy + crop_ioffsety + crop_iOsizeH-1);
    }
    if (image_zoom_dest)
    {
        glTexCoord2f(0, 0);
        glVertex2i(image_posx + crop_foffsetx, image_posy + crop_foffsety);
        glTexCoord2f((float)WiSelection / 256.0f, 0);
        glVertex2i(image_posx + crop_foffsetx + crop_fOsizeW-1, image_posy + crop_foffsety);
        glTexCoord2f((float)WiSelection / 256.0f, (float)HiSelection / 64.0f);
        glVertex2i(image_posx + crop_foffsetx + crop_fOsizeW-1, image_posy + crop_foffsety + crop_fOsizeH-1);
        glTexCoord2f(0, 0);
        glVertex2i(image_posx + crop_foffsetx, image_posy + crop_foffsety);
        glTexCoord2f((float)WiSelection / 256.0f, (float)HiSelection / 64.0f);
        glVertex2i(image_posx + crop_foffsetx + crop_fOsizeW-1, image_posy + crop_foffsety + crop_fOsizeH-1);
        glTexCoord2f(0, (float)HiSelection / 64.0f);
        glVertex2i(image_posx + crop_foffsetx, image_posy + crop_foffsety + crop_fOsizeH-1);
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glColor4f(0, 0, 0, 200.0f / 255.0f);
    OrderRectas();
    glBegin(GL_TRIANGLES);
    for (int ti = 0; ti < nlrectas; ti++)
    {
        for (int tj = 0; tj < nrectas[ti]; tj++)
        {
            glVertex2i(image_posx + myrectas[ti].x[tj]-1, image_posy + myrectas[ti].y[tj]);
            glVertex2i(image_posx + myrectas[ti].x[tj] + myrectas[ti].w[tj] - 1, image_posy + myrectas[ti].y[tj]);
            glVertex2i(image_posx + myrectas[ti].x[tj] + myrectas[ti].w[tj] - 1, image_posy + myrectas[ti].y[tj] + myrectas[ti].h[tj] - 1);
            glVertex2i(image_posx + myrectas[ti].x[tj]-1, image_posy + myrectas[ti].y[tj]);
            glVertex2i(image_posx + myrectas[ti].x[tj] + myrectas[ti].w[tj] - 1, image_posy + myrectas[ti].y[tj] + myrectas[ti].h[tj] - 1);
            glVertex2i(image_posx + myrectas[ti].x[tj]-1, image_posy + myrectas[ti].y[tj] + myrectas[ti].h[tj] - 1);
        }
    }
    glEnd();

    glColor4f(1, 1, 1, 1);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor4f(1, 0, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex2i(image_posx + crop_offsetx, image_posy + crop_offsety);
    glVertex2i(image_posx + crop_offsetx + crop_sizeW-1, image_posy + crop_offsety);
    glVertex2i(image_posx + crop_offsetx + crop_sizeW-1, image_posy + crop_offsety + crop_sizeH - 1);
    glVertex2i(image_posx + crop_offsetx, image_posy + crop_offsety + crop_sizeH - 1);
    glVertex2i(image_posx + crop_offsetx, image_posy + crop_offsety);
    glEnd();
    if (image_zoom_srce)
    {
        glColor4f(1, 0, 0, 1);
        glBegin(GL_LINE_STRIP);
        glVertex2i(image_posx + crop_ioffsetx, image_posy + crop_ioffsety);
        glVertex2i(image_posx + crop_ioffsetx + crop_iOsizeW-1, image_posy + crop_ioffsety);
        glVertex2i(image_posx + crop_ioffsetx + crop_iOsizeW-1, image_posy + crop_ioffsety + crop_iOsizeH - 1);
        glVertex2i(image_posx + crop_ioffsetx, image_posy + crop_ioffsety + crop_iOsizeH - 1);
        glVertex2i(image_posx + crop_ioffsetx, image_posy + crop_ioffsety);
        glEnd();
    }
    if (image_zoom_dest)
    {
        glColor4f(0, 1, 0, 1);
        glBegin(GL_LINE_STRIP);
        glVertex2i(image_posx + crop_foffsetx, image_posy + crop_foffsety);
        glVertex2i(image_posx + crop_foffsetx + crop_fOsizeW-1, image_posy + crop_foffsety);
        glVertex2i(image_posx + crop_foffsetx + crop_fOsizeW-1, image_posy + crop_foffsety + crop_fOsizeH - 1);
        glVertex2i(image_posx + crop_foffsetx, image_posy + crop_foffsety + crop_fOsizeH - 1);
        glVertex2i(image_posx + crop_foffsetx, image_posy + crop_foffsety);
        glEnd();
    }
}

#pragma endregion Window_Tools_And_Drawings

#pragma region Project_File_Functions

void Free_cRP(bool freeimported)
{
    if (MycRP.name[0] != 0)
    {
        MycRP.name[0] = 0;
    }
    if (MycRP.oFrames)
    {
        free(MycRP.oFrames);
        MycRP.oFrames = NULL;
    }
    if (MycRP.FrameDuration)
    {
        free(MycRP.FrameDuration);
        MycRP.FrameDuration = NULL;
    }
    if (MycRP.importedPal && freeimported)
    {
        free(MycRP.importedPal);
        MycRP.importedPal = NULL;
    }
}
void Free_cRom(void)
{
    if (MycRom.name[0] != 0)
    {
        MycRom.name[0] = 0;
        if (MycRom.HashCode)
        {
            free(MycRom.HashCode);
            MycRom.HashCode = NULL;
        }
        if (MycRom.CompMaskID)
        {
            free(MycRom.CompMaskID);
            MycRom.CompMaskID = NULL;
        }
        if (MycRom.ShapeCompMode)
        {
            free(MycRom.ShapeCompMode);
            MycRom.ShapeCompMode = NULL;
        }
        if (MycRom.CompMasks)
        {
            free(MycRom.CompMasks);
            MycRom.CompMasks = NULL;
        }
        if (MycRom.isExtraFrame)
        {
            free(MycRom.isExtraFrame);
            MycRom.isExtraFrame = NULL;
        }
        if (MycRom.cFrames)
        {
            free(MycRom.cFrames);
            MycRom.cFrames = NULL;
        }
        if (MycRom.cFramesX)
        {
            free(MycRom.cFramesX);
            MycRom.cFramesX = NULL;
        }
        if (MycRom.DynaMasks)
        {
            free(MycRom.DynaMasks);
            MycRom.DynaMasks = NULL;
        }
        if (MycRom.DynaMasksX)
        {
            free(MycRom.DynaMasksX);
            MycRom.DynaMasksX = NULL;
        }
        if (MycRom.Dyna4Cols)
        {
            free(MycRom.Dyna4Cols);
            MycRom.Dyna4Cols = NULL;
        }
        if (MycRom.Dyna4ColsX)
        {
            free(MycRom.Dyna4ColsX);
            MycRom.Dyna4ColsX = NULL;
        }
        if (MycRom.isExtraSprite)
        {
            free(MycRom.isExtraSprite);
            MycRom.isExtraSprite = NULL;
        }
        if (MycRom.FrameSprites)
        {
            free(MycRom.FrameSprites);
            MycRom.FrameSprites = NULL;
        }
        if (MycRom.FrameSpriteBB)
        {
            free(MycRom.FrameSpriteBB);
            MycRom.FrameSpriteBB = NULL;
        }
        if (MycRom.SpriteOriginal)
        {
            free(MycRom.SpriteOriginal);
            MycRom.SpriteOriginal = NULL;
        }
        if (MycRom.SpriteColored)
        {
            free(MycRom.SpriteColored);
            MycRom.SpriteColored = NULL;
        }
        if (MycRom.SpriteMaskX)
        {
            free(MycRom.SpriteMaskX);
            MycRom.SpriteMaskX = NULL;
        }
        if (MycRom.SpriteColoredX)
        {
            free(MycRom.SpriteColoredX);
            MycRom.SpriteColoredX = NULL;
        }
        if (MycRom.TriggerID)
        {
            free(MycRom.TriggerID);
            MycRom.TriggerID = NULL;
        }
        if (MycRom.ColorRotations)
        {
            free(MycRom.ColorRotations);
            MycRom.ColorRotations = NULL;
        }
        if (MycRom.ColorRotationsX)
        {
            free(MycRom.ColorRotationsX);
            MycRom.ColorRotationsX = NULL;
        }
        if (MycRom.SpriteDetDwords)
        {
            free(MycRom.SpriteDetDwords);
            MycRom.SpriteDetDwords = NULL;
        }
        if (MycRom.SpriteDetDwordPos)
        {
            free(MycRom.SpriteDetDwordPos);
            MycRom.SpriteDetDwordPos = NULL;
        }
        if (MycRom.SpriteDetAreas)
        {
            free(MycRom.SpriteDetAreas);
            MycRom.SpriteDetAreas = NULL;
        }
        if (MycRom.isExtraBackground)
        {
            free(MycRom.isExtraBackground);
            MycRom.isExtraBackground = NULL;
        }
        if (MycRom.BackgroundFrames)
        {
            free(MycRom.BackgroundFrames);
            MycRom.BackgroundFrames = NULL;
        }
        if (MycRom.BackgroundFramesX)
        {
            free(MycRom.BackgroundFramesX);
            MycRom.BackgroundFramesX = NULL;
        }
        if (MycRom.BackgroundID)
        {
            free(MycRom.BackgroundID);
            MycRom.BackgroundID = NULL;
        }
        if (MycRom.BackgroundMask)
        {
            free(MycRom.BackgroundMask);
            MycRom.BackgroundMask = NULL;
        }
        if (MycRom.BackgroundMaskX)
        {
            free(MycRom.BackgroundMaskX);
            MycRom.BackgroundMaskX = NULL;
        }
    }
    acBG = acSprite = acFrame = 0;
    CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
    CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
    prevAcFrame = (UINT)-1;
    prevAcBG = (UINT16)-1;
    PreFrameInStrip = 0;
}
void Free_Project(void)
{
    Free_cRP(true);
    Free_cRom();
    InitVariables();
}
void SaveVars(void)
{
    HKEY tKey;
    LSTATUS ls = RegCreateKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\ColorizingDMD", 0, NULL, 0, KEY_WRITE, NULL, &tKey, NULL);
    if (ls == ERROR_SUCCESS)
    {
        DWORD nm = 0;
        if (Night_Mode) nm = 1;
        RegSetValueExA(tKey, "VAR_DARKMODE", 0, REG_DWORD, (const BYTE*)&nm, 4);
    }
}

void LoadVars(void)
{
    HKEY tKey;
    LSTATUS ls = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\ColorizingDMD", 0, KEY_READ, &tKey);
    Night_Mode = false;
    if (ls == ERROR_SUCCESS)
    {
        DWORD size = 4;
        DWORD val;
        RegGetValueA(tKey, NULL, "VAR_DARKMODE", RRF_RT_REG_DWORD, 0, &val, &size);
        if (val == 0) Night_Mode = true; else Night_Mode = false;
        SendMessage(hwTB, WM_COMMAND, IDC_NIGHTDAY, 0);
    }
}

void SavePaths(void)
{
    HKEY tKey;
    LSTATUS ls = RegCreateKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\ColorizingDMD", 0, NULL, 0, KEY_WRITE, NULL, &tKey, NULL);
    if (ls == ERROR_SUCCESS)
    {
        RegSetValueExA(tKey, "DIR_DUMPS", 0, REG_SZ, (const BYTE*)Dir_Dumps, (DWORD)strlen(Dir_Dumps) + 1);
        RegSetValueExA(tKey, "DIR_IMAGES", 0, REG_SZ, (const BYTE*)Dir_Images, (DWORD)strlen(Dir_Images) + 1);
        RegSetValueExA(tKey, "DIR_SERUM", 0, REG_SZ, (const BYTE*)Dir_Serum, (DWORD)strlen(Dir_Serum) + 1);
        RegSetValueExA(tKey, "DIR_GIFS", 0, REG_SZ, (const BYTE*)Dir_GIFs, (DWORD)strlen(Dir_GIFs) + 1);
        RegSetValueExA(tKey, "DIR_VP", 0, REG_SZ, (const BYTE*)Dir_VP, (DWORD)strlen(Dir_VP) + 1);
    }
}

void LoadPaths(void)
{
    HKEY tKey;
    LSTATUS ls = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\ColorizingDMD", 0, KEY_READ, &tKey);
    if (ls == ERROR_SUCCESS)
    {
        DWORD size = 260;
        RegGetValueA(tKey, NULL, "DIR_DUMPS", RRF_RT_ANY, 0, Dir_Dumps, &size);
        size = 260;
        RegGetValueA(tKey, NULL, "DIR_IMAGES", RRF_RT_ANY, 0, Dir_Images, &size);
        size = 260;
        RegGetValueA(tKey, NULL, "DIR_SERUM", RRF_RT_ANY, 0, Dir_Serum, &size);
        size = 260;
        RegGetValueA(tKey, NULL, "DIR_GIFS", RRF_RT_ANY, 0, Dir_GIFs, &size);
        size = 260;
        RegGetValueA(tKey, NULL, "DIR_VP", RRF_RT_ANY, 0, Dir_VP, &size);
    }
}

void SaveWindowPosition(void)
{
    HKEY tKey;
    LSTATUS ls = RegCreateKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\ColorizingDMD", 0, NULL, 0, KEY_WRITE, NULL, &tKey, NULL);
    if (ls == ERROR_SUCCESS)
    {
        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hWnd, &wp);
        RegSetValueExA(tKey, "MAIN_LEFT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.left, 4);
        RegSetValueExA(tKey, "MAIN_RIGHT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.right, 4);
        RegSetValueExA(tKey, "MAIN_TOP", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.top, 4);
        RegSetValueExA(tKey, "MAIN_BOTTOM", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.bottom, 4);
        int isIcon = IsIconic(hWnd);
        if (!isIcon) isIcon = 2 * IsZoomed(hWnd);
        RegSetValueExA(tKey, "MAIN_MINIMIZED", 0, REG_DWORD, (const BYTE*)&isIcon, 4);
        GetWindowPlacement(hSprites, &wp);
        RegSetValueExA(tKey, "SPRITE_LEFT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.left, 4);
        RegSetValueExA(tKey, "SPRITE_RIGHT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.right, 4);
        RegSetValueExA(tKey, "SPRITE_TOP", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.top, 4);
        RegSetValueExA(tKey, "SPRITE_BOTTOM", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.bottom, 4);
        isIcon = IsIconic(hSprites);
        if (!isIcon) isIcon = 2 * IsZoomed(hSprites);
        RegSetValueExA(tKey, "SPRITE_MINIMIZED", 0, REG_DWORD, (const BYTE*)&isIcon, 4);
        GetWindowPlacement(hImages, &wp);
        RegSetValueExA(tKey, "IMAGE_LEFT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.left, 4);
        RegSetValueExA(tKey, "IMAGE_RIGHT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.right, 4);
        RegSetValueExA(tKey, "IMAGE_TOP", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.top, 4);
        RegSetValueExA(tKey, "IMAGE_BOTTOM", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.bottom, 4);
        isIcon = IsIconic(hImages);
        if (!isIcon) isIcon = 2 * IsZoomed(hImages);
        RegSetValueExA(tKey, "IMAGE_MINIMIZED", 0, REG_DWORD, (const BYTE*)&isIcon, 4);
        GetWindowPlacement(hBG, &wp);
        RegSetValueExA(tKey, "BG_LEFT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.left, 4);
        RegSetValueExA(tKey, "BG_RIGHT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.right, 4);
        RegSetValueExA(tKey, "BG_TOP", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.top, 4);
        RegSetValueExA(tKey, "BG_BOTTOM", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.bottom, 4);
        isIcon = IsIconic(hBG);
        if (!isIcon) isIcon = 2 * IsZoomed(hBG);
        RegSetValueExA(tKey, "BG_MINIMIZED", 0, REG_DWORD, (const BYTE*)&isIcon, 4);
        RegSetValueExA(tKey, "TESTER_LEFT", 0, REG_DWORD, (const BYTE*)&Tester_Posx, 4);
        RegSetValueExA(tKey, "TESTER_TOP", 0, REG_DWORD, (const BYTE*)&Tester_Posy, 4);
        RegCloseKey(tKey);
    }
}

void LoadWindowPosition(void)
{
    HKEY tKey;
    LSTATUS ls = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\ColorizingDMD",0,KEY_READ,&tKey);
    int posxm = 0, posym = 0, widm = 500, heim = 500;
    int posxs = 500, posys = 0, wids = 500, heis = 500;
    int posxi = 250, posyi = 500, widi = 500, heii = 600;
    int posxb = 150, posyb = 450, widb = 500, heib = 500;
    int isIconm, isIcons, isIconi, isIconb;
    if (ls == ERROR_SUCCESS)
    {
        DWORD size = 4;
        RegGetValueA(tKey, NULL, "TESTER_LEFT", RRF_RT_REG_DWORD, 0, &Tester_Posx, &size);
        RegGetValueA(tKey, NULL, "TESTER_TOP", RRF_RT_REG_DWORD, 0, &Tester_Posy, &size);
        RegGetValueA(tKey, NULL, "IMAGE_LEFT", RRF_RT_REG_DWORD, 0, &posxm, &size);
        RegGetValueA(tKey, NULL, "IMAGE_TOP", RRF_RT_REG_DWORD, 0, &posym, &size);
        RegGetValueA(tKey, NULL, "IMAGE_RIGHT", RRF_RT_REG_DWORD, 0, &widm, &size);
        widm -= posxm - 1;
        RegGetValueA(tKey, NULL, "IMAGE_BOTTOM", RRF_RT_REG_DWORD, 0, &heim, &size);
        heim -= posym - 1;
        RegGetValueA(tKey, NULL, "IMAGE_MINIMIZED", RRF_RT_REG_DWORD, 0, &isIconm, &size);

        RegGetValueA(tKey, NULL, "BG_LEFT", RRF_RT_REG_DWORD, 0, &posxb, &size);
        RegGetValueA(tKey, NULL, "BG_TOP", RRF_RT_REG_DWORD, 0, &posyb, &size);
        RegGetValueA(tKey, NULL, "BG_RIGHT", RRF_RT_REG_DWORD, 0, &widb, &size);
        widb -= posxb - 1;
        RegGetValueA(tKey, NULL, "BG_BOTTOM", RRF_RT_REG_DWORD, 0, &heib, &size);
        heib -= posyb - 1;
        RegGetValueA(tKey, NULL, "BG_MINIMIZED", RRF_RT_REG_DWORD, 0, &isIconb, &size);

        RegGetValueA(tKey, NULL, "SPRITE_LEFT", RRF_RT_REG_DWORD, 0, &posxs, &size);
        RegGetValueA(tKey, NULL, "SPRITE_TOP", RRF_RT_REG_DWORD, 0, &posys, &size);
        RegGetValueA(tKey, NULL, "SPRITE_RIGHT", RRF_RT_REG_DWORD, 0, &wids, &size);
        wids -= posxs - 1;
        RegGetValueA(tKey, NULL, "SPRITE_BOTTOM", RRF_RT_REG_DWORD, 0, &heis, &size);
        heis -= posys - 1;
        RegGetValueA(tKey, NULL, "SPRITE_MINIMIZED", RRF_RT_REG_DWORD, 0, &isIcons, &size);

        RegGetValueA(tKey, NULL, "MAIN_LEFT", RRF_RT_REG_DWORD, 0, &posxi, &size);
        RegGetValueA(tKey, NULL, "MAIN_TOP", RRF_RT_REG_DWORD, 0, &posyi, &size);
        RegGetValueA(tKey, NULL, "MAIN_RIGHT", RRF_RT_REG_DWORD, 0, &widi, &size);
        widi -= posxi - 1;
        RegGetValueA(tKey, NULL, "MAIN_BOTTOM", RRF_RT_REG_DWORD, 0, &heii, &size);
        heii -= posyi - 1;
        RegGetValueA(tKey, NULL, "MAIN_MINIMIZED", RRF_RT_REG_DWORD, 0, &isIconi, &size);

        RegCloseKey(tKey);
        
        SetWindowPos(hImages, HWND_TOP, posxm, posym, widm, heim, SWP_SHOWWINDOW);
        if (isIconm == 1) ShowWindow(hImages, SW_SHOWMINIMIZED);
        else if (isIconm == 2) ShowWindow(hImages, SW_SHOWMAXIMIZED);
        SetWindowPos(hBG, HWND_TOP, posxb, posyb, widb, heib, SWP_SHOWWINDOW);
        if (isIconb == 1) ShowWindow(hBG, SW_SHOWMINIMIZED);
        else if (isIconb == 2) ShowWindow(hBG, SW_SHOWMAXIMIZED);
        SetWindowPos(hSprites, HWND_TOP, posxs, posys, wids, heis, SWP_SHOWWINDOW);
        if (isIcons == 1) ShowWindow(hSprites, SW_SHOWMINIMIZED);
        else if (isIcons == 2) ShowWindow(hSprites, SW_SHOWMAXIMIZED);
        SetWindowPos(hWnd, HWND_TOP, posxi, posyi, widi, heii, SWP_SHOWWINDOW);
        if (isIconi == 1) ShowWindow(hWnd, SW_SHOWMINIMIZED);
        else if (isIconi == 2) ShowWindow(hWnd, SW_SHOWMAXIMIZED);
    }
}
bool ColorizedFrame(UINT nofr)
{
    UINT16* pcfr = &MycRom.cFrames[nofr * MycRom.fHeight * MycRom.fWidth];
    UINT16* pcfrX = NULL;
    UINT8* pofr = &MycRP.oFrames[nofr * MycRom.fHeight * MycRom.fWidth];
    UINT8* pdyn = &MycRom.DynaMasks[nofr * MycRom.fHeight * MycRom.fWidth];
    UINT8* pdynX = NULL;
    for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        if (((*pcfr) != originalcolors[*pofr]) || (*pdyn != 255)) return true;
        pcfr++;
        pofr++;
        pdyn++;
    }
    return false;
}
bool Set_Detection_Dwords(void)
{
    UINT32 Dwords[MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
    UINT16 DwNum[MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
    for (UINT tm = 0; tm < MycRom.nSprites; tm++)
    {
        for (UINT tl = 0; tl < MAX_SPRITE_DETECT_AREAS; tl++)
        {
            memset(DwNum, 0, sizeof(UINT16) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
            UINT16* pdetarea = &MycRom.SpriteDetAreas[tm * 4 * MAX_SPRITE_DETECT_AREAS + tl * 4];
            if (pdetarea[0] == 0xffff) continue;
            UINT8* pspro = &MycRom.SpriteOriginal[tm * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + pdetarea[1] * MAX_SPRITE_WIDTH + pdetarea[0]];
            for (UINT16 tj = 0; tj < pdetarea[3]; tj++)
            {
                for (UINT16 ti = 0; ti < pdetarea[2] - 3; ti++)
                {
                    if ((pspro[tj * MAX_SPRITE_WIDTH + ti] < 255) && (pspro[tj * MAX_SPRITE_WIDTH + ti + 1] < 255) &&
                        (pspro[tj * MAX_SPRITE_WIDTH + ti + 2] < 255) && (pspro[tj * MAX_SPRITE_WIDTH + ti + 3] < 255))
                    {
                        UINT32 val = (UINT32)(pspro[tj * MAX_SPRITE_WIDTH + ti]) + (UINT32)(pspro[tj * MAX_SPRITE_WIDTH + ti + 1] << 8) +
                            (UINT32)(pspro[tj * MAX_SPRITE_WIDTH + ti + 2] << 16) + (UINT32)(pspro[tj * MAX_SPRITE_WIDTH + ti + 3] << 24);
                        UINT tk = pdetarea[0];
                        UINT tn = pdetarea[1];
                        UINT to = (pdetarea[1] + tj) * MAX_SPRITE_WIDTH + pdetarea[0] + ti;
                        while (tk + tn * MAX_SPRITE_WIDTH < to)
                        {
                            if (DwNum[tk + tn * MAX_SPRITE_WIDTH] != 0)
                            {
                                if (val == Dwords[tk + tn * MAX_SPRITE_WIDTH])
                                {
                                    DwNum[tk + tn * MAX_SPRITE_WIDTH]++;
                                    break;
                                }
                            }
                            tk++;
                            if (tk == pdetarea[0] + pdetarea[2])
                            {
                                tk = pdetarea[0];
                                tn++;
                            }
                            if (tk + tn * MAX_SPRITE_WIDTH == to)
                            {
                                Dwords[to] = val;
                                DwNum[to] = 1;
                            }
                        }
                    }
                }
            }
            UINT16 minval = MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH;
            INT16 acpos = -1;
            for (UINT16 tj = 0; tj < pdetarea[3]; tj++)
            {
                for (UINT16 ti = 0; ti < pdetarea[2] - 3; ti++)
                {
                    if (DwNum[(pdetarea[1] + tj) * MAX_SPRITE_WIDTH + pdetarea[0] + ti] > 0)
                    {
                        if (DwNum[(pdetarea[1] + tj) * MAX_SPRITE_WIDTH + pdetarea[0] + ti] < minval)
                        {
                            minval = DwNum[(pdetarea[1] + tj) * MAX_SPRITE_WIDTH + pdetarea[0] + ti];
                            acpos = (pdetarea[1] + tj) * MAX_SPRITE_WIDTH + pdetarea[0] + ti;
                        }
                    }
                }
            }
            if (acpos == -1)
            {
                char tbuf[256];
                sprintf_s(tbuf, 256, "The sprite \"%s\" (number %i) has no contiguous 4 pixels in the detection area number %i, it can not be used in our format. Save cancelled!", &MycRP.Sprite_Names[tm * SIZE_SECTION_NAMES], tm + 1, tl + 1);
                MessageBoxA(hWnd, tbuf, "Caution", MB_OK);
                return false;
            }
            MycRom.SpriteDetDwords[tm * MAX_SPRITE_DETECT_AREAS + tl] = Dwords[acpos];
            MycRom.SpriteDetDwordPos[tm * MAX_SPRITE_DETECT_AREAS + tl] = acpos;
        }
    }
    return true;
}
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    LPITEMIDLIST pidlNavigate;
    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            pidlNavigate = (LPITEMIDLIST)lpData;
            if (pidlNavigate != NULL) SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)FALSE, (LPARAM)pidlNavigate);
            break;
        }
    }
    return 0;
}

long long ac_pos_in_file;

size_t my_fwrite(const void* pBuffer, size_t sizeElement, size_t nElements, FILE* stream)
{
    size_t written = fwrite(pBuffer, sizeElement, nElements, stream);
    ac_pos_in_file += written * sizeElement;
    cprintf(false, "sent elements: %llu / written elements: %llu / written bytes: %llu / current position: %llu", nElements, written, written * sizeElement, ac_pos_in_file);
    return written;
}

bool Save_cRom(bool autosave, bool fastsave, char* forcepath)
{
    ac_pos_in_file = 0;
    if (MycRom.name[0] == 0) return true;
    UINT8* pactiveframes = (UINT8*)malloc(MycRom.nFrames);
    if (!pactiveframes)
    {
        cprintf(true, "Can't get memory for active frames. Action canceled");
        return false;
    }
    for (UINT32 ti = 0; ti < MycRom.nFrames; ti++)
    {
        if (MycRom.CompMaskID[ti] == 255) MycRom.HashCode[ti] = crc32_fast(&MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight, MycRom.ShapeCompMode[ti]);
        else MycRom.HashCode[ti] = crc32_fast_mask_shape(&MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight], &MycRom.CompMasks[MycRom.CompMaskID[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight, MycRom.ShapeCompMode[ti]);
    }
    if (!Set_Detection_Dwords()) return false;

    char acDir[260];
    GetCurrentDirectoryA(260, acDir);

    char tbuf[MAX_PATH];
    if (forcepath[0] != 0)
    {
        sprintf_s(tbuf, MAX_PATH, "%s%s.cRom", forcepath, MycRom.name);
    }
    else if ((!fastsave && !autosave) || NewProj)
    {
        LPITEMIDLIST pidlStart;
        pidlStart = ILCreateFromPathA(Dir_Serum);
        char szDir[MAX_PATH];
        BROWSEINFOA bInfo;
        bInfo.hwndOwner = hWnd;
        bInfo.pidlRoot = NULL;
        bInfo.pszDisplayName = szDir;
        bInfo.lpszTitle = "Please, select the Serum directory";
        bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        bInfo.lpfn = BrowseCallbackProc;
        bInfo.lParam = (LPARAM)pidlStart;
        bInfo.iImage = -1;
        LPITEMIDLIST lpItem = SHBrowseForFolderA(&bInfo);
        CoTaskMemFree(pidlStart);
        if (lpItem != NULL)
        {
            SHGetPathFromIDListA(lpItem, Dir_Serum);
            if ((Dir_Serum[strlen(Dir_Serum) - 1] != '/') && (Dir_Serum[strlen(Dir_Serum) - 1] != '\\')) strcat_s(Dir_Serum, MAX_PATH, "\\");
            SavePaths();
            sprintf_s(tbuf, MAX_PATH, "%s%s.crom", Dir_Serum, MycRom.name);
            CoTaskMemFree(lpItem);
        }
        else
        {
            cprintf(true, "The Serum file was not saved");
            return false;
        }
    }
    else
    {
        if (!autosave) sprintf_s(tbuf, MAX_PATH, "%s%s.cROM", Dir_Serum, MycRom.name); else sprintf_s(tbuf, MAX_PATH, "%s%s(auto).cROM", Dir_Serum, MycRom.name);
    }
    FILE* pfile;
    if (fopen_s(&pfile, tbuf, "wb") != 0)
    {
        if (fastsave || autosave)
        {
            LPITEMIDLIST pidlStart;
            pidlStart = ILCreateFromPathA(Dir_Serum);
            char szDir[MAX_PATH];
            BROWSEINFOA bInfo;
            bInfo.hwndOwner = hWnd;
            bInfo.pidlRoot = NULL;
            bInfo.pszDisplayName = szDir;
            bInfo.lpszTitle = "Please, select the Serum directory";
            bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            bInfo.lpfn = BrowseCallbackProc;
            bInfo.lParam = (LPARAM)pidlStart;
            bInfo.iImage = -1;
            LPITEMIDLIST lpItem = SHBrowseForFolderA(&bInfo);
            CoTaskMemFree(pidlStart);
            if (lpItem != NULL)
            {
                SHGetPathFromIDListA(lpItem, Dir_Serum);
                if ((Dir_Serum[strlen(Dir_Serum) - 1] != '/') && (Dir_Serum[strlen(Dir_Serum) - 1] != '\\')) strcat_s(Dir_Serum, MAX_PATH, "\\");
                SavePaths();
                sprintf_s(tbuf, MAX_PATH, "%s%s.crom", Dir_Serum, MycRom.name);
                CoTaskMemFree(lpItem);
            }
            else
            {
                cprintf(true, "The Serum file was not saved");
                return false;
            }
            if (fopen_s(&pfile, tbuf, "wb") != 0)
            {
                MessageBoxA(hWnd, "Unable to save the Serum project, save again and check you chose a directory where you have write rights", "Failed", MB_OK);
                NewProj = true;
                return false;
            }
        }
        else
        {
            MessageBoxA(hWnd, "Unable to save the Serum project, save again and check you chose a directory where you have write rights", "Failed", MB_OK);
            NewProj = true;
            return false;
        }
    }
    NewProj = false;
    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
    {
        for (UINT tj = 0; tj < MycRom.fHeight * MycRom.fWidth; tj++)
        {
            if (MycRom.DynaMasks[ti * MycRom.fHeight * MycRom.fWidth + tj] < 255)
                MycRom.cFrames[ti * MycRom.fHeight * MycRom.fWidth + tj] = 0;
        }
        for (UINT tj = 0; tj < MycRom.fHeightX * MycRom.fWidthX; tj++)
        {
            if (MycRom.DynaMasksX[ti * MycRom.fHeightX * MycRom.fWidthX + tj] < 255)
                MycRom.cFramesX[ti * MycRom.fHeightX * MycRom.fWidthX + tj] = 0;
        }
    }
    my_fwrite(MycRom.name, 1, 64, pfile);
    UINT lengthheader = 14 * sizeof(UINT);
    my_fwrite(&lengthheader, sizeof(UINT), 1, pfile);
    my_fwrite(&MycRom.fWidth, sizeof(UINT), 1, pfile);
    my_fwrite(&MycRom.fHeight, sizeof(UINT), 1, pfile);
    my_fwrite(&MycRom.fWidthX, sizeof(UINT), 1, pfile);
    my_fwrite(&MycRom.fHeightX, sizeof(UINT), 1, pfile);
    my_fwrite(&MycRom.nFrames, sizeof(UINT), 1, pfile);
    my_fwrite(&MycRom.noColors, sizeof(UINT), 1, pfile);
    //my_fwrite(&MycRom.ncColors, sizeof(UINT), 1, pfile);
    my_fwrite(&MycRom.nCompMasks, sizeof(UINT), 1, pfile);
    //my_fwrite(&MycRom.nMovMasks, sizeof(UINT), 1, pfile);
    my_fwrite(&MycRom.nSprites, sizeof(UINT), 1, pfile);
    my_fwrite(&MycRom.nBackgrounds, sizeof(UINT16), 1, pfile);
    my_fwrite(MycRom.HashCode, sizeof(UINT), MycRom.nFrames, pfile);
    my_fwrite(MycRom.ShapeCompMode, 1, MycRom.nFrames, pfile);
    my_fwrite(MycRom.CompMaskID, 1, MycRom.nFrames, pfile);
    //my_fwrite(MycRom.MovRctID, 1, MycRom.nFrames, pfile);
    if (MycRom.nCompMasks) my_fwrite(MycRom.CompMasks, 1, MycRom.nCompMasks * MycRom.fWidth * MycRom.fHeight, pfile);
    //if (MycRom.nMovMasks) my_fwrite(MycRom.MovRcts, 1, MycRom.nMovMasks * 4, pfile);
    my_fwrite(MycRom.isExtraFrame, 1, MycRom.nFrames, pfile);
    //my_fwrite(MycRom.cPal, 1, MycRom.nFrames * 3 * MycRom.ncColors, pfile);
    my_fwrite(MycRom.cFrames, sizeof(UINT16), MycRom.nFrames* MycRom.fWidth* MycRom.fHeight , pfile);
    my_fwrite(MycRom.cFramesX, sizeof(UINT16), MycRom.nFrames* MycRom.fWidthX* MycRom.fHeightX, pfile);
    my_fwrite(MycRom.DynaMasks, 1, MycRom.nFrames* MycRom.fWidth* MycRom.fHeight, pfile);
    my_fwrite(MycRom.DynaMasksX, 1, MycRom.nFrames* MycRom.fWidthX* MycRom.fHeightX, pfile);
    my_fwrite(MycRom.Dyna4Cols, sizeof(UINT16), MycRom.nFrames* MAX_DYNA_SETS_PER_FRAMEN* MycRom.noColors, pfile);
    my_fwrite(MycRom.Dyna4ColsX, sizeof(UINT16), MycRom.nFrames* MAX_DYNA_SETS_PER_FRAMEN* MycRom.noColors, pfile);
    my_fwrite(MycRom.isExtraSprite, 1, MycRom.nSprites, pfile);
    my_fwrite(MycRom.FrameSprites, 1, MycRom.nFrames * MAX_SPRITES_PER_FRAME, pfile);
    my_fwrite(MycRom.SpriteOriginal, 1, MycRom.nSprites* MAX_SPRITE_WIDTH* MAX_SPRITE_HEIGHT, pfile);
    my_fwrite(MycRom.SpriteColored, sizeof(UINT16), MycRom.nSprites* MAX_SPRITE_WIDTH* MAX_SPRITE_HEIGHT, pfile);
    my_fwrite(MycRom.SpriteMaskX, 1, MycRom.nSprites* MAX_SPRITE_WIDTH* MAX_SPRITE_HEIGHT, pfile);
    my_fwrite(MycRom.SpriteColoredX, sizeof(UINT16), MycRom.nSprites* MAX_SPRITE_WIDTH* MAX_SPRITE_HEIGHT, pfile);
    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
    {
        pactiveframes[ti] = 1;
        if (MycRP.FrameDuration[ti] >= SKIP_FRAME_DURATION || ColorizedFrame(ti)) continue;
        pactiveframes[ti] = 0;
    }
    my_fwrite(pactiveframes, 1, MycRom.nFrames, pfile);
    free(pactiveframes);
    my_fwrite(MycRom.ColorRotations, sizeof(UINT16), MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * MycRom.nFrames, pfile);
    my_fwrite(MycRom.ColorRotationsX, sizeof(UINT16), MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * MycRom.nFrames, pfile);
    my_fwrite(MycRom.SpriteDetDwords, sizeof(UINT32), MycRom.nSprites * MAX_SPRITE_DETECT_AREAS, pfile);
    my_fwrite(MycRom.SpriteDetDwordPos, sizeof(UINT16), MycRom.nSprites * MAX_SPRITE_DETECT_AREAS, pfile);
    my_fwrite(MycRom.SpriteDetAreas, sizeof(UINT16), MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
    my_fwrite(MycRom.TriggerID, sizeof(UINT32), MycRom.nFrames, pfile);
    my_fwrite(MycRom.FrameSpriteBB, sizeof(UINT16), MycRom.nFrames* MAX_SPRITES_PER_FRAME * 4, pfile);
    my_fwrite(MycRom.isExtraBackground, 1, MycRom.nBackgrounds, pfile);
    my_fwrite(MycRom.BackgroundFrames, sizeof(UINT16), MycRom.nBackgrounds* MycRom.fWidth* MycRom.fHeight, pfile);
    my_fwrite(MycRom.BackgroundFramesX, sizeof(UINT16), MycRom.nBackgrounds* MycRom.fWidthX* MycRom.fHeightX, pfile);
    my_fwrite(MycRom.BackgroundID, sizeof(UINT16), MycRom.nFrames, pfile);
    my_fwrite(MycRom.BackgroundMask, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    my_fwrite(MycRom.BackgroundMaskX, 1, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX, pfile);
    fclose(pfile);
    HZIP hz;
    if (!autosave)
    {
        char tbuf2[MAX_PATH];
        if (forcepath[0] == 0) sprintf_s(tbuf2, MAX_PATH, "%s%s.cRZ", Dir_Serum, MycRom.name); else sprintf_s(tbuf2, MAX_PATH, "%s%s.cRZ", forcepath, MycRom.name);
        ZipCreateFileA(&hz, tbuf2, 0);
        sprintf_s(tbuf2, MAX_PATH, "%s.cRom", MycRom.name);
        ZipAddFileA(hz, tbuf2, tbuf);
        ZipClose(hz);
        if (forcepath[0] != 0)
        {
            sprintf_s(tbuf2, MAX_PATH, "%s%s.cRom", forcepath, MycRom.name);
            DeleteFileA(tbuf2);
        }

    }
    return true;
}

void ConvertPalFramesToRGB565Frames(UINT8* cPal, UINT8* oldFrames, UINT nFrames, UINT nValPerFrame, UINT16* newFrames)
{
    UINT8* pof = oldFrames;
    UINT16* pnf = newFrames;
    for (UINT ti = 0; ti < nFrames; ti++)
    {
        for (UINT tj = 0; tj < nValPerFrame; tj++)
        {
            *pnf = rgb888_to_rgb565(cPal[ti * 3 * 64 + (*pof) * 3], cPal[ti * 3 * 64 + (*pof) * 3 + 1], cPal[ti * 3 * 64 + (*pof) * 3 + 2]);
            pnf++;
            pof++;
        }
    }
}
void ConvertPalDynaColsToRGB565DynaCols(UINT8* cPal, UINT8* oDynaCols, UINT nFrames, UINT16* nDynaCols)
{
    for (UINT ti = 0; ti < nFrames; ti++)
    {
        UINT8* pof = &oDynaCols[ti * MycRom.noColors * 16];
        UINT16* pnf = &nDynaCols[ti * MycRom.noColors * MAX_DYNA_SETS_PER_FRAMEN];
        for (UINT tj = 0; tj < MAX_DYNA_SETS_PER_FRAMEN; tj++)
        {
            for (UINT tk = 0; tk < MycRom.noColors; tk++)
            {
                if (tj < 16) pnf[tj * MycRom.noColors + tk] = rgb888_to_rgb565(cPal[ti * 3 * 64 + pof[tj * MycRom.noColors + tk] * 3], cPal[ti * 3 * 64 + pof[tj * MycRom.noColors + tk] * 3 + 1], cPal[ti * 3 * 64 + pof[tj * MycRom.noColors + tk] * 3 + 2]);
                else pnf[tj * MycRom.noColors + tk] = 0;
            }
        }
    }
}
UINT GetAFrameUsingThisSprite(int nospr, UINT8* framesprites, UINT nframes)
{
    for (UINT ti = 0; ti < nframes; ti++)
    {
        for (int tj = 0; tj < MAX_SPRITES_PER_FRAME; tj++)
        {
            if (nospr == framesprites[ti * MAX_SPRITES_PER_FRAME + tj]) return ti;
        }
    }
    return 0;
}

void ConvertPalSpritesToRGB565Sprites(UINT8* cPal, UINT16* SpriteDescriptions, UINT8* FrameSprites, UINT nSprites, UINT nFrames, UINT8* SpriteOriginal, UINT16* SpriteColored)
{
    for (UINT ti = 0; ti < nSprites; ti++)
    {
        int sproffs = ti * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT;
        int sproffso = ti * 128 * 128;
        for (int tj = 0; tj < min(128, MAX_SPRITE_HEIGHT); tj++)
        {
            for (int tk = 0; tk < 256; tk++)
            {
                if (tk >= 128) SpriteOriginal[sproffs + tj * 256 + tk] = 255;
                else
                {
                    if ((SpriteDescriptions[sproffso + tj * 128 + tk] & 0x8000) != 0) SpriteOriginal[sproffs + tj * 256 + tk] = 255;
                    else
                    {
                        SpriteOriginal[sproffs + tj * 256 + tk] = SpriteDescriptions[sproffso + tj * 128 + tk] >> 8;
                        UINT nofr = GetAFrameUsingThisSprite(ti, FrameSprites, nFrames);
                        SpriteColored[sproffs + tj * 256 + tk] = rgb888_to_rgb565(cPal[nofr * 3 * 64 + (SpriteDescriptions[sproffso + tj * 128 + tk] & 0xff) * 3],
                            cPal[nofr * 3 * 64 + (SpriteDescriptions[sproffso + tj * 128 + tk] & 0xff) * 3 + 1],
                            cPal[nofr * 3 * 64 + (SpriteDescriptions[sproffso + tj * 128 + tk] & 0xff) * 3 + 2]);
                    }
                }
            }
        }
    }
}
void ConvertPalRotationsToRGB565Rotations(UINT8* cPal, UINT8* oColorRotations, UINT nFrames, UINT16* cColorRotations)
{
    for (UINT ti = 0; ti < nFrames; ti++)
    {
        int tj = 0;
        int tj2 = 0;
        while (tj < MAX_COLOR_ROTATIONN && tj2 < 8)
        {
            UINT8* psrot = &oColorRotations[ti * 8 * 3 + tj2 * 3];
            UINT16* pdrot = &cColorRotations[ti * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + tj * MAX_LENGTH_COLOR_ROTATION];
            tj2++;
            UINT8 prerotcol = psrot[0];
            if (prerotcol == 255) continue;
            pdrot[0] = (UINT16)min((int)psrot[1], MAX_LENGTH_COLOR_ROTATION - 2);
            pdrot[1] = (UINT16)psrot[2] * 10;
            for (UINT8 tk = 0; tk < pdrot[0]; tk++)
            {
                pdrot[2 + tk] = rgb888_to_rgb565(cPal[ti * 64 * 3 + (prerotcol + tk) * 3],
                    cPal[ti * 64 * 3 + (prerotcol + tk) * 3 + 1],
                    cPal[ti * 64 * 3 + (prerotcol + tk) * 3 + 2]);
            }
            tj++;
        } 
    }
}

void ConvertBackgrounbdBBsToMasks(UINT16* BackgroundBB, UINT16* BackgroundID, UINT16 nFrames, UINT width, UINT height, UINT8* BackgroundMask)
{
    for (UINT tk = 0; tk < nFrames; tk++)
    {
        if (BackgroundID[tk] == 0xffff) continue;
        UINT16* pbbb = &BackgroundBB[tk * 4];
        UINT8* pbm = &BackgroundMask[tk * width * height];
        UINT minx = (UINT)pbbb[0], miny = (UINT)pbbb[1], maxx = (UINT)pbbb[2], maxy = (UINT)pbbb[3];
        for (UINT tj = 0; tj < height; tj++)
        {
            for (UINT ti = 0; ti < width; ti++)
            {
                if (ti<minx || ti>maxx || tj<miny || tj>maxy) pbm[tj * width + ti] = 0;
                else pbm[tj * width + ti] = 1;
            }
        }
    }
}

UINT lengthheader = 14 * sizeof(UINT);
bool Load_cRom(char* name)
{
    Free_cRom();

    FILE* pfile;
    if (fopen_s(&pfile, name, "rb") != 0)
    {
        AffLastError((char*)"Load_cRom:fopen_s");
        return false;
    }
    UINT trash;
    fread(MycRom.name, 1, 64, pfile);
    fread(&lengthheader, sizeof(UINT), 1, pfile);
    bool isNewFormat = (lengthheader >= 14 * sizeof(UINT));
    fread(&MycRom.fWidth, sizeof(UINT), 1, pfile);
    fread(&MycRom.fHeight, sizeof(UINT), 1, pfile);
    if (isNewFormat)
    {
        MycRP.importedPal = NULL;
        fread(&MycRom.fWidthX, sizeof(UINT), 1, pfile);
        fread(&MycRom.fHeightX, sizeof(UINT), 1, pfile);
    }
    else
    {
        if (MycRom.fHeight == 64)
        {
            MycRom.fHeightX = MycRom.fHeight / 2;
            MycRom.fWidthX = MycRom.fWidth / 2;
        }
        else
        {
            MycRom.fHeightX = MycRom.fHeight * 2;
            MycRom.fWidthX = MycRom.fWidth * 2;
        }
    }
    fread(&MycRom.nFrames, sizeof(UINT), 1, pfile);
    fread(&MycRom.noColors, sizeof(UINT), 1, pfile);
    if (!isNewFormat)
    {
        MycRP.isImported = MycRom.nFrames;
        fread(&trash, sizeof(UINT), 1, pfile);
    }
    else MycRP.isImported = 0;
    fread(&MycRom.nCompMasks, sizeof(UINT), 1, pfile);
    if (!isNewFormat) fread(&trash, sizeof(UINT), 1, pfile);
    fread(&MycRom.nSprites, sizeof(UINT), 1, pfile);
    if (lengthheader >= 13 * sizeof(UINT)) fread(&MycRom.nBackgrounds, sizeof(UINT16), 1, pfile); else MycRom.nBackgrounds = 0;
    MycRom.HashCode = (UINT*)malloc(sizeof(UINT) * MycRom.nFrames);
    MycRom.ShapeCompMode = (UINT8*)malloc(MycRom.nFrames);
    MycRom.CompMaskID = (UINT8*)malloc(MycRom.nFrames);
    //UINT8* MovRctID = (UINT8*)malloc(MycRom.nFrames); // never used
    MycRom.CompMasks = (UINT8*)malloc(MAX_MASKS * MycRom.fWidth * MycRom.fHeight);
    //UINT8* MovRcts = (UINT8*)malloc(MycRom.nMovMasks * 4); // never has been so not needed, the number is mandatory 0
    MycRom.isExtraFrame = (UINT8*)malloc(MycRom.nFrames);
    if (!isNewFormat)
    {
        MycRP.importedPal = (UINT8*)malloc(MycRP.isImported * 3 * 64);
        if (!MycRP.importedPal)
        {
            cprintf(true, "Can't get the importedpal buffer in Load_cRom");
            Free_cRom();
            fclose(pfile);
            return false;
        }
    }
    else MycRP.importedPal = NULL;
    UINT8* cFrames = (UINT8*)malloc(MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.cFrames = (UINT16*)malloc(MycRom.nFrames * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    MycRom.cFramesX = (UINT16*)malloc(MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    MycRom.DynaMasks = (UINT8*)malloc(MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.DynaMasksX = (UINT8*)malloc(MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    UINT8* Dyna4Cols = (UINT8*)malloc(MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    MycRom.Dyna4Cols = (UINT16*)malloc(MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    MycRom.Dyna4ColsX = (UINT16*)malloc(MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    MycRom.isExtraSprite = (UINT8*)malloc(MycRom.nSprites);
    MycRom.FrameSprites = (UINT8*)malloc(MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    MycRom.FrameSpriteBB = (UINT16*)malloc(MycRom.nFrames * MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16));
    UINT16* SpriteDescriptions = (UINT16*)malloc(MycRom.nSprites * 128 * 128 * sizeof(UINT16)); 
    MycRom.SpriteOriginal = (UINT8*)malloc(MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    MycRom.SpriteColored = (UINT16*)malloc(MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    MycRom.SpriteMaskX = (UINT8*)malloc(MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    MycRom.SpriteColoredX = (UINT16*)malloc(MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
    UINT8* ColorRotations = (UINT8*)malloc(MycRom.nFrames * 3 * 8);
    MycRom.ColorRotations = (UINT16*)malloc(MycRom.nFrames * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    MycRom.ColorRotationsX = (UINT16*)malloc(MycRom.nFrames * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    MycRom.SpriteDetDwords = (UINT32*)malloc(MycRom.nSprites * sizeof(UINT32) * MAX_SPRITE_DETECT_AREAS);
    MycRom.SpriteDetDwordPos = (UINT16*)malloc(MycRom.nSprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS);
    MycRom.SpriteDetAreas = (UINT16*)malloc(MycRom.nSprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS * 4);
    MycRom.TriggerID = (UINT32*)malloc(MycRom.nFrames * sizeof(UINT32));
    MycRom.isExtraBackground = (UINT8*)malloc(MycRom.nBackgrounds);
    //UINT8* BackgroundFrames = (UINT8*)malloc(MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight);
    MycRom.BackgroundFrames = (UINT16*)malloc(MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    MycRom.BackgroundFramesX = (UINT16*)malloc(MycRom.nBackgrounds * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    MycRom.BackgroundID = (UINT16*)malloc(MycRom.nFrames * sizeof(UINT16));
    UINT16* BackgroundBB = (UINT16*)malloc(4 * sizeof(UINT16) * MycRom.nFrames);
    MycRom.BackgroundMask = (UINT8*)malloc(MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.BackgroundMaskX = (UINT8*)malloc(MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
    if (!MycRom.HashCode || !MycRom.ShapeCompMode || !MycRom.CompMaskID || !MycRom.CompMasks || !MycRom.isExtraFrame ||
        !cFrames || !MycRom.cFrames || !MycRom.cFramesX || !MycRom.DynaMasks || !MycRom.DynaMasksX ||
        !Dyna4Cols || !MycRom.Dyna4Cols || !MycRom.Dyna4ColsX || !MycRom.isExtraSprite || !MycRom.FrameSprites ||
        !MycRom.FrameSpriteBB || !SpriteDescriptions||!MycRom.SpriteOriginal || !MycRom.SpriteColored || !MycRom.SpriteMaskX ||
        !MycRom.SpriteColoredX || !ColorRotations || !MycRom.ColorRotations || !MycRom.ColorRotationsX ||
        !MycRom.SpriteDetDwords ||!MycRom.SpriteDetDwordPos || !MycRom.SpriteDetAreas || !MycRom.TriggerID ||
        !MycRom.isExtraBackground || !MycRom.BackgroundFrames || !MycRom.BackgroundFramesX ||
        !BackgroundBB || !MycRom.BackgroundID || !MycRom.BackgroundMask || !MycRom.BackgroundMaskX)
    {
        cprintf(true, "Can't get the buffers in Load_cRom");
        Free_cRom();
        fclose(pfile);
        return false;
    }
    memset(MycRom.CompMasks, 0, MAX_MASKS * MycRom.fWidth * MycRom.fHeight);
    fread(MycRom.HashCode, sizeof(UINT), MycRom.nFrames, pfile);
    fread(MycRom.ShapeCompMode, 1, MycRom.nFrames, pfile);
    fread(MycRom.CompMaskID, 1,  MycRom.nFrames, pfile);
    if (!isNewFormat) fseek(pfile, MycRom.nFrames, SEEK_CUR);
    if (MycRom.nCompMasks) fread(MycRom.CompMasks, 1, MycRom.nCompMasks * MycRom.fWidth * MycRom.fHeight, pfile);
    //if (MycRom.nMovMasks) fread(MycRom.MovRcts, 1, MycRom.nMovMasks * 4, pfile);
    if (!isNewFormat)
    {
        memset(MycRom.isExtraFrame, 0, MycRom.nFrames);
        fread(MycRP.importedPal, 1, MycRom.nFrames * 3 * 64, pfile);
        fread(cFrames, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
        ConvertPalFramesToRGB565Frames(MycRP.importedPal, cFrames, MycRom.nFrames, MycRom.fWidth*MycRom.fHeight, MycRom.cFrames);
        memset(MycRom.cFramesX, 0, MycRom.nFrames * MycRom.fWidthX* MycRom.fHeightX);
    }
    else
    {
        fread(MycRom.isExtraFrame, 1, MycRom.nFrames, pfile);
        fread(MycRom.cFrames, sizeof(UINT16), MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
        fread(MycRom.cFramesX, sizeof(UINT16), MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX, pfile);
    }
    free(cFrames);
    fread(MycRom.DynaMasks, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    if (!isNewFormat)
    {
        memset(MycRom.DynaMasksX, 0xff, MycRom.nFrames* MycRom.fWidthX* MycRom.fHeightX);
        fread(Dyna4Cols, 1, MycRom.nFrames * 16 * MycRom.noColors, pfile);
        ConvertPalDynaColsToRGB565DynaCols(MycRP.importedPal, Dyna4Cols, MycRom.nFrames, MycRom.Dyna4Cols);
        memcpy(MycRom.Dyna4ColsX, MycRom.Dyna4Cols, MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
        memset(MycRom.isExtraSprite, 0, MycRom.nSprites);
    }
    else
    {
        fread(MycRom.DynaMasksX, 1, MycRom.nFrames* MycRom.fWidthX* MycRom.fHeightX, pfile);
        fread(MycRom.Dyna4Cols, sizeof(UINT16), MycRom.nFrames* MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors, pfile);
        fread(MycRom.Dyna4ColsX, sizeof(UINT16), MycRom.nFrames* MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors, pfile);
        fread(MycRom.isExtraSprite, 1, MycRom.nSprites, pfile);
    }
    free(Dyna4Cols);
    fread(MycRom.FrameSprites, 1, MycRom.nFrames * MAX_SPRITES_PER_FRAME, pfile);
    if (!isNewFormat)
    {
        fread(SpriteDescriptions, sizeof(UINT16), MycRom.nSprites * 128 * 128, pfile);
        ConvertPalSpritesToRGB565Sprites(MycRP.importedPal, SpriteDescriptions, MycRom.FrameSprites, MycRom.nSprites, MycRom.nFrames, MycRom.SpriteOriginal, MycRom.SpriteColored);
        memset(MycRom.SpriteMaskX, 255, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
        memset(MycRom.SpriteColoredX, 0, sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    }
    else
    {
        fread(MycRom.SpriteOriginal, 1, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile);
        fread(MycRom.SpriteColored, sizeof(UINT16), MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile);
        fread(MycRom.SpriteMaskX, 1, MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile);
        fread(MycRom.SpriteColoredX, sizeof(UINT16), MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile);
    }
    free(SpriteDescriptions);
    fseek(pfile, MycRom.nFrames, SEEK_CUR);
    memset(MycRom.ColorRotations, 0, sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * MycRom.nFrames);
    memset(MycRom.ColorRotationsX, 0, sizeof(UINT16) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * MycRom.nFrames);
    memset(MycRom.SpriteDetAreas, 255, sizeof(UINT16) * 4 * MAX_SPRITE_DETECT_AREAS * MycRom.nSprites);
    memset(MycRom.TriggerID, 0xff, sizeof(UINT32) * MycRom.nFrames);
    memset(MycRom.BackgroundID, 0xff, sizeof(UINT16) * MycRom.nFrames);
    for (UINT tj = 0; tj < MycRom.nFrames; tj++)
    {
        for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
        {
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = 0;
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = 0;
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = MycRom.fWidth - 1;
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = MycRom.fHeight - 1;
        }
    }
    memset(MycRom.isExtraBackground, 0, MycRom.nBackgrounds);
    if (lengthheader >= 9 * sizeof(UINT))
    {
        if (!isNewFormat)
        {
            fread(ColorRotations, 1, 3 * 8 * MycRom.nFrames, pfile);
            ConvertPalRotationsToRGB565Rotations(MycRP.importedPal, ColorRotations, MycRom.nFrames, MycRom.ColorRotations);
        }
        else
        {
            fread(MycRom.ColorRotations, sizeof(UINT16), MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * MycRom.nFrames, pfile);
            fread(MycRom.ColorRotationsX, sizeof(UINT16), MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * MycRom.nFrames, pfile);
        }
        if (lengthheader >= 10 * sizeof(UINT))
        {
            fread(MycRom.SpriteDetDwords, sizeof(UINT32), MycRom.nSprites * MAX_SPRITE_DETECT_AREAS, pfile);
            fread(MycRom.SpriteDetDwordPos, sizeof(UINT16), MycRom.nSprites * MAX_SPRITE_DETECT_AREAS, pfile);
            fread(MycRom.SpriteDetAreas, sizeof(UINT16), MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
            if (lengthheader >= 11 * sizeof(UINT))
            {
                fread(MycRom.TriggerID, sizeof(UINT32), MycRom.nFrames, pfile);
                if (lengthheader >= 12 * sizeof(UINT))
                {
                    fread(MycRom.FrameSpriteBB, sizeof(UINT16), MycRom.nFrames * MAX_SPRITES_PER_FRAME * 4, pfile);
                    if (lengthheader >= 13 * sizeof(UINT))
                    {
                        if (!isNewFormat)
                        {
                            fread(MycRom.BackgroundFrames, 1, MycRom.nBackgrounds* MycRom.fWidth* MycRom.fHeight, pfile);
                            memset(MycRom.BackgroundFramesX, 0, sizeof(UINT16) * MycRom.nBackgrounds * MycRom.fWidthX * MycRom.fHeightX);
                        }
                        else
                        {
                            fread(MycRom.isExtraBackground, 1, MycRom.nBackgrounds, pfile);
                            fread(MycRom.BackgroundFrames, sizeof(UINT16), MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight, pfile);
                            fread(MycRom.BackgroundFramesX, sizeof(UINT16), MycRom.nBackgrounds * MycRom.fWidthX * MycRom.fHeightX, pfile);
                        }
                        fread(MycRom.BackgroundID, sizeof(UINT16), MycRom.nFrames, pfile);
                        if (!isNewFormat)
                        {
                            fread(BackgroundBB, 4 * sizeof(UINT16), MycRom.nFrames, pfile);
                            ConvertBackgrounbdBBsToMasks(BackgroundBB, MycRom.BackgroundID, MycRom.nFrames, MycRom.fWidth, MycRom.fHeight, MycRom.BackgroundMask);
                            memset(MycRom.BackgroundMaskX, 0, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX);
                        }
                        else
                        {
                            fread(MycRom.BackgroundMask, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
                            fread(MycRom.BackgroundMaskX, 1, MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX, pfile);
                        }
                    }
                }
            }
        }
    }
    free(BackgroundBB);
    free(ColorRotations);
    //free(cPal);
    image_zoom_srce = false;
    image_zoom_dest = false;
    BlocPause = true;
    Ident_Pushed = false;
    BBIdent_Pushed = false;
    Common_Pushed = false;
    Update_Toolbar = true;
    fclose(pfile);
    return true;
}

bool Save_cRP(bool autosave)
{
    if (MycRP.name[0] == 0) return true;
    char tbuf[MAX_PATH];
    if (!autosave) sprintf_s(tbuf, MAX_PATH, "%s%s.cRP", Dir_Serum, MycRP.name); else sprintf_s(tbuf, MAX_PATH, "%s%s(auto).cRP", Dir_Serum, MycRP.name);
    FILE* pfile;
    if (fopen_s(&pfile, tbuf, "wb") != 0)
    {
        AffLastError((char*)"Save_cRP:fopen_s");
        return false;
    }
    fwrite(MycRP.name, 1, 64, pfile);
    fwrite(MycRP.oFrames, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    fwrite(MycRP.activeColSet, sizeof(BOOL), MAX_COL_SETS, pfile);
    fwrite(MycRP.ColSets, sizeof(UINT16), MAX_COL_SETS * 16, pfile);
    fwrite(&MycRP.acColSet, sizeof(UINT8), 1, pfile);
    fwrite(&MycRP.preColSet, sizeof(UINT8), 1, pfile);
    fwrite(MycRP.nameColSet, sizeof(char), MAX_COL_SETS * 64, pfile);
    fwrite(&MycRP.DrawColMode, sizeof(UINT32), 1, pfile);
    fwrite(&MycRP.Draw_Mode, sizeof(UINT8), 1, pfile);
    fwrite(&MycRP.Mask_Sel_Mode, sizeof(int), 1, pfile);
    fwrite(&MycRP.Fill_Mode, sizeof(BOOL), 1, pfile);
    fwrite(MycRP.Mask_Names, sizeof(char), MAX_MASKS * SIZE_MASK_NAME, pfile);
    fwrite(&MycRP.nSections, sizeof(UINT32), 1, pfile);
    fwrite(MycRP.Section_Firsts, sizeof(UINT32), MAX_SECTIONS, pfile);
    fwrite(MycRP.Section_Names, sizeof(char), MAX_SECTIONS * SIZE_SECTION_NAMES, pfile);
    fwrite(MycRP.Sprite_Names, sizeof(char), 255 * SIZE_SECTION_NAMES, pfile);
    fwrite(MycRP.Sprite_Col_From_Frame, sizeof(UINT32), 255, pfile);
    fwrite(MycRP.FrameDuration, sizeof(UINT32), MycRom.nFrames, pfile);
    fwrite(MycRP.SpriteRect, sizeof(UINT16), 4 * 255, pfile);
    fwrite(MycRP.SpriteRectMirror, sizeof(BOOL), 2 * 255, pfile);
    fwrite(MycRP.Palette, sizeof(UINT16), N_PALETTES * 64, pfile);
    fwrite(MycRP.acEditColorsS, sizeof(UINT16), 16, pfile);
    fwrite(&MycRP.nImagePosSaves, sizeof(UINT), 1, pfile);
    fwrite(MycRP.ImagePosSaveName, 1, N_IMAGE_POS_TO_SAVE * 64, pfile);
    fwrite(MycRP.ImagePosSave, sizeof(int), N_IMAGE_POS_TO_SAVE * 16, pfile);
    fwrite(MycRP.PalNames, 1, N_PALETTES * 64, pfile);
    fwrite(&MycRP.isImported, sizeof(UINT), 1, pfile);
    if (MycRP.isImported) fwrite(MycRP.importedPal, 1, 64 * 3 * MycRP.isImported, pfile);
    fclose(pfile);
    return true;
}

bool Load_cRP(char* name)
{
    bool isNewFormat = (lengthheader >= 14 * sizeof(UINT));
    Free_cRP(false);
    //char tbuf[MAX_PATH];
    //sprintf_s(tbuf, MAX_PATH, "%s%s", DumpDir, name);
    FILE* pfile;
    if (fopen_s(&pfile, name, "rb") != 0)
    {
        AffLastError((char*)"Load_cRP:fopen_s");
        Free_Project();
        return false;
    }
    MycRP.oFrames = (UINT8*)malloc(MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    if (!MycRP.oFrames)
    {
        cprintf(true, "Can't get the buffer in Load_cRP");
        Free_Project();
        fclose(pfile);
        return false;
    }
    MycRP.FrameDuration = (UINT32*)malloc(MycRom.nFrames * sizeof(UINT32));
    if (!MycRP.FrameDuration)
    {
        cprintf(true, "Can't get the buffer in Load_cRP");
        Free_Project();
        fclose(pfile);
        return false;
    }
    UINT8* BGPal = (UINT8*)malloc(MycRom.nBackgrounds * 3 * 64);
    if (!BGPal)
    {
        cprintf(true, "Can't get the buffer in Load_cRP");
        Free_Project();
        fclose(pfile);
        return false;
    }
    UINT8* BackgroundFrames = (UINT8*)malloc(MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight);
    if (!BackgroundFrames)
    {
        cprintf(true, "Can't get the buffer in Load_cRP");
        free(BGPal);
        Free_Project();
        fclose(pfile);
        return false;
    }
    fread(MycRP.name, 1, 64, pfile);
    fread(MycRP.oFrames, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    fread(MycRP.activeColSet, sizeof(BOOL), MAX_COL_SETS, pfile);
    if (!isNewFormat)
    {
        fseek(pfile, 64 * 16, SEEK_CUR);
        memset(MycRP.ColSets, 0, sizeof(UINT16) * MAX_COL_SETS * 16);
    }
    else
        fread(MycRP.ColSets, sizeof(UINT16), MAX_COL_SETS * 16, pfile);
    fread(&MycRP.acColSet, sizeof(UINT8), 1, pfile);
    fread(&MycRP.preColSet, sizeof(UINT8), 1, pfile);
    fread(MycRP.nameColSet, sizeof(char), MAX_COL_SETS * 64, pfile);
    fread(&MycRP.DrawColMode, sizeof(UINT32), 1, pfile);
    if (MycRP.DrawColMode == 2) MycRP.DrawColMode = 0;
    fread(&MycRP.Draw_Mode, sizeof(UINT8), 1, pfile);
    fread(&MycRP.Mask_Sel_Mode, sizeof(int), 1, pfile);
    fread(&MycRP.Fill_Mode, sizeof(BOOL), 1, pfile);
    fread(MycRP.Mask_Names, sizeof(char), MAX_MASKS * SIZE_MASK_NAME, pfile);
    fread(&MycRP.nSections, sizeof(UINT32), 1, pfile);
    fread(MycRP.Section_Firsts, sizeof(UINT32), MAX_SECTIONS, pfile);
    fread(MycRP.Section_Names, sizeof(char), MAX_SECTIONS * SIZE_SECTION_NAMES, pfile);
    fread(MycRP.Sprite_Names, sizeof(char), 255 * SIZE_SECTION_NAMES, pfile);
    fread(MycRP.Sprite_Col_From_Frame, sizeof(UINT32), 255, pfile);
    fread(MycRP.FrameDuration, sizeof(UINT32), MycRom.nFrames, pfile);
    if (!isNewFormat) fseek(pfile, 16 * 255 + 260, SEEK_CUR);
    fread(MycRP.SpriteRect, sizeof(UINT16), 4 * 255, pfile);
    fread(MycRP.SpriteRectMirror, sizeof(BOOL), 2 * 255, pfile);
    if (!isNewFormat && lengthheader >= 13 * sizeof(UINT))
    {
        fread(BGPal, 1, MycRom.nBackgrounds * 3 * 64, pfile);
        memcpy(BackgroundFrames, MycRom.BackgroundFrames, MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight);
        ConvertPalFramesToRGB565Frames(BGPal, BackgroundFrames, MycRom.nBackgrounds, MycRom.fWidth * MycRom.fHeight, MycRom.BackgroundFrames);
        fseek(pfile, MycRom.nBackgrounds * 3 * 8, SEEK_CUR);
    }
    if (isNewFormat)
    {
        fread(MycRP.Palette, sizeof(UINT16), N_PALETTES * 64, pfile);
        fread(MycRP.acEditColorsS, sizeof(UINT16), 16, pfile);
        fread(&MycRP.nImagePosSaves, sizeof(UINT), 1, pfile);
        fread(MycRP.ImagePosSaveName, 1, N_IMAGE_POS_TO_SAVE * 64, pfile);
        fread(MycRP.ImagePosSave, sizeof(int), N_IMAGE_POS_TO_SAVE * 16, pfile);
        fread(MycRP.PalNames, 1, N_PALETTES * 64, pfile);
        fread(&MycRP.isImported, sizeof(UINT), 1, pfile);
        if (MycRP.isImported)
        {
            MycRP.importedPal = (UINT8*)malloc(MycRP.isImported * 3 * 64);
            if (MycRP.importedPal) fread(MycRP.importedPal, 1, 64 * 3 * MycRP.isImported, pfile);
        }
    }
    else
    {
        memset(MycRP.Palette, 0, sizeof(UINT16) * N_PALETTES * 64);
        memset(MycRP.PalNames, 0, N_PALETTES * 64);
        Init_cFrame_Palette2();
    }
    free(BackgroundFrames);
    free(BGPal);
    fclose(pfile);
    acFrame = acSprite = acBG = 0;
    if (MycRom.isExtraBackground && MycRom.isExtraBackground[acBG] > 0) CheckDlgButton(hwTB4, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
    if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
    prevAcFrame = (UINT)-1;
    prevAcBG = (UINT16)-1;
    if (Edit_Mode == 1) Predraw_Frame_For_Rotations(0);
    return true;
}

#pragma endregion Project_File_Functions

#pragma region Txt_File_Operations
unsigned int Count_TXT_Frames(char* TXTF_buffer,size_t TXTF_buffer_len)
{
    // check the number of frames in the txt file
    unsigned int nF = 0;
    for (size_t curPos = 0; curPos < TXTF_buffer_len; curPos++)
    {
        if (TXTF_buffer[curPos] == 'x') nF++;
    }
    return nF;
}

bool Get_Frames_Ptr_Size_And_Number_Of_Colors(sFrames** ppFrames,UINT nFrames,char* TXTF_buffer,size_t TXTF_buffer_len)
{
    MycRom.noColors = 4;
    sFrames* pFrames = (sFrames*)malloc(sizeof(sFrames) * nFrames);
    *ppFrames = pFrames;
    //char tbuf[16];
    if (!pFrames)
    {
        cprintf(true, "Unable to get buffer memory for the frames");
        return false;
    }
    unsigned int acFr = 0;
    for (size_t curPos = 0; curPos < TXTF_buffer_len; curPos++)
    {
        if (TXTF_buffer[curPos] == 'x')
        {
            char tbuf[12];
            tbuf[0] = '0';
            for (UINT ti = 0; ti < 9; ti++) tbuf[ti + 1] = TXTF_buffer[curPos + ti];
            tbuf[10] = 0;
            pFrames[acFr].timecode = (UINT32)strtoul(tbuf, NULL, 16);
            // next line
            while ((TXTF_buffer[curPos] != '\n') && (TXTF_buffer[curPos] != '\r')) curPos++;
            while ((TXTF_buffer[curPos] == '\n') || (TXTF_buffer[curPos] == '\r') || (TXTF_buffer[curPos] == ' ')) curPos++;
            // we are at the beginning of the frame
            pFrames[acFr].active = TRUE;
            pFrames[acFr].ptr = &TXTF_buffer[curPos];
            if (acFr == 0)
            {
                size_t finPos = curPos;
                while (TXTF_buffer[finPos] > ' ') finPos++;
                MycRom.fWidth = (unsigned int)(finPos - curPos);
                while (TXTF_buffer[finPos + 1] <= ' ') finPos++;
                MycRom.fHeight = 1;
                while (TXTF_buffer[finPos + 1] > ' ')
                {
                    finPos += 2;
                    while (TXTF_buffer[finPos] != '\n') finPos++;
                    MycRom.fHeight++;
                }
                if (MycRom.fHeight == 64)
                {
                    MycRom.fHeightX = MycRom.fHeight/2;
                    MycRom.fWidthX = MycRom.fWidth / 2;
                }
                else
                {
                    MycRom.fHeightX = MycRom.fHeight * 2;
                    MycRom.fWidthX = MycRom.fWidth * 2;
                }
            }
            char* pPos = pFrames[acFr].ptr;
            char* pPos2 = pPos;
            for (UINT tj = 0; tj < MycRom.fHeight; tj++)
            {
                for (UINT ti = 0; ti < MycRom.fWidth; ti++)
                {
                    if ((*pPos >= (UINT8)'0') && (*pPos <= (UINT8)'9')) *pPos2 = *pPos - (UINT8)'0';
                    else if ((*pPos >= (UINT8)'A') && (*pPos <= (UINT8)'Z')) *pPos2 = *pPos - (UINT8)'A' + 10;
                    else if ((*pPos >= (UINT8)'a') && (*pPos <= (UINT8)'z')) *pPos2 = *pPos - (UINT8)'a' + 10;
                    if ((*pPos2) > 3) MycRom.noColors = 16;
                    pPos++;
                    pPos2++;
                }
                while (((*pPos) == '\n') || ((*pPos) == '\r')) pPos++;
            }
            curPos = pPos - TXTF_buffer;
            acFr++;
        }
    }
    //if (MycRom.fHeight == 64) acZoom = basezoom; else acZoom = 2 * basezoom;
    return true;
}

bool Parse_TXT(char* TXTF_name, char* TXTF_buffer, size_t TXTF_buffer_len, sFrames** ppFrames, UINT* pnFrames)
{
    if (!TXTF_buffer) return false;
    *pnFrames = Count_TXT_Frames(TXTF_buffer, TXTF_buffer_len);
    if (!Get_Frames_Ptr_Size_And_Number_Of_Colors(ppFrames, *pnFrames, TXTF_buffer, TXTF_buffer_len))
    {
        Free_Project();
        return false;
    }
    cprintf(false, "Opened txt file %s with %i frames: resolution %ix%i, %i colors", TXTF_name, *pnFrames,MycRom.fWidth,MycRom.fHeight,MycRom.noColors);
    return true;
}

void CompareFrames(UINT nFrames, sFrames* pFrames)
{
    UINT nfremoved = 0;
    UINT nfrremtime = 0, nfrremcol = 0, nfrremsame = 0;
    for (int ti = 0; ti < (int)nFrames; ti++)
    {
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(nFrames - 1), 0, 2);
        if (ti < (int)nFrames - 1)
        {
            UINT32 nextfrlen = pFrames[ti + 1].timecode;
            UINT32 acfrlen = pFrames[ti].timecode;
            if (nextfrlen < acfrlen) pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
            else if (nextfrlen > acfrlen + MAX_FRAME_DURATION) pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
            else
            {
                pFrames[ti].timecode = nextfrlen - acfrlen;
                if (filter_time && (pFrames[ti].timecode < (UINT32)filter_length) && pFrames[ti].active)
                {
                    nfrremtime++;
                    nfremoved++;
                    pFrames[ti].active = FALSE;
                    continue;
                }
            }
        }
        else pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
        UINT8 ncols;
        pFrames[ti].hashcode = crc32_fast_count((UINT8*)pFrames[ti].ptr, MycRom.fWidth * MycRom.fHeight, FALSE, &ncols);
        if (filter_color && (filter_ncolor < ncols) && pFrames[ti].active)
        {
            nfrremcol++;
            nfremoved++;
            pFrames[ti].active = FALSE;
            continue;
        }
    }
    if (nFrames < 2) return;
    for (int ti = 0; ti < (int)nFrames - 1; ti++)
    {
        if (!(ti%200)) Display_Avancement((float)ti / (float)(nFrames - 1), 1, 2);
        if (pFrames[ti].active == FALSE) continue;
        for (int tj = ti + 1; tj < (int)nFrames; tj++)
        {
            if (pFrames[tj].active == FALSE) continue;
            if (pFrames[ti].hashcode == pFrames[tj].hashcode)
            {
                nfrremsame++;
                nfremoved++;
                pFrames[tj].active = FALSE;
            }
        }
    }
    cprintf(false, "%i frames removed (%i for short duration, %i for too many colors, %i identical), %i added", nfremoved, nfrremtime, nfrremcol, nfrremsame, nFrames - nfremoved);
}

void CompareAdditionalFrames(UINT nFrames, sFrames* pFrames)
{
    //unsigned int nfkilled = 0;
    UINT nfremoved = 0;
    UINT nfrremtime = 0, nfrremcol = 0, nfrremmask = 0, nfrremsame = 0;
    for (int ti = 0; ti < (int)nFrames; ti++)
    {
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(nFrames - 1), 0, 4);
        if (ti < (int)nFrames - 1)
        {
            UINT32 nextfrlen = pFrames[ti + 1].timecode;
            UINT32 acfrlen = pFrames[ti].timecode;
            if (nextfrlen < acfrlen) pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
            else if (nextfrlen > acfrlen + MAX_FRAME_DURATION) pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
            else
            {
                pFrames[ti].timecode = nextfrlen - acfrlen;
                if (filter_time && (pFrames[ti].timecode < (UINT32)filter_length) && pFrames[ti].active)
                {
                    nfrremtime++;
                    nfremoved++;
                    pFrames[ti].active = FALSE;
                    continue;
                }
            }
        }
        else pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
        UINT8 ncols;
        pFrames[ti].hashcode = crc32_fast_count((UINT8*)pFrames[ti].ptr, MycRom.fWidth * MycRom.fHeight, FALSE, &ncols);
        if (filter_color && (filter_ncolor < ncols) && pFrames[ti].active)
        {
            nfrremcol++;
            nfremoved++;
            pFrames[ti].active = FALSE;
            continue;
        }
    }
    if (nFrames < 2) return;
    for (int ti = 0; ti < (int)nFrames - 1; ti++)
    {
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(nFrames - 1), 1, 4);
        if (pFrames[ti].active == FALSE) continue;
        for (int tj = ti + 1; tj < (int)nFrames; tj++)
        {
            if (pFrames[tj].active == FALSE) continue;
            if (pFrames[ti].hashcode == pFrames[tj].hashcode)
            {
                nfrremsame++;
                nfremoved++;
                pFrames[tj].active = FALSE;
            }
        }
    }
    UINT32* pnomaskhash = (UINT32*)malloc(sizeof(UINT32) * MycRom.nFrames);
    if (!pnomaskhash)
    {
        MessageBoxA(hWnd, "Impossible to get memory for flat hashes, incomplete comparison!", "Error", MB_OK);
        return;
    }
    for (int ti = 0; ti < (int)MycRom.nFrames; ti++)
    {
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(MycRom.nFrames - 1), 2, 4);
        if (MycRom.CompMaskID[ti] != 255)
        {
            MycRom.HashCode[ti] = crc32_fast_mask_shape(&MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * ti], &MycRom.CompMasks[MycRom.CompMaskID[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight, (BOOL)MycRom.ShapeCompMode[ti]);
            pnomaskhash[ti] = crc32_fast(&MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * ti], MycRom.fWidth * MycRom.fHeight, FALSE);
        }
        else
        {
            MycRom.HashCode[ti] = crc32_fast(&MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * ti], MycRom.fWidth * MycRom.fHeight, (BOOL)MycRom.ShapeCompMode[ti]);
            if (MycRom.ShapeCompMode[ti] == FALSE) pnomaskhash[ti] = MycRom.HashCode[ti];
            else pnomaskhash[ti] = crc32_fast(&MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * ti], MycRom.fWidth * MycRom.fHeight, FALSE);
        }
    }
    for (unsigned int ti = 0; ti < nFrames; ti++)
    {
        if (pFrames[ti].active == FALSE) continue;
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(nFrames - 1), 3, 4);
        UINT8 premask = 255;
        BOOL isshapemode = FALSE;
        UINT32 achash = crc32_fast((UINT8*)pFrames[ti].ptr, MycRom.fWidth * MycRom.fHeight, FALSE);
        for (int tj = 0; tj < (int)MycRom.nFrames; tj++)
        {
            if ((MycRom.CompMaskID[tj] != 255) && filter_allmask)
            {
                if ((premask != MycRom.CompMaskID[tj]) || (isshapemode != (BOOL)MycRom.ShapeCompMode[tj]))
                {
                    achash = crc32_fast_mask_shape((UINT8*)pFrames[ti].ptr, &MycRom.CompMasks[MycRom.CompMaskID[tj] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight, (BOOL)MycRom.ShapeCompMode[tj]);
                    premask = MycRom.CompMaskID[tj];
                    isshapemode = (BOOL)MycRom.ShapeCompMode[tj];
                }
                if (MycRom.HashCode[tj] == achash)
                {
                    pFrames[ti].active = FALSE;
                    nfrremmask++;
                    nfremoved++;
                    break;
                }
            }
            else if (filter_allmask && (MycRom.ShapeCompMode[tj] == TRUE))
            {
                if ((premask != 255) || (isshapemode == FALSE))
                {
                    achash = crc32_fast((UINT8*)pFrames[ti].ptr, MycRom.fWidth * MycRom.fHeight, TRUE);
                    premask = 255;
                    isshapemode = TRUE;
                }
                if (MycRom.HashCode[tj] == achash)
                {
                    pFrames[ti].active = FALSE;
                    nfrremmask++;
                    nfremoved++;
                    break;
                }
            }
            else
            {
                if (pnomaskhash[tj] == pFrames[ti].hashcode)
                {
                    pFrames[ti].active = FALSE;
                    nfrremsame++;
                    nfremoved++;
                    break;
                }
            }
        }
    }
    free(pnomaskhash);
    cprintf(false, "%i frames removed (%i for short duration, %i for too many colors, %i identical with mask and/or shapemode, %i identical) %i added", nfremoved, nfrremtime, nfrremcol, nfrremmask, nfrremsame, nFrames - nfremoved);
}

bool CopyTXTFrames2Frame(UINT nFrames, sFrames* pFrames)
{
    unsigned int nF = 0;
    for (unsigned int ti = 0; ti < nFrames; ti++)
    {
        if (pFrames[ti].active == TRUE) nF++;
    }
    MycRP.oFrames = (UINT8*)malloc(nF * sizeof(UINT8) * MycRom.fWidth * MycRom.fHeight);
    if (!MycRP.oFrames)
    {
        cprintf(true, "Unable to allocate memory for original frames");
        return false;
    }
    MycRom.HashCode = (UINT*)malloc(nF * sizeof(UINT));
    if (!MycRom.HashCode)
    {
        Free_cRom();
        cprintf(true, "Unable to allocate memory for hashtags");
        return false;
    }
    MycRom.ShapeCompMode = (UINT8*)malloc(nF);
    if (!MycRom.ShapeCompMode)
    {
        Free_cRom();
        cprintf(true, "Unable to allocate memory for shape mode");
        return false;
    }
    memset(MycRom.ShapeCompMode, FALSE, nF);
    MycRom.CompMaskID = (UINT8*)malloc(nF * sizeof(UINT8));
    if (!MycRom.CompMaskID)
    {
        Free_cRom();
        cprintf(true, "Unable to allocate memory for comparison mask IDs");
        return false;
    }
    memset(MycRom.CompMaskID, 255, nF * sizeof(UINT8));
    MycRom.isExtraFrame = (UINT8*)malloc(nF * sizeof(UINT8));
    if (!MycRom.isExtraFrame)
    {
        Free_cRom();
        cprintf(true, "Unable to allocate memory for isExtraFrame");
        return false;
    }
    memset(MycRom.isExtraFrame, 0, nF * sizeof(UINT8));
    MycRom.CompMasks = (UINT8*)malloc(MAX_MASKS * MycRom.fWidth * MycRom.fHeight);
    if (!MycRom.CompMasks)
    {
        cprintf(true, "Unable to allocate memory for comparison masks");
        return false;
    }
    memset(MycRom.CompMasks, 0, MAX_MASKS * MycRom.fWidth * MycRom.fHeight);
    MycRom.nCompMasks = 0;
    MycRom.nSprites = 0;
    MycRom.nBackgrounds= 0;
    //size_t sizepalette = MycRom.ncColors * 3;
    /*MycRom.cPal = (UINT8*)malloc(nF * sizepalette);
    if (!MycRom.cPal)
    {
        Free_cRom();
        cprintf(true, "Unable to allocate memory for colorized palettes");
        return false;
    }*/
    MycRom.cFrames = (UINT16*)malloc(nF * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    if (!MycRom.cFrames)
    {
        cprintf(true, "Unable to allocate memory for colorized frames");
        Free_cRom();
        return false;
    }
    MycRom.cFramesX = (UINT16*)malloc(nF * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    if (!MycRom.cFramesX)
    {
        cprintf(true, "Unable to allocate memory for extra colorized frames");
        Free_cRom();
        return false;
    }
    memset(MycRom.cFramesX, 0, nF * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    MycRom.DynaMasks = (UINT8*)malloc(nF * MycRom.fWidth * MycRom.fHeight);
    if (!MycRom.DynaMasks)
    {
        cprintf(true, "Unable to allocate memory for dynamic masks");
        Free_cRom();
        return false;
    }
    memset(MycRom.DynaMasks, 255, nF * MycRom.fWidth * MycRom.fHeight);
    MycRom.DynaMasksX = (UINT8*)malloc(nF * MycRom.fWidthX * MycRom.fHeightX);
    if (!MycRom.DynaMasksX)
    {
        cprintf(true, "Unable to allocate memory for extra dynamic masks");
        Free_cRom();
        return false;
    }
    memset(MycRom.DynaMasksX, 255, nF * MycRom.fWidthX * MycRom.fHeightX);
    MycRom.Dyna4Cols = (UINT16*)malloc(nF * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    if (!MycRom.Dyna4Cols)
    {
        cprintf(true, "Unable to allocate memory for dynamic color sets");
        Free_cRom();
        return false;
    }
    memset(MycRom.Dyna4Cols, 0, nF * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    MycRom.Dyna4ColsX = (UINT16*)malloc(nF * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    if (!MycRom.Dyna4ColsX)
    {
        cprintf(true, "Unable to allocate memory for extradynamic color sets");
        Free_cRom();
        return false;
    }
    memset(MycRom.Dyna4ColsX, 0, nF * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    MycRom.FrameSprites = (UINT8*)malloc(nF * MAX_SPRITES_PER_FRAME);
    if (!MycRom.FrameSprites)
    {
        cprintf(true, "Unable to allocate memory for sprite IDs");
        Free_cRom();
        return false;
    }
    memset(MycRom.FrameSprites, 255, nF * MAX_SPRITES_PER_FRAME);
    MycRom.FrameSpriteBB = (UINT16*)malloc(nF * MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16));
    if (!MycRom.FrameSpriteBB)
    {
        cprintf(true, "Unable to allocate memory for sprite bounding boxes");
        Free_cRom();
        return false;
    }
    for (UINT tj = 0; tj < nF; tj++)
    {
        for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
        {
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = 0;
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = 0;
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = MycRom.fWidth - 1;
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = MycRom.fHeight - 1;
        }
    }
    MycRom.TriggerID = (UINT32*)malloc(nF * sizeof(UINT32));
    if (!MycRom.TriggerID)
    {
        cprintf(true, "Unable to allocate memory for trigger IDs");
        Free_cRom();
        return false;
    }
    memset(MycRom.TriggerID, 0xFF, nF * sizeof(UINT32));
    MycRom.ColorRotations = (UINT16*)malloc(nF * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    if (!MycRom.ColorRotations)
    {
        cprintf(true, "Unable to allocate memory for color rotations");
        Free_cRom();
        return false;
    }
    memset(MycRom.ColorRotations, 0, nF * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    MycRom.ColorRotationsX = (UINT16*)malloc(nF * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    if (!MycRom.ColorRotationsX)
    {
        cprintf(true, "Unable to allocate memory for color rotations");
        Free_cRom();
        return false;
    }
    memset(MycRom.ColorRotationsX, 0, nF * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    MycRP.FrameDuration = (UINT32*)malloc(nF * sizeof(UINT32));
    if (!MycRP.FrameDuration)
    {
        cprintf(true, "Unable to allocate memory for frame duration");
        Free_cRom();
        return false;
    }
    MycRom.nBackgrounds = 0;
    MycRom.BackgroundID = (UINT16*)malloc(nF * sizeof(UINT16));
    if (!MycRom.BackgroundID)
    {
        cprintf(true, "Unable to allocate memory for background IDs");
        Free_cRom();
        return false;
    }
    memset(MycRom.BackgroundID, 0xff, sizeof(UINT16) * nF);
    MycRom.BackgroundMask = (UINT8*)malloc(MycRom.fWidth * MycRom.fHeight * nF);
    if (!MycRom.BackgroundMask)
    {
        cprintf(true, "Unable to allocate memory for background masks");
        Free_cRom();
        return false;
    }
    memset(MycRom.BackgroundMask, 1, MycRom.fWidth * MycRom.fHeight * nF);
    MycRom.BackgroundMaskX = (UINT8*)malloc(MycRom.fWidthX * MycRom.fHeightX * nF);
    if (!MycRom.BackgroundMaskX)
    {
        cprintf(true, "Unable to allocate memory for extra background masks");
        Free_cRom();
        return false;
    }
    memset(MycRom.BackgroundMaskX, 1, MycRom.fWidthX * MycRom.fHeightX * nF);
    MycRom.isExtraSprite = NULL;
    MycRom.isExtraBackground = NULL;
    MycRom.BackgroundFrames = NULL;
    MycRom.SpriteOriginal = NULL;
    MycRom.SpriteColored= NULL;
    MycRom.SpriteMaskX = NULL;
    MycRom.SpriteColoredX = NULL;
    MycRom.SpriteDetDwords = NULL;
    MycRom.SpriteDetDwordPos = NULL;
    MycRom.SpriteDetAreas = NULL;
    MycRP.nSections = 0;
    MycRom.nFrames = 0;
    for (unsigned int tk = 0; tk < nFrames; tk++)
    {
        if (pFrames[tk].active == TRUE)
        {
            MycRP.FrameDuration[MycRom.nFrames] = pFrames[tk].timecode;
            char* psFr = pFrames[tk].ptr;
            UINT8* pdoFr = &MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * MycRom.nFrames];
            UINT16* pdcFr = &MycRom.cFrames[MycRom.fWidth * MycRom.fHeight * MycRom.nFrames];
/*            if (tk < nFrames - 1)
            {
                UINT32 time1 = pFrames[tk].timecode;
                UINT32 time2 = pFrames[tk + 1].timecode;
                if (time2 < time1) MycRP.FrameDuration[MycRom.nFrames] = 0;
                else if (time2 - time1 > 30000) MycRP.FrameDuration[MycRom.nFrames] = 0;
                else MycRP.FrameDuration[MycRom.nFrames] = time2 - time1;
                if (filter_time && (filter_length > MycRP.FrameDuration[MycRom.nFrames])) pFrames[tk].active = FALSE;
            }
            else MycRP.FrameDuration[MycRom.nFrames] = 0;*/
            memset(MycRP.Palette, 0, sizeof(UINT16) * N_PALETTES * 64);
            memset(MycRP.PalNames, 0, N_PALETTES * 64);
            Init_cFrame_Palette2();
            for (unsigned int tj = 0; tj < MycRom.fHeight* MycRom.fWidth; tj++)
            {
                *pdoFr = (UINT8)(*psFr);
                *pdcFr = originalcolors[*pdoFr];
                pdoFr++;
                pdcFr++;
                psFr++;
            }
            MycRom.nFrames++;
        }
    }
    return true;
}

bool AddTXTFrames2Frame(UINT nFrames, sFrames* pFrames)
{
    unsigned int nF = 0;
    for (unsigned int ti = 0; ti < nFrames; ti++)
    {
        if (pFrames[ti].active == TRUE) nF++;
    }
    if (nF == 0) return true;
    MycRP.oFrames = (UINT8*)realloc(MycRP.oFrames, (nF + MycRom.nFrames) * sizeof(UINT8) * MycRom.fWidth * MycRom.fHeight);
    if (!MycRP.oFrames)
    {
        cprintf(true, "Unable to reallocate memory for original frames");
        return false;
    }
    MycRom.HashCode = (UINT*)realloc(MycRom.HashCode, (nF + MycRom.nFrames) * sizeof(UINT));
    if (!MycRom.HashCode)
    {
        Free_cRom();
        cprintf(true, "Unable to reallocate memory for hashcodes");
        return false;
    }
    MycRom.ShapeCompMode = (UINT8*)realloc(MycRom.ShapeCompMode, nF + MycRom.nFrames);
    if (!MycRom.ShapeCompMode)
    {
        Free_cRom();
        cprintf(true, "Unable to allocate memory for shape mode");
        return false;
    }
    memset(&MycRom.ShapeCompMode[MycRom.nFrames], FALSE, nF);
    MycRom.CompMaskID = (UINT8*)realloc(MycRom.CompMaskID, (nF + MycRom.nFrames) * sizeof(UINT8));
    if (!MycRom.CompMaskID)
    {
        Free_cRom();
        cprintf(true, "Unable to reallocate memory for comparison masks");
        return false;
    }
    memset(&MycRom.CompMaskID[MycRom.nFrames], 255, nF * sizeof(UINT8));
    MycRom.isExtraFrame = (UINT8*)realloc(MycRom.isExtraFrame, (nF + MycRom.nFrames) * sizeof(UINT8));
    if (!MycRom.isExtraFrame)
    {
        Free_cRom();
        cprintf(true, "Unable to reallocate memory for isExtraFrames");
        return false;
    }
    memset(&MycRom.isExtraFrame[MycRom.nFrames], 0, nF * sizeof(UINT8));
    //size_t sizepalette = MycRom.ncColors * 3;
    /*MycRom.cPal = (UINT8*)realloc(MycRom.cPal, (nF + MycRom.nFrames) * sizepalette);
    if (!MycRom.cPal)
    {
        Free_cRom();
        cprintf(true, "Unable to reallocate memory for colorized palettes");
        return false;
    }*/
    MycRom.cFrames = (UINT16*)realloc(MycRom.cFrames, (nF + MycRom.nFrames) * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    if (!MycRom.cFrames)
    {
        cprintf(true, "Unable to reallocate memory for colorized frames");
        Free_cRom();
        return false;
    }
    MycRom.cFramesX = (UINT16*)realloc(MycRom.cFramesX, (nF + MycRom.nFrames) * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    if (!MycRom.cFramesX)
    {
        cprintf(true, "Unable to reallocate memory for extra colorized frames");
        Free_cRom();
        return false;
    }
    memset(&MycRom.cFramesX[MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX], 0, nF * MycRom.fHeightX * sizeof(UINT16));
    MycRom.DynaMasks = (UINT8*)realloc(MycRom.DynaMasks, (nF + MycRom.nFrames) * MycRom.fWidth * MycRom.fHeight);
    if (!MycRom.DynaMasks)
    {
        cprintf(true, "Unable to reallocate memory for dynamic masks");
        Free_cRom();
        return false;
    }
    memset(&MycRom.DynaMasks[MycRom.nFrames * MycRom.fWidth * MycRom.fHeight], 255, nF * MycRom.fWidth * MycRom.fHeight);
    MycRom.DynaMasksX = (UINT8*)realloc(MycRom.DynaMasksX, (nF + MycRom.nFrames) * (size_t)MycRom.fWidthX * (size_t)MycRom.fHeightX);
    if (!MycRom.DynaMasks)
    {
        cprintf(true, "Unable to reallocate memory for extradynamic masks");
        Free_cRom();
        return false;
    }
    memset(&MycRom.DynaMasksX[MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX], 255, nF * MycRom.fWidthX * MycRom.fHeightX);
    MycRom.Dyna4Cols = (UINT16*)realloc(MycRom.Dyna4Cols, (nF + MycRom.nFrames) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    if (!MycRom.Dyna4Cols)
    {
        cprintf(true, "Unable to reallocate memory for dynamic color sets");
        Free_cRom();
        return false;
    }
    memset(&MycRom.Dyna4Cols[MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], 0, nF * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    MycRom.Dyna4ColsX = (UINT16*)realloc(MycRom.Dyna4ColsX, (nF + MycRom.nFrames) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    if (!MycRom.Dyna4ColsX)
    {
        cprintf(true, "Unable to reallocate memory for extra dynamic color sets");
        Free_cRom();
        return false;
    }
    memset(&MycRom.Dyna4ColsX[MycRom.nFrames * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], 0, nF * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
    MycRom.FrameSprites = (UINT8*)realloc(MycRom.FrameSprites, (nF + MycRom.nFrames) * MAX_SPRITES_PER_FRAME);
    if (!MycRom.FrameSprites)
    {
        cprintf(true, "Unable to reallocate memory for sprite IDs");
        Free_cRom();
        return false;
    }
    memset(&MycRom.FrameSprites[MycRom.nFrames * MAX_SPRITES_PER_FRAME], 255, nF * MAX_SPRITES_PER_FRAME);
    MycRom.FrameSpriteBB = (UINT16*)realloc(MycRom.FrameSpriteBB, (nF + MycRom.nFrames) * MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16));
    if (!MycRom.FrameSpriteBB)
    {
        cprintf(true, "Unable to allocate memory for sprite bounding boxes");
        Free_cRom();
        return false;
    }
    for (UINT tj = MycRom.nFrames; tj < MycRom.nFrames + nF; tj++)
    {
        for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
        {
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = 0;
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = 0;
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = MycRom.fWidth - 1;
            MycRom.FrameSpriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = MycRom.fHeight - 1;
        }
    }
    MycRom.TriggerID = (UINT32*)realloc(MycRom.TriggerID, (nF + MycRom.nFrames) * sizeof(UINT32));
    if (!MycRom.TriggerID)
    {
        cprintf(true, "Unable to reallocate memory for trigger IDs");
        Free_cRom();
        return false;
    }
    memset(&MycRom.TriggerID[MycRom.nFrames], 0xff, nF * sizeof(UINT32));
    MycRom.ColorRotations = (UINT16*)realloc(MycRom.ColorRotations, (nF + MycRom.nFrames) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    if (!MycRom.ColorRotations)
    {
        cprintf(true, "Unable to reallocate memory for color rotations");
        Free_cRom();
        return false;
    }
    memset(&MycRom.ColorRotations[MycRom.nFrames * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], 0, nF * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    MycRom.ColorRotationsX = (UINT16*)realloc(MycRom.ColorRotationsX, (nF + MycRom.nFrames) * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    if (!MycRom.ColorRotationsX)
    {
        cprintf(true, "Unable to reallocate memory for color rotations");
        Free_cRom();
        return false;
    }
    memset(&MycRom.ColorRotationsX[MycRom.nFrames * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], 0, nF * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
    MycRP.FrameDuration = (UINT32*)realloc(MycRP.FrameDuration, (nF + MycRom.nFrames) * sizeof(UINT32));
    if (!MycRP.FrameDuration)
    {
        cprintf(true, "Unable to reallocate memory for frame duration");
        Free_cRom();
        return false;
    }
    MycRom.BackgroundID = (UINT16*)realloc(MycRom.BackgroundID, (nF + MycRom.nFrames) * sizeof(UINT16));
    if (!MycRom.BackgroundID)
    {
        cprintf(true, "Unable to allocate memory for background IDs");
        Free_cRom();
        return false;
    }
    memset(&MycRom.BackgroundID[MycRom.nFrames], 0xff, sizeof(UINT16) * nF);
    MycRom.BackgroundMask = (UINT8*)realloc(MycRom.BackgroundMask, MycRom.fWidth * MycRom.fHeight * (nF + MycRom.nFrames));
    if (!MycRom.BackgroundMask)
    {
        cprintf(true, "Unable to allocate memory for background bounding boxes");
        Free_cRom();
        return false;
    }
    memset(&MycRom.BackgroundMask[MycRom.nFrames * MycRom.fWidth * MycRom.fHeight], 1, MycRom.fWidth * MycRom.fHeight * nF);
    MycRom.BackgroundMaskX = (UINT8*)realloc(MycRom.BackgroundMaskX, MycRom.fWidthX * MycRom.fHeightX * (nF + MycRom.nFrames));
    if (!MycRom.BackgroundMaskX)
    {
        cprintf(true, "Unable to allocate memory for background bounding boxes");
        Free_cRom();
        return false;
    }
    memset(&MycRom.BackgroundMaskX[MycRom.nFrames * MycRom.fWidthX * MycRom.fHeightX], 1, MycRom.fWidthX * MycRom.fHeightX * nF);
    for (unsigned int tk = 0; tk < nFrames; tk++)
    {
        if (pFrames[tk].active == TRUE)
        {
            MycRP.FrameDuration[MycRom.nFrames]= pFrames[tk].timecode;
            UINT8* psFr = (UINT8*)pFrames[tk].ptr;
            UINT8* pdoFr = &MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * MycRom.nFrames];
            UINT16* pdcFr = &MycRom.cFrames[MycRom.fWidth * MycRom.fHeight * MycRom.nFrames];
/*            if (tk < nFrames - 1)
            {
                UINT32 time1 = pFrames[tk].timecode;
                UINT32 time2 = pFrames[tk + 1].timecode;
                if (time2 < time1) MycRP.FrameDuration[MycRom.nFrames] = 0;
                else if (time2 - time1 > 30000) MycRP.FrameDuration[MycRom.nFrames] = 0;
                else MycRP.FrameDuration[MycRom.nFrames] = time2 - time1;
            }
            else MycRP.FrameDuration[MycRom.nFrames] = 0;*/
            Init_cFrame_Palette2();
            for (unsigned int tj = 0; tj < MycRom.fHeight * MycRom.fWidth; tj++)
            {
                *pdoFr = (UINT8)(*psFr);
                *pdcFr = originalcolors[*pdoFr];
                pdoFr++;
                pdcFr++;
                psFr++;
            }
            MycRom.nFrames++;
        }
    }
    return true;
}

void Load_TXT_File(void)
{
    char acDir[260];
    GetCurrentDirectoryA(260, acDir);

    char* TXTF_buffer = NULL;
    size_t TXTF_buffer_len = 0;
    unsigned int nFrames = 0;
    sFrames* pFrames = NULL;

    OPENFILENAMEA ofn;
    char szFile[260];
    //LoadPaths();
    strcpy_s(szFile, 260, Dir_Dumps);
    strcat_s(szFile, 260, "*.txt");

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrTitle = "Choose the initial TXT file";
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text (.txt)\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = Dir_Dumps;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        strcpy_s(Dir_Dumps, 260, ofn.lpstrFile);
        int i = (int)strlen(Dir_Dumps) - 1;
        while ((i > 0) && (Dir_Dumps[i] != '\\')) i--;
        Dir_Dumps[i + 1] = 0;
        NewProj = true;
        SavePaths();
        if (isLoadedProject)
        {
            if (MessageBox(hWnd, L"Confirm you want to close the current project and load a new one", L"Caution", MB_YESNO) == IDYES)
            {
                Free_Project();
            }
            else
            {
                SetCurrentDirectoryA(acDir);
                return;
            }
        }
        FILE* pfile;
        if (fopen_s(&pfile, ofn.lpstrFile, "rb") != 0)
        {
            cprintf(true, "Unable to open the file %s", ofn.lpstrFile);
            SetCurrentDirectoryA(acDir);
            return;
        }
        if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_FILTERS), hWnd, Filter_Proc, (LPARAM)1) == -1)
        {
            fclose(pfile);
            return;
        }
        fseek(pfile, 0, SEEK_END);
        TXTF_buffer_len = (size_t)ftell(pfile);
        rewind(pfile);
        TXTF_buffer = (char*)malloc(TXTF_buffer_len + 1);
        if (!TXTF_buffer)
        {
            TXTF_buffer_len = 0;
            cprintf(true, "Unable to get the memory buffer for the TXT file");
            fclose(pfile);
            SetCurrentDirectoryA(acDir);
            return;
        }
        size_t nread = fread_s(TXTF_buffer, TXTF_buffer_len + 1, 1, TXTF_buffer_len, pfile);
        TXTF_buffer[TXTF_buffer_len] = 0;
        fclose(pfile);
        if (!Parse_TXT(ofn.lpstrFile,TXTF_buffer, TXTF_buffer_len, &pFrames, &nFrames))
        {
            free(TXTF_buffer);
            SetCurrentDirectoryA(acDir);
            return;
        }
        char tpath[MAX_PATH];
        strcpy_s(tpath, MAX_PATH, ofn.lpstrFile);
        unsigned int ti = 0;
        size_t tj = strlen(tpath) - 1;
        while ((tpath[tj] != '\\') && (tpath[tj] != ':') && (tj > 0)) tj--;
        if (tj > 0)
        {
            tj++;
            while (tpath[tj] != '.')
            {
                MycRom.name[ti] = tpath[tj];
                ti++;
                tj++;
            }
        }
        MycRom.name[ti] = 0;
        strcpy_s(MycRP.name, 64, MycRom.name);
        CompareFrames(nFrames, pFrames);
        CopyTXTFrames2Frame(nFrames, pFrames);
        free(TXTF_buffer);
        free(pFrames);
        image_zoom_srce = false;
        image_zoom_dest = false;
        acBG = acSprite = acFrame = 0;
        CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
        CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
        prevAcBG = (UINT16)-1;
        prevAcFrame = (UINT)-1;
        if (Edit_Mode == 1) Predraw_Frame_For_Rotations(0);
        InitColorRotation();
        nSelFrames = 1;
        SetMultiWarningF();
        SelFrames[0] = 0;
        PreFrameInStrip = 0;
        nSelSprites = 1;
        SetMultiWarningS();
        SelSprites[0] = 0;
        PreSpriteInStrip = 0;
        PreBGInStrip = 0;
        isLoadedProject = true;
        BlocPause = true;
        Ident_Pushed = false;
        BBIdent_Pushed = false;
        Common_Pushed = false;
        Update_Toolbar = true;
        MycRP.isImported = 0;
        MycRP.importedPal = NULL;
    }
    SetCurrentDirectoryA(acDir);
}

LRESULT CALLBACK Filter_Proc(HWND hwDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        SendMessage(GetDlgItem(hwDlg, IDC_DELTIME), BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(GetDlgItem(hwDlg, IDC_DELMASK), BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(GetDlgItem(hwDlg, IDC_DELCOL), BM_SETCHECK, BST_UNCHECKED, 0);
        EnableWindow(GetDlgItem(hwDlg, IDC_TIMELEN), FALSE);
        EnableWindow(GetDlgItem(hwDlg, IDC_NCOL), FALSE);
        if (lParam==0) EnableWindow(GetDlgItem(hwDlg, IDC_DELMASK), TRUE); else EnableWindow(GetDlgItem(hwDlg, IDC_DELMASK), FALSE);
    }
    case WM_COMMAND:
    {
        switch (wParam)
        {
        case IDC_DELTIME:
        {
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELTIME), BM_GETCHECK, 0, 0) == BST_UNCHECKED) EnableWindow(GetDlgItem(hwDlg, IDC_TIMELEN), FALSE);
            else EnableWindow(GetDlgItem(hwDlg, IDC_TIMELEN), TRUE);
            return TRUE;
        }
        case IDC_DELCOL:
        {
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELCOL), BM_GETCHECK, 0, 0) == BST_UNCHECKED) EnableWindow(GetDlgItem(hwDlg, IDC_NCOL), FALSE);
            else EnableWindow(GetDlgItem(hwDlg, IDC_NCOL), TRUE);
            return TRUE;
        }
        case IDOK:
        {
            bool istimemod = false;
            bool iscolmod = false;
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELTIME), BM_GETCHECK, 0, 0) == BST_CHECKED)
            {
                char tbuf[256];
                filter_time = true;
                GetWindowTextA(GetDlgItem(hwDlg, IDC_TIMELEN), tbuf, 256);
                filter_length = atoi(tbuf);
                if (filter_length < 5)
                {
                    filter_length = 5;
                    _itoa_s(filter_length, tbuf, 256, 10);
                    SetWindowTextA(GetDlgItem(hwDlg, IDC_TIMELEN), tbuf);
                    istimemod = true;
                }
                else if (filter_length > 3000)
                {
                    filter_length = 3000;
                    _itoa_s(filter_length, tbuf, 256, 10);
                    SetWindowTextA(GetDlgItem(hwDlg, IDC_TIMELEN), tbuf);
                    istimemod = true;
                }
            }
            else filter_time = false;
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELCOL), BM_GETCHECK, 0, 0) == BST_CHECKED)
            {
                char tbuf[256];
                filter_color = true;
                GetWindowTextA(GetDlgItem(hwDlg, IDC_NCOL), tbuf, 256);
                filter_ncolor = atoi(tbuf);
                if (filter_ncolor < 1)
                {
                    filter_ncolor = 1;
                    _itoa_s(filter_ncolor, tbuf, 256, 10);
                    SetWindowTextA(GetDlgItem(hwDlg, IDC_NCOL), tbuf);
                    iscolmod = true;
                }
                else if (filter_ncolor > 16)
                {
                    filter_ncolor = 16;
                    _itoa_s(filter_ncolor, tbuf, 256, 10);
                    SetWindowTextA(GetDlgItem(hwDlg, IDC_NCOL), tbuf);
                    iscolmod = true;
                }
            }
            else filter_color = false;
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELMASK),BM_GETCHECK,0,0) == BST_CHECKED) filter_allmask = true; else filter_allmask = false;
            if (istimemod && iscolmod)
            {
                MessageBoxA(hWnd, "The time length and number of colors values have been automatically changed, check them and confirm again", "Confirm", MB_OK);
                return TRUE;
            }
            else if (istimemod)
            {
                MessageBoxA(hWnd, "The time length value has been automatically changed, check it and confirm again", "Confirm", MB_OK);
                return TRUE;
            }
            else if (iscolmod)
            {
                MessageBoxA(hWnd, "The number of colors value has been automatically changed, check it and confirm again", "Confirm", MB_OK);
                return TRUE;
            }
            EndDialog(hwDlg, 0);
            return TRUE;
        }
        case IDCANCEL:
        {
            EndDialog(hwDlg, -1);
            return TRUE;
        }
        }
    }
    }
    return FALSE;
}

void Add_TXT_File(void)
{
    char acDir[260];
    GetCurrentDirectoryA(260, acDir);

    char* TXTF_buffer = NULL;
    size_t TXTF_buffer_len = 0;
    unsigned int nFrames = 0;
    sFrames* pFrames = NULL; 

    OPENFILENAMEA ofn;
    char szFile[260];
    strcpy_s(szFile, 260, Dir_Dumps);
    strcat_s(szFile, 260, "*.txt");

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lpstrTitle = "Choose the additional TXT file";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text (.txt)\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = Dir_Dumps;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        strcpy_s(Dir_Dumps, MAX_PATH, ofn.lpstrFile);
        int i = (int)strlen(Dir_Dumps) - 1;
        while ((i > 0) && (Dir_Dumps[i] != '\\')) i--;
        Dir_Dumps[i + 1] = 0;
        SavePaths();
        FILE* pfile;
        if (fopen_s(&pfile, ofn.lpstrFile, "rb") != 0)
        {
            cprintf(true, "Unable to open the file %s", ofn.lpstrFile);
            SetCurrentDirectoryA(acDir);
            return;
        }
        if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_FILTERS), hWnd, Filter_Proc,(LPARAM)0) == -1)
        {
            fclose(pfile);
            return;
        }
        fseek(pfile, 0, SEEK_END);
        TXTF_buffer_len = (size_t)ftell(pfile);
        rewind(pfile);
        TXTF_buffer = (char*)malloc(TXTF_buffer_len + 1);
        if (!TXTF_buffer)
        {
            TXTF_buffer_len = 0;
            cprintf(true, "Unable to get the memory buffer for the TXT file");
            fclose(pfile);
            SetCurrentDirectoryA(acDir);
            return;
        }
        size_t nread = fread_s(TXTF_buffer, TXTF_buffer_len + 1, 1, TXTF_buffer_len, pfile);
        TXTF_buffer[TXTF_buffer_len] = 0;
        fclose(pfile);
        if (!Parse_TXT(ofn.lpstrFile,TXTF_buffer, TXTF_buffer_len, &pFrames, &nFrames))
        {
            free(TXTF_buffer);
            SetCurrentDirectoryA(acDir);
            return;
        }
        CompareAdditionalFrames(nFrames, pFrames);
        AddTXTFrames2Frame(nFrames, pFrames);
        free(TXTF_buffer);
        free(pFrames);
    }
    SetCurrentDirectoryA(acDir);
}

#pragma endregion Txt_File_Operations

#pragma region Window_Procs

LRESULT CALLBACK Wait_Proc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
        return TRUE;
    }

    return FALSE;
}
void SortSections(void)
{
    unsigned short sortlist[MAX_SECTIONS];
    for (UINT ti = 0; ti < MycRP.nSections; ti++) sortlist[ti] = ti;
    for (UINT ti = 0; ti < MycRP.nSections; ti++)
    {
        for (UINT tj = ti + 1; tj < MycRP.nSections; tj++)
            if (_stricmp(&MycRP.Section_Names[sortlist[tj] * SIZE_SECTION_NAMES], &MycRP.Section_Names[sortlist[ti] * SIZE_SECTION_NAMES]) < 0)  //Edit.
            {
                unsigned short tamp = sortlist[tj];
                sortlist[tj] = sortlist[ti];
                sortlist[ti] = tamp;
            }
    }
    UINT32		Section_Firsts[MAX_SECTIONS];
    char		Section_Names[MAX_SECTIONS * SIZE_SECTION_NAMES];
    for (UINT ti = 0; ti < MycRP.nSections; ti++)
    {
        Section_Firsts[ti] = MycRP.Section_Firsts[sortlist[ti]];
        strcpy_s(&Section_Names[ti * SIZE_SECTION_NAMES], SIZE_SECTION_NAMES, &MycRP.Section_Names[sortlist[ti] * SIZE_SECTION_NAMES]);
    }
    memcpy(MycRP.Section_Firsts, Section_Firsts, sizeof(UINT32) * MAX_SECTIONS);
    memcpy(MycRP.Section_Names, Section_Names, MAX_SECTIONS * SIZE_SECTION_NAMES);
}

void MoveSection(int nosec, int decalage)
{
    if (nosec == LB_ERR) return;
/*	UINT32		Section_Firsts[MAX_SECTIONS]; // first frame of each section
	char		Section_Names[MAX_SECTIONS * SIZE_SECTION_NAMES]; // Names of the sections
*/
    UINT32 sfirst;
    char sName[SIZE_SECTION_NAMES];
    sfirst = MycRP.Section_Firsts[nosec + decalage];
    MycRP.Section_Firsts[nosec + decalage] = MycRP.Section_Firsts[nosec];
    MycRP.Section_Firsts[nosec] = sfirst;
    strcpy_s(sName, SIZE_SECTION_NAMES, &MycRP.Section_Names[(nosec + decalage) * SIZE_SECTION_NAMES]);
    strcpy_s(&MycRP.Section_Names[(nosec + decalage) * SIZE_SECTION_NAMES], SIZE_SECTION_NAMES, &MycRP.Section_Names[nosec * SIZE_SECTION_NAMES]);
    strcpy_s(&MycRP.Section_Names[nosec * SIZE_SECTION_NAMES], SIZE_SECTION_NAMES, sName);
}

HWND hListBox, hwndButtonU, hwndButtonD, hwndButtonAlpha;
LRESULT CALLBACK MovSecProc(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        hListBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,WS_CHILD | WS_VISIBLE | WS_VSCROLL,25, 25, 200, 550, hWin, NULL, hInst, NULL);
        if (!hListBox)
        {
            AffLastError((char*)"Create ListBox Window");
            return FALSE;
        }
        ShowWindow(hListBox, TRUE);
        hwndButtonU = CreateWindow(L"BUTTON", L"UP", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 230, 205, 50, 50, hWin, NULL, hInst, NULL);      // Pointer not needed.
        if (!hwndButtonU)
        {
            AffLastError((char*)"Create Button Up Window");
            return FALSE;
        }
        ShowWindow(hwndButtonU, TRUE);
        hwndButtonAlpha = CreateWindow(L"BUTTON", L"ALPHA", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 230, 275, 50, 50, hWin, NULL, hInst, NULL);      // Pointer not needed.
        if (!hwndButtonAlpha)
        {
            AffLastError((char*)"Create Button Alpha Window");
            return FALSE;
        }
        ShowWindow(hwndButtonU, TRUE);
        hwndButtonD = CreateWindow(L"BUTTON", L"DOWN", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 230, 345, 50, 50, hWin, NULL, hInst, NULL);      // Pointer not needed.
        if (!hwndButtonD)
        {
            AffLastError((char*)"Create Button Down Window");
            return FALSE;
        }
        ShowWindow(hwndButtonD, TRUE);
        for (UINT ti = 0; ti < MycRP.nSections; ti++)
        {
            SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)&MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
        }
        return TRUE;
    }
    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT me{};
        me.cbSize = sizeof(TRACKMOUSEEVENT);
        me.dwFlags = TME_LEAVE;
        me.hwndTrack = hMovSec;
        me.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&me);
    }
    case WM_MOUSELEAVE:
    {
        RECT rc;
        POINT pt;
        GetClientRect(hWin, &rc);
        GetCursorPos(&pt);
        ScreenToClient(hWin, &pt);
        if ((pt.x < 0) || (pt.x >= rc.right) || (pt.y < 0) || (pt.y >= rc.bottom))
        {
            UpdateSectionList();
            UpdateFSneeded = true;
            DestroyWindow(hMovSec);
            hMovSec = NULL;
        }
        return TRUE;
    }
    case WM_COMMAND:
    {
        int acpos=(int)SendMessage(hListBox,LB_GETCURSEL,0,0);
        if (((HWND)lParam == hwndButtonU) && (acpos > 0))
        {
            if (acpos == LB_ERR) return TRUE;
            SaveAction(true, SA_SECTIONS);
            MoveSection(acpos, -1);
            SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
            for (UINT ti = 0; ti < MycRP.nSections; ti++)
            {
                SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)&MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
            }
            SendMessage(hListBox, LB_SETCURSEL, acpos - 1, 0);
        }
        else if (((HWND)lParam == hwndButtonD) && (acpos < (int)MycRP.nSections - 1))
        {
            if (acpos == LB_ERR) return TRUE;
            SaveAction(true, SA_SECTIONS);
            MoveSection(acpos, +1);
            SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
            for (UINT ti = 0; ti < MycRP.nSections; ti++)
            {
                SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)&MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
                SendMessage(hListBox, LB_SETCURSEL, acpos + 1, 0);
            }   
        }
        else if ((HWND)lParam == hwndButtonAlpha)
        {
            if (MessageBoxA(hWnd, "Do you really want to reorder section according the names?", "Confirm?", MB_YESNO) == IDYES)
            {
                SaveAction(true, SA_SECTIONS);
                SortSections();
                SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
                for (UINT ti = 0; ti < MycRP.nSections; ti++)
                {
                    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)&MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
                    SendMessage(hListBox, LB_SETCURSEL, acpos + 1, 0);
                }
            }
            UpdateSectionList();
            UpdateFSneeded = true;
            DestroyWindow(hMovSec);
            hMovSec = NULL;
        }
        return TRUE;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}
void UpdatePalProcList(HWND hComboBox,HWND hEdit)
{
    SendMessageA(hComboBox, CB_RESETCONTENT, 0, 0);
    for (int ti = 0; ti < N_PALETTES; ti++)
    {
        char tbuf[128];
        sprintf_s(tbuf, 128, "%i - %s", ti, &MycRP.PalNames[64 * ti]);
        SendMessageA(hComboBox, CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    SendMessage(hComboBox, CB_SETCURSEL, acPalette, 0);
    SetWindowTextA(hEdit, &MycRP.PalNames[64 * acPalette]);
}
static HWND hPalProcComboBox = NULL;
static HWND hPalProcEdit = NULL;
static HWND hPalProcButton = NULL;
LRESULT CALLBACK PalProc(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        hPalProcComboBox = CreateWindowEx(0, WC_COMBOBOX, TEXT("Combo Box"), WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST, 20, 20, 160, 300, hWin, NULL, GetModuleHandle(NULL), NULL);
        hPalProcEdit = CreateWindowEx(0, WC_EDIT, TEXT(""), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER, 20, 50, 160, 20, hWin, NULL, GetModuleHandle(NULL), NULL);
        hPalProcButton = CreateWindowEx(0, WC_BUTTON, TEXT("Set"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 70, 160, 20, hWin, NULL, GetModuleHandle(NULL), NULL);
        ShowWindow(hPalProcComboBox, SW_SHOWNORMAL);
        ShowWindow(hPalProcEdit, SW_SHOWNORMAL);
        ShowWindow(hPalProcButton, SW_SHOWNORMAL);
        UpdatePalProcList(hPalProcComboBox, hPalProcEdit);
        InvalidateRect(hWin, NULL, TRUE);
        Start_Gradient = false;
        Start_Gradient2 = false;
        Start_Imported_Col_Exchange = 0;
        Start_Col_Exchange = false;
        return TRUE;
    }
    case WM_COMMAND:
    {
        if ((HWND)lParam == hPalProcComboBox)
        {
            acPalette = (UINT)SendMessage(hPalProcComboBox, CB_GETCURSEL, 0, 0);
            InvalidateRect(hWin, NULL, TRUE);
        }
        else if ((HWND)lParam == hPalProcButton)
        {
            char tbuf[128];
            GetWindowTextA(hPalProcEdit, tbuf, 64);
            tbuf[63] = 0;
            strcpy_s(&MycRP.PalNames[acPalette * 64], 64, tbuf);
            UpdatePalProcList(hPalProcComboBox, hPalProcEdit);
            InvalidateRect(hWin, NULL, TRUE);
        }
    }
    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT me{};
        me.cbSize = sizeof(TRACKMOUSEEVENT);
        me.dwFlags = TME_HOVER | TME_LEAVE;
        me.hwndTrack = hWin;
        me.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&me);
        if (hWin == hPal2) break;
        if (hWin == hPal)
        {
            if (Start_Gradient)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWin, &pt);
                if ((pt.x < MARGIN_PALETTE_X) || (pt.x >= 160 + MARGIN_PALETTE_X) || (pt.y < MARGIN_PALETTE_Y) || (pt.y >= 160 + MARGIN_PALETTE_Y)) break;
                Fin_Gradient_Color = (UINT)(acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20));
                if (Fin_Gradient_Color < MycRom.noColors) Fin_Gradient_Color = MycRom.noColors;
            }
            else if (Start_Gradient2)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWin, &pt);
                if ((pt.x < MARGIN_PALETTE_X) || (pt.x >= 160 + MARGIN_PALETTE_X) || (pt.y < MARGIN_PALETTE_Y) || (pt.y >= 160 + MARGIN_PALETTE_Y)) break;
                Draw_Grad_Fin = (UINT)(acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20));
            }
            else if (Start_Imported_Col_Exchange == 1)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWin, &pt);
                if ((pt.x >= MARGIN_PALETTE_X) && (pt.x < 160 + MARGIN_PALETTE_X) && (pt.y >= MARGIN_PALETTE_Y + MARGIN_PALETTE_X + 160) && (pt.y <= 160 + MARGIN_PALETTE_Y + MARGIN_PALETTE_X + 160))
                {
                    Fin_Import_Color = ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y - MARGIN_PALETTE_X - 160) / 20);
                }
            }
        }
        else
        {
            if (Start_Gradient)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWin, &pt);
                if ((pt.x < MARGIN_PALETTE_X) || (pt.x >= 160 + MARGIN_PALETTE_X) || (pt.y < MARGIN_PALETTE_Y) || (pt.y >= 160 + MARGIN_PALETTE_Y)) break;
                Fin_Gradient_Color = (UINT)(acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20));
                
            }
        }
        break;
    }
    case WM_MOUSELEAVE:
    {
        POINT pt;
        RECT rcDialog;
        GetCursorPos(&pt);
        GetWindowRect(hWin, &rcDialog);
        if (PtInRect(&rcDialog, pt)) break;
        // then we leave
        Start_Gradient = false;
        Start_Gradient2 = false;
        Start_Col_Exchange = false;
        Start_Imported_Col_Exchange = 0;
        DestroyWindow(hWin);
        if (hWin == hPal) InvalidateRect(GetDlgItem(hwTB, IDC_GRADMODEB), NULL, FALSE);
        if (hWin == hPal3)
        {
            InvalidateRect(GetDlgItem(hColSet, IDC_COLORS), NULL, TRUE);
            InvalidateRect(hwTB, NULL, TRUE);
        }
        hPal = hPal2 = hPal3 = NULL;
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = GetDC(hWin);
        RECT rc,rci;
        HBRUSH hbr;
        BeginPaint(hWin, &ps);
        UINT16 col = MycRP.acEditColorsS[noColSel];
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(mselcol, mselcol, mselcol));
        for (UINT tj = 0; tj < 8; tj++)
        {
            for (UINT ti = 0; ti < 8; ti++)
            {
                rci.left = rc.left = MARGIN_PALETTE_X + 20 * ti;
                rci.right = rc.right = MARGIN_PALETTE_X + 20 * (ti + 1) - 1;
                rc.top = MARGIN_PALETTE_Y + 20 * tj;
                rci.top = rc.top + MARGIN_PALETTE_X + 160;
                rc.bottom = MARGIN_PALETTE_Y + 20 * (tj + 1) - 1;
                rci.bottom = rc.bottom + MARGIN_PALETTE_X + 160;

                hbr = CreateSolidBrush(RGB565_to_RGB888(MycRP.Palette[acPalette * 64 + tj * 8 + ti]));
                FillRect(hdc, &rc, hbr);
                if (col == MycRP.Palette[acPalette * 64 + tj * 8 + ti])
                {
                    DeleteObject(hbr);
                    SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    SelectObject(hdc, hPen);
                    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
                }
                if (MycRP.isImported > 0)
                {
                    DeleteObject(hbr);
                    Import_Color_Frame = min((int)MycRP.isImported - 1, (int)acFrame);
                    hbr = CreateSolidBrush(RGB(MycRP.importedPal[(Import_Color_Frame * 64 + tj * 8 + ti) * 3], MycRP.importedPal[(Import_Color_Frame * 64 + tj * 8 + ti) * 3 + 1], MycRP.importedPal[(Import_Color_Frame * 64 + tj * 8 + ti) * 3 + 2]));
                    FillRect(hdc, &rci, hbr);
                    if (Start_Imported_Col_Exchange >= 1)
                    {
                        if ((tj * 8 + ti >= min(Ini_Import_Color, Fin_Import_Color)) && (tj * 8 + ti <= max(Ini_Import_Color, Fin_Import_Color)))
                        {
                            DeleteObject(hbr);
                            hbr = CreateSolidBrush(RGB(0, SelFrameColor, SelFrameColor));
                            FrameRect(hdc, &rci, hbr);
                        }
                    }
                }
                if (Start_Gradient)
                {
                    if ((acPalette * 64 + tj * 8 + ti >= min(Ini_Gradient_Color, Fin_Gradient_Color)) && (acPalette * 64 + tj * 8 + ti <= max(Ini_Gradient_Color, Fin_Gradient_Color)))
                    {
                        DeleteObject(hbr);
                        hbr = CreateSolidBrush(RGB(0, SelFrameColor, SelFrameColor));
                        FrameRect(hdc, &rc, hbr);
                    }
                }
                else if (Start_Gradient2)
                {
                    if ((acPalette * 64 + tj * 8 + ti >= min(Draw_Grad_Ini, Draw_Grad_Fin)) && (acPalette * 64 + tj * 8 + ti <= max(Draw_Grad_Ini, Draw_Grad_Fin)))
                    {
                        DeleteObject(hbr);
                        hbr = CreateSolidBrush(RGB(SelFrameColor, 0, SelFrameColor));
                        FrameRect(hdc, &rc, hbr);
                    }
                }
                else if (Start_Col_Exchange)
                {
                    if (acPalette * 64 + tj * 8 + ti == Pre_Col_Pos)
                    {
                        DeleteObject(hbr);
                        hbr = CreateSolidBrush(RGB(SelFrameColor, 0, SelFrameColor));
                        FrameRect(hdc, &rc, hbr);
                    }
                }
                UINT quelfr = acFrame;
                if (hWin == hPal2) quelfr = MycRP.Sprite_Col_From_Frame[acSprite];
                DeleteObject(hbr);
            }
        }
        DeleteObject(hPen);
        EndPaint(hWin, &ps);
        ReleaseDC(hWin, hdc);
        break;
    }
    case WM_LBUTTONDOWN:
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWin, &pt);
        if (MycRP.isImported > 0 && Start_Imported_Col_Exchange == 0 && (pt.x >= MARGIN_PALETTE_X) && (pt.x < 160 + MARGIN_PALETTE_X) && (pt.y >= MARGIN_PALETTE_Y + MARGIN_PALETTE_X + 160) && (pt.y <= 160 + MARGIN_PALETTE_Y + MARGIN_PALETTE_X + 160))
        {
            Start_Imported_Col_Exchange = 1;
            Ini_Import_Color = Fin_Import_Color = ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y - MARGIN_PALETTE_X - 160) / 20);
            break;
        }
        if ((pt.x < MARGIN_PALETTE_X) || (pt.x >= 160 + MARGIN_PALETTE_X) || (pt.y < MARGIN_PALETTE_Y) || (pt.y >= 160 + MARGIN_PALETTE_Y)) break;
        if (hWin == hPal2)
        {
            SaveAction(true, SA_ACCOLORS);
            MycRP.acEditColorsS[noColMod] = MycRP.Palette[acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20)];
            InvalidateRect(hwTB, NULL, TRUE);
            InvalidateRect(hwTB2, NULL, TRUE);
        }
        else if (Start_Imported_Col_Exchange == 2)
        {
            SaveAction(true, SA_PALETTE);
            UINT precolpos = acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20);
            if (precolpos < MycRom.noColors) break;
            for (int tco = 0; tco <= abs((int)(Fin_Import_Color - Ini_Import_Color)); tco++)
            {
                if (precolpos + tco >= N_PALETTES * 64) continue;
                MycRP.Palette[precolpos + tco] = rgb888_to_rgb565(MycRP.importedPal[(Import_Color_Frame * 64 + min((int)Ini_Import_Color,(int)Fin_Import_Color) + tco) * 3], MycRP.importedPal[(Import_Color_Frame * 64 + min((int)Ini_Import_Color, (int)Fin_Import_Color) + tco) * 3 + 1], MycRP.importedPal[(Import_Color_Frame * 64 + min((int)Ini_Import_Color, (int)Fin_Import_Color) + tco) * 3 + 2]);
            }
            Start_Imported_Col_Exchange = 0;
            InvalidateRect(hWin, NULL, TRUE);
        }
        else if (Start_Col_Exchange)
        {
            UINT Der_Col_Pos = (UINT)(acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20));
            if (Der_Col_Pos < MycRom.noColors)
            {
                //Start_Col_Exchange = false;
                break;
            }
            UINT16 tmpCol = MycRP.Palette[Pre_Col_Pos];
            MycRP.Palette[Pre_Col_Pos] = MycRP.Palette[Der_Col_Pos];
            MycRP.Palette[Der_Col_Pos] = tmpCol;
            Start_Col_Exchange = false;
        }
        else if ((hWin == hPal) && (!(GetKeyState(VK_SHIFT) & 0x8000)) && (!(GetKeyState(VK_CONTROL) & 0x8000)) && (!(GetKeyState(VK_MENU) & 0x8000)))
        {
            if (noColMod < 16)
            {
                SaveAction(true, SA_ACCOLORS);
                MycRP.acEditColorsS[noColSel] = MycRP.Palette[acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20)];
                InvalidateRect(hWin, NULL, TRUE);
                InvalidateRect(hwTB, NULL, TRUE);
                InvalidateRect(hwTB2, NULL, TRUE);
            }
            else
            {
                SaveAction(true, SA_DYNACOLOR);
                for (UINT tj = 0; tj < nSelFrames; tj++)
                {
                    if (!nEditExtraResolutionF)
                        MycRom.Dyna4Cols[SelFrames[tj] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + acDynaSet * MycRom.noColors + (noColMod - 16)] = MycRP.Palette[acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20)];
                    else
                        MycRom.Dyna4ColsX[SelFrames[tj] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + acDynaSet * MycRom.noColors + (noColMod - 16)] = MycRP.Palette[acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20)];
                }
            }
            DestroyWindow(hWin);
            int colid;
            if (noColMod < 16) colid = IDC_COL1 + noColMod;
            else colid = IDC_DYNACOL1 + noColMod - 16;
            InvalidateRect(GetDlgItem(hwTB, colid), NULL, TRUE);
        }
        else if (GetKeyState(VK_SHIFT) & 0x8000 && !(GetKeyState(VK_CONTROL) & 0x8000))
        {
            Ini_Gradient_Color = (UINT)(acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20));
            if (Ini_Gradient_Color >= MycRom.noColors || hWin == hPal3)
            {
                Start_Gradient = true;
                Fin_Gradient_Color = Ini_Gradient_Color;
            }
        }
        else if ((hWin == hPal) && (GetKeyState(VK_CONTROL) & 0x8000))
        {
            if (GetKeyState(VK_SHIFT) & 0x8000) Draw_Grad_Opposite = true;
            else Draw_Grad_Opposite = false;
            Draw_Grad_Ini = (UINT)(acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20));
            Start_Gradient2 = true;
            Draw_Grad_Fin = Draw_Grad_Ini;
        }
        else if (GetKeyState(VK_MENU) & 0x8000)
        {
            if (!Start_Col_Exchange)
            {
                Pre_Col_Pos = (UINT)(acPalette * 64 + ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20));
                if (Pre_Col_Pos < MycRom.noColors) break;
                Start_Col_Exchange = true;
            }
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        if (hWin == hPal2) break;
        if (Start_Gradient)
        {
            if (hWin == hPal)
            {
                SaveAction(true, SA_PALETTE);
                if (abs(Ini_Gradient_Color - Fin_Gradient_Color) < 2)
                {
                    Start_Gradient = false;
                    break;
                }
                UINT icol = min(Ini_Gradient_Color, Fin_Gradient_Color);
                UINT fcol = max(Ini_Gradient_Color, Fin_Gradient_Color);
                UINT8 rcoli, gcoli, bcoli;
                rgb565_to_rgb888(MycRP.Palette[icol], &rcoli, &gcoli, &bcoli);
                UINT8 rcolf, gcolf, bcolf;
                rgb565_to_rgb888(MycRP.Palette[fcol], &rcolf, &gcolf, &bcolf);
                float nint = (float)(fcol - icol);
                for (UINT8 ti = 1; ti < fcol - icol; ti++)
                {
                    MycRP.Palette[icol + ti] = rgb888_to_rgb565((UINT8)(rcoli + (float)ti * ((float)rcolf - (float)rcoli) / (float)nint),
                        (UINT8)(gcoli + (float)ti * ((float)gcolf - (float)gcoli) / (float)nint),
                        (UINT8)(bcoli + (float)ti * ((float)bcolf - (float)bcoli) / (float)nint));
                }
                InvalidateRect(hWin, NULL, TRUE);
                InvalidateRect(hwTB, NULL, TRUE);
            }
            else
            {
                SaveAction(true, SA_COLROT);
                UINT icol = min(Ini_Gradient_Color, Fin_Gradient_Color);
                UINT fcol = max(Ini_Gradient_Color, Fin_Gradient_Color);
                UINT ncolrot = fcol - icol + 1;
                if (ncolrot > MAX_LENGTH_COLOR_ROTATION - 2) ncolrot = MAX_LENGTH_COLOR_ROTATION - 2;
                for (UINT ti = 0; ti < nSelFrames; ti++)
                {
                    UINT16* pcolrot;
                    if (nEditExtraResolutionF)
                        pcolrot = &MycRom.ColorRotationsX[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + acColRot * MAX_LENGTH_COLOR_ROTATION];
                    else
                        pcolrot = &MycRom.ColorRotations[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + acColRot * MAX_LENGTH_COLOR_ROTATION];
                    pcolrot[0] = (UINT16)ncolrot;
                    pcolrot[1] = 50;
                    for (UINT tj = 0; tj < ncolrot; tj++) pcolrot[2 + tj] = MycRP.Palette[icol + tj];
                }
                UpdateColorRotDur(hColSet);
                InitColorRotation();
                InvalidateRect(GetDlgItem(hColSet, IDC_COLORS), NULL, TRUE);
                DestroyWindow(hWin);
                hPal = hPal2 = hPal3 = NULL;
            }
        }
        else if (Start_Imported_Col_Exchange == 1) Start_Imported_Col_Exchange = 2;
        Start_Gradient = false;
        Start_Gradient2 = false;
        //Start_Imported_Col_Exchange = 0;
//        Start_Col_Exchange = false;
        break;
    }
    case WM_RBUTTONDOWN:
    {
        if (hWin == hPal3) break;
        POINT pt;
        UINT16 rRGB;
        GetCursorPos(&pt);
        LONG xm = pt.x, ym = pt.y;
        ScreenToClient(hWin, &pt);
        if ((pt.x < MARGIN_PALETTE_X) || (pt.x >= 160 + MARGIN_PALETTE_X) || (pt.y < MARGIN_PALETTE_Y) || (pt.y >= 160 + MARGIN_PALETTE_Y)) break;
        UINT8 colclicked = ((UINT8)(pt.x - MARGIN_PALETTE_X) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE_Y) / 20);
        if (acPalette == 0 && colclicked < MycRom.noColors) break;
        if (!(GetKeyState(VK_SHIFT) & 0x8000) && !(GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
        {
            rRGB = MycRP.Palette[acPalette * 64 + colclicked];
            if (ColorPicker(&rRGB, xm, ym, hWin))
            {
                for (UINT ti = 0; ti < nSelFrames; ti++)
                {
                    MycRP.Palette[acPalette * 64 + colclicked] = rRGB;
                }
                InvalidateRect(hWin, NULL, FALSE);
                if (hWin == hPal)
                {
                    for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++)
                    {
                        InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                        InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                    }
                    for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                }
                else
                {
                    for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                }
            }
        }
        else if ((GetKeyState(VK_CONTROL) & 0x8000) > 0 && Color_Pipette == 0)
        {
            Color_Pipette = acPalette * 64 + colclicked;
            Mouse_Mode = 0;
            glfwSetCursor(glfwframe, glfwdropcur);
            SetCursor(hcColPick);
        }
        else if ((GetKeyState(VK_MENU) & 0x8000) > 0)
        {
            SaveAction(true, SA_PALETTE);
            MycRP.Palette[acPalette * 64 + colclicked] = MycRP.acEditColorsS[noColSel];
        }
        break;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}


LRESULT CALLBACK WndProc(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hWin);
            break;
        default:
            return DefWindowProc(hWin, message, wParam, lParam);
        }
        break;
    }
    case WM_MOUSEMOVE:
        if (Paste_Mode)
        {
            POINT tpt;
            GetCursorPos(&tpt);
            int xspos, yspos;
            glfwGetWindowPos(glfwframe, &xspos, &yspos);
            if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
            {
                paste_offsetx = (int)((tpt.x - xspos) / (2 * frame_zoom) - Paste_Width / 2);
                paste_offsety = (int)((tpt.y - yspos) / (2 * frame_zoom) - Paste_Height / 2);
            }
            else
            {
                paste_offsetx = (int)((tpt.x - xspos) / frame_zoom - Paste_Width / 2);
                paste_offsety = (int)((tpt.y - yspos) / frame_zoom - Paste_Height / 2);
            }
            return TRUE;
        }
        break;
    case WM_MOUSEWHEEL:
    {
        if (MycRom.name[0] == 0) return TRUE;
        short step = (short)HIWORD(wParam) / WHEEL_DELTA;
        if (!(LOWORD(wParam) & MK_SHIFT)) PreFrameInStrip -= step;
        else PreFrameInStrip -= step * (int)NFrameToDraw;
        if (PreFrameInStrip < 0) PreFrameInStrip = 0;
        if (PreFrameInStrip >= (int)MycRom.nFrames) PreFrameInStrip = (int)MycRom.nFrames - 1;
        UpdateFSneeded = true;
        return TRUE;
    }
    case WM_SETCURSOR:
    {
        if (Paste_Mode) SetCursor(hcPaste);
        else if (Color_Pipette > 0) SetCursor(hcColPick);
        else DefWindowProc(hWin, message, wParam, lParam);
        return TRUE;
    }
    case WM_GETMINMAXINFO:
    {
        int valx = GetSystemMetrics(SM_CXFULLSCREEN) + 15;
        int valy = GetSystemMetrics(SM_CYFULLSCREEN) + 25;
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 1680 + 16;
        mmi->ptMinTrackSize.y = 546 + 59;
        mmi->ptMaxTrackSize.x = valx;
        mmi->ptMaxTrackSize.y = valy;
        return 0;
        break;
    }
    case WM_SIZE:
    {
        if (!IsIconic(hWnd))
        {
            Calc_Resize_Frame();
            UpdateFSneeded = true;
            int cxClient = LOWORD(lParam);
            int cyClient = HIWORD(lParam);
            SendMessage(hStatus, WM_SIZE, 0, MAKELPARAM(cxClient, cyClient));
            RECT rcStatusBar;
            GetWindowRect(hStatus, &rcStatusBar);
            int statusBarHeight = rcStatusBar.bottom - rcStatusBar.top;
            MoveWindow(hStatus, 0, cyClient - statusBarHeight, cxClient, statusBarHeight, TRUE);
        }
    }
    case WM_MOVE:
    {
        if (Paste_Mode)
        {
            int xmin = 257, xmax = -1, ymin = 65, ymax = -1;
            for (int tj = 0; tj < (int)Paste_Height; tj++)
            {
                for (int ti = 0; ti < (int)Paste_Width; ti++)
                {
                    if (Paste_Mask[ti + tj * Paste_Width] == 0) continue;
                    if (ti < xmin) xmin = ti;
                    if (ti > xmax) xmax = ti;
                    if (tj < ymin) ymin = tj;
                    if (tj > ymax) ymax = tj;
                }
            }
            /*int tx, ty;
            glfwGetWindowPos(glfwframe, &tx, &ty);
            if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
            {
                paste_centerx = (int)(tx + 2 * frame_zoom * (xmax + xmin) / 2);
                paste_centery = (int)(ty + 2 * frame_zoom * (ymax + ymin) / 2);
            }
            else
            {
                paste_centerx = (int)(tx + frame_zoom * (xmax + xmin) / 2);
                paste_centery = (int)(ty + frame_zoom * (ymax + ymin) / 2);
            }*/
        }
        break;
    }
    case WM_CLOSE:
    {
        if (MessageBoxA(hWnd, "Confirm you want to exit?", "Confirm", MB_YESNO) == IDYES)
        {
            MycRom.name[0] = 0;
            SaveWindowPosition();
            DestroyWindow(hWnd);
        }
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        fDone = true;
        break;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndProc2(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        if (!IsIconic(hWin))
        {
            Calc_Resize_Sprite();
            UpdateSSneeded = true;
        }
        int cxClient = LOWORD(lParam);
        int cyClient = HIWORD(lParam);
        SendMessage(hStatus2, WM_SIZE, 0, MAKELPARAM(cxClient, cyClient));
        RECT rcStatusBar;
        GetWindowRect(hStatus2, &rcStatusBar);
        int statusBarHeight = rcStatusBar.bottom - rcStatusBar.top;
        MoveWindow(hStatus2, 0, cyClient - statusBarHeight, cxClient, statusBarHeight, TRUE);
        break;
    }
    case WM_GETMINMAXINFO:
    {
        int valx = GetSystemMetrics(SM_CXFULLSCREEN) + 15;
        int valy = GetSystemMetrics(SM_CYFULLSCREEN) + 25;
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 1250 + 16;
        mmi->ptMinTrackSize.y = 546 + 59;
        mmi->ptMaxTrackSize.x = valx;
        mmi->ptMaxTrackSize.y = valy;
        return 0;
        break;
    }
    case WM_CLOSE:
    {
        if (MessageBoxA(hWin, "Confirm you want to exit?", "Confirm", MB_YESNO) == IDYES)
        {
            MycRom.name[0] = 0;
            SaveWindowPosition();
            DestroyWindow(hWin);
        }
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        fDone = true;
        break;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndProc3(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        if (!IsIconic(hWin)) Calc_Resize_Image();
        int cxClient = LOWORD(lParam);
        int cyClient = HIWORD(lParam);
        SendMessage(hStatus3, WM_SIZE, 0, MAKELPARAM(cxClient, cyClient));
        RECT rcStatusBar;
        GetWindowRect(hStatus3, &rcStatusBar);
        int statusBarHeight = rcStatusBar.bottom - rcStatusBar.top;
        MoveWindow(hStatus3, 0, cyClient - statusBarHeight, cxClient, statusBarHeight, TRUE);
        float imgratio = (float)width_image / (float)height_image;
        UINT oldw = image_sizeW, oldcropw = initcropwidth, oldooffx = crop_ioffsetx, oldooffy = crop_ioffsety, oldofoffx = crop_foffsetx, oldofoffy = crop_foffsety;
        int oldcropred = crop_reductioni,oldfcropred = crop_reductionf;
        if (imgratio > (float)ScrW3 / (float)ScrH3)
        {
            image_sizeW = ScrW3;
            image_sizeH = (UINT)((float)image_sizeW / imgratio);
        }
        else
        {
            image_sizeH = ScrH3;
            image_sizeW = (UINT)((float)image_sizeH * imgratio);
        }
        image_posx = (ScrW3 - image_sizeW) / 2;
        image_posy = (ScrH3 - image_sizeH) / 2;
        float resizeratio = (float)image_sizeW / (float)oldw;
        initcropwidth = (int)(oldcropw * resizeratio);
        crop_reductioni = (int)(oldcropred * resizeratio);
        crop_ioffsetx = (int)(oldooffx * resizeratio);
        crop_ioffsety = (int)(oldooffy * resizeratio);
        crop_reductionf = (int)(oldfcropred * resizeratio);
        crop_foffsetx = (int)(oldofoffx * resizeratio);
        crop_foffsety = (int)(oldofoffy * resizeratio);
        mouse_scroll_callback3(glfwimages, 0, 0);
        UpdateCropSize();
        break;
    }
    case WM_GETMINMAXINFO:
    {
        int valx = GetSystemMetrics(SM_CXFULLSCREEN) + 15;
        int valy = GetSystemMetrics(SM_CYFULLSCREEN) + 25;
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 1410 + 16;
        mmi->ptMinTrackSize.y = 546 + 59;
        mmi->ptMaxTrackSize.x = valx;
        mmi->ptMaxTrackSize.y = valy;
        return 0;
        break;
    }
    case WM_CLOSE:
    {
        if (MessageBoxA(hWin, "Confirm you want to exit?", "Confirm", MB_YESNO) == IDYES)
        {
            MycRom.name[0] = 0;
            SaveWindowPosition();
            DestroyWindow(hWin);
        }
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        fDone = true;
        break;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}

void UpdateColorSetNames(HWND hwDlg)
{
    for (UINT ti = 0; ti < 8; ti++)
    {
        if (MycRP.activeColSet[MycRP.preColSet + ti])
        {
            SetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), &MycRP.nameColSet[(MycRP.preColSet + ti) * 64]);
            EnableWindow(GetDlgItem(hwDlg, ti + IDC_NOMCOLSET1), TRUE);
        }
        else
        {
            char Num[8];
            sprintf_s(Num, 8, "%02i", MycRP.preColSet + ti);
            SetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), Num);
            EnableWindow(GetDlgItem(hwDlg, ti + IDC_NOMCOLSET1), FALSE);
        }
    }
}
void UpdateColorRotDur(HWND hwDlg)
{
    char tbuf[8];
    UINT16* pcolrot;
    if (nEditExtraResolutionF)
        pcolrot = &MycRom.ColorRotationsX[acFrame * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN];
    else
        pcolrot = &MycRom.ColorRotations[acFrame * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN];
    for (UINT ti = 0; ti < 8; ti++)
    {
        if (ti < MAX_COLOR_ROTATIONN && pcolrot[(ti + preColRot) * MAX_LENGTH_COLOR_ROTATION] != 0)
        {
            sprintf_s(tbuf, 8, "%i", pcolrot[(ti + preColRot) * MAX_LENGTH_COLOR_ROTATION + 1]);
            SetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), tbuf);
            EnableWindow(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), TRUE);
        }
        else
        {
            SetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), "");
            EnableWindow(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), FALSE);
        }
    }
}
void activateColSet(int noSet)
{
    MycRP.activeColSet[noSet + MycRP.preColSet] = true;
    EnableWindow(GetDlgItem(hColSet, noSet + IDC_NOMCOLSET1), TRUE);
}
/*void activateColRot(int noSet)
{
    MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + noSet * 3 + preColRot] = Ini_Gradient_Color;
    MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + noSet * 3 + preColRot + 1] = Fin_Gradient_Color;
    MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + noSet * 3 + preColRot + 2] = 50;
    EnableWindow(GetDlgItem(hColSet, noSet + IDC_NOMCOLSET1), TRUE);
}*/

void SaveColSetNames(HWND hwDlg)
{
    for (UINT ti = 0; ti < 8; ti++)
    {
        if (MycRP.activeColSet[ti + MycRP.preColSet])
        {
            GetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), &MycRP.nameColSet[(ti + MycRP.preColSet) * 64], 64);
        }
    }
}

LRESULT CALLBACK ColSet_Proc(HWND hwDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_MOUSEWHEEL:
        {
            if ((short)(HIWORD(wParam)) > 0) SendMessage(hwDlg, WM_VSCROLL, SB_LINEUP, (LPARAM)GetDlgItem(hwDlg, IDC_SBV));
            else SendMessage(hwDlg, WM_VSCROLL, SB_LINEDOWN, (LPARAM)GetDlgItem(hwDlg, IDC_SBV));
            return TRUE;
        }
        case WM_VSCROLL:
        {
            int acpos = GetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL);
            if (ColSetMode==0) SaveColSetNames(hwDlg);
            switch (LOWORD(wParam))
            {
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
                SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, HIWORD(wParam), TRUE);
                break;
            case SB_LINEDOWN:
                if (ColSetMode == 0)
                {
                    if (acpos < MAX_COL_SETS - 8) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, acpos + 1, TRUE);
                }
                else
                {
                    if (acpos < MAX_COLOR_ROTATIONN - 8) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, acpos + 1, TRUE);
                }
                break;
            case SB_LINEUP:
                if (acpos > 0) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, acpos - 1, TRUE);
                break;
            case SB_PAGEDOWN:
                if (ColSetMode == 0)
                {
                    if (acpos < MAX_COL_SETS - 8) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, min(MAX_COL_SETS - 8, acpos + 8), TRUE);
                }
                else
                {
                    if (acpos < MAX_COLOR_ROTATIONN - 8) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, min(0, acpos + 8), TRUE);
                }
                break;
            case SB_PAGEUP:
                if (acpos > 0) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, max(0, acpos - 8), TRUE);
                break;
            }
            MycRP.preColSet = GetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL);
            InvalidateRect(GetDlgItem(hwDlg, IDC_COLORS),NULL, FALSE);
            if (ColSetMode == 0) UpdateColorSetNames(hwDlg); else UpdateColorRotDur(hwDlg);
            return TRUE;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
            if (pDIS->CtlID != IDC_COLORS) return FALSE;
            RECT rcColors, rcFirst, rcNext;
            GetWindowRect(GetDlgItem(hwDlg, IDC_COLORS), &rcColors);
            rcColors.right -= rcColors.left;
            rcColors.bottom -= rcColors.top;
            rcColors.left = 0;
            rcColors.top = 0;
            GetWindowRect(GetDlgItem(hwDlg, IDC_SETSET1), &rcFirst);
            GetWindowRect(GetDlgItem(hwDlg, IDC_SETSET2), &rcNext);
            int colstep = rcNext.top - rcFirst.top;
            int colsize = rcFirst.bottom - rcFirst.top-2;
            HBRUSH hbr = CreateSolidBrush(RGB(255,255,255));
            FillRect(pDIS->hDC, &rcColors, hbr);
            DeleteObject(hbr);
            for (UINT tj = 0; tj < 8; tj++)
            {
                if (ColSetMode == 0)
                {
                    if (MycRP.activeColSet[tj + MycRP.preColSet])
                    {
                        for (UINT ti = 0; ti < MycRom.noColors; ti++)
                        {
                            rcFirst.left = (ti % 4) * colsize;
                            rcFirst.right = ((ti % 4) + 1) * colsize - 1;
                            rcFirst.top = tj * colstep + (ti / 4) * (colstep - 2) / 4;
                            rcFirst.bottom = rcFirst.top + (colstep - 2) / 4;
                            hbr = CreateSolidBrush(RGB565_to_RGB888(MycRP.ColSets[(tj + MycRP.preColSet) * 16 + ti]));
                            FillRect(pDIS->hDC, &rcFirst, hbr);
                            DeleteObject(hbr);
                        }
                    }
                }
                else
                {
                    if (tj >= MAX_COLOR_ROTATIONN) continue;
                    UINT16* pcolrot;
                    if (nEditExtraResolutionF)
                        pcolrot=&MycRom.ColorRotationsX[acFrame * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + MAX_LENGTH_COLOR_ROTATION * (tj + preColRot)];
                    else
                        pcolrot = &MycRom.ColorRotations[acFrame * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + MAX_LENGTH_COLOR_ROTATION * (tj + preColRot)];
                    if (pcolrot[0] != 0)
                    {
                        UINT16 ncol = pcolrot[0];
                        for (UINT ti = 0; ti < ncol; ti++)
                        {
                            rcFirst.left = ti * rcColors.right / ncol;
                            rcFirst.right = (ti + 1) * rcColors.right / ncol;
                            rcFirst.top = tj * colstep;
                            rcFirst.bottom = (tj + 1) * colstep - 20;
                            hbr = CreateSolidBrush(RGB565_to_RGB888(pcolrot[ti+2]));
                            FillRect(pDIS->hDC, &rcFirst, hbr);
                            DeleteObject(hbr);
                        }
                    }
                }
            }
            return TRUE;
        }
        case WM_LBUTTONDOWN:
        {
            POINT mp;
            GetCursorPos(&mp);
            RECT rcColors;
            GetWindowRect(GetDlgItem(hwDlg, IDC_COLORS), &rcColors);
            if ((mp.x < rcColors.left) || (mp.x >= rcColors.right) || (mp.y < rcColors.top) || (mp.y >= rcColors.bottom)) return FALSE; // we are not on the colors zone, so we don't manage the code
            RECT rcFirst, rcNext;
            GetWindowRect(GetDlgItem(hwDlg, IDC_SETSET1), &rcFirst);
            GetWindowRect(GetDlgItem(hwDlg, IDC_SETSET2), &rcNext);
            int colstep = rcNext.top - rcFirst.top;
            int colsize=colstep;
            //if (MycRom.noColors == 4) colsize /= 4;
            int py = mp.y - rcColors.top;
            if (py % colstep < colsize)
            {
                if (ColSetMode == 0)
                {
                    int scolset = py / colstep + MycRP.preColSet;
                    if (MycRP.activeColSet[scolset])
                    {
                        SaveAction(true, SA_ACCOLORS);
                        for (UINT ti = 0; ti < MycRom.noColors; ti++) MycRP.acEditColorsS[ti] = MycRP.ColSets[scolset * 16 + ti];
                        for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                        for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++)
                        {
                            InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                            InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                        }
                    }
                }
                else
                {
                    int scolset = py / colstep + preColRot;
                    if (scolset >= MAX_COLOR_ROTATIONN) break;
                    UINT16* pcolrot;
                    if (nEditExtraResolutionF)
                        pcolrot = &MycRom.ColorRotationsX[acFrame * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + scolset * MAX_LENGTH_COLOR_ROTATION];
                    else
                        pcolrot = &MycRom.ColorRotations[acFrame * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + scolset * MAX_LENGTH_COLOR_ROTATION];
                    //if (pcolrot[0] != 0)
                    {
                        acColRot = scolset;
                        InvalidateRect(GetDlgItem(hwDlg, IDC_COLORS), NULL, TRUE);
                        Ini_Gradient_Color = Fin_Gradient_Color = 255;
                        Choose_Color_Palette3();
                    }
                }
            }
            return TRUE;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT me{};
            me.cbSize = sizeof(TRACKMOUSEEVENT);
            me.dwFlags = TME_HOVER | TME_LEAVE;
            me.hwndTrack = hwDlg;
            me.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&me);
            return TRUE;
        }
        case WM_MOUSELEAVE:
        {
            POINT mp;
            RECT rcColSet;
            GetWindowRect(hwDlg, &rcColSet);
            GetCursorPos(&mp);
            if ((mp.x > rcColSet.left) && (mp.x < rcColSet.right) && (mp.y > rcColSet.top) && (mp.y < rcColSet.bottom)) break; // we are on a child control of the dialog, but the cursor didn't leave the dialog
            SaveColSetNames(hwDlg);
            DestroyWindow(hwDlg);
            hColSet = NULL;
            return TRUE;
        }
        case WM_INITDIALOG:
        {
            //if (ColSetMode == 0) SaveAction(true, SA_COLSETS);
            HWND hSB = GetDlgItem(hwDlg, IDC_SBV);
            SCROLLINFO si = { 0 };
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = 0;
            if (ColSetMode == 0) si.nMax = MAX_COL_SETS - 1; else si.nMax = MAX_COLOR_ROTATIONN - 1;
            si.nPage = 8;
            if (ColSetMode == 0) si.nPos = MycRP.preColSet; else si.nPos = preColRot;
            SetScrollInfo(hSB, SB_CTL, &si, TRUE);
            for (UINT ti = IDC_SETSET1; ti <= IDC_SETSET8; ti++)
            {
                if (ColSetMode == 0) SetDlgItemTextA(hwDlg, ti, "Set");
                else SetDlgItemTextA(hwDlg, ti, "Del");
            }
            if (ColSetMode == 0) UpdateColorSetNames(hwDlg); else UpdateColorRotDur(hwDlg);
            return TRUE;
        }
        case WM_COMMAND:
        {
            if ((LOWORD(wParam) >= IDC_SETSET1) && (LOWORD(wParam) <= IDC_SETSET8) && (HIWORD(wParam) == BN_CLICKED))
            {
                if (ColSetMode == 0)
                {
                    SaveAction(true, SA_COLSETS);
                    activateColSet(LOWORD(wParam) - IDC_SETSET1);
                    for (UINT ti = 0; ti < MycRom.noColors; ti++) MycRP.ColSets[(MycRP.preColSet + LOWORD(wParam) - IDC_SETSET1) * 16 + ti] = MycRP.acEditColorsS[ti];
                    InvalidateRect(GetDlgItem(hwDlg, IDC_COLORS), NULL, TRUE);
                }
                else
                {
                    SaveAction(true, SA_COLROT);
                    int noset = LOWORD(wParam) - IDC_SETSET1;
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        if (nEditExtraResolutionF)
                            MycRom.ColorRotationsX[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + MAX_LENGTH_COLOR_ROTATION * noset] = 0;
                        else
                            MycRom.ColorRotations[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + MAX_LENGTH_COLOR_ROTATION * noset] = 0;
                        //MyColRot.firstcol[noset] = 255;
                        EnableWindow(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + noset), FALSE);
                        SetDlgItemTextA(hwDlg, IDC_NOMCOLSET1 + noset, "");
                        InvalidateRect(GetDlgItem(hwDlg, IDC_COLORS), NULL, TRUE);
                    }
                    InitColorRotation();
                }
                return TRUE;
            }
            else if ((LOWORD(wParam) >= IDC_NOMCOLSET1) && (LOWORD(wParam) <= IDC_NOMCOLSET8) && (HIWORD(wParam) == EN_KILLFOCUS))
            {
                if (ColSetMode == 0)
                {
                    SaveAction(true, SA_COLSETS);
                    char tbuf[64];
                    GetDlgItemTextA(hwDlg, LOWORD(wParam), tbuf, 63);
                    tbuf[63] = 0;
                    int noset = LOWORD(wParam) - IDC_NOMCOLSET1;
                    strcpy_s(&MycRP.nameColSet[noset * 64], 64, tbuf);
                    return TRUE;
                }
                else
                {
                    SaveAction(true, SA_COLROT);
                    char tbuf[256];
                    GetDlgItemTextA(hwDlg, LOWORD(wParam), tbuf, 256);
                    int noset = LOWORD(wParam) - IDC_NOMCOLSET1;
                    int val = atoi(tbuf);
                    if (val == 0) val = 5;
                    if (val > 255) val = 255;
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        if (nEditExtraResolutionF)
                            MycRom.ColorRotationsX[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + MAX_LENGTH_COLOR_ROTATION * noset + 1] = (UINT16)val;
                        else
                            MycRom.ColorRotations[SelFrames[ti] * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN + MAX_LENGTH_COLOR_ROTATION * noset + 1] = (UINT16)val;
                    }
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

void ScreenToClient(HWND hwnd, RECT *prc)
{
    POINT pt;
    pt.x = prc->left;
    pt.y = prc->top;
    ScreenToClient(hwnd, &pt);
    prc->left = pt.x;
    prc->top = pt.y;
    pt.x = prc->right;
    pt.y = prc->bottom;
    ScreenToClient(hwnd, &pt);
    prc->right = pt.x;
    prc->bottom = pt.y;
}

char askname_res[SIZE_MASK_NAME];

INT_PTR AskName_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetDlgItemTextA(hDlg, IDC_NAME, askname_res);
            //SetFocus(GetDlgItem(hDlg, IDC_NAME));
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDOK:
                {
                    GetDlgItemTextA(hDlg, IDC_NAME, askname_res, SIZE_MASK_NAME - 1);
                }
                case IDCANCEL:
                {
                    EndDialog(hDlg, TRUE);
                    return wParam;
                }
            }
        }
    }
    return FALSE;
}

/*LRESULT CALLBACK CBList_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (message)
    {
        case WM_RBUTTONDOWN:
        {
            if (MycRom.name[0] == 0) return TRUE;
            POINT tp;
            GetCursorPos(&tp);
            ScreenToClient(hDlg, &tp);
            int noItem=(int)SendMessage(hDlg, LB_ITEMFROMPOINT, 0, MAKELPARAM(tp.x, tp.y));
            if (noItem == 0) return TRUE;
            strcpy_s(askname_res, 64, &MycRP.Mask_Names[SIZE_MASK_NAME * (noItem - 1)]);
            if (DialogBox(hInst, MAKEINTRESOURCE(IDD_ASKNAME), hDlg, (DLGPROC)AskName_Proc) == IDOK)
            {
                strcpy_s(&MycRP.Mask_Names[SIZE_MASK_NAME * (noItem - 1)], SIZE_MASK_NAME-1, askname_res);
                UpdateMaskList();
            }
            return TRUE;
        }
    }
    return DefSubclassProc(hDlg,message,wParam,lParam);
}*/

HBRUSH CreateGradientBrush(UINT icol, UINT fcol, HDC hDC,LPRECT prect)
{
    HBRUSH Brush = NULL;
    HDC hdcmem = CreateCompatibleDC(hDC);
    HBITMAP hbitmap = CreateCompatibleBitmap(hDC, prect->right - prect->left, prect->bottom - prect->top);
    if (!SelectObject(hdcmem, hbitmap))
    {
        int tmp = 0;
    }

    const UINT fc = max(icol, fcol), ic = min(icol, fcol);
    double colwid = (double)(fc - ic + 1) / (double)(prect->right - prect->left);
    int cstart = 0;
    for (int ti = 0; ti < prect->right - prect->left; ti++)
    {
        int nsamecol = 0;
        int quelcol;
        if (!Draw_Grad_Opposite)
            quelcol = ic + (int)((double)ti * colwid);
        else
            quelcol = fc - (int)((double)ti * colwid);
        while ((ti < prect->right - prect->left) && ((int)((double)ti * colwid) == quelcol))
        {
            nsamecol++;
            ti++;
        }
        if (Draw_Grad_Opposite)
            Brush = CreateSolidBrush(RGB565_to_RGB888(MycRP.Palette[quelcol]));
        else
            Brush = CreateSolidBrush(RGB565_to_RGB888(MycRP.Palette[quelcol]));
        RECT temp;
        temp.left = cstart;
        temp.top = 0;
        temp.right = cstart + nsamecol+1;
        temp.bottom = prect->bottom - prect->top;
        FillRect(hdcmem, &temp, Brush);
        DeleteObject(Brush);
        cstart += nsamecol + 1;
    }
    HBRUSH pattern = CreatePatternBrush(hbitmap);

    DeleteDC(hdcmem);
    DeleteObject(hbitmap);

    return pattern;
}
void EnableDrawButtons(void)
{
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWPOINT), TRUE);
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWLINE), TRUE);
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWRECT), TRUE);
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWCIRC), TRUE);
    EnableWindow(GetDlgItem(hwTB, IDC_FILL), TRUE);
    EnableWindow(GetDlgItem(hwTB, IDC_ELLIPSE), TRUE);
}
void DisableDrawButtons(void)
{
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWPOINT), FALSE);
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWRECT), FALSE);
}
void UpdateFrameSpriteList(void)
{
    SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_RESETCONTENT, 0, 0);
    int ti = 0;
    char tbuf[256];
    wchar_t tbuf2[512];
    while (ti < MAX_SPRITES_PER_FRAME)
    {
        if (MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + ti] == 255)
        {
            ti++;
            continue;
        }
        UINT nospr = MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + ti];
        sprintf_s(tbuf, 256, "%i - %s", nospr, &MycRP.Sprite_Names[SIZE_SECTION_NAMES * nospr]);
        size_t origsize = strlen(tbuf) + 1;
        size_t convertedChars = 0;
        mbstowcs_s(&convertedChars, tbuf2, origsize, tbuf, _TRUNCATE);
        if ((MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + ti) * 4] != 0) || (MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + ti) * 4 + 1] != 0) ||
            (MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + ti) * 4 + 2] != MycRom.fWidth - 1) || (MycRom.FrameSpriteBB[(acFrame * MAX_SPRITES_PER_FRAME + ti) * 4 + 3] != MycRom.fHeight - 1))
            wcscat_s(tbuf2, 512, L" \u2610");
        SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_ADDSTRING, 0, (LPARAM)tbuf2);
        ti++;
    }
    SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_SETCURSEL, 0, 0);
}
const char* ButtonDescription(HWND hOver)
{
    if (hOver == GetDlgItem(hwTB, IDC_NEW)) return (const char*)"Create a new Serum project from a TXT dump file";
    if (hOver == GetDlgItem(hwTB, IDC_ADDTXT)) return (const char*)"Add a TXT dump file to the Serum project";
    if (hOver == GetDlgItem(hwTB, IDC_OPEN)) return (const char*)"Open a Serum project";
    if (hOver == GetDlgItem(hwTB, IDC_SAVE)) return (const char*)"Save the Serum project";
    if (hOver == GetDlgItem(hwTB, IDC_UNDO)) return (const char*)"Undo last action";
    if (hOver == GetDlgItem(hwTB, IDC_REDO)) return (const char*)"Redo last action";
    if (hOver == GetDlgItem(hwTB, IDC_SECTIONNAME)) return (const char*)"Enter the name for a new section here";
    if (hOver == GetDlgItem(hwTB, IDC_ADDSECTION)) return (const char*)"Add a section named after the text in the box beside";
    if (hOver == GetDlgItem(hwTB, IDC_DELSECTION)) return (const char*)"Delete the section";
    if (hOver == GetDlgItem(hwTB, IDC_MOVESECTION)) return (const char*)"Open a dialog to reorder your sections";
    if (hOver == GetDlgItem(hwTB, IDC_NIGHTDAY)) return (const char*)"Switch dark mode ON/OFF";
    if (Edit_Mode == 0)
    {
        if (hOver == GetDlgItem(hwTB, IDC_COLMODE)) return (const char*)"Switch to Colorization mode";
        if (hOver == GetDlgItem(hwTB, IDC_MASKLIST)) return (const char*)"Choose a mask in the list";
        if (hOver == GetDlgItem(hwTB, IDC_POINTMASK)) return (const char*)"Point tool to draw masks";
        if (hOver == GetDlgItem(hwTB, IDC_RECTMASK)) return (const char*)"Filled-rectangle tool to draw masks";
        if (hOver == GetDlgItem(hwTB, IDC_ZONEMASK)) return (const char*)"Magic wand tool to draw masks";
        if (hOver == GetDlgItem(hwTB, IDC_INVERTSEL)) return (const char*)"Invert the mask selection";
        if (hOver == GetDlgItem(hwTB, IDC_SHAPEMODE)) return (const char*)"Switch to shape mode";
        if (hOver == GetDlgItem(hwTB, IDC_ZONEMASK)) return (const char*)"Magic wand tool to draw masks";
        if (hOver == GetDlgItem(hwTB, IDC_SAMEFRAME)) return (const char*)"How many same frames detected";
        if (hOver == GetDlgItem(hwTB, IDC_DELSAMEFR)) return (const char*)"Delete frames similar to the current displayed one taking into account masks and shape mode";
        if (hOver == GetDlgItem(hwTB, IDC_DELSELSAMEFR)) return (const char*)"Delete frames similar to the current selected ones taking into account masks and shape mode";
        if (hOver == GetDlgItem(hwTB, IDC_DELALLSAMEFR)) return (const char*)"Delete all the frames similar taking into account masks and shape mode";
        if (hOver == GetDlgItem(hwTB, IDC_SAMEFRAMELIST)) return (const char*)"List of all the similar frames";
        if (hOver == GetDlgItem(hwTB, IDC_DELFRAME)) return (const char*)"Delete selected frames";
        if (hOver == GetDlgItem(hwTB, IDC_TRIGID)) return (const char*)"[PuP packs] Trigger ID for an event";
        if (hOver == GetDlgItem(hwTB, IDC_DELTID)) return (const char*)"[PuP packs] Delete the trigger";
        if (hOver == GetDlgItem(hwTB, IDC_MASKLIST2)) return (const char*)"Choose a mask here to list the frames using it below";
        if (hOver == GetDlgItem(hwTB, IDC_MOVESECTION)) return (const char*)"List of frames using the mask above";
        if (hOver == GetDlgItem(hwTB, IDC_SECTIONLIST)) return (const char*)"Choose a section here to jump to its first frame";
        if (hOver == GetDlgItem(hwTB, IDC_DURATION)) return (const char*)"Give a new duration value to the selected frame(s) [5;3000]";
        if (hOver == GetDlgItem(hwTB, IDC_SETDUR)) return (const char*)"Apply this duration to the frame(s)";
        if (hOver == GetDlgItem(hwTB, IDC_KEEPSEL)) return (const char*)"All the selected frames won't be deleted while sorting";
        if (hOver == GetDlgItem(hwTB, IDC_FRUSEMASK)) return (const char*)"Frames using the mask above";
        if (hOver == GetDlgItem(hwTB, IDC_COMMON)) return (const char*)"Show you which points in the selected frames are different";
        if (hOver == GetDlgItem(hwTB, IDC_UPDATESAME)) return (const char*)"Update and recalc the same frame list (may take a few seconds)";
        if (hOver == GetDlgItem(hwTB, IDC_TRIGGERLIST)) return (const char*)"List all the triggers available in the Serum";
    }
    else
    {
        if (hOver == GetDlgItem(hwTB, IDC_ORGMODE)) return (const char*)"Switch to Comparison mode";
        for (int ti = 0; ti < 16; ti++)
        {
            if (hOver == GetDlgItem(hwTB, IDC_COL1 + ti))
            {
                char tbuf[512];
                sprintf_s(tbuf, 512, "Select the color #%i for drawing in the 64-colour palette", ti + 1);
                return tbuf;
            }
            if (hOver == GetDlgItem(hwTB, IDC_DYNACOL1 + ti))
            {
                char tbuf[512];
                sprintf_s(tbuf, 512, "Select the color #%i for dynamic colorization in the 64-colour palette", ti + 1);
                return tbuf;
            }
        }
        if (hOver == GetDlgItem(hwTB, IDC_ORGMODE)) return (const char*)"Switch to Comparison mode";
        if (hOver == GetDlgItem(hwTB, IDC_COLSET)) return (const char*)"Show a dialog box to manage the color sets";
        if (hOver == GetDlgItem(hwTB, IDC_COLPICK)) return (const char*)"Pick a color from the displayed frame";
        if (hOver == GetDlgItem(hwTB, IDC_1COLMODE)) return (const char*)"Draw using a single color";
        if (hOver == GetDlgItem(hwTB, IDC_4COLMODE)) return (const char*)"Draw replacing the original frame with the selected set of colors";
        if (hOver == GetDlgItem(hwTB, IDC_GRADMODE)) return (const char*)"Draw using gradients";
        if (hOver == GetDlgItem(hwTB, IDC_GRADMODEB)) return (const char*)"Draw using gradients";
        if (hOver == GetDlgItem(hwTB, IDC_DRAWPOINT)) return (const char*)"Point tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_DRAWLINE)) return (const char*)"Line tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_DRAWRECT)) return (const char*)"Rectangle tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_DRAWCIRC)) return (const char*)"Circle tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_FILL)) return (const char*)"Magic wand tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_ELLIPSE)) return (const char*)"Ellipse tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_FILLED)) return (const char*)"Draw/select rectangles or circles are filled or not?";
//        if (hOver == GetDlgItem(hwTB, IDC_COPYCOLS)) return (const char*)"Copy the color set from the displayed frame to the other selected ones";
        if (hOver == GetDlgItem(hwTB, IDC_COPYCOLS2)) return (const char*)"Copy all the dynamic color sets from the displayed frame to the other selected ones";
        if (hOver == GetDlgItem(hwTB, IDC_CHANGECOLSET)) return (const char*)"Switch between the different dynamic color sets";
        if (hOver == GetDlgItem(hwTB, IDC_COLTODYNA)) return (const char*)"Copy the color set for drawing to the current dynamic set";
        if (hOver == GetDlgItem(hwTB, IDC_IDENT)) return (const char*)"Identify the points on the displayed frame where the active dynamic set is used";
        if (hOver == GetDlgItem(hwTB, IDC_COPY)) return (const char*)"Copy selected content";
        if (hOver == GetDlgItem(hwTB, IDC_PASTE)) return (const char*)"Paste selected content";
        if (hOver == GetDlgItem(hwTB, IDC_INVERTSEL2)) return (const char*)"Invert selection for copy";
        if (hOver == GetDlgItem(hwTB, IDC_SECTIONLIST2)) return (const char*)"Choose a section here to jump to its first frame";
        if (hOver == GetDlgItem(hwTB, IDC_ADDSPRITE2)) return (const char*)"Add the current sprite in the sprite window to the list of sprites to be detected when this frame is found";
        if (hOver == GetDlgItem(hwTB, IDC_DELSPRITE2)) return (const char*)"Delete the current sprite in the sprite window from the list of sprites to be detected when this frame is found";
        if (hOver == GetDlgItem(hwTB, IDC_SPRITELIST2)) return (const char*)"List of sprites to be detected when this frame is found";
        if (hOver == GetDlgItem(hwTB, IDC_COLROT)) return (const char*)"Display a dialog to manage the color rotations";
        if (hOver == GetDlgItem(hwTB, IDC_SCROLLFILL)) return (const char*)"Automatically colorize intermediate frames of a scrolling";
        if (hOver == GetDlgItem(hwTB, IDC_GENAGIF)) return (const char*)"Generate an animated GIF from a continuous sequence of frames";
        if (hOver == GetDlgItem(hwTB, IDC_FRAMETEST)) return (const char*)"Start dmddevice.dll to test specific frames (F5)";
        if (hOver == GetDlgItem(hwTB, IDC_AUTOCOPY)) return (const char*)"Look for the selected part of the frame in the whole uncolorized dump and apply current frame colorization to them";
        if (hOver == GetDlgItem(hwTB, IDC_BBSPOT)) return (const char*)"Show in yellow the bounding box of the sprite in this window";
        if (hOver == GetDlgItem(hwTB, IDC_BGNB)) return (const char*)"Number of the background assigned to this frame";
        if (hOver == GetDlgItem(hwTB, IDC_DELBG)) return (const char*)"Remove the background assigned to this frame";
        if (hOver == GetDlgItem(hwTB, IDC_COPYDYNA)) return (const char*)"Copy all the dynamic content (dynamic mask and dynamic palette) from the displayed frame to the other selected ones";
        if (hOver == GetDlgItem(hwTB, IDC_OUTA)) return (const char*)"Create shadow for the selection with the current selected color all around";
        if (hOver == GetDlgItem(hwTB, IDC_OUTL)) return (const char*)"Create shadow for the selection with the current selected color to the left";
        if (hOver == GetDlgItem(hwTB, IDC_OUTBL)) return (const char*)"Create shadow for the selection with the current selected color to the bottom left";
        if (hOver == GetDlgItem(hwTB, IDC_OUTB)) return (const char*)"Create shadow for the selection with the current selected color to the bottom";
        if (hOver == GetDlgItem(hwTB, IDC_OUTBR)) return (const char*)"Create shadow for the selection with the current selected color to the bottom right";
        if (hOver == GetDlgItem(hwTB, IDC_OUTR)) return (const char*)"Create shadow for the selection with the current selected color to the right";
        if (hOver == GetDlgItem(hwTB, IDC_OUTTR)) return (const char*)"Create shadow for the selection with the current selected color to the top right";
        if (hOver == GetDlgItem(hwTB, IDC_OUTT)) return (const char*)"Create shadow for the selection with the current selected color to the top";
        if (hOver == GetDlgItem(hwTB, IDC_OUTTL)) return (const char*)"Create shadow for the selection with the current selected color to the top left";
        if (hOver == GetDlgItem(hwTB, IDC_UCOUNT)) return (const char*)"Number of Undo actions available";
        if (hOver == GetDlgItem(hwTB, IDC_RCOUNT)) return (const char*)"Number of Redo actions available";
        if (hOver == GetDlgItem(hwTB, IDC_MULTIF)) return (const char*)"Are there multiple frames selected or not?";
        if (hOver == GetDlgItem(hwTB, IDC_EXTRARES)) return (const char*)"Check if you want to add an extra res frame for this displayed frame";
        if (hOver == GetDlgItem(hwTB, IDC_FILTERTYPE)) return (const char*)"What kind of filter do you want to use for original to extra or extra to original res conversion?";
        if (hOver == GetDlgItem(hwTB, IDC_ZOOM2X)) return (const char*)"Zoom 2X the frame display when the frame height is 64 pixels";
        if (hOver == GetDlgItem(hwTB, IDC_DISPEXTRA)) return (const char*)"Display the extra frame if available (a red cross, if not)";
        if (hOver == GetDlgItem(hwTB, IDC_ORGTOXTRA2)) return (const char*)"Create an extra resolution frame from the original one using the filter above";
        if (hOver == GetDlgItem(hwTB, IDC_XTRATOORG2)) return (const char*)"Create an original resolution frame from the extra one using the filter above";
        if (hOver == GetDlgItem(hwTB, IDC_ADDBG)) return (const char*)"Select the current displayed background in the background window as this frame background";
        if (hOver == GetDlgItem(hwTB, IDC_CFTOOF)) return (const char*)"Create a new frame at the end of the project. The ROM image is the current frame colorization. If the colors used to colorize the frame are not the 4 or 16 ones of the first color panel, the action is ognored.";
        if (hOver == GetDlgItem(hwTB, IDC_BAW)) return (const char*)"Convert the full project to black and white. Caution, this action is NOT undoable.";
}
    return "";
}
LRESULT CALLBACK ButtonSubclassProc(HWND hBut, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) 
{
    switch (message) {
        case WM_KEYDOWN:
        {
            return TRUE;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hBut;
            tme.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&tme);
            SetWindowTextA(hStatus, ButtonDescription(hBut));
            break;
        }
        case WM_MOUSELEAVE:
        {
            SetWindowTextA(hStatus, "");
            break;
        }
    }

    return DefSubclassProc(hBut, message, wParam, lParam);
}
void MaskDrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    HDC hdc = lpDIS->hDC;
    RECT rcItem = lpDIS->rcItem;

    wchar_t name[256];
    int itemID = lpDIS->itemID - 1;
    if (itemID <0)
        swprintf_s(name, 256, L"- None -");
    else
    {
        wchar_t coche = L'\u2610';
        if (MaskUsed(itemID)) coche= L'\u2611';
        swprintf_s(name, 256, L"%c Mask #%i :", coche, itemID);
    }

    if (AllSameFramesUpdated)
    {
        if (itemID < 0)
            swprintf_s(name, 256, L"- None - : same frames %i, with shape mode %i", nSameFramesPerMask[0], nSameFramesPerMask[MAX_MASKS + 1]);
        else if (itemID < (int)MycRom.nCompMasks)
        {
            wchar_t coche = L'\u2610';
            if (MaskUsed(itemID)) coche = L'\u2611';
            swprintf_s(name, 256, L"%c Mask #%i : same frames %i, with shape mode %i", coche, itemID, nSameFramesPerMask[lpDIS->itemID], nSameFramesPerMask[lpDIS->itemID + MAX_MASKS + 1]);
        }
    }

    if (lpDIS->itemState & ODS_SELECTED)
        SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    else
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

    ExtTextOut(hdc, rcItem.left, rcItem.top, ETO_OPAQUE, &rcItem, NULL, 0, NULL);

    if (lpDIS->itemState & ODS_SELECTED)
        SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    else
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    TextOut(hdc, rcItem.left + 5, rcItem.top + 2, name, (int)wcslen(name));

    unsigned char bitmap[256 * 64 * 3];
    int zoom = 1;
    if (MycRom.fHeight == 32) zoom = 2;
    int offsx = (256 - zoom * MycRom.fWidth) / 2;
    int offsy = (64 - zoom * MycRom.fHeight) / 2;
    if (lpDIS->itemID > MycRom.nCompMasks) memset(bitmap, 255, 256 * 64 * 3);
    else
    {
        memset(bitmap, 0, 256 * 64 * 3);
        unsigned char violet[3] = { 255,0,255 };

        for (UINT tj = 0; tj < MycRom.fHeight; tj++)
        {
            for (UINT ti = 0; ti < MycRom.fWidth; ti++)
            {
                unsigned char pByte[3] = { 0,0,0 };
                rgb565_to_rgb888(originalcolors[MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + tj * MycRom.fWidth + ti]], pByte);
                if (itemID >= 0 && itemID < (int)MycRom.nCompMasks)
                {
                    if (MycRom.CompMasks[itemID * MycRom.fWidth * MycRom.fHeight + tj * MycRom.fWidth + ti] != 0) memcpy(pByte, violet, 3);
                }
                if (zoom == 2)
                {
                    memcpy(&bitmap[((tj + offsy) * 2 * 256 + ti * 2 + offsx) * 3], pByte, 3);
                    memcpy(&bitmap[(((tj + offsy) * 2 + 1) * 256 + ti * 2 + offsx) * 3], pByte, 3);
                    memcpy(&bitmap[((tj + offsy) * 2 * 256 + ti * 2 + 1 + offsx) * 3], pByte, 3);
                    memcpy(&bitmap[(((tj + offsy) * 2 + 1) * 256 + ti * 2 + 1 + offsx) * 3], pByte, 3);
                }
                else
                    memcpy(&bitmap[((tj + offsy) * 256 + ti + offsx) * 3], pByte, 3);
            }
        }
        for (int i = 0; i < 256 * 64 * 3; i += 3)
        {
            unsigned char temp = bitmap[i];
            bitmap[i] = bitmap[i + 2];
            bitmap[i + 2] = temp;
        }
    }
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 256, 64);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 256;
    bmi.bmiHeader.biHeight = -64;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(hdcMem, 0, 0, 256, 64, 0, 0, 0, 64, bitmap, &bmi, DIB_RGB_COLORS);

    BitBlt(hdc, rcItem.left, rcItem.top + 18, 256, 64, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
}
void ShadowSelection(WORD wdir)
{
    UINT16 acColor = MycRP.acEditColorsS[noColSel];
    UINT16* pcfr;
    UINT8* pdynm;
    UINT fw, fh;
    if (nEditExtraResolutionF)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pcfr = MycRom.cFramesX;
        pdynm = MycRom.DynaMasksX;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pcfr = MycRom.cFrames;
        pdynm = MycRom.DynaMasks;
    }
    for (uint tk = 0; tk < nSelFrames; tk++)
    {
        for (uint tj = 0; tj < fh; tj++)
        {
            for (uint ti = 0; ti < fw; ti++)
            {
                if (Copy_Mask[tj * fw + ti] > 0)
                {
                    if (((wdir == IDC_OUTL) || (wdir == IDC_OUTA)) && (ti > 0))
                    {
                        if ((Copy_Mask[tj * fw + ti - 1] == 0) && (pdynm[SelFrames[tk] * fw * fh + tj * fw + ti - 1] == 255))
                        {
                            pcfr[SelFrames[tk] * fw * fh + tj * fw + ti - 1] = acColor;
                        }
                    }
                    if (((wdir == IDC_OUTBL) || (wdir == IDC_OUTA)) && (ti > 0) && (tj < fh - 1))
                    {
                        if ((Copy_Mask[(tj + 1) * fw + ti - 1] == 0) && (pdynm[SelFrames[tk] * fw * fh + (tj + 1) * fw + ti - 1] == 255))
                        {
                            pcfr[SelFrames[tk] * fw * fh + (tj + 1) * fw + ti - 1] = acColor;
                        }
                    }
                    if (((wdir == IDC_OUTB) || (wdir == IDC_OUTA)) && (tj < fh - 1))
                    {
                        if ((Copy_Mask[(tj + 1) * fw + ti] == 0) && (pdynm[SelFrames[tk] * fw * fh + (tj + 1) * fw + ti] == 255))
                        {
                            pcfr[SelFrames[tk] * fw * fh + (tj + 1) * fw + ti] = acColor;
                        }
                    }
                    if (((wdir == IDC_OUTBR) || (wdir == IDC_OUTA)) && (ti < fw - 1) && (tj < fh - 1))
                    {
                        if ((Copy_Mask[(tj + 1) * fw + ti + 1] == 0) && (pdynm[SelFrames[tk] * fw * fh + (tj + 1) * fw + ti + 1] == 255))
                        {
                            pcfr[SelFrames[tk] * fw * fh + (tj + 1) * fw + ti + 1] = acColor;
                        }
                    }
                    if (((wdir == IDC_OUTR) || (wdir == IDC_OUTA)) && (ti < fw - 1))
                    {
                        if ((Copy_Mask[tj * fw + ti + 1] == 0) && (pdynm[SelFrames[tk] * fw * fh + tj * fw + ti + 1] == 255))
                        {
                            pcfr[SelFrames[tk] * fw * fh + tj * fw + ti + 1] = acColor;
                        }
                    }
                    if (((wdir == IDC_OUTTR) || (wdir == IDC_OUTA)) && (ti < fw - 1) && (tj > 0))
                    {
                        if ((Copy_Mask[(tj - 1) * fw + ti + 1] == 0) && (pdynm[SelFrames[tk] * fw * fh + (tj - 1) * fw + ti + 1] == 255))
                        {
                            pcfr[SelFrames[tk] * fw * fh + (tj - 1) * fw + ti + 1] = acColor;
                        }
                    }
                    if (((wdir == IDC_OUTT) || (wdir == IDC_OUTA)) && (tj > 0))
                    {
                        if ((Copy_Mask[(tj - 1) * fw + ti] == 0) && (pdynm[SelFrames[tk] * fw * fh + (tj - 1) * fw + ti] == 255))
                        {
                            pcfr[SelFrames[tk] * fw * fh + (tj - 1) * fw + ti] = acColor;
                        }
                    }
                    if (((wdir == IDC_OUTTL) || (wdir == IDC_OUTA)) && (ti > 0) && (tj > 0))
                    {
                        if ((Copy_Mask[(tj - 1) * fw + ti - 1] == 0) && (pdynm[SelFrames[tk] * fw * fh + (tj - 1) * fw + ti - 1] == 255))
                        {
                            pcfr[SelFrames[tk] * fw * fh + (tj - 1) * fw + ti - 1] = acColor;
                        }
                    }
                }
            }
        }
    }
}

char isInOriginalColors(UINT16 color)
{
    for (UINT ti = 0; ti < MycRom.noColors; ti++)
    {
        if (originalcolors[ti] == color) return (char)ti;
    }
    return -1;
}
void ConvertCFrameToNewOFrame(void)
{
    if (nEditExtraResolutionF && MycRom.isExtraFrame[acFrame] > 0)
    {
        cprintf(true, "You must be in original resolution mode to convert to a fake ROM frame. Action canceled.");
        return;
    }
    UINT16* pfr = &MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight];
    for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        if (isInOriginalColors(pfr[ti]) == -1)
        {
            cprintf(true, "The colorized frame must only use the original colors of the ROM (4 or 16 firsts of color panel 0) to be converted to a fake ROM frame. Action canceled.");
            return;
        }
    }
    sFrames tsFrame;
    tsFrame.ptr = (char*)malloc(MycRom.fWidth * MycRom.fHeight);
    if (!tsFrame.ptr)
    {
        cprintf(true, "Can't get memory for the temporary sFrame. Action canceled.");
        return;
    }
    for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        tsFrame.ptr[ti] = isInOriginalColors(pfr[ti]);
    }
    tsFrame.active = true;
    tsFrame.timecode = 100;
    tsFrame.hashcode = 0;
    AddTXTFrames2Frame(1, &tsFrame);
    free(tsFrame.ptr);
    UpdateFSneeded = true;
}

INT_PTR CALLBACK Toolbar_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    //UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            /*HWND hwndChild = GetWindow(hDlg, GW_CHILD); // get the first child window
            while (hwndChild != NULL)
            {*/
            SetWindowSubclass(GetDlgItem(hDlg, IDC_NEW), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDTXT), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_OPEN), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_SAVE), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_UNDO), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_REDO), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_SECTIONNAME), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDSECTION), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSECTION), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_MOVESECTION), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_NIGHTDAY), ButtonSubclassProc, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_DURATION), EM_SETLIMITTEXT, 4, 0);
            if (Edit_Mode == 0)
            {
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLMODE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_MASKLIST), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_POINTMASK), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_RECTMASK), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ZONEMASK), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_INVERTSEL), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SHAPEMODE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SAMEFRAME), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELFRAME), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSAMEFR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSELSAMEFR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELALLSAMEFR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SAMEFRAMELIST), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_TRIGID), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELTID), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_MASKLIST2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_MOVESECTION), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SECTIONLIST), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DURATION), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SETDUR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_KEEPSEL), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_FRUSEMASK), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COMMON), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_UPDATESAME), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_TRIGGERLIST), ButtonSubclassProc, 0, 0);
                HFONT hFont = CreateFont(12, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
                SendMessage(GetDlgItem(hDlg, IDC_SAMEFRAME), WM_SETFONT, WPARAM(hFont), TRUE);
            }
            else
            {
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ORGMODE), ButtonSubclassProc, 0, 0);
                for (int ti = 0; ti < 16; ti++)
                {
                    SetWindowSubclass(GetDlgItem(hDlg, IDC_COL1 + ti), ButtonSubclassProc, 0, 0);
                    SetWindowSubclass(GetDlgItem(hDlg, IDC_DYNACOL1 + ti), ButtonSubclassProc, 0, 0);
                    //char tbuf[512];
                    //sprintf_s(tbuf, 512, "(Left click)Select/(Right click)Modify the color #%i of the palette for drawing", ti+1);
                }
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLSET), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLPICK), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_1COLMODE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_4COLMODE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_GRADMODE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWPOINT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWLINE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWRECT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWCIRC), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_FILL), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ELLIPSE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_FILLED), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COPYCOLS), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COPYCOLS2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_CHANGECOLSET), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLTODYNA), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_IDENT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COPY), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_PASTE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_INVERTSEL2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SECTIONLIST2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDSPRITE2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSPRITE2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SPRITELIST2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLROT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SCROLLFILL), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_GENAGIF), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_FRAMETEST), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_AUTOCOPY), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_BBSPOT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_BGNB), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELBG), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COPYDYNA), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_OUTA), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_OUTL), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_OUTBL), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_OUTB), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_OUTBR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_OUTR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_OUTTR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_OUTT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_OUTTL), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_UCOUNT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_RCOUNT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_MULTIF), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_EXTRARES), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_FILTERTYPE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ZOOM2X), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DISPEXTRA), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ORGTOXTRA2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_XTRATOORG2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDBG), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_CFTOOF), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_BAW), ButtonSubclassProc, 0, 0);

                if (MycRP.DrawColMode == 1)
                {
                    SendMessage(GetDlgItem(hDlg, IDC_1COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_4COLMODE), BM_SETCHECK, TRUE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_GRADMODE), BM_SETCHECK, FALSE, 0);
                }
                else if (MycRP.DrawColMode == 0)
                {
                    SendMessage(GetDlgItem(hDlg, IDC_4COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_1COLMODE), BM_SETCHECK, TRUE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_GRADMODE), BM_SETCHECK, FALSE, 0);
                }
                else
                {
                    SendMessage(GetDlgItem(hDlg, IDC_4COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_1COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_GRADMODE), BM_SETCHECK, TRUE, 0);
                    if (MycRP.Fill_Mode == TRUE) Button_SetCheck(GetDlgItem(hwTB, IDC_FILLED), BST_CHECKED); else Button_SetCheck(GetDlgItem(hwTB, IDC_FILLED), BST_UNCHECKED);
                }
            }
            return TRUE;
        }
        case WM_CTLCOLORDLG:
        {
            if (Night_Mode) return (INT_PTR)GetStockObject(DKGRAY_BRUSH);
            return (INT_PTR)GetStockObject(GRAY_BRUSH);
        }
        case WM_PAINT:
        {
            if (MycRom.name[0] == 0) return FALSE;
            if (Edit_Mode == 1)
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hDlg, &ps);
                RECT rc;
                GetWindowRect(GetDlgItem(hDlg, IDC_DRAWPOINT + MycRP.Draw_Mode), &rc);
                rc.bottom += 5;
                HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
                ScreenToClient(hDlg, &rc);
                FillRect(hdc, &rc, br);
                DeleteObject(br);
                EndPaint(hDlg, &ps);
                return TRUE;
            }
            else
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hDlg, &ps);
                RECT rc;
                GetWindowRect(GetDlgItem(hDlg, IDC_POINTMASK + MycRP.Mask_Sel_Mode), &rc);
                rc.bottom += 5;
                HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
                ScreenToClient(hDlg, &rc);
                FillRect(hdc, &rc, br);
                DeleteObject(br);
                EndPaint(hDlg, &ps);
                return TRUE;
            }
            return FALSE;
        }
        case WM_DRAWITEM:
        {
            if (wParam == IDC_MASKLIST && lParam != NULL && Edit_Mode == 0) MaskDrawItem((LPDRAWITEMSTRUCT)lParam);
            else if ((MycRom.name[0] != 0) && (Edit_Mode == 1))
            {
                LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
                UINT tpm;
                if ((lpdis->CtlID >= IDC_COL1) && (lpdis->CtlID < IDC_COL1 + MycRom.noColors))
                {
                    if (lpdis->CtlID)
                        tpm = lpdis->CtlID - IDC_COL1;
                    HBRUSH bg = CreateSolidBrush(RGB565_to_RGB888(MycRP.acEditColorsS[tpm]));
                    RECT rc = lpdis->rcItem;
                    if (tpm == noColSel)
                    {
                        HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
                        FillRect(lpdis->hDC, &rc, br);
                        DeleteObject(br);
                    }
                    rc.left += 2;
                    rc.right -= 2;
                    rc.top += 2;
                    rc.bottom -= 2;
                    FillRect(lpdis->hDC, &rc, bg);
                    DeleteObject(bg);
                }
                else if ((lpdis->CtlID >= IDC_DYNACOL1) && (lpdis->CtlID < IDC_DYNACOL1 + MycRom.noColors))
                {
                    UINT16* pdync;
                    if (nEditExtraResolutionF)
                        pdync = &MycRom.Dyna4ColsX[acFrame * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + acDynaSet * MycRom.noColors];
                    else
                        pdync = &MycRom.Dyna4Cols[acFrame * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + acDynaSet * MycRom.noColors];
                    tpm = lpdis->CtlID - IDC_DYNACOL1;
                    HBRUSH bg = CreateSolidBrush(RGB565_to_RGB888(pdync[tpm]));
                    RECT rc = lpdis->rcItem;
                    rc.left += 2;
                    rc.right -= 2;
                    rc.top += 2;
                    rc.bottom -= 2;
                    FillRect(lpdis->hDC, &rc, bg);
                    DeleteObject(bg);
                }
                else if (lpdis->CtlID == IDC_GRADMODEB)
                {
                    RECT tr;
                    tr.left = lpdis->rcItem.left;
                    tr.right = lpdis->rcItem.right;
                    tr.top = lpdis->rcItem.top;
                    tr.bottom = lpdis->rcItem.bottom;
                    HBRUSH bg = CreateGradientBrush(Draw_Grad_Ini, Draw_Grad_Fin, lpdis->hDC, &tr);
                    FillRect(lpdis->hDC, &tr, bg);
                    DeleteObject(bg);
                    SetTextColor(lpdis->hDC, RGB(SelFrameColor, SelFrameColor, SelFrameColor));
                    SetBkMode(lpdis->hDC, TRANSPARENT);
                    TextOutA(lpdis->hDC, 24, 0, "Gradient draw", (int)strlen("Gradient draw"));
                }
                else if (lpdis->CtlID == IDC_DISPEXTRA)
                {
                    RECT tr;
                    tr.left = lpdis->rcItem.left;
                    tr.right = lpdis->rcItem.right;
                    tr.top = lpdis->rcItem.top;
                    tr.bottom = lpdis->rcItem.bottom;
                    HBRUSH bg;
                    if (ExtraResFClicked) bg = hActiveBrush; else bg = hInactiveBrush;
                    FillRect(lpdis->hDC, &tr, bg);
                    if (ExtraResFClicked) DrawEdge(lpdis->hDC, &tr, EDGE_SUNKEN, BF_RECT);
                    else DrawEdge(lpdis->hDC, &tr, EDGE_RAISED, BF_RECT);
                    tr.top += 5;
                    SetTextColor(lpdis->hDC, GetSysColor(COLOR_BTNTEXT));
                    SetBkMode(lpdis->hDC, TRANSPARENT);
                    DrawTextA(lpdis->hDC, "Display\rExtra\rRes.", -1, &tr, DT_WORDBREAK | DT_CENTER);
                }
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_NEW:
                {
                    Load_TXT_File();
                    InitVariables();
                    UpdateURCounts();
                    Calc_Resize_Frame();
                    Calc_Resize_Sprite();
                    Calc_Resize_Image();
                    Calc_Resize_BG();
                    UpdateFSneeded = true;
                    Update_Toolbar = true;// CreateToolbar();
                    Update_Toolbar2 = true;
                    return TRUE;
                }
                case IDC_ADDTXT:
                {
                    if (MycRom.name[0])
                    {
                        Add_TXT_File();
                        Calc_Resize_Frame();
                        Calc_Resize_Sprite();
                        Calc_Resize_Image();
                        Calc_Resize_BG();
                        UpdateFSneeded = true;
                        Update_Toolbar = true;// CreateToolbar();
                        Update_Toolbar2 = true;
                    }
                    return TRUE;
                }
                case IDC_ORGMODE:
                {
                    if (hPal)
                    {
                        DestroyWindow(hPal);
                        hPal = NULL;
                    }
                    if (hColSet)
                    {
                        DestroyWindow(hColSet);
                        hColSet = NULL;
                    }
                    Edit_Mode = 0;
                    UpdateFSneeded = true;
                    Update_Toolbar = true;// CreateToolbar();
                    glfwSetCursor(glfwframe, glfwarrowcur);
                    SetCursor(hcArrow);
                    Color_Pipette = 0;
                    Paste_Mode = false;
                    Calc_Resize_Frame();
                    return TRUE;
                }
                case IDC_COLMODE:
                {
                    Edit_Mode = 1;
                    Predraw_Frame_For_Rotations(acFrame);
                    Predraw_BG_For_Rotations(acBG);
                    UpdateFSneeded = true;
                    Update_Toolbar = true;// CreateToolbar();
                    Calc_Resize_Frame();
                    return TRUE;
                }
                case IDC_EXTRARES:
                {
                    if (MycRom.name[0] == 0 || MycRom.nFrames == 0 || Paste_Mode) return TRUE;
                    SaveAction(true, SA_ISFRAMEX);
                    MycRom.isExtraFrame[acFrame] = !MycRom.isExtraFrame[acFrame];
                    if (MycRom.isExtraFrame[acFrame] > 0)
                    {
                        ExtraResFClicked = true;
                        nEditExtraResolutionF = true;
                    }
                    else
                    {
                        ExtraResFClicked = false;
                        nEditExtraResolutionF = false;
                    }
                    for (UINT ti = 0; ti < nSelFrames; ti++) MycRom.isExtraFrame[SelFrames[ti]] = MycRom.isExtraFrame[acFrame];
                    UpdateFSneeded = true;
                    FreeCopyMasks();
                    Calc_Resize_Frame();
                    InitColorRotation();
                    InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                    return TRUE;
                }
                case IDC_DISPEXTRA:
                {
                    if (MycRom.name[0] == 0 || MycRom.nFrames == 0 || Paste_Mode) return TRUE;
                    ExtraResFClicked = !ExtraResFClicked;
                    if (MycRom.isExtraFrame[acFrame] > 0 && ExtraResFClicked) nEditExtraResolutionF = true;
                    else nEditExtraResolutionF = false;
                    FreeCopyMasks();
                    UpdateFSneeded = true;
                    //Update_Toolbar = true;
                    Calc_Resize_Frame();
                    Calc_Resize_Image();
                    InitColorRotation();
                    InvalidateRect(hwTB, NULL, TRUE);
                    return TRUE;
                }
                case IDC_SAVE:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (Save_cRom(false, false,(char*)""))
                    {
                        cprintf(false, "%s.cROM saved in %s", MycRom.name, Dir_Serum);
                        if (Save_cRP(false)) cprintf(false, "%s.cRP saved in %s", MycRom.name, Dir_Serum);
                    }
                    return TRUE;
                }
                case IDC_OPEN:
                {
                    char acDir[260],acFile[260];
                    GetCurrentDirectoryA(260, acDir);
                    OPENFILENAMEA ofn;
                    char szFile[260];
                    strcpy_s(szFile, 260, Dir_Serum);
                    strcat_s(szFile, 260, "*.crom");
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.lpstrTitle = "Choose the project file";
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Serum (*.crom)\0*.cROM\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrInitialDir = Dir_Serum;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                    if (GetOpenFileNameA(&ofn) == TRUE)
                    {
                        if (isLoadedProject)
                        {
                            if (MessageBox(hWnd, L"Confirm you want to close the current project and load a new one", L"Caution", MB_YESNO) == IDYES)
                            {
                                Free_Project();
                            }
                            else
                            {
                                SetCurrentDirectoryA(acDir);
                                return TRUE;
                            }
                        }
                        strcpy_s(Dir_Serum, 260, ofn.lpstrFile);
                        int i = (int)strlen(Dir_Serum) - 1;
                        while ((i > 0) && (Dir_Serum[i] != '\\')) i--;
                        Dir_Serum[i + 1] = 0;
                        SavePaths();
                        strcpy_s(acFile, 260, ofn.lpstrFile);
                        if (strstr(acFile, "(auto)") != NULL)
                        {
                            if (MessageBoxA(hWnd, "This cRom was automatically saved, if you manually save it, it will overwrite any previously manually saved file on the same rom, even if it was more recent. Do you still want to open it?", "Caution", MB_YESNO) == IDNO) return TRUE;
                        }
                        if (!Load_cRom(acFile))
                        {
                            Free_Project();
                            return FALSE;
                        }
                        InitVariables();
                        UpdateURCounts();
                        int endst = (int)strlen(acFile);
                        while ((acFile[endst] != '.') && (endst > 0)) endst--;
                        acFile[endst] = 0;
                        strcat_s(acFile,260, ".cRP");
                        if (!Load_cRP(acFile)) Free_Project();
                        else cprintf(false, "The cRom has been loaded.");
                        SetCurrentDirectoryA(acDir);
                        Init_cFrame_Palette2();
                        Calc_Resize_Frame();
                        Calc_Resize_Sprite();
                        Calc_Resize_Image();
                        Calc_Resize_BG();
                        InitCropZoneList();
                        InitColorRotation();
                        UpdateFSneeded = true;
                        UpdateSSneeded = true;
                        Update_Toolbar = true;// CreateToolbar();
                        Update_Toolbar2 = true;
                        UpdateSpriteList3();
                    }
                    return TRUE;
                }
                case IDC_COL1:
                case IDC_COL2:
                case IDC_COL3:
                case IDC_COL4:
                case IDC_COL5:
                case IDC_COL6:
                case IDC_COL7:
                case IDC_COL8:
                case IDC_COL9:
                case IDC_COL10:
                case IDC_COL11:
                case IDC_COL12:
                case IDC_COL13:
                case IDC_COL14:
                case IDC_COL15:
                case IDC_COL16:
                {
                    if (LOWORD(wParam) >= IDC_COL1 + MycRom.noColors) return TRUE;
                    noColMod = noColSel = LOWORD(wParam) - IDC_COL1;
                    InvalidateRect(hwTB, NULL, TRUE);
                    if (MycRom.name[0] == 0) return TRUE;
                    if (hColSet)
                    {
                        DestroyWindow(hColSet);
                        hColSet = NULL;
                    }
                    Choose_Color_Palette(LOWORD(wParam));
                    return TRUE;
                }
                case IDC_COLSET:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (hPal)
                    {
                        DestroyWindow(hPal);
                        hPal = NULL;
                    }
                    ColSetMode = 0;
                    if (hColSet) DestroyWindow(hColSet);
                    hColSet = CreateDialog(hInst, MAKEINTRESOURCE(IDD_COLSET), hDlg, (DLGPROC)ColSet_Proc);
                    if (!hColSet)
                    {
                        AffLastError((char*)"ColSet dialog display");
                        break;
                    }
                    RECT rc;
                    GetWindowRect(GetDlgItem(hwTB, IDC_COLSET), &rc);
                    SetWindowPos(hColSet, 0, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    ShowWindow(hColSet, TRUE);
                    return TRUE;
                }
                case IDC_1COLMODE:
                {
                    MycRP.DrawColMode = 0;
                    EnableDrawButtons();
                    return TRUE;
                }
                case IDC_4COLMODE:
                {
                    MycRP.DrawColMode = 1;
                    EnableDrawButtons();
                    return TRUE;
                }
                case IDC_GRADMODE:
                {
                    MycRP.DrawColMode = 2;
                    if ((MycRP.Draw_Mode != 1) && (MycRP.Draw_Mode != 3) && (MycRP.Draw_Mode != 5))
                        MycRP.Draw_Mode = 1;
                    InvalidateRect(hwTB, NULL, TRUE);
                    DisableDrawButtons();
                    return TRUE;
                }
                case IDC_GRADMODEB:
                {
                    MycRP.DrawColMode = 2;
                    SendMessage(GetDlgItem(hDlg, IDC_4COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_1COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_GRADMODE), BM_SETCHECK, TRUE, 0);
                    if ((MycRP.Draw_Mode != 1) && (MycRP.Draw_Mode != 3) && (MycRP.Draw_Mode != 5)) MycRP.Draw_Mode = 1;
                    InvalidateRect(hwTB, NULL, TRUE);
                    DisableDrawButtons();
                    return TRUE;
                }
                case IDC_UNDO:
                {
                    RecoverAction(true);
                    return TRUE;
                }
                case IDC_REDO:
                {
                    RecoverAction(false); 
                    return TRUE;
                }
                case IDC_DRAWPOINT:
                case IDC_DRAWLINE:
                case IDC_DRAWRECT:
                case IDC_DRAWCIRC:
                case IDC_FILL:
                case IDC_ELLIPSE:
                {
                    MycRP.Draw_Mode = (UINT8)(LOWORD(wParam) - IDC_DRAWPOINT);
                    InvalidateRect(hDlg, NULL, TRUE);
                    return TRUE;
                }
                case IDC_FILLED:
                {
                    if (Button_GetCheck(GetDlgItem(hwTB, IDC_FILLED)) == BST_CHECKED) MycRP.Fill_Mode = TRUE; else MycRP.Fill_Mode = FALSE;
                    return TRUE;
                }
                case IDC_COPYCOLS2:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_DYNACOLOR);
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        if (SelFrames[ti] == acFrame) continue;
                        if (!nEditExtraResolutionF) memcpy(&MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], &MycRom.Dyna4Cols[acFrame * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors * sizeof(UINT16));
                        else memcpy(&MycRom.Dyna4ColsX[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], &MycRom.Dyna4ColsX[acFrame * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], MAX_DYNA_SETS_PER_FRAMEN* MycRom.noColors * sizeof(UINT16));
                    }
                    return TRUE;
                }
                case IDC_COLPICK:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (Color_Pipette > 0) Color_Pipette = 0; else Color_Pipette = 1;
                    Mouse_Mode = 0;
                    if (Color_Pipette > 0)
                    {
                        glfwSetCursor(glfwframe, glfwdropcur);
                        SetCursor(hcColPick);
                    }
                    else
                    {
                        glfwSetCursor(glfwframe, glfwarrowcur);
                        SetCursor(hcArrow);
                    }
                    return TRUE;
                }
                case IDC_POINTMASK:
                {
                    MycRP.Mask_Sel_Mode = 0;
                    InvalidateRect(hDlg, NULL, TRUE);
                    return TRUE;
                }
                case IDC_RECTMASK:
                {
                    MycRP.Mask_Sel_Mode = 1;
                    MouseIniPosx = -1;
                    InvalidateRect(hDlg, NULL, TRUE);
                    return TRUE;
                }
                case IDC_ZONEMASK:
                {
                    MycRP.Mask_Sel_Mode = 2;
                    InvalidateRect(hDlg, NULL, TRUE);
                    return TRUE;
                }
                case IDC_MASKLIST:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    unsigned char acpos = (unsigned char)SendMessage(GetDlgItem(hDlg, IDC_MASKLIST), CB_GETCURSEL, 0, 0) - 1;
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        SaveAction(true, SA_MASKID);
                        if ((acpos != 255) && (acpos + 1 > (UINT8)MycRom.nCompMasks)) MycRom.nCompMasks = acpos + 1;
                        if (nSelFrames > 1)
                        {
                            if (MessageBox(hWnd, L"Change the mask ID for all the selected frames (\"Yes\") or just the current frame (\"No\")", L"Confirmation", MB_YESNO) == IDYES)
                            {
                                for (UINT32 ti = 0; ti < nSelFrames; ti++)
                                    MycRom.CompMaskID[SelFrames[ti]] = acpos;
                                UpdateMaskList();
                                CheckSameFrames();
                                UpdateFSneeded = true;
                                return TRUE;
                            }
                        }
                        MycRom.CompMaskID[acFrame] = acpos;
                        SetDlgItemTextA(hDlg, IDC_MASKLIST, &MycRP.Mask_Names[acpos * SIZE_MASK_NAME]);
                        CheckSameFrames();
                        UpdateFSneeded = true;
                        UpdateMaskList();
                        return TRUE;
                    }
                    return TRUE;
                }
                case IDC_MASKLIST2:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    unsigned char acpos = (unsigned char)SendMessage(GetDlgItem(hDlg, IDC_MASKLIST2), CB_GETCURSEL, 0, 0) - 1;
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        UpdateMaskList2();
                    }
                    return TRUE;
                }
                case IDC_MASKTOSEL:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    UINT nfrfound = 0;
                    HWND hlst = GetDlgItem(hwTB, IDC_MASKLIST2);
                    int acpos = (int)SendMessage(hlst, CB_GETCURSEL, 0, 0);
                    if (acpos == 0) return TRUE;
                    acpos--;
                    SaveAction(true, SA_SELECTION);
                    nSelFrames = 0;
                    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
                    {
                        if (MycRom.CompMaskID[ti] == acpos)
                        {
                            SelFrames[nSelFrames] = ti;
                            nSelFrames++;
                            if (nSelFrames == MAX_SEL_FRAMES) break;
                        }
                    }
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_DYNACOL1:
                case IDC_DYNACOL2:
                case IDC_DYNACOL3:
                case IDC_DYNACOL4:
                case IDC_DYNACOL5:
                case IDC_DYNACOL6:
                case IDC_DYNACOL7:
                case IDC_DYNACOL8:
                case IDC_DYNACOL9:
                case IDC_DYNACOL10:
                case IDC_DYNACOL11:
                case IDC_DYNACOL12:
                case IDC_DYNACOL13:
                case IDC_DYNACOL14:
                case IDC_DYNACOL15:
                case IDC_DYNACOL16:
                {
                    if (LOWORD(wParam) >= IDC_DYNACOL1 + MycRom.noColors) return TRUE;
                    if (MycRom.name[0] == 0) return TRUE;
                    if (hColSet)
                    {
                        DestroyWindow(hColSet);
                        hColSet = NULL;
                    }
                    noColMod = 16 + LOWORD(wParam) - IDC_DYNACOL1;
                    Choose_Color_Palette(LOWORD(wParam));
                    return TRUE;
                }
                case IDC_DELSAMEFR:
                {
                    if ((MycRom.name[0] == 0) || (nSameFrames == 0)) return TRUE;
                    SaveAction(true, SA_FRAMES);
                    int inisfr = nSameFrames;
                    int ti = 0;
                    while (nSameFrames > ti)
                    {
                        Display_Avancement((float)(inisfr - (nSameFrames - ti)) / (float)(nSameFrames - ti), 0, 1);
                        if (isFrameSelected2(SameFrames[ti]) && (SendMessage(GetDlgItem(hDlg, IDC_KEEPSEL), BM_GETCHECK, 0, 0) == BST_CHECKED))
                        {
                            ti++;
                            continue;
                        }
                        Delete_Frame(SameFrames[ti]);
                    }
                    UpdateSectionList();
                    UpdateFSneeded = true;
                    SetDlgItemText(hDlg, IDC_SAMEFRAME, L"0");
                    return TRUE;
                }
                case IDC_DELSELSAMEFR:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    int nDelFrames = 0;
                    SaveAction(true, SA_FRAMES);
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        acFrame = SelFrames[ti];
                        CheckSameFrames();
                        int inisfr = nSameFrames;
                        Display_Avancement((float)ti / (float)nSelFrames, 0, 1);
                        int tj = 0;
                        while (nSameFrames > tj)
                        {
                            if (isFrameSelected2(SameFrames[tj]) && (SendMessage(GetDlgItem(hDlg, IDC_KEEPSEL), BM_GETCHECK, 0, 0) == BST_CHECKED))
                            {
                                tj++;
                                continue;
                            }
                            Delete_Frame(SameFrames[tj]);
                        }
                        acFrame++;
                    }
                    if (!isFrameSelected2(acFrame))
                    {
                        nSelFrames = 1;
                        SetMultiWarningF();
                        SelFrames[0] = acFrame;
                    }
                    if (acFrame >= MycRom.nFrames) acFrame = MycRom.nFrames - 1;
                    UpdateNewacFrame();
                    UpdateSectionList();
                    UpdateFSneeded = true;
                    acFrame = SelFrames[0];
                    InitColorRotation();
                    SetMultiWarningF();
                    SetDlgItemText(hDlg, IDC_SAMEFRAME, L"0");
                    cprintf(false, "%i frames were deleted as duplicate frames using the comparison masks", nDelFrames);
                    return TRUE;
                }
                case IDC_DELALLSAMEFR:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    //if (MessageBox(hDlg, L"CAUTION!!!!! This action can't be undone, are you sure you want to delete all the similar frames?\r\nBe aware that if your comparison masks are not correctly/accurately drawn, you may lost some frames that you still need.", L"Confirm", MB_YESNO) == IDYES)
                    {
                        SaveAction(true, SA_FRAMES);
                        acFrame = 0;
                        int nDelFrames = 0;
                        while (acFrame < MycRom.nFrames)
                        {
                            CheckSameFrames();
                            int inisfr = nSameFrames;
                            Display_Avancement((float)acFrame / (float)MycRom.nFrames, 0, 1);
                            int tj = 0;
                            while (nSameFrames > tj)
                            {
                                if (isFrameSelected2(SameFrames[tj]) && (SendMessage(GetDlgItem(hDlg, IDC_KEEPSEL), BM_GETCHECK, 0, 0) == BST_CHECKED))
                                {
                                    tj++;
                                    continue;
                                }
                                Delete_Frame(SameFrames[tj]);
                            }
                            acFrame++;
                        }
                        if (!isFrameSelected2(acFrame))
                        {
                            nSelFrames = 1;
                            SetMultiWarningF();
                            SelFrames[0] = acFrame;
                        }
                        if (acFrame >= MycRom.nFrames) acFrame = MycRom.nFrames - 1;
                        UpdateNewacFrame();
                        UpdateSectionList();
                        UpdateFSneeded = true;
                        acFrame = 0;
                        prevAcFrame = (UINT)-1;
                        InitColorRotation();
                        SetDlgItemText(hDlg, IDC_SAMEFRAME, L"0");
                        cprintf(false, "%i frames were deleted as duplicate frames using the comparison masks", nDelFrames);
                    }
                    return TRUE;
                }
                case IDC_DELFRAME:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    //if (MessageBox(hDlg, L"This action can't be undone, are you sure you want to delete those frames?", L"Confirm", MB_YESNO) == IDYES)
                    {
                        SaveAction(true, SA_FRAMES);
                        int inisfr = nSelFrames;
                        while (nSelFrames > 0)
                        {
                            Display_Avancement((float)(inisfr - nSelFrames) / (float)nSelFrames,0,1);
                            Delete_Frame(SelFrames[0]);
                        }
                        UpdateSectionList();
                        UpdateFSneeded = true;
                        acFrame = PreFrameInStrip;
                        UpdateNewacFrame();
                        InitColorRotation();
                    }
                    return TRUE;
                }
                case IDC_DELDURFRAME:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    //if (MessageBox(hDlg, L"This action can't be undone, are you sure you want to delete those frames?", L"Confirm", MB_YESNO) == IDYES)
                    {
                        SaveAction(true, SA_FRAMES);
                        int inisfr = nSelFrames;
                        char tbuf[8];
                        GetDlgItemTextA(hwTB, IDC_DELFRMS, tbuf, 8);
                        UINT durmax = (UINT)atoi(tbuf);
                        if (durmax < 0) return TRUE;
                        int acfrm = 0;
                        while ((int)nSelFrames > acfrm)
                        {
                            Display_Avancement((float)(inisfr - nSelFrames) / (float)nSelFrames, 0, 1);
                            if (MycRP.FrameDuration[SelFrames[acfrm]] < durmax)
                                Delete_Frame(SelFrames[acfrm]);
                            else acfrm++;
                        }
                        UpdateSectionList();
                        UpdateFSneeded = true;
                        acFrame = PreFrameInStrip;
                        UpdateNewacFrame();
                        InitColorRotation();
                    }
                    return TRUE;
                }
                case IDC_SHAPEMODE:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_SHAPEMODE);
                    if (SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        for (unsigned int ti = 0; ti < nSelFrames; ti++) MycRom.ShapeCompMode[SelFrames[ti]] = TRUE;
                    }
                    else
                    {
                        for (unsigned int ti = 0; ti < nSelFrames; ti++) MycRom.ShapeCompMode[SelFrames[ti]] = FALSE;
                    }
                    CheckSameFrames();
                    return TRUE;
                }
                case IDC_COPY:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_COPYMASK);
                    bool datafound = false;
                    UINT fw, fh;
                    UINT16* pcfr;
                    UINT8* pdynm;
                    if (nEditExtraResolutionF)
                    {
                        if (Copy_Content != 2)
                        {
                            MessageBoxA(hwTB, "There is no extra resolution content in the selection, you can't copy in extra resolution mode.", "Failed", MB_OK);
                            return TRUE;
                        }
                        fw = MycRom.fWidthX;
                        fh = MycRom.fHeightX;
                        pcfr = &MycRom.cFramesX[acFrame * fw * fh];
                        pdynm = &MycRom.DynaMasksX[acFrame * fw * fh];
                        Paste_Content = 2;
                    }
                    else
                    {
                        if (Copy_Content != 1)
                        {
                            MessageBoxA(hwTB, "There is no original resolution content in the selection, you can't copy in original resolution mode.", "Failed", MB_OK);
                            return TRUE;
                        }
                        fw = MycRom.fWidth;
                        fh = MycRom.fHeight;
                        pcfr = &MycRom.cFrames[acFrame * fw * fh];
                        pdynm = &MycRom.DynaMasks[acFrame * fw * fh];
                        Paste_Content = 1;
                    }
                    int cminx = fw, cmaxx = -1;
                    int cminy = fh, cmaxy = -1;
                    for (int tj = 0; tj < (int)fh; tj++)
                    {
                        for (int ti = 0; ti < (int)fw; ti++)
                        {
                            if (Copy_Mask[ti + tj * fw] > 0)
                            {
                                Copy_ColN[ti + tj * fw] = pcfr[ti + tj * fw];
                                if (!nEditExtraResolutionF)
                                    Copy_Colo[ti + tj * MycRom.fWidth] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti + tj * MycRom.fWidth];
                                else
                                {
                                    if (fh == 64) Copy_Colo[ti / 2 + tj / 2 * MycRom.fWidth] =
                                        MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti / 2 + tj / 2 * MycRom.fWidth];
                                    else
                                    {
                                        Copy_Colo[ti * 2 + tj * 2 * MycRom.fWidth] =
                                            MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti * 2 + tj * 2 * MycRom.fWidth];
                                        Copy_Colo[ti * 2 + 1 + tj * 2 * MycRom.fWidth] =
                                            MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti * 2 + 1 + tj * 2 * MycRom.fWidth];
                                        Copy_Colo[ti * 2 + (tj * 2 + 1) * MycRom.fWidth] =
                                            MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti * 2 + (tj * 2 + 1) * MycRom.fWidth];
                                        Copy_Colo[ti * 2 + 1 + (tj * 2 + 1) * MycRom.fWidth] =
                                            MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti * 2 + 1 + (tj * 2 + 1) * MycRom.fWidth];
                                    }
                                }
                                Copy_Dyna[ti + tj * fw] = pdynm[ti + tj * fw];
                                datafound = true;
                                if (ti > cmaxx) cmaxx = ti;
                                if (ti < cminx) cminx = ti;
                                if (tj > cmaxy) cmaxy = tj;
                                if (tj < cminy) cminy = tj;
                            }
                        }
                    }
                    if (!datafound) return TRUE;
                    Paste_Width = cmaxx - cminx + 1;
                    Paste_Height = cmaxy - cminy + 1;
                    for (int tj = cminy; tj <= cmaxy; tj++)
                    {
                        for (int ti = cminx; ti <= cmaxx; ti++)
                        {
                            Paste_Mask[ti - cminx + (tj - cminy) * Paste_Width] = Copy_Mask[ti + tj * fw];
                            Paste_ColN[ti - cminx + (tj - cminy) * Paste_Width] = Copy_ColN[ti + tj * fw];
                            if (!nEditExtraResolutionF)
                                Paste_Colo[ti - cminx + (tj - cminy) * Paste_Width] = Copy_Colo[ti + tj * MycRom.fWidth];
                            else
                            {
                                if (fh == 64) Paste_Colo[(ti - cminx)/2 + (tj - cminy)/2 * Paste_Width] = Copy_Colo[ti/2 + tj/2 * MycRom.fWidth];
                                else
                                {
                                    Paste_Colo[(ti - cminx) * 2 + (tj - cminy) * 2 * Paste_Width] = Copy_Colo[ti * 2 + tj * 2 * MycRom.fWidth];
                                    Paste_Colo[(ti - cminx) * 2 + 1 + (tj - cminy) * 2 * Paste_Width] = Copy_Colo[ti * 2 + 1 + tj * 2 * MycRom.fWidth];
                                    Paste_Colo[(ti - cminx) * 2 + (tj - cminy + 1) * 2 * Paste_Width] = Copy_Colo[ti * 2 + (tj * 2 + 1) * MycRom.fWidth];
                                    Paste_Colo[(ti - cminx) * 2 + 1 + (tj - cminy + 1) * 2 * Paste_Width] = Copy_Colo[ti * 2 + 1 + (tj * 2 + 1) * MycRom.fWidth];
                                }
                            }
                            Paste_Dyna[ti - cminx + (tj - cminy) * Paste_Width] = Copy_Dyna[ti + tj * fw];
                        }
                    }
                    Copy_Available = true;
                    Copy_From_Frame = acFrame;
                    Copy_From_DynaMask = acDynaSet;
                    BlocPause = false;
                    EnableWindow(GetDlgItem(hwTB, IDC_PASTE), TRUE);
                    EnableWindow(GetDlgItem(hwTB2, IDC_PASTE), TRUE);
                    return TRUE;
                }
                case IDC_PASTE:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (Paste_Mode) Paste_Mode = false; else Paste_Mode = true;
                    Mouse_Mode = 0;
                    if (Paste_Mode)
                    {
                        glfwSetCursor(glfwframe, glfwpastecur);
                        SetCursor(hcPaste);
                    }
                    else
                    {
                        glfwSetCursor(glfwframe, glfwarrowcur);
                        SetCursor(hcArrow);
                        return TRUE;
                    }
                    Paste_Mirror = 0;
                    if (GetKeyState('K') & 0x8000) Paste_Mirror |= 1;
                    if (GetKeyState('L') & 0x8000) Paste_Mirror |= 2;
                    int xmin = 257, xmax = -1, ymin = 65, ymax = -1;
                    for (int tj = 0; tj < (int)Paste_Height; tj++)
                    {
                        for (int ti = 0; ti < (int)Paste_Width; ti++)
                        {
                            if (Paste_Mask[ti + tj * Paste_Width] == 0) continue;
                            if (ti < xmin) xmin = ti;
                            if (ti > xmax) xmax = ti;
                            if (tj < ymin) ymin = tj;
                            if (tj > ymax) ymax = tj;
                        }
                    }
                    /*int tx, ty;
                    glfwGetWindowPos(glfwframe, &tx, &ty);
                    if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                    {
                        paste_centerx = (int)(tx + 2 * frame_zoom * (xmax + xmin) / 2);
                        paste_centery = (int)(ty + 2 * frame_zoom * (ymax + ymin) / 2);
                    }
                    else
                    {
                        paste_centerx = (int)(tx + frame_zoom * (xmax + xmin) / 2);
                        paste_centery = (int)(ty + frame_zoom * (ymax + ymin) / 2);
                    }*/
                    return TRUE;
                }
                case IDC_ADDSECTION:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    int tj = is_Section_First(acFrame);
                    char tbuf[SIZE_SECTION_NAMES];
                    GetDlgItemTextA(hwTB, IDC_SECTIONNAME, tbuf, 31);
                    tbuf[31] = 0;
                    int tnampos = Duplicate_Section_Name(tbuf);
                    if (tnampos > -1)
                    {
                        MessageBoxA(hWnd, "This name is already used for another section, action ignored.", "Caution", MB_OK);
                        return TRUE;
                    }
                    SaveAction(true, SA_SECTIONS);
                    if (tj > -1)
                    {
                        strcpy_s(&MycRP.Section_Names[SIZE_SECTION_NAMES * tj], SIZE_SECTION_NAMES, tbuf);
                    }
                    else
                    {
                        MycRP.Section_Firsts[MycRP.nSections] = acFrame;
                        strcpy_s(&MycRP.Section_Names[SIZE_SECTION_NAMES * MycRP.nSections], SIZE_SECTION_NAMES, tbuf);
                        MycRP.nSections++;
                    }
                    UpdateSectionList();
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_DELSECTION:
                {
                    int ti = Which_Section(acFrame);
                    if (ti == -1) return TRUE;
                    char tbuf[256];
                    sprintf_s(tbuf, 256, "Confirm you want to delete the section \"%s\" this frame is part of?", &MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
                    if (MessageBoxA(hwTB, tbuf, "Confirm", MB_YESNO) == IDYES)
                    {
                        SaveAction(true, SA_SECTIONS);
                        Delete_Section(ti);
                        UpdateSectionList();
                    }
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_MOVESECTION:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    RECT rc;
                    GetWindowRect(GetDlgItem(hwTB, IDC_MOVESECTION), &rc);
                    hMovSec = CreateWindowEx(0, L"MovSection", L"", WS_POPUP, rc.left-50, rc.top, 300, 600, NULL, NULL, hInst, NULL);       // Parent window.
                    if (!hMovSec)
                    {
                        AffLastError((char*)"Create Move Section Window");
                        return TRUE;
                    }
                    ShowWindow(hMovSec, true);
                    return TRUE;
                }
                case IDC_SECTIONLIST:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    unsigned char acpos = (unsigned char)SendMessage(GetDlgItem(hDlg, IDC_SECTIONLIST), CB_GETCURSEL, 0, 0) - 1;
                    if ((HIWORD(wParam) == CBN_SELCHANGE) && (acpos < MAX_SECTIONS - 1))
                    {
                        SaveAction(true, SA_SECTIONS);
                        PreFrameInStrip = MycRP.Section_Firsts[acpos];
                        UpdateFSneeded = true;
                        SetDlgItemTextA(hwTB, IDC_SECTIONNAME, "");
                    }
                    return TRUE;
                }
                case IDC_COLTODYNA:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true,SA_DYNACOLOR);
                    for (UINT tj = 0; tj < nSelFrames; tj++)
                    {
                        for (UINT ti = 0; ti < MycRom.noColors; ti++)
                        {
                            if (nEditExtraResolutionF)
                                MycRom.Dyna4ColsX[(SelFrames[tj] * MAX_DYNA_SETS_PER_FRAMEN + acDynaSet) * MycRom.noColors + ti] = MycRP.acEditColorsS[ti];
                            else
                                MycRom.Dyna4Cols[(SelFrames[tj] * MAX_DYNA_SETS_PER_FRAMEN + acDynaSet) * MycRom.noColors + ti] = MycRP.acEditColorsS[ti];
                        }
                    }
                    for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                    return TRUE;
                }
                case IDC_INVERTSEL:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_COMPMASK);
                    UINT nomsk = (UINT)MycRom.CompMaskID[acFrame];
                    if (nomsk < 255)
                    {
                        nomsk *= MycRom.fWidth * MycRom.fHeight;
                        for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
                        {
                            if (MycRom.CompMasks[nomsk + ti] == 0) MycRom.CompMasks[nomsk + ti] = 1; else MycRom.CompMasks[nomsk + ti] = 0;
                        }
                    }
                    CheckSameFrames();
                    return TRUE;
                }
                case IDC_INVERTSEL2:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_COPYMASK);
                    for (UINT ti = 0; ti < 64 * 256; ti++)
                    {
                        if (Copy_Mask[ti] == 0) Copy_Mask[ti] = 1; else Copy_Mask[ti] = 0;
                    }
                    GetSelectionSize();
                    return TRUE;
                }
                case IDC_ADDSPRITE2:
                {
                    if (MycRom.name[07] == 0) return TRUE;
                    if (NSelection > 0)
                    {
                        if (MessageBoxA(hWnd, "There is a selection in the frame, it will be used as a bounding box for the sprite. Do you confirm?", "Confirm", MB_YESNO) == IDNO) return TRUE;
                    }
                    SaveAction(true, SA_FRAMESPRITES);
                    int notodisplay = 0;
                    for (UINT tj = 0; tj < nSelFrames; tj++)
                    {
                        UINT ti;
                        for (ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
                        {
                            if (MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] == acSprite) break; 
                        }
                        if (ti == MAX_SPRITES_PER_FRAME)
                        {
                            ti = 0;
                            while ((MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] != 255) && (ti < MAX_SPRITES_PER_FRAME)) ti++;
                        }
                        if (ti == MAX_SPRITES_PER_FRAME)
                        {
                            MessageBoxA(hwTB2, "One of the selected frames has already reach the maximum number of sprites. Action canceled.", "Failed", MB_OK);
                        }
                        else
                        {
                            MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] = acSprite;
                            if (NSelection > 0)
                            {
                                if (nEditExtraResolutionF)
                                {
                                    if (MycRom.fHeightX == 64)
                                    {
                                        MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = XSelection / 2;
                                        MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = YSelection / 2;
                                        MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = (XSelection + WSelection - 1) / 2;
                                        MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = (YSelection + HSelection - 1) / 2;
                                    }
                                    else
                                    {
                                        MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = XSelection * 2;
                                        MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = YSelection * 2;
                                        MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = (XSelection + WSelection - 1) * 2 + 1;
                                        MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = (YSelection + HSelection - 1) * 2 + 1;
                                    }
                                }
                                else
                                {
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = XSelection;
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = YSelection;
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = XSelection + WSelection - 1;
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = YSelection + HSelection - 1;
                                }
                            }
                            else
                            {
                                MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = 0;
                                MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = 0;
                                MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = MycRom.fWidth - 1;
                                MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = MycRom.fHeight - 1;
                            }
                            if (SelFrames[tj] == acFrame) notodisplay = ti;
                        }
                    }
                    UpdateFrameSpriteList();
                    UpdateSpriteList3();
                    SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_SETCURSEL, notodisplay, 0);
                    return TRUE;
                }
                case IDC_DELSPRITE2:
                {
                    int acpos = (int)SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_GETCURSEL, 0, 0);
                    if (acpos == -1) return TRUE;
                    int acsprite = MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + acpos];
                    //if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
                    SaveAction(true, SA_FRAMESPRITES);
                    for (UINT tj = 0; tj < nSelFrames; tj++)
                    {
                        acpos = -1;
                        for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
                        {
                            if (MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] == acsprite) acpos = ti;
                        }
                        if (acpos == -1) continue;
                        for (UINT ti = acpos; ti < MAX_SPRITES_PER_FRAME - 1; ti++)
                        {
                            MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] = MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti + 1];
                            MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + (ti + 1) * 4];
                            MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + (ti + 1) * 4 + 1];
                            MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + (ti + 1) * 4 + 2];
                            MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + (ti + 1) * 4 + 3];
                        }
                        MycRom.FrameSprites[(SelFrames[tj] + 1) * MAX_SPRITES_PER_FRAME - 1] = 255;
                    }
                    UpdateFrameSpriteList();
                    UpdateSpriteList3();
                    return TRUE;
                }
                case IDC_COLROT:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (hPal)
                    {
                        DestroyWindow(hPal);
                        hPal = NULL;
                    }
                    if (hColSet) DestroyWindow(hColSet);
                    ColSetMode = 1;
                    hColSet = CreateDialog(hInst, MAKEINTRESOURCE(IDD_COLSET), hDlg, (DLGPROC)ColSet_Proc);
                    if (!hColSet)
                    {
                        AffLastError((char*)"ColSet dialog display");
                        break;
                    }
                    RECT rc;
                    GetWindowRect(GetDlgItem(hwTB, IDC_COLROT), &rc);
                    SetWindowPos(hColSet, 0, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    ShowWindow(hColSet, TRUE);
                    return TRUE;
                }
                case IDC_FRUSEMASK:
                {
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        int acpos = (int)SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                        if (acpos >= 0)
                        {
                            char tbuf[16];
                            SendMessageA((HWND)lParam, LB_GETTEXT, acpos, (LPARAM)tbuf);
                            PreFrameInStrip = atoi(tbuf);
                            UpdateFSneeded = true;
                        }
                    }
                    return TRUE;
                }
                case IDC_IDENT:
                {
                    if (Ident_Pushed) Ident_Pushed = false;
                    else Ident_Pushed = true;
                    SetSpotButton(Ident_Pushed);
                    return TRUE;
                }
                case IDC_BBSPOT:
                {
                    if (BBIdent_Pushed) BBIdent_Pushed = false;
                    else BBIdent_Pushed = true;
                    SetBackgroundMaskSpotButton(BBIdent_Pushed);
                    return TRUE;
                }
                case IDC_COMMON:
                {
                    if (Common_Pushed) Common_Pushed = false;
                    else
                    {
                        Common_Pushed = true;
                        Check_Commons();
                    }
                    SetCommonButton(Common_Pushed);
                    return TRUE;
                }
                case IDC_ZOOM2X:
                {
                    if (MycRom.name[0] == 0 || MycRom.nFrames == 0 || Paste_Mode) return TRUE;
                    if (Zoom_Pushed) Zoom_Pushed = false;
                    else Zoom_Pushed = true;
                    SetZoomButton(Zoom_Pushed);
                    return TRUE;
                }
                case IDC_DELTID:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    MycRom.TriggerID[acFrame] = 0xFFFFFFFF;
                    UpdateTriggerID();
                    return TRUE;
                }
                case IDC_TRIGID:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        char tbuf[256];
                        GetWindowTextA((HWND)lParam, tbuf, 256);
                        if (strcmp(tbuf, "- None -") == 0) return TRUE;
                        int tID = atoi(tbuf);
                        if (tID < 0)
                        {
                            tID = 0;
                            strcpy_s(tbuf, 256, "0");
                            SetWindowTextA((HWND)lParam, tbuf);
                        }
                        if (tID >= 65536)
                        {
                            tID = 65535;
                            strcpy_s(tbuf, 256, "65535");
                            SetWindowTextA((HWND)lParam, tbuf);
                        }
                        SaveAction(true, SA_TRIGGERID);
                        MycRom.TriggerID[acFrame] = tID;
                    }
                    return TRUE;
                }
                case IDC_NIGHTDAY:
                {
                    HBRUSH brush;
                    if (!Night_Mode)
                    {
                        brush = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
                        Night_Mode = true;
                    }
                    else
                    {
                        brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
                        Night_Mode = false;
                    }
                    SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hwTB, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hSprites, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hwTB2, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hImages, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hwTB3, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hBG, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hwTB4, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    InvalidateRect(hWnd, NULL, TRUE);
                    InvalidateRect(hwTB, NULL, TRUE);
                    InvalidateRect(hSprites, NULL, TRUE);
                    InvalidateRect(hwTB2, NULL, TRUE);
                    InvalidateRect(hImages, NULL, TRUE);
                    InvalidateRect(hwTB3, NULL, TRUE);
                    InvalidateRect(hBG, NULL, TRUE);
                    InvalidateRect(hwTB4, NULL, TRUE);
                    return TRUE;
                }
                case IDC_SCROLLFILL:
                {
                    SaveAction(true, SA_DRAW);
                    AutoFillScrolling();
                    return TRUE;
                }
                case IDC_GENAGIF:
                {
                    SaveAnimatedGif();
                    return TRUE;
                }
                case IDC_FRAMETEST:
                {
                    TestSelectedFrames();
                    return TRUE;
                }
                case IDC_DELBG:
                {
                    SaveAction(true, SA_DELFRAMEBG);
                    for (UINT ti = 0; ti < nSelFrames; ti++) MycRom.BackgroundID[SelFrames[ti]] = 0xffff;
                    UpdateFrameBG();
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_ADDBG:
                {
                    SetBackground();
                    UpdateFrameBG();
                    UpdateFSneeded = true;
                    return FALSE;
                }
                case IDC_SETDUR:
                {
                    char tbuf[8];
                    SaveAction(true, SA_DURATION);
                    GetDlgItemTextA(hwTB, IDC_DURATION, tbuf, 8);
                    int val = atoi(tbuf);
                    bool modif = false;
                    if (val < 5)
                    {
                        modif = true;
                        val = 5;
                    }
                    else if (val > 3000)
                    {
                        modif = true;
                        val = 3000;
                    }
                    if (modif)
                    {
                        _itoa_s(val, tbuf, 8, 10);
                        SetDlgItemTextA(hwTB, IDC_DURATION, tbuf);
                        MessageBoxA(hWnd, "The value was not correct (must be between 5 and 3000), it has been changed, review it and confirm again", "Error", MB_OK);
                        return TRUE;
                    }
                    for (UINT ti = 0; ti < nSelFrames; ti++) MycRP.FrameDuration[SelFrames[ti]] = (UINT32)val;
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_AUTOCOPY:
                {
                    AutoCopy();
                    return TRUE;
                }
                case IDC_UPDATESAME:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SetDlgItemTextA(hwTB, IDC_UPDATESAME, "Please Wait");
                    Check_SameFrames_Masks_All();
                    SetDlgItemTextA(hwTB, IDC_UPDATESAME, "Update Same List");
                    AllSameFramesUpdated = true;
                    if (Edit_Mode == 0) InvalidateRect(GetDlgItem(hwTB, IDC_MASKLIST), NULL, FALSE);
                    return TRUE;
                }
                case IDC_SAMEFRAMELIST:
                {
                    int idx = (int)SendMessageA(GetDlgItem(hwTB, IDC_SAMEFRAMELIST), CB_GETCURSEL, 0, 0);
                    if (idx == -1) return TRUE;
                    char tbuf[16];
                    SendMessageA(GetDlgItem(hwTB, IDC_SAMEFRAMELIST), CB_GETLBTEXT, idx, (LPARAM)tbuf);
                    int nofr = atoi(tbuf);
                    PreFrameInStrip = nofr;
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_COPYDYNA:
                {
                    if (nSelFrames < 2) return TRUE;
                    UINT fwmfh;
                    UINT16* pdync;
                    UINT8* pdynm;
                    if (nEditExtraResolutionF)
                    {
                        fwmfh = MycRom.fWidthX * MycRom.fHeightX;
                        pdync = MycRom.Dyna4ColsX;
                        pdynm = MycRom.DynaMasksX;
                    }
                    else
                    {
                        fwmfh = MycRom.fWidth * MycRom.fHeight;
                        pdync = MycRom.Dyna4Cols;
                        pdynm = MycRom.DynaMasks;
                    }
                    SaveAction(true, SA_DYNAALL);
                    for (uint ti = 0; ti < nSelFrames; ti++)
                    {
                        if (SelFrames[ti] == acFrame) continue;
                        if (nEditExtraResolutionF && MycRom.isExtraFrame[SelFrames[ti]] == 0) continue;
                        memcpy(&pdync[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], &pdync[acFrame * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], MAX_DYNA_SETS_PER_FRAMEN* MycRom.noColors);
                        memcpy(&pdynm[SelFrames[ti] * fwmfh], &pdynm[acFrame * fwmfh], fwmfh);
                    }
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_OUTL:
                case IDC_OUTBL:
                case IDC_OUTB:
                case IDC_OUTBR:
                case IDC_OUTR:
                case IDC_OUTTR:
                case IDC_OUTT:
                case IDC_OUTTL:
                case IDC_OUTA:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (NSelection == 0) return TRUE;
                    SaveAction(true, SA_DRAW);
                    ShadowSelection(LOWORD(wParam));
                    return TRUE;
                }
                case IDC_ORGTOXTRA2:
                {
                    if (MycRom.name[0] == 0 || MycRom.nFrames == 0 || Paste_Mode) return TRUE;
                    SaveAction(true, SA_ISFRAMEX);
                    SaveAction(true, SA_FULLDRAW);
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        ResizeRGB565Image(&MycRom.cFramesX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX, MycRom.fHeightX, &MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth, MycRom.fHeight, FrameResizeFilter);
                        memcpy(&MycRom.Dyna4ColsX[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], &MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
                        memcpy(&MycRom.ColorRotationsX[SelFrames[ti] * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION], &MycRom.ColorRotations[SelFrames[ti] * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION], sizeof(UINT16) * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
                        ResizeDynaMask(&MycRom.DynaMasksX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX, MycRom.fHeightX, &MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fHeight == 64);
                        ResizeBGMask(&MycRom.BackgroundMaskX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX, MycRom.fHeightX, &MycRom.BackgroundMask[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fHeight == 64);
                        MycRom.isExtraFrame[SelFrames[ti]] = 1;
                    }
                    ExtraResFClicked = true;
                    InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                    nEditExtraResolutionF = true;
                    CheckDlgButton(hwTB, IDC_EXTRARES, BST_CHECKED);
                    UpdateFSneeded = true;
                    Calc_Resize_Frame();
                    FreeCopyMasks();
                    InitColorRotation();
                    return TRUE;
                }
                case IDC_XTRATOORG2:
                {
                    if (MycRom.name[0] == 0 || Paste_Mode || MycRom.isExtraFrame[acFrame] == 0) return TRUE;
                    SaveAction(true, SA_FULLDRAW);                    
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        if (MycRom.isExtraFrame[SelFrames[ti]] == 0) continue;
                        ResizeRGB565Image(&MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth, MycRom.fHeight, &MycRom.cFramesX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX, MycRom.fHeightX, FrameResizeFilter);
                        memcpy(&MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], &MycRom.Dyna4ColsX[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors], sizeof(UINT16) * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors);
                        memcpy(&MycRom.ColorRotations[SelFrames[ti] * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION], &MycRom.ColorRotationsX[SelFrames[ti] * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION], sizeof(UINT16) * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
                        ResizeDynaMask(&MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth, MycRom.fHeight, &MycRom.DynaMasksX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fHeightX == 64);
                        ResizeBGMask(&MycRom.BackgroundMask[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth, MycRom.fHeight, &MycRom.BackgroundMaskX[SelFrames[ti] * MycRom.fWidthX * MycRom.fHeightX], MycRom.fHeightX == 64);
                    }
                    ExtraResFClicked = false;
                    InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                    nEditExtraResolutionF = false;
                    UpdateFSneeded = true;
                    Calc_Resize_Frame();
                    InitColorRotation();
                    FreeCopyMasks();
                    return TRUE;
                }
                case IDC_FILTERTYPE:
                {
                    int nofilter = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                    switch (nofilter)
                    {
                    case 0:
                        FrameResizeFilter = cv::INTER_NEAREST;
                        break;
                    case 1:
                        FrameResizeFilter = cv::INTER_LINEAR;
                        break;
                    case 2:
                        FrameResizeFilter = cv::INTER_CUBIC;
                        break;
                    case 3:
                        FrameResizeFilter = cv::INTER_LANCZOS4;
                        break;
                    }
                    return TRUE;
                }
                case IDC_BAW:
                {
                    if (MessageBoxA(hwTB, "This action is NOT undo-able. The undo stack will be emptied. Always backup your colored project before doing this. Proceed?", "Caution:", MB_YESNO) == IDYES)
                        FullGrayConvert();
                    return TRUE;
                }
                case IDC_CFTOOF:
                {
                    ConvertCFrameToNewOFrame();
                    return TRUE;
                }
            }
            break;
        }
        case WM_SETCURSOR:
        {
            if (Paste_Mode) SetCursor(hcPaste);
            else if (Color_Pipette > 0) SetCursor(hcColPick);
            else DefWindowProc(hDlg, message, wParam, lParam);
            return TRUE;
        }
        case WM_NOTIFY:
        {
            UINT nCode = ((LPNMHDR)lParam)->code;

            switch (nCode)
            {
                case UDN_DELTAPOS:
                {
                    LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;
                    if (lpnmud->hdr.hwndFrom == GetDlgItem(hwTB, IDC_CHANGECOLSET))
                    {
                        if ((lpnmud->iDelta < 0) && (acDynaSet > 0)) acDynaSet--;
                        else if ((lpnmud->iDelta > 0) && (acDynaSet < MAX_DYNA_SETS_PER_FRAMEN - 1)) acDynaSet++;
                        char tbuf[10];
                        _itoa_s(acDynaSet + 1, tbuf, 8, 10);
                        SetDlgItemTextA(hwTB, IDC_NOSETCOL, tbuf);
                        for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                        return TRUE;
                    }
                    else if (lpnmud->hdr.hwndFrom == GetDlgItem(hwTB, IDC_LUMSPIN))
                    {
                        SaveAction(true, SA_DRAW);
                        if (lpnmud->iDelta < 0) ApplyBrightness(1.10f);
                        else if (lpnmud->iDelta > 0)ApplyBrightness(0.90f);
                        UpdateFSneeded = true;
                    }
                    break;
                }
            }
        }
    }
    return (INT_PTR)FALSE;
}

const char* ButtonDescription2(HWND hOver)
{
    if (hOver == GetDlgItem(hwTB2, IDC_SAVE)) return (const char*)"Save the Serum project";
    for (int ti = 0; ti < 16; ti++)
    {
        if (hOver == GetDlgItem(hwTB2, IDC_COL1 + ti))
        {
            char tbuf[512];
            sprintf_s(tbuf, 512, "Select the color #%i for drawing in the 64-colour palette", ti + 1);
            return tbuf;
        }
    }
    if (hOver == GetDlgItem(hwTB2, IDC_SPRITENAME)) return (const char*)"Enter the name for a new sprite here";
    if (hOver == GetDlgItem(hwTB2, IDC_SPRITELIST)) return (const char*)"List of all the sprites";
    if (hOver == GetDlgItem(hwTB2, IDC_ADDSPRITE)) return (const char*)"Add a sprite named after the text in the box beside";
    if (hOver == GetDlgItem(hwTB2, IDC_DELSPRITE)) return (const char*)"Delete the sprite";
    if (hOver == GetDlgItem(hwTB2, IDC_UNDO)) return (const char*)"Undo last action";
    if (hOver == GetDlgItem(hwTB2, IDC_REDO)) return (const char*)"Redo last action";
    if (hOver == GetDlgItem(hwTB2, IDC_COPY)) return (const char*)"Copy the sprite in the program clipboard";
    if (hOver == GetDlgItem(hwTB2, IDC_PASTE)) return (const char*)"Paste the selection from the frame as the selected sprite";
    if (hOver == GetDlgItem(hwTB2, IDC_DRAWPOINT)) return (const char*)"Point tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_DRAWLINE)) return (const char*)"Line tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_DRAWRECT)) return (const char*)"Rectangle tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_DRAWCIRC)) return (const char*)"Circle tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_FILL)) return (const char*)"Magic wand tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_FILLED)) return (const char*)"Draw rectangles or circles are filled or not?";
    if (hOver == GetDlgItem(hwTB2, IDC_ADDTOFRAME)) return (const char*)"Add this sprite to the list of sprites to be detected to the current displayed frame";
    if (hOver == GetDlgItem(hwTB2, IDC_TOFRAME)) return (const char*)"Display the frame this sprite has been extracted from in the colorization window (will be wrong if this frame has been deleted)";
    if (hOver == GetDlgItem(hwTB2, IDC_DETSPR)) return (const char*)"Select the detection zone to draw";
    if (hOver == GetDlgItem(hwTB2, IDC_DELDETSPR)) return (const char*)"Delete this detection zone from the sprite";
    if (hOver == GetDlgItem(hwTB2, IDC_SPRITELIST2)) return (const char*)"Select a sprite to show the frames using it below";
    if (hOver == GetDlgItem(hwTB2, IDC_FRUSESPRITE)) return (const char*)"List of frames using the sprite selected above";
    return "";
}

LRESULT CALLBACK ButtonSubclassProc2(HWND hBut, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (message) {
        case WM_KEYDOWN:
        {
            return TRUE;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hBut;
            tme.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&tme);
            SetWindowTextA(hStatus2, ButtonDescription2(hBut));
            break;
        }
        case WM_MOUSELEAVE:
        {
            SetWindowTextA(hStatus2, "");
            break;
        }
    }

    return DefSubclassProc(hBut, message, wParam, lParam);
}
INT_PTR CALLBACK Toolbar_Proc2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        SetWindowSubclass(GetDlgItem(hDlg, IDC_SAVE), ButtonSubclassProc2, 0, 0);
        for (int ti = 0; ti < 16; ti++) SetWindowSubclass(GetDlgItem(hDlg, IDC_COL1 + ti), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_SPRITENAME), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_SPRITELIST), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDSPRITE), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSPRITE), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_UNDO), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_REDO), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_COPY), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_PASTE), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWPOINT), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWLINE), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWRECT), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWCIRC), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_FILL), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_FILLED), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDTOFRAME), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_TOFRAME), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DETSPR), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DELDETSPR), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_SPRITELIST2), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_FRUSESPRITE), ButtonSubclassProc2, 0, 0);
    }
    case WM_CTLCOLORDLG:
    {
        if (Night_Mode) return (INT_PTR)GetStockObject(DKGRAY_BRUSH);
        return (INT_PTR)GetStockObject(GRAY_BRUSH);
    }
    case WM_PAINT:
    {
        if (MycRom.name[0] == 0) return FALSE;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        RECT rc;
        GetWindowRect(GetDlgItem(hDlg, IDC_DRAWPOINT + Sprite_Mode), &rc);
        rc.bottom += 5;
        HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
        ScreenToClient(hDlg, &rc);
        FillRect(hdc, &rc, br);
        DeleteObject(br);
        EndPaint(hDlg, &ps);
        return TRUE;
    }
    case WM_DRAWITEM:
    {
        // draw the selected colors
        if ((MycRom.name[0] == 0) || (MycRom.nSprites == 0)) return TRUE;
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        UINT tpm;
        if ((lpdis->CtlID >= IDC_COL1) && (lpdis->CtlID < IDC_COL1+MycRom.noColors))
        {
            tpm = lpdis->CtlID - IDC_COL1;
            HBRUSH bg = CreateSolidBrush(RGB565_to_RGB888(MycRP.acEditColorsS[tpm]));
            RECT rc = lpdis->rcItem;
            if (tpm == noSprSel)
            {
                HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
                FillRect(lpdis->hDC, &rc, br);
                DeleteObject(br);
            }
            rc.left += 2;
            rc.right -= 2;
            rc.top += 2;
            rc.bottom -= 2;
            FillRect(lpdis->hDC, &rc, bg);
            DeleteObject(bg);
        }
        else if (lpdis->CtlID == IDC_DISPEXTRA)
        {
            RECT tr;
            tr.left = lpdis->rcItem.left;
            tr.right = lpdis->rcItem.right;
            tr.top = lpdis->rcItem.top;
            tr.bottom = lpdis->rcItem.bottom;
            HBRUSH bg;
            if (ExtraResSClicked) bg = hActiveBrush; else bg = hInactiveBrush;
            FillRect(lpdis->hDC, &tr, bg);
            if (ExtraResSClicked) DrawEdge(lpdis->hDC, &tr, EDGE_SUNKEN, BF_RECT);
            else DrawEdge(lpdis->hDC, &tr, EDGE_RAISED, BF_RECT);
            tr.top += 5;
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_BTNTEXT));
            SetBkMode(lpdis->hDC, TRANSPARENT);
            DrawTextA(lpdis->hDC, "Display\rExtra\rRes.", -1, &tr, DT_WORDBREAK | DT_CENTER);
        }        return TRUE;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
            case IDC_ADDSPRITE:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (MycRom.nSprites == 255) return TRUE;
                char tbuf[SIZE_SECTION_NAMES];
                GetDlgItemTextA(hwTB2, IDC_SPRITENAME, tbuf, SIZE_SECTION_NAMES-1);
                if (tbuf[0] == 0) return TRUE;
                tbuf[SIZE_SECTION_NAMES-1] = 0;
                int tnampos = Duplicate_Sprite_Name(tbuf);
                if (tnampos > -1)
                {
                    MessageBoxA(hSprites, "This name is already used for another sprite, action ignored.", "Caution", MB_OK);
                    return TRUE;
                }
                SaveAction(true, SA_SPRITES);
                MycRP.Sprite_Col_From_Frame[MycRom.nSprites] = acFrame;
                strcpy_s(&MycRP.Sprite_Names[SIZE_SECTION_NAMES * MycRom.nSprites], SIZE_SECTION_NAMES, tbuf);
                MycRom.isExtraSprite = (UINT8*)realloc(MycRom.isExtraSprite, MycRom.nSprites + 1);
                MycRom.SpriteOriginal = (UINT8*)realloc(MycRom.SpriteOriginal, (MycRom.nSprites + 1) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
                MycRom.SpriteColored = (UINT16*)realloc(MycRom.SpriteColored, sizeof(UINT16) * (MycRom.nSprites + 1) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
                MycRom.SpriteMaskX = (UINT8*)realloc(MycRom.SpriteMaskX, (MycRom.nSprites + 1) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
                MycRom.SpriteColoredX = (UINT16*)realloc(MycRom.SpriteColoredX, sizeof(UINT16) * (MycRom.nSprites + 1) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
                MycRom.isExtraSprite[MycRom.nSprites] = 0;
                memset(&MycRom.SpriteOriginal[MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], 255, MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
                memset(&MycRom.SpriteMaskX[MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], 255, MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
                memset(&MycRom.SpriteColoredX[MycRom.nSprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], 0, MAX_SPRITE_WIDTH* MAX_SPRITE_HEIGHT * sizeof(UINT16));
                MycRom.SpriteDetDwords = (UINT32*)realloc(MycRom.SpriteDetDwords, sizeof(UINT32) * (MycRom.nSprites + 1)*MAX_SPRITE_DETECT_AREAS);
                MycRom.SpriteDetDwordPos = (UINT16*)realloc(MycRom.SpriteDetDwordPos, sizeof(UINT16) * (MycRom.nSprites + 1) * MAX_SPRITE_DETECT_AREAS);
                MycRom.SpriteDetAreas = (UINT16*)realloc(MycRom.SpriteDetAreas, sizeof(UINT16) * (MycRom.nSprites + 1) * MAX_SPRITE_DETECT_AREAS * 4);
                for (UINT ti = 0; ti < MAX_SPRITE_DETECT_AREAS; ti++) MycRom.SpriteDetAreas[MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS + ti * 4] = 0xffff; // no area defined
                SendMessage(GetDlgItem(hwTB2, IDC_ISDWORD), BM_SETCHECK, BST_UNCHECKED, 0);
                acSprite = MycRom.nSprites;
                CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
                MycRom.nSprites++;
                for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                UpdateSpriteList();
                if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                UpdateSSneeded = true;
                return TRUE;
            }
            case IDC_DELSPRITE:
            {
                if (MycRom.nSprites == 0) return TRUE;
                char tbuf[256];
                sprintf_s(tbuf, 256, "Confirm you want to delete the sprite %s, this action can not be undone ?", &MycRP.Sprite_Names[acSprite * SIZE_SECTION_NAMES]);
                if (MessageBoxA(hSprites, tbuf, "Confirm", MB_YESNO) == IDYES)
                {
                    SaveAction(true, SA_SPRITES);
                    Delete_Sprite(acSprite);
                    UpdateSpriteList();
                    UpdateFrameSpriteList();
                    for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                }
                if ((acSprite >= MycRom.nSprites) && (MycRom.nSprites > 0)) acSprite = MycRom.nSprites - 1;
                if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
                UpdateSSneeded = true;
                return TRUE;
            }
            case IDC_SPRITELIST:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    SaveAction(true, SA_ACSPRITE);
                    acSprite = (UINT)SendMessage(GetDlgItem(hDlg, IDC_SPRITELIST), CB_GETCURSEL, 0, 0);
                    if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
                    for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                    if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                    if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                    UpdateSSneeded = true;
                }
                return TRUE;
            }
            case IDC_ADDTOFRAME:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (NSelection > 0)
                {
                    if (MessageBoxA(hWnd, "There is a selection in the frame, it will be used as a bounding box for the sprite. Do you confirm?", "Confirm", MB_YESNO) == IDNO) return TRUE;
                }
                SaveAction(true, SA_FRAMESPRITES);
                int notodisplay = 0;
                for (UINT tj = 0; tj < nSelFrames; tj++)
                {
                    UINT ti;
                    for (ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
                    {
                        if (MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] == acSprite) break;
                    }
                    if (ti == MAX_SPRITES_PER_FRAME)
                    {
                        ti = 0;
                        while ((MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] != 255) && (ti < MAX_SPRITES_PER_FRAME)) ti++;
                    }
                    if (ti == MAX_SPRITES_PER_FRAME)
                    {
                        MessageBoxA(hwTB2, "One of the selected frames has already reach the maximum number of sprites.", "Failed", MB_OK);
                    }
                    else
                    {
                        MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] = acSprite;
                        if (NSelection > 0)
                        {
                            if (nEditExtraResolutionF)
                            {
                                if (MycRom.fHeightX == 64)
                                {
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = XSelection / 2;
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = YSelection / 2;
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = (XSelection + WSelection - 1) / 2;
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = (YSelection + HSelection - 1) / 2;
                                }
                                else
                                {
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = XSelection * 2;
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = YSelection * 2;
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = (XSelection + WSelection - 1) * 2 + 1;
                                    MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = (YSelection + HSelection - 1) * 2 + 1;
                                }
                            }
                            else
                            {
                                MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = XSelection;
                                MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = YSelection;
                                MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = XSelection + WSelection - 1;
                                MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = YSelection + HSelection - 1;
                            }
                        }
                        else
                        {
                            MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = 0;
                            MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = 0;
                            MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = MycRom.fWidth - 1;
                            MycRom.FrameSpriteBB[SelFrames[tj] * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = MycRom.fHeight - 1;
                        }
                        if (SelFrames[tj] == acFrame) notodisplay = ti;
                    }
                }
                UpdateFrameSpriteList();
                UpdateSpriteList3();
                SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_SETCURSEL, notodisplay, 0);
                return TRUE;
            }
            case IDC_COPY:
            {
                if (MycRom.name[0] == 0 || MycRom.nSprites == 0) return TRUE;
                int minx=1024, miny=1024, maxx=0, maxy=0;
                UINT16* pspr;
                UINT8* pspro;
                if (nEditExtraResolutionS)
                {
                    if (Copy_Content != 2)
                    {
                        MessageBoxA(hwTB, "There is no extra resolution content in the selection, you can't copy in extra resolution mode.", "Failed", MB_OK);
                        return TRUE;
                    }
                    pspr = &MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
                    pspro = &MycRom.SpriteMaskX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
                    Paste_Content = 2;
                }
                else
                {
                    if (Copy_Content != 1)
                    {
                        MessageBoxA(hwTB, "There is no original resolution content in the selection, you can't copy in original resolution mode.", "Failed", MB_OK);
                        return TRUE;
                    }
                    pspr = &MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
                    pspro = &MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
                    Paste_Content = 1;
                }
                SaveAction(true, SA_COPYMASK);
                for (int tj = 0; tj < MAX_SPRITE_HEIGHT; tj++)
                {
                    for (int ti = 0; ti < MAX_SPRITE_WIDTH; ti++)
                    {
                        UINT16 val = pspr[tj * MAX_SPRITE_WIDTH + ti];
                        if (pspro[tj * MAX_SPRITE_WIDTH + ti] != 0xff)
                        {
                            if (minx > ti) minx = ti;
                            if (miny > tj) miny = tj;
                            if (maxx < ti) maxx = ti;
                            if (maxy < tj) maxy = tj;
                        }
                    }
                }
                if (minx > maxx) return TRUE;
                Copy_From_Frame = -1;
                Copy_Available = true;
                Paste_Width = maxx - minx + 1;
                Paste_Height = maxy - miny + 1;
                memset(Paste_Mask, 0, 256 * 64);
                for (int tj = miny; tj <= maxy; tj++)
                {
                    for (int ti = minx; ti <= maxx; ti++)
                    {
                        UINT16 val = pspr[tj * MAX_SPRITE_WIDTH + ti];
                        UINT8 spro = pspro[tj * MAX_SPRITE_WIDTH + ti];
                        if (spro != 0xff)
                        {
                            Paste_Mask[(tj - miny) * Paste_Width + ti - minx] = 1;
                            Paste_ColN[(tj - miny) * Paste_Width + ti - minx] = val;
                            Paste_Colo[(tj - miny) * Paste_Width + ti - minx] = spro;
                        }
                        else Paste_Mask[(tj - miny) * Paste_Width + ti - minx] = 0;
                        Paste_Dyna[(tj - miny) * Paste_Width + ti - minx] = 255;
                    }
                }
                EnableWindow(GetDlgItem(hwTB, IDC_PASTE), TRUE);
                EnableWindow(GetDlgItem(hwTB2, IDC_PASTE), TRUE);
                return TRUE;
            }
            case IDC_PASTE:
            {
                if (MycRom.name[0] == 0 || MycRom.nSprites == 0) return TRUE;
                UpdateSSneeded = true;
                MycRP.Sprite_Col_From_Frame[acSprite] = acFrame;
                UINT8 minx_cpy = 255, maxx_cpy = 0, miny_cpy = 63, maxy_cpy = 0;
                UINT16* pspr;
                UINT8* pspro;
                UINT fw, fh;
                if (nEditExtraResolutionS)
                {
                    if (Copy_Content != 2)
                    {
                        MessageBoxA(hwTB, "There is no extra resolution content in the selection, you can't copy in extra resolution mode.", "Failed", MB_OK);
                        return TRUE;
                    }
                    fw = MycRom.fWidthX;
                    fh = MycRom.fHeightX;
                    pspr = &MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
                    pspro = &MycRom.SpriteMaskX[acSprite * MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH];
                }
                else
                {
                    if (Copy_Content != 1)
                    {
                        MessageBoxA(hwTB, "There is no original resolution content in the selection, you can't copy in original resolution mode.", "Failed", MB_OK);
                        return TRUE;
                    }
                    fw = MycRom.fWidth;
                    fh = MycRom.fHeight;
                    pspr = &MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
                    pspro = &MycRom.SpriteOriginal[acSprite * MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH];
                }
                SaveAction(true, SA_SPRITE);
                for (UINT tj = 0; tj < Paste_Height; tj++)
                {
                    for (UINT ti = 0; ti < Paste_Width; ti++)
                    {
                        if (Paste_Mask[tj * Paste_Width + ti] > 0)
                        {
                            if (tj > maxy_cpy) maxy_cpy = tj;
                            if (tj < miny_cpy) miny_cpy = tj;
                            if (ti > maxx_cpy) maxx_cpy = ti;
                            if (ti < minx_cpy) minx_cpy = ti;
                        }
                    }
                }
                MycRP.SpriteRect[acSprite * 4] = minx_cpy;
                MycRP.SpriteRect[acSprite * 4 + 1] = miny_cpy;
                MycRP.SpriteRect[acSprite * 4 + 2] = maxx_cpy;
                MycRP.SpriteRect[acSprite * 4 + 3] = maxy_cpy;
                if (maxy_cpy - miny_cpy >= MAX_SPRITE_HEIGHT) maxy_cpy = miny_cpy + MAX_SPRITE_HEIGHT - 1;
                if (maxx_cpy - minx_cpy >= MAX_SPRITE_WIDTH) maxx_cpy = minx_cpy + MAX_SPRITE_WIDTH - 1;
                memset(pspro, 255, MAX_SPRITE_HEIGHT* MAX_SPRITE_WIDTH);
                for (UINT tj = miny_cpy; tj <= maxy_cpy; tj++)
                {
                    for (UINT ti = minx_cpy; ti <= maxx_cpy; ti++)
                    {
                        int i = ti, j = tj;
                        if (GetKeyState('K') & 0x8000)
                        {
                            i = maxx_cpy - (ti - minx_cpy);
                            MycRP.SpriteRectMirror[acSprite * 2] = TRUE;
                        }
                        else MycRP.SpriteRectMirror[acSprite * 2] = FALSE;
                        if (GetKeyState('L') & 0x8000)
                        {
                            j = maxy_cpy - (tj - miny_cpy);
                            MycRP.SpriteRectMirror[acSprite * 2 + 1] = TRUE;
                        }
                        else MycRP.SpriteRectMirror[acSprite * 2 + 1] = FALSE;
                        if (Paste_Mask[tj * Paste_Width + ti] > 0)
                        {
                            pspro[(j - miny_cpy) * MAX_SPRITE_WIDTH + (i - minx_cpy)] = Paste_Colo[tj * Paste_Width + ti];
                            pspr[(j - miny_cpy) * MAX_SPRITE_WIDTH + (i - minx_cpy)] = Paste_ColN[tj * Paste_Width + ti];
                        }
                    }
                }
                return TRUE;
            }
            case IDC_UNDO:
            {
                if (MycRom.name[0] == 0) return TRUE;
                RecoverAction(true);
                return TRUE;
            }
            case IDC_REDO:
            {
                if (MycRom.name[0] == 0) return TRUE;
                RecoverAction(false);
                return TRUE;
            }
            case IDC_COL1:
            case IDC_COL2:
            case IDC_COL3:
            case IDC_COL4:
            case IDC_COL5:
            case IDC_COL6:
            case IDC_COL7:
            case IDC_COL8:
            case IDC_COL9:
            case IDC_COL10:
            case IDC_COL11:
            case IDC_COL12:
            case IDC_COL13:
            case IDC_COL14:
            case IDC_COL15:
            case IDC_COL16:
            {
                if (LOWORD(wParam) - IDC_COL1 >= (int)MycRom.noColors) return TRUE;
                noSprSel = LOWORD(wParam) - IDC_COL1;
                if (MycRom.name[0] == 0) return TRUE;
                if (hColSet)
                {
                    DestroyWindow(hColSet);
                    hColSet = NULL;
                }
                Choose_Color_Palette2(LOWORD(wParam));
                InvalidateRect(hwTB2, NULL, TRUE);
                return TRUE;
            }
            case IDC_DRAWPOINT:
            case IDC_DRAWLINE:
            case IDC_DRAWRECT:
            case IDC_DRAWCIRC:
            case IDC_FILL:
            {
                Sprite_Mode = (UINT8)(LOWORD(wParam) - IDC_DRAWPOINT);
                InvalidateRect(hDlg, NULL, TRUE);
                return TRUE;
            }
            case IDC_FILLED:
            {
                if (Button_GetCheck(GetDlgItem(hwTB2, IDC_FILLED)) == BST_CHECKED) SpriteFill_Mode = TRUE; else SpriteFill_Mode = FALSE;
                return TRUE;
            }
            case IDC_SAVE:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (Save_cRom(false, false, (char*)""))
                {
                    cprintf(false, "%s.cROM saved in %s", MycRom.name, Dir_Serum);
                    if (Save_cRP(false)) cprintf(false, "%s.cRP saved in %s", MycRom.name, Dir_Serum);
                }
                return TRUE;
            }
            case IDC_DETSPR:
            {
                if (MycRom.name[0] == 0) return TRUE;
                acDetSprite = (UINT)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                return TRUE;
            }
            case IDC_DELDETSPR:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (MycRom.nSprites == 0) return TRUE;
                SaveAction(true, SA_SPRITE);
                MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4] = 0xffff;
                return TRUE;
            }
            case IDC_TOFRAME:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (MycRom.nSprites == 0) return TRUE;
                acFrame = MycRP.Sprite_Col_From_Frame[acSprite];

                if (acFrame >= MycRom.nFrames) acFrame = MycRom.nFrames - 1;
                if (acFrame >= PreFrameInStrip + NFrameToDraw) PreFrameInStrip = acFrame - NFrameToDraw + 1;
                SelFrames[0] = acFrame;
                nSelFrames = 1;
                SetMultiWarningF();
                if ((int)acFrame < PreFrameInStrip) PreFrameInStrip = acFrame;
                UpdateFSneeded = true;
                UpdateNewacFrame();
                /*
                // select the bounding box of the sprite in the Copy_Mask
                if (MycRP.SpriteRect[4 * acSprite] < 0xffff)
                {
                    memset(Copy_Mask, 0, MycRom.fWidth * MycRom.fHeight);
                    for (UINT tj = MycRP.SpriteRect[4 * acSprite + 1]; tj <= MycRP.SpriteRect[4 * acSprite + 3]; tj++)
                    {
                        for (UINT ti = MycRP.SpriteRect[4 * acSprite]; ti <= MycRP.SpriteRect[4 * acSprite + 2]; ti++)
                        {
                            UINT i = ti, j = tj;
                            if (MycRP.SpriteRectMirror[2 * acSprite] == TRUE) i = MycRP.SpriteRect[4 * acSprite + 2] - (ti - MycRP.SpriteRect[4 * acSprite]);
                            if (MycRP.SpriteRectMirror[2 * acSprite + 1] == TRUE) j = MycRP.SpriteRect[4 * acSprite + 3] - (tj - MycRP.SpriteRect[4 * acSprite + 1]);
                            if ((MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (tj - MycRP.SpriteRect[4 * acSprite + 1]) * MAX_SPRITE_SIZE + ti - MycRP.SpriteRect[4 * acSprite]] & 0x8000) == 0)
                            {
                                Copy_Mask[i + j * MycRom.fWidth] = 1;
                            }
                        }
                    }
                }
                */
                return TRUE;
            }
            case IDC_SPRITELIST2:
            {
                UpdateSpriteList2();
                return TRUE;
            }
            case IDC_ZOOM2X:
            {
                if (Zoom_Pushed_Sprite) Zoom_Pushed_Sprite = false;
                else Zoom_Pushed_Sprite = true;
                SetZoomSpriteButton(Zoom_Pushed_Sprite);
                return TRUE;
            }
            case IDC_EXTRARES:
            {
                SaveAction(true, SA_ISSPRITEX);
                MycRom.isExtraSprite[acSprite] = !MycRom.isExtraSprite[acSprite];
                if (MycRom.isExtraSprite[acSprite] > 0)
                {
                    ExtraResSClicked = true;
                    nEditExtraResolutionS = true;
                }
                else
                {
                    ExtraResSClicked = false;
                    nEditExtraResolutionS = false;
                }
                for (UINT ti = 0; ti < nSelSprites; ti++) MycRom.isExtraSprite[SelSprites[ti]] = MycRom.isExtraSprite[acSprite];
                UpdateSSneeded = true;
                Calc_Resize_Sprite();
                FreeCopyMasks();
                InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                return TRUE;
            }
            case IDC_DISPEXTRA:
            {
                if (MycRom.name[0] == 0 || MycRom.nSprites == 0) return TRUE;
                ExtraResSClicked = !ExtraResSClicked;
                if (MycRom.isExtraSprite[acSprite] > 0 && ExtraResSClicked) nEditExtraResolutionS = true;
                else nEditExtraResolutionS = false;
                FreeCopyMasks();
                UpdateSSneeded = true;
                Update_Toolbar2 = true;
                FreeCopyMasks();
                Calc_Resize_Sprite();
                Calc_Resize_Image();
                InitColorRotation();
                InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                return TRUE;
            }
            case IDC_ORGTOXTRA2:
            {
                if (MycRom.name[0] == 0) return TRUE;
                SaveAction(true, SA_ISSPRITEX);
                SaveAction(true, SA_SPRITEDRAW);
                ResizeRGB565Sprite(&MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteMaskX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], MycRom.fHeight == 64, SpriteResizeFilter);
                ExtraResSClicked = true;
                InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                MycRom.isExtraSprite[acSprite] = 1;
                nEditExtraResolutionS = true;
                CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED);
                UpdateSSneeded = true;
                FreeCopyMasks();
                Calc_Resize_Sprite();
                return TRUE;
            }
            case IDC_XTRATOORG2:
            {
                if (MycRom.name[0] == 0 || MycRom.isExtraSprite[acSprite] == 0) return TRUE;
                SaveAction(true, SA_SPRITEDRAW);
                ResizeRGB565Sprite(&MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], &MycRom.SpriteMaskX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT], MycRom.fHeightX == 64, SpriteResizeFilter);
                ExtraResSClicked = false;
                InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                nEditExtraResolutionS = false;
                UpdateSSneeded = true;
                FreeCopyMasks();
                Calc_Resize_Sprite();
                return TRUE;
            }
            case IDC_FILTERTYPE:
            {
                int nofilter = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                switch (nofilter)
                {
                case 0:
                    SpriteResizeFilter = cv::INTER_NEAREST;
                    break;
                case 1:
                    SpriteResizeFilter = cv::INTER_LINEAR;
                    break;
                case 2:
                    SpriteResizeFilter = cv::INTER_CUBIC;
                    break;
                case 3:
                    SpriteResizeFilter = cv::INTER_LANCZOS4;
                    break;
                }
                return TRUE;
            }
        }
        return (INT_PTR)FALSE;
    }
    }
    return (INT_PTR)FALSE;
}

const char* ButtonDescription3(HWND hOver)
{
    if (hOver == GetDlgItem(hwTB3, IDC_BROWSEIMAGE)) return (const char*)"Open an image file (CTRL+O)";
    if (hOver == GetDlgItem(hwTB3, IDC_CBPASTE)) return (const char*)"Use an image file from the clipboard (CTRL+V)";
    //if (hOver == GetDlgItem(hwTB3, IDC_DELIMAGE)) return (const char*)"Delete the image resource";
    if (hOver == GetDlgItem(hwTB3, IDC_COPY)) return (const char*)"Copy this image to the frame selection (CTRL+C)";
    if (hOver == GetDlgItem(hwTB3, IDC_ZOOMIN)) return (const char*)"Zoom in the image (MOUSE WHEEL UP)";
    if (hOver == GetDlgItem(hwTB3, IDC_ZOOMOUT)) return (const char*)"Zoom out in the image (MOUSE WHEEL DOWN)";
    if (hOver == GetDlgItem(hwTB3, IDC_HOUR)) return (const char*)"The slider will modify the hour number of the current time";
    if (hOver == GetDlgItem(hwTB3, IDC_MINUTE)) return (const char*)"The slider will modify the minute number of the current time";
    if (hOver == GetDlgItem(hwTB3, IDC_SECOND)) return (const char*)"The slider will modify the second number of the current time";
    if (hOver == GetDlgItem(hwTB3, IDC_FRAME)) return (const char*)"The slider will modify the frame number of the current time";
    if (hOver == GetDlgItem(hwTB3, IDC_VIDEOSLIDER)) return (const char*)"Slider to modify the current time according the left checked option";
    if (hOver == GetDlgItem(hwTB3, IDC_FRAMEDUR)) return (const char*)"Use the frame duration (in yellow in the frame strip) to calculate the next frame to pick in the video";
    if (hOver == GetDlgItem(hwTB3, IDC_REGULDUR)) return (const char*)"Use a regular time span you manually provide to calculate the next frame to pick in the video ";
    if (hOver == GetDlgItem(hwTB3, IDC_CURFRAMEALL)) return (const char*)"The current video frame will be copied to all the frames selected";
    if (hOver == GetDlgItem(hwTB3, IDC_INTERV)) return (const char*)"Manual value in milliseconds for the regular time span between the frames to pick in the video";
    if (hOver == GetDlgItem(hwTB3, IDC_BRIGHTNESS)) return (const char*)"With this slider, you can modify the brightness (initial value 0)";
    if (hOver == GetDlgItem(hwTB3, IDC_SCROLLCOPY)) return (const char*)"Select initial image part (position and zoom) with left mouse button, final image part (idem) with right mouse button, then press to create a nice scrolling/zoom between them";
    if (hOver == GetDlgItem(hwTB3, IDC_CONTRAST)) return (const char*)"With this slider, you can modify the contrast (initial value 1.00)";
    if (hOver == GetDlgItem(hwTB3, IDC_PREV)) return (const char*)"Jump to the previous time step";
    if (hOver == GetDlgItem(hwTB3, IDC_NEXT)) return (const char*)"Jump to the next time step";
    return "";
}

LRESULT CALLBACK ButtonSubclassProc3(HWND hBut, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (message) {
    case WM_KEYDOWN:
    {
        return TRUE;
    }
    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hBut;
        tme.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&tme);
        SetWindowTextA(hStatus3, ButtonDescription3(hBut));
        break;
    }
    case WM_MOUSELEAVE:
    {
        SetWindowTextA(hStatus3, "");
        break;
    }
    }

    return DefSubclassProc(hBut, message, wParam, lParam);
}

void ApplyBrightnessContrastAndBlur(cv::Mat mat)
{
    cv::Mat floatImage;
    mat.convertTo(floatImage, CV_32F);

    double alpha = 1 + 0.05 * image_contrast;
    if (alpha == 1.0) alpha = 1.001;
    double beta = 2.5 * image_brightness;
    floatImage = alpha * floatImage + beta;

    if (image_blur > 0) cv::blur(floatImage, floatImage, cv::Size(image_blur, image_blur));

    floatImage.convertTo(mat, mat.type());
}

UINT CreateTextureFromImage(char* filename, UINT* width, UINT* height)
{
    cv::Mat mat = cv::imread(filename);
    ApplyBrightnessContrastAndBlur(mat);
    if (mat.empty()) return (UINT)-1;
    if (mat.cols % 4 != 0)
    {
        float ratio = (float)mat.cols / (float)mat.rows;
        int cols = mat.cols - (mat.cols % 4) + 4;
        int rows = (int)((float)cols / ratio);
        cv::Mat tmat;
        resize(mat, tmat, cv::Size(cols, rows), 0, 0, cv::INTER_CUBIC);
        mat.release();
        mat = tmat.clone();
        tmat.release();
    }

    GLenum format;
    GLenum iformat;
    GLenum type;
    switch (mat.channels())
    {
    case 3:
        format = GL_RGB;
        iformat = GL_BGR;
        image_mat = mat.clone();
        mat.release();
        break;
    case 4:
        format = GL_RGB;
        iformat = GL_BGR;
        cvtColor(mat, image_mat, COLOR_BGRA2BGR);
        mat.release();
        break;
    default:
        std::cerr << "Unsupported number of channels: " << mat.channels() << std::endl;
        mat.release();
        return (UINT)-1;
    }
    switch (image_mat.depth())
    {
    case CV_8U:
        type = GL_UNSIGNED_BYTE;
        break;
    default:
        std::cerr << "Unsupported data type: " << image_mat.depth() << std::endl;
        image_mat.release();
        return (UINT)-1;
    }
    *width = image_mat.cols;
    *height = image_mat.rows;
    glfwMakeContextCurrent(glfwimages);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, image_mat.cols, image_mat.rows, 0, iformat, type, image_mat.ptr());

    image_loaded = true;
    return texture;
}

UINT CreateTextureFromMat(void)
{
    cv::Mat mat = image_org_mat.clone();
    ApplyBrightnessContrastAndBlur(mat);
    if (mat.empty()) return (UINT)-1;
    if (mat.cols % 4 != 0)
    {
        float ratio = (float)mat.cols / (float)mat.rows;
        int cols = mat.cols - (mat.cols % 4) + 4;
        int rows = (int)((float)cols / ratio);
        cv::Mat tmat;
        resize(mat, tmat, cv::Size(cols, rows), 0, 0, cv::INTER_CUBIC);
        mat.release();
        mat = tmat.clone();
        tmat.release();
    }

    GLenum format;
    GLenum iformat;
    GLenum type;
    switch (mat.channels())
    {
    case 3:
        format = GL_RGB;
        iformat = GL_BGR;
        image_mat = mat.clone();
        mat.release();
        break;
    case 4:
        format = GL_RGB;
        iformat = GL_BGR;
        cvtColor(mat, image_mat, COLOR_BGRA2BGR);
        mat.release();
        break;
    default:
        std::cerr << "Unsupported number of channels: " << mat.channels() << std::endl;
        mat.release();
        return (UINT)-1;
    }
    switch (image_mat.depth())
    {
    case CV_8U:
        type = GL_UNSIGNED_BYTE;
        break;
    default:
        std::cerr << "Unsupported data type: " << image_mat.depth() << std::endl;
        image_mat.release();
        return (UINT)-1;
    }
    glfwMakeContextCurrent(glfwimages);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, image_mat.cols, image_mat.rows, 0, iformat, type, image_mat.ptr());

    image_loaded = true;
    return texture;
}

cv::Mat getFrameAtTime(VideoCapture cap)
{
    int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
    image_video_frame_rate = (long)(cap.get(cv::CAP_PROP_FPS) + 0.5);
    double timeInSeconds = image_video_hour * 3600 + image_video_minute * 60 + image_video_second + ((float)image_video_frame / (float)image_video_frame_rate);
    int positionInFrames = static_cast<int>(timeInSeconds * cap.get(cv::CAP_PROP_FPS));
    positionInFrames = std::max(0, std::min(totalFrames - 1, positionInFrames));
    cap.set(cv::CAP_PROP_POS_FRAMES, positionInFrames);
    cv::Mat frame;
    if (!cap.read(frame))
        frame.data = NULL;
    return frame;
}

void CreateMatFromVideo(VideoCapture video)
{
    cv::Mat mat = getFrameAtTime(video);
    if (!mat.data) return;
    ApplyBrightnessContrastAndBlur(mat);
    if (mat.cols % 4 != 0)
    {
        float ratio = (float)mat.cols / (float)mat.rows;
        int cols = mat.cols - (mat.cols % 4) + 4;
        int rows = (int)((float)cols / ratio);
        cv::Mat tmat;
        resize(mat, tmat, cv::Size(cols, rows), 0, 0, cv::INTER_CUBIC);
        mat.release();
        mat = tmat.clone();
        tmat.release();
    }

    GLenum format;
    GLenum iformat;
    GLenum type;
    switch (mat.channels())
    {
    case 3:
        format = GL_RGB;
        iformat = GL_BGR;
        image_mat = mat.clone();
        mat.release();
        break;
    case 4:
        format = GL_RGB;
        iformat = GL_BGR;
        cvtColor(mat, image_mat, COLOR_BGRA2BGR);
        mat.release();
        break;
    default:
        std::cerr << "Unsupported number of channels: " << mat.channels() << std::endl;
        mat.release();
        return;
    }
    switch (image_mat.depth())
    {
    case CV_8U:
        type = GL_UNSIGNED_BYTE;
        break;
    default:
        std::cerr << "Unsupported data type: " << image_mat.depth() << std::endl;
        image_mat.release();
        return;
    }
}
UINT CreateTextureFromVideo(VideoCapture video, UINT* width, UINT* height)
{
    cv::Mat mat = getFrameAtTime(video);
    if (!mat.data) return (UINT)-1;
    ApplyBrightnessContrastAndBlur(mat);
    if (mat.cols % 4 != 0)
    {
        float ratio = (float)mat.cols / (float)mat.rows;
        int cols = mat.cols - (mat.cols % 4) + 4;
        int rows = (int)((float)cols / ratio);
        cv::Mat tmat;
        resize(mat, tmat, cv::Size(cols, rows), 0, 0, cv::INTER_CUBIC);
        mat.release();
        mat = tmat.clone();
        tmat.release();
    }

    GLenum format;
    GLenum iformat;
    GLenum type;
    switch (mat.channels())
    {
    case 3:
        format = GL_RGB;
        iformat = GL_BGR;
        image_mat = mat.clone();
        mat.release();
        break;
    case 4:
        format = GL_RGB;
        iformat = GL_BGR;
        cvtColor(mat, image_mat, COLOR_BGRA2BGR);
        mat.release();
        break;
    default:
        std::cerr << "Unsupported number of channels: " << mat.channels() << std::endl;
        mat.release();
        return (UINT)-1;
    }
    switch (image_mat.depth())
    {
    case CV_8U:
        type = GL_UNSIGNED_BYTE;
        break;
    default:
        std::cerr << "Unsupported data type: " << image_mat.depth() << std::endl;
        image_mat.release();
        return (UINT)-1;
    }
    *width = image_mat.cols;
    *height = image_mat.rows;
    glfwMakeContextCurrent(glfwimages);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, image_mat.cols, image_mat.rows, 0, iformat, type, image_mat.ptr());

    image_loaded = true;
    return texture;
}

void CopyImageToSelection(cv::Mat tmp32, cv::Mat tmp64)
{
    UINT tSelFrames[MAX_SEL_FRAMES];
    for (UINT ti = 0; ti < nSelFrames; ti++) tSelFrames[ti] = SelFrames[ti];
    BubbleSort(tSelFrames, nSelFrames);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
            MycRom.isExtraFrame[tSelFrames[ti]] = 1;
        UINT fw64, fw32;
        UINT16* pfr32 = NULL, * pfr64 = NULL;
        bool is64 = false, is32 = false;
        if (MycRom.fHeightX == 32 && Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
        {
            is32 = true;
            fw32 = MycRom.fWidthX;
            if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
                pfr32 = &MycRom.BackgroundFramesX[(tSelFrames[ti]) * fw32 * 32];
            else
                pfr32 = &MycRom.cFramesX[(tSelFrames[ti]) * fw32 * 32];
        }
        else if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
        {
            is64 = true;
            fw64 = MycRom.fWidthX;
            if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
                pfr64 = &MycRom.BackgroundFramesX[(tSelFrames[ti]) * fw64 * 64];
            else
                pfr64 = &MycRom.cFramesX[(tSelFrames[ti]) * fw64 * 64];
        }
        if (MycRom.fHeight == 32 && Button_GetCheck(GetDlgItem(hwTB3, IDC_TOORGRES)) == BST_CHECKED)
        {
            is32 = true;
            fw32 = MycRom.fWidth;
            if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
                pfr32 = &MycRom.BackgroundFrames[(tSelFrames[ti]) * fw32 * 32];
            else
                pfr32 = &MycRom.cFrames[(tSelFrames[ti]) * fw32 * 32];
        }
        else if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOORGRES)) == BST_CHECKED)
        {
            is64 = true;
            fw64 = MycRom.fWidth;
            if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
                pfr64 = &MycRom.BackgroundFrames[(tSelFrames[ti]) * fw64 * 64];
            else
                pfr64 = &MycRom.cFrames[(tSelFrames[ti]) * fw64 * 64];
        }
        for (UINT tj = 0; tj < HiSelection; tj++)
        {
            for (UINT tk = 0; tk < WiSelection; tk++)
            {
                if (is64)
                {
                    if (Copy_iMask[(YiSelection + tj) * fw64 + XiSelection + tk] != 0)
                    {
                        cv::Vec3b color = tmp64.at<cv::Vec3b>(tj, tk);
                        pfr64[(YiSelection + tj) * fw64 + (XiSelection + tk)] = rgb888_to_rgb565(color[2], color[1], color[0]);
                    }
                }
                if (is32)
                {
                    if (Copy_iMask[(YiSelection + tj) * fw32 * 2 + XiSelection + tk] != 0)
                    {
                        cv::Vec3b color = tmp32.at<cv::Vec3b>(tj / 2, tk / 2);
                        pfr32[(YiSelection + tj) / 2 * fw32 + (XiSelection + tk) / 2] = rgb888_to_rgb565(color[2], color[1], color[0]);
                    }
                }
            }
        }
    }
}
void CopyImageToBackground(cv::Mat tmp32, cv::Mat tmp64)
{
    if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
        MycRom.isExtraBackground[acBG] = 1;
    UINT fw64, fw32;
    UINT16* pfr32 = NULL, * pfr64 = NULL;
    bool is64 = false, is32 = false;
    if (MycRom.fHeightX == 32 && Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
    {
        is32 = true;
        fw32 = MycRom.fWidthX;
        pfr32 = &MycRom.BackgroundFramesX[acBG * fw32 * 32];
    }
    else if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
    {
        is64 = true;
        fw64 = MycRom.fWidthX;
        pfr64 = &MycRom.BackgroundFramesX[acBG * fw64 * 64];
    }
    if (MycRom.fHeight == 32 && Button_GetCheck(GetDlgItem(hwTB3, IDC_TOORGRES)) == BST_CHECKED)
    {
        is32 = true;
        fw32 = MycRom.fWidth;
        pfr32 = &MycRom.BackgroundFrames[acBG * fw32 * 32];
    }
    else if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOORGRES)) == BST_CHECKED)
    {
        is64 = true;
        fw64 = MycRom.fWidth;
        pfr64 = &MycRom.BackgroundFrames[acBG * fw64 * 64];
    }
    for (UINT tj = 0; tj < HiSelection; tj++)
    {
        for (UINT tk = 0; tk < WiSelection; tk++)
        {
            if (is64)
            {
                cv::Vec3b color = tmp64.at<cv::Vec3b>(tj, tk);
                pfr64[(YiSelection + tj) * fw64 + (XiSelection + tk)] = rgb888_to_rgb565(color[2], color[1], color[0]);
            }
            if (is32)
            {
                cv::Vec3b color = tmp32.at<cv::Vec3b>(tj / 2, tk / 2);
                pfr32[(YiSelection + tj) / 2 * fw32 + (XiSelection + tk) / 2] = rgb888_to_rgb565(color[2], color[1], color[0]);
            }
        }
    }
}

void CopyImageTo1Selection(cv::Mat tmp32, cv::Mat tmp64, UINT nofr)
{
    if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
        MycRom.isExtraFrame[nofr] = 1;
    UINT fw64, fw32;
    UINT16* pfr32 = NULL, * pfr64 = NULL;
    bool is64 = false, is32 = false;
    if (MycRom.fHeightX == 32 && Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
    {
        is32 = true;
        fw32 = MycRom.fWidthX;
        if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
            pfr32 = &MycRom.BackgroundFramesX[nofr * fw32 * 32];
        else
            pfr32 = &MycRom.cFramesX[nofr * fw32 * 32];
    }
    else if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
    {
        is64 = true;
        fw64 = MycRom.fWidthX;
        if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
            pfr64 = &MycRom.BackgroundFramesX[nofr * fw64 * 64];
        else
            pfr64 = &MycRom.cFramesX[nofr * fw64 * 64];
    }
    if (MycRom.fHeight == 32 && Button_GetCheck(GetDlgItem(hwTB3, IDC_TOORGRES)) == BST_CHECKED)
    {
        is32 = true;
        fw32 = MycRom.fWidth;
        if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
            pfr32 = &MycRom.BackgroundFrames[nofr * fw32 * 32];
        else
            pfr32 = &MycRom.cFrames[nofr * fw32 * 32];
    }
    else if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOORGRES)) == BST_CHECKED)
    {
        is64 = true;
        fw64 = MycRom.fWidth;
        if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
            pfr64 = &MycRom.BackgroundFrames[nofr * fw64 * 64];
        else
            pfr64 = &MycRom.cFrames[nofr * fw64 * 64];
    }
    for (UINT tj = 0; tj < HiSelection; tj++)
    {
        for (UINT tk = 0; tk < WiSelection; tk++)
        {
            if (is64)
            {
                if (Copy_iMask[(YiSelection + tj) * fw64 + XiSelection + tk] != 0)
                {
                    cv::Vec3b color = tmp64.at<cv::Vec3b>(tj, tk);
                    pfr64[(YiSelection + tj) * fw64 + (XiSelection + tk)] = rgb888_to_rgb565(color[2], color[1], color[0]);
                }
            }
            if (is32)
            {
                if (Copy_iMask[(YiSelection + tj) * fw32 * 2 + XiSelection + tk] != 0)
                {
                    cv::Vec3b color = tmp32.at<cv::Vec3b>(tj / 2, tk / 2);
                    pfr32[(YiSelection + tj) / 2 * fw32 + (XiSelection + tk) / 2] = rgb888_to_rgb565(color[2], color[1], color[0]);
                }
            }
        }
    }
}
/*void CopyImageTo1Selection(cv::Mat mat, UINT nofr)
{
    UINT fw, fh;
    UINT16* pfr;
    if (ExtraResFClicked && MycRom.isExtraFrame[nofr] > 0)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pfr = &MycRom.cFramesX[nofr * fw * fh];
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pfr = &MycRom.cFrames[nofr * fw * fh];
    }
    for (UINT tj = 0; tj < HiSelection; tj++)
    {
        for (UINT ti = 0; ti < WiSelection; ti++)
        {
            if (Copy_Mask[(tj + YiSelection) * fw + ti + XiSelection] > 0)
            {
                cv::Vec3b color = mat.at<cv::Vec3b>(tj, ti);
                pfr[(tj + YiSelection) * fw + ti + XiSelection] = rgb888_to_rgb565(color[2], color[1], color[0]);
            }
        }
    }
}*/

cv::Mat Create24bcvMatFrom32bBitmap(HBITMAP hbmp)
{
    BITMAP bm;
    GetObject(hbmp, sizeof(bm), &bm);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = bm.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pPixels = nullptr;
    HDC hDC = CreateCompatibleDC(nullptr);
    HBITMAP hDib = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pPixels, nullptr, 0);
    DeleteDC(hDC);

    HDC hMemDC = CreateCompatibleDC(nullptr);
    SelectObject(hMemDC, hDib);
    GetDIBits(hMemDC, hbmp, 0, bm.bmHeight, pPixels, &bmi, DIB_RGB_COLORS);
    DeleteDC(hMemDC);

    cv::Mat mat(bm.bmHeight, bm.bmWidth, CV_8UC3);
    unsigned char* pDest = mat.data;
    unsigned char* pSrc = static_cast<unsigned char*>(pPixels);

    for (int y = 0; y < bm.bmHeight; ++y) {
        for (int x = 0; x < bm.bmWidth; ++x) {
            pDest[(y * bm.bmWidth + x) * 3 + 0] = pSrc[((bm.bmHeight - 1 - y) * bm.bmWidth + x) * 4 + 0];
            pDest[(y * bm.bmWidth + x) * 3 + 1] = pSrc[((bm.bmHeight - 1 - y) * bm.bmWidth + x) * 4 + 1];
            pDest[(y * bm.bmWidth + x) * 3 + 2] = pSrc[((bm.bmHeight - 1 - y) * bm.bmWidth + x) * 4 + 2];
        }
    }
    DeleteObject(hDib);
    return mat;
}

GLuint CreateTextureFromClipboard(UINT* pw, UINT* ph)
{
    if (!IsClipboardFormatAvailable(CF_BITMAP))
    {
        MessageBoxA(hImages, "No image in clipboard available", "Failed", MB_OK);
        return (GLuint)-1;
    }
    if (!OpenClipboard(NULL))
    {
        MessageBoxA(hImages, "Failed to open clipboard", "Failed", MB_OK);
        return (GLuint)-1;
    }
    HBITMAP hBmp = (HBITMAP)GetClipboardData(CF_BITMAP);
    if (hBmp == NULL)
    {
        MessageBoxA(hImages, "Failed to get bitmap data from clipboard", "Failed", MB_OK);
        CloseClipboard();
        return (GLuint)-1;
    }
    BITMAP bm;
    GetObject(hBmp, sizeof(BITMAP), &bm);
    if (bm.bmWidth % 4 != 0)
    {
        float ratio = (float)bm.bmWidth / (float)bm.bmHeight;
        int cols = bm.bmWidth - (bm.bmWidth % 4) + 4;
        int rows = (int)((float)cols / ratio);
        HDC hdc = GetDC(NULL);
        HDC hdcOrig = CreateCompatibleDC(hdc);
        HDC hdcNew = CreateCompatibleDC(hdc);
        HBITMAP hNewBmp = CreateCompatibleBitmap(hdc, cols, rows);
        SelectObject(hdcOrig, hBmp);
        SelectObject(hdcNew, hNewBmp);
        StretchBlt(hdcNew, 0, 0, cols, rows, hdcOrig, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
        DeleteDC(hdcOrig);
        DeleteDC(hdcNew);
        ReleaseDC(NULL, hdc);
        DeleteObject(hBmp);
        hBmp = hNewBmp;
    }
    GetObject(hBmp, sizeof(BITMAP), &bm);
    *pw = bm.bmWidth;
    *ph = bm.bmHeight;
    int iformat, format;
    if (bm.bmBitsPixel == 32)
    {
        format = GL_RGBA;
        iformat = GL_BGRA;
        image_mat = Create24bcvMatFrom32bBitmap(hBmp);
        image_org_mat = image_mat.clone();
        ApplyBrightnessContrastAndBlur(image_mat);
    }
    else if (bm.bmBitsPixel == 24)
    {
        format = GL_RGB;
        iformat = GL_BGR;
        image_mat.create(*ph, *pw, CV_8UC3);
        BITMAPINFOHEADER bi = { sizeof(bi), bm.bmWidthBytes, -bm.bmHeight, 1, 24, BI_RGB };
        HDC hdc = GetDC(hImages);
        GetDIBits(hdc, hBmp, 0, bm.bmHeight, image_mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        ReleaseDC(hImages, hdc);
    }
    else
    {
        MessageBoxA(hImages, "Unknown image format in the clipboard", "Failed", MB_OK);
        CloseClipboard();
        return (UINT) -1;
    }
    LPVOID lpBitmap = (LPVOID)malloc(bm.bmHeight * bm.bmWidthBytes * bm.bmBitsPixel / 8);
    if (!lpBitmap) return (UINT)-1;
    GetBitmapBits(hBmp, bm.bmHeight * bm.bmWidthBytes * bm.bmBitsPixel / 8, lpBitmap);
    glfwMakeContextCurrent(glfwimages);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, bm.bmWidth , bm.bmHeight , 0, iformat, GL_UNSIGNED_BYTE, lpBitmap);
    free(lpBitmap);
    image_loaded = true;
    CloseClipboard();
    DeleteObject(hBmp);
    return textureID;
}

long HMSFToLong(UINT8 h, UINT8 m, UINT8 s, UINT8 f)
{
    return (f + image_video_frame_rate * (s + 60 * m + 3600 * h));
}
void LongToHMSF(long dpos, UINT8* ph, UINT8* pm, UINT8* ps, UINT8* pf)
{
    *ph = (INT8)(dpos / (3600 * image_video_frame_rate));
    *pm = (INT8)((dpos - *ph * 3600 * image_video_frame_rate) / (60 * image_video_frame_rate));
    *ps = (INT8)((dpos - (*ph * 3600 + *pm * 60) * image_video_frame_rate) / image_video_frame_rate);
    *pf = (INT8)(dpos - (*ph * 3600 + *pm * 60 + *ps) * image_video_frame_rate);
}

void UpdateHMSF(HWND hDlg)
{
    HWND hS = GetDlgItem(hDlg, IDC_VIDEOSLIDER);
    if (SendMessage(GetDlgItem(hDlg, IDC_HOUR), BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        SendMessage(hS, TBM_SETRANGE, TRUE, MAKELONG(0, image_video_nhours));
        SendMessage(hS, TBM_SETPOS, TRUE, image_video_hour);
    }
    else if (SendMessage(GetDlgItem(hDlg, IDC_MINUTE), BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        if (image_video_hour == image_video_nhours) SendMessage(hS, TBM_SETRANGE, TRUE, MAKELONG(0, image_video_nminutes));
        else SendMessage(hS, TBM_SETRANGE, TRUE, MAKELONG(0, 59));
        SendMessage(hS, TBM_SETPOS, TRUE, image_video_minute);
    }
    else if (SendMessage(GetDlgItem(hDlg, IDC_SECOND), BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        if ((image_video_hour == image_video_nhours) && (image_video_minute == image_video_nminutes)) SendMessage(hS, TBM_SETRANGE, TRUE, MAKELONG(0, image_video_nseconds));
        else SendMessage(hS, TBM_SETRANGE, TRUE, MAKELONG(0, 59));
        SendMessage(hS, TBM_SETPOS, TRUE, image_video_second);
    }
    else
    {
        if ((image_video_hour == image_video_nhours) && (image_video_minute == image_video_nminutes) && (image_video_second == image_video_nseconds)) SendMessage(hS, TBM_SETRANGE, TRUE, MAKELONG(0, image_video_nframes));
        else SendMessage(hS, TBM_SETRANGE, TRUE, MAKELONG(0, (int)image_video_frame_rate - 1));
        SendMessage(hS, TBM_SETPOS, TRUE, image_video_frame);
    }
}

void UpdateHMSFfromSlider(HWND hDlg)
{
    UINT8 pos = (UINT8)SendMessage(GetDlgItem(hDlg, IDC_VIDEOSLIDER), TBM_GETPOS, 0, 0);
    if (SendMessage(GetDlgItem(hDlg, IDC_HOUR), BM_GETCHECK, 0, 0) == BST_CHECKED) image_video_hour = pos;
    else if (SendMessage(GetDlgItem(hDlg, IDC_MINUTE), BM_GETCHECK, 0, 0) == BST_CHECKED) image_video_minute = pos;
    else if (SendMessage(GetDlgItem(hDlg, IDC_SECOND), BM_GETCHECK, 0, 0) == BST_CHECKED)  image_video_second = pos;
    else image_video_frame = pos;
    if (image_video_hour == image_video_nhours)
    {
        if (image_video_minute > image_video_nminutes) image_video_minute = image_video_nminutes;
        if (image_video_minute == image_video_nminutes)
        {
            if (image_video_second > image_video_nseconds) image_video_second = image_video_nseconds;
            if (image_video_second == image_video_nseconds)
            {
                if (image_video_frame > image_video_nframes) image_video_frame = image_video_nframes;
            }
        }
    }
    char tbuf[32];
    sprintf_s(tbuf, 32, "%i:%02i:%02i:%02i", image_video_hour, image_video_minute, image_video_second, image_video_frame);
    SetDlgItemTextA(hDlg, IDC_CURSORTIME, tbuf);
    glfwMakeContextCurrent(glfwimages);
    if (TxImage != (UINT)-1)
    {
        image_mat.release();
        glDeleteTextures(1, &TxImage);
    }
    TxImage = CreateTextureFromVideo(image_video_cap, &width_image, &height_image);
}

void VideoMove(HWND hDlg, long direction)
{
    long addtime;
    if (SendMessage(GetDlgItem(hDlg, IDC_HOUR), BM_GETCHECK, 0, 0) == BST_CHECKED) addtime = direction * 3600 * image_video_frame_rate;
    else if (SendMessage(GetDlgItem(hDlg, IDC_MINUTE), BM_GETCHECK, 0, 0) == BST_CHECKED) addtime = direction * 60 * image_video_frame_rate;
    else if (SendMessage(GetDlgItem(hDlg, IDC_SECOND), BM_GETCHECK, 0, 0) == BST_CHECKED) addtime = direction * image_video_frame_rate;
    else addtime = direction;
    addtime += HMSFToLong(image_video_hour, image_video_minute, image_video_second, image_video_frame);
    long maxtime = HMSFToLong(image_video_nhours, image_video_nminutes, image_video_nseconds, image_video_nframes);
    if (addtime < 0) addtime = 0;
    else if (addtime > maxtime) addtime = maxtime;
    LongToHMSF(addtime, &image_video_hour, &image_video_minute, &image_video_second, &image_video_frame);
    if (SendMessage(GetDlgItem(hDlg, IDC_HOUR), BM_GETCHECK, 0, 0) == BST_CHECKED) SendMessage(GetDlgItem(hDlg, IDC_VIDEOSLIDER), TBM_SETPOS, TRUE, image_video_hour);
    else if (SendMessage(GetDlgItem(hDlg, IDC_MINUTE), BM_GETCHECK, 0, 0) == BST_CHECKED) SendMessage(GetDlgItem(hDlg, IDC_VIDEOSLIDER), TBM_SETPOS, TRUE, image_video_minute);
    else if (SendMessage(GetDlgItem(hDlg, IDC_SECOND), BM_GETCHECK, 0, 0) == BST_CHECKED)  SendMessage(GetDlgItem(hDlg, IDC_VIDEOSLIDER), TBM_SETPOS, TRUE, image_video_second);
    else  SendMessage(GetDlgItem(hDlg, IDC_VIDEOSLIDER), TBM_SETPOS, TRUE, image_video_frame);
    UpdateHMSF(hDlg);
    char tbuf[32];
    sprintf_s(tbuf, 32, "%i:%02i:%02i:%02i", image_video_hour, image_video_minute, image_video_second, image_video_frame);
    SetDlgItemTextA(hDlg, IDC_CURSORTIME, tbuf);
    glfwMakeContextCurrent(glfwimages);
    if (TxImage != (UINT)-1)
    {
        image_mat.release();
        glDeleteTextures(1, &TxImage);
    }
    TxImage = CreateTextureFromVideo(image_video_cap, &width_image, &height_image);
}
void BubbleSort(UINT* arr, UINT n)
{
    UINT temp;
    for (UINT ti = 0; ti < n - 1; ti++)
    {
        for (UINT tj = 0; tj < n - ti - 1; tj++)
        {
            if (arr[tj] > arr[tj + 1])
            {
                temp = arr[tj];
                arr[tj] = arr[tj + 1];
                arr[tj + 1] = temp;
            }
        }
    }
}

void ScrollCopy(HWND hDlg)
{
    if (MycRom.name[0] == 0) return;
    if ((WiSelection == 0) || (HiSelection == 0))
    {
        MessageBoxA(hImages, "No selection has been done in the frame", "Failed", MB_OK);
        return;
    }
    if ((!image_zoom_srce) || (!image_zoom_dest))
    {
        MessageBoxA(hImages, "You need to define an initial and final source image for the scroll", "Failed", MB_OK);
        return;
    }
    if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_UNCHECKED && Button_GetCheck(GetDlgItem(hwTB3, IDC_TOORGRES)) == BST_UNCHECKED)
    {
        MessageBoxA(hImages, "Select at least a resolution as destination", "Faile" ,MB_OK);
        return;
    }
    UINT tSelFrames[MAX_SEL_FRAMES];
    for (UINT ti = 0; ti < nSelFrames; ti++) tSelFrames[ti] = SelFrames[ti];
    BubbleSort(tSelFrames, nSelFrames);
    /*UINT preframe = acFrame, derframe = acFrame;
    bool frfound = true;
    while (frfound)
    {
        frfound = false;
        for (UINT ti = 0; ti < nSelFrames; ti++)
        {
            if (derframe < MycRom.nFrames - 1)
            {
                if (SelFrames[ti] == derframe + 1)
                {
                    derframe++;
                    frfound = true;
                }
            }
        }
    }
    UINT nfr = derframe - preframe + 1;*/
    UINT8* image = NULL;
    float ratio = (float)width_image / (float)image_sizeW;
    float crop_minx = ratio * min(crop_ioffsetx, crop_foffsetx), crop_miny = ratio * min(crop_ioffsety, crop_foffsety);
    float crop_maxx = ratio * max(crop_ioffsetx + crop_iOsizeW - 1, crop_foffsetx + crop_fOsizeW - 1), crop_maxy = ratio * max(crop_ioffsety + crop_iOsizeH - 1, crop_foffsety + crop_fOsizeH - 1);
    image = (UINT8*)malloc(WiSelection * HiSelection);
    if (!image)
    {
        MessageBoxA(hImages, "Cant' get memory for scroll copy", "Failed", MB_OK);
        return;
    }
    SaveAction(true, SA_DRAW);
    EnableWindow(GetDlgItem(hDlg, IDC_SCROLLCOPY), FALSE);

    float crop_isemiw = ratio * crop_iOsizeW / 2, crop_isemih = ratio * crop_iOsizeH / 2;
    float crop_fsemiw = ratio * crop_fOsizeW / 2, crop_fsemih = ratio * crop_fOsizeH / 2;
    float centreix = ratio * crop_ioffsetx + crop_isemiw, centreiy = ratio * crop_ioffsety + crop_isemih;
    float centrefx = ratio * crop_foffsetx + crop_fsemiw, centrefy = ratio * crop_foffsety + crop_fsemih;
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
        {
            MycRom.isExtraFrame[SelFrames[ti]] = 1;
            CheckDlgButton(hwTB, IDC_EXTRARES, BST_CHECKED);
        }
        float accentrex = centreix + (centrefx - centreix) * (float)ti / (float)(nSelFrames - 1);
        float accentrey = centreiy + (centrefy - centreiy) * (float)ti / (float)(nSelFrames - 1);
        float acsemiw = crop_isemiw + (crop_fsemiw - crop_isemiw) * (float)ti / (float)(nSelFrames - 1);
        float acsemih = crop_isemih + (crop_fsemih - crop_isemih) * (float)ti / (float)(nSelFrames - 1);
        cv::Rect croprect((int)(accentrex - acsemiw), (int)(accentrey - acsemih), (int)(2 * acsemiw), (int)(2 * acsemih));
        cv::Mat croppedimg = image_mat(croprect);
        cv::Mat tmp64;
        cv::resize(croppedimg, tmp64, cv::Size(WiSelection, HiSelection), 0, 0, ImgResizeFilter);
        cv::Mat tmp32;
        cv::resize(croppedimg, tmp32, cv::Size(WiSelection / 2, HiSelection / 2), 0, 0, ImgResizeFilter);
        croppedimg.release();
        UINT fw64, fw32;
        UINT16* pfr32 = NULL, * pfr64 = NULL;
        bool is64 = false, is32 = false;
        if (MycRom.fHeightX == 32 && Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
        {
            is32 = true;
            fw32 = MycRom.fWidthX;
            fw64 = MycRom.fWidth;
            if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
                pfr32 = &MycRom.BackgroundFramesX[(tSelFrames[ti]) * fw32 * 32];
            else
                pfr32 = &MycRom.cFramesX[(tSelFrames[ti]) * fw32 * 32];
        }
        else if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOXTRARES)) == BST_CHECKED)
        {
            is64 = true;
            fw64 = MycRom.fWidthX;
            fw32 = MycRom.fWidth;
            if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
                pfr64 = &MycRom.BackgroundFramesX[(tSelFrames[ti]) * fw64 * 64];
            else
                pfr64 = &MycRom.cFramesX[(tSelFrames[ti]) * fw64 * 64];
        }
        if (MycRom.fHeight == 32 && Button_GetCheck(GetDlgItem(hwTB3, IDC_TOORGRES)) == BST_CHECKED)
        {
            is32 = true;
            fw32 = MycRom.fWidth;
            fw64 = MycRom.fWidthX;
            if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
                pfr32 = &MycRom.BackgroundFrames[(tSelFrames[ti]) * fw32 * 32];
            else
                pfr32 = &MycRom.cFrames[(tSelFrames[ti]) * fw32 * 32];
        }
        else if (Button_GetCheck(GetDlgItem(hwTB3, IDC_TOORGRES)) == BST_CHECKED)
        {
            is64 = true;
            fw64 = MycRom.fWidth;
            fw32 = MycRom.fWidthX;
            if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED)
                pfr64 = &MycRom.BackgroundFrames[(tSelFrames[ti]) * fw64 * 64];
            else
                pfr64 = &MycRom.cFrames[(tSelFrames[ti]) * fw64 * 64];
        }
        for (UINT tj = 0; tj < HiSelection; tj++)
        {
            for (UINT tk = 0; tk < WiSelection; tk++)
            {
                if (is64)
                {
                    if (Copy_iMask[(YiSelection + tj) * fw64 + XiSelection + tk] != 0)
                    {
                        cv::Vec3b color = tmp64.at<cv::Vec3b>(tj, tk);
                        pfr64[(YiSelection + tj) * fw64 + (XiSelection + tk)] = rgb888_to_rgb565(color[2], color[1], color[0]);
                    }
                }
                if (is32)
                {
                    if (Copy_iMask[(YiSelection + tj) * fw64 + XiSelection + tk] != 0)
                    {
                        cv::Vec3b color = tmp32.at<cv::Vec3b>(tj / 2, tk / 2);
                        pfr32[(YiSelection + tj) / 2 * fw32 + (XiSelection + tk) / 2] = rgb888_to_rgb565(color[2], color[1], color[0]);
                    }
                }
            }
        }
        tmp64.release();
        tmp32.release();
    }
    EnableWindow(GetDlgItem(hDlg, IDC_SCROLLCOPY), TRUE);
    free(image);
    InvalidateRect(hwTB, NULL, TRUE);
    UpdateFSneeded = true;
}

void strtrim(char* str, int maxlength)
{
    if (str[0] == 0) return;
    char* tstr = new char[maxlength];
    strcpy_s(tstr, maxlength, str);
    int ti = 0, tj = min((int)strlen(tstr), maxlength - 1);
    while (tstr[ti] <= ' ' && ti < maxlength - 1) ti++;
    while (tstr[tj] <= ' ' && tj > ti) tj--;
    if (ti == maxlength - 1 || tj == ti) str[0] = 0;
    else
    {
        int tl = 0;
        for (int tk = ti; tk <= tj; tk++)
        {
            str[tl] = tstr[tk];
            tl++;
        }
        str[tl] = 0;
    }
    delete tstr;
}

void InitCropZoneList(void)
{
    SendMessage(GetDlgItem(hwTB3, IDC_CZONELIST), CB_RESETCONTENT, 0, 0);
    for (UINT ti = 0; ti < MycRP.nImagePosSaves; ti++)
        SendMessageA(GetDlgItem(hwTB3, IDC_CZONELIST), CB_ADDSTRING, 0, (LPARAM)&MycRP.ImagePosSaveName[64*ti]);
}
POINT g_ptPicControl;
INT_PTR CALLBACK Toolbar_Proc3(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CTLCOLORDLG:
        {
            if (Night_Mode) return (INT_PTR)GetStockObject(DKGRAY_BRUSH);
            return (INT_PTR)GetStockObject(GRAY_BRUSH);
        }
        case WM_INITDIALOG:
        {
            SetWindowSubclass(GetDlgItem(hDlg, IDC_BROWSEIMAGE), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_CBPASTE), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_COPY), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_ZOOMIN), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_ZOOMOUT), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_HOUR), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_MINUTE), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_SECOND), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_FRAME), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_VIDEOSLIDER), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_FRAMEDUR), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_REGULDUR), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_CURFRAMEALL), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_INTERV), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_BRIGHTNESS), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_CONTRAST), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_SCROLLCOPY), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_PREV), ButtonSubclassProc3, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_NEXT), ButtonSubclassProc3, 0, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_VIDEOSLIDER), FALSE);
            //EnableWindow(GetDlgItem(hDlg, IDC_TIMESPIN), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_HOUR), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_MINUTE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_SECOND), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_FRAME), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_FRAMEDUR), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_REGULDUR), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CURFRAMEALL), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_PREV), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_NEXT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_INTERV), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BRIGHTNESS), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CONTRAST), FALSE);
            SendMessage(GetDlgItem(hDlg, IDC_BRIGHTNESS), TBM_SETRANGE, TRUE, MAKELONG(-50, 50));
            SendMessage(GetDlgItem(hDlg, IDC_CONTRAST), TBM_SETRANGE, TRUE, MAKELONG(-15, 20));
            SendMessage(GetDlgItem(hDlg, IDC_BLUR), TBM_SETRANGE, TRUE, MAKELONG(0, 10));
            SendMessage(GetDlgItem(hDlg, IDC_BRIGHTNESS), TBM_SETPOS, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_CONTRAST), TBM_SETPOS, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_BLUR), TBM_SETPOS, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_INTERV), EM_SETLIMITTEXT, 4, 0);
            SendMessage(GetDlgItem(hDlg, IDC_CZONENAME), EM_SETLIMITTEXT, 63, 0);
            InitCropZoneList();
            image_org_mat.data = NULL;
            return TRUE;
        }
        case WM_HSCROLL:
        {
            glfwMakeContextCurrent(glfwimages);
            if (!image_loaded) return TRUE;
            int controlId = GetDlgCtrlID((HWND)lParam);
            if (controlId == IDC_VIDEOSLIDER) UpdateHMSFfromSlider(hDlg);
            else if (controlId == IDC_BRIGHTNESS)
            {
                image_brightness = (int)SendMessage(GetDlgItem(hDlg, IDC_BRIGHTNESS), TBM_GETPOS, 0, 0);
                char tbuf[64];
                sprintf_s(tbuf, 64, "Brightness %i", (int)(2.5 * image_brightness));
                SetDlgItemTextA(hDlg, IDC_STATICB, tbuf);
                if (TxImage != (UINT)-1)
                {
                    image_mat.release();
                    glDeleteTextures(1, &TxImage);
                }
                if (image_source_format_video) TxImage = CreateTextureFromVideo(image_video_cap, &width_image, &height_image);
                else if (!image_org_mat.data) TxImage = CreateTextureFromImage(image_path, &width_image, &height_image);
                else TxImage = CreateTextureFromMat();

            }
            else if (controlId == IDC_CONTRAST)
            {
                image_contrast = (int)SendMessage(GetDlgItem(hDlg, IDC_CONTRAST), TBM_GETPOS, 0, 0);
                char tbuf[64];
                double alpha = 1 + 0.05 * image_contrast;
                sprintf_s(tbuf, 64, "Contrast %.2f", alpha);
                SetDlgItemTextA(hDlg, IDC_STATICC, tbuf);
                if (TxImage != (UINT)-1)
                {
                    image_mat.release();
                    glDeleteTextures(1, &TxImage);
                }
                if (image_source_format_video) TxImage = CreateTextureFromVideo(image_video_cap, &width_image, &height_image);
                else if (!image_org_mat.data) TxImage = CreateTextureFromImage(image_path, &width_image, &height_image);
                else TxImage = CreateTextureFromMat();
            }
            else if (controlId == IDC_BLUR)
            {
                image_blur = (int)SendMessage(GetDlgItem(hDlg, IDC_BLUR), TBM_GETPOS, 0, 0);
                char tbuf[64];
                sprintf_s(tbuf, 64, "Blur %i", image_blur);
                SetDlgItemTextA(hDlg, IDC_STATICBL, tbuf);
                if (TxImage != (UINT)-1)
                {
                    image_mat.release();
                    glDeleteTextures(1, &TxImage);
                }
                if (image_source_format_video) TxImage = CreateTextureFromVideo(image_video_cap, &width_image, &height_image);
                else if (!image_org_mat.data) TxImage = CreateTextureFromImage(image_path, &width_image, &height_image);
                else TxImage = CreateTextureFromMat();
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BROWSEIMAGE:
                {
                    glfwMakeContextCurrent(glfwimages);
                    OPENFILENAMEA ofn;
                    char szFile[260];
                    strcpy_s(szFile, 260, Dir_Images);
                    strcat_s(szFile, 260, "*.jpg;*.jpeg;*.png;*.bmp");

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.lpstrTitle = "Choose an image file";
                    ofn.hwndOwner = hImages;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Image (.jpg;.jpeg;.png;.bmp)\0*.jpg;*.jpeg;*.png;*.bmp\0Video (.avi;.mpg;.mpeg;.mp4;.mov)\0*.avi;*.mpg;*.mpeg;*.mp4;*.mov\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrInitialDir = Dir_Images;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                    if (GetOpenFileNameA(&ofn) == TRUE)
                    {
                        strcpy_s(Dir_Images, 260, ofn.lpstrFile);
                        int i = (int)strlen(Dir_Images) - 1;
                        while ((i > 0) && (Dir_Images[i] != '\\')) i--;
                        Dir_Images[i + 1] = 0;
                        SavePaths();
                        Mat tmat = imread(ofn.lpstrFile);
                        if (tmat.data != NULL)
                        {
                            tmat.release();
                            if (TxImage != (UINT)-1)
                            {
                                glfwMakeContextCurrent(glfwimages);
                                image_mat.release();
                                if (image_org_mat.data) image_org_mat.release();
                                glDeleteTextures(1, &TxImage);
                                if (image_source_format_video) image_video_cap.release();
                            }
                            image_source_format_video = false;
                            EnableWindow(GetDlgItem(hDlg, IDC_BRIGHTNESS), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_CONTRAST), TRUE);
                            strcpy_s(image_path, MAX_PATH, ofn.lpstrFile);
                            TxImage = CreateTextureFromImage(ofn.lpstrFile, &width_image, &height_image);
                            float imgratio = (float)width_image / (float)height_image;
                            if (imgratio > (float)ScrW3 / (float)ScrH3)
                            {
                                image_sizeW = ScrW3;
                                image_sizeH = (UINT)((float)image_sizeW / imgratio);
                            }
                            else
                            {
                                image_sizeH = ScrH3;
                                image_sizeW = (UINT)((float)image_sizeH * imgratio);
                            }
                            image_posx = (ScrW3 - image_sizeW) / 2;
                            image_posy = (ScrH3 - image_sizeH) / 2;
                            EnableWindow(GetDlgItem(hDlg, IDC_VIDEOSLIDER), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_HOUR), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_MINUTE), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_SECOND), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_FRAME), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_FRAMEDUR), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_REGULDUR), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_CURFRAMEALL), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_INTERV), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_PREV), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_NEXT), FALSE);
                        }
                        else
                        {
                            cv::VideoCapture cap(ofn.lpstrFile);
                            if (!cap.isOpened())
                            {
                                MessageBoxA(hImages, "Can't open the file", "Failed", MB_OK);
                                return false;
                            }
                            cv::Mat tmat;
                            cap >> tmat;
                            if (tmat.empty())
                            {
                                cap.release();
                                tmat.release();
                                return TRUE;
                            }
                            tmat.release();
                            if (TxImage != (UINT)-1)
                            {
                                glfwMakeContextCurrent(glfwimages);
                                image_mat.release();
                                if (image_org_mat.data) image_org_mat.release();
                                glDeleteTextures(1, &TxImage);
                                if (image_source_format_video) image_video_cap.release();
                            }
                            EnableWindow(GetDlgItem(hDlg, IDC_BRIGHTNESS), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_CONTRAST), TRUE);
                            image_mat = tmat;
                            image_source_format_video = true;
                            image_video_cap.open(ofn.lpstrFile);
                            image_video_hour = image_video_minute = image_video_second = image_video_frame = 0;
                            TxImage = CreateTextureFromVideo(image_video_cap, &width_image, &height_image);
                            EnableWindow(GetDlgItem(hDlg, IDC_VIDEOSLIDER), TRUE);
                            //EnableWindow(GetDlgItem(hDlg, IDC_TIMESPIN), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_HOUR), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_MINUTE), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_SECOND), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_FRAME), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_FRAMEDUR), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_REGULDUR), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_CURFRAMEALL), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_INTERV), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_PREV), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_NEXT), TRUE);
                            CheckDlgButton(hDlg, IDC_HOUR, TRUE);
                            CheckDlgButton(hDlg, IDC_MINUTE, FALSE);
                            CheckDlgButton(hDlg, IDC_SECOND, FALSE);
                            CheckDlgButton(hDlg, IDC_FRAME, FALSE);
                            CheckDlgButton(hDlg, IDC_FRAMEDUR, TRUE);
                            CheckDlgButton(hDlg, IDC_REGULDUR, FALSE);
                            CheckDlgButton(hDlg, IDC_CURFRAMEALL, FALSE);
                            SetDlgItemTextA(hDlg, IDC_CURSORTIME, "0:00:00:00");
                            int total_frames = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
                            LongToHMSF((long)total_frames, &image_video_nhours, &image_video_nminutes, &image_video_nseconds, &image_video_nframes);
                            UpdateHMSF(hDlg);
                            SendMessage(GetDlgItem(hDlg, IDC_VIDEOSLIDER), TBM_SETTICFREQ, 1, 0);
                            char tbuf[32];
                            sprintf_s(tbuf, 32, "%i:%02i:%02i:%02i", image_video_nhours, image_video_nminutes, image_video_nseconds, image_video_nframes);
                            SetDlgItemTextA(hDlg, IDC_TOTALTIME, tbuf);
                            SetDlgItemTextA(hDlg, IDC_CURSORTIME, "0:00:00:00");
                            float imgratio = (float)width_image / (float)height_image;
                            if (imgratio > (float)ScrW3 / (float)ScrH3)
                            {
                                image_sizeW = ScrW3;
                                image_sizeH = (UINT)((float)image_sizeW / imgratio);
                            }
                            else
                            {
                                image_sizeH = ScrH3;
                                image_sizeW = (UINT)((float)image_sizeH * imgratio);
                            }
                            image_posx = (ScrW3 - image_sizeW) / 2;
                            image_posy = (ScrH3 - image_sizeH) / 2;
                        }
                    }
                    GetSelectionSize();
                    UpdateCropSize();
                    return TRUE;
                }
                case IDC_SCROLLCOPY:
                {
                    glfwMakeContextCurrent(glfwimages);
                    if (nSelFrames == 1)
                    {
                        SendMessage(hDlg, WM_COMMAND, IDC_COPY, (LPARAM)GetDlgItem(hDlg, IDC_COPY));
                        return TRUE;
                    }
                    ScrollCopy(hDlg);
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_CBPASTE:
                {
                    glfwMakeContextCurrent(glfwimages);
                    if (TxImage != (UINT)-1)
                    {
                        glfwMakeContextCurrent(glfwimages);
                        image_mat.release();
                        if (image_org_mat.data) image_org_mat.release();
                        glDeleteTextures(1, &TxImage);
                        if (image_source_format_video) image_video_cap.release();
                    }
                    image_source_format_video = false;
                    TxImage = CreateTextureFromClipboard(&width_image, &height_image);
                    if (TxImage == (UINT)-1) return TRUE;
                    EnableWindow(GetDlgItem(hDlg, IDC_BRIGHTNESS), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_CONTRAST), TRUE);
                    float imgratio = (float)width_image / (float)height_image;
                    if (imgratio > (float)ScrW3 / (float)ScrH3)
                    {
                        image_sizeW = ScrW3;
                        image_sizeH = (UINT)((float)image_sizeW / imgratio);
                    }
                    else
                    {
                        image_sizeH = ScrH3;
                        image_sizeW = (UINT)((float)image_sizeH * imgratio);
                    }
                    image_posx = (ScrW3 - image_sizeW) / 2;
                    image_posy = (ScrH3 - image_sizeH) / 2;
                    GetSelectionSize();
                    UpdateCropSize();
                    return TRUE;
                }
                case IDC_ZOOMIN:
                {
                    mouse_scroll_callback3(glfwimages, 0, -1);
                    return TRUE;
                }
                case IDC_ZOOMOUT:
                {
                    mouse_scroll_callback3(glfwimages, 0, 1);
                    return TRUE;
                }
                case IDC_COPY:
                {
                    if (!image_loaded || (MycRom.name[0] == 0)) return TRUE;
                    if (!image_zoom_srce)
                    {
                        MessageBoxA(hImages, "No source zone defined for the copy", "Failed", MB_OK);
                        return TRUE;
                    }
                    glfwMakeContextCurrent(glfwimages);
                    if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOFRAME)) == BST_CHECKED)
                    {
                        UINT fw, fh;
                        if (nEditExtraResolutionF)
                        {
                            fw = MycRom.fWidthX;
                            fh = MycRom.fHeightX;
                        }
                        else
                        {
                            fw = MycRom.fWidth;
                            fh = MycRom.fHeight;
                        }
                        if ((WiSelection < 4) || (HiSelection < 4) || ((int)WiSelection > max((int)MycRom.fWidthX, (int)MycRom.fWidth)) || ((int)HiSelection > max((int)MycRom.fHeightX, (int)MycRom.fHeight)))
                        {
                            MessageBoxA(hImages, "The selection must be at least 4x4 pixels in the frame window to use this function", "Failed", MB_OK);
                            return TRUE;
                        }
                        SaveAction(true, SA_DRAW);
                    }
                    else
                    {
                        if (MycRom.nBackgrounds == 0) return TRUE;
                        SaveAction(true, SA_REIMPORTBACKGROUND);
                    }
                    float ratio = (float)width_image / (float)image_sizeW;
                    if ((!image_source_format_video) || (nSelFrames == 1) || (SendMessage(GetDlgItem(hDlg, IDC_CURFRAMEALL), BM_GETCHECK, 0, 0) == BST_CHECKED) || (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOBG)) == BST_CHECKED))
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_COPY), FALSE);
                        int ix = (int)(crop_ioffsetx * ratio), iy = (int)(crop_ioffsety * ratio), wid = (int)(crop_iOsizeW * ratio), hei = (int)(crop_iOsizeH * ratio);
                        if (ix + wid > image_mat.cols) wid = image_mat.cols - ix;
                        if (iy + hei > image_mat.rows) hei = image_mat.rows - iy;
                        cv::Rect croprect(ix, iy, wid, hei);
                        cv::Mat croppedimg = image_mat(croprect);
                        cv::Mat tmp64;
                        cv::resize(croppedimg, tmp64, cv::Size(WiSelection, HiSelection), 0, 0, ImgResizeFilter);
                        cv::Mat tmp32;
                        cv::resize(croppedimg, tmp32, cv::Size(WiSelection / 2, HiSelection / 2), 0, 0, ImgResizeFilter);
                        croppedimg.release();
                        if (Button_GetCheck(GetDlgItem(hwTB3, IDC_IMGTOFRAME)) == BST_CHECKED) CopyImageToSelection(tmp32, tmp64);
                        else 
                            CopyImageToBackground(tmp32, tmp64);
                        tmp32.release();
                        tmp64.release();
                        EnableWindow(GetDlgItem(hDlg, IDC_COPY), TRUE);
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_COPY), FALSE);
                        unsigned int preframe = acFrame, derframe = acFrame;
                        bool frfound = true;
                        while (frfound)
                        {
                            frfound = false;
                            for (UINT ti = 0; ti < nSelFrames; ti++)
                            {
                                if (preframe > 0)
                                {
                                    if (SelFrames[ti] == preframe - 1)
                                    {
                                        preframe--;
                                        frfound = true;
                                    }
                                }
                                if (derframe < MycRom.nFrames - 1)
                                {
                                    if (SelFrames[ti] == derframe + 1)
                                    {
                                        derframe++;
                                        frfound = true;
                                    }
                                }
                            }
                        }
                        double acposs = (double)HMSFToLong(image_video_hour, image_video_minute, image_video_second, image_video_frame);
                        double finposs = (double)HMSFToLong(image_video_nhours, image_video_nminutes, image_video_nseconds, image_video_nframes);
                        double msstep = image_video_frame_rate / 1000.0; // frames/ms
                        double interv;
                        UINT8 ih = image_video_hour, im = image_video_minute, is = image_video_second, ifr = image_video_frame;
                        UINT iinterv;
                        if (SendMessage(GetDlgItem(hDlg, IDC_REGULDUR), BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            char tbuf[8];
                            GetDlgItemTextA(hDlg, IDC_INTERV, tbuf, 8);
                            iinterv = (UINT)atoi(tbuf);
                            interv = (double)iinterv;
                            if (interv < 5)
                            {
                                SetDlgItemTextA(hDlg, IDC_INTERV, "5");
                                MessageBoxA(hImages, "The value in the interval was too low, it has been reset, confirm again", "Error", MB_OK);
                                return TRUE;
                            }
                            else if (interv > 3000)
                            {
                                SetDlgItemTextA(hDlg, IDC_INTERV, "3000");
                                MessageBoxA(hImages, "The value in the interval was too high, it has been reset, confirm again", "Error", MB_OK);
                                return TRUE;
                            }
                            interv *= msstep;
                        }
                        do
                        {
                            LongToHMSF((long)acposs, &image_video_hour, &image_video_minute, &image_video_second, &image_video_frame);
                            image_mat.release();
                            CreateMatFromVideo(image_video_cap);
                            int ix = (int)(crop_ioffsetx * ratio), iy = (int)(crop_ioffsety * ratio), wid = (int)(crop_iOsizeW * ratio), hei = (int)(crop_iOsizeH * ratio);
                            if (ix + wid > image_mat.cols) wid = image_mat.cols - ix;
                            if (iy + hei > image_mat.rows) hei = image_mat.rows - iy;
                            cv::Rect croprect(ix, iy, wid, hei);
                            cv::Mat croppedimg = image_mat(croprect);
                            cv::Mat tmp64;
                            cv::resize(croppedimg, tmp64, cv::Size(WiSelection, HiSelection), 0, 0, ImgResizeFilter);
                            cv::Mat tmp32;
                            cv::resize(croppedimg, tmp32, cv::Size(WiSelection / 2, HiSelection / 2), 0, 0, ImgResizeFilter);
                            croppedimg.release();
                            //unsigned char palette[64 * 3];
                            //unsigned char image[256 * 64];
                            //if (image_ncolsel > NiSelection) image_ncolsel = NiSelection;
                            //ReduceRGB24ToNColorsImage(tmp, image_ncolsel, palette, image);
                            CopyImageTo1Selection(tmp32, tmp64, preframe);
                            tmp32.release();
                            tmp64.release();
                            if (SendMessage(GetDlgItem(hDlg, IDC_FRAMEDUR), BM_GETCHECK, 0, 0) == BST_CHECKED)
                                acposs += MycRP.FrameDuration[preframe] * msstep;
                            else
                                acposs += interv;
                            preframe++;
                        } while ((preframe <= derframe) && (acposs < finposs));
                        image_video_hour = ih;
                        image_video_minute = im;
                        image_video_second = is;
                        image_video_frame = ifr;
                        CreateMatFromVideo(image_video_cap);
                    }
                    InvalidateRect(hwTB, NULL, TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_COPY), TRUE);
                    UpdateFSneeded = true;
                    UpdateBSneeded = true;
                    return TRUE;
                }
                case IDC_HOUR:
                case IDC_MINUTE:
                case IDC_SECOND:
                case IDC_FRAME:
                {
                    UpdateHMSF(hDlg);
                    return TRUE;
                }
                case IDC_FRAMEDUR:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_INTERV), FALSE);
                    return TRUE;
                }
                case IDC_REGULDUR:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_INTERV), TRUE);
                    SetDlgItemTextA(hDlg, IDC_INTERV, "20");
                    return TRUE;
                }
                case IDC_NEXT:
                {
                    VideoMove(hDlg, +1);
                    return TRUE;
                }
                case IDC_PREV:
                {
                    VideoMove(hDlg, -1);
                    return TRUE;
                }
                case IDC_CZONELIST:
                {
                    if (TxImage == -1) return TRUE;
                    int wmEvent = HIWORD(wParam);
                    if (wmEvent != CBN_SELCHANGE) return TRUE;
                    int posl = (int)SendMessage(GetDlgItem(hwTB3, IDC_CZONELIST), CB_GETCURSEL, 0, 0);
                    if (posl == -1) return TRUE;
                    SetDlgItemTextA(hwTB3, IDC_CZONENAME, &MycRP.ImagePosSaveName[posl * 64]);
                    if (MycRP.ImagePosSave[16 * posl + 5] > 0) image_zoom_srce = true; else image_zoom_srce = false;
                    glfwMakeContextCurrent(glfwimages);
                    if (image_zoom_srce)
                    {
                        crop_reductioni = MycRP.ImagePosSave[16 * posl];
                        crop_ioffsetx = MycRP.ImagePosSave[16 * posl + 1];
                        crop_ioffsety = MycRP.ImagePosSave[16 * posl + 2];
                        crop_iOsizeW = MycRP.ImagePosSave[16 * posl + 3];
                        crop_iOsizeH = MycRP.ImagePosSave[16 * posl + 4];
                    }
                    if (MycRP.ImagePosSave[16 * posl + 11] > 0) image_zoom_dest = true; else image_zoom_dest = false;
                    if (image_zoom_dest)
                    {
                        crop_reductionf = MycRP.ImagePosSave[16 * posl + 6];
                        crop_foffsetx = MycRP.ImagePosSave[16 * posl + 7];
                        crop_foffsety = MycRP.ImagePosSave[16 * posl + 8];
                        crop_fOsizeW = MycRP.ImagePosSave[16 * posl + 9];
                        crop_fOsizeH = MycRP.ImagePosSave[16 * posl + 10];
                    }
                    SendMessage(GetDlgItem(hwTB3, IDC_BRIGHTNESS), TBM_SETPOS, TRUE, (LPARAM)MycRP.ImagePosSave[16 * posl + 12]);
                    image_brightness = MycRP.ImagePosSave[16 * posl + 12];
                    SendMessage(GetDlgItem(hwTB3, IDC_CONTRAST), TBM_SETPOS, TRUE, (LPARAM)MycRP.ImagePosSave[16 * posl + 13]);
                    image_contrast = MycRP.ImagePosSave[16 * posl + 13];
                    SendMessage(GetDlgItem(hwTB3, IDC_BLUR), TBM_SETPOS, TRUE, (LPARAM)MycRP.ImagePosSave[16 * posl + 14]);
                    image_blur = MycRP.ImagePosSave[16 * posl + 14];
                    char tbuf[64];
                    sprintf_s(tbuf, 64, "Brightness %i", (int)(2.5 * image_brightness));
                    SetDlgItemTextA(hDlg, IDC_STATICB, tbuf);
                    double alpha = 1 + 0.05 * image_contrast;
                    sprintf_s(tbuf, 64, "Contrast %.2f", alpha);
                    SetDlgItemTextA(hDlg, IDC_STATICC, tbuf);
                    sprintf_s(tbuf, 64, "Blur %i", image_blur);
                    SetDlgItemTextA(hDlg, IDC_STATICBL, tbuf);
                    if (TxImage != (UINT)-1)
                    {
                        image_mat.release();
                        glDeleteTextures(1, &TxImage);
                    }
                    if (image_source_format_video) TxImage = CreateTextureFromVideo(image_video_cap, &width_image, &height_image);
                    else if (!image_org_mat.data) TxImage = CreateTextureFromImage(image_path, &width_image, &height_image);
                    else TxImage = CreateTextureFromMat();
                    crop_reduction = 0;
                    CalcImageCropFrame();
                    UpdateCropSize();
                    return TRUE;
                }
                case IDC_CZONENAME:
                {
                    return TRUE;
                }
                case IDC_ADDCZONE:
                {
                    if (TxImage == -1) return TRUE;
                    if (MycRP.nImagePosSaves == N_IMAGE_POS_TO_SAVE)
                    {
                        MessageBoxA(hwTB3, "Maximum number of crop zone saves reached.", "Failed", MB_OK);
                        return TRUE;
                    }
                    int posl = MycRP.nImagePosSaves;// (int)SendMessage(GetDlgItem(hwTB3, IDC_CZONELIST), CB_GETCURSEL, 0, 0
                    //if (posl == -1) return TRUE;
                    GetDlgItemTextA(hwTB3, IDC_CZONENAME, &MycRP.ImagePosSaveName[posl * 64], 63);
                    strtrim(&MycRP.ImagePosSaveName[posl * 64], 64);
                    if (MycRP.ImagePosSaveName[posl * 64] == 0)
                    {
                        MessageBoxA(hwTB3, "Please give a name to that crop zone save.", "Failed", MB_OK);
                        return TRUE;
                    }
                    for (UINT ti = 0; ti < MycRP.nImagePosSaves; ti++)
                    {
                        if (strcmp(&MycRP.ImagePosSaveName[posl * 64],&MycRP.ImagePosSaveName[ti * 64])==0)
                        {
                            MessageBoxA(hwTB3, "There is already one crop zone save with that name.", "Failed", MB_OK);
                            return TRUE;
                        }
                    }
                    MycRP.ImagePosSave[16 * posl] = crop_reductioni;
                    MycRP.ImagePosSave[16 * posl + 1] = crop_ioffsetx;
                    MycRP.ImagePosSave[16 * posl + 2] = crop_ioffsety;
                    MycRP.ImagePosSave[16 * posl + 3] = crop_iOsizeW;
                    MycRP.ImagePosSave[16 * posl + 4] = crop_iOsizeH;
                    if (image_zoom_srce) MycRP.ImagePosSave[16 * posl + 5] = 1; else MycRP.ImagePosSave[16 * posl + 5] = 0;
                    MycRP.ImagePosSave[16 * posl + 6] = crop_reductionf;
                    MycRP.ImagePosSave[16 * posl + 7] = crop_foffsetx;
                    MycRP.ImagePosSave[16 * posl + 8] = crop_foffsety;
                    MycRP.ImagePosSave[16 * posl + 9] = crop_fOsizeW;
                    MycRP.ImagePosSave[16 * posl + 10] = crop_fOsizeH;
                    if (image_zoom_dest) MycRP.ImagePosSave[16 * posl + 11] = 1; else MycRP.ImagePosSave[16 * posl + 11] = 0;
                    MycRP.ImagePosSave[16 * posl + 12] = (int)SendMessage(GetDlgItem(hwTB3, IDC_BRIGHTNESS), TBM_GETPOS, 0, 0);
                    MycRP.ImagePosSave[16 * posl + 13] = (int)SendMessage(GetDlgItem(hwTB3, IDC_CONTRAST), TBM_GETPOS, 0, 0);
                    MycRP.ImagePosSave[16 * posl + 14] = (int)SendMessage(GetDlgItem(hwTB3, IDC_BLUR), TBM_GETPOS, 0, 0);
                    MycRP.nImagePosSaves++;
                    InitCropZoneList();
                    return TRUE;
                }
                case IDC_DELCZONE:
                {
                    if (TxImage == -1) return TRUE;
                    int posl = (int)SendMessage(GetDlgItem(hwTB3, IDC_CZONELIST), CB_GETCURSEL, 0, 0);
                    if (posl == -1) return TRUE;
                    for (UINT ti = posl; ti < MycRP.nImagePosSaves - 1; ti++)
                    {
                        strcpy_s(&MycRP.ImagePosSaveName[ti * 64], 63, &MycRP.ImagePosSaveName[(ti + 1) * 64]);
                        MycRP.ImagePosSave[16 * ti] = MycRP.ImagePosSave[16 * (ti + 1)];
                        MycRP.ImagePosSave[16 * ti + 1] = MycRP.ImagePosSave[16 * (ti + 1) + 1];
                        MycRP.ImagePosSave[16 * ti + 2] = MycRP.ImagePosSave[16 * (ti + 1) + 2];
                        MycRP.ImagePosSave[16 * ti + 3] = MycRP.ImagePosSave[16 * (ti + 1) + 3];
                        MycRP.ImagePosSave[16 * ti + 4] = MycRP.ImagePosSave[16 * (ti + 1) + 4];
                        MycRP.ImagePosSave[16 * ti + 5] = MycRP.ImagePosSave[16 * (ti + 1) + 5];
                        MycRP.ImagePosSave[16 * ti + 6] = MycRP.ImagePosSave[16 * (ti + 1) + 6];
                        MycRP.ImagePosSave[16 * ti + 7] = MycRP.ImagePosSave[16 * (ti + 1) + 7];
                        MycRP.ImagePosSave[16 * ti + 8] = MycRP.ImagePosSave[16 * (ti + 1) + 8];
                        MycRP.ImagePosSave[16 * ti + 9] = MycRP.ImagePosSave[16 * (ti + 1) + 9];
                        MycRP.ImagePosSave[16 * ti + 10] = MycRP.ImagePosSave[16 * (ti + 1) + 10];
                        MycRP.ImagePosSave[16 * ti + 11] = MycRP.ImagePosSave[16 * (ti + 1) + 11];
                        MycRP.ImagePosSave[16 * ti + 12] = MycRP.ImagePosSave[16 * (ti + 1) + 12];
                        MycRP.ImagePosSave[16 * ti + 13] = MycRP.ImagePosSave[16 * (ti + 1) + 13];
                        MycRP.ImagePosSave[16 * ti + 14] = MycRP.ImagePosSave[16 * (ti + 1) + 14];
                    }
                    MycRP.nImagePosSaves--;
                    InitCropZoneList();
                    return TRUE;
                }
                case IDC_IMGTOFRAME:
                {
                    EnableWindow(GetDlgItem(hwTB3, IDC_SCROLLCOPY), TRUE);
                    GetSelectionSize();
                    return TRUE;
                }
                case IDC_IMGTOBG:
                {
                    EnableWindow(GetDlgItem(hwTB3, IDC_SCROLLCOPY), FALSE);
                    GetSelectionSize();
                    return TRUE;
                }
                case IDC_FILTERTYPE:
                {
                    int nofilter = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                    switch (nofilter)
                    {
                    case 0:
                        ImgResizeFilter = cv::INTER_NEAREST;
                        break;
                    case 1:
                        ImgResizeFilter = cv::INTER_LINEAR;
                        break;
                    case 2:
                        ImgResizeFilter = cv::INTER_CUBIC;
                        break;
                    case 3:
                        ImgResizeFilter = cv::INTER_LANCZOS4;
                        break;
                    }
                    return TRUE;
                }
            }
            return (INT_PTR)FALSE;
        }
    }
    return (INT_PTR)FALSE;
}

#pragma region Backgrounds

void Calc_Resize_BG(void)
{
    if (!glfwBG) return;
    glfwMakeContextCurrent(glfwBG);
    RECT winrect;
    GetClientRect(hBG, &winrect);
    ScrW4 = winrect.right;
    ScrH4 = winrect.bottom;
    NBGToDraw = (int)((float)(ScrW4 - FRAME_STRIP_W_MARGIN) / (float)(256 + FRAME_STRIP_W_MARGIN));
    BS_LMargin = (ScrW4 - (NBGToDraw * (FRAME_STRIP_W_MARGIN + 256) + FRAME_STRIP_W_MARGIN)) / 2;
    if (MycRom.name[0])
    {
        if (MycRom.fWidth == 192)
        {
            NBGToDraw = (int)((float)(ScrW4 - FRAME_STRIP_W_MARGIN) / (float)(192 + FRAME_STRIP_W_MARGIN));
            BS_LMargin = (ScrW4 - (NBGToDraw * (FRAME_STRIP_W_MARGIN + 192) + FRAME_STRIP_W_MARGIN)) / 2; // calculate the left and right margin in the strip
        }
    }
    int thei = winrect.bottom - (TOOLBAR_HEIGHT + 20) - FRAME_STRIP_HEIGHT - statusBarHeight - 20;
    int twid = winrect.right;
    int mul = 4;
    if (MycRom.name[0])
    {
        if (MycRom.fWidth == 192) mul = 3;
    }
    if (((float)twid / (float)thei) > mul) twid = thei * mul; else
    {
        thei = twid / mul;
        twid = thei * mul;
    }
    if (MycRom.name[0])
    {
        UINT fw, fh;
        if (nEditExtraResolutionB)
        {
            fh = MycRom.fHeightX;
            fw = MycRom.fWidthX;
        }
        else
        {
            fh = MycRom.fHeight;
            fw = MycRom.fWidth;
        }
        BG_zoom = ((float)thei / (float)fh);
        twid = (int)(BG_zoom * fw);
        thei = (int)(BG_zoom * fh);
    }
    else
    {
        BG_zoom = ((float)thei / (float)32);
        twid = (int)(BG_zoom * 128);
        thei = (int)(BG_zoom * 32);
    }
    glfwSetWindowSize(glfwBG, twid, thei);
    ScrWBG = twid;
    ScrHBG = thei;
    offset_BG_x = (winrect.right - twid) / 2;
    offset_BG_y = TOOLBAR_HEIGHT + 10 + (winrect.bottom - (TOOLBAR_HEIGHT + 20) - thei - FRAME_STRIP_HEIGHT) / 2;
    glfwSetWindowPos(glfwBG, offset_BG_x, offset_BG_y);
    SetViewport(glfwBG);
    glfwMakeContextCurrent(glfwBGstrip);
    glfwSetWindowSize(glfwBGstrip, ScrW4, FRAME_STRIP_HEIGHT);
    glfwSetWindowPos(glfwBGstrip, 0, ScrH4 - FRAME_STRIP_HEIGHT - statusBarHeight);
    SetViewport(glfwBGstrip);
}

LRESULT CALLBACK BGProc(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    /*case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Analyse les sélections de menu :
        switch (wmId)
        {
        default:
            return DefWindowProc(hWin, message, wParam, lParam);
        }
        break;
    }*/
    case WM_MOUSEWHEEL:
    {
        if (MycRom.name[0] == 0) return TRUE;
        short step = (short)HIWORD(wParam) / WHEEL_DELTA;
        if (!(LOWORD(wParam) & MK_SHIFT)) PreBGInStrip -= step;
        else PreBGInStrip -= step * (int)NBGToDraw;
        if (PreBGInStrip < 0) PreBGInStrip = 0;
        if (PreBGInStrip >= (int)MycRom.nFrames) PreBGInStrip = (int)MycRom.nFrames - 1;
        UpdateBSneeded = true;
        return TRUE;
    }
    case WM_GETMINMAXINFO:
    {
        int valx = GetSystemMetrics(SM_CXFULLSCREEN) + 15;
        int valy = GetSystemMetrics(SM_CYFULLSCREEN) + 25;
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 1152 + 16;
        mmi->ptMinTrackSize.y = 546 + 59;
        mmi->ptMaxTrackSize.x = valx;
        mmi->ptMaxTrackSize.y = valy;
        return 0;
        break;
    }
    case WM_SIZE:
    {
        if (!IsIconic(hWin))
        {
            Calc_Resize_BG();
            UpdateBSneeded = true;
            int cxClient = LOWORD(lParam);
            int cyClient = HIWORD(lParam);
            SendMessage(hBStatus, WM_SIZE, 0, MAKELPARAM(cxClient, cyClient));
            RECT rcStatusBar;
            GetWindowRect(hBStatus, &rcStatusBar);
            int statusBarHeight = rcStatusBar.bottom - rcStatusBar.top;
            MoveWindow(hBStatus, 0, cyClient - statusBarHeight, cxClient, statusBarHeight, TRUE);
        }
        break;
    }
    case WM_CLOSE:
    {
        if (MessageBoxA(hWnd, "Confirm you want to exit?", "Confirm", MB_YESNO) == IDYES)
        {
            MycRom.name[0] = 0;
            SaveWindowPosition();
            DestroyWindow(hWnd);
        }
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        fDone = true;
        break;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}

const char* ButtonDescription4(HWND hOver)
{
    if (hOver == GetDlgItem(hwTB4, IDC_IMPORT)) return (const char*)"Create a new background importing frame, palette and color rotation from the current displayed frame";
    if (hOver == GetDlgItem(hwTB4, IDC_REIMPORT)) return (const char*)"Import frame, palette and color rotation from the current displayed frame to the current displayed background";
    if (hOver == GetDlgItem(hwTB4, IDC_EXPORT)) return (const char*)"Export frame, palette and color rotation from the current displayed background to the current displayed frame";
    if (hOver == GetDlgItem(hwTB4, IDC_DELBG)) return (const char*)"Delete this background";
    if (hOver == GetDlgItem(hwTB4, IDC_SETBG)) return (const char*)"Set this background as the selected frame background";
    if (hOver == GetDlgItem(hwTB4, IDC_UNDO)) return (const char*)"Undo last action";
    if (hOver == GetDlgItem(hwTB4, IDC_REDO)) return (const char*)"Redo last undone action";
    if (hOver == GetDlgItem(hwTB4, IDC_BGLIST)) return (const char*)"List of frames using this background";
    return "";
}

LRESULT CALLBACK ButtonSubclassProc4(HWND hBut, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (message) {
        case WM_KEYDOWN:
        {
            return TRUE;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hBut;
            tme.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&tme);
            SetWindowTextA(hBStatus, ButtonDescription4(hBut));
            break;
        }
        case WM_MOUSELEAVE:
        {
            SetWindowTextA(hBStatus, "");
            break;
        }
    }

    return DefSubclassProc(hBut, message, wParam, lParam);
}

void Predraw_BG_For_Rotations(unsigned int noBG)
{
    if (MycRom.nBackgrounds == 0) return;
    UINT fw, fh;
    UINT16* pBG;
    DWORD lt = timeGetTime();
    for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
    {
        lastrotTime[ti] = lt;
        acrotShift[ti] = 0;
    }
    bool isextra;
    if (ExtraResBClicked && MycRom.isExtraBackground[noBG] > 0)
    {
        isextra = true;
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pBG = &MycRom.BackgroundFramesX[noBG * fw * fh];
    }
    else
    {
        isextra = false;
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pBG = &MycRom.BackgroundFrames[noBG * fw * fh];
    }
    for (UINT tj = 0; tj < fh; tj++)
    {
        for (UINT ti = 0; ti < fw; ti++)
        {
            UINT8 norot;
            UINT16 norotcol;
            if (isColorInRotation(pBG[tj * fw + ti], acFrame, isextra, &norot, &norotcol))
            {
                RotationsInBG[tj * fw + ti][0] = (UINT16)norot;
                RotationsInBG[tj * fw + ti][1] = norotcol;
            }
            else RotationsInBG[tj * fw + ti][0] = 0xffff;
        }
    }
}
void CheckAcBGChanged(void)
{
    if (acBG == prevAcBG && prevEditExtraResolutionB == nEditExtraResolutionB) return;
    prevAcBG = acBG;
    prevEditExtraResolutionB = nEditExtraResolutionB;
    Predraw_BG_For_Rotations(acBG);
}
void Draw_Background(GLFWwindow* glfwin, float zoom, UINT16 noBG, unsigned int x, unsigned int y, unsigned int clipw, unsigned int cliph)
{
    if (MycRom.nBackgrounds == 0) return;
    if (noBG >= MycRom.nBackgrounds)
    {
        cprintf(true, "Unknown background requested in Draw_Background");
        return;
    }
    CheckAcBGChanged();
    glfwMakeContextCurrent(glfwin);
    glEnable(GL_BLEND);
    if (ExtraResBClicked && MycRom.isExtraBackground[acBG] == 0)
    {
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLES);
        glColor4f(1, 0, 0, 1);
        glVertex2i(10, 0);
        glVertex2i(ScrWBG, ScrHBG - 10);
        glVertex2i(ScrWBG - 10, ScrHBG);
        glVertex2i(10, 0);
        glVertex2i(ScrWBG - 10, ScrHBG);
        glVertex2i(0, 10);
        glVertex2i(ScrWBG - 10, 0);
        glVertex2i(ScrWBG, 10);
        glVertex2i(10, ScrHBG);
        glVertex2i(ScrWBG - 10, 0);
        glVertex2i(10, ScrHBG);
        glVertex2i(0, ScrHBG - 10);
        glEnd();
        return;
    }
    glBindTexture(GL_TEXTURE_2D, TxCircleBG);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    UINT16* pBG,* pcolrot;
    UINT fw, fh;
    if (ExtraResBClicked && MycRom.isExtraBackground[noBG] > 0)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pBG = &MycRom.BackgroundFramesX[noBG * fw * fh];
        pcolrot = &MycRom.ColorRotationsX[acFrame * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pBG = &MycRom.BackgroundFrames[noBG * fw * fh];
        pcolrot = &MycRom.ColorRotations[acFrame * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
    }
    DWORD actime = timeGetTime();
    for (UINT ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
    {
        if (pcolrot[ti * MAX_LENGTH_COLOR_ROTATION] > 0)
        {
            if (actime >= lastrotTime[ti] + (DWORD)pcolrot[ti * MAX_LENGTH_COLOR_ROTATION + 1])
            {
                lastrotTime[ti] = actime;
                acrotShift[ti]++;
                if (acrotShift[ti] >= (UINT8)pcolrot[ti * MAX_LENGTH_COLOR_ROTATION]) acrotShift[ti] = 0;
            }
        }
    }
    for (unsigned int tj = 0; tj < fh; tj++)
    {
        for (unsigned int ti = 0; ti < fw; ti++)
        {
            UINT16 norot = RotationsInBG[tj * fw + ti][0];
            if (norot < 0xffff)
            {
                UINT16 nocolinrot = RotationsInBG[tj * fw + ti][1];
                UINT16 collength = pcolrot[norot * MAX_LENGTH_COLOR_ROTATION];
                SetRenderDrawColor565(pcolrot[norot * MAX_LENGTH_COLOR_ROTATION +
                    (nocolinrot + acrotShift[norot]) % collength + 2], 255);
            }
            else SetRenderDrawColor565(pBG[tj * fw + ti], 255);
            if (clipw != 0) RenderDrawPointClip(glfwin, x + ti * zoom, y + tj * zoom, x + clipw - 1, y + cliph - 1, zoom);
            else RenderDrawPoint(glfwin, x + ti * zoom, y + tj * zoom, zoom);
        }
    }
    glEnd();
}

void Draw_BG_Strip(void)
{
    float RTexCoord = (float)ScrW4 / (float)MonWidth;
    glfwMakeContextCurrent(glfwBGstrip);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxBGStrip[acBSText]);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    glColor4f(1, 1, 1, 1);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 0);
    glVertex2i(ScrW4, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW4, FRAME_STRIP_HEIGHT);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW4, FRAME_STRIP_HEIGHT);
    glTexCoord2f(0, 1);
    glVertex2i(0, FRAME_STRIP_HEIGHT);
    glEnd();
}

void CalcPosSlider4(void)
{
    float div = (float)MycRom.nBackgrounds / (float)SliderWidth4;
    PosSlider4 = (int)((float)PreBGInStrip / div);
}
void Get_BG_Strip_Line_Color(UINT pos)
{
    UINT corframe = (int)((float)(pos - FRAME_STRIP_SLIDER_MARGIN) * (float)MycRom.nBackgrounds / (float)SliderWidth4);
    UINT cornextframe = (int)((float)(pos - FRAME_STRIP_SLIDER_MARGIN + 1) * (float)MycRom.nBackgrounds / (float)SliderWidth4);
    if (cornextframe == corframe) cornextframe++;
    bool isac = false;
    for (UINT ti = corframe; ti < cornextframe; ti++)
    {
        if (ti == acBG) isac = true;
    }
    if (isac == true)
    {
        draw_color[0] = acColor[0];
        draw_color[1] = acColor[1];
        draw_color[2] = acColor[2];
    }
    else
    {
        draw_color[0] = UnselColor[0];
        draw_color[1] = UnselColor[1];
        draw_color[2] = UnselColor[2];
    }
}
void Background_Strip_Update(void)
{
    if (pBGStrip) memset(pBGStrip, 0, MonWidth * FRAME_STRIP_HEIGHT * 4);
    if (MycRom.name[0] == 0 || MycRom.nBackgrounds == 0) return;
    bool doublepixsize = true;
    UINT fw, fh;
    UINT16* pfr;
    UINT8* pstrip, * psmem, * psmem2;
    UINT addrow = ScrW4 * 4;
    pstrip = pBGStrip + (BS_LMargin + FRAME_STRIP_W_MARGIN) * 4 + FRAME_STRIP_H_MARGIN * addrow;
    UINT8 frFrameColor[3] = { 255,255,255 };
    int fwid = 256;
    if (MycRom.fWidth == 192) fwid = 192;
    for (int ti = 0; ti < (int)NBGToDraw; ti++)
    {
        if (ExtraResBClicked && MycRom.isExtraBackground[PreBGInStrip + ti] > 0)
        {
            fw = MycRom.fWidthX;
            fh = MycRom.fHeightX;
            pfr = &MycRom.BackgroundFramesX[(PreBGInStrip + ti) * fw * fh];
        }
        else
        {
            fw = MycRom.fWidth;
            fh = MycRom.fHeight;
            pfr = &MycRom.BackgroundFrames[(PreBGInStrip + ti) * fw * fh];
        }
        if (fh == 64) doublepixsize = false; else doublepixsize = true;
        if ((PreBGInStrip + ti < 0) || (PreBGInStrip + ti >= (int)MycRom.nBackgrounds))
        {
            pstrip += (fwid + FRAME_STRIP_W_MARGIN) * 4;
            continue;
        }
        if (PreBGInStrip + ti == acBG)
        {
            frFrameColor[0] = acColor[0];
            frFrameColor[1] = acColor[1];
            frFrameColor[2] = acColor[2];
        }
        else
        {
            frFrameColor[0] = UnselColor[0];
            frFrameColor[1] = UnselColor[1];
            frFrameColor[2] = UnselColor[2];
        }
        for (int tj = -1; tj < fwid + 1; tj++)
        {
            *(pstrip + tj * 4 + 64 * addrow) = *(pstrip - addrow + tj * 4) = frFrameColor[0];
            *(pstrip + tj * 4 + 64 * addrow + 1) = *(pstrip - addrow + tj * 4 + 1) = frFrameColor[1];
            *(pstrip + tj * 4 + 64 * addrow + 2) = *(pstrip - addrow + tj * 4 + 2) = frFrameColor[2];
            *(pstrip + tj * 4 + 64 * addrow + 3) = *(pstrip - addrow + tj * 4 + 3) = 255;
        }
        for (int tj = 0; tj < 64; tj++)
        {
            *(pstrip + tj * addrow + fwid * 4) = *(pstrip - 4 + tj * addrow) = frFrameColor[0];
            *(pstrip + tj * addrow + fwid * 4 + 1) = *(pstrip - 4 + tj * addrow + 1) = frFrameColor[1];
            *(pstrip + tj * addrow + fwid * 4 + 2) = *(pstrip - 4 + tj * addrow + 2) = frFrameColor[2];
            *(pstrip + tj * addrow + fwid * 4 + 3) = *(pstrip - 4 + tj * addrow + 3) = 255;
        }
        psmem = pstrip;
        for (UINT tj = 0; tj < fh; tj++)
        {
            psmem2 = pstrip;
            for (UINT tk = 0; tk < fw; tk++)
            {
                rgb565_to_rgb888(*pfr, &pstrip[0], &pstrip[1], &pstrip[2]);
                pstrip[3] = 255;
                if (doublepixsize)
                {
                    pstrip[addrow + 4] = pstrip[addrow] = pstrip[4] = pstrip[0];
                    pstrip[addrow + 5] = pstrip[addrow + 1] = pstrip[5] = pstrip[1];
                    pstrip[addrow + 6] = pstrip[addrow + 2] = pstrip[6] = pstrip[2];
                    pstrip[addrow + 7] = pstrip[addrow + 3] = pstrip[7] = 255;
                    pstrip += 4;
                }
                pstrip += 4;
                pfr++;
            }
            pstrip = psmem2 + addrow;
            if (doublepixsize) pstrip += addrow;
        }
        pstrip = psmem + (fwid + FRAME_STRIP_W_MARGIN) * 4;
    }
    for (UINT ti = 0; ti < NBGToDraw; ti++)
    {
        SetRenderDrawColor(128, 128, 128, 255);
        if (PreBGInStrip + ti < 0) continue;
        if (PreBGInStrip + ti >= MycRom.nBackgrounds) continue;
        Draw_Raw_Number(PreBGInStrip + ti, BS_LMargin + FRAME_STRIP_W_MARGIN + ti * (fwid + FRAME_STRIP_W_MARGIN) + 15, FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pBGStrip, ScrW4, FRAME_STRIP_H_MARGIN);
        if (MycRom.isExtraBackground[PreBGInStrip + ti] > 0)
        {
            SetRenderDrawColor(80, 80, 255, 255);
            Draw_Raw_Digit(10, BS_LMargin + FRAME_STRIP_W_MARGIN + ti * (fwid + FRAME_STRIP_W_MARGIN), FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pBGStrip, ScrW4, FRAME_STRIP_H_MARGIN);
        }
    }
    for (UINT ti = addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 5); ti < addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 7); ti++)
    {
        pBGStrip[ti] = 50;
    }
    for (UINT ti = FRAME_STRIP_SLIDER_MARGIN; ti < ScrW4 - FRAME_STRIP_SLIDER_MARGIN; ti++)
    {
        Get_BG_Strip_Line_Color(ti);
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti] = draw_color[0];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 1] = draw_color[1];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 2] = draw_color[2];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 3] = 255;
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti] = draw_color[0];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 1] = draw_color[1];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 2] = draw_color[2];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 3] = 255;
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti] = draw_color[0];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 1] = draw_color[1];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 2] = draw_color[2];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 3] = 255;
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti] = draw_color[0];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 1] = draw_color[1];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 2] = draw_color[2];
        pBGStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 3] = 255;
    }
    SliderWidth4 = ScrW4 - 2 * FRAME_STRIP_SLIDER_MARGIN;
    CalcPosSlider4();
    for (int ti = -5; ti <= 6; ti++)
    {
        for (int tj = 0; tj <= 2; tj++)
        {
            int offset = (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + ti) * addrow + (FRAME_STRIP_SLIDER_MARGIN + tj + PosSlider4) * 4;
            pBGStrip[offset] = 255;
            pBGStrip[offset + 1] = 255;
            pBGStrip[offset + 2] = 255;
            pBGStrip[offset + 3] = 255;
        }
    }
    SetRenderDrawColor(255, 255, 255, 255);
    glfwMakeContextCurrent(glfwBGstrip);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, TxBGStrip[!(acBSText)]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ScrW4, FRAME_STRIP_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pBGStrip); //RGBA with 4 bytes alignment for efficiency
    acBSText = !(acBSText);
}
void CheckPosBG(void)
{
    if (acBG < PreBGInStrip) PreBGInStrip = acBG;
    else if (acBG >= PreBGInStrip + NBGToDraw) PreBGInStrip = acBG - NBGToDraw + 1;
}
void AddBackground(void)
{
    SaveAction(true, SA_ADDBACKGROUND);
    MycRom.isExtraBackground = (UINT8*)realloc(MycRom.isExtraBackground, MycRom.nBackgrounds + 1);
    MycRom.BackgroundFrames = (UINT16*)realloc(MycRom.BackgroundFrames, (MycRom.nBackgrounds + 1) * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    MycRom.BackgroundFramesX = (UINT16*)realloc(MycRom.BackgroundFramesX, (MycRom.nBackgrounds + 1) * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    memcpy(&MycRom.BackgroundFrames[MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight], &MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    memset(&MycRom.BackgroundFramesX[MycRom.nBackgrounds * MycRom.fWidthX * MycRom.fHeightX], 0, MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    MycRom.isExtraBackground[MycRom.nBackgrounds] = 0;
    SendMessage(GetDlgItem(hwTB4, IDC_EXTRARES), BM_SETCHECK, BST_UNCHECKED, 0);
    acBG = MycRom.nBackgrounds;
    CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
    UpdateBackgroundList();
    MycRom.nBackgrounds++;
    InitColorRotation2();
    UpdateBSneeded = true;
}
void ReimportBackground(void)
{
    if (ExtraResBClicked && MycRom.isExtraFrame[acFrame] == 0)
    {
        MessageBoxA(hwTB4, "There is no extra resolution for the current displayed frame, action canceled.", "Failed", MB_OK);
        return;
    }
    if (ExtraResBClicked && MycRom.isExtraBackground[acBG] == 0)
    {
        if (MessageBoxA(hwTB4, "This background has no extra resolution version, do you want to activate it?", "Confirmation", MB_YESNO) == IDNO) return;
        SaveAction(true, SA_REIMPORTBACKGROUND);
        MycRom.isExtraBackground[acBG] = 1;
        SendMessage(GetDlgItem(hwTB4, IDC_EXTRARES), BM_SETCHECK, BST_CHECKED, 0);
    }
    else SaveAction(true, SA_REIMPORTBACKGROUND);
    if (nEditExtraResolutionB)
        memcpy(&MycRom.BackgroundFramesX[acBG * MycRom.fWidthX * MycRom.fHeightX], &MycRom.cFramesX[acFrame * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    else
        memcpy(&MycRom.BackgroundFrames[acBG * MycRom.fWidth * MycRom.fHeight], &MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    InitColorRotation2();
    UpdateBSneeded = true;
}
void CopyBackground(void)
{
    if (ExtraResBClicked && MycRom.isExtraBackground[acBG] == 0)
    {
        MessageBoxA(hwTB4, "There is no extra resolution for the current displayed background, action canceled.", "Failed", MB_OK);
        return;
    }
    if (ExtraResBClicked && MycRom.isExtraFrame[acFrame] == 0)
    {
        if (MessageBoxA(hwTB4, "The displayed frame has no extra resolution version, do you want to activate it?", "Confirmation", MB_YESNO) == IDNO) return;
        SaveAction(true, SA_COPYBACKGROUND);
        MycRom.isExtraFrame[acFrame] = 1;
        if (Edit_Mode == 1) SendMessage(GetDlgItem(hwTB, IDC_EXTRARES), BM_SETCHECK, BST_CHECKED, 0);
    }
    else SaveAction(true, SA_COPYBACKGROUND);
    if (nEditExtraResolutionB)
    {
        memset(&MycRom.DynaMasksX[acFrame * MycRom.fWidthX * MycRom.fHeightX], 255, MycRom.fWidthX * MycRom.fHeightX);
        memcpy(&MycRom.cFramesX[acFrame * MycRom.fWidthX * MycRom.fHeightX], &MycRom.BackgroundFramesX[acBG * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    }
    else
    {
        memset(&MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight], 255, MycRom.fWidth * MycRom.fHeight);
        memcpy(&MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight], &MycRom.BackgroundFrames[acBG * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    }
    InitColorRotation();
    UpdateFSneeded = true;
}
void DeleteBackground(void)
{
    if (ExtraResBClicked && MycRom.isExtraBackground[acBG] == 0)
    {
        MessageBoxA(hwTB4, "The current background has no extra resolution version, the action is ignored", "Ignored", MB_OK);
        return;
    }
    SaveAction(true, SA_DELBACKGROUND);
    memmove(&MycRom.isExtraBackground[acBG], &MycRom.isExtraBackground[acBG + 1], MycRom.nBackgrounds - 1 - acBG);
    memmove(&MycRom.BackgroundFrames[acBG * MycRom.fWidth * MycRom.fHeight], &MycRom.BackgroundFrames[(acBG + 1) * MycRom.fWidth * MycRom.fHeight], (MycRom.nBackgrounds - 1 - acBG) * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    memmove(&MycRom.BackgroundFramesX[acBG * MycRom.fWidthX * MycRom.fHeightX], &MycRom.BackgroundFramesX[(acBG + 1) * MycRom.fWidthX * MycRom.fHeightX], (MycRom.nBackgrounds - 1 - acBG) * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
    {
        if (MycRom.BackgroundID[ti] == 0xffff) continue;
        if (MycRom.BackgroundID[ti] == acBG) MycRom.BackgroundID[ti] = 0xffff;
        else if (MycRom.BackgroundID[ti] > acBG) MycRom.BackgroundID[ti]--;
    }
    MycRom.nBackgrounds--;
    MycRom.isExtraBackground = (UINT8*)realloc(MycRom.isExtraBackground, MycRom.nBackgrounds);
    MycRom.BackgroundFrames = (UINT16*)realloc(MycRom.BackgroundFrames, MycRom.nBackgrounds * MycRom.fWidth * MycRom.fHeight * sizeof(UINT16));
    MycRom.BackgroundFramesX = (UINT16*)realloc(MycRom.BackgroundFramesX, MycRom.nBackgrounds * MycRom.fWidthX * MycRom.fHeightX * sizeof(UINT16));
    if (acBG >= MycRom.nBackgrounds)
    {
        acBG = MycRom.nBackgrounds - 1;
        if (MycRom.isExtraBackground && MycRom.isExtraBackground[acBG] > 0) CheckDlgButton(hwTB4, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
        UpdateBackgroundList();
    }
    InitColorRotation2();
    UpdateBSneeded = true;
    UpdateFSneeded = true;
}
void SetBackground(void)
{
    if (MycRom.nBackgrounds == 0) return;
    if (NSelection == 0)
    {
        MessageBoxA(hBG,"No selection made in the frame for the background, action ignored", "Failed", MB_OK);
        return;
    }
    SaveAction(true,SA_SETBACKGROUND);
    UINT fw, fh;
    UINT8* pbgm;
    if (nEditExtraResolutionB)
    {
        if (Copy_Content != 2)
        {
            MessageBoxA(hwTB, "There is no extra resolution content in the selection, you can't import in extra resolution mode.", "Failed", MB_OK);
            return;
        }
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pbgm = &MycRom.BackgroundMaskX[acFrame * fw * fh];
    }
    else
    {
        if (Copy_Content != 1)
        {
            MessageBoxA(hwTB, "There is no original resolution content in the selection, you can't import in original resolution mode.", "Failed", MB_OK);
            return;
        }
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pbgm = &MycRom.BackgroundMask[acFrame * fw * fh];
    }

    for (UINT tk = 0; tk < nSelFrames; tk++)
    {
        MycRom.BackgroundID[SelFrames[tk]] = acBG;
        for (UINT tj = 0; tj < fh * fw; tj++)
        {
            pbgm[tj] = Copy_Mask[tj];
        }
    }
    InitColorRotation();
}
void UpdateFrameBG(void)
{
    if (MycRom.name[0] == 0) return;
    if (MycRom.BackgroundID[acFrame] >= MycRom.nBackgrounds) SetDlgItemTextA(hwTB, IDC_BGNB, "No BG");
    else
    {
        char tbuf[64];
        sprintf_s(tbuf, 64, "BG #%i", MycRom.BackgroundID[acFrame]);
        SetDlgItemTextA(hwTB, IDC_BGNB, tbuf);
    }
}
INT_PTR CALLBACK Toolbar_Proc4(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetWindowSubclass(GetDlgItem(hDlg, IDC_IMPORT), ButtonSubclassProc4, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_EXPORT), ButtonSubclassProc4, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_REIMPORT), ButtonSubclassProc4, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_DELBG), ButtonSubclassProc4, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_SETBG), ButtonSubclassProc4, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_UNDO), ButtonSubclassProc4, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_REDO), ButtonSubclassProc4, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_BGLIST), ButtonSubclassProc4, 0, 0);
        }
        case WM_CTLCOLORDLG:
        {
            if (Night_Mode) return (INT_PTR)GetStockObject(DKGRAY_BRUSH);
            return (INT_PTR)GetStockObject(GRAY_BRUSH);
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
            if (lpdis->CtlID == IDC_DISPEXTRA)
            {
                RECT tr;
                tr.left = lpdis->rcItem.left;
                tr.right = lpdis->rcItem.right;
                tr.top = lpdis->rcItem.top;
                tr.bottom = lpdis->rcItem.bottom;
                HBRUSH bg;
                if (ExtraResBClicked) bg = hActiveBrush; else bg = hInactiveBrush;
                FillRect(lpdis->hDC, &tr, bg);
                if (ExtraResBClicked) DrawEdge(lpdis->hDC, &tr, EDGE_SUNKEN, BF_RECT);
                else DrawEdge(lpdis->hDC, &tr, EDGE_RAISED, BF_RECT);
                tr.top += 5;
                SetTextColor(lpdis->hDC, GetSysColor(COLOR_BTNTEXT));
                SetBkMode(lpdis->hDC, TRANSPARENT);
                DrawTextA(lpdis->hDC, "Display\rExtra\rRes.", -1, &tr, DT_WORDBREAK | DT_CENTER);
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_IMPORT:
                {
                    if (MycRom.nBackgrounds == MAX_BACKGROUNDS)
                    {
                        cprintf(true, "Maximum number of backgrounds reached, action ignored!");
                        return FALSE;
                    }
                    AddBackground();
                    CheckPosBG();
                    return FALSE;
                }
                case IDC_EXPORT:
                {
                    if (MycRom.nBackgrounds == 0) return FALSE;
                    CopyBackground();
                    return FALSE;
                }
                case IDC_REIMPORT:
                {
                    ReimportBackground();
                    return FALSE;
                }
                case IDC_DELBG:
                {
                    if (MycRom.nBackgrounds == 0) return TRUE;
                    if (MessageBoxA(hBG, "Confirm that you want to delete this background?", "Sure?", MB_YESNO) == IDNO) return TRUE;
                    DeleteBackground();
                    CheckPosBG();
                    UpdateBackgroundList();
                    return FALSE;
                }
                case IDC_SETBG:
                {
                    SetBackground();
                    UpdateFrameBG();
                    UpdateFSneeded = true;
                    return FALSE;
                }
                case IDC_UNDO:
                {
                    RecoverAction(true);
                    return TRUE;
                }
                case IDC_REDO:
                {
                    RecoverAction(false);
                    return TRUE;
                }
                case IDC_BGLIST:
                {
                    int idx = (int)SendMessageA(GetDlgItem(hwTB4, IDC_BGLIST), CB_GETCURSEL, 0, 0);
                    if (idx == -1) return TRUE;
                    
                    char tbuf[10];
                    SendMessageA(GetDlgItem(hwTB4, IDC_BGLIST), CB_GETLBTEXT, idx, (LPARAM)tbuf);
                    int nofr = atoi(tbuf);
                    PreFrameInStrip = nofr;
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_EXTRARES:
                {
                    SaveAction(true, SA_ISBGX);
                    MycRom.isExtraBackground[acBG] = !MycRom.isExtraBackground[acBG];
                    if (MycRom.isExtraBackground[acBG] > 0)
                    {
                        ExtraResBClicked = true;
                        nEditExtraResolutionB = true;
                    }
                    else
                    {
                        ExtraResBClicked = false;
                        nEditExtraResolutionB = false;
                    }
                    UpdateBSneeded = true;
                    Calc_Resize_BG();
                    InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                    return TRUE;
                }
                case IDC_DISPEXTRA:
                {
                    if (MycRom.name[0] == 0 || MycRom.nBackgrounds == 0) return TRUE;
                    ExtraResBClicked = !ExtraResBClicked;
                    if (MycRom.isExtraBackground[acBG] > 0 && ExtraResBClicked) nEditExtraResolutionB = true;
                    else nEditExtraResolutionB = false;
                    FreeCopyMasks();
                    UpdateBSneeded = true;
                    UpdateFSneeded = true;
                    Calc_Resize_Image();
                    Calc_Resize_BG();
                    InitColorRotation();
                    InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                    return TRUE;
                }
                case IDC_ORGTOXTRA2:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_REIMPORTBACKGROUND);
                    ResizeRGB565Image(&MycRom.BackgroundFramesX[acBG * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX, MycRom.fHeightX, &MycRom.BackgroundFrames[acBG * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth, MycRom.fHeight, BGResizeFilter);
                    MycRom.isExtraBackground[acBG] = 1;
                    ExtraResBClicked = true;
                    InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                    nEditExtraResolutionB = true;
                    CheckDlgButton(hwTB4, IDC_EXTRARES, BST_CHECKED);
                    UpdateBSneeded = true;
                    UpdateFSneeded = true;
                    Calc_Resize_BG();
                    InitColorRotation2();
                    return TRUE;
                }
                case IDC_XTRATOORG2:
                {
                    if (MycRom.name[0] == 0 || MycRom.isExtraBackground[acBG] == 0) return TRUE;
                    SaveAction(true, SA_REIMPORTBACKGROUND);
                    ResizeRGB565Image(&MycRom.BackgroundFrames[acBG * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth, MycRom.fHeight, &MycRom.BackgroundFramesX[acBG * MycRom.fWidthX * MycRom.fHeightX], MycRom.fWidthX, MycRom.fHeightX, BGResizeFilter);
                    ExtraResBClicked = false;
                    InvalidateRect(GetDlgItem(hDlg, IDC_DISPEXTRA), NULL, TRUE);
                    nEditExtraResolutionB = false;
                    UpdateBSneeded = true;
                    UpdateFSneeded = true;
                    Calc_Resize_BG();
                    InitColorRotation2();
                    return TRUE;
                }
                case IDC_FILTERTYPE:
                {
                    int nofilter = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                    switch (nofilter)
                    {
                    case 0:
                        BGResizeFilter = cv::INTER_NEAREST;
                        break;
                    case 1:
                        BGResizeFilter = cv::INTER_LINEAR;
                        break;
                    case 2:
                        BGResizeFilter = cv::INTER_CUBIC;
                        break;
                    case 3:
                        BGResizeFilter = cv::INTER_LANCZOS4;
                        break;
                    }
                    return TRUE;
                }
            }
            return (INT_PTR)FALSE;
        }
    }
    return (INT_PTR)FALSE;
}

bool CreateToolbar4(void)
{
    if (hwTB4)
    {
        DestroyWindow(hwTB4);
    }
    hwTB4 = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BGDLG), hBG, Toolbar_Proc4);
    if (!hwTB4)
    {
        cprintf(true, "Unable to create the BG toolbar");
        return false;
    }
    ShowWindow(hwTB4, TRUE);
    SetIcon(GetDlgItem(hwTB4, IDC_IMPORT), IDI_IMPORT);
    SetIcon(GetDlgItem(hwTB4, IDC_REIMPORT), IDI_REIMPORT);
    SetIcon(GetDlgItem(hwTB4, IDC_EXPORT), IDI_EXPORT);
    SetIcon(GetDlgItem(hwTB4, IDC_DELBG), IDI_DELBG);
    SetIcon(GetDlgItem(hwTB4, IDC_SETBG), IDI_SETBG);
    SetIcon(GetDlgItem(hwTB4, IDC_UNDO), IDI_UNDO);
    SetIcon(GetDlgItem(hwTB4, IDC_REDO), IDI_REDO);
//    SetIcon(GetDlgItem(hwTB4, IDC_ZOOM2X), IDI_ZOOM2X);
    SetWindowLong(GetDlgItem(hwTB4, IDC_STRY6), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB4, IDC_STRY6), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB4, IDC_STRY7), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB4, IDC_STRY7), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB4, IDC_STRY8), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB4, IDC_STRY8), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    if (UndoAvailableN > 0) EnableWindow(GetDlgItem(hwTB4, IDC_UNDO), TRUE); else EnableWindow(GetDlgItem(hwTB4, IDC_UNDO), FALSE);
    if (RedoAvailableN > 0) EnableWindow(GetDlgItem(hwTB4, IDC_REDO), TRUE); else EnableWindow(GetDlgItem(hwTB4, IDC_REDO), FALSE);
    UpdateBackgroundList();
    SetExtraResBBox();
    UpdateURCounts();
    SendMessageA(GetDlgItem(hwTB4, IDC_FILTERTYPE), CB_RESETCONTENT, 0, 0);
    SendMessageA(GetDlgItem(hwTB4, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Nearest");
    SendMessageA(GetDlgItem(hwTB4, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Bilinear");
    SendMessageA(GetDlgItem(hwTB4, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Bicubic");
    SendMessageA(GetDlgItem(hwTB4, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Lanczos4");
    FrameResizeFilter = 0;
    SendMessageA(GetDlgItem(hwTB4, IDC_FILTERTYPE), CB_SETCURSEL, 0, 0);
    return true;
}

void mouse_scroll_callback4(GLFWwindow* window, double xoffset, double yoffset)
{
    if (MycRom.name[0] == 0) return;
    short step = (short)yoffset;
    if (!(GetKeyState(VK_SHIFT) & 0x8000)) PreBGInStrip -= step;
    else PreBGInStrip -= step * (int)NBGToDraw;
    if (PreBGInStrip < 0) PreBGInStrip = 0;
    if (PreBGInStrip >= (int)MycRom.nBackgrounds) PreBGInStrip = (int)MycRom.nBackgrounds - 1;
    UpdateBSneeded = true;
}
void mouse_button_callback4(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int xipos = (int)xpos, yipos = (int)ypos;

    if (MycRom.name[0] != 0)
    {
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_RELEASE)) MouseBGSliderLPressed = false;
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS))
        {
            if ((yipos >= FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 5) && (yipos <= FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 6))
            {
                if ((xipos >= FRAME_STRIP_SLIDER_MARGIN) && (xipos <= (int)ScrW4 - FRAME_STRIP_SLIDER_MARGIN))
                {
                    MouseBGSliderLPressed = true;
                }
                return;
            }
            UINT wid = 256;
            if (MycRom.fWidth == 192) wid = 192;
            if ((yipos >= FRAME_STRIP_H_MARGIN) && (yipos < FRAME_STRIP_H_MARGIN + 128))
            {
                if ((xipos >= (int)BS_LMargin + FRAME_STRIP_W_MARGIN) && (xipos <= (int)ScrW4 - (int)BS_LMargin - FRAME_STRIP_W_MARGIN))
                {
                    if ((xipos - BS_LMargin - FRAME_STRIP_W_MARGIN) % (wid + FRAME_STRIP_W_MARGIN) < wid) // check that we are not between the frames
                    {
                        SaveAction(true, SA_SELBACKGROUND);
                        acBG = PreBGInStrip + (xipos - BS_LMargin - FRAME_STRIP_W_MARGIN) / (wid + FRAME_STRIP_W_MARGIN);
                        if (acBG >= MycRom.nBackgrounds) acBG = MycRom.nBackgrounds - 1;
                        if (MycRom.isExtraBackground[acBG] > 0) CheckDlgButton(hwTB4, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
                        UpdateBackgroundList();
                        InitColorRotation2();
                        UpdateBSneeded = true;
                        if (ExtraResBClicked && MycRom.isExtraBackground[acBG] > 0) nEditExtraResolutionB = true; else nEditExtraResolutionB = false;
                    }
                }
            }
        }
        return;
    }
}

bool bg_CreateWindow(void)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = BGProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CROM));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;// MAKEINTRESOURCEW(IDC_COLORIZINGDMD);
    wcex.lpszClassName = L"BGClass";
    wcex.hIconSm = NULL; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    if (!RegisterClassEx(&wcex))
    {
        return false;
    }
    hBG = CreateWindow(L"BGClass", L"Backgrounds", WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1480, 900, nullptr, nullptr, hInst, nullptr);
    if (!hBG)
    {
        AffLastError((char*)"CreateWindow");
        return false;
    }
    ShowWindow(hBG, SW_SHOW);
    UpdateWindow(hBG);
    hBStatus = DoCreateStatusBar(hBG, hInst);

    if (!gl33_InitWindow(&glfwBG, 10, 10, "BG", hBG)) return false;
    if (!gl33_InitWindow(&glfwBGstrip, 10, 10, "BG Strip", hBG)) return false;
    glfwSetMouseButtonCallback(glfwBGstrip, mouse_button_callback4);
    glfwSetScrollCallback(glfwBGstrip, mouse_scroll_callback4);

    return CreateToolbar4();
}


#pragma endregion


bool SCKeyState(char inKey)
{
    unsigned char scancode = 0;
    switch (inKey)
    {
    case '1':
        scancode = 2;
        break;
    case '2':
        scancode = 3;
        break;
    case '3':
        scancode = 4;
        break;
    case '4':
        scancode = 5;
        break;
    case 'A':
        scancode = 16;
        break;
    case 'Z':
        scancode = 17;
        break;
    case 'E':
        scancode = 18;
        break;
    case 'R':
        scancode = 19;
        break;
    case 'Q':
        scancode = 30;
        break;
    case 'S':
        scancode = 31;
        break;
    case 'D':
        scancode = 32;
        break;
    case 'F':
        scancode = 33;
        break;
    case 'W':
        scancode = 44;
        break;
    case 'X':
        scancode = 45;
        break;
    case 'C':
        scancode = 46;
        break;
    case 'V':
        scancode = 47;
        break;
    }
    UINT key = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
    if (GetKeyState(key) & 0x8000) return true;
    return false;
}

bool isSReleased = false, isEnterReleased = false, isZReleased = false, isYReleased = false, isMReleased = false, isAReleased = false,
isCReleased = false, isVReleased = false, isBReleased = false, isFReleased = false, isDReleased = false, isEReleased = false,
isTabReleased = false, isXReleased = false;
bool is4Released = false, is6Released = false, is2Released = false, is8Released = false;
bool is1Rel = false, is2Rel = false, is3Rel = false, is4Rel = false, isARel = false, isZRel = false, isERel = false, isRRel = false,
    isQRel = false, isSRel = false, isDRel = false, isFRel = false, isWRel = false, isXRel = false, isCRel = false, isVRel = false;
DWORD NumPadNextTime = 0;
void CheckAccelerators(void)
{
    if (!(GetKeyState('S') & 0x8000)) isSReleased = true;
    if (!(GetKeyState('Z') & 0x8000)) isZReleased = true;
    if (!(GetKeyState('Y') & 0x8000)) isYReleased = true;
    if (!(GetKeyState('M') & 0x8000)) isMReleased = true;
    if (!(GetKeyState('X') & 0x8000)) isXReleased = true;
    if (!(GetKeyState('F') & 0x8000)) isFReleased = true;
    if (!(GetKeyState('C') & 0x8000)) isCReleased = true;
    if (!(GetKeyState('A') & 0x8000)) isAReleased = true;
    if (!(GetKeyState('D') & 0x8000)) isDReleased = true;
    if (!(GetKeyState('E') & 0x8000)) isEReleased = true;
    if (!(GetKeyState('V') & 0x8000)) isVReleased = true;
    if (!(GetKeyState('B') & 0x8000)) isBReleased = true;
    if (!(GetKeyState(VK_TAB) & 0x8000)) isTabReleased = true;
    if (!(GetKeyState(VK_NUMPAD2) & 0x8000)) is2Released = true;
    if (!(GetKeyState(VK_NUMPAD4) & 0x8000)) is4Released = true;
    if (!(GetKeyState(VK_NUMPAD6) & 0x8000)) is6Released = true;
    if (!(GetKeyState(VK_NUMPAD8) & 0x8000)) is8Released = true;
    if (!(GetKeyState(VK_RETURN) & 0x8000)) isEnterReleased = true;
    if (GetForegroundWindow() == hWnd)
    {
        if (MycRom.name[0])
        {
            if (GetKeyState('Z') & 0x8000 && GetKeyState(VK_SHIFT) & 0x8000 && isZReleased)
            {
                SendMessage(hwTB, WM_COMMAND, MAKEWPARAM(IDC_ZOOM2X, 0), 0);
                isZReleased = false;
            }
            if (GetKeyState(VK_NUMPAD2) & 0x8000)
            {
                if (Paste_Mode)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is2Released)
                    {
                        if (is2Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is2Released = false;
                        paste_offsety += 1;
                    }
                }
                else if (Zoom_Pushed)
                {
                    bool ishft = (GetKeyState(VK_CONTROL) & 0x8000) > 0;
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is2Released)
                    {
                        if (is2Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is2Released = false;
                        int tmax;
                        if (nEditExtraResolutionF) tmax = (int)MycRom.fHeightX / 2; else tmax = (int)MycRom.fHeight / 2;
                        if (offset_frame_y < tmax)
                        {
                            if (ishft)
                            {
                                offset_frame_y += 10;
                                if (offset_frame_y > tmax) offset_frame_y = tmax;
                            }
                            else offset_frame_y++;
                        }
                    }
                }
            }
            if (GetKeyState(VK_NUMPAD4) & 0x8000)
            {
                if (Paste_Mode)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is4Released)
                    {
                        if (is4Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is4Released = false;
                        paste_offsetx -= 1;
                    }
                }
                else if (Zoom_Pushed)
                {
                    bool ishft = (GetKeyState(VK_CONTROL) & 0x8000) > 0;
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is4Released)
                    {
                        if (is4Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is4Released = false;
                        if (offset_frame_x > 0)
                        {
                            if (ishft)
                            {
                                offset_frame_x -= 10;
                                if (offset_frame_x < 0) offset_frame_x = 0;
                            }
                            else offset_frame_x--;
                        }
                    }
                }
            }
            if (GetKeyState(VK_NUMPAD6) & 0x8000)
            {
                if (Paste_Mode)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is6Released)
                    {
                        if (is6Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is6Released = false;
                        paste_offsetx += 1;
                    }
                }
                else if (Zoom_Pushed)
                {
                    bool ishft = (GetKeyState(VK_CONTROL) & 0x8000) > 0;
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is6Released)
                    {
                        if (is6Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is6Released = false;
                        int tmax;
                        if (nEditExtraResolutionF) tmax = (int)MycRom.fWidthX / 2; else tmax = (int)MycRom.fWidth / 2;
                        if (offset_frame_x < tmax)
                        {
                            if (ishft)
                            {
                                offset_frame_x += 10;
                                if (offset_frame_x > tmax) offset_frame_x = tmax;
                            }
                            else offset_frame_x++;
                        }
                    }
                }
            }
            if (GetKeyState(VK_NUMPAD8) & 0x8000)
            {
                if (Paste_Mode)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is8Released)
                    {
                        if (is8Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is8Released = false;
                        paste_offsety -= 1;
                    }
                }
                else if (Zoom_Pushed)
                {
                    bool ishft = (GetKeyState(VK_CONTROL) & 0x8000) > 0;
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is8Released)
                    {
                        if (is8Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is8Released = false;
                        if (offset_frame_y > 0)
                        {
                            if (ishft)
                            {
                                offset_frame_y -= 10;
                                if (offset_frame_y < 0) offset_frame_y = 0;
                            }
                            else offset_frame_y--;
                        }
                    }
                }
            }
            if ((!Paste_Mode) && (Color_Pipette == 0))
            {
                if ((isTabReleased) && (GetKeyState(VK_TAB) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000) && !(GetKeyState(VK_CONTROL) & 0x8000) &&
                    !(GetKeyState(VK_LWIN) & 0x8000) && !(GetKeyState(VK_RWIN) & 0x8000))
                {
                    Edit_Mode = !Edit_Mode;
                    Update_Toolbar = true;
                    UpdateFSneeded = true;
                    isTabReleased = false;
                    Calc_Resize_Frame();
                }
                else if (GetKeyState(VK_CONTROL) & 0x8000)
                {
                    if ((isSReleased) && (GetKeyState('S') & 0x8000))
                    {
                        isSReleased = false;
                        if (Save_cRom(false, true, (char*)""))
                        {
                            cprintf(false, "%s.cROM saved in %s", MycRom.name, Dir_Serum);
                            if (Save_cRP(false)) cprintf(false, "%s.cRP saved in %s", MycRom.name, Dir_Serum);
                        }
                    }
                    if ((isXReleased) && (GetKeyState('X') & 0x8000))
                    {
                        isXReleased = false;
                        SendMessage(hwTB, WM_COMMAND, MAKEWPARAM(IDC_DISPEXTRA, 0), 0);
                    }
                    if ((isZReleased) && (GetKeyState('Z') & 0x8000))
                    {
                        isZReleased = false;
                        RecoverAction(true);
                    }
                    if ((isYReleased) && (GetKeyState('Y') & 0x8000))
                    {
                        isYReleased = false;
                        RecoverAction(false);
                    }
                    if ((isMReleased) && (GetKeyState('M') & 0x8000))
                    {
                        isMReleased = false;
                        Edit_Mode = !Edit_Mode;
                        Update_Toolbar = true;
                        UpdateFSneeded = true;
                        Calc_Resize_Frame();
                    }
                    if ((isAReleased) && (GetKeyState('A') & 0x8000))
                    {
                        if (Edit_Mode == 0)
                        {
                            if (MycRom.CompMaskID[acFrame] != 255)
                            {
                                SaveAction(true, SA_COMPMASK);
                                memset(&MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight], 1, MycRom.fWidth * MycRom.fHeight);
                                CheckSameFrames();
                            }
                        }
                        else
                        {
                            SaveAction(true, SA_COPYMASK);
                            memset(Draw_Extra_Surface, 1, 256 * 64);
                            Add_Surface_To_Copy(Draw_Extra_Surface, false);
                        }
                        isAReleased = false;
                    }
                    if ((isDReleased) && (GetKeyState('D') & 0x8000))
                    {
                        if (Edit_Mode == 0)
                        {
                            if (MycRom.CompMaskID[acFrame] != 255)
                            {
                                SaveAction(true, SA_COMPMASK);
                                memset(&MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight], 0, MycRom.fWidth * MycRom.fHeight);
                                CheckSameFrames();
                            }
                        }
                        else
                        {
                            SaveAction(true, SA_COPYMASK);
                            memset(Draw_Extra_Surface, 1, 256 * 64);
                            Add_Surface_To_Copy(Draw_Extra_Surface, true);
                        }
                        isDReleased = false;
                    }
                    if ((isFReleased) && (GetKeyState('F') & 0x8000))
                    {
                        isFReleased = false;
                        PreFrameInStrip = acFrame;
                        UpdateFSneeded = true;
                    }
                    if (isBReleased && (GetKeyState('B') & 0x8000))
                    {
                        isBReleased = false;
                        SaveAction(true, SA_DRAW);
                        UpdateFSneeded = true;
                        UINT fw, fh;
                        UINT16* pfr, * pfra;
                        UINT8* pfro;
                        if (nEditExtraResolutionF)
                        {
                            fw = MycRom.fWidthX;
                            fh = MycRom.fHeightX;
                            pfr = MycRom.cFramesX;
                        }
                        else if (ExtraResFClicked) return;
                        else
                        {
                            fw = MycRom.fWidth;
                            fh = MycRom.fHeight;
                            pfr = MycRom.cFrames;
                        }
                        for (UINT tk = 0; tk < nSelFrames; tk++)
                        {
                            pfra = &pfr[SelFrames[tk] * fw * fh];
                            pfro = &MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight];
                            for (UINT tj = 0; tj < fh; tj++)
                            {
                                for (UINT ti = 0; ti < fw; ti++)
                                {
                                    UINT8 val;
                                    if (nEditExtraResolutionF && fh == 64)
                                        val = pfro[tj / 2 * MycRom.fWidth + ti / 2];
                                    else if (nEditExtraResolutionF)
                                        val = ValuePlus2x2(&pfro[tj * 2 * MycRom.fWidth + ti * 2], MycRom.fWidth);
                                    else val = pfro[tj * fw + ti];
                                    if (val == 0) pfra[tj * fw + ti] = 0;
                                }
                            }
                        }
                    }
                    if (Edit_Mode == 1)
                    {
                        if ((isCReleased) && (GetKeyState('C') & 0x8000))
                        {
                            isCReleased = false;
                            SendMessage(hwTB, WM_COMMAND, IDC_COPY, 0);
                        }
                        if ((isVReleased) && (GetKeyState('V') & 0x8000))
                        {
                            isVReleased = false;
                            if (IsWindowEnabled(GetDlgItem(hwTB, IDC_PASTE)) == TRUE) SendMessage(hwTB, WM_COMMAND, IDC_PASTE, 0);
                        }
                        if ((isEReleased) && (GetKeyState('E') & 0x8000))
                        {
                            isEReleased = false;
                            SaveAction(true, SA_DYNAMASK);
                            UINT fw, fh;
                            UINT8* pdynm;
                            UINT16* pfr;
                            for (UINT tk = 0; tk < nSelFrames; tk++)
                            {
                                if (ExtraResFClicked)
                                {
                                    if (MycRom.isExtraFrame[SelFrames[tk]] == 0) return;
                                    fw = MycRom.fWidthX;
                                    fh = MycRom.fHeightX;
                                    pdynm = &MycRom.DynaMasksX[SelFrames[tk] * fw * fh];
                                    pfr = &MycRom.cFramesX[SelFrames[tk] * fw * fh];
                                }
                                else
                                {
                                    fw = MycRom.fWidth;
                                    fh = MycRom.fHeight;
                                    pdynm = &MycRom.DynaMasks[SelFrames[tk] * fw * fh];
                                    pfr = &MycRom.cFrames[SelFrames[tk] * fw * fh];
                                }
                                for (UINT tj = 0; tj < fh; tj++)
                                {
                                    for (UINT ti = 0; ti < fw; ti++)
                                    {
                                        UINT i=ti, j=tj;
                                        if (ExtraResFClicked)
                                        {
                                            if (fh == 64)
                                            {
                                                i = ti / 2;
                                                j = tj / 2;
                                            }
                                            else
                                            {
                                                i = ti * 2;
                                                j = tj * 2;
                                            }
                                        }
                                        if (pdynm[tj * fw + ti] < 255)
                                            pfr[tj * fw + ti] = MycRP.Palette[MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + j * MycRom.fWidth + i]];
                                    }
                                }
                                memset(pdynm, 255, fw* fh);
                            }
                        }
                    }
                }
            }
            else if ((GetKeyState(VK_MENU) & 0x8000) && (!Paste_Mode) && (Color_Pipette == 0))
            {
                if ((isDReleased) && (GetKeyState('D') & 0x8000) && (Edit_Mode == 1))
                {
                    isDReleased = false;
                    SaveAction(true, SA_DYNAMASK);
                    UINT fw, fh;
                    UINT8* pdynm;
                    UINT16* pfr;
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        if (nEditExtraResolutionF)
                        {
                            if (MycRom.isExtraFrame[SelFrames[ti]] == 0) continue;
                            fw = MycRom.fWidthX;
                            fh = MycRom.fHeightX;
                            pdynm = &MycRom.DynaMasksX[SelFrames[ti] * fw * fh];
                            pfr = &MycRom.cFramesX[SelFrames[ti] * fw * fh];
                        }
                        else
                        {
                            fw = MycRom.fWidth;
                            fh = MycRom.fHeight;
                            pdynm = &MycRom.DynaMasks[SelFrames[ti] * fw * fh];
                            pfr = &MycRom.cFrames[SelFrames[ti] * fw * fh];
                        }
                        for (UINT tj = 0; tj < fw * fh; tj++)
                        {
                            if (pdynm[tj] < 255)
                                pfr[tj] = MycRP.Palette[MycRP.oFrames[SelFrames[ti] * fw * fh + tj]];
                        }
                        memset(pdynm, 255, fw * fh);
                    }
                }
                if ((isAReleased) && (GetKeyState('A') & 0x8000) && (Edit_Mode == 1))
                {
                    isAReleased = false;
                    SaveAction(true, SA_DYNAMASK);
                    UINT fw, fh;
                    UINT8* pdynm;
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        if (ExtraResFClicked)
                        {
                            if (MycRom.isExtraFrame[SelFrames[ti]] == 0) return;
                            fw = MycRom.fWidthX;
                            fh = MycRom.fHeightX;
                            pdynm = &MycRom.DynaMasksX[SelFrames[ti] * fw * fh];
                        }
                        else
                        {
                            fw = MycRom.fWidth;
                            fh = MycRom.fHeight;
                            pdynm = &MycRom.DynaMasks[SelFrames[ti] * fw * fh];
                        }
                        memset(pdynm, acDynaSet, fw* fh);
                    }
                }
            }
            else
            {
                if ((isEnterReleased) && (GetKeyState(VK_RETURN) & 0x8000) && (Paste_Mode == true))
                {
                    mouse_button_callback(glfwframe, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
                }
                if (Edit_Mode == 1)
                {
                    if (!SCKeyState('1')) is1Rel = true;
                    else if (is1Rel)
                    {
                        is1Rel = false;
                        noColMod = noColSel = 0;
                        InvalidateRect(hwTB, NULL, TRUE);
                    }
                    if (!SCKeyState('2')) is2Rel = true;
                    else if (is2Rel)
                    {
                        is2Rel = false;
                        noColMod = noColSel = 1;
                        InvalidateRect(hwTB, NULL, TRUE);
                    }
                    if (!SCKeyState('3')) is3Rel = true;
                    else if (is3Rel)
                    {
                        is3Rel = false;
                        noColMod = noColSel = 2;
                        InvalidateRect(hwTB, NULL, TRUE);
                    }
                    if (!SCKeyState('4')) is4Rel = true;
                    else if (is4Rel)
                    {
                        is4Rel = false;
                        noColMod = noColSel = 3;
                        InvalidateRect(hwTB, NULL, TRUE);
                    }
                    if (MycRom.noColors == 16)
                    {
                        if (!SCKeyState('A')) isARel = true;
                        else if (isARel)
                        {
                            isARel = false;
                            noColMod = noColSel = 4;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('Z')) isZRel = true;
                        else if (isZRel)
                        {
                            isZRel = false;
                            noColMod = noColSel = 5;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('E')) isERel = true;
                        else if (isERel)
                        {
                            isERel = false;
                            noColMod = noColSel = 6;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('R')) isRRel = true;
                        else if (isRRel)
                        {
                            isRRel = false;
                            noColMod = noColSel = 7;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('Q')) isQRel = true;
                        else if (isQRel)
                        {
                            isQRel = false;
                            noColMod = noColSel = 8;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('S')) isSRel = true;
                        else if (isSRel)
                        {
                            isSRel = false;
                            noColMod = noColSel = 9;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('D')) isDRel = true;
                        else if (isDRel)
                        {
                            isDRel = false;
                            noColMod = noColSel = 10;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('F')) isFRel = true;
                        else if (isFRel)
                        {
                            isFRel = false;
                            noColMod = noColSel = 11;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('W')) isWRel = true;
                        else if (isWRel)
                        {
                            isWRel = false;
                            noColMod = noColSel = 12;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('X')) isXRel = true;
                        else if (isXRel)
                        {
                            isXRel = false;
                            noColMod = noColSel = 13;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('C')) isCRel = true;
                        else if (isCRel)
                        {
                            isCRel = false;
                            noColMod = noColSel = 14;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                        if (!SCKeyState('V')) isVRel = true;
                        else if (isVRel)
                        {
                            isVRel = false;
                            noColMod = noColSel = 15;
                            InvalidateRect(hwTB, NULL, TRUE);
                        }
                    }
                }
            }
        }
    }
    else if (GetForegroundWindow() == hSprites)
    {
        if (MycRom.name[0])
        {
            if (Zoom_Pushed_Sprite)// && ((nEditExtraResolutionS && MycRom.fHeightX == 64) || (!nEditExtraResolutionS && MycRom.fHeight == 64)))
            {
                bool ishft = (GetKeyState(VK_CONTROL) & 0x8000) > 0;
                if (GetKeyState(VK_NUMPAD2) & 0x8000)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is2Released)
                    {
                        if (is2Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is2Released = false;
                        if (offset_sprite_y < MAX_SPRITE_HEIGHT / 2)
                        {
                            if (ishft)
                            {
                                offset_sprite_y += 10;
                                if (offset_sprite_y > MAX_SPRITE_HEIGHT / 2) offset_sprite_y = MAX_SPRITE_HEIGHT / 2;
                            }
                            else offset_sprite_y++;
                        }
                    }
                }
                if (GetKeyState(VK_NUMPAD4) & 0x8000)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is4Released)
                    {
                        if (is4Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is4Released = false;
                        if (offset_sprite_x > 0)
                        {
                            if (ishft)
                            {
                                offset_sprite_x -= 10;
                                if (offset_sprite_x < 0) offset_sprite_x = 0;
                            }
                            else offset_sprite_x--;
                        }
                    }
                }
                if (GetKeyState(VK_NUMPAD6) & 0x8000)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is6Released)
                    {
                        if (is6Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is6Released = false;
                        if (offset_sprite_x < 128)
                        {
                            if (ishft)
                            {
                                offset_sprite_x += 10;
                                if (offset_sprite_x > MAX_SPRITE_WIDTH / 2) offset_sprite_x = MAX_SPRITE_WIDTH / 2;
                            }
                            else offset_sprite_x++;
                        }
                    }
                }
                if (GetKeyState(VK_NUMPAD8) & 0x8000)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime || is8Released)
                    {
                        if (is8Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is8Released = false;
                        if (offset_sprite_y > 0)
                        {
                            if (ishft)
                            {
                                offset_sprite_y -= 10;
                                if (offset_sprite_y < 0) offset_sprite_y = 0;
                            }
                            else offset_sprite_y--;
                        }
                    }
                }
            }
            if (GetKeyState('Z') & 0x8000 && GetKeyState(VK_SHIFT) & 0x8000 && isZReleased)
            {
                SendMessage(hwTB2, WM_COMMAND, MAKEWPARAM(IDC_ZOOM2X, 0), 0);
                isZReleased = false;
            }
            if (GetKeyState(VK_CONTROL) & 0x8000)
            {
                if ((isZReleased) && (GetKeyState('Z') & 0x8000))
                {
                    isZReleased = false;
                    RecoverAction(true);
                }
                if ((isYReleased) && (GetKeyState('Y') & 0x8000))
                {
                    isYReleased = false;
                    RecoverAction(false);
                }
                if ((isSReleased) && (GetKeyState('S') & 0x8000))
                {
                    isSReleased = false;
                    if (Save_cRom(false, true, (char*)""))
                    {
                        cprintf(false, "%s.cROM saved in %s", MycRom.name, Dir_Serum);
                        if (Save_cRP(false)) cprintf(false, "%s.cRP saved in %s", MycRom.name, Dir_Serum);
                    }
                }
                if ((isVReleased) && (GetKeyState('V') & 0x8000))
                {
                    isVReleased = false;
                    if (IsWindowEnabled(GetDlgItem(hwTB2, IDC_PASTE)) == TRUE) SendMessage(hwTB2, WM_COMMAND, IDC_PASTE, 0);
                }
            }
            if (MouseSpSliderLPressed)
            {
                double x, y;
                glfwGetCursorPos(glfwspritestrip, &x, &y);
                int px = (int)x - FRAME_STRIP_SLIDER_MARGIN;
                if (px != MouseSpSliderlx)
                {
                    MouseSpSliderlx = px;
                    if (px < 0) px = 0;
                    if (px > (int)ScrW2 - 2 * FRAME_STRIP_SLIDER_MARGIN) px = (int)ScrW2 - 2 * FRAME_STRIP_SLIDER_MARGIN;
                    PreSpriteInStrip = (int)((float)px * (float)MycRom.nSprites / (float)SliderWidth2);
                    if (PreSpriteInStrip < 0) PreSpriteInStrip = 0;
                    if (PreSpriteInStrip >= (int)MycRom.nSprites) PreSpriteInStrip = (int)MycRom.nSprites - 1;
                    UpdateSSneeded = true;
                }
            }
        }
    }
    else if (GetForegroundWindow() == hImages)
    {
        if ((isVReleased) && (GetKeyState('V') & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000))
        {
            isVReleased = false;
            SendMessage(hwTB3, WM_COMMAND, IDC_CBPASTE, 0);
        }
        if ((isCReleased) && (GetKeyState('C') & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000))
        {
            isCReleased = false;
            SendMessage(hwTB3, WM_COMMAND, IDC_COPY, 0);
        }
   }
}
void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (MycRom.name[0] == 0) return;
    short step = (short)yoffset;
    if (!(GetKeyState(VK_SHIFT) & 0x8000)) PreFrameInStrip -= step;
    else PreFrameInStrip -= step * (int)NFrameToDraw;
    if (PreFrameInStrip < 0) PreFrameInStrip = 0;
    if (PreFrameInStrip >= (int)MycRom.nFrames) PreFrameInStrip = (int)MycRom.nFrames - 1;
    UpdateFSneeded = true;
}
void mouse_scroll_callback2(GLFWwindow* window, double xoffset, double yoffset)
{
    if (MycRom.name[0] == 0) return;
    short step = (short)yoffset;
    if (!(GetKeyState(VK_SHIFT) & 0x8000)) PreSpriteInStrip -= step;
    else PreSpriteInStrip -= step * (int)NSpriteToDraw;
    if (PreSpriteInStrip < 0) PreSpriteInStrip = 0;
    if (PreSpriteInStrip >= (int)MycRom.nSprites) PreSpriteInStrip = (int)MycRom.nSprites - 1;
    UpdateSSneeded = true;
}
void mouse_move_callback2(GLFWwindow* window, double xpos, double ypos)
{
    if (MycRom.name[0] == 0) return;
    if (ExtraResSClicked && MycRom.isExtraSprite[acSprite] == 0) return;
    if (window == glfwsprites)
    {
        if ((Mouse_Mode == 4) || (Mouse_Mode == 3) || (Mouse_Mode == 2))
        {
            if (Zoom_Pushed_Sprite)// && ((nEditExtraResolutionS && MycRom.fHeightX == 64) || (!nEditExtraResolutionS && MycRom.fHeight == 64)))
            {
                MouseFinPosx = (int)(xpos / (2 * sprite_zoom) + offset_sprite_x);
                MouseFinPosy = (int)(ypos / (2 * sprite_zoom) + offset_sprite_y);
            }
            else
            {
                MouseFinPosx = (int)(xpos / sprite_zoom);
                MouseFinPosy = (int)(ypos / sprite_zoom);
            }
            if (MouseFinPosx < 0) MouseFinPosx = 0;
            if (MouseFinPosx >= MAX_SPRITE_WIDTH) MouseFinPosx = MAX_SPRITE_WIDTH - 1;
            if (MouseFinPosy < 0) MouseFinPosy = 0;
            if (MouseFinPosy >= (int)MAX_SPRITE_HEIGHT) MouseFinPosy = MAX_SPRITE_HEIGHT - 1;
            if (Mouse_Mode == 3)
            {
                if (nEditExtraResolutionS)
                    MycRom.SpriteColoredX[(acSprite * MAX_SPRITE_HEIGHT + MouseFinPosy) * MAX_SPRITE_WIDTH + MouseFinPosx] =
                    MycRP.acEditColorsS[noSprSel];
                else
                    MycRom.SpriteColored[(acSprite * MAX_SPRITE_HEIGHT + MouseFinPosy) * MAX_SPRITE_WIDTH + MouseFinPosx] =
                    MycRP.acEditColorsS[noSprSel];
            }
            return;
        }
    }
}
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (MycRom.name[0] == 0) return;
    if (ExtraResFClicked && MycRom.isExtraFrame[acFrame] == 0 && Edit_Mode == 1) return;
    UINT fw, fh;
    UINT16* pfr;
    UINT8* pdynm;
    if (ExtraResFClicked && Edit_Mode == 1)
    {
        fw = MycRom.fWidthX;
        fh = MycRom.fHeightX;
        pfr = MycRom.cFramesX;
        pdynm = MycRom.DynaMasksX;
    }
    else
    {
        fw = MycRom.fWidth;
        fh = MycRom.fHeight;
        pfr = MycRom.cFrames;
        pdynm = MycRom.DynaMasksX;
    }
    if (Paste_Mode)
    {
        POINT tpt;
        GetCursorPos(&tpt);
        int xspos, yspos;
        glfwGetWindowPos(glfwframe, &xspos, &yspos);
        if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
        {
            paste_offsetx = (int)((tpt.x - xspos) / (2 * frame_zoom) - Paste_Width / 2);
            paste_offsety = (int)((tpt.y - yspos) / (2 * frame_zoom) - Paste_Height / 2);
        }
        else
        {
            paste_offsetx = (int)((tpt.x - xspos) / frame_zoom - Paste_Width / 2);
            paste_offsety = (int)((tpt.y - yspos) / frame_zoom - Paste_Height / 2);
        }
    }
    else if ((window == glfwframe) && (Color_Pipette == 0))
    {
        if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
        {
            MouseFinPosx = (int)(xpos / (2 * frame_zoom) + offset_frame_x);
            MouseFinPosy = (int)(ypos / (2 * frame_zoom) + offset_frame_y);
        }
        else
        {
            MouseFinPosx = (int)(xpos / frame_zoom);
            MouseFinPosy = (int)(ypos / frame_zoom);
        }
        if (Edit_Mode == 0 || (Edit_Mode == 1 && MycRP.Draw_Mode != 3 && MycRP.Draw_Mode != 5))
        {
            if (MouseFinPosx < 0) MouseFinPosx = 0;
            if (MouseFinPosx >= (int)fw) MouseFinPosx = fw - 1;
            if (MouseFinPosy < 0) MouseFinPosy = 0;
            if (MouseFinPosy >= (int)fh) MouseFinPosy = fh - 1;
        }
        
        switch (Mouse_Mode)
        {
            case 1:
            {
                if (!isDel_Mode) MycRom.CompMasks[(MycRom.CompMaskID[acFrame] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] = 1;
                else MycRom.CompMasks[(MycRom.CompMaskID[acFrame] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] = 0;
                break;
            }
            case 3:
            {
                if (!Copy_Mode)
                {
                    UINT16 tcol;
                    if (MycRP.DrawColMode == 1)
                    {
                        if (nEditExtraResolutionF)
                        {
                            if (fh == 64)
                                tcol = MycRP.acEditColorsS[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy / 2) * MycRom.fWidth + MouseFinPosx / 2]];
                            else
                                tcol = MycRP.acEditColorsS[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy * 2) * MycRom.fWidth + MouseFinPosx * 2]];
                        }
                        else
                            tcol = MycRP.acEditColorsS[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx]];
                    }
                    else
                        tcol = MycRP.acEditColorsS[noColSel];
                    for (UINT32 ti = 0; ti < nSelFrames; ti++)
                    {
                        if (isDel_Mode)
                        {
                            if (nEditExtraResolutionF)
                            {
                                if (MycRom.isExtraFrame[SelFrames[ti]] == 0) continue;
                                if (fh == 64)
                                    pfr[(SelFrames[ti] * fh + MouseFinPosy) * fw + MouseFinPosx] = originalcolors[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy / 2) * MycRom.fWidth + MouseFinPosx / 2]];
                                else
                                    pfr[(SelFrames[ti] * fh + MouseFinPosy) * fw + MouseFinPosx] = originalcolors[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy * 2) * MycRom.fWidth + MouseFinPosx * 2]];
                            }
                            else
                                pfr[(SelFrames[ti] * fh + MouseFinPosy) * fw + MouseFinPosx] = originalcolors[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx]];
                        }
                        else pfr[(SelFrames[ti] * fh + MouseFinPosy) * fw + MouseFinPosx] = tcol;
                    }
                    CheckNewRotation(MouseFinPosx, MouseFinPosy, tcol);
                }
                else
                {
                    if ((GetKeyState(VK_SHIFT) & 0x8000) > 0) Copy_Mask[MouseFinPosx + MouseFinPosy * fw] = 0;
                    else Copy_Mask[MouseFinPosx + MouseFinPosy * fw] = 1;
                }
                break;
            }
            case 5:
            {
                for (UINT32 ti = 0; ti < nSelFrames; ti++)
                {
                    if (isDel_Mode)
                    {
                        pdynm[(SelFrames[ti] * fh + MouseFinPosy) * fw + MouseFinPosx] = 255;
                        if (nEditExtraResolutionF)
                        {
                            if (fh == 64)
                                pfr[(SelFrames[ti] * fh + MouseFinPosy) * fw + MouseFinPosx] = originalcolors[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy / 2) * MycRom.fWidth + MouseFinPosx / 2]];
                            else
                                pfr[(SelFrames[ti] * fh + MouseFinPosy) * fw + MouseFinPosx] = originalcolors[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy * 2) * MycRom.fWidth + MouseFinPosx * 2]];
                        }
                        else
                            MycRom.cFrames[(SelFrames[ti] * fh + MouseFinPosy) * fw + MouseFinPosx] = originalcolors[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx]];
                    }
                    else pdynm[(SelFrames[ti] * fh + MouseFinPosy) * fw + MouseFinPosx] = acDynaSet;
                }
                RotationsInFrame[MouseFinPosy * fw + MouseFinPosx][0] = 0xffff;
                break;
            }
            case 7:
            {
                if (isDel_Mode) Copy_Mask[MouseFinPosy * fw + MouseFinPosx] = 0;
                else
                {
                    if (!(GetKeyState('W') & 0x8000) || (pfr[(acFrame * fh + MouseFinPosy) * fw + MouseFinPosx] != 0))
                        Copy_Mask[MouseFinPosy * fw + MouseFinPosx] = 1;
                }
                break;
            }
        }
    }
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (MycRom.name[0] == 0) return;
    if ((action == GLFW_PRESS) && (Mouse_Mode != 0)) return;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int xipos = (int)xpos, yipos = (int)ypos;
    if (window == glfwframe)
    {
        if (ExtraResFClicked && MycRom.isExtraFrame[acFrame] == 0) return;
        int xgrid, ygrid;
        if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
        {
            xgrid = (int)(xipos / (2 * frame_zoom) + offset_frame_x);
            ygrid = (int)(yipos / (2 * frame_zoom) + offset_frame_y);
        }
        else
        {
            xgrid = (int)(xipos / frame_zoom);// +ac_offset_frame_x;
            ygrid = (int)(yipos / frame_zoom);// +ac_offset_frame_y;
        }
        //if (xgrid < 0) xgrid = 0;
        //if (ygrid < 0) ygrid = 0;
        if (action == GLFW_PRESS)
        {
            if (Edit_Mode == 0)
            {
                //if (xgrid >= MycRom.fWidth) xgrid = MycRom.fWidth - 1;
                //if (ygrid >= MycRom.fHeight) ygrid = MycRom.fHeight - 1;
                if ((button == GLFW_MOUSE_BUTTON_LEFT) && (!(mods & (GLFW_MOD_ALT | GLFW_MOD_CONTROL))) && (MycRom.CompMaskID[acFrame] < 255))
                {
                    if (mods & GLFW_MOD_SHIFT) isDel_Mode = true; else isDel_Mode = false;
                    if (MycRP.Mask_Sel_Mode == 0)
                    {
                        SaveAction(true, SA_COMPMASK);
                        Mouse_Mode = 1;
                        if (!isDel_Mode) MycRom.CompMasks[(MycRom.CompMaskID[acFrame] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] = 1;
                        else MycRom.CompMasks[(MycRom.CompMaskID[acFrame] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] = 0;
                    }
                    else
                    {
                        Mouse_Mode = 2;
                        MouseIniPosx = xgrid;
                        MouseIniPosy = ygrid;
                        MouseFinPosx = xgrid;
                        MouseFinPosy = ygrid;
                    }
                    return;
                }
            }
            {
                UINT fh, fw;
                UINT16* pfr, * pdync, * pbg = NULL;
                UINT8* pdynm, * pbgm = NULL;
                UINT16 BGID = MycRom.BackgroundID[acFrame], fcol;
                if (nEditExtraResolutionF)
                {
                    fw = MycRom.fWidthX;
                    fh = MycRom.fHeightX;
                    pfr = MycRom.cFramesX;

                    pdynm = MycRom.DynaMasksX;
                    pdync = MycRom.Dyna4ColsX;
                    if (BGID<0xffff && MycRom.isExtraBackground[BGID] > 0)
                    {
                        pbg = MycRom.BackgroundFramesX;
                        pbgm = MycRom.BackgroundMaskX;
                    }
                }
                else
                {
                    fw = MycRom.fWidth;
                    fh = MycRom.fHeight;
                    pfr = MycRom.cFrames;
                    pdynm = MycRom.DynaMasks;
                    pdync = MycRom.Dyna4Cols;
                    if (BGID < 0xffff)
                    {
                        pbg = MycRom.BackgroundFrames;
                        pbgm = MycRom.BackgroundMask;
                    }
                }
                if (Color_Pipette > 0)
                {
                    if (button == GLFW_MOUSE_BUTTON_LEFT)
                    {
                        if (xgrid < 0) xgrid = 0;
                        if (ygrid < 0) ygrid = 0;
                        if (xgrid >= (int)fw) xgrid = fw - 1;
                        if (ygrid >= (int)fh) ygrid = fh - 1;
                        UINT xgo, ygo;
                        if (nEditExtraResolutionF)
                        {
                            if (fh == 64)
                            {
                                xgo = xgrid / 2;
                                ygo = ygrid / 2;
                            }
                            else
                            {
                                xgo = xgrid * 2;
                                ygo = ygrid * 2;
                            }
                        }
                        else
                        {
                            xgo = xgrid;
                            ygo = ygrid;
                        }

                        if (pbgm != NULL && pbgm[(acFrame * fh + ygrid) * fw + xgrid] > 0 && MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ygo * MycRom.fWidth + xgo] == 0)
                            fcol = pbg[(BGID * fh + ygrid) * fw + xgrid];
                        else if (pdynm[(acFrame * fh + ygrid) * fw + xgrid] < 255)
                            fcol = pdync[acFrame * MAX_DYNA_SETS_PER_FRAMEN * MycRom.noColors + pdynm[(acFrame * fh + ygrid) * fw + xgrid] * MycRom.noColors + MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ygo * MycRom.fWidth + xgo]];
                        else
                            fcol = pfr[acFrame * fw * fh + ygrid * fw + xgrid];
                        if (Color_Pipette > 1)
                        {
                            SaveAction(true, SA_PALETTE);
                            MycRP.Palette[Color_Pipette] = fcol;
                            Color_Pipette = 0;
                        }
                        else if (!(mods & GLFW_MOD_SHIFT))
                        {
                            SaveAction(true, SA_ACCOLORS);
                            MycRP.acEditColorsS[noColSel] = fcol;// pfr[(acFrame * fh + ygrid) * fw + xgrid];
                            InvalidateRect(GetDlgItem(hwTB, IDC_COL1 + noColSel), NULL, FALSE);
                            InvalidateRect(GetDlgItem(hwTB2, IDC_COL1 + noColSel), NULL, FALSE);
                            Color_Pipette = 0;
                        }
                        else
                        {
                            //UINT16 nocolor= MycRom.cFrames[(acFrame * fh + ygrid) * fw + xgrid];
                            for (UINT ti = 0; ti < MycRom.noColors; ti++)
                            {
                                if (MycRP.acEditColorsS[ti] == fcol) noColSel = ti;
                                InvalidateRect(GetDlgItem(hwTB, IDC_COL1 + noColSel), NULL, FALSE);
                                InvalidateRect(GetDlgItem(hwTB2, IDC_COL1 + noColSel), NULL, FALSE);
                                Color_Pipette = 0;
                            }
                        }
                        glfwSetCursor(glfwframe, glfwarrowcur);
                    }
                    return;
                }
                else if (Paste_Mode)
                {
                    int ofx=0, ofy=0;
                    if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                    {
                        ofx = offset_frame_x;
                        ofy = offset_frame_y;
                    }
                    if (GetKeyState(VK_MENU) & 0x8000)
                    {
                        SaveAction(true, SA_DYNAMASK);
                        for (UINT32 tk = 0; tk < nSelFrames; tk++)
                        {
                            for (int tj = 0; tj < (int)Paste_Height; tj++)
                            {
                                for (int ti = 0; ti < (int)Paste_Width; ti++)
                                {
                                    if (((ti + paste_offsetx) < 0) || ((ti + paste_offsetx) >= (int)fw) || ((tj + paste_offsety) < 0) || ((tj + paste_offsety) >= (int)fh)) continue;
                                    if (Paste_Mask[tj * Paste_Width + ti] > 0) //&& Paste_Dyna[tj * Paste_Width + ti] == Copy_From_DynaMask)
                                    {
                                        pdynm[SelFrames[tk] * fw * fh + (tj + paste_offsety) * fw + (ti + paste_offsetx)] = Paste_Dyna[tj * Paste_Width + ti];
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        bool condpaste = false;
                        if (GetKeyState(VK_SHIFT) & 0x8000) condpaste = true;
                        SaveAction(true, SA_FULLDRAW);
                        for (UINT32 tk = 0; tk < nSelFrames; tk++)
                        {
                            if (nEditExtraResolutionF && MycRom.isExtraFrame[SelFrames[tk]] == 0) continue;
                            int cff = Copy_From_Frame;
                            if (cff == -1) cff = acFrame;
                            CopyDynaCol(cff, SelFrames[tk]);
                            for (UINT tj = 0; tj < Paste_Height; tj++)
                            {
                                for (UINT ti = 0; ti < Paste_Width; ti++)
                                {
                                    int i = ti, j = tj;
                                    if (Paste_Mirror & 1) i = Paste_Width - 1 - ti;
                                    if (Paste_Mirror & 2) j = Paste_Height - 1 - tj;
                                    if (Paste_Mask[j * Paste_Width + i] == 0) continue;
                                    if (((ti + paste_offsetx) < 0) || ((ti + paste_offsetx) >= fw) || ((tj + paste_offsety) < 0) || ((tj + paste_offsety) >= fh)) continue;
                                    if (condpaste)
                                    {
                                        if (nEditExtraResolutionF)
                                        {
                                            if (fh == 64)
                                            {
                                                if (Paste_Colo[i / 2 + j / 2 * Paste_Width] != MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + (tj + paste_offsety) / 2 * MycRom.fWidth + (ti + paste_offsetx) / 2]) continue;
                                            }
                                            else
                                            {
                                                if (Paste_Colo[i * 2 + j * 2 * Paste_Width] != MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + (tj + paste_offsety) * 2 * MycRom.fWidth + (ti + paste_offsetx) * 2]) continue;
                                            }
                                        }
                                        else if (Paste_Colo[i + j * Paste_Width] != MycRP.oFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + (tj + paste_offsety) * MycRom.fWidth + (ti + paste_offsetx)]) continue;
                                    }
                                    pdynm[SelFrames[tk] * fw * fh + (tj + paste_offsety + ofy) * fw + (ti + paste_offsetx + ofx)] = Paste_Dyna[i + j * Paste_Width];
                                    pfr[SelFrames[tk] * fw * fh + (tj + paste_offsety + ofy) * fw + (ti + paste_offsetx + ofx)] = Paste_ColN[i + j * Paste_Width];
                                }
                            }
                        }
                    }
                    Paste_Mode = false;
                    glfwSetCursor(glfwframe, glfwarrowcur);
                    InitColorRotation();
                    UpdateFSneeded = true;
                    for (UINT ti = 0; ti < MycRom.noColors; ti++) InvalidateRect(GetDlgItem(hwTB, IDC_DYNACOL1 + ti), NULL, TRUE);
                    return;
                }
                else if ((button == GLFW_MOUSE_BUTTON_LEFT) && (!(mods & (GLFW_MOD_ALT | GLFW_MOD_CONTROL))))
                {
                    isDel_Mode = false;
                    if (mods & GLFW_MOD_SHIFT) isDel_Mode = true; else isDel_Mode = false;
                    if (MycRP.Draw_Mode == 0)
                    {
                        SaveAction(true, SA_DRAW);
                        Mouse_Mode = 3;
                        UINT16 tcol;
                        UINT8 colo;
                        if (nEditExtraResolutionF)
                        {
                            if (fh == 64)
                                colo = MycRP.oFrames[(acFrame * fh + ygrid / 2) * fw + xgrid / 2];
                            else
                                colo = MycRP.oFrames[(acFrame * fh + ygrid * 2) * fw + xgrid * 2];
                        }
                        else colo = MycRP.oFrames[(acFrame * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid];
                        if (MycRP.DrawColMode == 1) tcol = MycRP.acEditColorsS[colo];
                        else tcol = MycRP.acEditColorsS[noColSel];
                        for (UINT32 ti = 0; ti < nSelFrames; ti++)
                        {
                            if (nEditExtraResolutionF && MycRom.isExtraFrame[SelFrames[ti]] == 0) continue;
                            if (isDel_Mode) pfr[(SelFrames[ti] * fh + ygrid) * fw + xgrid] = MycRP.Palette[colo];
                            else pfr[(SelFrames[ti] * fh + ygrid) * fw + xgrid] = tcol;
                        }
                        CheckNewRotation(xgrid, ygrid, tcol);
                    }
                    else
                    {
                        Mouse_Mode = 4;
                        MouseIniPosx = xgrid;
                        MouseIniPosy = ygrid;
                        MouseFinPosx = xgrid;
                        MouseFinPosy = ygrid;
                    }
                    return;
                }
                else if (button == GLFW_MOUSE_BUTTON_LEFT)
                {
                    if (!(mods & GLFW_MOD_ALT))
                    {
                        isDel_Mode = false;
                        if (mods & GLFW_MOD_SHIFT) isDel_Mode = true; else isDel_Mode = false;
                        if (MycRP.Draw_Mode == 0)
                        {
                            SaveAction(true, SA_COPYMASK);
                            Mouse_Mode = 7;
                            int ti;
                            int tio;
                            if (nEditExtraResolutionF)
                            {
                                fw = MycRom.fWidthX;
                                fh = MycRom.fHeightX;
                                ti = ygrid * fw + xgrid;
                                if (fh == 64) tio = ti / 2; else tio = ti * 2;
                                pfr = &MycRom.cFramesX[acFrame * fw * fh];
                                pdynm = &MycRom.DynaMasksX[acFrame * fw * fh];
                            }
                            else
                            {
                                fw = MycRom.fWidth;
                                fh = MycRom.fHeight;
                                ti = ygrid * fw + xgrid;
                                tio = ti;
                                pfr = &MycRom.cFrames[acFrame * fw * fh];
                                pdynm = &MycRom.DynaMasks[acFrame * fw * fh];
                            }
                            if (!isDel_Mode) Copy_Mask[ti] = 1;
                            else Copy_Mask[ti] = 0;
                            Copy_ColN[ti] = pfr[ti];
                            Copy_Colo[ti] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + tio];
                            Copy_Dyna[ti] = pdynm[ti];
                            GetSelectionSize();
                        }
                        else
                        {
                            Mouse_Mode = 8;
                            MouseIniPosx = xgrid;
                            MouseIniPosy = ygrid;
                            MouseFinPosx = xgrid;
                            MouseFinPosy = ygrid;
                        }
                        return;
                    }
                    else
                    { 
                        if (MycRP.Draw_Mode != 4) return;
                        
                        if ((!ExtraResFClicked && MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + ygrid * MycRom.fWidth + xgrid] != 255) ||
                            (ExtraResFClicked && MycRom.DynaMasksX[acFrame * MycRom.fWidthX * MycRom.fHeightX + ygrid * MycRom.fWidthX + xgrid] != 255))
                        {
                            MessageBoxA(hWnd, "The pixel you clicked is dynamically colored, this function works only for static content.", "Improper use", MB_OK);
                            return;
                        }
                        if (mods & GLFW_MOD_CONTROL)
                        {
                            SaveAction(true, SA_COPYMASK);
                            if (mods & GLFW_MOD_SHIFT) isDel_Mode = true; else isDel_Mode = false;
                            Mouse_Mode = 7;
                            if (nEditExtraResolutionF)
                            {
                                fw = MycRom.fWidthX;
                                fh = MycRom.fHeightX;
                                pfr = &MycRom.cFramesX[acFrame * fw * fh];
                                pdynm = &MycRom.DynaMasksX[acFrame * fw * fh];
                            }
                            else
                            {
                                fw = MycRom.fWidth;
                                fh = MycRom.fHeight;
                                pfr = &MycRom.cFrames[acFrame * fw * fh];
                                pdynm = &MycRom.DynaMasks[acFrame * fw * fh];
                            }
                            UINT16 col_to_find = pfr[ygrid * fw + xgrid];
                            for (UINT ti = 0; ti < fw * fh; ti++)
                            {
                                if (pfr[ti] == col_to_find)
                                {
                                    if (pdynm[ti] != 255) continue;
                                    if (!isDel_Mode) Copy_Mask[ti] = 1;
                                    else Copy_Mask[ti] = 0;
                                    Copy_ColN[ti] = pfr[ti];
                                    if (!nEditExtraResolutionF)
                                        Copy_Colo[ti] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
                                    else
                                    {
                                        if (fh == 64)
                                            Copy_Colo[ti] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti / 2];
                                        else
                                            Copy_Colo[ti] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti * 2];
                                    }
                                    Copy_Dyna[ti] = pdynm[ti];
                                }
                            }
                            GetSelectionSize();
                            return;
                        }
                        else
                        {
                            SaveAction(true, SA_DRAW);
                            Mouse_Mode = 0;
                            if (nEditExtraResolutionF)
                            {
                                fw = MycRom.fWidthX;
                                fh = MycRom.fHeightX;
                                pfr = MycRom.cFramesX;
                                pdynm = MycRom.DynaMasksX;
                            }
                            else
                            {
                                fw = MycRom.fWidth;
                                fh = MycRom.fHeight;
                                pfr = MycRom.cFrames;
                                pdynm = MycRom.DynaMasks;
                            }
                            UINT16 col_to_find = pfr[acFrame * fw * fh + ygrid * fw + xgrid];
                            for (UINT tj = 0; tj < nSelFrames; tj++)
                            {
                                if (nEditExtraResolutionF && MycRom.isExtraFrame[SelFrames[tj]] == 0) continue;
                                for (UINT ti = 0; ti < fw * fh; ti++)
                                {
                                    if (pfr[SelFrames[tj] * fw * fh + ti] == col_to_find)
                                    {
                                        if (MycRP.DrawColMode == 1)
                                        {
                                            if (!nEditExtraResolutionF)
                                                pfr[SelFrames[tj] * fw * fh + ti] = MycRP.acEditColorsS[MycRP.oFrames[SelFrames[tj] * MycRom.fHeight * MycRom.fWidth + ti]];
                                            else
                                            {
                                                if (fh == 64)
                                                    pfr[SelFrames[tj] * fw * fh + ti] = MycRP.acEditColorsS[MycRP.oFrames[SelFrames[tj] * MycRom.fHeight * MycRom.fWidth + ti / 2]];
                                                else
                                                    pfr[SelFrames[tj] * fw * fh + ti] = MycRP.acEditColorsS[MycRP.oFrames[SelFrames[tj] * MycRom.fHeight * MycRom.fWidth + ti * 2]];
                                            }
                                        }
                                        else pfr[SelFrames[tj] * fw * fh + ti] = MycRP.acEditColorsS[noColSel];
                                    }
                                }
                            }
                        }
                        UpdateFSneeded = true;
                        return;
                    }

                }
                else if ((button == GLFW_MOUSE_BUTTON_RIGHT) && (!(mods & (GLFW_MOD_ALT | GLFW_MOD_CONTROL))))
                {
                    isDel_Mode = false;
                    if (mods & GLFW_MOD_SHIFT) isDel_Mode = true; else isDel_Mode = false;
                    if (MycRP.Draw_Mode == 0)
                    {
                        SaveAction(true, SA_DYNAMASK);
                        Mouse_Mode = 5;
                        if (nEditExtraResolutionF)
                        {
                            fw = MycRom.fWidthX;
                            fh = MycRom.fHeightX;
                            pfr = MycRom.cFramesX;
                            pdynm = MycRom.DynaMasksX;
                        }
                        else
                        {
                            fw = MycRom.fWidth;
                            fh = MycRom.fHeight;
                            pfr = MycRom.cFrames;
                            pdynm = MycRom.DynaMasks;
                        }
                        for (UINT32 ti = 0; ti < nSelFrames; ti++)
                        {
                            if (nEditExtraResolutionF && MycRom.isExtraFrame[SelFrames[ti]] == 0) continue;
                            if (!isDel_Mode) pdynm[(SelFrames[ti] * fh + ygrid) * fw + xgrid] = acDynaSet;
                            else
                            {
                                if (pdynm[(SelFrames[ti] * fh + ygrid) * fw + xgrid] < 255)
                                {
                                    if (!nEditExtraResolutionF)
                                        pfr[(SelFrames[ti] * fh + ygrid) * fw + xgrid] =
                                        MycRP.oFrames[(SelFrames[ti] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid];
                                    else
                                    {
                                        if (fh == 64)
                                        {
                                            pfr[(SelFrames[ti] * fh + ygrid) * fw + xgrid] =
                                                MycRP.oFrames[(SelFrames[ti] * MycRom.fHeight + ygrid / 2) * MycRom.fWidth + xgrid / 2];
                                        }
                                        else
                                        {
                                            pfr[(SelFrames[ti] * fh + ygrid) * fw + xgrid] =
                                                MycRP.oFrames[(SelFrames[ti] * MycRom.fHeight + ygrid * 2) * MycRom.fWidth + xgrid * 2];
                                        }
                                    }
                                }
                                pdynm[(SelFrames[ti] * fh + ygrid) * fw + xgrid] = 255;
                            }
                        }
                        RotationsInFrame[ygrid * fw + xgrid][0] = 0xffff;
                    }
                    else
                    {
                        Mouse_Mode = 6;
                        MouseIniPosx = xgrid;
                        MouseIniPosy = ygrid;
                        MouseFinPosx = xgrid;
                        MouseFinPosy = ygrid;
                    }
                    return;
                }
                else if (button == GLFW_MOUSE_BUTTON_RIGHT && MycRP.Draw_Mode == 4 && mods & GLFW_MOD_ALT)
                {
                    if ((!ExtraResFClicked && MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + ygrid * MycRom.fWidth + xgrid] != 255) ||
                        (ExtraResFClicked && MycRom.DynaMasksX[acFrame * MycRom.fWidthX * MycRom.fHeightX + ygrid * MycRom.fWidthX + xgrid] != 255))
                    {
                        MessageBoxA(hWnd, "The pixel you clicked is dynamically colored, this function works only for static content.", "Improper use", MB_OK);
                        return;
                    }
                    SaveAction(true, SA_DYNAMASK);
                    Mouse_Mode = 0;
                    if (nEditExtraResolutionF)
                    {
                        fw = MycRom.fWidthX;
                        fh = MycRom.fHeightX;
                        pfr = MycRom.cFramesX;
                        pdynm = MycRom.DynaMasksX;
                    }
                    else
                    {
                        fw = MycRom.fWidth;
                        fh = MycRom.fHeight;
                        pfr = MycRom.cFrames;
                        pdynm = MycRom.DynaMasks;
                    }
                    UINT16 col_to_find = pfr[acFrame * fw * fh + ygrid * fw + xgrid];
                    for (UINT tj = 0; tj < nSelFrames; tj++)
                    {
                        if (nEditExtraResolutionF && MycRom.isExtraFrame[SelFrames[tj]] == 0) continue;
                        for (UINT ti = 0; ti < fw * fh; ti++)
                        {
                            if (pfr[SelFrames[tj] * fw * fh + ti] == col_to_find)
                            {
                                pdynm[SelFrames[tj] * fw * fh + ti] = acDynaSet;
                            }
                        }
                    }
                    UpdateFSneeded = true;
                    return;
                }
            }
        }
        else if ((action == GLFW_RELEASE) && (Color_Pipette == 0) && (!Paste_Mode))
        {
            if (Edit_Mode == 0 && MycRom.CompMaskID[acFrame] != 255)
            {
                if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 2))
                {
                    SaveAction(true, SA_COMPMASK);
                    Add_Surface_To_Mask(Draw_Extra_Surface, isDel_Mode);
                }
            }
            else
            {
                if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 4))
                {
                    SaveAction(true, SA_DRAW);
                    if (MycRP.DrawColMode == 2)
                    {
                        if (MycRP.Draw_Mode==1) ConvertCopyToGradient(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy);
                        else if (MycRP.Draw_Mode == 3) ConvertCopyToRadialGradient(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy);
                        else if (MycRP.Draw_Mode == 5) ConvertCopyToEllipseRadialGradient(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy);
                    }
                    else ConvertSurfaceToFrame(Draw_Extra_Surface, isDel_Mode);
                    UpdateFSneeded = true;
                    InitColorRotation();
                }
                else if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 3)) UpdateFSneeded = true;
                else if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 8))
                {
                    SaveAction(true, SA_COPYMASK);
                    Add_Surface_To_Copy(Draw_Extra_Surface, isDel_Mode);
                }
                else if (button == GLFW_MOUSE_BUTTON_RIGHT)
                {
                    if (Mouse_Mode == 6)
                    {
                        SaveAction(true, SA_DYNAMASK);
                        Add_Surface_To_Dyna(Draw_Extra_Surface, isDel_Mode);
                        InitColorRotation();
                    }
                    UpdateFSneeded = true;
                }
            }
            Mouse_Mode = 0;
            return;
        }
    }
    else if ((window == glfwframestrip) && (MycRom.name[0] != 0))
    {
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_RELEASE)) MouseFrSliderLPressed = false;
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS))
        {
            if ((yipos >= FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 5) && (yipos <= FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 6))
            {
                if ((xipos >= FRAME_STRIP_SLIDER_MARGIN) && (xipos <= (int)ScrW - FRAME_STRIP_SLIDER_MARGIN))
                {
                    MouseFrSliderLPressed = true;
                }
                return;
            }
            UINT wid = 256;
            if (MycRom.fWidth == 192) wid = 192;
            if ((yipos >= FRAME_STRIP_H_MARGIN) && (yipos < FRAME_STRIP_H_MARGIN + 128))
            {
                if ((xipos >= (int)FS_LMargin + FRAME_STRIP_W_MARGIN) && (xipos <= (int)ScrW - (int)FS_LMargin - FRAME_STRIP_W_MARGIN))
                {
                    if ((xipos - FS_LMargin- FRAME_STRIP_W_MARGIN) % (wid + FRAME_STRIP_W_MARGIN) < wid)
                    {
                        SaveAction(true, SA_SELECTION);
                        prevFrame = acFrame;
                        acFrame = (UINT)-1;
                        bool frameshift = false;
                        UINT tpFrame = PreFrameInStrip + (xipos - FS_LMargin - FRAME_STRIP_W_MARGIN) / (wid + FRAME_STRIP_W_MARGIN);
                        if (tpFrame >= MycRom.nFrames) tpFrame = MycRom.nFrames - 1;
                        if (tpFrame < 0) tpFrame = 0;
                        if (mods & GLFW_MOD_SHIFT)
                        {
                            if (mods & GLFW_MOD_CONTROL)
                            {
                                for (UINT ti = min(prevFrame, tpFrame); ti <= max(prevFrame, tpFrame); ti++) Add_Selection_Frame(ti);
                            }
                            else if (mods & GLFW_MOD_ALT)
                            {
                                for (UINT ti = min(prevFrame, tpFrame); ti <= max(prevFrame, tpFrame); ti++) Del_Selection_Frame(ti);
                            }
                            else
                            {
                                nSelFrames = 0;
                                for (UINT ti = min(prevFrame, tpFrame); ti <= max(prevFrame, tpFrame); ti++) Add_Selection_Frame(ti);
                            }
                        }
                        else if (mods & GLFW_MOD_CONTROL)
                        {
                            if ((isFrameSelected(tpFrame) != -1) && (nSelFrames > 1)) Del_Selection_Frame(tpFrame); else Add_Selection_Frame(tpFrame);
                        }
                        else
                        {
                            int iSF = isFrameSelected(tpFrame);
                            if ((iSF == -1) || (tpFrame == prevFrame))
                            {
                                SelFrames[0] = tpFrame;
                                nSelFrames = 1;
                            }
                            else frameshift = true;
                        }
                        if (isFrameSelected(tpFrame) == -1)
                        {
                            acFrame = SelFrames[0];
                        }
                        else acFrame = tpFrame;
                        InitColorRotation();

                        if (Edit_Mode == 1)
                        {
                            for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++)
                            {
                                InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                                InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                            }
                            for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                        }
                        if (!frameshift)
                        {
                            for (UINT ti = 0; ti < 256 * 64; ti++) Copy_Mask[ti] = 0;
                        }
                        GetSelectionSize();
                        Check_Commons();
                        Paste_Mode = false;
                        UpdateFSneeded = true;
                        Deactivate_Draw_All_Sel();
                        UpdateNewacFrame();
                        AllSameFramesUpdated = false;
                        //SetDlgItemTextA(hwTB, IDC_LISTSAMEFR, "Best Mask ???");
                        if (Edit_Mode == 0) InvalidateRect(GetDlgItem(hwTB, IDC_MASKLIST), NULL, FALSE);
                        SetMultiWarningF();
                    }
                }
            }
        }
        return;
    }
}
void mouse_button_callback2(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int xipos = (int)xpos, yipos = (int)ypos;
    if (MycRom.name[0] == 0 || MycRom.nSprites == 0) return;
    if (window == glfwsprites)
    {
        if (ExtraResSClicked && MycRom.isExtraSprite[acSprite] == 0) return;
        int xgrid, ygrid;
        if (Zoom_Pushed_Sprite)// && ((nEditExtraResolutionS && MycRom.fHeightX == 64) || (!nEditExtraResolutionS && MycRom.fHeight == 64)))
        {
            xgrid = (int)(xipos / (2 * sprite_zoom) + offset_sprite_x);
            ygrid = (int)(yipos / (2 * sprite_zoom) + offset_sprite_y);
        }
        else
        {
            xgrid = (int)(xipos / sprite_zoom);
            ygrid = (int)(yipos / sprite_zoom);
        }
        if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_LEFT))
        {
            if (Sprite_Mode == 0)
            {
                SaveAction(true, SA_SPRITEDRAW);
                Mouse_Mode = 3;
                UINT16 tcol = MycRP.acEditColorsS[noSprSel];
                UINT fw, fh;
                UINT16* pspr;
                if (nEditExtraResolutionS)
                {
                    fw = MycRom.fWidthX;
                    fh = MycRom.fHeightX;
                    pspr = &MycRom.SpriteColoredX[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
                }
                else
                {
                    fw = MycRom.fWidth;
                    fh = MycRom.fHeight;
                    pspr = &MycRom.SpriteColored[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
                }
                pspr[ygrid * MAX_SPRITE_WIDTH + xgrid] = tcol;
            }
            else
            {
                Mouse_Mode = 4;
                MouseIniPosx = xgrid;
                MouseIniPosy = ygrid;
                MouseFinPosx = xgrid;
                MouseFinPosy = ygrid;
            }
            return;
        }
        else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (Mouse_Mode == 4)
            {
                SaveAction(true, SA_SPRITEDRAW);
                ConvertSurfaceToSprite(Draw_Extra_Surface2);
                UpdateSSneeded = true;
            }
            else if (Mouse_Mode == 3) UpdateSSneeded = true;
            Mouse_Mode = 0;
            return;
        }
        else if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_RIGHT))
        {
            if (nEditExtraResolutionS)
            {
                MessageBoxA(hwTB2, "Please switch to original resolution edition to draw detection areas", "Incompatible edition mode", MB_OK);
                return;
            }
            Mouse_Mode = 2;
            MouseIniPosx = xgrid;
            MouseIniPosy = ygrid;
            MouseFinPosx = xgrid;
            MouseFinPosy = ygrid;
            return;
        }
        else if ((action == GLFW_RELEASE) && (button == GLFW_MOUSE_BUTTON_RIGHT))
        {
            if (nEditExtraResolutionS) return;
            if (Mouse_Mode == 2)
            {
                UINT nsuiv = 0;
                Mouse_Mode = 0;
                for (UINT ti = 0; ti < MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT - 3; ti++)
                {
                    if (Draw_Extra_Surface2[ti] > 0)
                    {
                        if (MycRom.SpriteOriginal[acSprite * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT + ti] < 255) nsuiv++; else nsuiv = 0;
                    }
                    if (nsuiv == 4) break;
                }
                if (nsuiv < 4)
                {
                    MessageBoxA(hSprites, "This selection doesn't have a sequence of 4 consecutive pixels active in the sprite, it can't be used for identification", "Invalid selection", MB_OK);
                    return;
                }
                SaveAction(true, SA_SPRITE);
                ConvertSurfaceToDetection(Draw_Extra_Surface2);
            }
            return;
        }
    }
    else if ((window == glfwspritestrip) && (MycRom.name[0] != 0))
    {
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_RELEASE)) MouseSpSliderLPressed = false;
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS))
        {
            if ((yipos >= FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 5) && (yipos <= FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 6))
            {
                if ((xipos >= FRAME_STRIP_SLIDER_MARGIN) && (xipos <= (int)ScrW2 - FRAME_STRIP_SLIDER_MARGIN))
                {
                    MouseSpSliderLPressed = true;
                }
                return;
            }
            UINT wid = MAX_SPRITE_WIDTH;
            if ((yipos >= FRAME_STRIP_H_MARGIN) && (yipos < FRAME_STRIP_H_MARGIN + MAX_SPRITE_HEIGHT))
            {
                if ((xipos >= (int)SS_LMargin + FRAME_STRIP_W_MARGIN) && (xipos <= (int)ScrW2 - (int)SS_LMargin - FRAME_STRIP_W_MARGIN))
                {
                    if ((xipos - SS_LMargin- FRAME_STRIP_W_MARGIN) % (wid + FRAME_STRIP_W_MARGIN) < wid)
                    {
                        SaveAction(true, SA_ACSPRITE);
                        UINT tnspr = PreSpriteInStrip + (xipos - SS_LMargin- FRAME_STRIP_W_MARGIN) / (wid + FRAME_STRIP_W_MARGIN);
                        if ((tnspr < 0) || (tnspr >= MycRom.nSprites)) return;
                        acSprite = tnspr;
                        nSelSprites = 1;
                        SelSprites[0] = tnspr;
                        //for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                        Paste_Mode = false;
                        if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
                        UpdateSSneeded = true;
                        SendMessage(GetDlgItem(hwTB2, IDC_SPRITELIST), CB_SETCURSEL, (WPARAM)acSprite, 0);
                        SetDlgItemTextA(hwTB2, IDC_SPRITENAME, &MycRP.Sprite_Names[acSprite * SIZE_SECTION_NAMES]);
                        if (ExtraResSClicked && MycRom.isExtraSprite[acSprite] > 0) nEditExtraResolutionS = true; else nEditExtraResolutionS = false;
                    }
                }
            }
        }
        return;
    }
}
void mouse_scroll_callback3(GLFWwindow* window, double xoffset, double yoffset)
{
    short step = (short)-yoffset;
    if (GetKeyState(VK_SHIFT) & 0x8000) step *= 10;
    if (!image_loaded) return;
    crop_reduction += step;
    if (crop_reduction < 0) crop_reduction = 0;
    else if ((crop_reduction > (int)image_sizeW - 20) && (initcropwidth > 0)) crop_reduction = image_sizeW - 20;
    else if ((crop_reduction > (int)image_sizeH - 20) && (initcropheight > 0)) crop_reduction = image_sizeH - 20;
    if (crop_offsetx + crop_sizeW > image_sizeW) crop_offsetx = image_sizeW - crop_sizeW;
    if (crop_offsetx < 0) crop_offsetx = 0;
    if (crop_offsety + crop_sizeH > image_sizeH) crop_offsety = image_sizeH - crop_sizeH;
    if (crop_offsety < 0) crop_offsety = 0;
    UpdateCropSize();
}
void mouse_move_callback3(GLFWwindow* window, double xpos, double ypos)
{
    //bool backreset = false;
    float tratiow = (float)MonWidth / (float)image_sizeW;
    float tratioh = (float)MonHeight / (float)image_sizeH;
    if ((xpos >= image_posx) && (xpos < image_posx + image_sizeW) && (ypos >= image_posy) && (ypos < image_posy + image_sizeH))
    {
        if (xpos < image_posx + crop_sizeW / 2)
            crop_offsetx = 0;
        else if (xpos > image_posx + image_sizeW - crop_sizeW / 2)
            crop_offsetx = image_sizeW - crop_sizeW;
        else crop_offsetx = (int)xpos - image_posx - crop_sizeW / 2;
        if (ypos < image_posy + crop_sizeH / 2)
            crop_offsety = 0;
        else if (ypos > image_posy + image_sizeH - crop_sizeH / 2)
            crop_offsety = image_sizeH - crop_sizeH;
        else crop_offsety = (int)ypos - image_posy - crop_sizeH / 2;
    }
    if (image_mouseLpressed || image_mouseRpressed)
    {
        //if (!backreset) for (int ti = 0; ti < MonWidth * MonHeight; ti++) pFilterImage[ti * 4 + 3] = IMAGE_MASK_OPACITY;
        if (image_mouseLpressed)
        {
            crop_ioffsetx = crop_offsetx;
            crop_ioffsety = crop_offsety;
            crop_iOsizeW = crop_sizeW;
            crop_iOsizeH = crop_sizeH;
            crop_reductioni = crop_reduction;
            image_zoom_srce = true;
        }
        if (image_mouseRpressed)
        {
            crop_foffsetx = crop_offsetx;
            crop_foffsety = crop_offsety;
            crop_fOsizeW = crop_sizeW;
            crop_fOsizeH = crop_sizeH;
            crop_reductionf = crop_reduction;
            image_zoom_dest = true;
        }
    }
}
void mouse_button_callback3(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (!image_loaded) return;
    if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_LEFT))
    {
        image_mouseLpressed = true;
        mouse_move_callback3(window, xpos, ypos);
    }
    else if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_RIGHT))
    {
        image_mouseRpressed = true;
        mouse_move_callback3(window, xpos, ypos);
    }
    else if ((action == GLFW_RELEASE) && (button == GLFW_MOUSE_BUTTON_LEFT))
    {
        image_mouseLpressed = false;
    }
    else if ((action == GLFW_RELEASE) && (button == GLFW_MOUSE_BUTTON_RIGHT))
    {
        image_mouseRpressed = false;
    }
}
bool isPressed(int nVirtKey, DWORD* timePress)
{
    if (GetKeyState(nVirtKey) & 0x8000)
    {
        DWORD acTime = timeGetTime();
        if (acTime > (*timePress))
        {
            if ((*timePress) == 0) (*timePress) = FIRST_KEY_TIMER_INT + acTime; else(*timePress) = NEXT_KEY_TIMER_INT + acTime;
            return true;
        }
    }
    else (*timePress) = 0;
    return false;
}

#pragma endregion Window_Procs

#pragma region Window_Creations

bool SetIcon(HWND ButHWND, UINT ButIco)
{
    HICON hicon = LoadIcon(hInst, MAKEINTRESOURCE(ButIco));
    if (!hicon) AffLastError((char*)"SetIcon");
    SendMessageW(ButHWND, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon);
    DestroyIcon(hicon);
    return true;
}
bool SetImage(HWND ButHWND, UINT ButImg)
{
    HBITMAP hbitmap = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(ButImg));
    if (!hbitmap) AffLastError((char*)"SetImage");
    SendMessageW(ButHWND, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbitmap);
    DeleteObject(hbitmap);
    return true;
}

bool MaskUsed(UINT32 nomask)
{
    if (MycRom.name[0] == 0) return false;
    for (UINT32 ti = 0; ti < MycRom.nFrames; ti++)
    {
        if (MycRom.CompMaskID[ti] == nomask) return true;
    }
    return false;
}
void UpdateSectionList(void)
{
    HWND hlst = GetDlgItem(hwTB, IDC_SECTIONLIST);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)"- None -");
    for (UINT32 ti = 0; ti < MycRP.nSections; ti++)
    {
        char tbuf[256];
        sprintf_s(tbuf, 256, "%i - %s", ti + 1, &MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
        SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    int ti = Which_Section(acFrame);
    SendMessage(hlst, CB_SETCURSEL, ti + 1, 0);
}
void UpdatePuPList(void)
{
    HWND hlst = GetDlgItem(hwTB, IDC_TRIGGERLIST);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
    {
        if (MycRom.TriggerID[ti] != 0xffffffff)
        {
            char tbuf[32];
            sprintf_s(tbuf, 32, "Fr: %i/TID: %i", ti, MycRom.TriggerID[ti]);
        }
    }
}
void UpdateSpriteList(void)
{
    HWND hlst = GetDlgItem(hwTB2, IDC_SPRITELIST);
    //acSprite = (UINT)SendMessage(hlst, LB_GETCURSEL, 0, 0);
    //if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    for (UINT32 ti = 0; ti < MycRom.nSprites; ti++)
    {
        char tbuf[256];
        sprintf_s(tbuf, 256, "%s", &MycRP.Sprite_Names[ti * SIZE_SECTION_NAMES]);
        SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    SendMessage(hlst, CB_SETCURSEL, acSprite, 0);
}
void UpdateBackgroundList(void)
{
    HWND hlst = GetDlgItem(hwTB4, IDC_BGLIST);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    char tbuf[10];
    for (UINT32 ti = 0; ti < MycRom.nFrames; ti++)
    {
        if (MycRom.BackgroundID[ti] == acBG)
        {
            _itoa_s(ti, tbuf,10, 10);
            SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)tbuf);
        }
    }
}
void UpdateMaskList2(void)
{
    if (MycRom.name[0] == 0) return;
    HWND hlst = GetDlgItem(hwTB, IDC_MASKLIST2);
    HWND hlst2 = GetDlgItem(hwTB, IDC_FRUSEMASK);
    int acpos = (int)SendMessage(hlst, CB_GETCURSEL, 0, 0);
    SendMessage(hlst2, LB_RESETCONTENT, 0, 0);
    if (acpos >= 1)
    {
        for (UINT ti = 0; ti < MycRom.nFrames; ti++)
        {
            if (MycRom.CompMaskID[ti] == acpos - 1)
            {
                char tbuf[12];
                _itoa_s(ti, tbuf, 12, 10);
                SendMessageA(hlst2, LB_ADDSTRING, 0, (LPARAM)tbuf);
            }
        }
    }
    else if (acpos == 0)
    {
        for (UINT ti = 0; ti < MycRom.nFrames; ti++)
        {
            if (MycRom.CompMaskID[ti] == 255)
            {
                char tbuf[12];
                _itoa_s(ti, tbuf, 12, 10);
                SendMessageA(hlst2, LB_ADDSTRING, 0, (LPARAM)tbuf);
            }
        }
    }
}
void UpdateSpriteList2(void)
{
    if (MycRom.name[0] == 0) return;
    HWND hlst = GetDlgItem(hwTB2, IDC_SPRITELIST2);
    HWND hlst2 = GetDlgItem(hwTB2, IDC_FRUSESPRITE);
    int acpos = (int)SendMessage(hlst, CB_GETCURSEL, 0, 0);
    SendMessage(hlst2, LB_RESETCONTENT, 0, 0);
    if (acpos >= 0)
    {
        for (UINT ti = 0; ti < MycRom.nFrames; ti++)
        {
            for (UINT tj = 0; tj < MAX_SPRITES_PER_FRAME; tj++)
            {
                if (MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj] == acpos)
                {
                    char tbuf[12];
                    _itoa_s(ti, tbuf, 12, 10);
                    SendMessageA(hlst2, LB_ADDSTRING, 0, (LPARAM)tbuf);
                }
            }
        }
    }
}
void UpdateSpriteList3(void)
{
    if (MycRom.name[0] == 0) return;
    HWND hlst = GetDlgItem(hwTB2, IDC_SPRITELIST2);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    for (UINT ti = 0; ti < MycRom.nSprites; ti++) SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)&MycRP.Sprite_Names[ti * SIZE_SECTION_NAMES]);
    UpdateSpriteList2();
}
void UpdateMaskList(void)
{
    HWND hlst = GetDlgItem(hwTB, IDC_MASKLIST);
    HWND hlst2 = GetDlgItem(hwTB, IDC_MASKLIST2);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)"- None -");
    SendMessage(hlst2, CB_RESETCONTENT, 0, 0);
    SendMessageA(hlst2, CB_ADDSTRING, 0, (LPARAM)"- None -");
    wchar_t tbuf[256],tname[SIZE_MASK_NAME];
    size_t tout;
    for (UINT32 ti = 0; ti < MAX_MASKS; ti++)
    {
        if (MaskUsed(ti))
        {
            if (MycRP.Mask_Names[ti * SIZE_MASK_NAME] != 0)
                mbstowcs_s(&tout, tname, &MycRP.Mask_Names[ti * SIZE_MASK_NAME], SIZE_MASK_NAME - 1);
            else
                _itow_s(ti, tname, SIZE_MASK_NAME - 1, 10);
            swprintf_s(tbuf, 256, L"\u2611 %s", tname);
        }
        else
            swprintf_s(tbuf, 256, L"\u2610 %i", ti);
        SendMessage(hlst, CB_ADDSTRING, 0, (LPARAM)tbuf);
        SendMessage(hlst2, CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    if (MycRom.name[0])
    {
        UINT8 puc = MycRom.CompMaskID[acFrame];
        puc++;
        SendMessage(GetDlgItem(hwTB, IDC_MASKLIST), CB_SETCURSEL, (WPARAM)puc, 0);
        UpdateMaskList2();
    }
}
void UpdateTriggerID(void)
{
    if (MycRom.name[0] == 0) return;
    if (MycRom.TriggerID[acFrame] == 0xFFFFFFFF) SetDlgItemTextA(hwTB, IDC_TRIGID, "- None -");
    else
    {
        char tbuf[16];
        _itoa_s(MycRom.TriggerID[acFrame], tbuf, 16, 10);
        SetDlgItemTextA(hwTB, IDC_TRIGID,tbuf);
    }
    UpdatePuPList();
}
void SetSpotButton(bool ison)
{
    if (ison) SetIcon(GetDlgItem(hwTB, IDC_IDENT), IDI_SPOTON);
    else SetIcon(GetDlgItem(hwTB, IDC_IDENT), IDI_SPOTOFF);
}
void SetZoomButton(bool ison)
{
    if (ison) SetIcon(GetDlgItem(hwTB, IDC_ZOOM2X), IDI_ZOOM2XON);
    else SetIcon(GetDlgItem(hwTB, IDC_ZOOM2X), IDI_ZOOM2X);
}
void SetZoomSpriteButton(bool ison)
{
    if (ison) SetIcon(GetDlgItem(hwTB2, IDC_ZOOM2X), IDI_ZOOM2XON);
    else SetIcon(GetDlgItem(hwTB2, IDC_ZOOM2X), IDI_ZOOM2X);
}
void SetBackgroundMaskSpotButton(bool ison)
{
    if (ison) SetIcon(GetDlgItem(hwTB, IDC_BBSPOT), IDI_BBSPOTON);
    else SetIcon(GetDlgItem(hwTB, IDC_BBSPOT), IDI_BBSPOTOFF);
}
void SetCommonButton(bool ison)
{
    if (ison) SetIcon(GetDlgItem(hwTB, IDC_COMMON), IDI_COMMONON);
    else SetIcon(GetDlgItem(hwTB, IDC_COMMON), IDI_COMMONOFF);
}
void SetExtraResFBox(void)
{
    if (MycRom.name[0] != 0 && acFrame < MycRom.nFrames && MycRom.isExtraFrame[acFrame] > 0) SendMessage(GetDlgItem(hwTB, IDC_EXTRARES), BM_SETCHECK, BST_CHECKED, 0);
    else SendMessage(GetDlgItem(hwTB, IDC_EXTRARES), BM_SETCHECK, BST_UNCHECKED, 0);
}
void SetExtraResSBox(void)
{
    if (MycRom.name[0] != 0 && acSprite < MycRom.nSprites && MycRom.isExtraSprite[acSprite] > 0) SendMessage(GetDlgItem(hwTB2, IDC_EXTRARES), BM_SETCHECK, BST_CHECKED, 0);
    else SendMessage(GetDlgItem(hwTB2, IDC_EXTRARES), BM_SETCHECK, BST_UNCHECKED, 0);
}
void SetExtraResBBox(void)
{
    if (MycRom.name[0] != 0 && acBG < MycRom.nBackgrounds && MycRom.isExtraBackground[acBG] > 0) SendMessage(GetDlgItem(hwTB4, IDC_EXTRARES), BM_SETCHECK, BST_CHECKED, 0);
    else SendMessage(GetDlgItem(hwTB4, IDC_EXTRARES), BM_SETCHECK, BST_UNCHECKED, 0);
}

bool CreateToolbar(void)
{
    if (hwTB)
    {
        DestroyWindow(hwTB);
    }
    if (Edit_Mode == 0)
    {
        hwTB = CreateDialog(hInst, MAKEINTRESOURCE(IDD_ORGDLG), hWnd, Toolbar_Proc);
        if (!hwTB)
        {
            cprintf(true, "Unable to create the comparison toolbar");
            return false;
        }
        ShowWindow(hwTB, TRUE);
        SetIcon(GetDlgItem(hwTB, IDC_COLMODE), IDI_COLMODE);
        //CreateToolTip(IDC_COLMODE, hwTB, (PTSTR)L"Go to colorization mode");
        SetIcon(GetDlgItem(hwTB, IDC_NEW), IDI_NEW);
        SetIcon(GetDlgItem(hwTB, IDC_ADDTXT), IDI_ADDTXT);
        SetIcon(GetDlgItem(hwTB, IDC_OPEN), IDI_OPEN);
        SetIcon(GetDlgItem(hwTB, IDC_SAVE), IDI_SAVE);
        SetIcon(GetDlgItem(hwTB, IDC_UNDO), IDI_UNDO);
        SetIcon(GetDlgItem(hwTB, IDC_REDO), IDI_REDO);
        SetIcon(GetDlgItem(hwTB, IDC_POINTMASK), IDI_DRAWPOINT);
        SetIcon(GetDlgItem(hwTB, IDC_RECTMASK), IDI_DRAWRECT);
        SetIcon(GetDlgItem(hwTB, IDC_ZONEMASK), IDI_MAGICWAND);
        SetIcon(GetDlgItem(hwTB, IDC_CHECKFRAMES), IDI_SEARCH);
        SetIcon(GetDlgItem(hwTB, IDC_ADDSECTION), IDI_ADDTAB);
        SetIcon(GetDlgItem(hwTB, IDC_DELSECTION), IDI_DELTAB);
        SetIcon(GetDlgItem(hwTB, IDC_INVERTSEL), IDI_INVERTSEL);
        SetIcon(GetDlgItem(hwTB, IDC_DELALLSAMEFR), IDI_DELALLSAME);
        SetIcon(GetDlgItem(hwTB, IDC_DELSELSAMEFR), IDI_DELSELSAME);
        SetIcon(GetDlgItem(hwTB, IDC_DELFRAME), IDI_DELFRAME);
        SetIcon(GetDlgItem(hwTB, IDC_INVERTSEL), IDI_INVERTSEL);
        //SetIcon(GetDlgItem(hwTB, IDC_SELSAMEFR), IDI_SELSAMEFR);
        SetIcon(GetDlgItem(hwTB, IDC_DELSAMEFR), IDI_DELSAMEFR);
        SetIcon(GetDlgItem(hwTB, IDC_MOVESECTION), IDI_MOVESECTION);
        SetIcon(GetDlgItem(hwTB, IDC_NIGHTDAY), IDI_NIGHTDAY);
        SetIcon(GetDlgItem(hwTB, IDC_DELDURFRAME), IDI_DELFRMS);
        SendMessage(GetDlgItem(hwTB, IDC_DELFRMS), EM_SETLIMITTEXT, 4, 0);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY2), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY2), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY3), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY3), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY4), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY4), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY12), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY12), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY14), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY14), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY17), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY17), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY18), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY18), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        UpdateMaskList();
        UpdateSectionList();
        UpdateTriggerID();
        if (MycRom.name[0] != 0)
        {
            if (MycRom.ShapeCompMode[acFrame] == TRUE) SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_SETCHECK, BST_CHECKED, 0);
            else SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_SETCHECK, BST_UNCHECKED, 0);
            CheckSameFrames();
        }
        ComboBox_SetItemHeight(GetDlgItem(hwTB, IDC_MASKLIST), 0, 90);
        SetCommonButton(Common_Pushed);
        /*oldMaskListFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_MASKLIST), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_MASKLIST), (SUBCLASSPROC)SubclassMaskListProc, 0, 0);
        oldSectionListoFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_SECTIONLIST), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_SECTIONLIST), (SUBCLASSPROC)SubclassSecListoProc, 1, 0);
        oldMask2ListFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_MASKLIST2), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_MASKLIST2), (SUBCLASSPROC)SubclassMask2ListProc, 4, 0);
        functoremove = 1;*/
    }
    else
    {
        hwTB = CreateDialog(hInst, MAKEINTRESOURCE(IDD_COLDLG), hWnd, Toolbar_Proc);
        if (!hwTB)
        {
            cprintf(true, "Unable to create the colorization toolbar");
            return false;
        }
        ShowWindow(hwTB, TRUE);
        SetIcon(GetDlgItem(hwTB, IDC_ORGMODE), IDI_ORGMODE);
        SetIcon(GetDlgItem(hwTB, IDC_NEW), IDI_NEW);
        SetIcon(GetDlgItem(hwTB, IDC_ADDTXT), IDI_ADDTXT);
        SetIcon(GetDlgItem(hwTB, IDC_OPEN), IDI_OPEN);
        SetIcon(GetDlgItem(hwTB, IDC_SAVE), IDI_SAVE);
        SetIcon(GetDlgItem(hwTB, IDC_UNDO), IDI_UNDO);
        SetIcon(GetDlgItem(hwTB, IDC_REDO), IDI_REDO);
        SetIcon(GetDlgItem(hwTB, IDC_COLSET), IDI_COLSET);
        SetIcon(GetDlgItem(hwTB, IDC_DRAWPOINT), IDI_DRAWPOINT);
        SetIcon(GetDlgItem(hwTB, IDC_DRAWLINE), IDI_DRAWLINE);
        SetIcon(GetDlgItem(hwTB, IDC_DRAWRECT), IDI_DRAWRECT);
        SetIcon(GetDlgItem(hwTB, IDC_DRAWCIRC), IDI_DRAWCERC);
        SetIcon(GetDlgItem(hwTB, IDC_FILL), IDI_MAGICWAND);
        SetIcon(GetDlgItem(hwTB, IDC_ELLIPSE), IDI_ELLIPSE);
        SetIcon(GetDlgItem(hwTB, IDC_COPYCOLS), IDI_4COLSCOPY);
        SetIcon(GetDlgItem(hwTB, IDC_COPYCOLS2), IDI_64COLSCOPY);
        SetIcon(GetDlgItem(hwTB, IDC_DYNACOLSET), IDI_COLSET);
        SetIcon(GetDlgItem(hwTB, IDC_COLPICK), IDI_COLPICK);
        SetIcon(GetDlgItem(hwTB, IDC_COPY), IDI_COPY);
        SetIcon(GetDlgItem(hwTB, IDC_PASTE), IDI_PASTE);
        SetIcon(GetDlgItem(hwTB, IDC_ADDSECTION), IDI_ADDTAB);
        SetIcon(GetDlgItem(hwTB, IDC_DELSECTION), IDI_DELTAB);
        SetIcon(GetDlgItem(hwTB, IDC_COLTODYNA), IDI_COLTODYNA);
        SetIcon(GetDlgItem(hwTB, IDC_INVERTSEL2), IDI_INVERTSEL);
        SetIcon(GetDlgItem(hwTB, IDC_MOVESECTION), IDI_MOVESECTION);
        SetIcon(GetDlgItem(hwTB, IDC_ADDSPRITE2), IDI_ADDSPR);
        SetIcon(GetDlgItem(hwTB, IDC_DELSPRITE2), IDI_DELSPR);
        SetIcon(GetDlgItem(hwTB, IDC_NIGHTDAY), IDI_NIGHTDAY);
        SetIcon(GetDlgItem(hwTB, IDC_DELBG), IDI_DELBG);
        SetIcon(GetDlgItem(hwTB, IDC_ZOOM2X), IDI_ZOOM2X);
        SetIcon(GetDlgItem(hwTB, IDC_ADDBG), IDI_SETBG);
        SendMessage(GetDlgItem(hwTB, IDC_CHANGECOLSET), UDM_SETRANGE, 0, MAKELPARAM(MAX_DYNA_SETS_PER_FRAMEN-1,0));
        SendMessage(GetDlgItem(hwTB, IDC_CHANGECOLSET), UDM_SETPOS, 0, (LPARAM)acDynaSet);
        char tbuf[10];
        _itoa_s(acDynaSet + 1, tbuf, 8, 10);
        SetDlgItemTextA(hwTB, IDC_NOSETCOL, tbuf);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY5), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY5), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY6), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY6), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY7), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY7), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY8), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY8), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY9), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY9), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY10), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY10), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY11), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY11), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY13), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY13), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY15), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY15), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY16), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY16), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY20), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY20), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY21), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY21), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY22), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY22), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetDlgItemTextA(hwTB, IDC_EDIT1, "L\r\nU\r\nM");
        if (BlocPause)
        {
            EnableWindow(GetDlgItem(hwTB, IDC_PASTE), FALSE);
            EnableWindow(GetDlgItem(hwTB, IDC_PASTE2), FALSE);
        }
        else
        {
            EnableWindow(GetDlgItem(hwTB, IDC_PASTE), TRUE);
            EnableWindow(GetDlgItem(hwTB, IDC_PASTE2), TRUE);
        }
        if (MycRom.name[0] != 0)
        {
            if (MycRP.Fill_Mode == TRUE) SendMessage(GetDlgItem(hwTB, IDC_FILLED), BM_SETCHECK, BST_CHECKED, 0); else SendMessage(GetDlgItem(hwTB, IDC_FILLED), BM_SETCHECK, BST_UNCHECKED, 0);
            UpdateFrameSpriteList();
        }
        SetSpotButton(Ident_Pushed);
        SetBackgroundMaskSpotButton(BBIdent_Pushed);
        SetZoomButton(Zoom_Pushed);
        SetExtraResFBox();
        SendMessageA(GetDlgItem(hwTB, IDC_FILTERTYPE), CB_RESETCONTENT, 0, 0);
        SendMessageA(GetDlgItem(hwTB, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Nearest");
        SendMessageA(GetDlgItem(hwTB, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Bilinear");
        SendMessageA(GetDlgItem(hwTB, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Bicubic");
        SendMessageA(GetDlgItem(hwTB, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Lanczos4");
        FrameResizeFilter = 0;
        SendMessageA(GetDlgItem(hwTB, IDC_FILTERTYPE), CB_SETCURSEL, 0, 0);
        /*oldSectionListcFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_SECTIONLIST), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_SECTIONLIST), (SUBCLASSPROC)SubclassSecListcProc, 2, 0);
        oldSpriteList2Func = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_SPRITELIST2), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_SPRITELIST2), (SUBCLASSPROC)SubclassSpriteList2Proc, 3, 0);
        functoremove = 2;*/
    }
    if (UndoAvailableN > 0) EnableWindow(GetDlgItem(hwTB, IDC_UNDO), TRUE); else EnableWindow(GetDlgItem(hwTB, IDC_UNDO), FALSE);
    if (RedoAvailableN > 0) EnableWindow(GetDlgItem(hwTB, IDC_REDO), TRUE); else EnableWindow(GetDlgItem(hwTB, IDC_REDO), FALSE);

    UpdateSectionList();
    SetFocus(hwTB);
    PostMessage(hwTB, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwTB, IDC_NEW), TRUE);
    SetMultiWarningF();
    UpdateURCounts();
    return true;
}

bool CreateToolbar2(void)
{
    if (hwTB2)
    {
        /*RemoveWindowSubclass(hwTB2, (SUBCLASSPROC)SubclassSpriteListProc, 5);
        RemoveWindowSubclass(hwTB2, (SUBCLASSPROC)SubclassSpriteDetListProc, 6);*/
        DestroyWindow(hwTB2);
    }
    hwTB2 = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SPRDLG), hSprites, Toolbar_Proc2);
    if (!hwTB2)
    {
        cprintf(true, "Unable to create the sprite toolbar");
        return false;
    }
    ShowWindow(hwTB2, TRUE);
    SetIcon(GetDlgItem(hwTB2, IDC_SAVE), IDI_SAVE);
    SetIcon(GetDlgItem(hwTB2, IDC_PASTE), IDI_PASTE);
    SetIcon(GetDlgItem(hwTB2, IDC_COPY), IDI_COPY);
    SetIcon(GetDlgItem(hwTB2, IDC_ADDSPRITE), IDI_ADDSPR);
    SetIcon(GetDlgItem(hwTB2, IDC_DELSPRITE), IDI_DELSPR);
    SetIcon(GetDlgItem(hwTB2, IDC_DRAWPOINT), IDI_DRAWPOINT);
    SetIcon(GetDlgItem(hwTB2, IDC_DRAWLINE), IDI_DRAWLINE);
    SetIcon(GetDlgItem(hwTB2, IDC_DRAWRECT), IDI_DRAWRECT);
    SetIcon(GetDlgItem(hwTB2, IDC_DRAWCIRC), IDI_DRAWCERC);
    SetIcon(GetDlgItem(hwTB2, IDC_FILL), IDI_MAGICWAND);
    SetIcon(GetDlgItem(hwTB2, IDC_UNDO), IDI_UNDO);
    SetIcon(GetDlgItem(hwTB2, IDC_REDO), IDI_REDO);
    SetIcon(GetDlgItem(hwTB2, IDC_ADDTOFRAME), IDI_ADDSPRFR);
    SetIcon(GetDlgItem(hwTB2, IDC_DELDETSPR), IDI_DELDETSPR);
    SetIcon(GetDlgItem(hwTB2, IDC_TOFRAME), IDI_SPRTOFRAME);
    SetIcon(GetDlgItem(hwTB2, IDC_ZOOM2X), IDI_ZOOM2X);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY1), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY1), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY2), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY2), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY3), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY3), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY4), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY4), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY12), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY12), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY14), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY14), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY17), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY17), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY18), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY18), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY19), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY19), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    if (UndoAvailableN > 0) EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), TRUE); else EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), FALSE);
    if (RedoAvailableN > 0) EnableWindow(GetDlgItem(hwTB2, IDC_REDO), TRUE); else EnableWindow(GetDlgItem(hwTB2, IDC_REDO), FALSE);
    /*oldSpriteListFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB2, IDC_SPRITELIST), GWLP_WNDPROC);
    SetWindowSubclass(GetDlgItem(hwTB2, IDC_SPRITELIST), (SUBCLASSPROC)SubclassSpriteListProc, 5, 0);
    oldSpriteDetListFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB2, IDC_DETSPR), GWLP_WNDPROC);
    SetWindowSubclass(GetDlgItem(hwTB2, IDC_DETSPR), (SUBCLASSPROC)SubclassSpriteDetListProc, 6, 0);*/
    SetZoomSpriteButton(Zoom_Pushed_Sprite);
    SetExtraResFBox();
    char tbuf[8];
    SendMessage(GetDlgItem(hwTB2, IDC_DETSPR), CB_RESETCONTENT, 0, 0);
    for (int ti = 0; ti < MAX_SPRITE_DETECT_AREAS; ti++)
    {
        _itoa_s(ti + 1, tbuf, 8, 10);
        SendMessageA(GetDlgItem(hwTB2, IDC_DETSPR), CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    SendMessage(GetDlgItem(hwTB2, IDC_DETSPR), CB_SETCURSEL, 0, 1);
    acDetSprite = 0;
    UpdateSpriteList();
    UpdateSpriteList3();
    SetMultiWarningS();
    UpdateURCounts();
    SendMessageA(GetDlgItem(hwTB2, IDC_FILTERTYPE), CB_RESETCONTENT, 0, 0);
    SendMessageA(GetDlgItem(hwTB2, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Nearest");
    SendMessageA(GetDlgItem(hwTB2, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Bilinear");
    SendMessageA(GetDlgItem(hwTB2, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Bicubic");
    SendMessageA(GetDlgItem(hwTB2, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Lanczos4");
    FrameResizeFilter = 0;
    SendMessageA(GetDlgItem(hwTB2, IDC_FILTERTYPE), CB_SETCURSEL, 0, 0);
    return true;
}

bool CreateToolbar3(void)
{
    if (hwTB3)
    {
        /*RemoveWindowSubclass(hwTB2, (SUBCLASSPROC)SubclassSpriteListProc, 5);
        RemoveWindowSubclass(hwTB2, (SUBCLASSPROC)SubclassSpriteDetListProc, 6);*/
        DestroyWindow(hwTB3);
    }
    hwTB3 = CreateDialog(hInst, MAKEINTRESOURCE(IDD_IMGDLG), hImages, Toolbar_Proc3);
    if (!hwTB3)
    {
        cprintf(true, "Unable to create the image toolbar");
        return false;
    }
    ShowWindow(hwTB3, TRUE);
    SetIcon(GetDlgItem(hwTB3, IDC_BROWSEIMAGE), IDI_LOADIMAGE);
    SetIcon(GetDlgItem(hwTB3, IDC_CBPASTE), IDI_CLIPIMAGE);
    //SetIcon(GetDlgItem(hwTB3, IDC_DELIMAGE), IDI_DELIMAGE);
    SetIcon(GetDlgItem(hwTB3, IDC_COPY), IDI_COPY);
    SetIcon(GetDlgItem(hwTB3, IDC_ZOOMIN), IDI_DOWNSCALE);
    SetIcon(GetDlgItem(hwTB3, IDC_ZOOMOUT), IDI_UPSCALE);
    SetWindowLong(GetDlgItem(hwTB3, IDC_STRY4), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB3, IDC_STRY4), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB3, IDC_STRY12), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB3, IDC_STRY12), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB3, IDC_STRY14), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB3, IDC_STRY14), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB3, IDC_STRY17), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB3, IDC_STRY17), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB3, IDC_STRY18), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB3, IDC_STRY18), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    CheckDlgButton(hwTB3, IDC_IMGTOFRAME, BST_CHECKED);
    CheckDlgButton(hwTB3, IDC_IMGTOBG, BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwTB3, IDC_SCROLLCOPY), TRUE);
    SendMessageA(GetDlgItem(hwTB3, IDC_FILTERTYPE), CB_RESETCONTENT, 0, 0);
    SendMessageA(GetDlgItem(hwTB3, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Nearest");
    SendMessageA(GetDlgItem(hwTB3, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Bilinear");
    SendMessageA(GetDlgItem(hwTB3, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Bicubic");
    SendMessageA(GetDlgItem(hwTB3, IDC_FILTERTYPE), CB_ADDSTRING, 0, (LPARAM)"Lanczos4");
    FrameResizeFilter = 0;
    SendMessageA(GetDlgItem(hwTB3, IDC_FILTERTYPE), CB_SETCURSEL, 0, 0);
    //if (UndoAvailable > 0) EnableWindow(GetDlgItem(hwTB3, IDC_UNDO), TRUE); else EnableWindow(GetDlgItem(hwTB3, IDC_UNDO), FALSE);
    //if (RedoAvailable > 0) EnableWindow(GetDlgItem(hwTB3, IDC_REDO), TRUE); else EnableWindow(GetDlgItem(hwTB3, IDC_REDO), FALSE);
    /*char tbuf[8];
    SendMessage(GetDlgItem(hwTB2, IDC_DETSPR), CB_RESETCONTENT, 0, 0);
    for (int ti = 0; ti < MAX_SPRITE_DETECT_AREAS; ti++)
    {
        _itoa_s(ti + 1, tbuf, 8, 10);
        SendMessageA(GetDlgItem(hwTB2, IDC_DETSPR), CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    SendMessage(GetDlgItem(hwTB2, IDC_DETSPR), CB_SETCURSEL, 0, 1);*/
    //acDetSprite = 0;
    //UpdateSpriteList();
    //UpdateSpriteList3();
    return true;
}
bool CreateTextures(void)
{
    glfwMakeContextCurrent(glfwframestrip);
    MonWidth = GetSystemMetrics(SM_CXFULLSCREEN);
//    MonHeight = GetSystemMetrics(SM_CXFULLSCREEN);

    pFrameStrip = (UINT8*)malloc(MonWidth * FRAME_STRIP_HEIGHT * 4);
    if (!pFrameStrip)
    {
        return false;
    }

    glGenTextures(2, TxFrameStrip);

    glBindTexture(GL_TEXTURE_2D, TxFrameStrip[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, TxFrameStrip[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    Bitmap* pbmp = Bitmap::FromFile(L"textures\\chiffres.png");
    if (pbmp == NULL) return false;
    Gdiplus::Rect rect = Gdiplus::Rect(0, 0, pbmp->GetWidth(), pbmp->GetHeight());
    BitmapData bmpdata;
    pbmp->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpdata);
    glGenTextures(1, &TxChiffres);

    glBindTexture(GL_TEXTURE_2D, TxChiffres);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 200, 32, 0, GL_BGRA, GL_UNSIGNED_BYTE, bmpdata.Scan0);
    pbmp->UnlockBits(&bmpdata);
    delete pbmp;

    glfwMakeContextCurrent(glfwBGstrip);
    pBGStrip = (UINT8*)malloc(MonWidth * FRAME_STRIP_HEIGHT * 4);
    if (!pBGStrip)
    {
        free(pFrameStrip);
        pFrameStrip = NULL;
        return false;
    }

    glGenTextures(2, TxBGStrip);
    glBindTexture(GL_TEXTURE_2D, TxBGStrip[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, TxBGStrip[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glfwMakeContextCurrent(glfwspritestrip);

    pSpriteStrip = (UINT8*)malloc(MonWidth * FRAME_STRIP_HEIGHT2 * 4);
    if (!pSpriteStrip)
    {
        free(pFrameStrip);
        pFrameStrip = NULL;
        free(pBGStrip);
        pBGStrip = NULL;
        return false;
    }

    glGenTextures(2, TxSpriteStrip);

    glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT2, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT2, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    glfwMakeContextCurrent(glfwframe);
    pbmp = NULL;
    pbmp = Bitmap::FromFile(L"textures\\cRom.png");
    if (pbmp == NULL) return false;
    rect = Gdiplus::Rect(0, 0, pbmp->GetWidth(), pbmp->GetHeight());
    pbmp->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpdata);
    glGenTextures(1, &TxcRom);

    glBindTexture(GL_TEXTURE_2D, TxcRom);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rect.Width, rect.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, bmpdata.Scan0);
    pbmp->UnlockBits(&bmpdata);
    delete pbmp;

    FILE* pfile;
    if (fopen_s(&pfile, "textures\\chiffres.raw", "rb")) return false;
    fread(Raw_Digit_Def, 1, RAW_DIGIT_W * RAW_DIGIT_H * 11, pfile);
    fclose(pfile);

    glfwMakeContextCurrent(glfwimages);
    glGenTextures(1, &TxSelImage);
    glBindTexture(GL_TEXTURE_2D, TxSelImage);
    pSelImage = (UINT8*)malloc(4 * 256 * IMAGE_ZOOM_TEXMUL * 64 * IMAGE_ZOOM_TEXMUL);
    if (!pSelImage)
    {
        free(pFrameStrip);
        pFrameStrip = NULL;
        free(pSpriteStrip);
        pSpriteStrip = NULL;
        return false;
    }
    memset(pSelImage, 0, 4 * 256 * IMAGE_ZOOM_TEXMUL * 64 * IMAGE_ZOOM_TEXMUL);
    for (int ti = 0; ti < 256 * IMAGE_ZOOM_TEXMUL * 64 * IMAGE_ZOOM_TEXMUL; ti++) pSelImage[ti * 4 + 3] = IMAGE_MASK_OPACITY;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256 * IMAGE_ZOOM_TEXMUL, 64 * IMAGE_ZOOM_TEXMUL, 0, GL_RGBA, GL_UNSIGNED_BYTE, pSelImage);

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}
bool glfwCreateCursorFromFile(char* rawfile,GLFWcursor** pcursor)
{
    unsigned char pdata[32 * 32 * 4];
    FILE* pf;
    if (fopen_s(&pf, rawfile, "rb")) return false;
    rewind(pf);
    fread(pdata, 1, 4 * 32 * 32, pf);
    fclose(pf);
    GLFWimage image;
    image.height = image.width = 32;
    image.pixels = pdata;
    *pcursor = glfwCreateCursor(&image, 0, 0);
    return true;
}
HWND DoCreateStatusBar(HWND hwndParent, HINSTANCE  hinst)
{
    HWND hwndStatus;
    RECT rcClient;
    HLOCAL hloc;
    PINT paParts;
    int nWidth;

    InitCommonControls();

    hwndStatus = CreateWindowEx(
        0,                       // no extended styles
        STATUSCLASSNAME,         // name of status bar class
        (PCTSTR)NULL,           // no text when first created
        WS_CHILD | WS_VISIBLE,   // creates a visible child window
        0, 0, 0, 0,              // ignores size and position
        hwndParent,              // handle to parent window
        NULL,       // child window identifier
        hinst,                   // handle to application instance
        NULL);                   // no window creation data

    GetClientRect(hwndParent, &rcClient);

    hloc = LocalAlloc(LHND, sizeof(int));
    paParts = (PINT)LocalLock(hloc);

    nWidth = rcClient.right;
    int rightEdge = nWidth;
    paParts[0] = rightEdge;
    rightEdge += nWidth;

    SendMessage(hwndStatus, SB_SETPARTS, 0, (LPARAM)paParts);

    LocalUnlock(hloc);
    LocalFree(hloc);
    statusBarHeight = GetSystemMetrics(SM_CYCAPTION) + 10;
    return hwndStatus;
}  

#pragma endregion Window_Creations

#pragma region Main

bool isLeftReleased = false, isRightReleased = false;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    bool mselcolup = true;
    mselcol = 0;

    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

    hInst = hInstance;

    GetModuleFileNameA(NULL, Dir_Serum, MAX_PATH);
    int i = (int)strlen(Dir_Serum) - 1;
    while ((i > 0) && (Dir_Serum[i] != '\\')) i--;
    Dir_Serum[i + 1] = 0;
    strcpy_s(Dir_Dumps, MAX_PATH, Dir_Serum);
    strcpy_s(Dir_GIFs, MAX_PATH, Dir_Serum);
    strcpy_s(Dir_Images, MAX_PATH, Dir_Serum);
    Dir_VP[0] = 0;
    LoadPaths();

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);   
    CoInitialize(NULL);
    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != 0)
    {
        cprintf(true, "Can't initialize GDI+");
        return -1;
    }
    AllocConsole();
    hConsole = GetConsoleWindow();
    HMENU thmen= GetSystemMenu(hConsole, FALSE);
    DeleteMenu(thmen, SC_CLOSE, MF_BYCOMMAND);
    ShowWindow(hConsole, SW_SHOWMINIMIZED);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    cprintf(false, "ColorizingDMD started");
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CROM)); // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COLORIZINGDMD));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;// MAKEINTRESOURCEW(IDC_COLORIZINGDMD);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = NULL; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    if (!RegisterClassEx(&wcex))
    {
        cprintf(true, "Call to RegisterClassEx failed!");
        return 1;
    }
    WNDCLASSEXW wcex2;
    wcex2.cbSize = sizeof(WNDCLASSEX);
    wcex2.style = CS_HREDRAW | CS_VREDRAW;
    wcex2.lpfnWndProc = WndProc2;
    wcex2.cbClsExtra = 0;
    wcex2.cbWndExtra = 0;
    wcex2.hInstance = hInstance;
    wcex2.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CROM)); // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COLORIZINGDMD));
    wcex2.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex2.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex2.lpszMenuName = NULL;// MAKEINTRESOURCEW(IDC_COLORIZINGDMD);
    wcex2.lpszClassName = szWindowClass2;
    wcex2.hIconSm = NULL; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    if (!RegisterClassEx(&wcex2))
    {
        cprintf(true, "Call to RegisterClassEx2 failed!");
        return 1;
    }
    WNDCLASSEXW wcex3;
    wcex3.cbSize = sizeof(WNDCLASSEX);
    wcex3.style = CS_HREDRAW | CS_VREDRAW;
    wcex3.lpfnWndProc = WndProc3;
    wcex3.cbClsExtra = 0;
    wcex3.cbWndExtra = 0;
    wcex3.hInstance = hInstance;
    wcex3.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CROM)); // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COLORIZINGDMD));
    wcex3.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex3.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex3.lpszMenuName = NULL;// MAKEINTRESOURCEW(IDC_COLORIZINGDMD);
    wcex3.lpszClassName = szWindowClass3;
    wcex3.hIconSm = NULL; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    if (!RegisterClassEx(&wcex3))
    {
        cprintf(true, "Call to RegisterClassEx3 failed!");
        return 1;
    }
    hWnd = CreateWindow(szWindowClass, L"ColorizingDMD", WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1480,900, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        AffLastError((char*)"CreateWindow");
        return FALSE;
    }
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    hStatus=DoCreateStatusBar(hWnd, hInst);
    hSprites = CreateWindow(szWindowClass2, L"Sprites", WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1480, 900, nullptr, nullptr, hInstance, nullptr);
    if (!hSprites)
    {
        AffLastError((char*)"CreateWindow2");
        return FALSE;
    }
    ShowWindow(hSprites, SW_SHOW);
    UpdateWindow(hSprites);
    hStatus2 = DoCreateStatusBar(hSprites, hInst);
    hImages = CreateWindow(szWindowClass3, L"Images", WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1480, 900, nullptr, nullptr, hInstance, nullptr);
    if (!hImages)
    {
        AffLastError((char*)"CreateWindow3");
        return FALSE;
    }
    ShowWindow(hImages, SW_SHOW);
    UpdateWindow(hImages);
    hStatus3 = DoCreateStatusBar(hImages, hInst);

    WNDCLASSEX child;
    child.cbSize = sizeof(WNDCLASSEX);
    child.style = 0;
    child.lpfnWndProc = PalProc;
    child.cbClsExtra = 0;
    child.cbWndExtra = 0;
    child.hInstance = hInstance;
    child.hIcon = NULL;
    child.hCursor = NULL;
    child.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    child.lpszMenuName = NULL;
    child.lpszClassName = L"Palette";
    child.hIconSm = NULL;
    if (!RegisterClassEx(&child))
    {
        cprintf(true, "Call to RegisterClassEx for palette failed!");
        return 1;
    }
    child.lpfnWndProc = MovSecProc;
    child.lpszClassName = L"MovSection";
    if (!RegisterClassEx(&child))
    {
        cprintf(true, "Call to RegisterClassEx for moving sections failed!");
        return 1;
    }

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = FrameWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"FrameTester";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        cprintf(true, "Call to RegisterClassEx for tester windows failed!");
        return 1;
    }

    if (!gl33_InitWindow(&glfwframe, 10, 10, "Frame", hWnd)) return -1;
    if (!gl33_InitWindow(&glfwframestrip, 10, 10, "Frame Strip", hWnd)) return -1;
    if (!gl33_InitWindow(&glfwsprites, 10, 10, "Sprite", hSprites)) return -1;
    if (!gl33_InitWindow(&glfwspritestrip, 10, 10, "Sprite Strip", hSprites)) return -1;
    if (!gl33_InitWindow(&glfwimages, 10, 10, "Image", hImages)) return -1;
    glfwSetMouseButtonCallback(glfwframe, mouse_button_callback);
    glfwSetMouseButtonCallback(glfwframestrip, mouse_button_callback);
    glfwSetCursorPosCallback(glfwframe, mouse_move_callback);
    glfwSetCursorPosCallback(glfwframestrip, mouse_move_callback);
    glfwSetScrollCallback(glfwframe, mouse_scroll_callback);
    glfwSetScrollCallback(glfwframestrip, mouse_scroll_callback);
    glfwSetMouseButtonCallback(glfwsprites, mouse_button_callback2);
    glfwSetMouseButtonCallback(glfwspritestrip, mouse_button_callback2);
    glfwSetCursorPosCallback(glfwsprites, mouse_move_callback2);
    glfwSetCursorPosCallback(glfwspritestrip, mouse_move_callback2);
    glfwSetScrollCallback(glfwsprites, mouse_scroll_callback2);
    glfwSetScrollCallback(glfwspritestrip, mouse_scroll_callback2);
    glfwSetMouseButtonCallback(glfwimages, mouse_button_callback3);
    glfwSetCursorPosCallback(glfwimages, mouse_move_callback3);
    glfwSetScrollCallback(glfwimages, mouse_scroll_callback3);

    if (!CreateToolbar()) return -1;
    if (!CreateToolbar2()) return -1;
    if (!CreateToolbar3()) return -1;

    if (!bg_CreateWindow()) return -1;

    if (!Load_GIFski_DLL())
    {
        return -1;
    }

    TxCircleFr = text_CreateTextureCircle(glfwframe);
    TxCircleSpr = text_CreateTextureCircle(glfwsprites);
    TxCircleBG = text_CreateTextureCircle(glfwBG);
    glfwMakeContextCurrent(glfwframe);
    MSG msg;

    // allocate space for undo/redo
    GetTempPathA(MAX_PATH, temporaryDir);
    
    UndoSaveN = (UINT8*)malloc(UNDO_REDO_BUFFER_SIZE);
    RedoSaveN = (UINT8*)malloc(UNDO_REDO_BUFFER_SIZE);
    if ((!UndoSaveN) || (!RedoSaveN))
    {
        cprintf(true, "Can't get the memory for undo/redo actions");
        FreeLibrary(hGIFDLL);
        return -1;
    }
    for (int ti = 0; ti < 256; ti++)
    {
        RedoUsedNumber[ti] = UndoUsedNumber[ti] = false;
    }

    hcArrow = LoadCursor(nullptr, IDC_ARROW);
    hcColPick = LoadCursor(hInst, MAKEINTRESOURCE(IDC_NODROP));
    hcPaste = LoadCursor(hInst, MAKEINTRESOURCE(IDC_PASTE));
    glfwCreateCursorFromFile((char*)"paste.raw", &glfwpastecur);
    glfwCreateCursorFromFile((char*)"NODROP.raw", &glfwdropcur);
    glfwarrowcur = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    
    if (!CreateTextures())
    {
        FreeLibrary(hGIFDLL);
        return -1;
    }
    Calc_Resize_Frame();
    Calc_Resize_Sprite();
    Calc_Resize_Image();
    Calc_Resize_BG();
    UpdateFSneeded = true;
    timeSelFrame = timeGetTime() + 500;
    DWORD tickCount = GetTickCount();
    build_crc32_table();
    LoadVars();

    LoadWindowPosition();

    UpdateURCounts();

    while (!fDone)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        if (MycRom.name[0] != 0)
        {
            DWORD acCount = GetTickCount();
            if ((acCount < tickCount) || (acCount - tickCount > AUTOSAVE_TICKS))
            {
                if (Save_cRom(true, false, (char*)""))
                {
                    cprintf(false, "%s.cROM autosaved in %s", MycRom.name, Dir_Serum);
                    if (Save_cRP(true)) cprintf(false, "%s.cRP autosaved in %s", MycRom.name, Dir_Serum);
                }
                tickCount = acCount;
            }
        }
        if (Update_Toolbar)
        {
            CreateToolbar();
            UpdateFrameBG();
            Update_Toolbar = false;
        }
        if (Update_Toolbar2)
        {
            CreateToolbar2();
            Update_Toolbar2 = false;
        }
        if (Update_Toolbar4)
        {
            CreateToolbar4();
            Update_Toolbar4 = false;
        }
        if (mselcolup)
        {
            mselcol+=3;
            if (mselcol > 250) mselcolup = false;
        }
        else
        {
            mselcol -= 3;
            if (mselcol < 5) mselcolup = true;
        }
        if (!IsIconic(hWnd))
        {
            if ((MycRom.name[0] != 0) && (timeGetTime() > timeSelFrame))
            {
                timeSelFrame = timeGetTime() + 500;
                if (SelFrameColor == 0) SelFrameColor = 255; else SelFrameColor = 0;
                if (hPal) InvalidateRect(hPal, NULL, FALSE);
                if (hPal2) InvalidateRect(hPal2, NULL, FALSE);
                if (hPal3) InvalidateRect(hPal3, NULL, FALSE);
                if ((Edit_Mode == 1) && (hwTB)) InvalidateRect(GetDlgItem(hwTB, IDC_GRADMODEB), NULL, FALSE);
            }
            if (!(GetKeyState(VK_LEFT) & 0x8000)) isLeftReleased = true;
            if (!(GetKeyState(VK_RIGHT) & 0x8000)) isRightReleased = true;
            if (GetForegroundWindow() == hWnd)
            {
                if (isPressed(VK_LEFT, &timeLPress))
                {
                    if ((GetKeyState(VK_CONTROL) & 0x8000) && (nSameFrames > 0))
                    {
                        int acsame = -1;
                        for (UINT ti = 0; ti < (UINT)nSameFrames; ti++)
                        {
                            if ((SameFrames[ti] < PreFrameInStrip) && (SameFrames[ti] > acsame)) acsame = SameFrames[ti];
                        }
                        if (acsame >= 0)
                        {
                            PreFrameInStrip = acsame;
                            UpdateFSneeded = true;
                        }
                    }
                    else if ((GetKeyState(VK_SHIFT) & 0x8000) && (nSelFrames > 1))
                    {
                        int acsel = -1;
                        for (UINT ti = 0; ti < nSelFrames; ti++)
                        {
                            if ((SelFrames[ti] < (UINT)PreFrameInStrip) && ((int)SelFrames[ti] > acsel)) acsel = (int)SelFrames[ti];
                        }
                        if (acsel >= 0)
                        {
                            PreFrameInStrip = acsel;
                            UpdateFSneeded = true;
                        }
                    }
                    else if ((!(GetKeyState(VK_CONTROL) & 0x8000)) && (!(GetKeyState(VK_SHIFT) & 0x8000)) && (acFrame > 0))
                    {
                        if (isLeftReleased)
                        {
                            SaveAction(true, SA_SELECTION);
                            isLeftReleased = false;
                        }
                        acFrame--;
                        InitColorRotation();
                        if (!isFrameSelected2(acFrame))
                        {
                            nSelFrames = 1;
                            SetMultiWarningF();
                            SelFrames[0] = acFrame;
                        }
                        if (acFrame >= PreFrameInStrip + NFrameToDraw) PreFrameInStrip = acFrame - NFrameToDraw + 1;
                        if ((int)acFrame < PreFrameInStrip) PreFrameInStrip = acFrame;
                        UpdateFSneeded = true;
                        UpdateNewacFrame();
                    }
                    Check_Commons();
                    AllSameFramesUpdated = false;
                    //SetDlgItemTextA(hwTB, IDC_LISTSAMEFR, "Best Mask ???");
                    if (Edit_Mode == 0) InvalidateRect(GetDlgItem(hwTB, IDC_MASKLIST), NULL, FALSE);
                }
                if (isPressed(VK_RIGHT, &timeRPress))
                {
                    if ((GetKeyState(VK_CONTROL) & 0x8000) && (nSameFrames > 0))
                    {
                        int acsame = 2000000000;
                        for (UINT ti = 0; ti < (UINT)nSameFrames; ti++)
                        {
                            if ((SameFrames[ti] > PreFrameInStrip) && (SameFrames[ti] < acsame)) acsame = SameFrames[ti];
                        }
                        if (acsame < 2000000000)
                        {
                            PreFrameInStrip = acsame;
                            UpdateFSneeded = true;
                        }
                    }
                    else if ((GetKeyState(VK_SHIFT) & 0x8000) && (nSelFrames > 1))
                    {
                        int acsel = 2000000000;
                        for (UINT ti = 0; ti < nSelFrames; ti++)
                        {
                            if ((SelFrames[ti] > (UINT)PreFrameInStrip) && ((int)SelFrames[ti] < acsel)) acsel = (int)SelFrames[ti];
                        }
                        if (acsel < 2000000000)
                        {
                            PreFrameInStrip = acsel;
                            UpdateFSneeded = true;
                        }
                    }
                    else if ((!(GetKeyState(VK_CONTROL) & 0x8000)) && (!(GetKeyState(VK_SHIFT) & 0x8000)) && (acFrame < MycRom.nFrames - 1))
                    {
                        if (isRightReleased)
                        {
                            SaveAction(true, SA_SELECTION);
                            isRightReleased = false;
                        }
                        acFrame++;
                        InitColorRotation();
                        if (!isFrameSelected2(acFrame))
                        {
                            nSelFrames = 1;
                            SetMultiWarningF();
                            SelFrames[0] = acFrame;
                        }
                        if (acFrame >= PreFrameInStrip + NFrameToDraw) PreFrameInStrip = acFrame - NFrameToDraw + 1;
                        if ((int)acFrame < PreFrameInStrip) PreFrameInStrip = acFrame;
                        UpdateFSneeded = true;
                        UpdateNewacFrame();
                    }
                    Check_Commons();
                    AllSameFramesUpdated = false;
                    //SetDlgItemTextA(hwTB, IDC_LISTSAMEFR, "Best Mask ???");
                    if (Edit_Mode == 0) InvalidateRect(GetDlgItem(hwTB, IDC_MASKLIST), NULL, FALSE);
                }
            }
            else if (GetForegroundWindow() == hImages)
            {
                int movestep = 1;
                if (GetKeyState(VK_SHIFT) & 0x8000) movestep = 10;
                if (isPressed(VK_LEFT, &timeLPress))
                {
                    if (crop_offsetx >= movestep) crop_offsetx -= movestep; else crop_offsetx = 0;
                }
                if (isPressed(VK_RIGHT, &timeRPress))
                {
                    if (crop_offsetx <= image_sizeW - crop_sizeW - movestep) crop_offsetx += movestep; else crop_offsetx = image_sizeW - crop_sizeW;
                }
                if (isPressed(VK_UP, &timeUPress))
                {
                    if (crop_offsety >= movestep) crop_offsety -= movestep; else crop_offsety = 0;
                }
                if (isPressed(VK_DOWN, &timeDPress))
                {
                    if (crop_offsety <= image_sizeH - crop_sizeH - movestep) crop_offsety += movestep; else crop_offsety = image_sizeH - crop_sizeH;
                }
            }
            else if (GetForegroundWindow() == hBG)
            {
                if (isPressed(VK_LEFT, &timeLPress))
                {
                    if (acBG > 0)
                    {
                        SaveAction(true, SA_SELBACKGROUND);
                        if (isLeftReleased) isLeftReleased = false;
                        acBG--;
                        if (acBG >= PreBGInStrip + NBGToDraw) PreBGInStrip = acBG - NBGToDraw + 1;
                        if ((int)acBG < PreBGInStrip) PreBGInStrip = acBG;
                        if (MycRom.isExtraBackground[acBG] > 0) CheckDlgButton(hwTB4, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
                        InitColorRotation2();
                        UpdateBackgroundList();
                        UpdateBSneeded = true;
                    }
                }
                if (isPressed(VK_RIGHT, &timeRPress))
                {
                    if ((UINT16)acBG < MycRom.nBackgrounds - 1)
                    {
                        SaveAction(true, SA_SELBACKGROUND);
                        if (isRightReleased) isRightReleased = false;
                        acBG++;
                        if (acBG >= PreBGInStrip + NBGToDraw) PreBGInStrip = acBG - NBGToDraw + 1;
                        if ((int)acBG < PreBGInStrip) PreBGInStrip = acBG;
                        if (MycRom.isExtraBackground[acBG] > 0) CheckDlgButton(hwTB4, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB4, IDC_EXTRARES, BST_UNCHECKED);
                        InitColorRotation2();
                        UpdateBackgroundList();
                        UpdateBSneeded = true;
                    }
                }
                if (ExtraResBClicked && MycRom.isExtraBackground[acBG] > 0) nEditExtraResolutionB = true; else nEditExtraResolutionB = false;
            }

            if (MycRom.name[0] != 0)
            {
                if (Edit_Mode == 1)
                {
                    if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                        Draw_Frame(glfwframe, 2 * frame_zoom, offset_frame_x, offset_frame_y, acFrame, 0, 0, false);
                    else Draw_Frame(glfwframe, frame_zoom, 0, 0, acFrame, 0, 0, false);
                }
                else Draw_Frame(glfwframe, frame_zoom, 0, 0, acFrame, 0, 0, true);
            }
            if ((MycRP.name[0] != 0) && (GetForegroundWindow() == hWnd))
            {
                EmptyExtraSurface();
                glfwMakeContextCurrent(glfwframe);
                UINT fw, fh;
                UINT16* pfr;
                UINT8* pdynm;
                if (nEditExtraResolutionF)
                {
                    fw = MycRom.fWidthX;
                    fh = MycRom.fHeightX;
                    pfr = MycRom.cFramesX;
                    pdynm = MycRom.DynaMasksX;
                }
                else
                {
                    fw = MycRom.fWidth;
                    fh = MycRom.fHeight;
                    pfr = MycRom.cFrames;
                    pdynm = MycRom.DynaMasks;
                }
                switch (Mouse_Mode)
                {
                    case 0:
                    {
                        if (Edit_Mode == 0)
                        {
                            if ((UINT)MouseFinPosx>=0 && (UINT)MouseFinPosy>=0 && (UINT)MouseFinPosx<MycRom.fWidth && (UINT)MouseFinPosy<MycRom.fHeight) Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * MycRom.fWidth] = 1;
                            SetRenderDrawColor(mselcol, mselcol, mselcol, 255);
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, frame_zoom, 0, 0, true, true);
                        }
                        break;
                    }
                    case 1:
                    {
                        Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * MycRom.fWidth] = 1;
                        SetRenderDrawColor(mselcol, mselcol, mselcol, 255);
                        Draw_Over_From_Surface(Draw_Extra_Surface, 0, frame_zoom, 0,0,true, true);
                        if (AllSameFramesUpdated)
                        {
                            AllSameFramesUpdated = false;
                            //SetDlgItemTextA(hwTB, IDC_LISTSAMEFR, "Best Mask ???");
                            if (Edit_Mode == 0) InvalidateRect(GetDlgItem(hwTB, IDC_MASKLIST), NULL, FALSE);
                        }
                        break;
                    }
                    case 2:
                    {
                        switch (MycRP.Mask_Sel_Mode)
                        {
                            case 1:
                                drawrectangle(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, TRUE, false, NULL);
                                break;
                            case 2:
                                drawfill2(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1);
                                break;
                        }
                        SetRenderDrawColor(mselcol, mselcol, mselcol, 255);
                        //if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                        //    Draw_Over_From_Surface(Draw_Extra_Surface, 0, 2 * frame_zoom, offset_frame_x, offset_frame_y, true, true);
                        //else
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, frame_zoom, 0, 0, true, true);
                        if (AllSameFramesUpdated)
                        {
                            AllSameFramesUpdated = false;
                            //SetDlgItemTextA(hwTB, IDC_LISTSAMEFR, "Best Mask ???");
                            if (Edit_Mode == 0) InvalidateRect(GetDlgItem(hwTB, IDC_MASKLIST), NULL, FALSE);
                        }
                        break;
                    }
                    case 3:
                    {
                        if (!Copy_Mode)
                        {
                            Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * fw] = 1;
                            //SetRenderDrawColor(mselcol, mselcol, mselcol, 255);
                            if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                                Draw_Over_From_Surface(Draw_Extra_Surface, 0, 2 * frame_zoom, offset_frame_x, offset_frame_y, true, true);
                            else
                                Draw_Over_From_Surface(Draw_Extra_Surface, 0, frame_zoom, 0, 0, true, true);
                        }
                        break;
                    }
                    case 4:
                    {
                        switch (MycRP.Draw_Mode)
                        {
                            case 1:
                            {
                                drawline(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, false, NULL);
                                break;
                            }
                            case 2:
                            {
                                drawrectangle(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                            case 3:
                            {
                                drawcircle(MouseIniPosx, MouseIniPosy, (int)sqrt((MouseFinPosx - MouseIniPosx)* (MouseFinPosx - MouseIniPosx) + (MouseFinPosy - MouseIniPosy) * (MouseFinPosy - MouseIniPosy)), Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                            case 4:
                            {
                                drawfill(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1);
                                break;
                            }
                            case 5:
                            {
                                drawellipse(MouseIniPosx, MouseIniPosy, (int)abs(MouseFinPosx - MouseIniPosx), (int)abs(MouseFinPosy - MouseIniPosy), Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                        }
                        if (!Copy_Mode) SetRenderDrawColor(mselcol, mselcol, mselcol, mselcol);
                        else SetRenderDrawColor(mselcol, 0, 0, mselcol);
                        if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, 2 * frame_zoom, offset_frame_x, offset_frame_y, true, true);
                        else
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, frame_zoom, 0, 0, true, true);
                        break;
                    }
                    case 5:
                    {
                        Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * fw] = 1;
                        SetRenderDrawColor(mselcol, 0, mselcol, 255);
                        if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, 2 * frame_zoom, offset_frame_x, offset_frame_y, true, true);
                        else
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, frame_zoom, 0, 0, true, true);
                        break;
                    }
                    case 6:
                    {
                        switch (MycRP.Draw_Mode)
                        {
                            case 1:
                            {
                                drawline(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, false, NULL);
                                break;
                            }
                            case 2:
                            {
                                drawrectangle(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                            case 3:
                            {
                                drawcircle(MouseIniPosx, MouseIniPosy, (int)sqrt((MouseFinPosx - MouseIniPosx) * (MouseFinPosx - MouseIniPosx) + (MouseFinPosy - MouseIniPosy) * (MouseFinPosy - MouseIniPosy)), Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                            case 4:
                            {
                                drawfill(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1);
                                break;
                            }
                            case 5:
                            {
                                drawellipse(MouseIniPosx, MouseIniPosy, (int)abs(MouseFinPosx - MouseIniPosx), (int)abs(MouseFinPosy - MouseIniPosy), Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                        }
                        SetRenderDrawColor(mselcol, 0, mselcol, 255);
                        if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, 2 * frame_zoom, offset_frame_x, offset_frame_y, true, true);
                        else
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, frame_zoom, 0, 0, true, true);
                        break;
                    }
                    case 7:
                    {
                        if (!(GetKeyState('W') & 0x8000) || (pfr[(acFrame * fh + MouseFinPosy) * fw + MouseFinPosx] != 0))
                            Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * fw] = 1;
                        SetRenderDrawColor(0, mselcol, mselcol, 255);
                        if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, 2 * frame_zoom, offset_frame_x, offset_frame_y, true, true);
                        else
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, frame_zoom, 0, 0, true, true);
                        break;
                    }
                    case 8:
                    {
                        bool shapeselect = false;
                        if (GetKeyState('W') & 0x8000) shapeselect = true;
                        switch (MycRP.Draw_Mode)
                        {
                            case 1:
                            {
                                drawline(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, shapeselect, &pfr[acFrame * fw * fh]);
                                break;
                            }
                            case 2:
                            {
                                drawrectangle(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, MycRP.Fill_Mode, shapeselect, &pfr[acFrame * fw * fh]);
                                break;
                            }
                            case 3:
                            {
                                drawcircle(MouseIniPosx, MouseIniPosy, (int)sqrt((MouseFinPosx - MouseIniPosx)* (MouseFinPosx - MouseIniPosx) + (MouseFinPosy - MouseIniPosy) * (MouseFinPosy - MouseIniPosy)), Draw_Extra_Surface, 1, MycRP.Fill_Mode, shapeselect, &pfr[acFrame * fw * fh]);
                                break;
                            }
                            case 4:
                            {
                                drawfill(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1);
                                break;
                            }
                            case 5:
                            {
                                drawellipse(MouseIniPosx, MouseIniPosy, (int)abs(MouseFinPosx - MouseIniPosx), (int)abs(MouseFinPosy - MouseIniPosy), Draw_Extra_Surface, 1, MycRP.Fill_Mode, shapeselect, &pfr[acFrame * fw * fh]);
                                break;
                            }
                        }
                        SetRenderDrawColor(0, mselcol, mselcol, 255);
                        if (Zoom_Pushed && ((nEditExtraResolutionF && MycRom.fHeightX == 64) || (!nEditExtraResolutionF && MycRom.fHeight == 64)))
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, 2 * frame_zoom, offset_frame_x, offset_frame_y, true, true);
                        else
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, frame_zoom, 0, 0, true, true);
                        break;
                    }
                }
            }
            if (GetKeyState(VK_ESCAPE) & 0x8000)
            {
                Mouse_Mode = 0;
                Paste_Mode = false;
                Color_Pipette = 0;
                glfwSetCursor(glfwframe, glfwarrowcur);
                SetCursor(hcArrow);
            }
            gl33_SwapBuffers(glfwframe, false);
            if (MycRom.name[0] != 0)
            {
                if (UpdateFSneeded)
                {
                    Frame_Strip_Update();
                    UpdateTriggerID();
                    UpdateFSneeded = false;
                }
                Draw_Frame_Strip();
            }
            if (MouseFrSliderLPressed)
            {
                double x, y;
                glfwGetCursorPos(glfwframestrip, &x, &y);
                int px = (int)x - FRAME_STRIP_SLIDER_MARGIN;
                if (px != MouseFrSliderlx)
                {
                    MouseFrSliderlx = px;
                    if (px < 0) px = 0;
                    if (px > (int)ScrW - 2 * FRAME_STRIP_SLIDER_MARGIN) px = (int)ScrW - 2 * FRAME_STRIP_SLIDER_MARGIN;
                    PreFrameInStrip = (int)((float)px * (float)MycRom.nFrames / (float)SliderWidth);
                    if (PreFrameInStrip < 0) PreFrameInStrip = 0;
                    if (PreFrameInStrip >= (int)MycRom.nFrames) PreFrameInStrip = (int)MycRom.nFrames - 1;
                    UpdateFSneeded = true;
                }
            }
            float fps = 0;
            fps = gl33_SwapBuffers(glfwframestrip, true);
            char tbuf[256];
            POINT tpt;
            GetCursorPos(&tpt);
            if (((Mouse_Mode == 2) && (MycRP.Mask_Sel_Mode == 1)) || ((Mouse_Mode == 4) && (MycRP.Draw_Mode == 2)) ||
                ((Mouse_Mode == 6) && (MycRP.Draw_Mode == 2)) || ((Mouse_Mode == 8) && (MycRP.Draw_Mode == 2)))
                sprintf_s(tbuf, 256, "ColorizingDMD v%i.%i.%i (by Zedrummer)     ROM name: %s      Frames: current %i / %i selected / %i total      Pos: (%i,%i)->(%i,%i)      @%.1fFPS", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, MycRom.name, acFrame, nSelFrames, MycRom.nFrames, MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, fps);
            else
                sprintf_s(tbuf, 256, "ColorizingDMD v%i.%i.%i (by Zedrummer)     ROM name: %s      Frames: current %i / %i selected / %i total      Pos: (%i,%i)      @%.1fFPS", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, MycRom.name, acFrame, nSelFrames, MycRom.nFrames, MouseFinPosx, MouseFinPosy, fps);
            SetWindowTextA(hWnd, tbuf);
            glfwPollEvents();
        }
        if (!IsIconic(hSprites))
        {
            if (Zoom_Pushed_Sprite)// && ((nEditExtraResolutionS && MycRom.fHeightX == 64) || (!nEditExtraResolutionS && MycRom.fHeight == 64)))
                Draw_Sprite(2 * sprite_zoom, offset_sprite_x, offset_sprite_y);
            else Draw_Sprite(sprite_zoom, 0, 0);
            if (GetForegroundWindow() == hSprites)
            {
                if ((MycRom.name[0] != 0) && (MycRom.nSprites > 0))
                {
                    if (isPressed(VK_LEFT, &timeLPress))
                    {
                        SaveAction(true, SA_ACSPRITE);
                        if (acSprite > 0) acSprite--;
                        if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                        if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                        if (MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
                        nSelSprites = 1;
                        SelSprites[0] = acSprite;
                        if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
                        UpdateSSneeded = true;
                        SendMessage(GetDlgItem(hwTB2, IDC_SPRITELIST), CB_SETCURSEL, (WPARAM)acSprite, 0);
                        SetDlgItemTextA(hwTB2, IDC_SPRITENAME, &MycRP.Sprite_Names[acSprite * SIZE_SECTION_NAMES]);
                        if (ExtraResSClicked && MycRom.isExtraSprite[acSprite] > 0) nEditExtraResolutionS = true; else nEditExtraResolutionS = false;
                    }
                    if (isPressed(VK_RIGHT, &timeRPress))
                    {
                        SaveAction(true, SA_ACSPRITE);
                        if (acSprite < MycRom.nSprites - 1) acSprite++;
                        if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                        if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                        if (MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
                        nSelSprites = 1;
                        SelSprites[0] = acSprite;
                        if (MycRom.isExtraSprite && MycRom.isExtraSprite[acSprite] > 0) CheckDlgButton(hwTB2, IDC_EXTRARES, BST_CHECKED); else CheckDlgButton(hwTB2, IDC_EXTRARES, BST_UNCHECKED);
                        UpdateSSneeded = true;
                        SendMessage(GetDlgItem(hwTB2, IDC_SPRITELIST), CB_SETCURSEL, (WPARAM)acSprite, 0);
                        SetDlgItemTextA(hwTB2, IDC_SPRITENAME, &MycRP.Sprite_Names[acSprite * SIZE_SECTION_NAMES]);
                        if (ExtraResSClicked && MycRom.isExtraSprite[acSprite] > 0) nEditExtraResolutionS = true; else nEditExtraResolutionS = false;
                    }
                    SetMultiWarningS();
                    glfwMakeContextCurrent(glfwsprites);
                    EmptyExtraSurface2();
                    if (Mouse_Mode == 4)
                    {
                        switch (Sprite_Mode)
                        {
                            case 1:
                            {
                                drawline2(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface2, 1);
                                break;
                            }
                            case 2:
                            {
                                drawrectangle2(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface2, 1, SpriteFill_Mode);
                                break;
                            }
                            case 3:
                            {
                                drawcircle2(MouseIniPosx, MouseIniPosy, (int)sqrt((MouseFinPosx - MouseIniPosx) * (MouseFinPosx - MouseIniPosx) + (MouseFinPosy - MouseIniPosy) * (MouseFinPosy - MouseIniPosy)), Draw_Extra_Surface2, 1, SpriteFill_Mode);
                                break;
                            }
                            case 4:
                            {
                                drawfill3(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface2, 1);
                                break;
                            }
                        }
                        SetRenderDrawColor(mselcol, mselcol, mselcol, mselcol);
                        if (Zoom_Pushed_Sprite)// && ((nEditExtraResolutionS && MycRom.fHeightX == 64) || (!nEditExtraResolutionS && MycRom.fHeight == 64)))
                            Draw_Over_From_Surface2(Draw_Extra_Surface2, 0, (float)2 * sprite_zoom, offset_sprite_x, offset_sprite_y, true, true);
                        else
                            Draw_Over_From_Surface2(Draw_Extra_Surface2, 0, (float)sprite_zoom, 0, 0, true, true);
                    }
                    else if (Mouse_Mode == 2)
                    {
                        drawrectangle2(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface2, 1, TRUE);
                        SetRenderDrawColor(mselcol, 0, mselcol, mselcol);
                        if (Zoom_Pushed_Sprite)// && ((nEditExtraResolutionS && MycRom.fHeightX == 64) || (!nEditExtraResolutionS && MycRom.fHeight == 64)))
                            Draw_Over_From_Surface2(Draw_Extra_Surface2, 0, (float)2 * sprite_zoom, offset_sprite_x, offset_sprite_y, true, true);
                        else
                            Draw_Over_From_Surface2(Draw_Extra_Surface2, 0, (float)sprite_zoom, 0, 0, true, true);
                    }
                }
            }
            if (UpdateSSneeded)
            {
                Sprite_Strip_Update();
                UpdateSSneeded = false;
            }
            Draw_Sprite_Strip();
            gl33_SwapBuffers(glfwsprites, true);
            gl33_SwapBuffers(glfwspritestrip, true);
        }
        if (!IsIconic(hImages))
        {
            char tbuf[256];
            sprintf_s(tbuf, 256, "Images          Selection size: (%i,%i)          Center of selection: (%i,%i)", crop_sizeW, crop_sizeH, crop_offsetx + crop_sizeW / 2, crop_offsety + crop_sizeH / 2);
            SetWindowTextA(hImages, tbuf);
            Draw_Image();
            gl33_SwapBuffers(glfwimages, true);
        }
        if (!IsIconic(hBG))
        {
            if (MycRom.name[0] != 0)
            {
                if (UpdateBSneeded)
                {
                    Background_Strip_Update();
                    UpdateBSneeded = false;
                }
                Draw_BG_Strip();
                Draw_Background(glfwBG, BG_zoom, acBG, 0, 0, 0, 0);
            }
            gl33_SwapBuffers(glfwBG, true);
            gl33_SwapBuffers(glfwBGstrip, true);
            if (MouseBGSliderLPressed)
            {
                double x, y;
                glfwGetCursorPos(glfwBGstrip, &x, &y);
                int px = (int)x - FRAME_STRIP_SLIDER_MARGIN;
                if (px != MouseBGSliderlx)
                {
                    MouseBGSliderlx = px;
                    if (px < 0) px = 0;
                    if (px > (int)ScrW4 - 2 * FRAME_STRIP_SLIDER_MARGIN) px = (int)ScrW4 - 2 * FRAME_STRIP_SLIDER_MARGIN;
                    PreBGInStrip = (int)((float)px * (float)MycRom.nBackgrounds / (float)SliderWidth4);
                    if (PreBGInStrip < 0) PreFrameInStrip = 0;
                    if (PreBGInStrip >= (int)MycRom.nBackgrounds) PreBGInStrip = (int)MycRom.nBackgrounds - 1;
                    UpdateBSneeded = true;
                }
            }
        }
        CheckAccelerators();
    }
    FreeLibrary(hGIFDLL);
    DeleteObject(hInactiveBrush);
    DeleteObject(hActiveBrush);
    SaveVars();
    SavePaths();
    free(RedoSaveN);
    free(UndoSaveN);
    if (pSelImage) free(pSelImage);
    if (pFrameStrip) free(pFrameStrip);
    CoUninitialize();
    GdiplusShutdown(gdiplusToken);
    glfwTerminate();
    cprintf(false, "ColorizingDMD terminated");
    return 0;
}

#pragma endregion Main
