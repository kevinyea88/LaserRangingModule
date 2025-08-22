using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using System;
using System.Collections.ObjectModel;
using System.IO.Ports;
using System.Linq;


namespace SGService.LaserRangingModule.Monitor
{
    public partial class MainWindow : Window
    {
        // 下拉選單的資料來源：改變內容會即時反映到 UI
        private readonly ObservableCollection<string> _ports = new();

        public MainWindow()
        {
            InitializeComponent();

            // 綁定資料來源到下拉選單
            CmbPorts.ItemsSource = _ports;
        }

        private void BtnScan_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var names = SerialPort.GetPortNames()
                    .OrderBy(PortSortKey)   // 讓 COM1, COM2, ... COM10 正確排序
                    .ToArray();

                _ports.Clear();
                foreach (var n in names)
                    _ports.Add(n);

                if (_ports.Count > 0)
                    CmbPorts.SelectedIndex = 0; // 預設選第一個
                else
                    MessageBox.Show("未找到可用的序列埠。", "訊息",
                        MessageBoxButton.OK, MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"掃描序列埠失敗：{ex.Message}", "錯誤",
                    MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        // 將 "COM10" 解析成 10 以正確排序；非標準格式放最後
        private static int PortSortKey(string portName)
        {
            if (!string.IsNullOrEmpty(portName) &&
                portName.StartsWith("COM", StringComparison.OrdinalIgnoreCase) &&
                int.TryParse(portName.Substring(3), out var n))
            {
                return n;
            }
            return int.MaxValue;
        }
    }
}