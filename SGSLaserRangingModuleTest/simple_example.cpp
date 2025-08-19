// Simple example demonstrating the new pool-based API
// No initialization or finalization required!

#include "../SGSLaserRangingModule/SGSLaserRangingModule.h"
#include <stdio.h>
#include <windows.h>

int main() {
    printf("SGS Laser Ranging Module - Simple Example\n");
    printf("==========================================\n\n");
    
    // No SGSLrm_Initialize() needed anymore!
    
    // Create a device handle (pool initializes automatically)
    SGSLrmHandle device;
    SGSLrmStatus status = SGSLrm_CreateHandle(&device);
    
    if (status != SGS_LRM_SUCCESS) {
        printf("Failed to create device handle\n");
        return -1;
    }
    
    printf("✓ Device handle created (pool initialized automatically)\n");
    
    // Enumerate available COM ports
    char portList[256];
    status = SGSLrm_EnumComPorts(portList, sizeof(portList));
    
    if (status == SGS_LRM_SUCCESS && strlen(portList) > 0) {
        printf("✓ Available COM ports: %s\n", portList);
        
        // Try to connect to first available port
        char* port = strtok(portList, ";");
        if (port) {
            printf("\nConnecting to %s...\n", port);
            status = SGSLrm_Connect(device, port);
            
            if (status == SGS_LRM_SUCCESS) {
                printf("✓ Connected successfully!\n\n");
                
                // Configure the device
                printf("Configuring device...\n");
                SGSLrm_SetRange(device, 30);        // 30 meters
                SGSLrm_SetResolution(device, 2);    // 0.1mm resolution (corrected!)
                SGSLrm_SetFrequency(device, 10);    // 10Hz
                printf("✓ Configuration complete\n\n");
                
                // Turn on laser and measure
                printf("Performing measurement...\n");
                SGSLrm_LaserOn(device);
                
                double distance;
                status = SGSLrm_SingleMeasurement(device, &distance);
                
                if (status == SGS_LRM_SUCCESS) {
                    printf("✓ Distance: %.3f meters\n", distance);
                } else {
                    printf("Measurement failed (error: %d)\n", status);
                }
                
                // Turn off laser
                SGSLrm_LaserOff(device);
                
                // Disconnect
                SGSLrm_Disconnect(device);
                printf("\n✓ Disconnected\n");
                
            } else {
                printf("Failed to connect (error: %d)\n", status);
                printf("Make sure device is connected to %s\n", port);
            }
        }
    } else {
        printf("No COM ports found\n");
        printf("Running in demo mode...\n\n");
        
        // Demo: Show that we can still configure without connection
        printf("Configuring device (not connected)...\n");
        status = SGSLrm_SetRange(device, 50);
        printf("  Set range: %s\n", 
               status == SGS_LRM_NOT_CONNECTED ? "Not connected (expected)" : "Error");
    }
    
    // Clean up - just destroy the handle
    SGSLrm_DestroyHandle(device);
    printf("\n✓ Device handle destroyed (slot returned to pool)\n");
    
    // No SGSLrm_Finalize() needed anymore!
    
    printf("\n==========================================\n");
    printf("Example completed successfully!\n");
    printf("\nNotice:\n");
    printf("• No SGSLrm_Initialize() was needed\n");
    printf("• No SGSLrm_Finalize() was needed\n");
    printf("• Pool managed automatically\n");
    printf("• Simpler, cleaner code!\n");
    
    return 0;
}
