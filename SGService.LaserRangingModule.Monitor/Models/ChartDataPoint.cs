using System;

namespace SGService.LaserRangingModule.Monitor.Models
{
    public class ChartDataPoint
    {
        public int SampleNumber { get; set; }
        public double? Distance { get; set; }  // null for error points
        public DateTime Timestamp { get; set; }
        public bool IsError { get; set; }
        public string? ErrorMessage { get; set; }  // e.g., "ERR-16"

        public ChartDataPoint(int sampleNumber, DateTime timestamp)
        {
            SampleNumber = sampleNumber;
            Timestamp = timestamp;
        }

        // Success point
        public static ChartDataPoint CreateSuccess(int sampleNumber, double distance)
        {
            return new ChartDataPoint(sampleNumber, DateTime.Now)
            {
                Distance = distance,
                IsError = false
            };
        }

        // Error point
        public static ChartDataPoint CreateError(int sampleNumber, string errorMessage)
        {
            return new ChartDataPoint(sampleNumber, DateTime.Now)
            {
                Distance = null,
                IsError = true,
                ErrorMessage = errorMessage
            };
        }

        public override string ToString()
        {
            if (IsError)
                return $"Sample #{SampleNumber}: {ErrorMessage}";
            else
                return $"Sample #{SampleNumber}: {Distance:F3}m";
        }
    }
}