using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
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
    public partial class ContentCatalog : Page
    {
        public class AnimeItem
        {
            public BitmapImage Picture { get; set; }
            public int PictureID { get; set; }
            public string Name { get; set; }
            public string Genre { get; set; }
            public string Studio { get; set; }
            public string Year { get; set; }
            public string Episodes { get; set; }
            public string LastUpdTime { get; set; }
        }

        public List<AnimeItem> anime_items_list { get; set; }

        unsafe int* AnimeIDs;
        unsafe int* GenreIDs;
        int min;
        unsafe int* StudioIDs;
        int expander_value = 0;
        bool expander_bool = false;
        DispatcherTimer dispatcherTimer;
        public unsafe ContentCatalog()
        {
            InitializeComponent();
            BitmapImage i1 = App.OpenImage("icons8-double-down-32.png");
            FilterExpanderImage.Source = i1;
            dispatcherTimer = new System.Windows.Threading.DispatcherTimer();
            dispatcherTimer.Tick += new EventHandler(Timer_Tick);
            dispatcherTimer.Interval = new TimeSpan(0, 0, 0, 0, 10);
            dispatcherTimer.Start();

            if (App.LoggedIn) Create.Visibility = Visibility.Visible;
            else Create.Visibility = Visibility.Hidden;
            //загрузка жанров
            List<string> data = new List<string>();
            GenreIDs = App.GetGenresIDs();   
            data.Add("Любой жанр");
            for (int i = 0; i < GenreIDs[0]; i++)
            {
                char* genre_char_ponter = App.GetGenreName(GenreIDs[i + 1]);
                string genre_str = new string(genre_char_ponter);
                data.Add(genre_str);
            }
            GenreComboBox.ItemsSource = data;
            GenreComboBox.SelectedIndex = 0;
            
            //загрузка студий
            data = new List<string>();
            data.Add("Любая студия");
            StudioIDs = App.GetStudioIDs();
            for (int i = 0; i < StudioIDs[0]; i++)
            {
                char* studio_char_ponter = App.GetStudioName(StudioIDs[i + 1]);
                string studio_str = new string(studio_char_ponter);
                data.Add(studio_str);
            }
            StudioComboBox.ItemsSource = data;
            StudioComboBox.SelectedIndex = 0;

            //загрузка годов
            data = new List<string>();
            min = App.GetMinAnimeYear();
            int max = App.GetGetMaxAnimeYear();
            for (int i = min; i <= max; i++)
            {
                data.Add(i.ToString());
            }
            YearFromComboBox.ItemsSource = data;
            YearFromComboBox.SelectedIndex = 0;
            YearToComboBox.ItemsSource = data;
            YearToComboBox.SelectedIndex = max - min;
            YearFromComboBox.SelectedIndex = 0;
            //загрузка числа cерий

            //загрузка аним
            AnimeIDs = App.stock_catalog();
            LoadItems();
            //загрузка видов сортировки
            data = new List<string>();
            data.Add("Сортировать по алфавиту");
            data.Add("Сортировать по дате последнего обновления");
            SortByComboBox.ItemsSource = data;
            SortByComboBox.SelectedIndex = 0;
        }
        private unsafe void LoadItems()
        {
            anime_items_list = new List<AnimeItem>();
            AnimeItem ani_item;
            for (int i = 0; i < AnimeIDs[0]; i++)
            {
                ani_item = new AnimeItem();
                App.anime_page* anime = App.getAnimePage(AnimeIDs[i+1]);
                char* chr = App.byte_to_char(anime->name, 128);
                ani_item.Name = new string(chr);
                chr = App.byte_to_char(anime->LastUpdTime, 128);
                ani_item.LastUpdTime = new string(chr);
                ani_item.Year = anime->release_year.ToString();
                ani_item.Episodes = anime->NumOfEpisodes.ToString();
                ani_item.PictureID = App.GetPosterID(anime->id);
                ani_item.LastUpdTime = new string(chr);
                App.GetPoster(ani_item.PictureID);
                BitmapImage i1 = App.OpenImage("Posters\\" + ani_item.PictureID.ToString() + ".png");
                ani_item.Picture = i1;
                int* studois_ids = App.GetAniStudioIDs(anime->id);
                ani_item.Studio = "";
                for(int j = 0; j < studois_ids[0]; j++){
                    chr = App.GetStudioName(studois_ids[j + 1]);
                    ani_item.Studio += new string(chr);
                    if (j + 1 != studois_ids[0]) ani_item.Studio += ", ";
                }
                ani_item.Genre = "";
                int* genre_ids = App.GetAnimeGenresIDs(anime->id);
                for (int j = 0; j < genre_ids[0]; j++)
                {
                    chr = App.GetGenreName(genre_ids[j + 1]);
                    ani_item.Genre += new string(chr);
                    if (j + 1 != genre_ids[0]) ani_item.Genre += ", ";
                }
                anime_items_list.Add(ani_item);
            }
            Items_LV.ItemsSource = anime_items_list;
            //this.Items_LV.DataContext = this;
        }
        private void Timer_Tick(object sender, EventArgs e)
        {
            if (expander_bool && expander_value < 10)
            {
                expander_value++;
                Filter.Height = new GridLength(20 + 90 * expander_value / 10);
            }
            if (expander_bool == false && expander_value > 0)
            {
                expander_value--;
                Filter.Height = new GridLength(20 + 90 * expander_value / 10);
            }
        }
        private void FilterMouseEntered(object sender, EventArgs e)
        {
            expander_bool = true;
        }
        private void FilterMouseLeaved(object sender, EventArgs e)
        {
            expander_bool = false;
        }
        int EpisodesFrom = 1;
        private void EpisodesFromTextBox_TextChanged(object sender, EventArgs e)
        {
            if (EpisodesFromTextBox == null)
            {
                return;
            }
            else
            {
                if (!int.TryParse(EpisodesFromTextBox.Text, out EpisodesFrom))
                {
                    EpisodesFromTextBox.Text = EpisodesFrom.ToString();
                }
                else
                {
                    if (EpisodesFrom < 1) EpisodesFrom = 1;
                    EpisodesFromTextBox.Text = EpisodesFrom.ToString();
                }
            }
            if (EpisodesFrom > EpisodesTo)
            {
                int temp = EpisodesTo;
                EpisodesTo = EpisodesFrom;
                EpisodesFrom = temp;
                EpisodesFromTextBox.Text = EpisodesFrom.ToString();
                EpisodesToTextBox.Text = EpisodesTo.ToString();
            }
        }
        int EpisodesTo = 999;
        private void EpisodesToTextBox_TextChanged(object sender, EventArgs e)
        {
            if (EpisodesToTextBox == null)
            {
                return;
            }
            else
            {
                if (!int.TryParse(EpisodesToTextBox.Text, out EpisodesTo))
                {
                    EpisodesToTextBox.Text = EpisodesTo.ToString();
                }
                else
                {
                    if (EpisodesTo < 1) EpisodesTo = 1;
                    EpisodesToTextBox.Text = EpisodesTo.ToString();
                }
            }
            if (EpisodesFrom > EpisodesTo)
            {
                int temp = EpisodesTo;
                EpisodesTo = EpisodesFrom;
                EpisodesFrom = temp;
                EpisodesFromTextBox.Text = EpisodesFrom.ToString();
                EpisodesToTextBox.Text = EpisodesTo.ToString();
            }
        }
        private void YearFromComboBox_SelectionChanged(object sender, EventArgs e)
        {
            if (YearFromComboBox.SelectedIndex > YearToComboBox.SelectedIndex)
            {
                int temp = YearFromComboBox.SelectedIndex;
                YearFromComboBox.SelectedIndex = YearToComboBox.SelectedIndex;
                YearToComboBox.SelectedIndex = temp;
            }
        }
        private void YearToComboBox_SelectionChanged(object sender, EventArgs e)
        {
            if (YearFromComboBox.SelectedIndex > YearToComboBox.SelectedIndex)
            {
                int temp = YearFromComboBox.SelectedIndex;
                YearFromComboBox.SelectedIndex = YearToComboBox.SelectedIndex;
                YearToComboBox.SelectedIndex = temp;
            }
        }
        private unsafe void SearchButton_Click(object sender, EventArgs e)
        {
            App.filter Filter;
            if (GenreComboBox.SelectedIndex == 0) Filter.Genre = 0;
            else Filter.Genre = GenreIDs[GenreComboBox.SelectedIndex];
            if (StudioComboBox.SelectedIndex == 0) Filter.Studio = 0;
            else Filter.Studio = StudioIDs[StudioComboBox.SelectedIndex];
            Filter.SortType = SortByComboBox.SelectedIndex;
            Filter.MinEp = EpisodesFrom;
            Filter.MaxEp = EpisodesTo;
            Filter.MinYear = min + YearFromComboBox.SelectedIndex;
            Filter.MaxYear = min + YearToComboBox.SelectedIndex;
            if (SearchBox.Text.Length > 1) Filter.IsThereName = 1;
            char[] str = SearchBox.Text.ToCharArray();
            App.char_to_byte(str, Filter.AniName, 128);
            AnimeIDs = App.catalog_after_filter(&Filter);
            LoadItems();
        }
        private unsafe void ItemClicked(object sender, EventArgs e)
        {
            App.AnimePageID = AnimeIDs[1 + Items_LV.SelectedIndex];
            App.mp.ContentFrame.Navigate(new System.Uri("AnimePage.xaml", UriKind.RelativeOrAbsolute));
        }

        private void Create_Click(object sender, RoutedEventArgs e)
        {
            App.AnimePageID = -100;
            App.mp.ContentFrame.Navigate(new System.Uri("AnimePage.xaml", UriKind.RelativeOrAbsolute));
        }
    }
}
