using Microsoft.Win32;
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
    /// <summary>
    /// Логика взаимодействия для AnimePage.xaml
    /// </summary>
    public partial class AnimePage : Page
    {
        public int PosterID;
        public int AnimeID;
        public List<int> GenresIDs = new List<int>();
        public List<int> StudiosIDs = new List<int>();
        public int ReleaseYear = 1;
        public string LastUpdateDate;
        public int TotalEpisodesNum = 1;
        public List<int> Episodes = new List<int>();
        public string AnimeName;
        public List<int> ScreenshotsIDs = new List<int>();
        public string Description;

        public List<int> GenresIDs_Upd = new List<int>();
        public List<int> StudiosIDs_Upd = new List<int>();
        public List<int> Episodes_Upd = new List<int>();

        public List<int> AllGenresIDs_Upd = new List<int>();
        public List<int> AllStudiosIDs_Upd = new List<int>();

        public class Screenshot
        {
            public BitmapImage bitmap { get; set; }
        }
        public List<Screenshot> screenshot_list { get; set; }

        public class EpisodeClass
        {
            public bool downloading { get; set; }
            public int EpisodeID { get; set; }
            public string tag { get; set; }          
            public int EpisodeNumber { get; set; }
            public string EpisodeNumberText { get; set; }
            public bool IsExist { get; set; }
            public bool IsNotExist { get; set; }
            public Visibility progrress_visibility { get; set; }
            public double progress { get; set; }
            public int loadingPocessID { get; set; }
        }
        public List<EpisodeClass> episodes_list { get; set; }
        
        DispatcherTimer dispatcherTimer;
        
        public unsafe AnimePage()
        {
            InitializeComponent();
            AnimeID = App.AnimePageID;
            

            if (AnimeID > 0)
            {
                InitPageForView();
                InitPageForUpdate();
            }
            else
            {
                InitPageForCreate();
                InitPageForUpdate();
            }

            dispatcherTimer = new System.Windows.Threading.DispatcherTimer();
            dispatcherTimer.Tick += new EventHandler(Timer_Tick);
            dispatcherTimer.Interval = new TimeSpan(0, 0, 0, 0, 50);
            dispatcherTimer.Start();
        }
        public unsafe void InitPageForView()
        {
            if (App.LoggedIn) Changing.Visibility = Visibility.Visible;
            else Changing.Visibility = Visibility.Hidden;
            PageDataLoad();
            LoadPosters();
            LoadScreenshots();
            LoadEpisodes();
        }
        public unsafe void InitPageForUpdate()
        {
            char* ch;
            List<string> list = new List<string>();
            int* ids = App.GetStudioIDs();
            for (int i = 0; i < ids[0]; i++)
            {
                AllStudiosIDs_Upd.Add(ids[1 + i]);
                ch = App.GetStudioName(ids[1 + i]);
                string s = new string(ch);
                list.Add(s);
            }
            StudioUpCB.ItemsSource = list;

            list = new List<string>();
            ids = App.GetGenresIDs();
            for (int i = 0; i < ids[0]; i++)
            {
                AllGenresIDs_Upd.Add(ids[1 + i]);
                ch = App.GetGenreName(ids[1 + i]);
                string s = new string(ch);
                list.Add(s);
            }
            GenreUpCB.ItemsSource = list;

        }
        public unsafe void InitPageForCreate()
        {
            Changing.Visibility = Visibility.Hidden;
            ReturnFromChanging.Visibility = Visibility.Hidden;
            ViewGrid.Visibility = Visibility.Hidden;
            UpdateGrid.Visibility = Visibility.Visible;
        }
        bool EpisodeListChanged = true;
        private void Timer_Tick(object sender, EventArgs e)
        {
            if (AnimeID > 0 && EpisodeListChanged)
            {
                //LoadEpisodes();
                for (int i = 0; i < episodes_list.Count; i++)
                {
                    if (episodes_list[i].downloading)
                    {
                        
                        if (episodes_list[i].loadingPocessID >= 0) 
                        {
                            episodes_list[i].IsNotExist = false;
                            episodes_list[i].IsExist = false;
                            episodes_list[i].progrress_visibility = Visibility.Visible;
                            int progr = App.GetLoadingProgress(episodes_list[i].loadingPocessID);
                            episodes_list[i].progress = progr;
                            if (progr == 100)
                            {
                                episodes_list[i].downloading = false;
                                episodes_list[i].IsExist = true;
                                episodes_list[i].progrress_visibility = Visibility.Hidden;
                            }
                        }
                        EpisodesLV.ItemsSource = null;
                        EpisodesLV.ItemsSource = episodes_list;
                    }
                }
                
            }
            else
            {
                if (EpisodeListChanged)
                {

                }
            }
        }
        private void DownloadButton_Click(object sender, RoutedEventArgs e)
        {
            if (AnimeID < 1) return;
            EpisodeListChanged = false;
            if (EpisodesLV.SelectedIndex >= 0 && EpisodesLV.SelectedIndex < Episodes.Count)
            {
                
                int id_proc = App.downloadEpisode(Episodes[EpisodesLV.SelectedIndex]);
                
                episodes_list[EpisodesLV.SelectedIndex].loadingPocessID = id_proc;
                episodes_list[EpisodesLV.SelectedIndex].progrress_visibility = Visibility.Visible;
                episodes_list[EpisodesLV.SelectedIndex].progress = 0.0;
                episodes_list[EpisodesLV.SelectedIndex].downloading = true;
                EpisodesLV.ItemsSource = null;
                EpisodesLV.ItemsSource = episodes_list;
                
                
            }
            EpisodeListChanged = true;
        }
        private void PlayButton_Click(object sender, RoutedEventArgs e)
        {
            if (EpisodesLV.SelectedIndex < 0 || EpisodesLV.SelectedIndex >= Episodes.Count) return;
            //EpisodesLV.SelectedItem
            App.AnimeVideoPath = "Episodes\\" + Episodes[EpisodesLV.SelectedIndex].ToString() + ".mp4";
            App.mp.ContentFrame.Navigate(new System.Uri("Video.xaml", UriKind.RelativeOrAbsolute)); 
        }

        public string PosterUpPath = "";
        public void LoadPosters()
        {
            if (AnimeID < 1) return;
            PosterID = App.GetPosterID(AnimeID);
            App.GetPoster(PosterID);
            BitmapImage i1 = App.OpenImage("Posters\\" + PosterID.ToString() + ".png");
            Poster.Source = i1;
            PosterUp.Source = i1;
        }
        private void PosterUpButton_Click(object sender, RoutedEventArgs e)
        {
            if (AnimeID < 1) return;
            var fileDialog = new OpenFileDialog();
            fileDialog.ShowDialog();
            if (fileDialog.CheckFileExists)
            {
                string file = fileDialog.FileName;
                char[] ch = file.ToCharArray();
                App.BindPoster(AnimeID, ch);
                LoadPosters();
            }
        }

        private unsafe void GenreUpCB_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (GenreUpCB.SelectedIndex < 0 || GenreUpCB.SelectedIndex > AllGenresIDs_Upd.Count) return;
            int id = AllGenresIDs_Upd[GenreUpCB.SelectedIndex];
            GenresIDs_Upd.Add(id);
            char* ch = App.GetGenreName(id);
            string s = new string(ch);
            if (GenresIDs_Upd.Count > 1)
            {
                GenreUp.Text += ", " + s;
            }
            else
            {
                GenreUp.Text += s;
            }
        }
        private unsafe void StudioUpCB_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (StudioUpCB.SelectedIndex < 0 || StudioUpCB.SelectedIndex > AllStudiosIDs_Upd.Count) return;
            int id = AllStudiosIDs_Upd[StudioUpCB.SelectedIndex];
            StudiosIDs_Upd.Add(id);
            char* ch = App.GetStudioName(id);
            string s = new string(ch);
            if (StudiosIDs_Upd.Count > 1)
            {
                StudioUp.Text += ", " + s;
            }
            else
            {
                StudioUp.Text += s;
            }
        }
        private void ClearGenresButton_Click(object sender, RoutedEventArgs e)
        {
            GenreUp.Text = "Жанр: ";
            GenresIDs_Upd = new List<int>();
        }
        private void ClearStudiosButton_Click(object sender, RoutedEventArgs e)
        {
            StudioUp.Text = "Студия: ";
            StudiosIDs_Upd = new List<int>();
        }
        private void EpisodeUpTB_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (EpisodeUpTB == null)
            {
                return;
            }
            else
            {
                if (!int.TryParse(EpisodeUpTB.Text, out TotalEpisodesNum))
                {
                    EpisodeUpTB.Text = TotalEpisodesNum.ToString();
                }
                else
                {
                    if (TotalEpisodesNum < 1) TotalEpisodesNum = 1;
                    EpisodeUpTB.Text = TotalEpisodesNum.ToString();
                }
            }
        }
        private void YearUpTB_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (YearUpTB == null)
            {
                return;
            }
            else
            {
                if (!int.TryParse(YearUpTB.Text, out ReleaseYear))
                {
                    YearUpTB.Text = ReleaseYear.ToString();
                }
                else
                {
                    if (ReleaseYear < 1) ReleaseYear = 1;
                    YearUpTB.Text = ReleaseYear.ToString();
                }
            }
        }

        public unsafe void PageDataLoad()
        {
            App.anime_page* anime = App.getAnimePage(AnimeID);
            char* chr = App.byte_to_char(anime->name, 128);
            AnimeName = new string(chr);
            Title.Text = AnimeName;
            TitleUp.Text = AnimeName;

            ReleaseYear = anime->release_year;
            Year.Text = "Год выпуска: " + ReleaseYear.ToString();
            YearUpTB.Text = ReleaseYear.ToString();

            chr = App.byte_to_char(anime->LastUpdTime, 128);
            LastUpd.Text = "Последнее изменение: " + new string(chr);

            chr = App.byte_to_char(anime->description, 4000);
            Description = new string(chr);
            DescriptionTB.Text = Description;
            DescriptionTBUp.Text = Description;

            int* IDs = App.GetAnimeGenresIDs(AnimeID);
            int temp;
            GenresIDs = new List<int>();
            GenresIDs_Upd = new List<int>();
            Genre.Text = "Жанры: ";
            for (int i = 0; i < IDs[0]; i++)
            {
                temp = IDs[i + 1];
                GenresIDs.Add(temp);
                GenresIDs_Upd.Add(temp);
                chr = App.GetGenreName(temp);
                Genre.Text += new string(chr);
                if (i < IDs[0] - 1) Genre.Text += ", ";
            }
            GenreUp.Text = Genre.Text;

            Studio.Text = "Студия: ";
            StudiosIDs = new List<int>();
            StudiosIDs_Upd = new List<int>();
            IDs = App.GetAniStudioIDs(AnimeID);
            for (int i = 0; i < IDs[0]; i++)
            {
                temp = IDs[i + 1];
                StudiosIDs.Add(temp);
                StudiosIDs_Upd.Add(temp);
                chr = App.GetStudioName(temp);
                Studio.Text += new string(chr);
                if (i < IDs[0] - 1) Studio.Text += ", ";
            }
            StudioUp.Text = Studio.Text;

            TotalEpisodesNum = anime->NumOfEpisodes;
            EpisodeUpTB.Text = TotalEpisodesNum.ToString();
        }
        private unsafe void SaveButton_Click(object sender, RoutedEventArgs e)
        {          
            if (AnimeID < 1)
            {
                App.anime_page anime = new App.anime_page();
                char[] ch = DescriptionTBUp.Text.ToCharArray();
                App.char_to_byte(ch, anime.description, 4000);
                anime.id = 0;
                ch = TitleUp.Text.ToCharArray();
                App.char_to_byte(ch, anime.name, 120);
                anime.NumOfEpisodes = TotalEpisodesNum;
                anime.release_year = ReleaseYear;
                AnimeID = App.BindAnimePage(&anime);
                if (GenresIDs_Upd.Count > 0)
                {
                    int[] ids = new int[GenresIDs_Upd.Count];
                    for (int i = 0; i < GenresIDs_Upd.Count; i++) ids[i] = GenresIDs_Upd[i];
                    fixed (int* p = &(ids[0])) { App.BindGenresToAnime(p, GenresIDs_Upd.Count, AnimeID); }
                }
                for (int i = 0; i < StudiosIDs_Upd.Count; i++) App.BindAniStudio(StudiosIDs_Upd[i], AnimeID);
            }
            else
            {
                App.anime_page anime = new App.anime_page();
                char[] ch = DescriptionTBUp.Text.ToCharArray();
                App.char_to_byte(ch, anime.description, 4000);
                anime.id = AnimeID;
                ch = TitleUp.Text.ToCharArray();
                App.char_to_byte(ch, anime.name, 120);
                anime.NumOfEpisodes = TotalEpisodesNum;
                anime.release_year = ReleaseYear;
                App.UpdateAnimePage(&anime);
                if (GenresIDs_Upd.Count > 0)
                {
                    int[] ids = new int[GenresIDs_Upd.Count];
                    for (int i = 0; i < GenresIDs_Upd.Count; i++) ids[i] = GenresIDs_Upd[i];
                    fixed (int* p = &(ids[0])) { App.BindGenresToAnime(p, GenresIDs_Upd.Count, AnimeID); }
                }
                for (int i = 0; i < StudiosIDs_Upd.Count; i++) App.BindAniStudio(StudiosIDs_Upd[i], AnimeID);
            }
            PageDataLoad();
        }

        private unsafe void SaveNewStudio_Click(object sender, RoutedEventArgs e)
        {
            if (NewStudio.Text.Length > 2)
            {
                char[] chr_arr = NewStudio.Text.ToCharArray();
                int new_id = App.AddNewStudio(chr_arr);
                
                char* ch;
                AllStudiosIDs_Upd = new List<int>();
                List<string> list = new List<string>();
                int* ids = App.GetStudioIDs();
                for (int i = 0; i < ids[0]; i++)
                {
                    AllStudiosIDs_Upd.Add(ids[1 + i]);
                    ch = App.GetStudioName(ids[1 + i]);
                    string s = new string(ch);
                    list.Add(s);
                }
                StudioUpCB.ItemsSource = list;
            }
        }
        private unsafe void SaveNewGenre_Click(object sender, RoutedEventArgs e)
        {
            if (NewGenre.Text.Length > 2)
            {
                char[] chr_arr = NewGenre.Text.ToCharArray();
                int new_id = App.AddNewGenre(chr_arr);

                char* ch;
                AllGenresIDs_Upd = new List<int>();
                List<string> list = new List<string>();
                int* ids = App.GetGenresIDs();
                for (int i = 0; i < ids[0]; i++)
                {
                    AllGenresIDs_Upd.Add(ids[1 + i]);
                    ch = App.GetGenreName(ids[1 + i]);
                    string s = new string(ch);
                    list.Add(s);
                }
                GenreUpCB.ItemsSource = list;
            }
        }
        private void DeleteSelectedScreenShot_Click(object sender, RoutedEventArgs e)
        {
            if (AnimeID < 1) return;
            if (ScreenshotListUp.SelectedIndex >= 0 && ScreenshotList.SelectedIndex < ScreenshotsIDs.Count)
            {
                App.DeleteScreenshot(ScreenshotsIDs[ScreenshotListUp.SelectedIndex]);
                LoadScreenshots();
            }
        }
        public unsafe void LoadScreenshots(){
            if (AnimeID < 1) return;
            int* IDs = App.GetScreenshotsIDs(AnimeID);
            int temp;
            screenshot_list = new List<Screenshot>();
            for (int i = 0; i < IDs[0]; i++)
            {
                temp = IDs[i + 1];
                App.DownloadScreenshot(temp);
                ScreenshotsIDs.Add(temp);
                Screenshot scr = new Screenshot();
                scr.bitmap = App.OpenImage("Screenshots\\" + temp.ToString() + ".png");
                screenshot_list.Add(scr);
            }
            ScreenshotList.ItemsSource = screenshot_list;
            ScreenshotListUp.ItemsSource = screenshot_list;
        }
        private void AddNewScreenShot_Click(object sender, RoutedEventArgs e)
        {
            if (AnimeID < 1) return;
            var fileDialog = new OpenFileDialog();
            fileDialog.ShowDialog();
            if (fileDialog.CheckFileExists)
            {
                string file = fileDialog.FileName;
                char[] ch = file.ToCharArray();
                App.UploadScreenshot(AnimeID, ch);
                LoadScreenshots();
            }
        }

        public unsafe void LoadEpisodes()
        {
            int* IDs = App.GetEpisodesIDs(AnimeID);
            int temp;
            Episode.Text = "Серий: " + IDs[0].ToString() + "/" + TotalEpisodesNum.ToString();
            episodes_list = new List<EpisodeClass>();
            for (int i = 0; i < IDs[0]; i++)
            {
                temp = IDs[i + 1];
                Episodes.Add(temp);
                EpisodeClass EpisodeItem = new EpisodeClass();
                EpisodeItem.EpisodeID = temp;
                EpisodeItem.EpisodeNumber = i + 1;
                EpisodeItem.EpisodeNumberText = (i + 1).ToString();
                bool dlex = true;
                foreach (App.Loading l in App.loads)
                {
                    if (l.EpisodeID == temp)
                    {
                        if (l.downloading)
                        {
                            EpisodeItem.IsExist = false;
                            EpisodeItem.IsNotExist = false;
                            EpisodeItem.downloading = true;
                            EpisodeItem.loadingPocessID = l.loadingPocessID;
                            EpisodeItem.progrress_visibility = Visibility.Visible;
                            int progress = App.GetLoadingProgress(l.loadingPocessID);
                            EpisodeItem.progress = progress;
                            EpisodeItem.tag = temp.ToString();
                        }
                        else
                        {
                            EpisodeItem.IsExist = File.Exists("Episodes\\" + temp.ToString() + ".mp4");
                            EpisodeItem.IsNotExist = !(EpisodeItem.IsExist);
                            EpisodeItem.downloading = false;
                            EpisodeItem.loadingPocessID = l.loadingPocessID;
                            EpisodeItem.progrress_visibility = Visibility.Hidden;
                            int progress = 100;
                            EpisodeItem.progress = progress;
                            EpisodeItem.tag = temp.ToString();
                        }
                        dlex = false;
                    }
                }
                if (dlex)
                {
                    EpisodeItem.IsExist = File.Exists("Episodes\\" + temp.ToString() + ".mp4");
                    EpisodeItem.IsNotExist = !(EpisodeItem.IsExist);
                    EpisodeItem.downloading = false;
                    EpisodeItem.loadingPocessID = -100;
                    EpisodeItem.progrress_visibility = Visibility.Hidden;
                    int progress = 0;
                    EpisodeItem.progress = progress;
                    EpisodeItem.tag = temp.ToString();
                }
                episodes_list.Add(EpisodeItem);
            }
            EpisodesLV.ItemsSource = episodes_list;
        }
        private void AddNewEpisode_Click(object sender, RoutedEventArgs e)
        {
            if (AnimeID < 1) return;
            var fileDialog = new OpenFileDialog();
            fileDialog.ShowDialog();
            if (fileDialog.CheckFileExists)
            {
                string file = fileDialog.FileName;
                char[] ch = file.ToCharArray();
                App.uploadEpisode(AnimeID, Episodes.Count, ch);
                LoadEpisodes();
            }
        }

        private void DeleteSelectedEpisode_Click(object sender, RoutedEventArgs e)
        {
            if (AnimeID < 1) return;

        }

        private void Changing_Click(object sender, RoutedEventArgs e)
        {
            UpdateGrid.Visibility = Visibility.Visible;
            ViewGrid.Visibility = Visibility.Hidden;
        }
        private void ReturnFromChanging_Click(object sender, RoutedEventArgs e)
        {
            UpdateGrid.Visibility = Visibility.Hidden;
            ViewGrid.Visibility = Visibility.Visible;
        }

    }
}
