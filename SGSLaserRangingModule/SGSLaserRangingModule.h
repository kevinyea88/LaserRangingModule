#pragma once

#include <stdbool.h>

#if defined(_SGS_LRM_EXPORT)
#define SGS_LRM_API __declspec(dllexport)
#else
#define SGS_LRM_API __declspec(dllimport)
#endif

#if defined(__cplusplus)
extern "C" {
#endif
	
	typedef int SGSLrmStatus;
	typedef void* SGSLrmHandle;

	// Status codes
#define SGS_LRM_SUCCESS                 0       // Operation successful
#define SGS_LRM_INVALID_PARAMETER      -1       // Invalid parameter
#define SGS_LRM_INVALID_HANDLE          -2       // Invalid handle
#define SGS_LRM_NOT_CONNECTED           -3       // Device not connected
#define SGS_LRM_COMMUNICATION_ERROR     -4       // Communication error
#define SGS_LRM_TIMEOUT                 -5       // Operation timeout
#define SGS_LRM_OUT_OF_MEMORY           -6       // Out of memory
#define SGS_LRM_MEASUREMENT_ERROR       -7       // Measurement error

	// Library management
	SGS_LRM_API SGSLrmStatus SGSLrm_GetVersion(int* major, int* minor, int* patch);

	// Device management
	SGS_LRM_API SGSLrmStatus SGSLrm_CreateHandle(SGSLrmHandle* handle);
	SGS_LRM_API SGSLrmStatus SGSLrm_DestroyHandle(SGSLrmHandle handle);
	SGS_LRM_API SGSLrmStatus SGSLrm_Connect(SGSLrmHandle handle, const char* comPort);
	SGS_LRM_API SGSLrmStatus SGSLrm_Disconnect(SGSLrmHandle handle);
	SGS_LRM_API SGSLrmStatus SGSLrm_IsConnected(SGSLrmHandle handle, bool* connected);

	// Device configuration
	SGS_LRM_API SGSLrmStatus SGSLrm_SetAddress(SGSLrmHandle handle, int address);
	SGS_LRM_API SGSLrmStatus SGSLrm_SetRange(SGSLrmHandle handle, int rangeMeters); // 5,10,30,50,80
	SGS_LRM_API SGSLrmStatus SGSLrm_SetResolution(SGSLrmHandle handle, int resolution); // 1 for 1mm, 2 for 0.1mm
	SGS_LRM_API SGSLrmStatus SGSLrm_SetFrequency(SGSLrmHandle handle, int frequency); // 3-20Hz
	SGS_LRM_API SGSLrmStatus SGSLrm_SetMeasurementInterval(SGSLrmHandle handle, int intervalMs);
	SGS_LRM_API SGSLrmStatus SGSLrm_SetDistanceCorrection(SGSLrmHandle handle, int correctionMm); // Distance correction in mm
	SGS_LRM_API SGSLrmStatus SGSLrm_SetStartPosition(SGSLrmHandle handle, int position); // 0=tail, 1=top
	SGS_LRM_API SGSLrmStatus SGSLrm_SetAutoMeasurement(SGSLrmHandle handle, int enable); // Auto measurement on power up

	// Measurement functions
	SGS_LRM_API SGSLrmStatus SGSLrm_SingleMeasurement(SGSLrmHandle handle, double* distance);
	SGS_LRM_API SGSLrmStatus SGSLrm_StartContinuousMeasurement(SGSLrmHandle handle);
	SGS_LRM_API SGSLrmStatus SGSLrm_StopContinuousMeasurement(SGSLrmHandle handle);
	SGS_LRM_API SGSLrmStatus SGSLrm_GetLastMeasurement(SGSLrmHandle handle, double* distance);
	SGS_LRM_API SGSLrmStatus SGSLrm_BroadcastMeasurement(SGSLrmHandle handle);
	SGS_LRM_API SGSLrmStatus SGSLrm_ReadCache(SGSLrmHandle handle, double* distance);

	// Laser control
	SGS_LRM_API SGSLrmStatus SGSLrm_LaserOn(SGSLrmHandle handle);
	SGS_LRM_API SGSLrmStatus SGSLrm_LaserOff(SGSLrmHandle handle);
	SGS_LRM_API SGSLrmStatus SGSLrm_GetLaserStatus(SGSLrmHandle handle, bool* isOn);

	// Callback function
	typedef void (*SGSLrm_MeasurementCallback)(SGSLrmHandle handle, double distance, SGSLrmStatus status, void* userdata);
	SGS_LRM_API SGSLrmStatus SGSLrm_SetMeasurementCallback(SGSLrmHandle handle, SGSLrm_MeasurementCallback callback, void* userdata);

	// Utility functions
	SGS_LRM_API SGSLrmStatus SGSLrm_EnumComPorts(char* portList, int bufferSize);
	SGS_LRM_API SGSLrmStatus SGSLrm_ReadDeviceID(SGSLrmHandle handle, char* deviceId, int bufferSize);
	SGS_LRM_API SGSLrmStatus SGSLrm_Shutdown(SGSLrmHandle handle);
	SGS_LRM_API SGSLrmStatus SGSLrm_GetMeasurementError(SGSLrmHandle handle, int* errorCode);
	SGS_LRM_API SGSLrmStatus SGSLrm_GetLastHardwareErrorAscii(SGSLrmHandle handle, char* buf, int bufSize);
	
#if defined(__cplusplus)
}
#endif