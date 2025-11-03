#ifndef GUI_HYPERVISOR_H
#define GUI_HYPERVISOR_H

#include <windows.h>
#include <winhvplatform.h>
#include <d3d11.h>
#include <dxgi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Estruturas do hypervisor GUI
typedef struct {
    WHV_PARTITION_HANDLE partition;
    WHV_VPINDEX vpindex;
    HANDLE vmThread;
    BOOL running;
    
    // Graphics
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* d3dContext;
    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* renderTarget;
    
    // Guest framebuffer
    uint32_t* framebuffer;
    uint32_t screenWidth;
    uint32_t screenHeight;
    uint32_t framebufferSize;
    
    // Input
    POINT lastMousePos;
    uint32_t keyboardState[256];
    
    // VM Memory
    void* guestMemory;
    uint64_t memorySize;
    uint64_t guestPhysicalBase;
} GUI_HYPERVISOR;

typedef struct {
    HWND hwnd;
    GUI_HYPERVISOR* hypervisor;
    BOOL fullscreen;
    
    // Window controls
    HWND startButton;
    HWND stopButton;
    HWND resetButton;
    HWND statusLabel;
} HYPERVISOR_WINDOW;

// Device emulation structures
typedef struct {
    uint32_t* vram;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint64_t framebufferAddr;
    BOOL dirty;
} VIRTUAL_GPU;

typedef struct {
    uint8_t scancode;
    uint8_t keycode;
    BOOL pressed;
    uint32_t modifiers;
} VIRTUAL_KEYBOARD;

typedef struct {
    int32_t x, y;
    int32_t deltaX, deltaY;
    uint8_t buttons;
    int32_t wheel;
} VIRTUAL_MOUSE;

// Function declarations
int CreateGUIHypervisor(HYPERVISOR_WINDOW* window);
int InitializeGraphics(GUI_HYPERVISOR* hv, HWND hwnd);
int CreateVirtualMachine(GUI_HYPERVISOR* hv);
int LoadGuestOS(GUI_HYPERVISOR* hv, const char* isoPath);

// Graphics functions
int InitializeVirtualGPU(VIRTUAL_GPU* gpu, uint32_t width, uint32_t height);
int RenderGuestFramebuffer(GUI_HYPERVISOR* hv);
int UpdateDisplay(GUI_HYPERVISOR* hv);

// Input handling
int ProcessKeyboardInput(GUI_HYPERVISOR* hv, WPARAM wParam, LPARAM lParam);
int ProcessMouseInput(GUI_HYPERVISOR* hv, UINT message, WPARAM wParam, LPARAM lParam);

// VM lifecycle
int StartVM(GUI_HYPERVISOR* hv);
int StopVM(GUI_HYPERVISOR* hv);
int ResetVM(GUI_HYPERVISOR* hv);

// Device emulation
int EmulateVGADevice(GUI_HYPERVISOR* hv, uint64_t gpa, uint32_t size, BOOL write, void* data);
int EmulateKeyboardController(GUI_HYPERVISOR* hv, uint64_t port, uint32_t size, BOOL write, void* data);
int EmulateMouseController(GUI_HYPERVISOR* hv, uint64_t port, uint32_t size, BOOL write, void* data);

// Boot and BIOS
int SetupGuestBIOS(GUI_HYPERVISOR* hv);
int LoadBootSector(GUI_HYPERVISOR* hv, const char* bootFile);

// Constants
#define GUEST_MEMORY_SIZE (512 * 1024 * 1024)  // 512MB
#define GUEST_PHYSICAL_BASE 0x0
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define FRAMEBUFFER_ADDR 0xA0000
#define VGA_MEMORY_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT * 4)

// VGA registers
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5

// Keyboard/Mouse ports
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define MOUSE_DATA_PORT 0x60
#define MOUSE_STATUS_PORT 0x64

// Window messages
#define WM_VM_STATUS (WM_USER + 1)
#define WM_VM_OUTPUT (WM_USER + 2)

// VM Status
typedef enum {
    VM_STATUS_STOPPED,
    VM_STATUS_STARTING,
    VM_STATUS_RUNNING,
    VM_STATUS_PAUSED,
    VM_STATUS_ERROR
} VM_STATUS;

#endif // GUI_HYPERVISOR_H