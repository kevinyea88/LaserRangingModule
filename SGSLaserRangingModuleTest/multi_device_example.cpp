// Multi-device example demonstrating pool management
// Shows how multiple devices can be managed simultaneously

#include "../SGSLaserRangingModule/SGSLaserRangingModule.h"
#include <stdio.h>
#include <windows.h>
#include <process.h>

#define NUM_DEVICES 4

// Structure for passing data to threads
typedef struct {
    SGSLrmHandle handle;
    int deviceId;
    const char* portName;
    bool running;
} DeviceThreadData;

// Thread function for continuous measurement
unsigned __stdcall device_thread(void* param) {
    DeviceThreadData* data = (DeviceThreadData*)param;
    
    printf("[Device %d] Thread started for %s\n", data->deviceId, data->portName);
    
    // Simulate connection attempt
    SGSLrmStatus status = SGSLrm_Connect(data->handle, data->portName);
    
    if (status == SGS_LRM_SUCCESS) {
        printf("[Device %d] Connected to %s\n", data->deviceId, data->portName);
        
        // Configure device
        SGSLrm_SetRange(data->handle, 30);
        SGSLrm_SetResolution(data->handle, 1); // 1mm
        SGSLrm_SetFrequency(data->handle, 5);  // 5Hz
        
        // Simulate measurements
        int measurementCount = 0;
        while (data->running && measurementCount < 5) {
            double distance;
            status = SGSLrm_SingleMeasurement(data->handle, &distance);
            
            if (status == SGS_LRM_SUCCESS) {
                printf("[Device %d] Measurement %d: %.3f meters\n", 
                       data->deviceId, measurementCount + 1, distance);
            } else {
                printf("[Device %d] Measurement failed: %d\n", 
                       data->deviceId, status);
            }
            
            Sleep(1000); // Wait 1 second between measurements
            measurementCount++;
        }
        
        SGSLrm_Disconnect(data->handle);
        printf("[Device %d] Disconnected\n", data->deviceId);
    } else {
        printf("[Device %d] Failed to connect to %s (simulated)\n", 
               data->deviceId, data->portName);
        
        // Simulate some operations even without connection
        for (int i = 0; i < 3; i++) {
            printf("[Device %d] Simulating operation %d...\n", data->deviceId, i + 1);
            Sleep(500);
        }
    }
    
    printf("[Device %d] Thread ending\n", data->deviceId);
    return 0;
}

int main() {
    printf("========================================\n");
    printf("Multi-Device Pool Management Example\n");
    printf("========================================\n\n");
    
    printf("This example demonstrates:\n");
    printf("• Managing multiple devices simultaneously\n");
    printf("• Thread-safe operations with device pool\n");
    printf("• Automatic pool management\n");
    printf("• No initialization/finalization needed\n\n");
    
    // Create multiple device handles
    SGSLrmHandle handles[NUM_DEVICES];
    DeviceThreadData threadData[NUM_DEVICES];
    HANDLE threads[NUM_DEVICES];
    
    printf("Creating %d device handles...\n", NUM_DEVICES);
    
    for (int i = 0; i < NUM_DEVICES; i++) {
        SGSLrmStatus status = SGSLrm_CreateHandle(&handles[i]);
        
        if (status == SGS_LRM_SUCCESS) {
            printf("  ✓ Device %d handle created\n", i + 1);
            
            // Setup thread data
            threadData[i].handle = handles[i];
            threadData[i].deviceId = i + 1;
            threadData[i].running = true;
            
            // Assign different simulated COM ports
            switch (i) {
                case 0: threadData[i].portName = "COM3"; break;
                case 1: threadData[i].portName = "COM4"; break;
                case 2: threadData[i].portName = "COM5"; break;
                case 3: threadData[i].portName = "COM6"; break;
            }
        } else {
            printf("  ❌ Failed to create device %d handle: %d\n", i + 1, status);
            handles[i] = NULL;
        }
    }
    
    printf("\n");
    
    // Get version info
    int major, minor, patch;
    SGSLrm_GetVersion(&major, &minor, &patch);
    printf("Library Version: %d.%d.%d (Pool-based)\n\n", major, minor, patch);
    
    // Start threads for each device
    printf("Starting device threads...\n");
    for (int i = 0; i < NUM_DEVICES; i++) {
        if (handles[i]) {
            threads[i] = (HANDLE)_beginthreadex(NULL, 0, device_thread, 
                                                &threadData[i], 0, NULL);
            if (threads[i]) {
                printf("  ✓ Thread started for device %d\n", i + 1);
            }
        } else {
            threads[i] = NULL;
        }
    }
    
    printf("\nDevices are running in parallel...\n");
    printf("(Simulating measurements for 5 seconds)\n\n");
    
    // Wait for threads to complete
    Sleep(6000); // Let threads run for 6 seconds
    
    // Signal threads to stop
    for (int i = 0; i < NUM_DEVICES; i++) {
        threadData[i].running = false;
    }
    
    // Wait for all threads to finish
    printf("\nWaiting for threads to complete...\n");
    for (int i = 0; i < NUM_DEVICES; i++) {
        if (threads[i]) {
            WaitForSingleObject(threads[i], INFINITE);
            CloseHandle(threads[i]);
            printf("  ✓ Thread %d completed\n", i + 1);
        }
    }
    
    // Clean up handles
    printf("\nCleaning up device handles...\n");
    for (int i = 0; i < NUM_DEVICES; i++) {
        if (handles[i]) {
            SGSLrm_DestroyHandle(handles[i]);
            printf("  ✓ Device %d handle destroyed\n", i + 1);
        }
    }
    
    printf("\n========================================\n");
    printf("Multi-Device Example Completed!\n");
    printf("========================================\n\n");
    
    printf("Summary:\n");
    printf("• %d devices managed simultaneously\n", NUM_DEVICES);
    printf("• Each device ran in its own thread\n");
    printf("• Pool managed all devices automatically\n");
    printf("• No initialization or cleanup required\n");
    printf("• Thread-safe operations throughout\n\n");
    
    printf("Pool Benefits Demonstrated:\n");
    printf("✓ Multiple devices without heap allocation\n");
    printf("✓ Thread-safe concurrent operations\n");
    printf("✓ Automatic resource management\n");
    printf("✓ Clean, simple API\n");
    
    return 0;
}
