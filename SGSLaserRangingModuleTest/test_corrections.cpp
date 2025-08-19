// Test file to verify the corrected functions
// This test ensures the API now properly matches the protocol specification

#include "../SGSLaserRangingModule/SGSLaserRangingModule.h"
#include <stdio.h>
#include <assert.h>

void test_resolution_api() {
    printf("Testing Resolution API corrections...\n");
    
    SGSLrmHandle handle;
    SGSLrmStatus status;
    
    // Initialize library
    
    
    
    // Create handle
    status = SGSLrm_CreateHandle(&handle);
     
    
    // Test valid resolution values as per protocol
    // Protocol specifies: 1 for 1mm, 2 for 0.1mm
    
    // Test 1: Setting resolution to 1mm (value = 1)
    printf("  - Testing resolution = 1 (1mm)... ");
    // Note: This will fail if not connected, but we're testing parameter validation
    // In real usage, connect first: SGSLrm_Connect(handle, "COM3");
    status = SGSLrm_SetResolution(handle, 1);  // Should be valid
    if (status == SGS_LRM_NOT_CONNECTED) {
        printf("OK (not connected, but parameter accepted)\n");
    } else if (status == SGS_LRM_SUCCESS) {
        printf("OK (connected and set)\n");
    } else {
        printf("FAILED (unexpected error: %d)\n", status);
    }
    
    // Test 2: Setting resolution to 0.1mm (value = 2, not 0!)
    printf("  - Testing resolution = 2 (0.1mm)... ");
    status = SGSLrm_SetResolution(handle, 2);  // Should be valid
    if (status == SGS_LRM_NOT_CONNECTED) {
        printf("OK (not connected, but parameter accepted)\n");
    } else if (status == SGS_LRM_SUCCESS) {
        printf("OK (connected and set)\n");
    } else {
        printf("FAILED (unexpected error: %d)\n", status);
    }
    
    // Test 3: Invalid resolution value (0 should now be rejected)
    printf("  - Testing resolution = 0 (invalid)... ");
    status = SGSLrm_SetResolution(handle, 0);  // Should be invalid
    if (status == SGS_LRM_INVALID_PARAMETER) {
        printf("OK (correctly rejected)\n");
    } else {
        printf("FAILED (should have been rejected)\n");
    }
    
    // Test 4: Invalid resolution value (3)
    printf("  - Testing resolution = 3 (invalid)... ");
    status = SGSLrm_SetResolution(handle, 3);  // Should be invalid
    if (status == SGS_LRM_INVALID_PARAMETER) {
        printf("OK (correctly rejected)\n");
    } else {
        printf("FAILED (should have been rejected)\n");
    }
    
    // Cleanup
    SGSLrm_DestroyHandle(handle);
    
    
    printf("Resolution API test completed.\n\n");
}

void test_device_id_response() {
    printf("Testing Device ID response format...\n");
    
    // This test would need an actual device connection
    // Here we just verify the API accepts the correct parameters
    
    SGSLrmHandle handle;
    SGSLrmStatus status;
    char deviceId[32];
    
     
    
    status = SGSLrm_CreateHandle(&handle);
     
    
    // Test with valid buffer
    printf("  - Testing with valid buffer... ");
    status = SGSLrm_ReadDeviceID(handle, deviceId, sizeof(deviceId));
    if (status == SGS_LRM_NOT_CONNECTED) {
        printf("OK (not connected, but parameters accepted)\n");
    } else if (status == SGS_LRM_SUCCESS) {
        printf("OK (connected, ID: %s)\n", deviceId);
    } else {
        printf("Status: %d\n", status);
    }
    
    // Test with NULL buffer (should fail)
    printf("  - Testing with NULL buffer... ");
    status = SGSLrm_ReadDeviceID(handle, NULL, 32);
    if (status == SGS_LRM_INVALID_PARAMETER) {
        printf("OK (correctly rejected)\n");
    } else {
        printf("FAILED (should have been rejected)\n");
    }
    
    // Test with zero buffer size (should fail)
    printf("  - Testing with zero buffer size... ");
    status = SGSLrm_ReadDeviceID(handle, deviceId, 0);
    if (status == SGS_LRM_INVALID_PARAMETER) {
        printf("OK (correctly rejected)\n");
    } else {
        printf("FAILED (should have been rejected)\n");
    }
    
    SGSLrm_DestroyHandle(handle);
    
    
    printf("Device ID test completed.\n\n");
}

void display_protocol_reference() {
    printf("==============================================\n");
    printf("Protocol Quick Reference (After Corrections):\n");
    printf("==============================================\n\n");
    
    printf("RESOLUTION VALUES:\n");
    printf("  API Value | Protocol Byte | Resolution\n");
    printf("  ----------|---------------|------------\n");
    printf("      1     |     0x01      | 1mm\n");
    printf("      2     |     0x02      | 0.1mm\n\n");
    
    printf("DEVICE ID RESPONSE FORMAT:\n");
    printf("  Command:  FA 06 04 FC\n");
    printf("  Response: FA 06 84 [ASCII_DATA...] CS\n");
    printf("            ^^ ^^ ^^\n");
    printf("            |  |  |\n");
    printf("            |  |  +-- Status byte (0x84 for device ID)\n");
    printf("            |  +----- Command type (0x06)\n");
    printf("            +-------- Broadcast address (0xFA)\n\n");
    
    printf("FREQUENCY VALUES:\n");
    printf("  API Value | Protocol Byte | Frequency\n");
    printf("  ----------|---------------|----------\n");
    printf("      3     |     0x00      | ~3Hz\n");
    printf("      5     |     0x05      | 5Hz\n");
    printf("     10     |     0x0A      | 10Hz\n");
    printf("     20     |     0x14      | 20Hz\n\n");
    
    printf("RANGE VALUES:\n");
    printf("  API Value | Protocol Byte | Range\n");
    printf("  ----------|---------------|-------\n");
    printf("      5     |     0x05      | 5m\n");
    printf("     10     |     0x0A      | 10m\n");
    printf("     30     |     0x1E      | 30m\n");
    printf("     50     |     0x32      | 50m\n");
    printf("     80     |     0x50      | 80m\n\n");
}

int main() {
    printf("========================================\n");
    printf("Laser Ranging Module Correction Tests\n");
    printf("========================================\n\n");
    
    // Run tests
    test_resolution_api();
    test_device_id_response();
    
    // Display protocol reference
    display_protocol_reference();
    
    printf("All tests completed!\n");
    printf("\nNOTE: To fully test with a real device, connect it and update the COM port in the code.\n");
    
    return 0;
}
