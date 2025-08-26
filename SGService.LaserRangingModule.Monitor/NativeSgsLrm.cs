using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace SGService.LaserRangingModule.Monitor
{
    internal static unsafe partial class NativeSgsLrm
    {
        // Library management
        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_GetVersion") ]
        public static partial int GetVersion(out int major, out int minor, out int patch);

        // Device management
        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_CreateHandle")]
        public static partial int CreateHandle(out nint handle);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_DestroyHandle")]
        public static partial int DestroyHandle(nint handle);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_Connect")]
        public static partial int Connect(nint handle,[MarshalAs(UnmanagedType.LPStr)] string comPort);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_Disconnect")]
        public static partial int Disconnect(nint handle);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_IsConnected")]
        public static partial int IsConnected(nint handle, [MarshalAs(UnmanagedType.I1)] out bool connected);

        // Device configuration
        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_SetAddress")]
        public static partial int SetAddress(nint handle, int address);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_SetRange")]
        public static partial int SetRange(nint handle, LrmRange range);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_SetResolution")]
        public static partial int SetResolution(nint handle, LrmResolution resolution);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_SetFrequency")]
        public static partial int SetFrequency(nint handle, LrmFrequency frequency);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_SetMeasurementInterval")]
        public static partial int SetMeasurementInterval(nint handle, int intervalMs);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_SetDistanceCorrection")]
        public static partial int SetDistanceCorrection(nint handle, int distanceCorrection);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_SetStartPosition")]
        public static partial int SetStartPosition(nint handle, LrmStartPosition position);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_SetAutoMeasurement")]
        public static partial int SetAutoMeasurement(nint handle, [MarshalAs(UnmanagedType.I1)]bool enable);

        // Measurement functions
        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_SingleMeasurement")]
        public static partial int SingleMeasurement(nint handle, out double distance);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_StartContinuousMeasurement")]
        public static partial int StartContinuousMeasurement(nint handle);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_StopContinuousMeasurement")]
        public static partial int StopContinuousMeasurement(nint handle);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_GetLastMeasurement")]
        public static partial int GetLastMeasurement(nint handle, out double distance);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_BroadcastMeasurement")]
        public static partial int BroadcastMeasurement(nint handle);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_ReadCache")]
        public static partial int ReadCache(nint handle, out double distance);

        // Laser control
        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_LaserOn")]
        public static partial int LaserOn(nint handle);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_LaserOff")]
        public static partial int LaserOff(nint handle);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_GetLaserStatus")]
        public static partial int GetLaserStatus(nint handle, [MarshalAs(UnmanagedType.I1)] out bool isOn);

        // Callback function

        // Utility functions
        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_EnumComPorts")]
        public static partial int EnumComPorts([MarshalAs(UnmanagedType.LPStr)] out string protlist, int bufferSize);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_ReadDeviceID")]
        public static partial int ReadDeviceID(nint handle, [MarshalAs(UnmanagedType.LPStr)] out string deviceId, int bufferSize);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_Shutdown")]
        public static partial int Shutdown(nint handle);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_GetMeasurementError")]
        public static partial int GetMeasurementError(nint handle, out int errorCode);

        [LibraryImport("SGSLaserRangingModule", EntryPoint = "SGSLrm_GetLastHardwareErrorAscii")]
        public static partial int GetLastHardwareErrorAscii(nint handle, [MarshalAs(UnmanagedType.LPStr)] out string buf, int bufferSize);

    }
}
