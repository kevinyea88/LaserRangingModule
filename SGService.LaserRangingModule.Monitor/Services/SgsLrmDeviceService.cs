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
    }
}
