// Services/SgsLrmDeviceService.cs
using System;
using SGService.LaserRangingModule.Monitor;

namespace SGService.LaserRangingModule.Monitor.Services
{
    public interface ILaserDeviceService : IDisposable
    {
        bool IsConnected { get; }
        int Connect(string port);          // 0 表成功
        int Disconnect();                  // 0 表成功
        int SetAddress(int address);       // 0 表成功
        int SetRange(LrmRange range);
        int SetResolution(LrmResolution resolution);
        int SetFrequency(LrmFrequency frequency);
        int SetStartPosition(LrmStartPosition pos);
        int SetMeasurementInterval(int intervalMs);
        int SetAutoMeasurement(bool enable);
        int SetDistanceCorrection(int mm);
        int BroadcastMeasurement();
        int ReadCache(out double distance);
    }

    public sealed class SgsLrmDeviceService : ILaserDeviceService
    {
        private nint _handle;
        public bool IsConnected { get; private set; }

        public int Connect(string port)
        {
            if (_handle == 0)
            {
                var rc = NativeSgsLrm.CreateHandle(out _handle);
                if (rc != 0 || _handle == 0) return rc != 0 ? rc : -1;
            }
            string path = MakeComDevicePath(port);
            var rc2 = NativeSgsLrm.Connect(_handle, path);
            if (rc2 == 0) IsConnected = true;
            return rc2;
        }

        public int Disconnect()
        {
            if (_handle == 0) return 0;
            try { return NativeSgsLrm.Disconnect(_handle); }
            finally { IsConnected = false; }
        }

        public int SetAddress(int address)
        {
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.SetAddress(_handle, address);
        }

        public void Dispose()
        {
            try
            {
                if (_handle != 0)
                {
                    if (IsConnected) { try { NativeSgsLrm.Disconnect(_handle); } catch { } }
                    try { NativeSgsLrm.DestroyHandle(_handle); } catch { }
                }
            }
            finally { _handle = 0; IsConnected = false; }
        }

        private static string MakeComDevicePath(string port)
        {
            if (!string.IsNullOrEmpty(port) &&
                port.StartsWith("COM", StringComparison.OrdinalIgnoreCase) &&
                int.TryParse(port.AsSpan(3), out var n) && n >= 10)
            {
                return @"\\.\" + port;
            }
            return port;
        }

        public int SetRange(LrmRange range)
        {
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.SetRange(_handle, range);
        }

        public int SetResolution(LrmResolution resolution)
        {
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.SetResolution(_handle, resolution);
        }

        public int SetFrequency(LrmFrequency freq)
        {
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.SetFrequency(_handle, freq);
        }

        public int SetStartPosition(LrmStartPosition pos)
        {
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.SetStartPosition(_handle, pos);
        }

        public int SetMeasurementInterval(int intervalMs)
        {
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.SetMeasurementInterval(_handle, intervalMs);
        }

        public int SetAutoMeasurement(bool enable)
        {
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.SetAutoMeasurement(_handle, enable);
        }

        public int SetDistanceCorrection(int mm)
        {
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.SetDistanceCorrection(_handle, mm);
        }

        public int BroadcastMeasurement()
        {
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.BroadcastMeasurement(_handle);
        }

        public int ReadCache(out double distance)
        {
            distance = 0;
            if (_handle == 0 || !IsConnected) return -1;
            return NativeSgsLrm.ReadCache(_handle, out distance);
        }

    }
}
