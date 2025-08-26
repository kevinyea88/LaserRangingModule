using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace SGService.LaserRangingModule.Monitor
{
    public partial class MainWindow : Window
    {
        // 只做輸入檢核用：限制 ADDR 欄位數字
        private static readonly Regex _digitsOnly = new(@"^\d+$");

        public MainWindow()
        {
            InitializeComponent();
            // DataContext 已在 XAML 指定；若想改由程式碼設定，取消下一行註解並移除 XAML 的 <Window.DataContext>
            DataContext = new SGService.LaserRangingModule.Monitor.ViewModels.MainViewModel();
        }

        // ---- ADDR 數字輸入限制 ----
        private void Addr_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            e.Handled = !_digitsOnly.IsMatch(e.Text);
        }

        private void Addr_Pasting(object sender, DataObjectPastingEventArgs e)
        {
            if (!e.DataObject.GetDataPresent(typeof(string)))
            {
                e.CancelCommand();
                return;
            }
            var text = (string)e.DataObject.GetData(typeof(string));
            if (!_digitsOnly.IsMatch(text))
                e.CancelCommand();
        }

        // ---- 這兩個先留空殼（之後會改成 PortRowViewModel 的 Command）----
        private void RowConnectButton_Click(object sender, RoutedEventArgs e)
        {
            // TODO: 下一步把這顆改用 PortRowViewModel 的 ToggleConnectCommand
        }

        private void RowRemoveButton_Click(object sender, RoutedEventArgs e)
        {
            // TODO: 下一步把這顆改用 PortRowViewModel 的 RemoveCommand
        }
    }
}
