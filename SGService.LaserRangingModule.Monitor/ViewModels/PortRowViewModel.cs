using SGService.LaserRangingModule.Monitor.Services;
using System.ComponentModel;
using System.Windows;
using System.Windows.Threading;
using SGService.LaserRangingModule.Monitor.Models;
using System;

namespace SGService.LaserRangingModule.Monitor.ViewModels
{
    public sealed class PortRowViewModel : INotifyPropertyChanged
    {
        public string Port { get; }
        private readonly ILaserDeviceService _dev;
        private readonly System.Action<PortRowViewModel> _removeMe;
        private DispatcherTimer? _measurementTimer;
        private ChartDataManager? _chartDataManager;  // 將透過注入提供
        // 量測資料事件
        public event Action<string, double>? MeasurementReceived;        // deviceName, distance
        public event Action<string, string>? MeasurementErrorReceived;   // deviceName, errorMessag
        public PortRowViewModel(string port, ILaserDeviceService dev, System.Action<PortRowViewModel> removeMe)
        {
            Port = port;
            _dev = dev;
            _removeMe = removeMe;

            ToggleConnectCommand = new RelayCommand(_ => ToggleConnect());
            RemoveCommand = new RelayCommand(_ => Remove(), _ => !IsConnected);

            // 加入量測命令
            StartMeasurementCommand = new RelayCommand(
                _ => StartMeasurement(),
                _ => IsConnected && !IsMeasuring);

            StopMeasurementCommand = new RelayCommand(
                _ => StopMeasurement(),
                _ => IsMeasuring);
        }


        private int _address = 128;                 // 0–255
        public int Address
        {
            get => _address;
            set
            {
                var v = value; if (v < 0) v = 0; if (v > 255) v = 255;
                if (_address == v) return; _address = v;
                PropertyChanged?.Invoke(this, new(nameof(Address)));
            }
        }

        // Add these new properties after the Address property

        private bool _isSelectedForChart;
        public bool IsSelectedForChart
        {
            get => _isSelectedForChart;
            set
            {
                if (_isSelectedForChart == value) return;
                _isSelectedForChart = value;
                PropertyChanged?.Invoke(this, new(nameof(IsSelectedForChart)));

                // Auto-deselect when disconnected
                if (value && !IsConnected)
                {
                    _isSelectedForChart = false;
                    PropertyChanged?.Invoke(this, new(nameof(IsSelectedForChart)));
                }
            }
        }

        private bool _isMeasuring;
        public bool IsMeasuring
        {
            get => _isMeasuring;
            set
            {
                if (_isMeasuring == value) return;
                _isMeasuring = value;
                PropertyChanged?.Invoke(this, new(nameof(IsMeasuring)));
                PropertyChanged?.Invoke(this, new(nameof(ConnectionStatus)));
                UpdateCommandStates();  // Add this line
            }
        }

        // Computed properties for binding
        public string PortName => Port;  // Alias for existing Port property

        public string ConnectionStatus
        {
            get
            {
                if (!IsConnected) return "Disconnected";
                if (IsMeasuring) return "Measuring";
                return "Connected";
            }
        }

        // 在 ConnectionStatus 之後加入這些屬性

        private double _lastMeasurement;
        public double LastMeasurement
        {
            get => _lastMeasurement;
            private set
            {
                if (Math.Abs(_lastMeasurement - value) < 0.0001) return;
                _lastMeasurement = value;
                PropertyChanged?.Invoke(this, new(nameof(LastMeasurement)));
                PropertyChanged?.Invoke(this, new(nameof(LastMeasurementText)));
            }
        }

        public string LastMeasurementText => LastMeasurement > 0 ? $"{LastMeasurement:F3}m" : "--";

        private int _measurementCount;
        public int MeasurementCount
        {
            get => _measurementCount;
            private set
            {
                if (_measurementCount == value) return;
                _measurementCount = value;
                PropertyChanged?.Invoke(this, new(nameof(MeasurementCount)));
            }
        }

        private string _lastError = "";
        public string LastError
        {
            get => _lastError;
            private set
            {
                if (_lastError == value) return;
                _lastError = value;
                PropertyChanged?.Invoke(this, new(nameof(LastError)));
            }
        }

        // 量測控制
        public RelayCommand StartMeasurementCommand { get; }
        public RelayCommand StopMeasurementCommand { get; }


        private bool _isConnected;

        public bool IsConnected
        {
            get => _isConnected;
            set
            {
                if (_isConnected == value) return;
                _isConnected = value;
                PropertyChanged?.Invoke(this, new(nameof(IsConnected)));
                PropertyChanged?.Invoke(this, new(nameof(ConnectButtonText)));
                PropertyChanged?.Invoke(this, new(nameof(ConnectionStatus)));
                RemoveCommand.RaiseCanExecuteChanged();

                // Auto-deselect from chart when disconnected
                if (!value && IsSelectedForChart)
                {
                    IsSelectedForChart = false;
                }
            }
        }

        public string ConnectButtonText => IsConnected ? "Disconnect" : "Connect";

        public RelayCommand ToggleConnectCommand { get; }
        public RelayCommand RemoveCommand { get; }

        private void ToggleConnect()
        {
            if (!IsConnected)
            {
                // 先打開指定 Port
                var rc = _dev.Connect(Port);
                if (rc == 0)
                {
                    // 連線成功後設置模組位址，以區分匯流排上的多台設備
                    rc = _dev.SetAddress(Address);
                    if (rc == 0)
                    {
                        IsConnected = true;
                        UpdateCommandStates();  // Add this line
                    }
                    else
                    {
                        System.Windows.MessageBox.Show(
                            $"設定位址失敗 (rc={rc})，請確認裝置是否在線",
                            "錯誤",
                            MessageBoxButton.OK,
                            MessageBoxImage.Error);
                        // 設置失敗即斷線，避免設備處於未知狀態
                        _dev.Disconnect();
                    }
                }
                else
                {
                    System.Windows.MessageBox.Show($"Connect 失敗 (rc={rc})", "錯誤",
                        System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                }
            }
            else
            {
                // Stop measurement if running
                if (IsMeasuring)
                {
                    StopMeasurement();
                }

                // 斷線並恢復 UI 狀態
                _dev.Disconnect();
                IsConnected = false;
                UpdateCommandStates();  // Add this line
            }
        }


        private void Remove()
        {
            // 未連線才能移除（CanExecute 已限制）；保險起見也斷一下
            if (IsConnected) _dev.Disconnect();
            _dev.Dispose();
            _removeMe(this);
        }

        // 加入以下新方法

        public void StartMeasurement()
        {
            if (!IsConnected || IsMeasuring) return;

            IsMeasuring = true;
            MeasurementCount = 0;
            LastError = "";

            // Create timer for continuous measurement
            _measurementTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(200) // 5Hz measurement rate
            };
            _measurementTimer.Tick += OnMeasurementTimer;
            _measurementTimer.Start();

            UpdateCommandStates();
        }
        public void StopMeasurement()
        {
            if (!IsMeasuring) return;

            _measurementTimer?.Stop();
            _measurementTimer = null;

            IsMeasuring = false;
            UpdateCommandStates();
        }
        private void OnMeasurementTimer(object? sender, EventArgs e)
        {
            try
            {
                // Perform single measurement using your native DLL
                var result = NativeSgsLrm.SingleMeasurement(_dev.Handle, out double distance);

                if (result == 0) // SGS_LRM_SUCCESS
                {
                    // Successful measurement
                    LastMeasurement = distance;
                    MeasurementCount++;
                    LastError = "";

                    // Fire event for chart data
                    MeasurementReceived?.Invoke(PortName, distance);
                }
                else if (result == -7) // SGS_LRM_MEASUREMENT_ERROR
                {
                    // Get hardware error details
                    var errorResult = NativeSgsLrm.GetLastHardwareErrorAscii(_dev.Handle, out string errorAscii, 16);
                    string errorMsg = errorResult == 0 ? errorAscii : "ERR-??";

                    MeasurementCount++;
                    LastError = errorMsg;

                    // Fire event for chart data
                    MeasurementErrorReceived?.Invoke(PortName, errorMsg);
                }
                else
                {
                    // Other communication errors - stop measurement
                    LastError = $"Communication Error ({result})";
                    StopMeasurement();
                }
            }
            catch (Exception ex)
            {
                LastError = $"Exception: {ex.Message}";
                StopMeasurement();
            }
        }

        private void UpdateCommandStates()
        {
            StartMeasurementCommand.RaiseCanExecuteChanged();
            StopMeasurementCommand.RaiseCanExecuteChanged();
        }




        public event PropertyChangedEventHandler? PropertyChanged;
    }
}
