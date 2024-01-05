// This file is derived from MagnifierSample.cpp of the Microsoft WinfFX
// SDK Code Samples, which is licensed under the MIT License.

#include <windows.h>
#include <magnification.h>
#include <string>
#include <fstream>

// For simplicity, the sample uses a constant magnification factor.
#define MAGFACTOR  2.0f
#define RESTOREDWINDOWSTYLES WS_SIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CAPTION | WS_MAXIMIZEBOX

// Global variables and strings.
HINSTANCE           hInst;
const TCHAR         WindowClassName[]= TEXT("ColorEffectWindow");
const TCHAR         WindowTitle[]= TEXT("ColorEffectCmd");
const UINT          timerInterval = 16; // close to the refresh rate @60hz
HWND                hwndMag;
HWND                hwndHost;
RECT                magWindowRect;
RECT                hostWindowRect;

// Forward declarations.
ATOM                RegisterHostWindowClass(HINSTANCE hInstance);
BOOL                SetupMagnifier(HINSTANCE hinst);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void CALLBACK       UpdateMagWindow(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void                GoFullScreen();
void                GoPartialScreen();
BOOL                isFullScreen = FALSE;
std::wstring        GetMatrixFilePath();

//
// FUNCTION: WinMain()
//
// PURPOSE: Entry point for the application.
//
int APIENTRY WindowedModeWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE /*hPrevInstance*/,
					 _In_ LPSTR     /*lpCmdLine*/,
					 _In_ int       nCmdShow)
{
	if (FALSE == MagInitialize())
	{
		return 0;
	}
	if (FALSE == SetupMagnifier(hInstance))
	{
		return 0;
	}

	ShowWindow(hwndHost, nCmdShow);
	UpdateWindow(hwndHost);

	// Create a timer to update the control.
	UINT_PTR timerId = SetTimer(hwndHost, 0, timerInterval, UpdateMagWindow);

	// Main message loop.
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Shut down.
	KillTimer(NULL, timerId);
	MagUninitialize();
	return (int) msg.wParam;
}

//
// FUNCTION: HostWndProc()
//
// PURPOSE: Window procedure for the window that hosts the magnifier control.
//
LRESULT CALLBACK HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// get the application icon
	static HINSTANCE hInstance = GetModuleHandle(NULL);
	static HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
	
	switch (message) 
	{
	case WM_CREATE:
		// set application icon
		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			if (isFullScreen) 
			{
				GoPartialScreen();
			}
		}
		break;

	case WM_SYSCOMMAND:
		if (GET_SC_WPARAM(wParam) == SC_MAXIMIZE)
		{
			GoFullScreen();
		}
		else
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		if ( hwndMag != NULL )
		{
			GetClientRect(hWnd, &magWindowRect);
			// Resize the control to fill the window.
			SetWindowPos(hwndMag, NULL, 
				magWindowRect.left, magWindowRect.top, magWindowRect.right, magWindowRect.bottom, 0);
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;  
}

//
//  FUNCTION: RegisterHostWindowClass()
//
//  PURPOSE: Registers the window class for the window that contains the magnification control.
//
ATOM RegisterHostWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = {};

	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = HostWndProc;
	wcex.hInstance      = hInstance;
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(1 + COLOR_BTNFACE);
	wcex.lpszClassName  = WindowClassName;

	return RegisterClassEx(&wcex);
}

//
// FUNCTION: SetupMagnifier
//
// PURPOSE: Creates the windows and initializes magnification.
//
BOOL SetupMagnifier(HINSTANCE hinst)
{
	// Set bounds of host window according to screen size.
	hostWindowRect.top = 0;
	hostWindowRect.bottom = GetSystemMetrics(SM_CYSCREEN) / 2;
	hostWindowRect.left = 0;
	hostWindowRect.right = GetSystemMetrics(SM_CXSCREEN) / 2;

	// Create the host window.
	RegisterHostWindowClass(hinst);
	hwndHost = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, 
		WindowClassName, WindowTitle, 
		RESTOREDWINDOWSTYLES,
		0, 0, hostWindowRect.right, hostWindowRect.bottom, NULL, NULL, hInst, NULL);
	if (!hwndHost)
	{
		return FALSE;
	}

	// Make the window opaque.
	SetLayeredWindowAttributes(hwndHost, 0, 255, LWA_ALPHA);

	// Create a magnifier control that fills the client area.
	GetClientRect(hwndHost, &magWindowRect);
	hwndMag = CreateWindow(WC_MAGNIFIER, TEXT("ColorEffectWindow"), 
		WS_CHILD | WS_VISIBLE,
		magWindowRect.left, magWindowRect.top, magWindowRect.right, magWindowRect.bottom, hwndHost, NULL, hInst, NULL );
	if (!hwndMag)
	{
		return FALSE;
	}

	// load matrix.txt from AppData/Local/ColorEffectCmd/matrix.txt
	// if it doesn't exist, create it as the identity matrix
	MAGCOLOREFFECT colorEffect;
	std::wstring matrixFilePath = GetMatrixFilePath();
	std::ifstream file(matrixFilePath);
	if (file) {
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				file >> colorEffect.transform[i][j];
			}
		}
		file.close();
	} else {
		file.close();
		std::ofstream newFile(matrixFilePath);
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				newFile << (i == j ? 1 : 0) << " ";
				colorEffect.transform[i][j] = (i == j ? 1 : 0);
			}
			newFile << std::endl;
		}
		newFile.close();
	}

	BOOL ret = MagSetColorEffect(hwndMag,&colorEffect);

	return ret;
}


//
// FUNCTION: UpdateMagWindow()
//
// PURPOSE: Sets the source rectangle and updates the window. Called by a timer.
//
void CALLBACK UpdateMagWindow(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/)
{
	// get location of the magnifier window
	RECT sourceRect;
	GetClientRect(hwndMag, &sourceRect);
	POINT p1, p2;
	p1.x = sourceRect.left;
	p1.y = sourceRect.top;
	p2.x = sourceRect.right;
	p2.y = sourceRect.bottom;
	ClientToScreen(hwndMag, &p1);
	ClientToScreen(hwndMag, &p2);
	sourceRect.left = p1.x;
	sourceRect.top = p1.y;
	sourceRect.right = p2.x;
	sourceRect.bottom = p2.y;

	// Set the source rectangle for the magnifier control.
	MagSetWindowSource(hwndMag, sourceRect);

	// Reclaim topmost status, to prevent unmagnified menus from remaining in view. 
	SetWindowPos(hwndHost, HWND_TOPMOST, 0, 0, 0, 0, 
		SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

	// Force redraw.
	InvalidateRect(hwndMag, NULL, TRUE);
}


//
// FUNCTION: GoFullScreen()
//
// PURPOSE: Makes the host window full-screen on the current monitor by placing non-client elements outside the display.
//
void GoFullScreen()
{
	isFullScreen = TRUE;
	// The window must be styled as layered for proper rendering. 
	// It is styled as transparent so that it does not capture mouse clicks.
	SetWindowLong(hwndHost, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT);
	// Give the window a system menu so it can be closed on the taskbar.
	SetWindowLong(hwndHost, GWL_STYLE, WS_POPUP | WS_SYSMENU);

	// Get the current monitor handle
	HMONITOR hMonitor = MonitorFromWindow(hwndHost, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &monitorInfo);

	// Calculate the window origin and span for full-screen mode.
	int xOrigin = monitorInfo.rcMonitor.left;
	int yOrigin = monitorInfo.rcMonitor.top;
	int xSpan = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	int ySpan = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

	SetWindowPos(hwndHost, HWND_TOPMOST, xOrigin, yOrigin, xSpan, ySpan, 
		SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE);
}

//
// FUNCTION: GoPartialScreen()
//
// PURPOSE: Makes the host window resizable and focusable.
//
void GoPartialScreen()
{
	isFullScreen = FALSE;

	SetWindowLong(hwndHost, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED);
	SetWindowLong(hwndHost, GWL_STYLE, RESTOREDWINDOWSTYLES);
	SetWindowPos(hwndHost, HWND_TOPMOST, 
		hostWindowRect.left, hostWindowRect.top, hostWindowRect.right, hostWindowRect.bottom, 
		SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE);
}
