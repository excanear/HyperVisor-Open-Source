#include "gui_hypervisor.h"

// OS Loader implementation
int InitializeOSLoader(OS_LOADER* loader) {
    if (!loader) return -1;
    
    memset(loader, 0, sizeof(OS_LOADER));
    loader->type = OS_TYPE_NONE;
    
    LogMessage("OS Loader initialized");
    return 0;
}

int LoadGuestOS(GUI_HYPERVISOR* hv, const char* osPath, OS_TYPE type) {
    if (!hv || !osPath) return -1;
    
    LogMessage("Loading guest OS: %s (type=%d)", osPath, type);
    
    hv->osLoader.type = type;
    strncpy_s(hv->osLoader.bootPath, sizeof(hv->osLoader.bootPath), osPath, _TRUNCATE);
    
    // Open OS image file
    HANDLE fileHandle = CreateFileA(osPath, GENERIC_READ, FILE_SHARE_READ, 
                                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (fileHandle == INVALID_HANDLE_VALUE) {
        LogMessage("Failed to open OS image: %s", osPath);
        return -1;
    }
    
    // Get file size
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle, &fileSize)) {
        LogMessage("Failed to get OS image size");
        CloseHandle(fileHandle);
        return -1;
    }
    
    hv->osLoader.imageSize = fileSize.QuadPart;
    LogMessage("OS image size: %lld bytes", hv->osLoader.imageSize);
    
    // Allocate memory for OS image
    size_t allocSize = (size_t)min(hv->osLoader.imageSize, MAX_OS_SIZE);
    hv->osLoader.imageData = (uint8_t*)VirtualAlloc(NULL, allocSize, 
                                                   MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!hv->osLoader.imageData) {
        LogMessage("Failed to allocate memory for OS image");
        CloseHandle(fileHandle);
        return -1;
    }
    
    // Read OS image
    DWORD bytesRead;
    if (!ReadFile(fileHandle, hv->osLoader.imageData, (DWORD)allocSize, &bytesRead, NULL)) {
        LogMessage("Failed to read OS image");
        VirtualFree(hv->osLoader.imageData, 0, MEM_RELEASE);
        hv->osLoader.imageData = NULL;
        CloseHandle(fileHandle);
        return -1;
    }
    
    CloseHandle(fileHandle);
    LogMessage("OS image loaded successfully: %u bytes read", bytesRead);
    
    // Analyze OS image based on type
    switch (type) {
        case OS_TYPE_LINUX:
            return AnalyzeLinuxKernel(hv);
        case OS_TYPE_WINDOWS:
            return AnalyzeWindowsPE(hv);
        case OS_TYPE_ISO:
            return AnalyzeISOImage(hv);
        default:
            return AnalyzeRawBinary(hv);
    }
}

int AnalyzeLinuxKernel(GUI_HYPERVISOR* hv) {
    if (!hv || !hv->osLoader.imageData) return -1;
    
    LogMessage("Analyzing Linux kernel image...");
    
    // Check for Linux kernel magic
    if (hv->osLoader.imageSize > 0x200) {
        uint8_t* header = hv->osLoader.imageData;
        
        // Check for ARM64 Linux kernel header
        if (header[0x38] == 'A' && header[0x39] == 'R' && 
            header[0x3A] == 'M' && header[0x3B] == 0x64) {
            LogMessage("ARM64 Linux kernel detected");
            
            // Extract kernel load address
            uint64_t* textOffset = (uint64_t*)(header + 0x08);
            uint64_t* imageSize = (uint64_t*)(header + 0x10);
            
            hv->osLoader.entryPoint = 0x80000 + *textOffset;
            hv->osLoader.loadAddress = 0x80000;
            
            LogMessage("Kernel entry point: 0x%llX", hv->osLoader.entryPoint);
            LogMessage("Kernel load address: 0x%llX", hv->osLoader.loadAddress);
            LogMessage("Kernel image size: 0x%llX", *imageSize);
            
            return 0;
        }
    }
    
    LogMessage("Warning: Linux kernel header not found, using defaults");
    hv->osLoader.entryPoint = 0x80000;
    hv->osLoader.loadAddress = 0x80000;
    return 0;
}

int AnalyzeWindowsPE(GUI_HYPERVISOR* hv) {
    if (!hv || !hv->osLoader.imageData) return -1;
    
    LogMessage("Analyzing Windows PE image...");
    
    // Check DOS header
    if (hv->osLoader.imageSize < sizeof(IMAGE_DOS_HEADER)) {
        LogMessage("Image too small for DOS header");
        return -1;
    }
    
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)hv->osLoader.imageData;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        LogMessage("Invalid DOS signature");
        return -1;
    }
    
    // Check PE header
    if (hv->osLoader.imageSize < dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS64)) {
        LogMessage("Image too small for PE header");
        return -1;
    }
    
    IMAGE_NT_HEADERS64* ntHeaders = (IMAGE_NT_HEADERS64*)(hv->osLoader.imageData + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        LogMessage("Invalid PE signature");
        return -1;
    }
    
    // Check if ARM64
    if (ntHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_ARM64) {
        LogMessage("Warning: Not an ARM64 PE file (machine=0x%X)", ntHeaders->FileHeader.Machine);
    }
    
    hv->osLoader.entryPoint = ntHeaders->OptionalHeader.AddressOfEntryPoint;
    hv->osLoader.loadAddress = ntHeaders->OptionalHeader.ImageBase;
    
    LogMessage("PE entry point: 0x%llX", hv->osLoader.entryPoint);
    LogMessage("PE image base: 0x%llX", hv->osLoader.loadAddress);
    
    return 0;
}

int AnalyzeISOImage(GUI_HYPERVISOR* hv) {
    if (!hv || !hv->osLoader.imageData) return -1;
    
    LogMessage("Analyzing ISO 9660 image...");
    
    // Look for ISO 9660 volume descriptor at sector 16
    if (hv->osLoader.imageSize < (16 + 1) * 2048) {
        LogMessage("Image too small for ISO 9660");
        return -1;
    }
    
    uint8_t* volumeDescriptor = hv->osLoader.imageData + (16 * 2048);
    
    // Check ISO 9660 signature
    if (memcmp(volumeDescriptor + 1, "CD001", 5) == 0) {
        LogMessage("ISO 9660 filesystem detected");
        
        // For bootable ISOs, we'd need to parse the boot record
        // For now, set generic boot parameters
        hv->osLoader.entryPoint = 0x7C00;  // Standard boot sector address
        hv->osLoader.loadAddress = 0x7C00;
        
        return 0;
    }
    
    LogMessage("Warning: ISO 9660 signature not found");
    return -1;
}

int AnalyzeRawBinary(GUI_HYPERVISOR* hv) {
    if (!hv || !hv->osLoader.imageData) return -1;
    
    LogMessage("Analyzing raw binary image...");
    
    // For raw binaries, use default ARM64 boot parameters
    hv->osLoader.entryPoint = 0x80000;
    hv->osLoader.loadAddress = 0x80000;
    
    LogMessage("Using default ARM64 boot parameters");
    LogMessage("Entry point: 0x%llX", hv->osLoader.entryPoint);
    LogMessage("Load address: 0x%llX", hv->osLoader.loadAddress);
    
    return 0;
}

int LoadOSIntoGuestMemory(GUI_HYPERVISOR* hv) {
    if (!hv || !hv->osLoader.imageData || !hv->guestMemory) return -1;
    
    LogMessage("Loading OS into guest memory...");
    
    uint64_t loadAddr = hv->osLoader.loadAddress;
    size_t imageSize = (size_t)hv->osLoader.imageSize;
    
    // Ensure load address is within guest memory
    if (loadAddr >= hv->memorySize) {
        LogMessage("Load address 0x%llX exceeds guest memory size 0x%llX", 
                   loadAddr, hv->memorySize);
        return -1;
    }
    
    // Ensure image fits in guest memory
    if (loadAddr + imageSize > hv->memorySize) {
        imageSize = (size_t)(hv->memorySize - loadAddr);
        LogMessage("Truncating OS image to fit in guest memory: %zu bytes", imageSize);
    }
    
    // Copy OS image to guest memory
    memcpy((uint8_t*)hv->guestMemory + loadAddr, hv->osLoader.imageData, imageSize);
    
    LogMessage("OS loaded into guest memory at 0x%llX (%zu bytes)", loadAddr, imageSize);
    
    // Set initial CPU state for OS boot
    return SetupBootState(hv);
}

int SetupBootState(GUI_HYPERVISOR* hv) {
    if (!hv) return -1;
    
    LogMessage("Setting up boot state...");
    
    WHV_REGISTER_NAME regNames[] = {
        WHvArm64RegisterX0, WHvArm64RegisterX1, WHvArm64RegisterX2, WHvArm64RegisterX3,
        WHvArm64RegisterSp, WHvArm64RegisterPc, WHvArm64RegisterPstate
    };
    
    WHV_REGISTER_VALUE regValues[7] = {0};
    
    // Set boot parameters based on OS type
    switch (hv->osLoader.type) {
        case OS_TYPE_LINUX:
            // Linux ARM64 boot protocol
            regValues[0].Reg64 = 0;  // X0 = device tree address (0 for now)
            regValues[1].Reg64 = 0;  // X1 = reserved
            regValues[2].Reg64 = 0;  // X2 = reserved  
            regValues[3].Reg64 = 0;  // X3 = reserved
            regValues[4].Reg64 = 0x80000000;  // SP = initial stack
            regValues[5].Reg64 = hv->osLoader.entryPoint;  // PC = kernel entry
            regValues[6].Reg64 = 0x3C5;  // PSTATE = EL1h, interrupts disabled
            break;
            
        case OS_TYPE_WINDOWS:
            // Windows ARM64 boot
            regValues[0].Reg64 = 0;  // X0 = loader block
            regValues[1].Reg64 = 0;  // X1 = reserved
            regValues[2].Reg64 = 0;  // X2 = reserved
            regValues[3].Reg64 = 0;  // X3 = reserved
            regValues[4].Reg64 = 0x80000000;  // SP = initial stack
            regValues[5].Reg64 = hv->osLoader.entryPoint + hv->osLoader.loadAddress;  // PC
            regValues[6].Reg64 = 0x3C5;  // PSTATE = EL1h
            break;
            
        default:
            // Generic boot
            regValues[0].Reg64 = 0;
            regValues[1].Reg64 = 0;
            regValues[2].Reg64 = 0;
            regValues[3].Reg64 = 0;
            regValues[4].Reg64 = 0x80000000;  // SP
            regValues[5].Reg64 = hv->osLoader.entryPoint;  // PC
            regValues[6].Reg64 = 0x3C5;  // PSTATE
            break;
    }
    
    // Set registers
    HRESULT hr = WHvSetVirtualProcessorRegisters(
        hv->partition, 0, regNames, 7, regValues);
    
    if (FAILED(hr)) {
        LogMessage("Failed to set boot registers: 0x%08X", hr);
        return -1;
    }
    
    LogMessage("Boot state configured successfully");
    LogMessage("Entry point: 0x%llX", regValues[5].Reg64);
    LogMessage("Stack pointer: 0x%llX", regValues[4].Reg64);
    
    return 0;
}

int CreateDeviceTree(GUI_HYPERVISOR* hv) {
    if (!hv) return -1;
    
    LogMessage("Creating device tree for Linux boot...");
    
    // This would create a proper device tree blob
    // For now, just allocate space and set address
    
    size_t dtbSize = 4096;  // 4KB device tree
    uint64_t dtbAddr = 0x40000000;  // Load at 1GB
    
    if (dtbAddr + dtbSize > hv->memorySize) {
        LogMessage("Not enough memory for device tree");
        return -1;
    }
    
    // Zero out device tree area
    memset((uint8_t*)hv->guestMemory + dtbAddr, 0, dtbSize);
    
    // Create minimal device tree header
    uint32_t* dtbHeader = (uint32_t*)((uint8_t*)hv->guestMemory + dtbAddr);
    dtbHeader[0] = 0xD00DFEED;  // Magic number (big endian)
    dtbHeader[1] = _byteswap_ulong((uint32_t)dtbSize);  // Total size
    
    LogMessage("Device tree created at 0x%llX (%zu bytes)", dtbAddr, dtbSize);
    
    // Update X0 register with device tree address
    WHV_REGISTER_NAME regName = WHvArm64RegisterX0;
    WHV_REGISTER_VALUE regValue = {0};
    regValue.Reg64 = dtbAddr;
    
    HRESULT hr = WHvSetVirtualProcessorRegisters(hv->partition, 0, &regName, 1, &regValue);
    if (FAILED(hr)) {
        LogMessage("Failed to set device tree address: 0x%08X", hr);
        return -1;
    }
    
    return 0;
}

void CleanupOSLoader(OS_LOADER* loader) {
    if (!loader) return;
    
    if (loader->imageData) {
        VirtualFree(loader->imageData, 0, MEM_RELEASE);
        loader->imageData = NULL;
    }
    
    memset(loader, 0, sizeof(OS_LOADER));
    LogMessage("OS Loader cleaned up");
}