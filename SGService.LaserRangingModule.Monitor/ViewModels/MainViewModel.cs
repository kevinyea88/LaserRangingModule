using SGService.LaserRangingModule.Monitor.Services;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO.Ports;
using System.Linq;

namespace SGService.LaserRangingModule.Monitor.ViewModels
{
    public sealed class MainViewModel : INotifyPropertyChanged
    {
        private static LrmRange _lastRange = LrmRange.FiveMeter;
        private static LrmResolution _lastResolution = LrmResolution.OneMillimeter;
        private static LrmFrequency _lastFrequency = default(LrmFrequency);
        private LrmRange _rangeSetting;
        public ObservableCollection<string> AvailablePorts { get; } = new();
        public ObservableCollection<PortRowViewModel> SelectedPorts { get; } = new();

        private string? _selectedPortName;
        public string? SelectedPortName
        {
            get => _selectedPortName;
            set
            {
                if (_selectedPortName == value) return;
                _selectedPortName = value;
                PropertyChanged?.Invoke(this, new(nameof(SelectedPortName)));
                AddSelectedPortCommand.RaiseCanExecuteChanged();
            }
        }

        public RelayCommand ScanCommand { get; }
        public RelayCommand AddSelectedPortCommand { get; }
        public bool CanConfigureSelectedPort => !string.IsNullOrWhiteSpace(SelectedPortName);
        public RelayCommand ApplySettingsCommand { get; }

        public MainViewModel()
        {
            ScanCommand = new RelayCommand(_ => Scan());
            // 允許同一 COM 被多次加入，位址後續由使用者在 Measurement 分頁自行指定
            AddSelectedPortCommand = new RelayCommand(
                _ => AddSelected(),
                _ => !string.IsNullOrWhiteSpace(SelectedPortName));
            RangeSetting = _lastRange;
            ResolutionSetting = _lastResolution;
            FrequencySetting = _lastFrequency;

            // Apply 按鈕指令，按下時儲存目前設定供下次載入使用
            ApplySettingsCommand = new RelayCommand(
                _ => ApplySettings(),
                _ => CanConfigureSelectedPort);

            // 當選中的 Port 改變時通知介面刷新參數區啟用狀態
            PropertyChanged += (s, e) =>
            {
                if (e.PropertyName == nameof(SelectedPortName))
                {
                    PropertyChanged?.Invoke(this, new(nameof(CanConfigureSelectedPort)));
                    ApplySettingsCommand.RaiseCanExecuteChanged();
                }
            };
        }

        // MainViewModel.cs

        private void ApplySettings()
        {
            // 1. 保存本次設定值，下次預設使用
            _lastRange = RangeSetting;
            _lastResolution = ResolutionSetting;
            _lastFrequency = FrequencySetting;

            // 2. 確認已選取有效的 COM 埠
            if (string.IsNullOrWhiteSpace(SelectedPortName)) return;

            // 3. 建立服務並連線
            using var dev = new SgsLrmDeviceService();
            var rc = dev.Connect(SelectedPortName);
            if (rc != 0)
            {
                System.Windows.MessageBox.Show(
                    $"連線失敗 (rc={rc})，請確認此 COM 埠是否可用",
                    "錯誤",
                    System.Windows.MessageBoxButton.OK,
                    System.Windows.MessageBoxImage.Error);
                return;
            }

            // 4. 設定 Range / Resolution / Frequency
            rc = dev.SetRange(RangeSetting);
            if (rc != 0)
            {
                System.Windows.MessageBox.Show(
                    $"設定 Range 失敗 (rc={rc})",
                    "錯誤",
                    System.Windows.MessageBoxButton.OK,
                    System.Windows.MessageBoxImage.Error);
            }
            rc = dev.SetResolution(ResolutionSetting);
            if (rc != 0)
            {
                System.Windows.MessageBox.Show(
                    $"設定 Resolution 失敗 (rc={rc})",
                    "錯誤",
                    System.Windows.MessageBoxButton.OK,
                    System.Windows.MessageBoxImage.Error);
            }
            rc = dev.SetFrequency(FrequencySetting);
            if (rc != 0)
            {
                System.Windows.MessageBox.Show(
                    $"設定 Frequency 失敗 (rc={rc})",
                    "錯誤",
                    System.Windows.MessageBoxButton.OK,
                    System.Windows.MessageBoxImage.Error);
            }

            // 5. 完成設定後斷線
            dev.Disconnect();
        }


        private void Scan()
        {
            var names = SerialPort.GetPortNames()
                .OrderBy(n => (n?.StartsWith("COM") == true && int.TryParse(n[3..], out var k)) ? k : int.MaxValue)
                .ToArray();

            AvailablePorts.Clear();
            foreach (var n in names) AvailablePorts.Add(n);

            if (SelectedPortName is null && AvailablePorts.Count > 0)
                SelectedPortName = AvailablePorts[0];

            AddSelectedPortCommand.RaiseCanExecuteChanged();
        }


        private void AddSelected()
        {
            if (string.IsNullOrWhiteSpace(SelectedPortName)) return;

            // 不再檢查 SelectedPorts.Any(...)；讓使用者自己調整每一個 Address
            var row = new PortRowViewModel(
                SelectedPortName!,
                new SgsLrmDeviceService(),
                removeMe: (me) =>
                {
                    SelectedPorts.Remove(me);
                    AddSelectedPortCommand.RaiseCanExecuteChanged();
                });

            SelectedPorts.Add(row);
            AddSelectedPortCommand.RaiseCanExecuteChanged();
        }

        public LrmRange RangeSetting
        {
            get => _rangeSetting;
            set
            {
                if (_rangeSetting != value)
                {
                    _rangeSetting = value;
                    PropertyChanged?.Invoke(this, new(nameof(RangeSetting)));
                }
            }
        }

        private LrmResolution _resolutionSetting;
        public LrmResolution ResolutionSetting
        {
            get => _resolutionSetting;
            set
            {
                if (_resolutionSetting != value)
                {
                    _resolutionSetting = value;
                    PropertyChanged?.Invoke(this, new(nameof(ResolutionSetting)));
                }
            }
        }

        private LrmFrequency _frequencySetting;
        public LrmFrequency FrequencySetting
        {
            get => _frequencySetting;
            set
            {
                if (_frequencySetting != value)
                {
                    _frequencySetting = value;
                    PropertyChanged?.Invoke(this, new(nameof(FrequencySetting)));
                }
            }
        }


        public event PropertyChangedEventHandler? PropertyChanged;
    }
}
