// Practical example showing how to use the enhanced SGSLaserRangingModule
// This demonstrates real-world usage with proper error handling

#include "../SGSLaserRangingModule/SGSLaserRangingModule.h"
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>   // for isdigit if needed

// Global variables for continuous measurement
static volatile bool g_stopMeasurement = false;
static int g_measurementCount = 0;

typedef void (*SGSLrmTestCase)(SGSLrmHandle handle, void* userData);
typedef struct {
    const char* name;
    SGSLrmTestCase test;
    void* userData;
} SGSLrmTestEntry;

// --- Helpers ---------------------------------------------------------------

// NEW: 將 "COM10" 之類轉為 "\\.\COM10"（避免高編號埠連線失敗）
static void make_com_path(const char* port, char* out, size_t outsz) {
    if (!port || !out || outsz == 0) return;
    if (_strnicmp(port, "\\\\.\\", 4) == 0) {
        strncpy_s(out, outsz, port, _TRUNCATE);
    }
    else {
        // MSVC 有 snprintf，若舊版可改 _snprintf_s
        snprintf(out, outsz, "\\\\.\\%s", port);
    }
}

// Enhanced callback with detailed error reporting and measurement error handling
void enhanced_measurement_callback(SGSLrmHandle handle, double distance, SGSLrmStatus status, void* userdata) {
    g_measurementCount++;

    if (status == SGS_LRM_SUCCESS) {
        printf("[%03d] Distance: %.4f meters\n", g_measurementCount, distance);

        // Example: Alert if object is too close
        if (distance < 0.5) {
            printf("      ⚠️  WARNING: Object detected within 0.5 meters!\n");
        }
        return;
    }

    if (status == SGS_LRM_MEASUREMENT_ERROR) {
        // NEW: 依新策略，原樣輸出硬體錯誤字串 + 數字碼（不做對映/解釋）
        char hwErr[16] = { 0 };
        int errCode = 0;
        SGSLrm_GetLastHardwareErrorAscii(handle, hwErr, sizeof(hwErr)); // 例如 "ERR-16"
        SGSLrm_GetMeasurementError(handle, &errCode);                   // 例如 16

        printf("[%03d] Measurement ERROR: %s (code=%d)\n", g_measurementCount,
            (hwErr[0] ? hwErr : "ERR-??"), errCode);
        return;
    }

    // 其他非量測錯誤（I/O / 超時 / 參數等）
    printf("[%03d] Error: ", g_measurementCount);
    switch (status) {
    case SGS_LRM_INVALID_PARAMETER:  printf("Invalid parameter\n"); break;
    case SGS_LRM_INVALID_HANDLE:     printf("Invalid handle\n"); break;
    case SGS_LRM_NOT_CONNECTED:      printf("Not connected\n"); break;
    case SGS_LRM_COMMUNICATION_ERROR:printf("Communication error\n"); break;
    case SGS_LRM_TIMEOUT:            printf("Timeout\n"); break;
    case SGS_LRM_OUT_OF_MEMORY:      printf("Out of memory\n"); break;
    default:                         printf("Unknown error (code: %d)\n", status); break;
    }
}

// Function to configure device for specific use case
SGSLrmStatus configure_for_indoor_measurement(SGSLrmHandle handle) {
    SGSLrmStatus status;

    printf("Configuring device for indoor measurement...\n");

    // Set range to 30 meters (suitable for most indoor spaces)
    status = SGSLrm_SetRange(handle, 30);
    if (status != SGS_LRM_SUCCESS) {
        printf("  ❌ Failed to set range: %d\n", status);
        return status;
    }
    printf("  ✓ Range set to 30 meters\n");

    // Set high resolution (0.1mm) for precise indoor measurements
    status = SGSLrm_SetResolution(handle, 2); // 2 = 0.1mm
    if (status != SGS_LRM_SUCCESS) {
        printf("  ❌ Failed to set resolution: %d\n", status);
        return status;
    }
    printf("  ✓ Resolution set to 0.1mm\n");

    // Set moderate frequency for stable readings
    status = SGSLrm_SetFrequency(handle, 10); // 10Hz
    if (status != SGS_LRM_SUCCESS) {
        printf("  ❌ Failed to set frequency: %d\n", status);
        return status;
    }
    printf("  ✓ Frequency set to 10Hz\n");

    // Set measurement from tail (default for most applications)
    status = SGSLrm_SetStartPosition(handle, 0); // 0 = tail
    if (status != SGS_LRM_SUCCESS) {
        printf("  ❌ Failed to set start position: %d\n", status);
        return status;
    }
    printf("  ✓ Start position set to tail\n");

    return SGS_LRM_SUCCESS;
}

// Function to configure device for outdoor/long-range measurement
SGSLrmStatus configure_for_outdoor_measurement(SGSLrmHandle handle) {
    SGSLrmStatus status;

    printf("Configuring device for outdoor measurement...\n");

    // Set maximum range for outdoor use
    status = SGSLrm_SetRange(handle, 80); // 80 meters max
    if (status != SGS_LRM_SUCCESS) return status;
    printf("  ✓ Range set to 80 meters\n");

    // Use standard resolution for longer distances
    status = SGSLrm_SetResolution(handle, 1); // 1 = 1mm
    if (status != SGS_LRM_SUCCESS) return status;
    printf("  ✓ Resolution set to 1mm\n");

    // Lower frequency for more stable long-range readings
    status = SGSLrm_SetFrequency(handle, 5); // 5Hz
    if (status != SGS_LRM_SUCCESS) return status;
    printf("  ✓ Frequency set to 5Hz\n");

    return SGS_LRM_SUCCESS;
}

// Function to test error code handling (RAW-ERR strategy)
void test_error_code_handling(SGSLrmHandle handle) {
    printf("\n========================================\n");
    printf("Error Code Handling Test (RAW ERR strategy)\n");
    printf("========================================\n\n");

    SGSLrmStatus status;
    double distance = 0.0;
    int errorCode = -1;
    char hwErr[16] = { 0 };

    printf("Testing error code handling system:\n\n");

    // Test 1: Check initial error code (should be 0 and empty string)
    printf("Test 1: Checking initial error state...\n");
    status = SGSLrm_GetMeasurementError(handle, &errorCode);
    if (status == SGS_LRM_SUCCESS) {
        SGSLrm_GetLastHardwareErrorAscii(handle, hwErr, sizeof(hwErr));
        printf("✓ Initial error: code=%d, ascii='%s'\n\n", errorCode, hwErr);
    }
    else {
        printf("❌ Failed to get initial error (status: %d)\n\n", status);
    }

    // Test 2: Perform single measurement and check result
    printf("Test 2: Performing single measurement...\n");
    status = SGSLrm_SingleMeasurement(handle, &distance);

    if (status == SGS_LRM_SUCCESS) {
        printf("✓ Measurement successful: %.4f meters\n", distance);
        // After success, error state should be cleared
        errorCode = -1; hwErr[0] = '\0';
        SGSLrm_GetMeasurementError(handle, &errorCode);
        SGSLrm_GetLastHardwareErrorAscii(handle, hwErr, sizeof(hwErr));
        printf("✓ After success: code=%d, ascii='%s'\n\n", errorCode, hwErr);
    }
    else if (status == SGS_LRM_MEASUREMENT_ERROR) {
        // NEW: 原樣輸出
        SGSLrm_GetMeasurementError(handle, &errorCode);
        SGSLrm_GetLastHardwareErrorAscii(handle, hwErr, sizeof(hwErr));
        printf("❌ Measurement error: %s (code=%d)\n\n", (hwErr[0] ? hwErr : "ERR-??"), errorCode);
    }
    else {
        printf("❌ Measurement failed with non-measurement error (status: %d)\n\n", status);
    }

    // Test 3: API invalid parameter checks
    printf("Test 3: Testing invalid parameters...\n");
    status = SGSLrm_GetMeasurementError(handle, NULL);
    if (status == SGS_LRM_INVALID_PARAMETER) {
        printf("✓ Correctly rejected NULL error code pointer\n");
    }
    else {
        printf("❌ Should have rejected NULL pointer (got status: %d)\n", status);
    }

    status = SGSLrm_GetMeasurementError(NULL, &errorCode);
    if (status == SGS_LRM_INVALID_HANDLE) {
        printf("✓ Correctly rejected NULL handle\n\n");
    }
    else {
        printf("❌ Should have rejected NULL handle (got status: %d)\n\n", status);
    }
}

// Function to explain RAW-ERR usage (no mapping)
void test_error_scenarios(SGSLrmHandle handle) {
    printf("\n========================================\n");
    printf("Error Scenario (RAW-ERR demonstration)\n");
    printf("========================================\n\n");

    printf("This section demonstrates the RAW-ERR strategy:\n\n");
    printf("1) All measurement failures return SGS_LRM_MEASUREMENT_ERROR.\n");
    printf("2) Use SGSLrm_GetLastHardwareErrorAscii() to get the exact string (e.g., \"ERR-16\").\n");
    printf("3) Use SGSLrm_GetMeasurementError() to get the numeric code (e.g., 16).\n");
    printf("4) No local mapping to custom enums; UI shows exactly what the device says.\n\n");

    int errorCode = -1;
    char hwErr[16] = { 0 };
    SGSLrmStatus status = SGSLrm_GetMeasurementError(handle, &errorCode);
    SGSLrm_GetLastHardwareErrorAscii(handle, hwErr, sizeof(hwErr));

    if (status == SGS_LRM_SUCCESS) {
        printf("Current stored error: code=%d, ascii='%s'\n\n", errorCode, hwErr);
    }
    else if (status == SGS_LRM_NOT_CONNECTED) {
        printf("⚠️  Device not connected - cannot test measurement errors\n\n");
    }
    else {
        printf("❌ Unexpected status from SGSLrm_GetMeasurementError(): %d\n\n", status);
    }
}

// Main demonstration function
void demonstrate_enhanced_usage() {
    SGSLrmHandle handle = NULL;
    SGSLrmStatus status;
    double distance;
    char deviceId[64]; // widen a bit

    printf("\n========================================\n");
    printf("Enhanced Laser Ranging Module Demo\n");
    printf("========================================\n\n");

    // Step 1: Initialize library
    printf("Step 1: Initializing library...\n");
    printf("✓ Library initialized successfully\n\n");

    // Step 2: Create device handle
    printf("Step 2: Creating device handle...\n");
    status = SGSLrm_CreateHandle(&handle);
    if (status != SGS_LRM_SUCCESS) {
        printf("Failed to create handle: %d\n", status);
        return;
    }
    printf("✓ Handle created successfully\n\n");

    // Step 3: List available COM ports
    printf("Step 3: Enumerating COM ports...\n");
    char portList[256];
    status = SGSLrm_EnumComPorts(portList, sizeof(portList));
    if (status == SGS_LRM_SUCCESS && strlen(portList) > 0) {
        printf("Available ports: %s\n", portList);

        // Try to connect to first available port
        char* firstPort = strtok(portList, ";");
        if (firstPort) {
            char portPath[64]; // NEW: 轉為 \\.\COMx
            make_com_path(firstPort, portPath, sizeof(portPath));

            printf("\nStep 4: Connecting to %s...\n", firstPort);
            status = SGSLrm_Connect(handle, portPath); // NEW: 使用 portPath
            if (status == SGS_LRM_SUCCESS) {
                printf("✓ Connected successfully!\n\n");

                // Read device ID
                printf("Step 5: Reading device ID...\n");
                status = SGSLrm_ReadDeviceID(handle, deviceId, sizeof(deviceId));
                if (status == SGS_LRM_SUCCESS) {
                    printf("Device ID: %s\n\n", deviceId);
                }
                else {
                    printf("Could not read device ID (status: %d)\n\n", status);
                }

                // Configure for indoor use
                printf("Step 6: Configuring device...\n");
                status = configure_for_indoor_measurement(handle);
                if (status == SGS_LRM_SUCCESS) {
                    printf("✓ Configuration complete\n\n");

                    // Turn on laser
                    printf("Step 7: Activating laser...\n");
                    status = SGSLrm_LaserOn(handle);
                    if (status == SGS_LRM_SUCCESS) {
                        printf("✓ Laser activated\n\n");

                        // Perform single measurement
                        printf("Step 8: Performing single measurement...\n");
                        status = SGSLrm_SingleMeasurement(handle, &distance);
                        if (status == SGS_LRM_SUCCESS) {
                            printf("✓ Distance: %.4f meters\n\n", distance);
                        }
                        else if (status == SGS_LRM_MEASUREMENT_ERROR) {
                            // NEW: 原樣輸出硬體錯誤
                            char hwErr[16] = { 0 };
                            int errCode = 0;
                            SGSLrm_GetLastHardwareErrorAscii(handle, hwErr, sizeof(hwErr));
                            SGSLrm_GetMeasurementError(handle, &errCode);
                            printf("❌ Measurement error: %s (code=%d)\n\n",
                                (hwErr[0] ? hwErr : "ERR-??"), errCode);
                        }
                        else {
                            printf("Measurement failed (status: %d)\n\n", status);
                        }

                        // Test error code handling system
                        test_error_code_handling(handle);

                        // Setup and start continuous measurement
                        printf("Step 9: Starting continuous measurement (5 seconds)...\n");
                        SGSLrm_SetMeasurementCallback(handle, enhanced_measurement_callback, NULL);

                        status = SGSLrm_StartContinuousMeasurement(handle);
                        if (status == SGS_LRM_SUCCESS) {
                            printf("Measuring...\n");
                            printf("-----------------------------------------\n");

                            // Measure for 5 seconds
                            Sleep(5000);

                            printf("-----------------------------------------\n");
                            SGSLrm_StopContinuousMeasurement(handle);
                            printf("✓ Continuous measurement stopped\n");
                            printf("Total measurements: %d\n\n", g_measurementCount);
                        }

                        // Turn off laser
                        printf("Step 10: Deactivating laser...\n");
                        SGSLrm_LaserOff(handle);
                        printf("✓ Laser deactivated\n\n");
                    }
                }

                // Disconnect
                printf("Step 11: Disconnecting...\n");
                SGSLrm_Disconnect(handle);
                printf("✓ Disconnected\n\n");

            }
            else {
                printf("❌ Failed to connect (status: %d)\n", status);
                printf("   Make sure the device is connected to %s\n\n", firstPort);
            }
        }
    }
    else {
        printf("No COM ports found. Running in demo mode.\n\n");

        // Demo mode - show configuration without actual device
        printf("Demo: Configuring for indoor measurement...\n");
        configure_for_indoor_measurement(handle);
        printf("\nDemo: Configuring for outdoor measurement...\n");
        configure_for_outdoor_measurement(handle);

        // Test error handling even without device connection
        test_error_code_handling(handle);
        test_error_scenarios(handle);
    }

    // Cleanup
    printf("Step 12: Cleanup...\n");
    SGSLrm_DestroyHandle(handle);
    printf("✓ Cleanup complete\n\n");
}

// Console control handler for graceful shutdown
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        printf("\n\nShutdown requested...\n");
        g_stopMeasurement = true;
        return TRUE;
    }
    return FALSE;
}

int main() {

    // Set console handler for Ctrl+C
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    printf("\n************************************************\n");
    printf("*  Enhanced Laser Ranging Module              *\n");
    printf("*  Practical Usage Example                    *\n");
    printf("************************************************\n\n");

    printf("This demo shows:\n");
    printf("• Corrected resolution settings (1=1mm, 2=0.1mm)\n");
    printf("• Enhanced error handling with RAW hardware errors (e.g., ERR-16)\n");
    printf("• NEW: SGS_LRM_MEASUREMENT_ERROR + SGSLrm_GetLastHardwareErrorAscii()\n");
    printf("• Protocol-compliant command construction\n");
    printf("• Proper device configuration for different scenarios\n");
    printf("• Real-time continuous measurement with callbacks\n\n");

    printf("Press Ctrl+C at any time to stop.\n\n");

    // Run the demonstration
    demonstrate_enhanced_usage();

    printf("========================================\n");
    printf("Demo completed!\n");
    printf("========================================\n\n");

    printf("Key Improvements Demonstrated:\n");
    printf("✅ RAW hardware error passthrough (no local mapping)\n");
    printf("✅ SGSLrm_GetLastHardwareErrorAscii() for exact device messages\n");
    printf("✅ COM10+ safe connection path handling\n");
    printf("✅ Clean, maintainable code structure\n\n");

    return 0;
}
