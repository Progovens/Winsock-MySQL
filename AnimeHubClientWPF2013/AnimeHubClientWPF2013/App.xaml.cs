using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media.Imaging;

namespace AnimeHubClientWPF2013
{
    public partial class App : Application
    {
        public unsafe struct anime_page
        {
            public int id;
            public fixed byte name[128];
            public int release_year;
            public fixed byte description[4096];
            public int NumOfEpisodes;
            public fixed byte LastUpdTime[32];
        };

        public unsafe struct filter
        {
        	//год, жанры, студии, имя, число серий и как сортировать
            public int MaxYear;
            public int MinYear;
            public int IsThereName;
            public fixed byte AniName[256];
            public int Studio;
            public int Genre;
            public int MinEp;
            public int MaxEp;
            public int SortType; // 0 - alfavit   1 - last upd date    2 - rating
        };
        
        //connection and login&registration
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe bool IsConnectedToServer();
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe bool LogIn(char[] Login, char[] Password);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void LogOut();
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe bool Register(char[] Login, char[] Password);
        //Catalog
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int* stock_catalog();
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int* catalog_after_filter(filter* Filter);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int GetGetMaxAnimeYear();
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int GetMinAnimeYear();
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int RandomAnimeID();
        //Genre
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int* GetGenresIDs();
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe char* GetGenreName(int GenreID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int* GetAnimeGenresIDs(int animeID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void BindGenresToAnime(int* GenresIdsArray, int GenresNum, int animeID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int AddNewGenre(char[] genreName);
        //Studio
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe char* GetStudioName(int studioID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int AddNewStudio(char[] studio_name);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void DeleteStudio(int studioID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int* GetStudioIDs();
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int* GetAniStudioIDs(int animeID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void BindAniStudio(int studioID, int animeID);
        //Poster
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int BindPoster(int animeID, char[] posterPath);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void GetPoster(int posterID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void DeletePoster(int posterID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int GetPosterID(int animeID);
        //Screenshots
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int* GetScreenshotsIDs(int animeID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int UploadScreenshot(int animeID, char[] screenshotPath);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe char* DownloadScreenshot(int screenshotID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void DeleteScreenshot(int screenshotID);
        //Score & vote
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe float GetAnimeScore(int animeID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe float vote(int animeID, int score);
        //AnimePage
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe anime_page* getAnimePage(int animeID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int BindAnimePage(anime_page* anipage);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void UpdateAnimePage(anime_page* anipage);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void DeleteAnimePage(int aniPageID);
        //Episodes
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void DeleteEpisode(int episodeID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int* GetEpisodesIDs(int animeID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int uploadEpisode(int animeID, int Num, char[] EpisodePath);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int downloadEpisode(int episodeID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int GetLoadingProgress(int DLprogrID);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe int GetUploadingProgress(int DLprogrID);

        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe void char_to_byte(char[] ch, byte* b, int len);
        [DllImport("/ClientDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern unsafe char* byte_to_char(byte* b, int len);


        public static MainWindow mp;
        public static bool LoggedIn = false;
        public static bool ConnectedToServer = false;
        public static string UserName = "";
        public static int AnimePageID;

        public static string AnimeVideoPath = "";
        //
        public class Loading
        {
            public bool downloading;
            public int EpisodeID;
            public int AnimeID;
            public string AnimeName;
            public int EpisodeNumber;
            public int loadingPocessID;
        }
        public static List<Loading> loads = new List<Loading>();

        public static BitmapImage OpenImage(string filename)
        {
            if (File.Exists(filename))
            {
                var fstream = new FileStream(filename, FileMode.Open);
                MemoryStream mstream = new MemoryStream();
                mstream.SetLength(fstream.Length);
                fstream.Read(mstream.GetBuffer(), 0, (int)fstream.Length);
                fstream.Close();

                BitmapImage bi = new BitmapImage();
                bi.BeginInit();
                bi.StreamSource = mstream;
                bi.EndInit();
                return bi;
            }
            else
            {
                var fstream = new FileStream("placeholder_image.png", FileMode.Open);
                MemoryStream mstream = new MemoryStream();
                mstream.SetLength(fstream.Length);
                fstream.Read(mstream.GetBuffer(), 0, (int)fstream.Length);
                fstream.Close();

                BitmapImage bi = new BitmapImage();
                bi.BeginInit();
                bi.StreamSource = mstream;
                bi.EndInit();
                return bi;
            }
            
        }

    }
}
