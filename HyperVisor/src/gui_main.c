#include "gui_hypervisor.h"
#include <commctrl.h>
#include <d3dcompiler.h>

// Global variables
HYPERVISOR_WINDOW g_mainWindow = {0};
const char* g_className = "ARM64HypervisorGUI";

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Helper functions
void CreateControlButtons(HWND hwnd);
void UpdateStatusDisplay(const char* status);
void LogMessage(const char* format, ...);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    printf("============================================\n");
    printf("    ARM64n"); Hypervisor with Graphics GUI\
    printf("============================================\n\n");
    
    LogMessage("Initializing GUI Hypervisor...");
    
    // Check Administrator privileges
    BOOL isAdmin = FALSE;
    HANDLE token = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }
    
    if (!isAdmin) {
        MessageBox(NULL, 
                  "This hypervisor requires Administrator privileges.\n\n"
                  "Please run as Administrator for full functionality.",
                  "Administrator Required", MB_OK | MB_ICONWARNING);
    }
    
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = g_className;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Failed to register window class!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Create main window
    HWND hwnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        g_className,
        "ARM64 Hypervisor - Real GUI with Graphics",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 900,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwnd) {
        MessageBox(NULL, "Failed to create window!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    g_mainWindow.hwnd = hwnd;
    
    // Initialize hypervisor
    g_mainWindow.hypervisor = (GUI_HYPERVISOR*)calloc(1, sizeof(GUI_HYPERVISOR));
    if (!g_mainWindow.hypervisor) {
        MessageBox(NULL, "Failed to allocate hypervisor memory!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Initialize graphics
    if (InitializeGraphics(g_mainWindow.hypervisor, hwnd) != 0) {
        MessageBox(NULL, "Failed to initialize graphics!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Create control buttons
    CreateControlButtons(hwnd);
    
    // Show window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    LogMessage("GUI Hypervisor initialized successfully!");
    UpdateStatusDisplay("Ready - Load an OS to start virtualization");
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    if (g_mainWindow.hypervisor) {
        StopVM(g_mainWindow.hypervisor);
        free(g_mainWindow.hypervisor);
    }
    
    CoUninitialize();
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            break;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Render guest framebuffer if VM is running
            if (g_mainWindow.hypervisor && g_mainWindow.hypervisor->running) {
                RenderGuestFramebuffer(g_mainWindow.hypervisor);
            } else {
                // Draw placeholder screen
                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                
                // Draw main display area
                RECT displayRect = {50, 50, clientRect.right - 50, clientRect.bottom - 150};
                FillRect(hdc, &displayRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
                
                // Draw border
                HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(128, 128, 128));
                HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
                Rectangle(hdc, displayRect.left, displayRect.top, displayRect.right, displayRect.bottom);
                SelectObject(hdc, oldPen);
                DeleteObject(borderPen);
                
                // Draw placeholder text
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkMode(hdc, TRANSPARENT);
                HFONT font = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                      ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
                HFONT oldFont = (HFONT)SelectObject(hdc, font);
                
                const char* text = "ARM64 Hypervisor - Virtual Machine Display";
                const char* subtext = "Start a VM to see graphics here";
                
                RECT textRect = displayRect;
                DrawText(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                
                textRect.top += 50;
                HFONT smallFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                           ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                           DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
                SelectObject(hdc, smallFont);
                SetTextColor(hdc, RGB(192, 192, 192));
                DrawText(hdc, subtext, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                
                SelectObject(hdc, oldFont);
                DeleteObject(font);
                DeleteObject(smallFont);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            
            if ((HWND)lParam == g_mainWindow.startButton) {
                LogMessage("Starting virtual machine...");
                UpdateStatusDisplay("Starting VM...");
                
                // Create and start VM in a separate thread
                if (CreateVirtualMachine(g_mainWindow.hypervisor) == 0) {
                    if (StartVM(g_mainWindow.hypervisor) == 0) {
                        UpdateStatusDisplay("VM Running - Graphics active");
                        InvalidateRect(hwnd, NULL, TRUE);
                    } else {
                        UpdateStatusDisplay("Failed to start VM");
                    }
                } else {
                    UpdateStatusDisplay("Failed to create VM");
                }
            }
            else if ((HWND)lParam == g_mainWindow.stopButton) {
                LogMessage("Stopping virtual machine...");
                StopVM(g_mainWindow.hypervisor);
                UpdateStatusDisplay("VM Stopped");
                InvalidateRect(hwnd, NULL, TRUE);
            }
            else if ((HWND)lParam == g_mainWindow.resetButton) {
                LogMessage("Resetting virtual machine...");
                StopVM(g_mainWindow.hypervisor);
                ResetVM(g_mainWindow.hypervisor);
                UpdateStatusDisplay("VM Reset - Ready to start");
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        }
        
        case WM_KEYDOWN:
        case WM_KEYUP:
            if (g_mainWindow.hypervisor && g_mainWindow.hypervisor->running) {
                ProcessKeyboardInput(g_mainWindow.hypervisor, wParam, lParam);
            }
            break;
            
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
            if (g_mainWindow.hypervisor && g_mainWindow.hypervisor->running) {
                ProcessMouseInput(g_mainWindow.hypervisor, uMsg, wParam, lParam);
            }
            break;
            
        case WM_SIZE:
            if (g_mainWindow.hypervisor) {
                // Resize graphics if needed
                UpdateDisplay(g_mainWindow.hypervisor);
            }
            break;
            
        case WM_VM_STATUS: {
            VM_STATUS status = (VM_STATUS)wParam;
            const char* statusText = "";
            
            switch (status) {
                case VM_STATUS_STOPPED: statusText = "VM Stopped"; break;
                case VM_STATUS_STARTING: statusText = "VM Starting..."; break;
                case VM_STATUS_RUNNING: statusText = "VM Running"; break;
                case VM_STATUS_PAUSED: statusText = "VM Paused"; break;
                case VM_STATUS_ERROR: statusText = "VM Error"; break;
            }
            
            UpdateStatusDisplay(statusText);
            break;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CreateControlButtons(HWND hwnd) {
    HFONT buttonFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    
    // Start button
    g_mainWindow.startButton = CreateWindow(
        "BUTTON", "Start VM",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        50, 600, 100, 30,
        hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    SendMessage(g_mainWindow.startButton, WM_SETFONT, (WPARAM)buttonFont, TRUE);
    
    // Stop button
    g_mainWindow.stopButton = CreateWindow(
        "BUTTON", "Stop VM",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        160, 600, 100, 30,
        hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    SendMessage(g_mainWindow.stopButton, WM_SETFONT, (WPARAM)buttonFont, TRUE);
    
    // Reset button
    g_mainWindow.resetButton = CreateWindow(
        "BUTTON", "Reset VM",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        270, 600, 100, 30,
        hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    SendMessage(g_mainWindow.resetButton, WM_SETFONT, (WPARAM)buttonFont, TRUE);
    
    // Status label
    g_mainWindow.statusLabel = CreateWindow(
        "STATIC", "Ready",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        50, 650, 500, 20,
        hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    SendMessage(g_mainWindow.statusLabel, WM_SETFONT, (WPARAM)buttonFont, TRUE);
}

void UpdateStatusDisplay(const char* status) {
    if (g_mainWindow.statusLabel) {
        SetWindowText(g_mainWindow.statusLabel, status);
    }
}

void LogMessage(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    printf("[GUI-HV] %s\n", buffer);
    
    // Could add to a log window here
}