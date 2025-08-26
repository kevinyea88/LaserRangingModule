using SGService.LaserRangingModule.Monitor.Services;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO.Ports;
using System.Linq;

namespace SGService.LaserRangingModule.Monitor.ViewModels
{
    public sealed class MainViewModel : INotifyPropertyChanged
    {
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

        public MainViewModel()
        {
            ScanCommand = new RelayCommand(_ => Scan());
            AddSelectedPortCommand = new RelayCommand(_ => AddSelected(),
                _ => !string.IsNullOrWhiteSpace(SelectedPortName) &&
                     !SelectedPorts.Any(p => p.Port == SelectedPortName));
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
            if (SelectedPorts.Any(p => p.Port == SelectedPortName)) return;

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

        public event PropertyChangedEventHandler? PropertyChanged;
    }
}
