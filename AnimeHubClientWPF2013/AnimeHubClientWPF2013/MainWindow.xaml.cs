using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace AnimeHubClientWPF2013
{
    public partial class MainWindow : Window
    {
        int expanded_value = 0; // max 10;
        bool expanded = false;
        DispatcherTimer dispatcherTimer1;
        DispatcherTimer dispatcherTimer2;
        public MainWindow()
        {
            InitializeComponent();
            BitmapImage i1 = App.OpenImage("icons8-right-32.png");
            BitmapImage i2 = App.OpenImage("icons8-contacts-32.png");
            BitmapImage i3 = App.OpenImage("icons8-search-property-32.png");
            BitmapImage i4 = App.OpenImage("icons8-picture-32.png");
            BitmapImage i5 = App.OpenImage("icons8-download-32.png");
            BitmapImage i6 = App.OpenImage("icons8-settings-32.png");
            expander_image.Source = i1;
            LogInButtonImage.Source = i2;
            CatalogButtonImage.Source = i3;
            RandomButtonImage.Source = i4;
            DownloadsButtonImage.Source = i5;
            SettingsButtonImage.Source = i6;
            dispatcherTimer1 = new System.Windows.Threading.DispatcherTimer();
            dispatcherTimer1.Tick += new EventHandler(Timer_Tick1);
            dispatcherTimer1.Interval = new TimeSpan(0, 0, 0, 0, 10);
            dispatcherTimer1.Start();
            dispatcherTimer2 = new System.Windows.Threading.DispatcherTimer();
            dispatcherTimer2.Tick += new EventHandler(Timer_Tick2);
            dispatcherTimer2.Interval = new TimeSpan(0, 0, 0, 0, 500);
            dispatcherTimer2.Start();
            App.mp = this;
            bool connection = App.IsConnectedToServer();
            if (connection)
            {
                App.ConnectedToServer = true;
                ContentFrame.Navigate(new System.Uri("ContentCatalog.xaml", UriKind.RelativeOrAbsolute));
            }
            else
            {
                App.ConnectedToServer = false;
                string message = "Не получается подключиться к серверу, проверьте подключение к интернету и попробуйте заново запустить приложение";
                MessageBox.Show(message, "Ошибка подключения к серверу", MessageBoxButton.OK);
                System.Windows.Application.Current.Shutdown();
            }
        }
        private void Timer_Tick1(object sender, EventArgs e)
        {
            if (expanded && expanded_value < 10)
            {
                expanded_value++;
                expander_image.RenderTransform = new RotateTransform(180.0 * expanded_value / 10.0, 15,15);
                MenuColumn.Width = new GridLength( 40 + 120 * expanded_value / 10);
            }
            if (expanded == false && expanded_value > 0)
            {
                expanded_value--;
                expander_image.RenderTransform = new RotateTransform(180.0 * expanded_value / 10.0, 15, 15);
                MenuColumn.Width = new GridLength(40 + 120 * expanded_value / 10);
            }
        }
        private void Timer_Tick2(object sender, EventArgs e)
        {
            if (App.LoggedIn)
            {
                UserName.Text = App.UserName;
            }
            else
            {
                UserName.Text = "Войти";
            }
        }
        private void MenuExpand(object sender, EventArgs e)
        {
            if (this.expander_lv.SelectedItem == null) return;
            if (expanded) expanded = false;
            else expanded = true;
            this.expander_lv.SelectedItem = null;
        }
        private unsafe void MenuItemSelected(object sender, EventArgs e)
        {
            object item = this.MenuList.SelectedItem;
            if (item == null) return;
            ListViewItem item_lw = item as ListViewItem;
            string tag = (string)item_lw.Tag;
            switch (tag)
            {
                case "log_in":
                    ContentFrame.Navigate(new System.Uri("LogInOutRegistration.xaml", UriKind.RelativeOrAbsolute));
                    break;
                case "random":
                    int* AnimePagesIDs = App.stock_catalog();
                    Random rand = new Random();
                    App.AnimePageID = AnimePagesIDs[rand.Next(AnimePagesIDs[0]) + 1];
                    ContentFrame.Navigate(new System.Uri("AnimePage.xaml", UriKind.RelativeOrAbsolute));
                    break;
                case "catalog":
                    ContentFrame.Navigate(new System.Uri("ContentCatalog.xaml", UriKind.RelativeOrAbsolute));
                    break;
                case "downloads":
                    ContentFrame.Navigate(new System.Uri("Downloads.xaml", UriKind.RelativeOrAbsolute));
                    break;
                case "settings":
                    ContentFrame.Navigate(new System.Uri("Settings.xaml", UriKind.RelativeOrAbsolute));
                    break;
            }
            this.MenuList.SelectedItem = null;
        }
    }
}
