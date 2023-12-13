#include <windows.h>
#include <magnification.h>
#include <shlobj_core.h>
#include <string>
#include <fstream>
#include <combaseapi.h>
#include <iostream>
#include <shellapi.h>
#include <sstream>
#include <iomanip>
#include <winreg.h>

// Global variables
HWND hEdit[5][5];
HWND hButtonPreview, hButtonSave, hButtonInstall;

// Function to get the path to matrix.txt
// creates ColorEffectCmd folder in AppData/Local if it doesn't exist
std::wstring GetMatrixFilePath() {
	PWSTR path;
	HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
	if (result != S_OK) {
		std::cerr << "Failed to get AppData/Local folder path" << std::endl;
		return L"";
	}
	// Create ColorEffectCmd folder if it doesn't exist
	std::wstring folderPath = std::wstring(path) + L"\\ColorEffectCmd";
	if (!CreateDirectoryW(folderPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
		std::cerr << "Failed to create ColorEffectCmd folder" << std::endl;
		return L"";
	}
	CoTaskMemFree(path);
	return folderPath + L"\\matrix.txt";
}

std::wstring GetInstalledEXEFilePath() {
	PWSTR path;
	HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
	if (result != S_OK) {
		std::cerr << "Failed to get AppData/Local folder path" << std::endl;
		return L"";
	}
	// Create ColorEffectCmd folder if it doesn't exist
	std::wstring folderPath = std::wstring(path) + L"\\ColorEffectCmd";
	if (!CreateDirectoryW(folderPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
		std::cerr << "Failed to create ColorEffectCmd folder" << std::endl;
		return L"";
	}
	CoTaskMemFree(path);
	return folderPath + L"\\ColorEffectCmd.exe";
}

std::wstring SelfEXEPath() {
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	return std::wstring(buffer);
}

int Preview(MAGCOLOREFFECT colorEffect) {
	// Initialize Magnification API
	if (!MagInitialize()) {
		std::cerr << "Failed to initialize Magnification API" << std::endl;
		return 1;
	}

	// Set the color effect
	BOOL success = MagSetFullscreenColorEffect(&colorEffect);

	// Check if the color effect was set successfully
	if (!success) {
		DWORD errorCode = GetLastError();
		LPSTR errorMessage = nullptr;

		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			errorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&errorMessage),
			0,
			nullptr);

		std::cerr << "Failed to set color effect. Error code: " << errorCode << std::endl;
		std::cerr << "Error message: " << errorMessage << std::endl;

		LocalFree(errorMessage);
		return 1;
	}

	// Wait for 3 seconds
	Sleep(3000);

	// Cleanup
	MagUninitialize();
	
	return 0;
}

int Apply(MAGCOLOREFFECT colorEffect) {
	// Initialize Magnification API
	if (!MagInitialize()) {
		std::cerr << "Failed to initialize Magnification API" << std::endl;
		return 1;
	}

	// Set the color effect
	BOOL success = MagSetFullscreenColorEffect(&colorEffect);

	// Check if the color effect was set successfully
	if (!success) {
		DWORD errorCode = GetLastError();
		LPSTR errorMessage = nullptr;

		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			errorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&errorMessage),
			0,
			nullptr);

		std::cerr << "Failed to set color effect. Error code: " << errorCode << std::endl;
		std::cerr << "Error message: " << errorMessage << std::endl;

		LocalFree(errorMessage);
		return 1;
	}

	return 0;
}

void Install() {
	// copy ColorEffectCmd.exe to AppData/Local/ColorEffectCmd/ColorEffectCmd.exe
	std::wstring sourcePath = SelfEXEPath();
	std::wstring destinationPath = GetInstalledEXEFilePath();
	if (!CopyFileW(sourcePath.c_str(), destinationPath.c_str(), FALSE)) {
		std::cerr << "Failed to copy ColorEffectCmd.exe to AppData/Local/ColorEffectCmd/ColorEffectCmd.exe" << std::endl;
		return;
	}
	
	// create registry key to run the program at login
	HKEY hKey;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
		std::cerr << "Failed to create registry key" << std::endl;
		return;
	}

	// Add the /h flag to the registry value data
	std::wstring registryValueData = destinationPath + L" /h";
	if (RegSetValueExW(hKey, L"ColorEffectCmd", 0, REG_SZ, (BYTE*)registryValueData.c_str(), (registryValueData.size() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
		std::cerr << "Failed to set registry value" << std::endl;
		return;
	}
	RegCloseKey(hKey);
}

void Uninstall() {
	// remove registry key to run the program at login
	if (RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"ColorEffectCmd") != ERROR_SUCCESS) {
		std::cerr << "Failed to delete registry key" << std::endl;
		return;
	}
}

bool IsInstalled() {
	// check if the AppData/Local/ColorEffectCmd/ColorEffectCmd.exe file exists
	std::ifstream file(GetInstalledEXEFilePath());
	bool fileExists = file.good();
	
	// check if the registry key exists to run the program at login
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		std::cerr << "Failed to open registry key" << std::endl;
		return false;
	}
	DWORD type;
	DWORD size;
	if (RegQueryValueExW(hKey, L"ColorEffectCmd", NULL, &type, NULL, &size) != ERROR_SUCCESS) {
		std::cerr << "Failed to query registry value" << std::endl;
		return false;
	}
	RegCloseKey(hKey);
	
	return fileExists && type == REG_SZ;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static HINSTANCE hInstance = GetModuleHandle(NULL);
	static HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
	static NOTIFYICONDATA nid = {
		sizeof(nid),
		hwnd, 1,
		NIF_ICON | NIF_MESSAGE | NIF_TIP, WM_APP + 1,
		hIcon,
		"ColorEffectCmd"
	};
	
	switch (msg) {
		case WM_CREATE: {
			// set application icon
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			
			// Add icon to system tray
			Shell_NotifyIcon(NIM_ADD, &nid);
			
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
			
			// create labels for each column
			CreateWindowEx(0, "STATIC", "R =", WS_CHILD | WS_VISIBLE,
				50, 10, 55, 25, hwnd, NULL, NULL, NULL);
			CreateWindowEx(0, "STATIC", "G =", WS_CHILD | WS_VISIBLE,
				110, 10, 55, 25, hwnd, NULL, NULL, NULL);
			CreateWindowEx(0, "STATIC", "B =", WS_CHILD | WS_VISIBLE,
				170, 10, 55, 25, hwnd, NULL, NULL, NULL);
			CreateWindowEx(0, "STATIC", "A =", WS_CHILD | WS_VISIBLE,
				230, 10, 55, 25, hwnd, NULL, NULL, NULL);
			
			// create labels for each row
			CreateWindowEx(0, "STATIC", "r *", WS_CHILD | WS_VISIBLE,
				10, 40, 30, 25, hwnd, NULL, NULL, NULL);
			CreateWindowEx(0, "STATIC", "g *", WS_CHILD | WS_VISIBLE,
				10, 70, 30, 25, hwnd, NULL, NULL, NULL);
			CreateWindowEx(0, "STATIC", "b *", WS_CHILD | WS_VISIBLE,
				10, 100, 30, 25, hwnd, NULL, NULL, NULL);
			CreateWindowEx(0, "STATIC", "a *", WS_CHILD | WS_VISIBLE,
				10, 130, 30, 25, hwnd, NULL, NULL, NULL);
			CreateWindowEx(0, "STATIC", "1 *", WS_CHILD | WS_VISIBLE,
				10, 160, 30, 25, hwnd, NULL, NULL, NULL);
			
			// Create 5x5 grid of text boxes and populate values from colorEffect
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 5; j++) {
					std::stringstream ss;
					ss << std::fixed << std::setprecision(4) << colorEffect.transform[i][j];
					hEdit[i][j] = CreateWindowEx(
						0, "EDIT", ss.str().c_str(),
						WS_CHILD | WS_VISIBLE | WS_BORDER,
						50 + j * 60, 40 + i * 30, 55, 25,
						hwnd, (HMENU)(i * 5 + j), NULL, NULL
					);

					// Disable the last column of inputs
					if (j == 4) {
						EnableWindow(hEdit[i][j], FALSE);
					}
				}
			}

			// Create "Preview" button
			hButtonPreview = CreateWindowEx(0, "BUTTON", "Preview", WS_CHILD | WS_VISIBLE,
				10, 200, 75, 25, hwnd, (HMENU)100, NULL, NULL);

			// Create "Save" button
			hButtonSave = CreateWindowEx(0, "BUTTON", "Save", WS_CHILD | WS_VISIBLE,
				90, 200, 75, 25, hwnd, (HMENU)200, NULL, NULL);
			
			// Create "Install" or "Uninstall" button
			hButtonInstall = CreateWindowEx(0, "BUTTON", IsInstalled() ? "Uninstall" : "Install", WS_CHILD | WS_VISIBLE,
				170, 200, 75, 25, hwnd, (HMENU)300, NULL, NULL);
				
			break;
		}
		case WM_COMMAND: {
			if (LOWORD(wParam) == 100) {
				// "Preview" button clicked
				MAGCOLOREFFECT colorEffect;
				for (int i = 0; i < 5; i++) {
					for (int j = 0; j < 5; j++) {
						char buffer[10];
						GetWindowText(hEdit[i][j], buffer, 10);
						colorEffect.transform[i][j] = atof(buffer);
					}
				}
				int error = Preview(colorEffect);
				if (error) {
					std::cerr << "Failed to preview color effect" << std::endl;
					exit(1);
				}
			} else if (LOWORD(wParam) == 200) {
				// "Save" button clicked
				// Read values from text boxes and save color effect matrix to a file
				MAGCOLOREFFECT colorEffect;
				std::ofstream file(GetMatrixFilePath());
				if (file) {
					for (int i = 0; i < 5; i++) {
						for (int j = 0; j < 5; j++) {
							char buffer[10];
							GetWindowText(hEdit[i][j], buffer, 10);
							colorEffect.transform[i][j] = atof(buffer);
							file << colorEffect.transform[i][j] << " ";
						}
						file << std::endl;
					}
					file.close();
				} else {
					file.close();
					std::cerr << "Failed to open file for writing" << std::endl;
				}
				
				// Apply the color effect
				int error = Apply(colorEffect);
				if (error) {
					std::cerr << "Failed to apply color effect" << std::endl;
					exit(1);
				}
				
				// Hide the window
				ShowWindow(hwnd, SW_HIDE);
			} else if (LOWORD(wParam) == 300) {
				// "Install" or "Uninstall" button clicked
				if (IsInstalled()) {
					Uninstall();
					SetWindowText(hButtonInstall, "Install");
				} else {
					Install();
					SetWindowText(hButtonInstall, "Uninstall");
				}
			}
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			// Remove icon from system tray
			Shell_NotifyIcon(NIM_DELETE, &nid);
			break;
		}
		case WM_APP + 1: {
			switch (lParam) {
				case WM_LBUTTONUP:
					MagUninitialize();
					ShowWindow(hwnd, SW_RESTORE); // Restore the window
					break;
			}
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// Register window class
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = "ColorEffectCmd";
	RegisterClass(&wc);

	// Create main window
	HWND hwnd = CreateWindowEx(0, "ColorEffectCmd", "ColorEffectCmd",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 380, 280, NULL, NULL, hInstance, NULL);
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	
	// /h option to hide the window
	if (__argc > 1 && strcmp(__argv[1], "/h") == 0) {
		// send WM_COMMAND 200 to simulate "Save" button click
		SendMessage(hwnd, WM_COMMAND, 200, 0);
	}

	// Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}
