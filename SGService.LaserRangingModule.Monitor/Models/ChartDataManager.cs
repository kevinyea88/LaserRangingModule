using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;

namespace SGService.LaserRangingModule.Monitor.Models
{
    public class ChartDataManager : INotifyPropertyChanged
    {
        private readonly Dictionary<string, DeviceChartData> _deviceData = new();
        private static readonly string[] ChartColors = { "Blue", "Red", "Green", "Orange", "Purple", "Brown", "Pink", "Gray" };
        private int _colorIndex = 0;

        public IEnumerable<DeviceChartData> AllDeviceData => _deviceData.Values;

        public DeviceChartData GetOrCreateDeviceData(string deviceName)
        {
            if (!_deviceData.ContainsKey(deviceName))
            {
                var deviceData = new DeviceChartData(deviceName)
                {
                    Color = ChartColors[_colorIndex % ChartColors.Length]
                };
                _colorIndex++;

                _deviceData[deviceName] = deviceData;
                deviceData.PropertyChanged += (s, e) => UpdateGlobalStatistics();

                PropertyChanged?.Invoke(this, new(nameof(AllDeviceData)));
                UpdateGlobalStatistics();
            }

            return _deviceData[deviceName];
        }

        public void RemoveDevice(string deviceName)
        {
            if (_deviceData.Remove(deviceName))
            {
                PropertyChanged?.Invoke(this, new(nameof(AllDeviceData)));
                UpdateGlobalStatistics();
            }
        }

        public void ClearAllData()
        {
            foreach (var deviceData in _deviceData.Values)
            {
                deviceData.ClearData();
            }
            UpdateGlobalStatistics();
        }

        public void ClearDevice(string deviceName)
        {
            if (_deviceData.TryGetValue(deviceName, out var deviceData))
            {
                deviceData.ClearData();
            }
        }

        // Global chart bounds for auto-scaling
        private void UpdateGlobalStatistics()
        {
            PropertyChanged?.Invoke(this, new(nameof(GlobalMinY)));
            PropertyChanged?.Invoke(this, new(nameof(GlobalMaxY)));
            PropertyChanged?.Invoke(this, new(nameof(GlobalMaxSamples)));
            PropertyChanged?.Invoke(this, new(nameof(HasAnyData)));
        }

        public double? GlobalMinY
        {
            get
            {
                var allMins = _deviceData.Values
                    .Where(d => d.MinDistance.HasValue)
                    .Select(d => d.MinDistance!.Value)
                    .ToList();

                return allMins.Any() ? allMins.Min() : null;
            }
        }

        public double? GlobalMaxY
        {
            get
            {
                var allMaxs = _deviceData.Values
                    .Where(d => d.MaxDistance.HasValue)
                    .Select(d => d.MaxDistance!.Value)
                    .ToList();

                return allMaxs.Any() ? allMaxs.Max() : null;
            }
        }

        public int GlobalMaxSamples
        {
            get
            {
                return _deviceData.Values.Any() ? _deviceData.Values.Max(d => d.TotalSamples) : 0;
            }
        }

        public bool HasAnyData => _deviceData.Values.Any(d => d.HasValidData);

        public event PropertyChangedEventHandler? PropertyChanged;
    }
}