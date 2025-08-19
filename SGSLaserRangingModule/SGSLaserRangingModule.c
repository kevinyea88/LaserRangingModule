#define _SGS_LRM_EXPORT
#include "SGSLaserRangingModule.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#pragma comment(lib, "kernel32.lib")

// Protocol command bytes
#define CMD_BROADCAST_ADDR      0xFA
#define CMD_CONFIG              0x04
#define CMD_MEASURE             0x06

// Protocol sub-commands for configuration (0x04)
#define SUBCMD_SET_ADDRESS      0x01
#define SUBCMD_SHUTDOWN         0x02
#define SUBCMD_SET_INTERVAL     0x05
#define SUBCMD_SET_CORRECTION   0x06
#define SUBCMD_SET_POSITION     0x08
#define SUBCMD_SET_RANGE        0x09
#define SUBCMD_SET_FREQUENCY    0x0A
#define SUBCMD_SET_RESOLUTION   0x0C
#define SUBCMD_SET_AUTO_MEASURE 0x0D

// Protocol sub-commands for measurement (0x06)
#define SUBCMD_SINGLE_MEASURE   0x02
#define SUBCMD_CONTINUOUS       0x03
#define SUBCMD_READ_ID          0x04
#define SUBCMD_LASER_CONTROL    0x05
#define SUBCMD_BROADCAST_MEASURE 0x06
#define SUBCMD_READ_CACHE       0x07

// Response status bytes
#define RESP_SINGLE_MEASURE     0x82
#define RESP_CONTINUOUS         0x83
#define RESP_DEVICE_ID          0x84
#define RESP_LASER_CONTROL      0x85

// Hardware error codes as per protocol specification
#define SGS_LRM_ERR_LOW_BATTERY         -110    // ERR-10: Low battery
#define SGS_LRM_ERR_CALCULATION_ERROR   -114    // ERR-14: Calculation error  
#define SGS_LRM_ERR_OUT_OF_RANGE        -115    // ERR-15: Out of range
#define SGS_LRM_ERR_WEAK_SIGNAL         -116    // ERR-16: Weak signal or timeout
#define SGS_LRM_ERR_STRONG_LIGHT        -118    // ERR-18: Strong ambient light
#define SGS_LRM_ERR_DISPLAY_RANGE       -126    // ERR-26: Display range exceeded

// Maximum number of devices that can be managed simultaneously
#define MAX_DEVICES 16

// Internal data structures
typedef struct {
    HANDLE hSerial;
    char comPort[16];
    bool isConnected;
    bool inUse;  // Flag to indicate if this slot is in use
    int deviceAddress;
    int currentRange;
    int currentResolution;
    int currentFrequency;
    SGSLrm_MeasurementCallback callback;
    void* userdata;
    HANDLE continuousThread;
    bool continuousMeasurement;
    double lastDistance;
    bool laserOn; // Track laser status
    CRITICAL_SECTION lock;  // Per-device lock for thread safety
} SGSLrmDevice;

// Global device pool
static SGSLrmDevice g_devicePool[MAX_DEVICES];
static CRITICAL_SECTION g_poolLock;  // Lock for pool management
static bool g_poolInitialized = false;

// Internal function declarations
static SGSLrmStatus ValidateHandle(SGSLrmHandle handle);
static SGSLrmStatus SendCommand(SGSLrmDevice* device, const unsigned char* command, int commandLength);
static SGSLrmStatus ReceiveResponse(SGSLrmDevice* device, unsigned char* response, int maxLength, int* receivedLength);
static unsigned char CalculateChecksum(const unsigned char* data, int length);
static SGSLrmStatus ParseMeasurementResponse(SGSLrmDevice* device, const unsigned char* response, int length, double* distance);
static DWORD WINAPI ContinuousMeasurementThread(LPVOID lpParam);
static const char* GetCommandDescription(unsigned char cmd1, unsigned char cmd2);
static void InitializeDevicePool();
static void CleanupDevicePool();

// Initialize device pool on first use
static void InitializeDevicePool()
{
    if (!g_poolInitialized) {
        InitializeCriticalSection(&g_poolLock);
        
        // Initialize all device slots
        for (int i = 0; i < MAX_DEVICES; i++) {
            memset(&g_devicePool[i], 0, sizeof(SGSLrmDevice));
            g_devicePool[i].hSerial = INVALID_HANDLE_VALUE;
            g_devicePool[i].inUse = false;
            g_devicePool[i].isConnected = false;
            g_devicePool[i].deviceAddress = 0x80; // Default address
            g_devicePool[i].currentRange = 80;    // Default 80m
            g_devicePool[i].currentResolution = 1; // Default 1mm
            g_devicePool[i].currentFrequency = 5;  // Default 5Hz
            g_devicePool[i].continuousMeasurement = false;
            g_devicePool[i].lastDistance = 0.0;
            g_devicePool[i].laserOn = false;
            InitializeCriticalSection(&g_devicePool[i].lock);
        }
        
        g_poolInitialized = true;
    }
}

// Cleanup device pool (can be called at program exit if needed)
static void CleanupDevicePool()
{
    if (g_poolInitialized) {
        EnterCriticalSection(&g_poolLock);
        
        // Clean up all devices
        for (int i = 0; i < MAX_DEVICES; i++) {
            if (g_devicePool[i].inUse) {
                // Disconnect if connected
                if (g_devicePool[i].isConnected) {
                    // Stop continuous measurement if running
                    if (g_devicePool[i].continuousMeasurement) {
                        g_devicePool[i].continuousMeasurement = false;
                        if (g_devicePool[i].continuousThread != NULL) {
                            WaitForSingleObject(g_devicePool[i].continuousThread, INFINITE);
                            CloseHandle(g_devicePool[i].continuousThread);
                            g_devicePool[i].continuousThread = NULL;
                        }
                    }
                    
                    // Close serial port
                    if (g_devicePool[i].hSerial != INVALID_HANDLE_VALUE) {
                        CloseHandle(g_devicePool[i].hSerial);
                        g_devicePool[i].hSerial = INVALID_HANDLE_VALUE;
                    }
                }
                
                // Mark as not in use
                g_devicePool[i].inUse = false;
            }
            
            DeleteCriticalSection(&g_devicePool[i].lock);
        }
        
        LeaveCriticalSection(&g_poolLock);
        DeleteCriticalSection(&g_poolLock);
        g_poolInitialized = false;
    }
}

SGS_LRM_API SGSLrmStatus SGSLrm_GetVersion(int* major, int* minor, int* patch)
{
    if (!major || !minor || !patch) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    *major = 1;
    *minor = 0;
    *patch = 1;  // Updated for pool version
    return SGS_LRM_SUCCESS;
}

// Device management
SGS_LRM_API SGSLrmStatus SGSLrm_CreateHandle(SGSLrmHandle* handle)
{
    if (!handle) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    // Initialize pool on first use
    if (!g_poolInitialized) {
        InitializeDevicePool();
    }

    EnterCriticalSection(&g_poolLock);

    // Find an unused slot in the pool
    SGSLrmDevice* device = NULL;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!g_devicePool[i].inUse) {
            device = &g_devicePool[i];
            
            // Reset device state
            device->hSerial = INVALID_HANDLE_VALUE;
            device->isConnected = false;
            device->deviceAddress = 0x80; // Default address
            device->currentRange = 80;    // Default 80m
            device->currentResolution = 1; // Default 1mm
            device->currentFrequency = 5;  // Default 5Hz
            device->continuousMeasurement = false;
            device->lastDistance = 0.0;
            device->laserOn = false;
            device->callback = NULL;
            device->userdata = NULL;
            device->continuousThread = NULL;
            memset(device->comPort, 0, sizeof(device->comPort));
            
            // Mark as in use
            device->inUse = true;
            break;
        }
    }

    LeaveCriticalSection(&g_poolLock);

    if (device == NULL) {
        return SGS_LRM_OUT_OF_MEMORY;  // No available slots
    }

    *handle = (SGSLrmHandle)device;
    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_DestroyHandle(SGSLrmHandle handle)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    // Disconnect if still connected
    if (device->isConnected) {
        SGSLrm_Disconnect(handle);
    }

    EnterCriticalSection(&g_poolLock);
    
    // Mark slot as available
    device->inUse = false;
    
    LeaveCriticalSection(&g_poolLock);
    
    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_Connect(SGSLrmHandle handle, const char* comPort)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    if (!comPort) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_INVALID_PARAMETER;
    }

    // Open COM port
    device->hSerial = CreateFileA(comPort,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (device->hSerial == INVALID_HANDLE_VALUE) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    // Configure COM port (9600, 8, N, 1)
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(device->hSerial, &dcb)) {
        CloseHandle(device->hSerial);
        device->hSerial = INVALID_HANDLE_VALUE;
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;  // Binary mode is required
    dcb.fParity = FALSE;  // No parity checking

    if (!SetCommState(device->hSerial, &dcb)) {
        CloseHandle(device->hSerial);
        device->hSerial = INVALID_HANDLE_VALUE;
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    // Set timeouts
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 1000;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 1000;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(device->hSerial, &timeouts)) {
        CloseHandle(device->hSerial);
        device->hSerial = INVALID_HANDLE_VALUE;
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    strncpy_s(device->comPort, sizeof(device->comPort), comPort, _TRUNCATE);
    device->isConnected = true;

    LeaveCriticalSection(&device->lock);
    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_Disconnect(SGSLrmHandle handle)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_SUCCESS;
    }

    // Stop continuous measurement if running
    if (device->continuousMeasurement) {
        device->continuousMeasurement = false;
        if (device->continuousThread != NULL) {
            LeaveCriticalSection(&device->lock);
            WaitForSingleObject(device->continuousThread, INFINITE);
            CloseHandle(device->continuousThread);
            EnterCriticalSection(&device->lock);
            device->continuousThread = NULL;
        }
    }

    if (device->hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(device->hSerial);
        device->hSerial = INVALID_HANDLE_VALUE;
    }

    device->isConnected = false;
    device->comPort[0] = '\0';
    device->laserOn = false; // Reset laser status on disconnect

    LeaveCriticalSection(&device->lock);
    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_IsConnected(SGSLrmHandle handle, bool* connected)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    if (!connected) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);
    *connected = device->isConnected;
    LeaveCriticalSection(&device->lock);

    return SGS_LRM_SUCCESS;
}

// Internal helper functions
static SGSLrmStatus ValidateHandle(SGSLrmHandle handle)
{
    if (!handle) {
        return SGS_LRM_INVALID_HANDLE;
    }

    // Check if handle points to a valid device in the pool
    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    if (device < &g_devicePool[0] || device >= &g_devicePool[MAX_DEVICES]) {
        return SGS_LRM_INVALID_HANDLE;
    }

    // Check if the device slot is in use
    if (!device->inUse) {
        return SGS_LRM_INVALID_HANDLE;
    }

    return SGS_LRM_SUCCESS;
}

static unsigned char CalculateChecksum(const unsigned char* data, int length)
{
    unsigned char sum = 0;
    for (int i = 0; i < length; i++) {
        sum += data[i];
    }
    return (~sum) + 1; // Two's complement
}

static SGSLrmStatus SendCommand(SGSLrmDevice* device, const unsigned char* command, int commandLength)
{
    if (!device || !command || commandLength <= 0) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    if (!device->isConnected || device->hSerial == INVALID_HANDLE_VALUE) {
        return SGS_LRM_NOT_CONNECTED;
    }

    DWORD bytesWritten;
    if (!WriteFile(device->hSerial, command, commandLength, &bytesWritten, NULL)) {
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    if (bytesWritten != (DWORD)commandLength) {
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    return SGS_LRM_SUCCESS;
}

static SGSLrmStatus ReceiveResponse(SGSLrmDevice* device, unsigned char* response, int maxLength, int* receivedLength)
{
    if (!device || !response || maxLength <= 0 || !receivedLength) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    if (!device->isConnected || device->hSerial == INVALID_HANDLE_VALUE) {
        return SGS_LRM_NOT_CONNECTED;
    }

    DWORD bytesRead;
    if (!ReadFile(device->hSerial, response, maxLength, &bytesRead, NULL)) {
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    *receivedLength = (int)bytesRead;
    
    if (bytesRead == 0) {
        return SGS_LRM_TIMEOUT;
    }

    return SGS_LRM_SUCCESS;
}

static SGSLrmStatus ParseMeasurementResponse(SGSLrmDevice* device, const unsigned char* response, int length, double* distance)
{
    if (!device || !response || !distance || length < 4) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    // Protocol response formats:
    // Single measurement (1mm):     ADDR 06 82 "XXX.XXX" CS
    // Single measurement (0.1mm):   ADDR 06 82 "XXX.XXXX" CS  
    // Continuous measurement (1mm):  ADDR 06 83 "XXX.XXX" CS
    // Continuous measurement (0.1mm): ADDR 06 83 "XXX.XXXX" CS
    // Error response:                ADDR 06 8X "ERR-XX" CS

    // Verify device address
    if (response[0] != (unsigned char)device->deviceAddress) {
        return SGS_LRM_COMMUNICATION_ERROR; // Wrong address
    }

    // Verify command type
    if (response[1] != CMD_MEASURE) {
        return SGS_LRM_COMMUNICATION_ERROR; // Wrong command type
    }

    // Check if this is an error response
    // Error format: "ERR-XX" where XX is the error code
    if (length >= 9) { // Minimum for error: ADDR CMD STATUS 'E' 'R' 'R' '-' 'X' 'X' CS
        // Check for "ERR-" pattern starting at response[3]
        if (response[3] == 'E' && response[4] == 'R' && response[5] == 'R' && response[6] == '-') {
            // Extract error code digits
            char errorCode[3] = {0};
            errorCode[0] = response[7];
            errorCode[1] = response[8];
            
            int code = atoi(errorCode);
            // Map protocol error codes to API status codes
            switch (code) {
                case 10: return SGS_LRM_ERR_LOW_BATTERY;      // ERR-10: Low battery
                case 14: return SGS_LRM_ERR_CALCULATION_ERROR; // ERR-14: Calculation error
                case 15: return SGS_LRM_ERR_OUT_OF_RANGE;     // ERR-15: Out of range
                case 16: return SGS_LRM_ERR_WEAK_SIGNAL;      // ERR-16: Weak signal or timeout
                case 18: return SGS_LRM_ERR_STRONG_LIGHT;     // ERR-18: Strong ambient light
                case 26: return SGS_LRM_ERR_DISPLAY_RANGE;    // ERR-26: Display range exceeded
                default: return -100; // Generic hardware error for unknown codes
            }
        }
    }

    // Parse successful measurement response
    // Check for valid measurement response format
    if (response[2] == RESP_SINGLE_MEASURE || response[2] == RESP_CONTINUOUS) {
        // response[2] == 0x82: Single measurement response
        // response[2] == 0x83: Continuous measurement response
        
        // Extract ASCII distance data
        // Data starts from response[3] and excludes the last checksum byte
        int dataLength = length - 4; // Exclude ADDR, CMD, STATUS, and CS
        
        if (dataLength > 0 && dataLength < 16) {
            char distanceStr[16] = {0};
            
            // Copy ASCII distance data (e.g., "123.456" or "123.4567")
            memcpy(distanceStr, &response[3], dataLength);
            distanceStr[dataLength] = '\0';
            
            // Convert ASCII string to double
            char* endPtr;
            double parsedDistance = strtod(distanceStr, &endPtr);
            
            // Validate conversion
            if (endPtr != distanceStr && parsedDistance >= 0.0) {
                *distance = parsedDistance;
                return SGS_LRM_SUCCESS;
            }
        }
    }

    return SGS_LRM_COMMUNICATION_ERROR; // Invalid response format
}

SGS_LRM_API SGSLrmStatus SGSLrm_SingleMeasurement(SGSLrmHandle handle, double* distance)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    if (!distance) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Single measurement command: ADDR 06 02 CS
    unsigned char command[4];
    command[0] = (unsigned char)device->deviceAddress; // Default 0x80
    command[1] = CMD_MEASURE;
    command[2] = SUBCMD_SINGLE_MEASURE;
    command[3] = CalculateChecksum(command, 3);

    status = SendCommand(device, command, 4);
    if (status != SGS_LRM_SUCCESS) {
        LeaveCriticalSection(&device->lock);
        return status;
    }

    // Receive response
    unsigned char response[64];
    int receivedLength;
    status = ReceiveResponse(device, response, sizeof(response), &receivedLength);
    if (status != SGS_LRM_SUCCESS) {
        LeaveCriticalSection(&device->lock);
        return status;
    }

    // Verify checksum
    if (receivedLength < 4) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }
    
    unsigned char expectedChecksum = CalculateChecksum(response, receivedLength - 1);
    if (response[receivedLength - 1] != expectedChecksum) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    // Parse measurement result
    status = ParseMeasurementResponse(device, response, receivedLength, distance);
    if (status == SGS_LRM_SUCCESS) {
        device->lastDistance = *distance;
    }

    LeaveCriticalSection(&device->lock);
    return status;
}

static DWORD WINAPI ContinuousMeasurementThread(LPVOID lpParam)
{
    SGSLrmDevice* device = (SGSLrmDevice*)lpParam;
    if (!device) {
        return -1;
    }

    // Send continuous measurement command only once
    EnterCriticalSection(&device->lock);
    
    if (!device->isConnected || !device->continuousMeasurement) {
        LeaveCriticalSection(&device->lock);
        return 0;
    }

    // Send continuous measurement command: ADDR 06 03 CS
    unsigned char command[4];
    command[0] = (unsigned char)device->deviceAddress;
    command[1] = CMD_MEASURE;
    command[2] = SUBCMD_CONTINUOUS;
    command[3] = CalculateChecksum(command, 3);

    SGSLrmStatus status = SendCommand(device, command, 4);
    LeaveCriticalSection(&device->lock);

    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    // Continuously receive responses
    while (device->continuousMeasurement) {
        double distance = 0.0;
        status = SGS_LRM_SUCCESS;

        EnterCriticalSection(&device->lock);
        
        if (!device->isConnected) {
            LeaveCriticalSection(&device->lock);
            break;
        }

        // Receive response
        unsigned char response[64];
        int receivedLength;
        status = ReceiveResponse(device, response, sizeof(response), &receivedLength);
        
        if (status == SGS_LRM_SUCCESS && receivedLength >= 4) {
            // Verify checksum
            unsigned char expectedChecksum = CalculateChecksum(response, receivedLength - 1);
            if (response[receivedLength - 1] == expectedChecksum) {
                // Parse measurement result
                if (ParseMeasurementResponse(device, response, receivedLength, &distance) == SGS_LRM_SUCCESS) {
                    device->lastDistance = distance;
                }
            }
        }

        LeaveCriticalSection(&device->lock);

        // Call callback if set
        if (device->callback) {
            device->callback((SGSLrmHandle)device, distance, status, device->userdata);
        }

        // Wait for a short interval to avoid overwhelming the system
        Sleep(10); // 10ms interval for receiving data
    }

    return 0;
}

SGS_LRM_API SGSLrmStatus SGSLrm_StartContinuousMeasurement(SGSLrmHandle handle)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    if (device->continuousMeasurement) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_INVALID_PARAMETER; // Already running
    }

    device->continuousMeasurement = true;
    
    // Create measurement thread
    device->continuousThread = CreateThread(
        NULL,                           // Default security attributes
        0,                              // Default stack size
        ContinuousMeasurementThread,    // Thread function
        device,                         // Thread parameter
        0,                              // Default creation flags
        NULL                            // Don't need thread ID
    );

    if (device->continuousThread == NULL) {
        device->continuousMeasurement = false;
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    LeaveCriticalSection(&device->lock);
    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_StopContinuousMeasurement(SGSLrmHandle handle)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->continuousMeasurement) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_SUCCESS; // Not running
    }

    device->continuousMeasurement = false;
    
    if (device->continuousThread != NULL) {
        LeaveCriticalSection(&device->lock);
        
        // Wait for thread to finish
        WaitForSingleObject(device->continuousThread, INFINITE);
        CloseHandle(device->continuousThread);
        
        EnterCriticalSection(&device->lock);
        device->continuousThread = NULL;
    }

    LeaveCriticalSection(&device->lock);
    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_SetRange(SGSLrmHandle handle, int rangeMeters)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    // Validate range values (5, 10, 30, 50, 80 meters) according to protocol
    unsigned char rangeValue;
    switch (rangeMeters) {
    case 5:  rangeValue = 0x05; break;  // 5m
    case 10: rangeValue = 0x0A; break;  // 10m  
    case 30: rangeValue = 0x1E; break;  // 30m
    case 50: rangeValue = 0x32; break;  // 50m
    case 80: rangeValue = 0x50; break;  // 80m
    default: return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Set range command: FA 04 09 RANGE CS (correct protocol format)
    unsigned char command[5];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_CONFIG;
    command[2] = SUBCMD_SET_RANGE;
    command[3] = rangeValue;
    command[4] = CalculateChecksum(command, 4);

    status = SendCommand(device, command, 5);
    if (status == SGS_LRM_SUCCESS) {
        device->currentRange = rangeMeters;
    }

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_SetResolution(SGSLrmHandle handle, int resolution)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    // Validate resolution values as per protocol specification
    // Protocol: Resolution 1 = 1mm, Resolution 2 = 0.1mm
    if (resolution != 1 && resolution != 2) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Set resolution command: FA 04 0C RESOLUTION CS
    // RESOLUTION: 0x01 for 1mm, 0x02 for 0.1mm
    unsigned char command[5];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_CONFIG;
    command[2] = SUBCMD_SET_RESOLUTION;
    command[3] = (unsigned char)resolution; // Direct mapping: 1->0x01, 2->0x02
    command[4] = CalculateChecksum(command, 4);

    status = SendCommand(device, command, 5);
    if (status == SGS_LRM_SUCCESS) {
        device->currentResolution = resolution;
    }

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_SetFrequency(SGSLrmHandle handle, int frequency)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    // Validate and map frequency values according to protocol specification
    unsigned char freqValue;
    switch (frequency) {
    case 3:  freqValue = 0x00; break;  // 3Hz (minimum frequency)
    case 5:  freqValue = 0x05; break;  // 5Hz
    case 10: freqValue = 0x0A; break;  // 10Hz  
    case 20: freqValue = 0x14; break;  // 20Hz (maximum frequency)
    default: return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Set frequency command: FA 04 0A FREQ CS (correct protocol format)
    unsigned char command[5];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_CONFIG;
    command[2] = SUBCMD_SET_FREQUENCY;
    command[3] = freqValue;
    command[4] = CalculateChecksum(command, 4);

    status = SendCommand(device, command, 5);
    if (status == SGS_LRM_SUCCESS) {
        device->currentFrequency = frequency;
    }

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_SetMeasurementInterval(SGSLrmHandle handle, int intervalMs)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    // Convert milliseconds to seconds and validate according to protocol
    // Protocol supports: 0S (continuous) and 1S intervals as per specification
    unsigned char intervalValue;
    if (intervalMs == 0) {
        intervalValue = 0x00; // 0S - continuous measurement
    } else if (intervalMs >= 1000) {
        intervalValue = 0x01; // 1S - 1 second interval (minimum discrete interval)
    } else {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Set measurement interval command: FA 04 05 INTERVAL CS
    // INTERVAL: 00=continuous (0S), 01=1 second interval as per protocol specification
    unsigned char command[5];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_CONFIG;
    command[2] = SUBCMD_SET_INTERVAL;
    command[3] = intervalValue; // 0x00 or 0x01 based on protocol
    command[4] = CalculateChecksum(command, 4);

    status = SendCommand(device, command, 5);
    // Note: Do not update currentFrequency here as interval and frequency are separate concepts

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_GetLastMeasurement(SGSLrmHandle handle, double* distance)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    if (!distance) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);
    *distance = device->lastDistance;
    LeaveCriticalSection(&device->lock);

    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_LaserOn(SGSLrmHandle handle)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Laser on command: ADDR 06 05 01 CS
    unsigned char command[5];
    command[0] = (unsigned char)device->deviceAddress;
    command[1] = CMD_MEASURE;
    command[2] = SUBCMD_LASER_CONTROL;
    command[3] = 0x01; // ON
    command[4] = CalculateChecksum(command, 4);

    status = SendCommand(device, command, 5);
    if (status == SGS_LRM_SUCCESS) {
        device->laserOn = true; // Update laser status on successful command
    }

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_LaserOff(SGSLrmHandle handle)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Laser off command: ADDR 06 05 00 CS
    unsigned char command[5];
    command[0] = (unsigned char)device->deviceAddress;
    command[1] = CMD_MEASURE;
    command[2] = SUBCMD_LASER_CONTROL;
    command[3] = 0x00; // OFF
    command[4] = CalculateChecksum(command, 4);

    status = SendCommand(device, command, 5);
    if (status == SGS_LRM_SUCCESS) {
        device->laserOn = false; // Update laser status on successful command
    }

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_SetAddress(SGSLrmHandle handle, int address)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    if (address < 0 || address > 255) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Set address command: FA 04 01 ADDR CS
    unsigned char command[5];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_CONFIG;
    command[2] = SUBCMD_SET_ADDRESS;
    command[3] = (unsigned char)address;
    command[4] = CalculateChecksum(command, 4);

    status = SendCommand(device, command, 5);
    if (status == SGS_LRM_SUCCESS) {
        device->deviceAddress = address;
    }

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_EnumComPorts(char* portList, int bufferSize)
{
    if (!portList || bufferSize <= 0) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    portList[0] = '\0'; // Initialize empty string
    int currentPos = 0;
    
    for (int i = 1; i <= 256; i++) {
        char portName[16];
        sprintf_s(portName, sizeof(portName), "COM%d", i);
        
        HANDLE hPort = CreateFileA(portName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
            
        if (hPort != INVALID_HANDLE_VALUE) {
            CloseHandle(hPort);
            
            // Add port to list
            int portNameLen = (int)strlen(portName);
            if (currentPos + portNameLen + 2 < bufferSize) // +2 for ';' and '\0'
            {
                if (currentPos > 0) {
                    strcat_s(portList, bufferSize, ";");
                    currentPos++;
                }
                strcat_s(portList, bufferSize, portName);
                currentPos += portNameLen;
            }
        }
    }
    
    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_GetLaserStatus(SGSLrmHandle handle, bool* isOn)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    if (!isOn) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Return the tracked laser status instead of default assumption
    *isOn = device->laserOn;

    LeaveCriticalSection(&device->lock);
    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_SetMeasurementCallback(SGSLrmHandle handle, SGSLrm_MeasurementCallback callback, void* userdata)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);
    device->callback = callback;
    device->userdata = userdata;
    LeaveCriticalSection(&device->lock);

    return SGS_LRM_SUCCESS;
}

SGS_LRM_API SGSLrmStatus SGSLrm_SetDistanceCorrection(SGSLrmHandle handle, int correctionMm)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    // Validate correction range (-255 to +255 mm)
    if (correctionMm < -255 || correctionMm > 255) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Distance correction command: FA 04 06 SIGN VALUE CS
    unsigned char command[5];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_CONFIG;
    command[2] = SUBCMD_SET_CORRECTION;
    
    if (correctionMm < 0) {
        command[3] = 0x2D; // Negative sign (-)
        command[4] = (unsigned char)(-correctionMm); // Absolute value
    } else {
        command[3] = 0x2B; // Positive sign (+)  
        command[4] = (unsigned char)correctionMm;
    }
    
    // This is actually a 6-byte command: FA 04 06 SIGN VALUE CS
    unsigned char fullCommand[6];
    fullCommand[0] = CMD_BROADCAST_ADDR;
    fullCommand[1] = CMD_CONFIG; 
    fullCommand[2] = SUBCMD_SET_CORRECTION;
    fullCommand[3] = command[3]; // Sign
    fullCommand[4] = command[4]; // Value
    fullCommand[5] = CalculateChecksum(fullCommand, 5); // Checksum

    status = SendCommand(device, fullCommand, 6);

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_SetStartPosition(SGSLrmHandle handle, int position)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    // Validate position (0=tail, 1=top)
    if (position != 0 && position != 1) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Set start position command: FA 04 08 POSITION CS
    unsigned char command[5];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_CONFIG;
    command[2] = SUBCMD_SET_POSITION;
    command[3] = (unsigned char)position; // 0=tail, 1=top
    command[4] = CalculateChecksum(command, 4);

    status = SendCommand(device, command, 5);

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_SetAutoMeasurement(SGSLrmHandle handle, int enable)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    // Validate enable flag (0=disable, 1=enable)
    if (enable != 0 && enable != 1) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Set auto measurement command: FA 04 0D ENABLE CS
    unsigned char command[5];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_CONFIG;
    command[2] = SUBCMD_SET_AUTO_MEASURE;
    command[3] = (unsigned char)enable; // 0=disable, 1=enable
    command[4] = CalculateChecksum(command, 4);

    status = SendCommand(device, command, 5);

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_BroadcastMeasurement(SGSLrmHandle handle)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Broadcast measurement command: FA 06 06 FA (no response, result stored in module cache)
    unsigned char command[4];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_MEASURE;
    command[2] = SUBCMD_BROADCAST_MEASURE;
    command[3] = 0xFA; // Fixed checksum as per protocol

    status = SendCommand(device, command, 4);

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_ReadCache(SGSLrmHandle handle, double* distance)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    if (!distance) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Read cache command: ADDR 06 07 CS
    unsigned char command[4];
    command[0] = (unsigned char)device->deviceAddress; // Device address
    command[1] = CMD_MEASURE;
    command[2] = SUBCMD_READ_CACHE;
    command[3] = CalculateChecksum(command, 3);

    status = SendCommand(device, command, 4);
    if (status != SGS_LRM_SUCCESS) {
        LeaveCriticalSection(&device->lock);
        return status;
    }

    // Receive response
    unsigned char response[64];
    int receivedLength;
    status = ReceiveResponse(device, response, sizeof(response), &receivedLength);
    if (status != SGS_LRM_SUCCESS) {
        LeaveCriticalSection(&device->lock);
        return status;
    }

    // Verify checksum
    if (receivedLength < 4) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }
    
    unsigned char expectedChecksum = CalculateChecksum(response, receivedLength - 1);
    if (response[receivedLength - 1] != expectedChecksum) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    // Parse measurement result (same format as single measurement)
    status = ParseMeasurementResponse(device, response, receivedLength, distance);
    if (status == SGS_LRM_SUCCESS) {
        device->lastDistance = *distance;
    }

    LeaveCriticalSection(&device->lock);
    return status;
}

SGS_LRM_API SGSLrmStatus SGSLrm_ReadDeviceID(SGSLrmHandle handle, char* deviceId, int bufferSize)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    if (!deviceId || bufferSize <= 0) {
        return SGS_LRM_INVALID_PARAMETER;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Read machine ID command: FA 06 04 FC
    // Protocol specifies fixed checksum 0xFC for this command
    unsigned char command[4];
    command[0] = CMD_BROADCAST_ADDR;
    command[1] = CMD_MEASURE;
    command[2] = SUBCMD_READ_ID;
    command[3] = 0xFC; // Fixed checksum as per protocol

    status = SendCommand(device, command, 4);
    if (status != SGS_LRM_SUCCESS) {
        LeaveCriticalSection(&device->lock);
        return status;
    }

    // Receive response
    // Expected format: FA 06 84 "DAT1 DAT2...DAT16" CS
    // DATn are in ASCII format
    unsigned char response[64];
    int receivedLength;
    status = ReceiveResponse(device, response, sizeof(response), &receivedLength);
    if (status != SGS_LRM_SUCCESS) {
        LeaveCriticalSection(&device->lock);
        return status;
    }

    // Verify minimum response length
    if (receivedLength < 5) { // At least FA 06 84 [1 byte data] CS
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    // Verify checksum
    unsigned char expectedChecksum = CalculateChecksum(response, receivedLength - 1);
    if (response[receivedLength - 1] != expectedChecksum) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    // Parse device ID response
    // Response format: FA 06 84 [ASCII DATA...] CS
    if (response[0] == CMD_BROADCAST_ADDR && response[1] == CMD_MEASURE && response[2] == RESP_DEVICE_ID) {
        // Extract device ID data (excluding FA, 06, 84, and CS)
        int dataLength = receivedLength - 4; // Subtract header (3 bytes) and checksum (1 byte)
        if (dataLength > 0 && dataLength < bufferSize) {
            // Copy ASCII device ID data starting from response[3]
            memcpy(deviceId, &response[3], dataLength);
            deviceId[dataLength] = '\0'; // Null-terminate the string
            
            LeaveCriticalSection(&device->lock);
            return SGS_LRM_SUCCESS;
        } else if (dataLength >= bufferSize) {
            LeaveCriticalSection(&device->lock);
            return SGS_LRM_INVALID_PARAMETER; // Buffer too small
        }
    }

    LeaveCriticalSection(&device->lock);
    return SGS_LRM_COMMUNICATION_ERROR;
}

// Helper function for debugging and maintenance
static const char* GetCommandDescription(unsigned char cmd1, unsigned char cmd2)
{
    // Protocol command reference
    if (cmd1 == CMD_BROADCAST_ADDR) { // 0xFA
        if (cmd2 == CMD_CONFIG) { // 0x04
            return "Broadcast configuration command";
        } else if (cmd2 == CMD_MEASURE) { // 0x06
            return "Broadcast measurement/query command";
        }
        return "Unknown broadcast command";
    } else {
        // Device-specific address (typically 0x80)
        if (cmd2 == CMD_CONFIG) { // 0x04
            return "Device control/configuration command";
        } else if (cmd2 == CMD_MEASURE) { // 0x06
            return "Device measurement command";
        }
        return "Unknown device command";
    }
}

SGS_LRM_API SGSLrmStatus SGSLrm_Shutdown(SGSLrmHandle handle)
{
    SGSLrmStatus status = ValidateHandle(handle);
    if (status != SGS_LRM_SUCCESS) {
        return status;
    }

    SGSLrmDevice* device = (SGSLrmDevice*)handle;
    
    EnterCriticalSection(&device->lock);

    if (!device->isConnected) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_NOT_CONNECTED;
    }

    // Shutdown command: ADDR 04 02 CS
    unsigned char command[4];
    command[0] = (unsigned char)device->deviceAddress; // Device address
    command[1] = CMD_CONFIG;
    command[2] = SUBCMD_SHUTDOWN;
    command[3] = CalculateChecksum(command, 3);

    status = SendCommand(device, command, 4);
    if (status != SGS_LRM_SUCCESS) {
        LeaveCriticalSection(&device->lock);
        return status;
    }

    // Receive response (expected: ADDR 04 82 CS as per protocol)
    unsigned char response[64];
    int receivedLength;
    status = ReceiveResponse(device, response, sizeof(response), &receivedLength);
    if (status != SGS_LRM_SUCCESS) {
        LeaveCriticalSection(&device->lock);
        return status;
    }

    // Verify response
    if (receivedLength < 4) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    // Verify checksum
    unsigned char expectedChecksum = CalculateChecksum(response, receivedLength - 1);
    if (response[receivedLength - 1] != expectedChecksum) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_COMMUNICATION_ERROR;
    }

    // Check if response matches expected format: ADDR 04 82 CS
    if (response[0] == (unsigned char)device->deviceAddress && 
        response[1] == 0x04 && 
        response[2] == 0x82) {
        LeaveCriticalSection(&device->lock);
        return SGS_LRM_SUCCESS;
    }

    LeaveCriticalSection(&device->lock);
    return SGS_LRM_COMMUNICATION_ERROR;
}
