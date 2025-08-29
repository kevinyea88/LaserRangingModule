using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;

namespace SGService.LaserRangingModule.Monitor.Models
{
    public class DeviceChartData : INotifyPropertyChanged
    {
        public string DeviceName { get; }
        public string Color { get; set; } = "Blue";  // Chart line color

        public ObservableCollection<ChartDataPoint> DataPoints { get; } = new();

        private int _nextSampleNumber = 1;

        public DeviceChartData(string deviceName)
        {
            DeviceName = deviceName;
        }

        public void AddSuccessPoint(double distance)
        {
            var point = ChartDataPoint.CreateSuccess(_nextSampleNumber++, distance);
            DataPoints.Add(point);
            UpdateStatistics();
        }

        public void AddErrorPoint(string errorMessage)
        {
            var point = ChartDataPoint.CreateError(_nextSampleNumber++, errorMessage);
            DataPoints.Add(point);
            UpdateStatistics();
        }

        public void ClearData()
        {
            DataPoints.Clear();
            _nextSampleNumber = 1;
            UpdateStatistics();
        }

        // Statistics calculations
        private void UpdateStatistics()
        {
            PropertyChanged?.Invoke(this, new(nameof(TotalSamples)));
            PropertyChanged?.Invoke(this, new(nameof(ValidSamples)));
            PropertyChanged?.Invoke(this, new(nameof(ErrorSamples)));
            PropertyChanged?.Invoke(this, new(nameof(ErrorRate)));
            PropertyChanged?.Invoke(this, new(nameof(AverageDistance)));
            PropertyChanged?.Invoke(this, new(nameof(MinDistance)));
            PropertyChanged?.Invoke(this, new(nameof(MaxDistance)));
            PropertyChanged?.Invoke(this, new(nameof(HasValidData)));
        }

        public int TotalSamples => DataPoints.Count;
        public int ValidSamples => DataPoints.Count(p => !p.IsError);
        public int ErrorSamples => DataPoints.Count(p => p.IsError);

        public double ErrorRate => TotalSamples == 0 ? 0.0 : (double)ErrorSamples / TotalSamples * 100.0;

        public double? AverageDistance
        {
            get
            {
                var validPoints = DataPoints.Where(p => !p.IsError && p.Distance.HasValue).ToList();
                return validPoints.Any() ? validPoints.Average(p => p.Distance!.Value) : null;
            }
        }

        public double? MinDistance
        {
            get
            {
                var validPoints = DataPoints.Where(p => !p.IsError && p.Distance.HasValue).ToList();
                return validPoints.Any() ? validPoints.Min(p => p.Distance!.Value) : null;
            }
        }

        public double? MaxDistance
        {
            get
            {
                var validPoints = DataPoints.Where(p => !p.IsError && p.Distance.HasValue).ToList();
                return validPoints.Any() ? validPoints.Max(p => p.Distance!.Value) : null;
            }
        }

        public bool HasValidData => ValidSamples > 0;

        public event PropertyChangedEventHandler? PropertyChanged;
    }
}