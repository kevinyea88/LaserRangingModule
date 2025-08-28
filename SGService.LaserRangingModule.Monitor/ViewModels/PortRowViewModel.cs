using SGService.LaserRangingModule.Monitor.Services;
using System.ComponentModel;
using System.Windows;

namespace SGService.LaserRangingModule.Monitor.ViewModels
{
    public sealed class PortRowViewModel : INotifyPropertyChanged
    {
        public string Port { get; }
        private readonly ILaserDeviceService _dev;
        private readonly System.Action<PortRowViewModel> _removeMe;

        public PortRowViewModel(string port, ILaserDeviceService dev, System.Action<PortRowViewModel> removeMe)
        {
            Port = port; _dev = dev; _removeMe = removeMe;
            ToggleConnectCommand = new RelayCommand(_ => ToggleConnect());
            RemoveCommand = new RelayCommand(_ => Remove(), _ => !IsConnected);
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
                RemoveCommand.RaiseCanExecuteChanged();
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
                // 斷線並恢復 UI 狀態
                _dev.Disconnect();
                IsConnected = false;
            }
        }

        private void Remove()
        {
            // 未連線才能移除（CanExecute 已限制）；保險起見也斷一下
            if (IsConnected) _dev.Disconnect();
            _dev.Dispose();
            _removeMe(this);
        }

        public event PropertyChangedEventHandler? PropertyChanged;
    }
}
