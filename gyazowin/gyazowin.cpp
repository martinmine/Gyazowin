// gyazowin.cpp : Application entry point
//

#include "stdafx.h"
#include "gyazowin.h"

// Global variables
HINSTANCE hInst;							// Current window
TCHAR *szTitle			= _T("Gyazo");		// Text in the window title
TCHAR *szWindowClass	= _T("GYAZOWIN");	// Main window class name
TCHAR *szWindowClassL	= _T("GYAZOWINL");	// Layer window class name
HWND hLayerWnd;

int ofX, ofY;	// Screen offset

// Prototype declaration
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	LayerWndProc(HWND, UINT, WPARAM, LPARAM);

int					GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

BOOL				isPng(LPCTSTR fileName);
VOID				drawRubberband(HDC hdc, LPRECT newRect, BOOL erase);
VOID				execUrl(const char* str);
VOID				setClipBoardText(const char* str);
BOOL				convertPNG(LPCTSTR destFile, LPCTSTR srcFile);
BOOL				savePNG(LPCTSTR fileName, HBITMAP newBMP);
BOOL				uploadFile(HWND hwnd, LPCTSTR fileName);
std::string			getId();
BOOL				saveId(const WCHAR* str);

// Main
int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;

	TCHAR	szThisPath[MAX_PATH];
	DWORD   sLen;

	// Find the path of this program
	sLen = GetModuleFileName(NULL, szThisPath, MAX_PATH);
	for(unsigned int i = sLen; i >= 0; i--)
	{
		if(szThisPath[i] == _T('\\'))
		{
			szThisPath[i] = _T('\0');
			break;
		}
	}

	// Set path of where the program is currently running
	SetCurrentDirectory(szThisPath);

	// File specified when the application was launched
	if(2 == __argc)
	{
		// If image is PNG, upload it
		if(isPng(__targv[1]))
		{
			// Upload the PNG file
			uploadFile(NULL, __targv[1]);
		}
		else
		{
			// Attempt to convert the PNG file
			TCHAR tmpDir[MAX_PATH], tmpFile[MAX_PATH];
			GetTempPath(MAX_PATH, tmpDir);
			GetTempFileName(tmpDir, _T("gya"), 0, tmpFile);

			if(convertPNG(tmpFile, __targv[1]))
			{
				// If the PNG convertion succeeded, upload it
				uploadFile(NULL, tmpFile);
			}
			else
			{
				// PNG conversion failed, notify the user
				MessageBox(NULL, _T("Cannot convert this image"), szTitle,
				           MB_OK | MB_ICONERROR);
			}
			DeleteFile(tmpFile);
		}
		return TRUE;
	}

	// Register the window class
	MyRegisterClass(hInstance);

	// Attempt to initialize the application
	if(!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Main program loop
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

// Validates if the file on the specified path is a valid PNG file
BOOL isPng(LPCTSTR fileName)
{
	unsigned char pngHead[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	unsigned char readHead[8];

	FILE *fp = NULL;

	if(0 != _tfopen_s(&fp, fileName, _T("rb")) || fread(readHead, 1, 8, fp) != 8)
	{
		// Invalid PNG file
		return FALSE;
	}
	fclose(fp);

	// compare
	for(unsigned int i = 0; i < 8; i++)
		if(pngHead[i] != readHead[i]) return FALSE;

	return TRUE;

}

// Registering the window instance
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASS wc;

	// ƒƒCƒ“ƒEƒBƒ“ƒhƒE
	wc.style         = 0;							// Do not set WM_PAINT
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GYAZOWIN));
	wc.hCursor       = LoadCursor(NULL, IDC_CROSS);	// Set cursor properties
	wc.hbrBackground = 0;							// Make background 0
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	RegisterClass(&wc);

	// ƒŒƒCƒ„[ƒEƒBƒ“ƒhƒE
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = LayerWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GYAZOWIN));
	wc.hCursor       = LoadCursor(NULL, IDC_CROSS);	// + ‚ÌƒJ[ƒ\ƒ‹
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClassL;

	return RegisterClass(&wc);
}


// Initialization of the window instance which covers the entire screen
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
//	HWND hLayerWnd;
	hInst = hInstance; // ƒOƒ[ƒoƒ‹•Ï”‚ÉƒCƒ“ƒXƒ^ƒ“ƒXˆ—‚ðŠi”[‚µ‚Ü‚·B

	int x, y, w, h;

	// Get the size of the screen and save to global variables
	x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// x, y ‚ÌƒIƒtƒZƒbƒg’l‚ðŠo‚¦‚Ä‚¨‚­
	ofX = x;
	ofY = y;

	// Š®‘S‚É“§‰ß‚µ‚½ƒEƒBƒ“ƒhƒE‚ðì‚é
	hWnd = CreateWindowEx(
	         WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST
#if(_WIN32_WINNT >= 0x0500)
	         | WS_EX_NOACTIVATE
#endif
	         ,
	         szWindowClass, NULL, WS_POPUP,
	         0, 0, 0, 0,
	         NULL, NULL, hInstance, NULL);

	// ì‚ê‚È‚©‚Á‚½...?
	if(!hWnd) return FALSE;

	// ‘S‰æ–Ê‚ð•¢‚¤
	MoveWindow(hWnd, x, y, w, h, FALSE);

	// nCmdShow ‚ð–³Ž‹ (SW_MAXIMIZE ‚Æ‚©‚³‚ê‚é‚Æ¢‚é)
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// ESCƒL[ŒŸ’mƒ^ƒCƒ}[
	SetTimer(hWnd, 1, 100, NULL);


	// ƒŒƒCƒ„[ƒEƒBƒ“ƒhƒE‚Ìì¬
	hLayerWnd = CreateWindowEx(
	              WS_EX_TOOLWINDOW
#if(_WIN32_WINNT >= 0x0500)
	              | WS_EX_LAYERED | WS_EX_NOACTIVATE
#endif
	              ,
	              szWindowClassL, NULL, WS_POPUP,
	              100, 100, 300, 300,
	              hWnd, NULL, hInstance, NULL);

	SetLayeredWindowAttributes(hLayerWnd, RGB(255, 0, 0), 100, LWA_COLORKEY | LWA_ALPHA);

	return TRUE;
}

// Get the CLSID of the Encoder corresponding to the specified format
// Cited from MSDN Library: Retrieving the Class Identifier for an Encoder
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if(wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

// Drawing the rubberband 
VOID drawRubberband(HDC hdc, LPRECT newRect, BOOL erase)
{

	static BOOL firstDraw = TRUE;	// 1 ‰ñ–Ú‚Í‘O‚Ìƒoƒ“ƒh‚ÌÁ‹Ž‚ðs‚í‚È‚¢
	static RECT lastRect  = {0};	// ÅŒã‚É•`‰æ‚µ‚½ƒoƒ“ƒh
	static RECT clipRect  = {0};	// ÅŒã‚É•`‰æ‚µ‚½ƒoƒ“ƒh

	if(firstDraw)
	{
		// Display Window layer
		ShowWindow(hLayerWnd, SW_SHOW);
		UpdateWindow(hLayerWnd);

		firstDraw = FALSE;
	}

	if(erase)
	{
		// Hide the window layer
		ShowWindow(hLayerWnd, SW_HIDE);
	}

	// Check the coordinates
	clipRect = *newRect;
	if (clipRect.right  < clipRect.left)
	{
		int tmp = clipRect.left;
		clipRect.left   = clipRect.right;
		clipRect.right  = tmp;
	}
	if (clipRect.bottom < clipRect.top)
	{
		int tmp = clipRect.top;
		clipRect.top    = clipRect.bottom;
		clipRect.bottom = tmp;
	}

	MoveWindow(hLayerWnd,  clipRect.left, clipRect.top,
	           clipRect.right -  clipRect.left + 1, clipRect.bottom - clipRect.top + 1, true);
}

// Converts the source file to PNG on the specified location
BOOL convertPNG(LPCTSTR destFile, LPCTSTR srcFile)
{
	BOOL				res = FALSE;

	GdiplusStartupInput	gdiplusStartupInput;
	ULONG_PTR			gdiplusToken;
	CLSID				clsidEncoder;

	// GDI+ ‚Ì‰Šú‰»
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	Image *b = new Image(srcFile, 0);

	if(0 == b->GetLastStatus())
	{
		if(GetEncoderClsid(L"image/png", &clsidEncoder))
		{
			// save!
			if(0 == b->Save(destFile, &clsidEncoder, 0))
			{
				// •Û‘¶‚Å‚«‚½
				res = TRUE;
			}
		}
	}

	// ŒãŽn––
	delete b;
	GdiplusShutdown(gdiplusToken);

	return res;
}

// Saves a PNG file to a specified location
BOOL savePNG(LPCTSTR fileName, HBITMAP newBMP)
{
	BOOL				res = FALSE;

	GdiplusStartupInput	gdiplusStartupInput;
	ULONG_PTR			gdiplusToken;
	CLSID				clsidEncoder;

	// GDI+ ‚Ì‰Šú‰»
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// HBITMAP ‚©‚ç Bitmap ‚ðì¬
	Bitmap *b = new Bitmap(newBMP, NULL);

	if(GetEncoderClsid(L"image/png", &clsidEncoder))
	{
		// save!
		if(0 ==
		    b->Save(fileName, &clsidEncoder, 0))
		{
			// •Û‘¶‚Å‚«‚½
			res = TRUE;
		}
	}

	// ŒãŽn––
	delete b;
	GdiplusShutdown(gdiplusToken);

	return res;
}

// Window layer procedure
LRESULT CALLBACK LayerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC		hdc;
	RECT	clipRect = {0, 0, 500, 500};
	HBRUSH	hBrush;
	HPEN	hPen;
	HFONT	hFont;

	switch(message)
	{
		case WM_ERASEBKGND:
			GetClientRect(hWnd, &clipRect);

			hdc = GetDC(hWnd);
			hBrush = CreateSolidBrush(RGB(100, 100, 100));
			SelectObject(hdc, hBrush);
			hPen = CreatePen(PS_DASH, 1, RGB(255, 255, 255));
			SelectObject(hdc, hPen);
			Rectangle(hdc, 0, 0, clipRect.right, clipRect.bottom);

			//‹éŒ`‚ÌƒTƒCƒY‚ðo—Í
			int fHeight;
			fHeight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			hFont = CreateFont(fHeight,    //ƒtƒHƒ“ƒg‚‚³
			                   0,                    //•¶Žš•
			                   0,                    //ƒeƒLƒXƒg‚ÌŠp“x
			                   0,                    //ƒx[ƒXƒ‰ƒCƒ“‚Æ‚˜Ž²‚Æ‚ÌŠp“x
			                   FW_REGULAR,            //ƒtƒHƒ“ƒg‚Ìd‚³i‘¾‚³j
			                   FALSE,                //ƒCƒ^ƒŠƒbƒN‘Ì
			                   FALSE,                //ƒAƒ“ƒ_[ƒ‰ƒCƒ“
			                   FALSE,                //‘Å‚¿Á‚µü
			                   ANSI_CHARSET,    //•¶ŽšƒZƒbƒg
			                   OUT_DEFAULT_PRECIS,    //o—Í¸“x
			                   CLIP_DEFAULT_PRECIS,//ƒNƒŠƒbƒsƒ“ƒO¸“x
			                   PROOF_QUALITY,        //o—Í•iŽ¿
			                   FIXED_PITCH | FF_MODERN,//ƒsƒbƒ`‚Æƒtƒ@ƒ~ƒŠ[
			                   L"Tahoma");    //‘‘Ì–¼

			SelectObject(hdc, hFont);
			// show size
			int iWidth, iHeight;
			iWidth  = clipRect.right  - clipRect.left;
			iHeight = clipRect.bottom - clipRect.top;

			wchar_t sWidth[200], sHeight[200];
			swprintf_s(sWidth, L"%d", iWidth);
			swprintf_s(sHeight, L"%d", iHeight);

			int w, h, h2;
			w = -fHeight * 2.5 + 8;
			h = -fHeight * 2 + 8;
			h2 = h + fHeight;

			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(0, 0, 0));
			TextOut(hdc, clipRect.right - w + 1, clipRect.bottom - h + 1, (LPCWSTR)sWidth, wcslen(sWidth));
			TextOut(hdc, clipRect.right - w + 1, clipRect.bottom - h2 + 1, (LPCWSTR)sHeight, wcslen(sHeight));
			SetTextColor(hdc, RGB(255, 255, 255));
			TextOut(hdc, clipRect.right - w, clipRect.bottom - h, (LPCWSTR)sWidth, wcslen(sWidth));
			TextOut(hdc, clipRect.right - w, clipRect.bottom - h2, (LPCWSTR)sHeight, wcslen(sHeight));

			DeleteObject(hPen);
			DeleteObject(hBrush);
			DeleteObject(hFont);
			ReleaseDC(hWnd, hdc);

			return TRUE;

			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;

}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;

	static BOOL onClip		= FALSE;
	static BOOL firstDraw	= TRUE;
	static RECT clipRect	= {0, 0, 0, 0};

	switch(message)
	{
		case WM_RBUTTONDOWN:	// Right button down, abort screen capture
			DestroyWindow(hWnd);
			return DefWindowProc(hWnd, message, wParam, lParam);
			break;

		case WM_TIMER:	// User spent too much time on selecting the area, program aborts
			if(GetKeyState(VK_ESCAPE) & 0x8000)
			{
				DestroyWindow(hWnd);
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_MOUSEMOVE:	// Whenever mouse has been moved
			if(onClip)
			{
				// V‚µ‚¢À•W‚ðƒZƒbƒg
				clipRect.right  = LOWORD(lParam) + ofX;
				clipRect.bottom = HIWORD(lParam) + ofY;

				hdc = GetDC(NULL);
				drawRubberband(hdc, &clipRect, FALSE);

				ReleaseDC(NULL, hdc);
			}
			break;


		case WM_LBUTTONDOWN:	// Left mouse button pressed, starting point of capture
			{
				// ƒNƒŠƒbƒvŠJŽn
				onClip = TRUE;

				// ‰ŠúˆÊ’u‚ðƒZƒbƒg
				clipRect.left = LOWORD(lParam) + ofX;
				clipRect.top  = HIWORD(lParam) + ofY;

				// ƒ}ƒEƒX‚ðƒLƒƒƒvƒ`ƒƒ
				SetCapture(hWnd);
			}
			break;

		case WM_LBUTTONUP:	// Released the mouse key press, upload the image
			{
				// ƒNƒŠƒbƒvI—¹
				onClip = FALSE;

				// ƒ}ƒEƒX‚ÌƒLƒƒƒvƒ`ƒƒ‚ð‰ðœ
				ReleaseCapture();

				// V‚µ‚¢À•W‚ðƒZƒbƒg
				clipRect.right  = LOWORD(lParam) + ofX;
				clipRect.bottom = HIWORD(lParam) + ofY;

				// ‰æ–Ê‚É’¼Ú•`‰æC‚Á‚ÄŒ`
				HDC hdc = GetDC(NULL);

				// ü‚ðÁ‚·
				drawRubberband(hdc, &clipRect, TRUE);

				// À•Wƒ`ƒFƒbƒN
				if(clipRect.right  < clipRect.left)
				{
					int tmp = clipRect.left;
					clipRect.left   = clipRect.right;
					clipRect.right  = tmp;
				}
				if(clipRect.bottom < clipRect.top)
				{
					int tmp = clipRect.top;
					clipRect.top    = clipRect.bottom;
					clipRect.bottom = tmp;
				}

				// ‰æ‘œ‚ÌƒLƒƒƒvƒ`ƒƒ
				int iWidth, iHeight;
				iWidth  = clipRect.right  - clipRect.left + 1;
				iHeight = clipRect.bottom - clipRect.top  + 1;

				if(iWidth == 0 || iHeight == 0)
				{
					// ‰æ‘œ‚É‚È‚Á‚Ä‚È‚¢, ‚È‚É‚à‚µ‚È‚¢
					ReleaseDC(NULL, hdc);
					DestroyWindow(hWnd);
					break;
				}

				// ƒrƒbƒgƒ}ƒbƒvƒoƒbƒtƒ@‚ðì¬
				HBITMAP newBMP = CreateCompatibleBitmap(hdc, iWidth, iHeight);
				HDC	    newDC  = CreateCompatibleDC(hdc);

				// ŠÖ˜A‚Ã‚¯
				SelectObject(newDC, newBMP);

				// ‰æ‘œ‚ðŽæ“¾
				BitBlt(newDC, 0, 0, iWidth, iHeight,
				       hdc, clipRect.left, clipRect.top, SRCCOPY);

				// ƒEƒBƒ“ƒhƒE‚ð‰B‚·!
				ShowWindow(hWnd, SW_HIDE);
		

				// ƒeƒ“ƒ|ƒ‰ƒŠƒtƒ@ƒCƒ‹–¼‚ðŒˆ’è
				TCHAR tmpDir[MAX_PATH], tmpFile[MAX_PATH];
				GetTempPath(MAX_PATH, tmpDir);
				GetTempFileName(tmpDir, _T("gya"), 0, tmpFile);

				if(savePNG(tmpFile, newBMP))
				{

					// ‚¤‚
					if(!uploadFile(hWnd, tmpFile))
					{
				
					}
				}
				else
				{
					// PNG•Û‘¶Ž¸”s...
					MessageBox(hWnd, _T("Cannot save png image"), szTitle,
					           MB_OK | MB_ICONERROR);
				}

				// Cleanup and exit
				DeleteFile(tmpFile);

				DeleteDC(newDC);
				DeleteObject(newBMP);

				ReleaseDC(NULL, hdc);
				DestroyWindow(hWnd);
				PostQuitMessage(0);
			}
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Copy the content of 'str' to the clipboard
VOID setClipBoardText(const char* str)
{

	HGLOBAL hText;
	char    *pText;
	size_t  slen;

	slen  = strlen(str) + 1; // NULL

	hText = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, slen * sizeof(TCHAR));

	pText = (char *)GlobalLock(hText);
	strncpy_s(pText, slen, str, slen);
	GlobalUnlock(hText);

	// ƒNƒŠƒbƒvƒ{[ƒh‚ðŠJ‚­
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hText);
	CloseClipboard();

	// ‰ð•ú
	GlobalFree(hText);
}

// Open the default browser on the client machine with the 
// address set to 'str'
VOID execUrl(const char* str)
{
	size_t  slen;
	size_t  dcount;
	slen  = strlen(str) + 1; // NULL

	TCHAR *wcUrl = (TCHAR *)malloc(slen * sizeof(TCHAR));

	// ƒƒCƒh•¶Žš‚É•ÏŠ·
	mbstowcs_s(&dcount, wcUrl, slen, str, slen);

	// open ƒRƒ}ƒ“ƒh‚ðŽÀs
	SHELLEXECUTEINFO lsw = {0};
	lsw.cbSize = sizeof(SHELLEXECUTEINFO);
	lsw.lpVerb = _T("open");
	lsw.lpFile = wcUrl;

	ShellExecuteEx(&lsw);

	free(wcUrl);
}

// ID ‚ð¶¬Eƒ[ƒh‚·‚é
std::string getId()
{

	TCHAR idFile[_MAX_PATH];
	TCHAR idDir[_MAX_PATH];

	SHGetSpecialFolderPath(NULL, idFile, CSIDL_APPDATA, FALSE);

	_tcscat_s(idFile, _T("\\Gyazo"));
	_tcscpy_s(idDir, idFile);
	_tcscat_s(idFile, _T("\\id.txt"));

	const TCHAR*	 idOldFile			= _T("id.txt");
	BOOL oldFileExist = FALSE;

	std::string idStr;

	// ‚Ü‚¸‚Íƒtƒ@ƒCƒ‹‚©‚ç ID ‚ðƒ[ƒh
	std::ifstream ifs;

	ifs.open(idFile);
	if(! ifs.fail())
	{
		// ID ‚ð“Ç‚Ýž‚Þ
		ifs >> idStr;
		ifs.close();
	}
	else
	{
		std::ifstream ifsold;
		ifsold.open(idOldFile);
		if(! ifsold.fail())
		{
			// “¯ˆêƒfƒBƒŒƒNƒgƒŠ‚©‚çID ‚ð“Ç‚Ýž‚Þ(‹Œƒo[ƒWƒ‡ƒ“‚Æ‚ÌŒÝŠ·«)
			ifsold >> idStr;
			ifsold.close();
		}
	}

	return idStr;
}

// Save ID of the image
BOOL saveId(const WCHAR* str)
{

	TCHAR idFile[_MAX_PATH];
	TCHAR idDir[_MAX_PATH];

	SHGetSpecialFolderPath(NULL, idFile, CSIDL_APPDATA, FALSE);

	_tcscat_s(idFile, _T("\\Gyazo"));
	_tcscpy_s(idDir, idFile);
	_tcscat_s(idFile, _T("\\id.txt"));

	const TCHAR*	 idOldFile			= _T("id.txt");

	size_t  slen;
	size_t  dcount;
	slen  = _tcslen(str) + 1; // NULL

	char *idStr = (char *)malloc(slen * sizeof(char));
	// ƒoƒCƒg•¶Žš‚É•ÏŠ·
	wcstombs_s(&dcount, idStr, slen, str, slen);

	// ID ‚ð•Û‘¶‚·‚é
	CreateDirectory(idDir, NULL);
	std::ofstream ofs;
	ofs.open(idFile);
	if(! ofs.fail())
	{
		ofs << idStr;
		ofs.close();

		// ‹ŒÝ’èƒtƒ@ƒCƒ‹‚Ìíœ
		if(PathFileExists(idOldFile))
		{
			DeleteFile(idOldFile);
		}
	}
	else
	{
		free(idStr);
		return FALSE;
	}

	free(idStr);
	return TRUE;
}

// Uploads the PNG file to Gyazo
BOOL uploadFile(HWND hwnd, LPCTSTR fileName)
{
	const TCHAR* UPLOAD_SERVER	= _T("gyazo.com");
	const TCHAR* UPLOAD_PATH	= _T("/upload.cgi");

	const char*  sBoundary = "----BOUNDARYBOUNDARY----";		// boundary
	const char   sCrLf[]   = { 0xd, 0xa, 0x0 };					// ‰üs(CR+LF)
	const TCHAR* szHeader  =
	  _T("Content-type: multipart/form-data; boundary=----BOUNDARYBOUNDARY----");

	std::ostringstream	buf;	// ‘—MƒƒbƒZ[ƒW
	std::string			idStr;	// ID

	// ID ‚ðŽæ“¾
	idStr = getId();

	// ƒƒbƒZ[ƒW‚Ì\¬
	// -- "id" part
	buf << "--";
	buf << sBoundary;
	buf << sCrLf;
	buf << "content-disposition: form-data; name=\"id\"";
	buf << sCrLf;
	buf << sCrLf;
	buf << idStr;
	buf << sCrLf;

	// -- "imagedata" part
	buf << "--";
	buf << sBoundary;
	buf << sCrLf;
	buf << "content-disposition: form-data; name=\"imagedata\"; filename=\"gyazo.com\"";
	buf << sCrLf;
	//buf << "Content-type: image/png";	// ˆê‰ž
	//buf << sCrLf;
	buf << sCrLf;

	// –{•¶: PNG ƒtƒ@ƒCƒ‹‚ð“Ç‚Ýž‚Þ
	std::ifstream png;
	png.open(fileName, std::ios::binary);
	if(png.fail())
	{
		MessageBox(hwnd, _T("PNG open failed"), szTitle, MB_ICONERROR | MB_OK);
		png.close();
		return FALSE;
	}
	buf << png.rdbuf();		// read all & append to buffer
	png.close();

	// ÅŒã
	buf << sCrLf;
	buf << "--";
	buf << sBoundary;
	buf << "--";
	buf << sCrLf;

	// ƒƒbƒZ[ƒWŠ®¬
	std::string oMsg(buf.str());

	// WinInet ‚ð€”õ (proxy ‚Í ‹K’è‚ÌÝ’è‚ð—˜—p)
	HINTERNET hSession    = InternetOpen(szTitle,
	                                     INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(NULL == hSession)
	{
		MessageBox(hwnd, _T("Cannot configure wininet"),
		           szTitle, MB_ICONERROR | MB_OK);
		return FALSE;
	}

	// Ú‘±æ
	HINTERNET hConnection = InternetConnect(hSession,
	                                        UPLOAD_SERVER, INTERNET_DEFAULT_HTTP_PORT,
	                                        NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	if(NULL == hSession)
	{
		MessageBox(hwnd, _T("Cannot initiate connection"),
		           szTitle, MB_ICONERROR | MB_OK);
		return FALSE;
	}

	// —v‹æ‚ÌÝ’è
	HINTERNET hRequest    = HttpOpenRequest(hConnection,
	                                        _T("POST"), UPLOAD_PATH, NULL,
	                                        NULL, NULL, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD, NULL);
	if(NULL == hSession)
	{
		MessageBox(hwnd, _T("Cannot compose post request"),
		           szTitle, MB_ICONERROR | MB_OK);
		return FALSE;
	}

	// User-Agent‚ðŽw’è
	const TCHAR* ua = _T("User-Agent: Gyazowin/1.0\r\n");
	BOOL bResult = HttpAddRequestHeaders(
	                 hRequest, ua, _tcslen(ua),
	                 HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
	if(FALSE == bResult)
	{
		MessageBox(hwnd, _T("Cannot set user agent"),
		           szTitle, MB_ICONERROR | MB_OK);
		return FALSE;
	}

	// —v‹‚ð‘—M
	if(HttpSendRequest(hRequest,
	                   szHeader,
	                   lstrlen(szHeader),
	                   (LPVOID)oMsg.c_str(),
	                   (DWORD) oMsg.length()))
	{
		// —v‹‚Í¬Œ÷

		DWORD resLen = 8;
		TCHAR resCode[8];

		// status code ‚ðŽæ“¾
		HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE, resCode, &resLen, 0);
		if(_ttoi(resCode) != 200)
		{
			// upload Ž¸”s (status error)
			MessageBox(hwnd, _T("Failed to upload (unexpected result code, under maintainance?)"),
			           szTitle, MB_ICONERROR | MB_OK);
		}
		else
		{
			// upload succeeded

			// get new id
			DWORD idLen = 100;
			TCHAR newid[100];

			memset(newid, 0, idLen * sizeof(TCHAR));
			_tcscpy_s(newid, _T("X-Gyazo-Id"));

			HttpQueryInfo(hRequest, HTTP_QUERY_CUSTOM, newid, &idLen, 0);
			if(GetLastError() != ERROR_HTTP_HEADER_NOT_FOUND && idLen != 0)
			{
				//save new id
				saveId(newid);
			}

			// Œ‹‰Ê (URL) ‚ð“ÇŽæ‚é
			DWORD len;
			char  resbuf[1024];
			std::string result;

			// ‚»‚ñ‚È‚É’·‚¢‚±‚Æ‚Í‚È‚¢‚¯‚Ç‚Ü‚ ˆê‰ž
			while(InternetReadFile(hRequest, (LPVOID) resbuf, 1024, &len)
			      && len != 0)
			{
				result.append(resbuf, len);
			}

			// Žæ“¾Œ‹‰Ê‚Í NULL terminate ‚³‚ê‚Ä‚¢‚È‚¢‚Ì‚Å
			result += '\0';

			// ƒNƒŠƒbƒvƒ{[ƒh‚É URL ‚ðƒRƒs[
			setClipBoardText(result.c_str());

			// URL ‚ð‹N“®
			execUrl(result.c_str());

			return TRUE;
		}
	}
	else
	{
		// ƒAƒbƒvƒ[ƒhŽ¸”s...
		MessageBox(hwnd, _T("Failed to upload"), szTitle, MB_ICONERROR | MB_OK);
	}

	return FALSE;

}