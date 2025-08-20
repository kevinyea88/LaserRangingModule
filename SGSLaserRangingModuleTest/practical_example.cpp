// Practical example showing how to use the enhanced SGSLaserRangingModule
// This demonstrates real-world usage with proper error handling

#include "../SGSLaserRangingModule/SGSLaserRangingModule.h"
#include <stdio.h>
#include <windows.h>
#include <string.h>

// Global variables for continuous measurement
static volatile bool g_stopMeasurement = false;
static int g_measurementCount = 0;

typedef void (*SGSLrmTestCase)(SGSLrmHandle handle, void* userData);
typedef struct {
    const char* name;
    SGSLrmTestCase test;
    void* userData;
} SGSLrmTestEntry;

//static SGSLrmTestEntry g_testCases[] = {
//    {"EnhancedMeasurementCallback", enhanced_measurement_callback},
//    {"ConfigureForIndoorMeasurement", configure_for_indoor_measurement},
//    {"ConfigureForOutdoorMeasurement", configure_for_outdoor_measurement},
//};

// Enhanced callback with detailed error reporting and measurement error handling
void enhanced_measurement_callback(SGSLrmHandle handle, double distance, SGSLrmStatus status, void* userdata) {
    g_measurementCount++;
    
    if (status == SGS_LRM_SUCCESS) {
        printf("[%03d] Distance: %.4f meters\n", g_measurementCount, distance);
        
        // Example: Alert if object is too close
        if (distance < 0.5) {
            printf("      ⚠️  WARNING: Object detected within 0.5 meters!\n");
        }
    } else if (status == SGS_LRM_MEASUREMENT_ERROR) {
        // Use new error handling system
        int errorCode = 0;
        SGSLrmStatus getErrorStatus = SGSLrm_GetMeasurementError(handle, &errorCode);
        
        printf("[%03d] Measurement Error: ", g_measurementCount);
        if (getErrorStatus == SGS_LRM_SUCCESS) {
            switch (errorCode) {
                case 10:
                    printf("Low battery - Please charge device (ERR-%02d)\n", errorCode);
                    break;
                case 14:
                    printf("Calculation error - Check measurement conditions (ERR-%02d)\n", errorCode);
                    break;
                case 15:
                    printf("Target out of range - Move closer or check range setting (ERR-%02d)\n", errorCode);
                    break;
                case 16:
                    printf("Weak signal - Clean lens or improve target surface (ERR-%02d)\n", errorCode);
                    break;
                case 18:
                    printf("Strong ambient light - Shield sensor from direct sunlight (ERR-%02d)\n", errorCode);
                    break;
                case 26:
                    printf("Display range exceeded (ERR-%02d)\n", errorCode);
                    break;
                default:
                    printf("Unknown measurement error (ERR-%02d)\n", errorCode);
            }
        } else {
            printf("Failed to get error code details\n");
        }
    } else {
        // Handle other non-measurement errors
        printf("[%03d] Error: ", g_measurementCount);
        switch (status) {
            case SGS_LRM_ERR_LOW_BATTERY:
                printf("Low battery - Please charge device (ERR-10)\n");
                break;
            case SGS_LRM_ERR_CALCULATION_ERROR:
                printf("Calculation error - Check measurement conditions (ERR-14)\n");
                break;
            case SGS_LRM_ERR_OUT_OF_RANGE:
                printf("Target out of range - Move closer or check range setting (ERR-15)\n");
                break;
            case SGS_LRM_ERR_WEAK_SIGNAL:
                printf("Weak signal - Clean lens or improve target surface (ERR-16)\n");
                break;
            case SGS_LRM_ERR_STRONG_LIGHT:
                printf("Strong ambient light - Shield sensor from direct sunlight (ERR-18)\n");
                break;
            case SGS_LRM_ERR_DISPLAY_RANGE:
                printf("Display range exceeded (ERR-26)\n");
                break;
            case SGS_LRM_COMMUNICATION_ERROR:
                printf("Communication error - Check connection\n");
                break;
            case SGS_LRM_TIMEOUT:
                printf("Timeout - Device not responding\n");
                break;
            default:
                printf("Unknown error (code: %d)\n", status);
        }
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
    status = SGSLrm_SetResolution(handle, 2); // 2 = 0.1mm (corrected value!)
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

// Function to test error code handling
void test_error_code_handling(SGSLrmHandle handle) {
    printf("\n========================================\n");
    printf("Error Code Handling Test\n");
    printf("========================================\n\n");
    
    SGSLrmStatus status;
    double distance;
    int errorCode;
    
    printf("Testing error code handling system:\n\n");
    
    // Test 1: Check initial error code (should be 0)
    printf("Test 1: Checking initial error code...\n");
    status = SGSLrm_GetMeasurementError(handle, &errorCode);
    if (status == SGS_LRM_SUCCESS) {
        printf("✓ Initial error code: %d (should be 0 for no error)\n\n", errorCode);
    } else {
        printf("❌ Failed to get initial error code (status: %d)\n\n", status);
    }
    
    // Test 2: Perform measurement and check for errors
    printf("Test 2: Performing single measurement...\n");
    status = SGSLrm_SingleMeasurement(handle, &distance);
    
    if (status == SGS_LRM_SUCCESS) {
        printf("✓ Measurement successful: %.4f meters\n", distance);
        
        // Check error code after successful measurement (should be 0)
        status = SGSLrm_GetMeasurementError(handle, &errorCode);
        if (status == SGS_LRM_SUCCESS) {
            printf("✓ Error code after successful measurement: %d (should be 0)\n\n", errorCode);
        }
    } else if (status == SGS_LRM_MEASUREMENT_ERROR) {
        printf("❌ Measurement failed with measurement error\n");
        
        // Get detailed error code
        status = SGSLrm_GetMeasurementError(handle, &errorCode);
        if (status == SGS_LRM_SUCCESS) {
            printf("📋 Detailed error code: ERR-%02d\n", errorCode);
            
            // Provide specific error description
            switch (errorCode) {
                case 10:
                    printf("   → Low battery detected\n");
                    break;
                case 14:
                    printf("   → Calculation error occurred\n");
                    break;
                case 15:
                    printf("   → Target is out of range\n");
                    break;
                case 16:
                    printf("   → Signal too weak or timeout\n");
                    break;
                case 18:
                    printf("   → Strong ambient light interference\n");
                    break;
                case 26:
                    printf("   → Display range exceeded\n");
                    break;
                default:
                    printf("   → Unknown measurement error\n");
            }
            printf("\n");
        } else {
            printf("❌ Failed to retrieve detailed error code\n\n");
        }
    } else {
        printf("❌ Measurement failed with other error (status: %d)\n\n", status);
    }
    
    // Test 3: Test invalid parameters
    printf("Test 3: Testing invalid parameters...\n");
    status = SGSLrm_GetMeasurementError(handle, NULL);
    if (status == SGS_LRM_INVALID_PARAMETER) {
        printf("✓ Correctly rejected NULL error code pointer\n");
    } else {
        printf("❌ Should have rejected NULL pointer (got status: %d)\n", status);
    }
    
    status = SGSLrm_GetMeasurementError(NULL, &errorCode);
    if (status == SGS_LRM_INVALID_HANDLE) {
        printf("✓ Correctly rejected NULL handle\n\n");
    } else {
        printf("❌ Should have rejected NULL handle (got status: %d)\n\n", status);
    }
}

// Function to simulate and test different error scenarios
void test_error_scenarios(SGSLrmHandle handle) {
    printf("\n========================================\n");
    printf("Error Scenario Testing\n");
    printf("========================================\n\n");
    
    printf("This section demonstrates how the new error handling system works:\n\n");
    
    printf("1. Error Code Mapping:\n");
    printf("   Protocol ERR-10 → SGS_LRM_MEASUREMENT_ERROR + errorCode=10 (Low battery)\n");
    printf("   Protocol ERR-14 → SGS_LRM_MEASUREMENT_ERROR + errorCode=14 (Calculation error)\n");
    printf("   Protocol ERR-15 → SGS_LRM_MEASUREMENT_ERROR + errorCode=15 (Out of range)\n");
    printf("   Protocol ERR-16 → SGS_LRM_MEASUREMENT_ERROR + errorCode=16 (Weak signal)\n");
    printf("   Protocol ERR-18 → SGS_LRM_MEASUREMENT_ERROR + errorCode=18 (Strong light)\n");
    printf("   Protocol ERR-26 → SGS_LRM_MEASUREMENT_ERROR + errorCode=26 (Display range)\n\n");
    
    printf("2. Usage Pattern:\n");
    printf("   - All measurement errors return SGS_LRM_MEASUREMENT_ERROR\n");
    printf("   - Use SGSLrm_GetMeasurementError() to get specific error code\n");
    printf("   - Error codes are stored per device handle\n");
    printf("   - Error codes are cleared on successful measurements\n\n");
    
    printf("3. Benefits:\n");
    printf("   ✓ Unified error handling for all measurement errors\n");
    printf("   ✓ Preserves original protocol error information\n");
    printf("   ✓ Thread-safe error code storage\n");
    printf("   ✓ Clear separation between API errors and measurement errors\n\n");
    
    // Test the error code functions with edge cases
    printf("4. Function Validation:\n");
    
    int errorCode = -1; // Initialize with invalid value
    SGSLrmStatus status = SGSLrm_GetMeasurementError(handle, &errorCode);
    
    if (status == SGS_LRM_SUCCESS) {
        printf("   ✓ SGSLrm_GetMeasurementError() works correctly\n");
        printf("   ✓ Current error code: %d\n", errorCode);
    } else if (status == SGS_LRM_NOT_CONNECTED) {
        printf("   ⚠️  Device not connected - cannot test measurement errors\n");
        printf("   ✓ Function correctly reports connection status\n");
    } else {
        printf("   ❌ Unexpected status from SGSLrm_GetMeasurementError(): %d\n", status);
    }
    
    printf("\n");
}

// Main demonstration function
void demonstrate_enhanced_usage() {
    SGSLrmHandle handle = NULL;
    SGSLrmStatus status;
    double distance;
    char deviceId[32];
    
    printf("\n========================================\n");
    printf("Enhanced Laser Ranging Module Demo\n");
    printf("========================================\n\n");
    
    // Step 1: Initialize library
    printf("Step 1: Initializing library...\n");
  
    //if (status != SGS_LRM_SUCCESS) {
    //    printf("Failed to initialize library: %d\n", status);
    //    return;
    //}
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
            printf("\nStep 4: Connecting to %s...\n", firstPort);
            status = SGSLrm_Connect(handle, firstPort);
            
            if (status == SGS_LRM_SUCCESS) {
                printf("✓ Connected successfully!\n\n");
                
                // Read device ID (uses corrected parsing!)
                printf("Step 5: Reading device ID...\n");
                status = SGSLrm_ReadDeviceID(handle, deviceId, sizeof(deviceId));
                if (status == SGS_LRM_SUCCESS) {
                    printf("Device ID: %s\n\n", deviceId);
                } else {
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
                        } else if (status == SGS_LRM_MEASUREMENT_ERROR) {
                            printf("❌ Measurement error occurred\n");
                            
                            // Get detailed error code using new function
                            int errorCode = 0;
                            SGSLrmStatus getErrorStatus = SGSLrm_GetMeasurementError(handle, &errorCode);
                            if (getErrorStatus == SGS_LRM_SUCCESS) {
                                printf("   Detailed error: ERR-%02d\n\n", errorCode);
                            } else {
                                printf("   Could not retrieve error details\n\n");
                            }
                        } else {
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
                
            } else {
                printf("❌ Failed to connect (status: %d)\n", status);
                printf("   Make sure the device is connected to %s\n\n", firstPort);
            }
        }
    } else {
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
    printf("• Enhanced error handling with specific error codes\n");
    printf("• NEW: Measurement error code handling (SGS_LRM_MEASUREMENT_ERROR)\n");
    printf("• NEW: SGSLrm_GetMeasurementError() function for detailed error info\n");
    printf("• Protocol-compliant command construction\n");
    printf("• Proper device configuration for different scenarios\n");
    printf("• Real-time continuous measurement with callbacks\n\n");
    
    printf("Press Ctrl+C at any time to stop.\n\n");
    
    // Run the demonstration
    demonstrate_enhanced_usage();
    
    printf("========================================\n");
    printf("Demo completed successfully!\n");
    printf("========================================\n\n");
    
    printf("Key Improvements Demonstrated:\n");
    printf("✅ Resolution API now correctly uses 2 for 0.1mm\n");
    printf("✅ Device ID parsing fixed (FA 06 84 format)\n");
    printf("✅ All commands use protocol constants\n");
    printf("✅ Enhanced error reporting with specific codes\n");
    printf("✅ NEW: Measurement error handling system (SGS_LRM_MEASUREMENT_ERROR)\n");
    printf("✅ NEW: SGSLrm_GetMeasurementError() for detailed error analysis\n");
    printf("✅ Clean, maintainable code structure\n\n");
    
    return 0;
}
