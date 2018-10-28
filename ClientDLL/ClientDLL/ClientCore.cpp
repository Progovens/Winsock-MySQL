#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <direct.h>
#include <boost\thread.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define CLIENT_CONSOLE_TITLE "AnimeHub client v0.1 pre alpha"

#define client_disconnection_code 1

#define client_registration_code 2

#define client_login_code 3
#define client_logout_code 31

#define client_catalog_code 4
#define client_catalog_stock_code 41
#define client_catalog_filter_code 42

#define client_anipage_upload_code 52
#define client_anipage_update_code 522
#define client_anipage_download_code 53
#define client_anipage_delete_code 534

#define client_poster_bind_code 6
#define client_poster_download_code 62
#define client_poster_delete_code 623543
#define client_poster_get_id_code 689832

#define client_screenshot_all_ids_code 61
#define client_screenshot_upload_code 63
#define client_screenshot_download_code 64
#define client_screenshot_delete_code 654353

#define client_vote_code 673
#define client_anime_score_code 674

#define client_episode_get_ids_code 71231 
#define client_episode_upload_code 7345356 
#define client_episode_download_code 7654652
#define client_episode_delete_code 71234

#define client_studio_name_code 212221
#define client_studio_add_code 212222
#define client_studio_ids_code 212223
#define client_studio_anime_ids_code 212264523 //=====
#define client_studio_anime_bind_code 26458583 //=====
#define client_studio_delete_code 21252554 

#define client_genre_add_code 2122221
#define client_genre_all_ids_code 2122222
#define client_genre_name_code 2122223
#define client_genre_anime_all_code 2122224
#define client_genre_anime_bind_code 2122225
#define client_genre_anime_delete_code 24352225 //=======

#define client_min_aniYear_code 908625 
#define client_max_aniYear_code 78952225 


//#define SERVER_ADDR "192.168.100.4"
#define SERVER_ADDR "25.0.135.87"
#define LOADING_THREADS_NUM_MAX 10
#define CLIENT_PORT 1234
#define SERVER_PORT 1235
#define DEFAULT_PORT 17010
#define SERVER_PORT1 17200

#define MAX_TRANSACTIONS_NUM 64

struct filter{
	//год, жанры, студии, имя, число серий и как сортировать
	int MaxYear;
	int MinYear;
	int IsThereName;
	char AniName[256];
	int Studio;
	int Genre;
	int MinEp;
	int MaxEp;
	int SortType; // 0 - alfavit   1 - last upd date    2 - rating
};

struct anime_page{
	int id;
	char name[128];
	int release_year;
	char description[4096];
	int NumOfEpisodes;
	char LastUpdTime[32];
};

#define CONNECTION_CODE 1
#define RECONNECTION_CODE 0

#define CLIENT_INVALID_ID 0xffffffff


class STACK_UINT {
private:
	boost::mutex Mutex;
	UINT NumOfElements;
	UINT MaxNumOfElements;
	UINT* ElementsMassive;
public:
	STACK_UINT(){
		NumOfElements = 0;
		MaxNumOfElements = 1000;
		ElementsMassive = new UINT[1000];	
		Mutex.initialize();
	}
	~STACK_UINT(){
		delete[] ElementsMassive;
		ElementsMassive = NULL;
		MaxNumOfElements = 0;
		NumOfElements = 0;
	}
	UINT GetMaxNumOfElements(){
		return MaxNumOfElements;
	}
	UINT GetNumOfFreeCells(){
		return MaxNumOfElements - NumOfElements;
	}
	UINT GetCurNumOfElements(){
		return NumOfElements;
	}
	bool AddElement(UINT Element){
		if (NumOfElements >= MaxNumOfElements) return false;
		Mutex.lock();
		ElementsMassive[NumOfElements] = Element;
		NumOfElements++;
		Mutex.unlock();
		return true;
	}
	UINT ExtractElement(){
		if (NumOfElements == 0) return 0xfffffff;
		Mutex.lock();
		NumOfElements--;
		UINT Element = ElementsMassive[NumOfElements];
		Mutex.unlock();
		return Element;
	}
	void clear(){
		Mutex.lock();
		NumOfElements = 0;
		Mutex.unlock();
	}
};

class SERVER;

#define DATA_POCKET_SIZE  65536
class EPISODE_LOADING{
private:	
	boost::thread EpisodeTranactionThread;
	static void EpisodeUploadingThreadFunction(EPISODE_LOADING* pEpLoad, char* path, UINT AnimeID, UINT Num);
	static void EpisodeDowloadingThreadFunction(EPISODE_LOADING* pEpLoad, UINT EpisodeID);
	SERVER* server;
	int Progress;
	UINT id;
public:
	EPISODE_LOADING(UINT ID, SERVER* pServer, char* path, UINT AnimeID, UINT Num){
		server = pServer;
		id = ID;
		Progress = 0;
		EpisodeTranactionThread = boost::thread(&EpisodeUploadingThreadFunction, this, path, AnimeID, Num);
	}
	EPISODE_LOADING(UINT ID, SERVER* pServer, UINT EpisodeID){
		server = pServer;
		id = ID;
		Progress = 0;
		EpisodeTranactionThread = boost::thread(&EpisodeDowloadingThreadFunction, this, EpisodeID);
	}
	~EPISODE_LOADING();
	int GetProgress();
};

class SERVER{
private:
	hostent *hp;
	sockaddr_in clnt_sin, srv_sin;
	int downloading_threads_num;
	int uploading_threads_num;
	bool connected;
	UINT ClientID;
	UINT ClientID_DB;	
	
	bool connect_to_server(){
		connected = false;
		WSADATA wsaData;
		if (WSAStartup(WINSOCK_VERSION, &wsaData)){
			printf ("Socket API initalization error!\n");
			return false;
		}
		else printf ("Socket API initalization success.\n");
		
		MainSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (MainSocket == INVALID_SOCKET) {
            printf("Socket failed with error: %ld\n", WSAGetLastError());
			return false;
        }
		clnt_sin.sin_family = AF_INET;
		clnt_sin.sin_addr.s_addr = INADDR_ANY;
		clnt_sin.sin_port = CLIENT_PORT;
		int iResult;
		srv_sin.sin_family = AF_INET;
		inet_pton(AF_INET, SERVER_ADDR, &(srv_sin.sin_addr));
		srv_sin.sin_port = htons(DEFAULT_PORT);
		iResult = connect(MainSocket, (sockaddr*)&srv_sin, sizeof(srv_sin));
		if (iResult == SOCKET_ERROR) {
            printf("Connection failure!\n");
			connected = false;
			return false;
        }
		connected = true;
		return true;
	}
	bool Reconnect(){
		while(!(connect_to_server())) {	Sleep(100); }
		UINT buf[2] = {RECONNECTION_CODE, ClientID};
		send(MainSocket, (char*)buf, sizeof(UINT) * 2, NULL);
		recv(MainSocket, (char*)&ClientID, sizeof(UINT), NULL);
		if (ClientID == CLIENT_INVALID_ID) {
			connected = false;
			return false;
		}
		printf("Client ID: %u reconnected!\n", ClientID);
		connected = true;
		return true;
	}
public:
	STACK_UINT LoadsFreePointersStack;
	EPISODE_LOADING* Loads[1000];
	SOCKET MainSocket;
	SOCKET listen_socket;
	SERVER(){
		setlocale(LC_ALL, "russian");
		downloading_threads_num = 0;
		uploading_threads_num = 0;
		MainSocket = INVALID_SOCKET;
		start_listen_socket();
		_mkdir("Posters");
		_mkdir("Screenshots");
		_mkdir("Episodes");
		if(connect_to_server()){
			UINT buf[2] = {CONNECTION_CODE, 0};
			send(MainSocket, (char*)buf, sizeof(UINT) * 2, NULL);
			recv(MainSocket, (char*)&ClientID, sizeof(UINT), NULL);
			printf("Client ID: %u\n", ClientID);
			//MainSocketThread = boost::thread(MainSocketThreadFunction, this);
		}
		for(int i = 0; i < 1000; i++) LoadsFreePointersStack.AddElement(i);
	}
	~SERVER(){
		if (MainSocket != INVALID_SOCKET){ 
			UINT buffer = client_disconnection_code;
			send(MainSocket, (char*)&buffer, sizeof(UINT), NULL);
			closesocket(MainSocket);
		}
		WSACleanup();
	}
	bool start_listen_socket(){
		int iResult;
		WSADATA wsaData;
		if (WSAStartup(WINSOCK_VERSION, &wsaData)){
			printf ("Socket API initalization error!\n");
			WSACleanup();
			system("pause");
			return false;
		}
		
		sockaddr_in srv_sin;
		srv_sin.sin_addr.s_addr = INADDR_ANY;	
		srv_sin.sin_family = AF_INET;
		srv_sin.sin_port = htons(SERVER_PORT1);
		
		listen_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (listen_socket == INVALID_SOCKET){
			printf("listen_socket creation error!\n");
			system("pause");
			WSACleanup();
			return false;
		}
		
		iResult = bind(listen_socket, (sockaddr*)&srv_sin, sizeof(srv_sin));
		if(iResult == SOCKET_ERROR){
			printf("listen_socket bind() error!\n");
			system("pause");
			closesocket(listen_socket);
			WSACleanup();
			return false;
		}
		
		iResult = listen(listen_socket, SOMAXCONN); //второй аргумент - максимальная длина очереди ожидающих подключения
		if (iResult == SOCKET_ERROR) {
			printf("listen() failed with error: %d\n", WSAGetLastError());
			system("pause");
			closesocket(listen_socket);
			WSACleanup();
			return false;
		}
		return true;
	}	

	bool IsConnected(){
		return connected;
	}
	bool LogIn(char* username, char* password){
		char UserName[128];
		char Password[128];
		sprintf_s(UserName, "%s", username);
		sprintf_s(Password, "%s", password);
		UINT buf = client_login_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, UserName, 128, NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, Password, 128, NULL);
		recv(MainSocket, (char*)&ClientID_DB, sizeof(UINT), NULL);
		return ClientID_DB != CLIENT_INVALID_ID;
	}
	void LogOut(){
		UINT buf = client_logout_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		ClientID_DB = CLIENT_INVALID_ID;
	}
	bool Register(char* username, char* password){
		char UserName[128];
		char Password[128];
		sprintf_s(UserName, "%s", username);
		sprintf_s(Password, "%s", password);
		UINT buf = client_registration_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, UserName, 128, NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, Password, 128, NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		return buf != CLIENT_INVALID_ID;
	}

	float GetAnimeScore(int animeID){
		UINT buf = client_anime_score_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = animeID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		float score;
		recv(MainSocket, (char*)&score, sizeof(float), NULL);
		return score;
	}
	float vote(int animeID, int score){
		if (ClientID_DB == CLIENT_INVALID_ID) return GetAnimeScore(animeID);
		UINT buf = client_vote_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = animeID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = score;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		float score_f;
		recv(MainSocket, (char*)&score_f, sizeof(float), NULL);
		return score_f;
	}

	char* GetStudioName(int studioID){
		char *StudioName = new char[128];
		UINT buf = client_studio_name_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = studioID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, StudioName, 128, NULL);
		return StudioName;
	}
	int AddNewStudio(char* studio_name){
		char StudioName[128];
		_snprintf_c(StudioName, 128, "%s", studio_name);
		UINT buf = client_studio_add_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, StudioName, 128, NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		return (int)buf;
	}
	void DeleteStudio(int studioID){
		UINT buf = client_studio_delete_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		UINT StudioID = studioID;
		send(MainSocket, (char*)&StudioID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(int), NULL);
	}
	int* GetStudioIDs(){
		UINT buf = client_studio_ids_code;
		int* StudioIDs = NULL;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		StudioIDs = new int[buf + 1];
		StudioIDs[0] = buf;
		if (buf == 0) return StudioIDs;
		send(MainSocket, (char*)&buf, sizeof(int), NULL);
		recv(MainSocket, (char*)&StudioIDs[1], sizeof(int)*buf, NULL);
		return StudioIDs;
	}
	int* GetAniStudioIDs(int animeID){
		UINT buf = client_studio_anime_ids_code;
		int* StudioIDs = NULL;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = animeID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		StudioIDs = new int[buf + 1];
		StudioIDs[0] = buf;
		if (buf == 0) return StudioIDs;
		send(MainSocket, (char*)&buf, sizeof(int), NULL);
		recv(MainSocket, (char*)&StudioIDs[1], sizeof(int)*buf, NULL);
		return StudioIDs;
	}
	void BindAniStudio(int studioID, int animeID){
		UINT buf = client_studio_anime_bind_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = studioID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = animeID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
	}

	int BindPoster(int animeID, char* posterPath){
		char* posterData;
		FILE* file;
		fopen_s(&file, posterPath, "rb");
		if(!file) return 0; 
		fseek(file, 0, SEEK_END);
		int filesize = ftell(file);
		rewind(file);
		posterData = (char*)malloc(filesize);
		fread_s(posterData, filesize, filesize, 1, file);
		fclose(file);
		
		UINT buf = client_poster_bind_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		UINT AnimeID = animeID;
		send(MainSocket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&AnimeID, sizeof(UINT), NULL);
		send(MainSocket, (char*)&filesize, sizeof(int), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, (char*)posterData, filesize, NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		free(posterData);
		return (int)buf;
	}
	void GetPoster(int posterID){
		UINT buf = client_poster_download_code;
		int filesize;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		UINT PosterID = posterID;
		send(MainSocket, (char*)&PosterID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&filesize, sizeof(int), NULL);
		if (filesize == 0) return;
		char* posterData = (char*)malloc(filesize);
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)posterData, filesize, NULL);
		
		char posterPath[128];
		_snprintf_c(posterPath, 128, "Posters\\%u.png", PosterID);
		FILE* file;
		fopen_s(&file, posterPath, "wb"); 
		fwrite(posterData, filesize, 1, file);
		fclose(file);
		free(posterData);
	}
	void DeletePoster(int posterID){
		UINT buf = client_poster_download_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		UINT PosterID = posterID;
		send(MainSocket, (char*)&PosterID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(int), NULL);
	}//также добавить то чтобы если картинки нет то вставала дефолтная
	int GetPosterID(int animeID){
		UINT buf = client_poster_get_id_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		UINT AnimeID = animeID;
		UINT PosterID;
		send(MainSocket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&PosterID, sizeof(UINT), NULL);
		return (int)PosterID;
	}

	int* GetScreenshotsIDs(int animeID){
		int* ScreenshotsIDs;
		int Num;
		UINT buf = client_screenshot_all_ids_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = animeID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&Num, sizeof(int), NULL);
		ScreenshotsIDs = new int[Num + 1];
		ScreenshotsIDs[0] = Num;
		if (Num == 0) return ScreenshotsIDs;
		send(MainSocket, (char*)&Num, sizeof(int), NULL);
		recv(MainSocket, (char*)&ScreenshotsIDs[1], sizeof(int)*Num, NULL);
		return ScreenshotsIDs;
	}
	int UploadScreenshot(int animeID, char* screenshotPath){
		char* screenshotData;
		FILE* file;
		fopen_s(&file, screenshotPath, "rb");
		if(!file) return 0; 
		fseek(file, 0, SEEK_END);
		int filesize = ftell(file);
		rewind(file);
		screenshotData = (char*)malloc(filesize);
		fread_s(screenshotData, filesize, filesize, 1, file);
		fclose(file);
		
		UINT buf = client_screenshot_upload_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		UINT AnimeID = animeID;
		send(MainSocket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&AnimeID, sizeof(UINT), NULL);
		send(MainSocket, (char*)&filesize, sizeof(int), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, (char*)screenshotData, filesize, NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		free(screenshotData);
		return (int)buf;
	}
	void DownloadScreenshot(int screenshotID){
		UINT buf = client_screenshot_download_code;
		UINT ScreenshotID = screenshotID;
		int filesize;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, (char*)&ScreenshotID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&filesize, sizeof(int), NULL);
		char* ScreenshotData = (char*)malloc(filesize);
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)ScreenshotData, filesize, NULL);
		
		char ScreenshotPath[128];
		_snprintf_c(ScreenshotPath, 128, "Screenshots\\%u.png", ScreenshotID);
		FILE* file;
		fopen_s(&file, ScreenshotPath, "wb"); 
		fwrite(ScreenshotData, filesize, 1, file);
		fclose(file);
		free(ScreenshotData);
	}
	void DeleteScreenshot(int screenshotID){
		UINT buf = client_screenshot_delete_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		UINT ScreenshotID = screenshotID;
		send(MainSocket, (char*)&ScreenshotID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(int), NULL);
	}
	
	void DeleteEpisode(int episodeID){
		UINT buf = client_episode_delete_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = episodeID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
	}
	int* GetEpisodesIDs(int animeID){
		int* EpisodesIDs = NULL;
		int EpNum;
		UINT buf = client_episode_get_ids_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = animeID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&EpNum, sizeof(int), NULL);
		EpisodesIDs = new int[EpNum + 1];
		EpisodesIDs[0] = EpNum;
		if (EpNum == 0) return EpisodesIDs;
		send(MainSocket, (char*)&EpNum, sizeof(int), NULL);
		recv(MainSocket, (char*)&EpisodesIDs[1], sizeof(int)*EpNum, NULL);
		return EpisodesIDs;
	}
	int uploadEpisode(int animeID, int Num, char* EpisodePath){
		int ID = LoadsFreePointersStack.ExtractElement();
		Loads[ID] = new EPISODE_LOADING(ID, this, EpisodePath, animeID, Num);
		return ID;
	}
	int downloadEpisode(int episodeID){
		UINT buf = client_episode_download_code;
		int ID = LoadsFreePointersStack.ExtractElement();
		Loads[ID] = new EPISODE_LOADING(ID, this, episodeID);
		return ID;
	}
	int GetLoadingProgress(int DLprogrID){
		return Loads[DLprogrID]->GetProgress();
	}

	anime_page* getAnimePage(int animeID){
		UINT buf = client_anipage_download_code;
		anime_page* anipage = new anime_page;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = animeID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)anipage, sizeof(anime_page), NULL);
		return anipage;
	}
	int BindAnimePage(anime_page* anipage){
		UINT buf = client_anipage_upload_code;	
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, (char*)anipage, sizeof(anime_page), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		return (int)buf;
	}
	void UpdateAnimePage(anime_page* anipage){
		UINT buf = client_anipage_update_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, (char*)anipage, sizeof(anime_page), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
	}
	void DeleteAnimePage(int aniPageID){
		UINT buf = client_anipage_delete_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		UINT AniPageID = aniPageID;
		send(MainSocket, (char*)AniPageID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
	}
	
	char* GetGenreName(int genreID){
		char *GenreName = new char[128];
		UINT buf = client_genre_name_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = genreID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, GenreName, 128, NULL);
		return GenreName;
	}
	int* GetGenresIDs(){
		int* GenreIDsMassive;
		int Num;
		UINT buf = client_genre_all_ids_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&Num, sizeof(int), NULL);
		GenreIDsMassive = new int[Num + 1];
		GenreIDsMassive[0] = Num;
		if (Num == 0) return GenreIDsMassive;
		send(MainSocket, (char*)&Num, sizeof(int), NULL);
		recv(MainSocket, (char*)&GenreIDsMassive[1], sizeof(int)*Num, NULL);
		return GenreIDsMassive;
	}
	int* GetAnimeGenresIDs(int animeID){
		int* GenreIDsMassive;
		int Num;
		UINT buf = client_genre_anime_all_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		buf = animeID;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&Num, sizeof(int), NULL);
		GenreIDsMassive = new int[Num + 1];
		GenreIDsMassive[0] = Num;
		if (Num == 0) return GenreIDsMassive;
		send(MainSocket, (char*)&Num, sizeof(int), NULL);
		recv(MainSocket, (char*)&GenreIDsMassive[1], sizeof(int)*Num, NULL);
		return GenreIDsMassive;
	}
	int AddNewGenre(char* genreName){
		if (ClientID_DB == CLIENT_INVALID_ID) return CLIENT_INVALID_ID;
		UINT GenreID;
		char GenreName[128];
		_snprintf_c(GenreName, 128, "%s", genreName);
		UINT buf = client_genre_add_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, (char*)&GenreName, 128, NULL);
		recv(MainSocket, (char*)&GenreID, sizeof(UINT), NULL);
		return (int)GenreID;
	}
	void BindGenresToAnime(int* GenresIdsArray, int GenresNum, int animeID){
		if (ClientID_DB == CLIENT_INVALID_ID) return;
		UINT *GenreID = new UINT[GenresNum];
		for (int i = 0; i < GenresNum; i++) GenreID[i] = GenresIdsArray[i]; 
		UINT AnimeID = animeID;
		UINT Num = GenresNum;
		UINT buf = client_genre_anime_bind_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&AnimeID, sizeof(UINT), NULL);
		send(MainSocket, (char*)&GenresNum, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&GenresNum, sizeof(UINT), NULL);
		send(MainSocket, (char*)GenreID, sizeof(UINT) * GenresNum, NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
	}
	
	int* catalog_after_filter(filter* Filter){//возвращает список айдишников анимешек, первый инт - число айдишников
		int* aniPageIDs = NULL;
		int apNum;
		UINT buf = client_catalog_filter_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		send(MainSocket, (char*)Filter, sizeof(filter), NULL);
		recv(MainSocket, (char*)&apNum, sizeof(int), NULL);
		aniPageIDs = new int[apNum + 1];
		aniPageIDs[0] = apNum;
		if (apNum == 0) return aniPageIDs;
		send(MainSocket, (char*)&apNum, sizeof(int), NULL);
		recv(MainSocket, (char*)&aniPageIDs[1], sizeof(int)*apNum, NULL);
		return aniPageIDs;
	}
	int* stock_catalog(){//возвращает список айдишников анимешек
		int* aniPageIDs = NULL;
		int apNum;
		UINT buf = client_catalog_stock_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&apNum, sizeof(int), NULL);
		aniPageIDs = new int[apNum + 1];
		aniPageIDs[0] = apNum;
		if (apNum == 0) return aniPageIDs;
		send(MainSocket, (char*)&apNum, sizeof(int), NULL);
		recv(MainSocket, (char*)&aniPageIDs[1], sizeof(int)*apNum, NULL);
		return aniPageIDs;
	}	
	int GetMaxAnimeYear(){
		UINT buf = client_max_aniYear_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		return (int)buf;
	}
	int GetMinAnimeYear(){
		UINT buf = client_min_aniYear_code;
		send(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		recv(MainSocket, (char*)&buf, sizeof(UINT), NULL);
		return (int)buf;
	}
};

void EPISODE_LOADING::EpisodeUploadingThreadFunction(EPISODE_LOADING* pEpLoad, char* path, UINT AnimeID, UINT Num){
	FILE* file;
	file = fopen(path, "rb");
	if (!file) {pEpLoad->~EPISODE_LOADING(); return;}
	fseek(file, 0, SEEK_END);
	int filesize = ftell(file);
	rewind(file);
	int NumOfPockets = filesize / DATA_POCKET_SIZE;
	if (filesize % DATA_POCKET_SIZE) NumOfPockets += 1;
	char* EpisodeData = (char*)malloc(NumOfPockets * DATA_POCKET_SIZE);
	fread_s(EpisodeData, filesize, filesize, 1, file);
	fclose(file);
	
	UINT buf = client_episode_upload_code;
	send(pEpLoad->server->MainSocket, (char*)&buf, sizeof(UINT), NULL);
	
	SOCKET new_socket;
	do{
		new_socket = INVALID_SOCKET;
		new_socket = accept(pEpLoad->server->listen_socket, NULL, NULL);
		Sleep(1);	
	} while(new_socket == INVALID_SOCKET);
	
	buf = AnimeID;
	send(new_socket, (char*)&buf, sizeof(UINT), NULL);
	recv(new_socket, (char*)&buf, sizeof(UINT), NULL);
	buf = Num;
	send(new_socket, (char*)&buf, sizeof(UINT), NULL);
	recv(new_socket, (char*)&buf, sizeof(UINT), NULL);
	buf = filesize;
	send(new_socket, (char*)&buf, sizeof(UINT), NULL);
	recv(new_socket, (char*)&buf, sizeof(UINT), NULL);
	
	for (int i = 0; i < NumOfPockets; i++){
		send(new_socket, EpisodeData + DATA_POCKET_SIZE * i, DATA_POCKET_SIZE, NULL);
		recv(new_socket, (char*)&buf, sizeof(UINT), NULL);
		pEpLoad->Progress = 100 * i /  NumOfPockets;
	}
	free(EpisodeData);
	pEpLoad->Progress = 100;
	closesocket(new_socket);
	printf("Episode uploaded\n");
}
void EPISODE_LOADING::EpisodeDowloadingThreadFunction(EPISODE_LOADING* pEpLoad, UINT EpisodeID){
	UINT buf = client_episode_download_code;
	send(pEpLoad->server->MainSocket, (char*)&buf, sizeof(UINT), NULL);
	
	SOCKET new_socket;
	do{
		new_socket = INVALID_SOCKET;
		new_socket = accept(pEpLoad->server->listen_socket, NULL, NULL);
		Sleep(1);	
	} while(new_socket == INVALID_SOCKET);
	
	buf = EpisodeID;
	send(new_socket, (char*)&buf, sizeof(UINT), NULL);
	recv(new_socket, (char*)&buf, sizeof(UINT), NULL);
	int filesize = buf;
	int NumOfPockets = filesize / DATA_POCKET_SIZE;
	if (filesize % DATA_POCKET_SIZE) NumOfPockets += 1;
	char* EpisodeData = (char*)malloc(DATA_POCKET_SIZE);
	char path[128];
	_snprintf_c(path, 128, "Episodes\\%u.mp4", EpisodeID);
	FILE* file;
	fopen_s(&file, path, "wb");
	for (int i = 0; i < NumOfPockets; i++){
		recv(new_socket, EpisodeData, DATA_POCKET_SIZE, NULL);
		send(new_socket, (char*)&buf, sizeof(UINT), NULL);
		if (i < NumOfPockets - 1) fwrite(EpisodeData, DATA_POCKET_SIZE, 1, file);
		else fwrite(EpisodeData, filesize - DATA_POCKET_SIZE * i, 1, file);
		pEpLoad->Progress = 100 * i /  NumOfPockets;
	}
	
	//fwrite(EpisodeData, filesize, 1, file);
	fclose(file);
	free(EpisodeData);
	closesocket(new_socket);
	pEpLoad->Progress = 100;
	printf("Episode downloaded\n");
}

EPISODE_LOADING::~EPISODE_LOADING(){
	server->Loads[id] = NULL;
	server->LoadsFreePointersStack.AddElement(id);
	EpisodeTranactionThread.interrupt();
}
int EPISODE_LOADING::GetProgress(){
	int pr = Progress;
	//this->~EPISODE_LOADING();
	return pr;
}

SERVER server;

extern "C"{
//Connection? login&register
__declspec(dllexport) int IsConnectedToServer(){
	return (server.IsConnected())? 1: 0;
}
__declspec(dllexport) int LogIn(char Login[], char Password[]){
	return (server.LogIn(Login, Password))? 1: 0;
}
__declspec(dllexport) void LogOut(){
	server.LogOut();
}
__declspec(dllexport) int Register(char Login[], char Password[]){
	return (server.Register(Login, Password))? 1: 0;
}
//Catalog
__declspec(dllexport) int* stock_catalog(){
	return server.stock_catalog();
}
__declspec(dllexport) int* catalog_after_filter(filter* Filter){
	return server.catalog_after_filter(Filter);
}
__declspec(dllexport) int GetGetMaxAnimeYear(){
	return server.GetMaxAnimeYear();
}
__declspec(dllexport) int GetMinAnimeYear(){
	return server.GetMinAnimeYear();
}
__declspec(dllexport) int RandomAnimeID(){
	int* IDs = server.stock_catalog();
	if (IDs[0] <= 0) return -1000;
	srand(time(NULL));
	int ID = IDs[1 + rand() % IDs[0]];
	return ID;
}

//Genres
__declspec(dllexport) int* GetGenresIDs(){
	return server.GetGenresIDs();
}
__declspec(dllexport) char* GetGenreName(int GenreID){
	wchar_t* wchar = new wchar_t[64];
	char* s = server.GetGenreName(GenreID);
	mbstowcs(wchar, s, 64);
	return (char*)wchar;
	//return server.GetGenreName(GenreID);
}
__declspec(dllexport) int* GetAnimeGenresIDs(int animeID){
	return server.GetAnimeGenresIDs(animeID);
}
__declspec(dllexport) void BindGenresToAnime(int GenresIdsArray[], int GenresNum, int animeID){
	return server.BindGenresToAnime(GenresIdsArray, GenresNum, animeID);
}
__declspec(dllexport) int AddNewGenre(char genreName[]){
	return server.AddNewGenre(genreName);
}

//Studio
__declspec(dllexport) char* GetStudioName(int studioID){
	wchar_t* wchar = new wchar_t[64];
	char* s = server.GetStudioName(studioID);
	mbstowcs(wchar, s, 64);
	return (char*)wchar;
}
__declspec(dllexport) int AddNewStudio(char studio_name[]){
	return server.AddNewStudio(studio_name);
}
__declspec(dllexport) void DeleteStudio(int studioID){
	server.DeleteStudio(studioID);
}
__declspec(dllexport) int* GetStudioIDs(){
	return server.GetStudioIDs();
}
__declspec(dllexport) int* GetAniStudioIDs(int animeID){
	return server.GetAniStudioIDs(animeID);
}
__declspec(dllexport) void BindAniStudio(int studioID, int animeID){
	server.BindAniStudio(studioID, animeID);
}

//Poster
__declspec(dllexport) int BindPoster(int animeID, char posterPath[]){
	return server.BindPoster(animeID, posterPath);
}
__declspec(dllexport) void GetPoster(int posterID){
	server.GetPoster(posterID);
}
__declspec(dllexport) void DeletePoster(int posterID){
	server.DeletePoster(posterID);
}
__declspec(dllexport) int GetPosterID(int animeID){
	return server.GetPosterID(animeID);
}

//Screenshots
__declspec(dllexport) int* GetScreenshotsIDs(int animeID){
	return server.GetScreenshotsIDs(animeID);
}
__declspec(dllexport) int UploadScreenshot(int animeID, char screenshotPath[]){
	return server.UploadScreenshot(animeID, screenshotPath);
}
__declspec(dllexport) void DownloadScreenshot(int screenshotID){
	server.DownloadScreenshot(screenshotID);
}
__declspec(dllexport) void DeleteScreenshot(int screenshotID){
	server.DeleteScreenshot(screenshotID);
}

//Score & vote
__declspec(dllexport) float GetAnimeScore(int animeID){
	return server.GetAnimeScore(animeID);
}
__declspec(dllexport) float vote(int animeID, int score){
	return server.vote(animeID, score);
}

//AnimePage
__declspec(dllexport) anime_page* getAnimePage(int animeID){
	return server.getAnimePage(animeID);
}
__declspec(dllexport) int BindAnimePage(anime_page* anipage){
	return server.BindAnimePage(anipage);
}
__declspec(dllexport) void UpdateAnimePage(anime_page* anipage){
	server.UpdateAnimePage(anipage);
}
__declspec(dllexport) void DeleteAnimePage(int aniPageID){
	server.DeleteAnimePage(aniPageID);
}

//Episodes
__declspec(dllexport) void DeleteEpisode(int episodeID){
	server.DeleteEpisode(episodeID);
}
__declspec(dllexport) int* GetEpisodesIDs(int animeID){
	return server.GetEpisodesIDs(animeID);
}
__declspec(dllexport) int uploadEpisode(int animeID, int Num, char EpisodePath[]){
	return server.uploadEpisode(animeID, Num, EpisodePath);
}
__declspec(dllexport) int downloadEpisode(int episodeID){
	return server.downloadEpisode(episodeID);
}
__declspec(dllexport) int GetLoadingProgress(int DLprogrID){
	return server.GetLoadingProgress(DLprogrID);
}



__declspec(dllexport) void char_to_byte(char ch[], byte b[], int len){
	//char* s = new char[1024];
	//_snprintf_c(s, 1024, "%s", ch);
	_snprintf_c((char*)b, len, "%s", ch);
	
	printf("%s\n", (char*)b);
	//return (byte*)s;
}
__declspec(dllexport) char* byte_to_char(byte b[], int len){
	char* s = new char[len];
	_snprintf_c(s, len, "%s", b);
	wchar_t* wchar = new wchar_t[len];
	mbstowcs(wchar, s, len);
	return (char*)wchar;
}
}


//int main(){
//	SetConsoleTitle(CLIENT_CONSOLE_TITLE);
//	//int *i = server.GetGenresIDs();
//	//printf("Жанры\n");
//	//for(int j = 0; j < i[0]; j++){
//	//	char* s = server.GetGenreName(i[j+1]);
//	//	printf("id %i - %s\n", i[j+1], s);
//	//}
//	//
//	////server.AddNewStudio("Aniplex");
//	//i = server.GetStudioIDs();
//	//printf("студии\n");
//	//for(int j = 0; j < i[0]; j++){
//	//	char* s = server.GetStudioName(i[j+1]);
//	//	printf("id %i - %s\n", i[j+1], s);
//	//}
//	//anime_page anipage;
//	//_snprintf_c(anipage.description, 4096, "Академия Сэйкё считается в городе таинственным местом, где всегда происходили разные чудеса и загадочные события. Неудивительно, что в школе существует кружок по изучению паранормальных явлений. И был бы это один из множества ничем не примечательных клубов школы, если бы не одно «но» — учредитель его на самом деле является призраком молодой девушки, оставленной умирать в стенах школы около 60 лет назад.\nЮко Каноэ не помнит, почему умерла и стала призраком. С ней никто не может общаться, кроме двух членов клуба, которые могут её видеть. Это мальчик с экстрасенсорными способностями Тэйити Ниия и родственница Юко в каком-то там поколении Кириэ Каноэ. Призрак просит этих двоих направить клуб на раскрытие её загадочной смерти, ведь Юко неоднократно посещает предчувствие, что её обстоятельства несут какую-то опасность потомкам. И вот смелые исследователи паранормального начинают раскрывать одну страшную загадку за другой, пытаясь приблизиться к тайне смерти Юко.");
//	//_snprintf_c(anipage.name, 128, "Амнезия сумеречной девы - Tasogare Otome x Amnesia");
//	//anipage.NumOfEpisodes = 12;
//	//anipage.release_year = 2013;
//	//int ani_id = server.BindAnimePage(&anipage);
//	//anime_page* a = server.getAnimePage(ani_id);
//	//printf("anime\n%s\nid %i\nгод %i\nэпизодов %i\n%s\n", a->name, a->id, a->release_year, a->NumOfEpisodes, a->description);
//	
//	//int pid = server.BindPoster(8, "1377845457_1.jpg");
//	//pid = server.GetPosterID(8);
//	//server.GetPoster(pid);
//	
//	//server.UploadScreenshot(8, "1377845439_3.jpg");
//	//server.UploadScreenshot(8, "1377845461_2.jpg");
//	//server.UploadScreenshot(8, "1377845521_4.jpg");
//	
//	//int *ids = server.GetScreenshotsIDs(8);
//	//server.DownloadScreenshot(ids[1]);
//	//server.DownloadScreenshot(ids[2]);
//	//server.DownloadScreenshot(ids[3]);
//	//int gen[10] = {46, 41, 58};
//	//server.BindGenresToAnime(gen, 3 , 8);
//	//int st = server.AddNewStudio("Silver Link");
//	//server.BindAniStudio(st, 8);
//	//server.uploadEpisode(8, 1, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\1.mp4");
//	//server.uploadEpisode(8, 2, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\2.mp4");
//	//server.uploadEpisode(8, 3, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\3.mp4");
//	//server.uploadEpisode(8, 4, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\4.mp4");
//	//server.uploadEpisode(8, 5, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\5.mp4");
//	//server.uploadEpisode(8, 6, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\6.mp4");
//	//server.uploadEpisode(8, 7, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\7.mp4");
//	//server.uploadEpisode(8, 8, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\8.mp4");
//	//server.uploadEpisode(8, 9, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\9.mp4");
//	//server.uploadEpisode(8, 10, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\10.mp4");
//	//server.uploadEpisode(8, 11, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\11.mp4");
//	//server.uploadEpisode(8, 12, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\12.mp4");
//	//server.uploadEpisode(8, 13, "Сумрачная дева и амнезия - Tasogare Otome x Amnesia\\13.mp4");
//	//int* ep_ids = server.GetEpisodesIDs(8);
//	//if (ep_ids[0] > 6) server.downloadEpisode(ep_ids[6]);
//	system("pause");
//	server.~SERVER();
//	return 0;
//}