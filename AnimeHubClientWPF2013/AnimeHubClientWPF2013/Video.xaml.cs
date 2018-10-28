using System;
using System.Collections.Generic;
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

namespace AnimeHubClientWPF2013
{
    /// <summary>
    /// Логика взаимодействия для Video.xaml
    /// </summary>
    public partial class Video : Page
    {
        public Video()
        {
            InitializeComponent();

            AnimeVideo.Source = new Uri(App.AnimeVideoPath, UriKind.RelativeOrAbsolute);
            AnimeVideo.Play();
        }
        public bool play = true;
        private void pause_play_btn_Click(object sender, RoutedEventArgs e)
        {
            if (play)
            {
                AnimeVideo.Pause();
                play = false;
            }
            else
            {
                AnimeVideo.Play();
                play = true;
            }
        }
    }
}
