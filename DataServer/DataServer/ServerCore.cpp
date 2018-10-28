#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <iostream>
#include <boost\thread.hpp>
#include <direct.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <mysql.h>
#pragma comment (lib, "libmysql.lib")
//#pragma comment (lib, "mysqlclient.lib")
//#pragma comment (lib, "mysqlcppconn.lib")
//#pragma comment (lib, "mysqlcppconn-static.lib")

#define SERVER_CONSOLE_TITLE "AnimeHub server v0.1 pre alpha"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 17010

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

#define CLIENT_INVALID_ID 0xffffffff


#define DATA_POCKET_SIZE  65536
//#define CLIENT_ADDR "25.1.67.86"
#define CLIENT_ADDR "192.168.100.4"
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
	int SortType; // 0 - alfavit   1 - last upd date
};

struct anime_page{
	int id;
	char name[128];
	int release_year;
	char description[4096];
	int NumOfEpisodes;
	char LastUpdTime[32];
};

class SERVER;
#define CLIENT_PORT 17200


class CLIENT{
private:
	SOCKET cl_socket;
	UINT ClientID;
	SERVER *pServer;
	bool ConnectionFailed;
	UINT ClientID_DB;
	
	MYSQL *connection_mysql = NULL, mysql;
	MYSQL_RES *result_mysql = NULL;
	MYSQL_ROW row;
	int querry_state;

	boost::thread ClientThread;
	boost::thread EpisodeThread;
	
	static void EpisodeSendThreadFunction(CLIENT* pClient){
		sockaddr_in clnt_sin, srv_sin;
		SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
		if (Socket == INVALID_SOCKET) {
            printf("Socket failed with error: %ld\n", WSAGetLastError());
			return;
        }
		clnt_sin.sin_family = AF_INET;
		clnt_sin.sin_addr.s_addr = INADDR_ANY;
		clnt_sin.sin_port = rand()%100 + 1200;
		int iResult;
		srv_sin.sin_family = AF_INET;
		inet_pton(AF_INET, CLIENT_ADDR, &(srv_sin.sin_addr));
		srv_sin.sin_port = htons(CLIENT_PORT);
		iResult = connect(Socket, (sockaddr*)&srv_sin, sizeof(srv_sin));
		
		UINT buf;
		recv(Socket, (char*)&buf, sizeof(UINT), NULL);
		char path[128];
		_snprintf_c(path, 128, "Episodes\\%u.mp4", buf);
		
		FILE* file;
		fopen_s(&file, path, "rb");
		fseek(file, 0, SEEK_END);
		int filesize = ftell(file);
		rewind(file);
		int NumOfPockets = filesize / DATA_POCKET_SIZE;
		if (filesize % DATA_POCKET_SIZE) NumOfPockets += 1;
		char* EpisodeData = (char*)malloc(NumOfPockets * DATA_POCKET_SIZE);
		fread_s(EpisodeData, filesize, filesize, 1, file);
		fclose(file);
		buf = filesize;
		send(Socket, (char*)&buf, sizeof(UINT), NULL);
		for (int i = 0; i < NumOfPockets; i++){
			send(Socket, EpisodeData + DATA_POCKET_SIZE * i, DATA_POCKET_SIZE, NULL);
			recv(Socket, (char*)&buf, sizeof(UINT), NULL);
		}
		free(EpisodeData);
		closesocket(Socket);
		printf("Episode downloaded by user\n");
	}
	static void EpisodeRecvThreadFunction(CLIENT* pClient){
		sockaddr_in clnt_sin, srv_sin;
		SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
		if (Socket == INVALID_SOCKET) {
            printf("Socket failed with error: %ld\n", WSAGetLastError());
			return;
        }
		clnt_sin.sin_family = AF_INET;
		clnt_sin.sin_addr.s_addr = INADDR_ANY;
		clnt_sin.sin_port = rand()%100 + 1200;
		int iResult;
		srv_sin.sin_family = AF_INET;
		inet_pton(AF_INET, CLIENT_ADDR, &(srv_sin.sin_addr));
		srv_sin.sin_port = htons(CLIENT_PORT);
		iResult = connect(Socket, (sockaddr*)&srv_sin, sizeof(srv_sin));
		
		UINT AnimeID;
		recv(Socket, (char*)&AnimeID, sizeof(UINT), NULL);
		send(Socket, (char*)&AnimeID, sizeof(UINT), NULL);
		UINT Num;
		recv(Socket, (char*)&Num, sizeof(UINT), NULL);
		send(Socket, (char*)&Num, sizeof(UINT), NULL);
		UINT filesize;
		recv(Socket, (char*)&filesize, sizeof(UINT), NULL);
		send(Socket, (char*)&filesize, sizeof(UINT), NULL);
		
		UINT EpisodeID;
		char query[256];
		_snprintf_c(query, 256, "DELETE FROM Episode WHERE Anime_AnimeID = %u AND Number = %u", AnimeID, Num);
		int result = mysql_query(pClient->connection_mysql, query);
		_snprintf_c(query, 256, "INSERT INTO Episode (Number, Anime_AnimeID) VALUES(%u, %u)", Num,  AnimeID);
		result = mysql_query(pClient->connection_mysql, query);		
		EpisodeID = mysql_insert_id(pClient->connection_mysql);
		
		int NumOfPockets = filesize / DATA_POCKET_SIZE;
		if (filesize % DATA_POCKET_SIZE) NumOfPockets += 1;
		char* EpisodeData = (char*)malloc(NumOfPockets * DATA_POCKET_SIZE);
		
		for (int i = 0; i < NumOfPockets; i++){
			recv(Socket, EpisodeData + DATA_POCKET_SIZE * i, DATA_POCKET_SIZE, NULL);
			send(Socket, (char*)&AnimeID, sizeof(UINT), NULL);
		}
		
		
		char path[128];
		_snprintf_c(path, 128, "Episodes\\%u.mp4", EpisodeID);
		FILE* file;
		fopen_s(&file, path, "wb");
		fwrite(EpisodeData, filesize, 1, file);
		fclose(file);
		free(EpisodeData);
		closesocket(Socket);
		printf("Episode uploaded\n");
	}

	void AddClientToDisConQueue();
	static void ClientThreadFunction(CLIENT* pClient){
		int res;
		UINT command_code;
		do{
			command_code = client_disconnection_code;
			res = recv(pClient->cl_socket, (char*)&command_code, sizeof(UINT), NULL);
			if (res == 0){
				pClient->ConnectionFailed = true; Sleep(500);
			}
			else{
				switch (command_code){
					case client_disconnection_code : {
						pClient->AddClientToDisConQueue();
						break;
					}
					case client_registration_code : {
						pClient->Register();
						break;
					}
					case client_login_code : {
						pClient->LogIn();
						break;
					}
					case client_logout_code : {
						pClient->LogOut();
						break;
					}
					case client_catalog_stock_code : {
						pClient->stock_catalog();
						break;
					}
					case client_catalog_filter_code : {
						pClient->catalog_after_filter();
						break;
					}
					case client_anipage_upload_code : {
						pClient->BindAniPage();
						break;
					}
					case client_anipage_update_code : {
						pClient->UpdateAniPage();
						break;
					}
					case client_anipage_download_code : {
						pClient->SendAniPage();
						break;
					}
					case client_anipage_delete_code : {
						pClient->DeleteAnimePage();
						break;
					}
					case client_vote_code : {
						pClient->vote();
						break;
					}
					case client_anime_score_code : {
						pClient->SendAnimeScore();
						break;
					}
					case client_poster_bind_code : {
						pClient->BindPoster();
						break;
					}
					case client_poster_get_id_code : {
						pClient->SendPosterID();
						break;
					}
					case client_poster_download_code : {
						pClient->SendPoster();
						break;
					}
					case client_poster_delete_code : {
						pClient->DeletePoster();
						break;
					}
					case client_screenshot_all_ids_code : {
						pClient->SendScreenshotsIDs();
						break;
					}
					case client_screenshot_upload_code : {
						pClient->UploadScreenshot();
						break;
					}
					case client_screenshot_download_code : {
						pClient->SendScreenshot();
						break;
					}
					case client_screenshot_delete_code : {
						pClient->DeleteScreenshot();
						break;
					}
					case client_episode_get_ids_code : {
						pClient->SendEpisodesIDs();
						break;
					}
					case client_episode_upload_code : {
						pClient->uploadEpisode();
						break;
					}
					case client_episode_download_code : {
						pClient->downloadEpisode();
						break;
					}
					case client_episode_delete_code : {
						pClient->DeleteEpisode();
						break;
					}
					case client_studio_name_code : {
						pClient->SendStudioName();
						break;
					}
					case client_studio_add_code : {
						pClient->AddNewStudio();
						break;
					}
					case client_studio_delete_code : {
						pClient->DeleteStudio();
						break;
					}
					case client_studio_ids_code : {
						pClient->SendStudioIDs();
						break;
					}
					case client_studio_anime_ids_code : {
						pClient->SendAniStudioIDs();
						break;
					}
					case client_studio_anime_bind_code : {
						pClient->BindAniStudio();
						break;
					}
					case client_genre_add_code : {
						pClient->AddNewGenre();
						break;
					}
					case client_genre_all_ids_code : {
						pClient->SendGenresIDs();
						break;
					}
					case client_genre_name_code : {
						pClient->SendGenreName();
						break;
					}
					case client_genre_anime_all_code : {
						pClient->SendAnimeGenresIDs();
						break;
					}
					case client_genre_anime_bind_code : {
						pClient->BindGenresToAnime();
						break;
					}
					case client_max_aniYear_code : {
						pClient->SendMaxAnimeYear();
						break;
					}
					case client_min_aniYear_code : {
						pClient->SendMinAnimeYear();
						break;
					}
				}
			}
		} while(command_code != client_disconnection_code && !(pClient->ClientThread.interruption_requested()));
	}
	bool connect_to_database(){
		mysql_init(&mysql);
		connection_mysql = mysql_real_connect(&mysql, "localhost", "root", "", "mydb", 3306, NULL, NULL);
		
		if (!connection_mysql){
			printf("Client id: %u connection to database failed! Error:\n%s\n", ClientID, mysql_error(&mysql));
			return false;
		}
		printf("Client id: %u successfully connected to database.\n", ClientID);
		mysql_query(connection_mysql, "use mydb");
		mysql_query(connection_mysql, "SET NAMES cp1251");
		return true;
	}
	void shutdown_database_connection(){
		mysql_free_result(result_mysql);
		mysql_close(connection_mysql);
	}
	void LogIn(){
		char UserName[128];
		char Password[128];
		UINT buf = client_login_code;
		send(cl_socket, (char*)&buf, sizeof(UINT), NULL);
		recv(cl_socket, UserName, 128, NULL);
		send(cl_socket, (char*)&buf, sizeof(UINT), NULL);
		recv(cl_socket, Password, 128, NULL);
		
		printf("Recieved from Client id: %u login \"%s\" and password \"%s\"\n", ClientID, UserName, Password);
		
		char query[256];
		_snprintf_c(query, 256, "SELECT userID FROM users WHERE username = '%s' and userpass = '%s'", UserName, Password);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u login query error!\nError: %s\n", ClientID, mysql_error(&mysql));
			ClientID_DB = CLIENT_INVALID_ID;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried log in with wrong login or password\n", ClientID);
				ClientID_DB = CLIENT_INVALID_ID;
			}
			else{
				row = mysql_fetch_row(result_mysql);
				sscanf_s(row[0], "%u", &ClientID_DB);
				printf("Client id: %u logged in with id_DB: %u\n", ClientID, ClientID_DB);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}

		send(cl_socket, (char*)&ClientID_DB, sizeof(UINT), NULL);
	}
	void LogOut(){
		ClientID_DB = CLIENT_INVALID_ID;
		printf("Client id: %u logged out\n", ClientID);
	}
	void Register(){
		char UserName[128];
		char Password[128];
		UINT buf = client_registration_code;
		send(cl_socket, (char*)&buf, sizeof(UINT), NULL);
		recv(cl_socket, UserName, 128, NULL);
		send(cl_socket, (char*)&buf, sizeof(UINT), NULL);
		recv(cl_socket, Password, 128, NULL);
		
		char query[256];
		//INSERT INTO users (username, password) VALUES('string_1', 'string_2')
		_snprintf_c(query, 256, "INSERT INTO users (username, userpass) VALUES('%s', '%s')", UserName, Password);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u registration query error!\nError: %s\n", ClientID, mysql_error(&mysql));
			ClientID_DB = CLIENT_INVALID_ID;
		}
		else{
			ClientID_DB = mysql_insert_id(connection_mysql);
			printf("Client id: %u registred in DB with id_DB: %u\n", ClientID, ClientID_DB);
		}
	
		send(cl_socket, (char*)&ClientID_DB, sizeof(UINT), NULL);
		ClientID_DB = CLIENT_INVALID_ID;
	}
	void SendStudioName(){
		UINT studioID = 0;
		char StudioName[128];
		send(cl_socket, (char*)&studioID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&studioID, sizeof(UINT), NULL);
		char query[256];
		_snprintf_c(query, 256, "SELECT Name FROM Studio WHERE StudioID = %u", studioID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u studio name search error!\nError: %s\n", ClientID, mysql_error(&mysql));
			_snprintf_c(StudioName, 128, "");
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried studio name search with wrong studio_id\n", ClientID);
				_snprintf_c(StudioName, 128, "");
			}
			else{
				row = mysql_fetch_row(result_mysql);
				strcpy_s(StudioName, 128, row[0]);
				printf("Client id: %u got studio name: %s\n", ClientID, StudioName);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, StudioName, 128, NULL);
	}
	void AddNewStudio(){
		UINT studioID = 0;
		char StudioName[128];
		send(cl_socket, (char*)&studioID, sizeof(UINT), NULL);
		recv(cl_socket, StudioName, 128, NULL);
		char query[256];
		_snprintf_c(query, 256, "INSERT INTO Studio (Name) VALUES('%s')", StudioName);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u studio name insert error!\nError: %s\n", ClientID, mysql_error(&mysql));
			studioID = CLIENT_INVALID_ID;
		}
		else{
			studioID = mysql_insert_id(connection_mysql);
			printf("Client id: %u added studio: %s\n", ClientID, StudioName);
		}
		send(cl_socket, (char*)&studioID, sizeof(UINT), NULL);
	}
	void DeleteStudio(){
		UINT StudioID = 0;
		send(cl_socket, (char*)&StudioID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&StudioID, sizeof(UINT), NULL);
		char query[256];
		_snprintf_c(query, 256, "DELETE FROM Studio WHERE StudioID = %u", StudioID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u studio deletion error!\nError: %s\n", ClientID, mysql_error(&mysql));
		}
		else{
			printf("Client id: %u studio %u successfully deleted!\n", ClientID, StudioID);
		}
		_snprintf_c(query, 256, "DELETE FROM Anime_Studio WHERE Studio_StudioID = %u", StudioID);
		result = mysql_query(connection_mysql, query);
		send(cl_socket, (char*)&StudioID, sizeof(UINT), NULL);
	}
	void SendStudioIDs(){
		int StudioIDs[256];
		char query[256];
		_snprintf_c(query, 256, "SELECT StudioID FROM Studio");
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u all studio output error!\nError: %s\n", ClientID, mysql_error(&mysql));
			StudioIDs[0] = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried to get studio IDs. There are no studios!\n", ClientID);
				StudioIDs[0] = 0;
			}
			else{
				StudioIDs[0] = result;
				for (int i = 0; i < result; i++){
					row = mysql_fetch_row(result_mysql);
					sscanf_s(row[0], "%i", &StudioIDs[i + 1]);
				}
				printf("Client id: %u got studio IDs\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&StudioIDs[0], sizeof(int), NULL);
		if (StudioIDs[0] == 0) return;
		recv(cl_socket, (char*)&StudioIDs[0], sizeof(int), NULL);
		send(cl_socket, (char*)&StudioIDs[1], sizeof(int) * StudioIDs[0], NULL);
	}
	void SendAniStudioIDs(){
		int StudioIDs[256];
		UINT animeID = 0;
		char query[256];
		send(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		_snprintf_c(query, 256, "SELECT Studio_StudioID FROM Anime_Studio WHERE Anime_AnimeID = %u", animeID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anime all studis output error!\nError: %s\n", ClientID, mysql_error(&mysql));
			StudioIDs[0] = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried to get anime studio IDs. There are no studios!\n", ClientID);
				StudioIDs[0] = 0;
			}
			else{
				StudioIDs[0] = result;
				for (int i = 0; i < result; i++){
					row = mysql_fetch_row(result_mysql);
					sscanf_s(row[0], "%i", &StudioIDs[i + 1]);
				}
				printf("Client id: %u got anime studio IDs\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&StudioIDs[0], sizeof(int), NULL);
		if (StudioIDs[0] == 0) return;
		recv(cl_socket, (char*)&StudioIDs[0], sizeof(int), NULL);
		send(cl_socket, (char*)&StudioIDs[1], sizeof(int) * StudioIDs[0], NULL);
	}			
	void BindAniStudio(){
		UINT studioID = 0;
		UINT animeID = 0;
		send(cl_socket, (char*)&studioID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&studioID, sizeof(UINT), NULL);
		send(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		char query[256];
		_snprintf_c(query, 256, "INSERT INTO Anime_Studio (Anime_AnimeID, Studio_StudioID) VALUES(%u, %u)", animeID, studioID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u studio name to anime insert error!\nError: %s\n", ClientID, mysql_error(&mysql));
		}
		send(cl_socket, (char*)&studioID, sizeof(UINT), NULL);
	}
	void SendGenreName(){
		UINT genreID = 0;
		char GenreName[128];
		send(cl_socket, (char*)&genreID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&genreID, sizeof(UINT), NULL);
		char query[256];
		_snprintf_c(query, 256, "SELECT Name FROM Genre WHERE GenreID = %u", genreID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u genre name search error!\nError: %s\n", ClientID, mysql_error(&mysql));
			_snprintf_c(GenreName, 128, "");
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried genre name search with wrong genre_id\n", ClientID);
				_snprintf_c(GenreName, 128, "");
			}
			else{
				row = mysql_fetch_row(result_mysql);
				strcpy_s(GenreName, 128, row[0]);
				printf("Client id: %u got genre name: %s\n", ClientID, GenreName);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, GenreName, 128, NULL);
	}
	void SendGenresIDs(){
		int genreIDs[256];
		char query[256];
		_snprintf_c(query, 256, "SELECT GenreID FROM Genre");
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u all genres output error!\nError: %s\n", ClientID, mysql_error(&mysql));
			genreIDs[0] = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried to get genre IDs. There are no genres!\n", ClientID);
				genreIDs[0] = 0;
			}
			else{
				genreIDs[0] = result;
				for (int i = 0; i < result; i++){
					row = mysql_fetch_row(result_mysql);
					sscanf_s(row[0], "%i", &genreIDs[i + 1]);
				}
				printf("Client id: %u got genre IDs\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&genreIDs[0], sizeof(int), NULL);
		if (genreIDs[0] == 0) return;
		recv(cl_socket, (char*)&genreIDs[0], sizeof(int), NULL);
		send(cl_socket, (char*)&genreIDs[1], sizeof(int) * genreIDs[0], NULL);
	}
	void SendAnimeGenresIDs(){
		UINT animeID = 0;
		int genreIDs[32];
		char query[256];
		send(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		_snprintf_c(query, 256, "SELECT Genre_GenreID FROM Anime_Genre WHERE Anime_AnimeID = %u", animeID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anime all genres output error!\nError: %s\n", ClientID, mysql_error(&mysql));
			genreIDs[0] = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried to get anime genre IDs. There are no genres!\n", ClientID);
				genreIDs[0] = 0;
			}
			else{
				genreIDs[0] = result;
				for (int i = 0; i < result; i++){
					row = mysql_fetch_row(result_mysql);
					sscanf_s(row[0], "%i", &genreIDs[i + 1]);
				}
				printf("Client id: %u got anime genre IDs\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&genreIDs[0], sizeof(int), NULL);
		if (genreIDs[0] == 0) return;
		recv(cl_socket, (char*)&genreIDs[0], sizeof(int), NULL);
		send(cl_socket, (char*)&genreIDs[1], sizeof(int) * genreIDs[0], NULL);
	}
	void AddNewGenre(){
		UINT GenreID = 0;
		char GenreName[128];
		send(cl_socket, (char*)&GenreID, sizeof(UINT), NULL);
		recv(cl_socket, GenreName, 128, NULL);
		char query[256];
		_snprintf_c(query, 256, "INSERT INTO Genre (Name) VALUES('%s')", GenreName);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u genre name insert error!\nError: %s\n", ClientID, mysql_error(&mysql));
			GenreID = CLIENT_INVALID_ID;
		}
		else{
			GenreID = mysql_insert_id(connection_mysql);
			printf("Client id: %u added genre: %s\n", ClientID, GenreName);
		}
		send(cl_socket, (char*)&GenreID, sizeof(UINT), NULL);
	}
	void BindGenresToAnime(){
		UINT *GenreID;
		UINT AnimeID = 0;
		UINT Num;
		send(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		send(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&Num, sizeof(UINT), NULL);
		send(cl_socket, (char*)&Num, sizeof(UINT), NULL);
		GenreID = new UINT[Num];
		recv(cl_socket, (char*)GenreID, sizeof(UINT) * Num, NULL);
		
		char query[256];
		_snprintf_c(query, 256, "DELETE FROM Anime_Genre WHERE Anime_AnimeID = %u", AnimeID);
		int result = mysql_query(connection_mysql, query);
		
		for (int i = 0; i < Num; i++){
			_snprintf_c(query, 256, "INSERT INTO Anime_Genre (Anime_AnimeID, Genre_GenreID) VALUES(%u, %u)", AnimeID, GenreID[i]);
			result = mysql_query(connection_mysql, query);
		}
		send(cl_socket, (char*)GenreID, sizeof(UINT), NULL);
	}
	void BindPoster(){
		UINT AnimeID = 0;
		send(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		send(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		int filesize;
		recv(cl_socket, (char*)&filesize, sizeof(int), NULL);
		char* posterData = (char*)malloc(filesize);
		send(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)posterData, filesize, NULL);
		UINT posterID;
		char query[256];
		_snprintf_c(query, 256, "DELETE FROM Poster WHERE Anime_AnimeID = %u", AnimeID);
		int result = mysql_query(connection_mysql, query);
		_snprintf_c(query, 256, "INSERT INTO Poster (Anime_AnimeID) VALUES(%u)", AnimeID);
		result = mysql_query(connection_mysql, query);		
		posterID = mysql_insert_id(connection_mysql);
		char path[128];
		_snprintf_c(path, 128, "Posters\\%u.png", posterID);
		FILE* file;
		fopen_s(&file, path, "wb");
		fwrite(posterData, filesize, 1, file);
		fclose(file);
		free(posterData);
		send(cl_socket, (char*)&posterID, sizeof(UINT), NULL);
	}
	void SendPoster(){//подготовить к случаю если постер не получилось найти
		UINT PosterID = 0;
		send(cl_socket, (char*)&PosterID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&PosterID, sizeof(UINT), NULL);	
		char posterPath[128];
		_snprintf_c(posterPath, 128, "Posters\\%u.png", PosterID);
		FILE* file;
		int filesize = 0;
		fopen_s(&file, posterPath, "rb");
		if (file){
			fseek(file, 0, SEEK_END);
			int filesize = ftell(file);
			rewind(file);
			char* posterData = (char*)malloc(filesize);
			fread_s(posterData, filesize, filesize, 1, file);
			fclose(file);
			send(cl_socket, (char*)&filesize, sizeof(int), NULL);
			recv(cl_socket, (char*)&PosterID, sizeof(UINT), NULL);	
			send(cl_socket, (char*)posterData, filesize, NULL);
			free(posterData);
		}
		else{
			
			send(cl_socket, (char*)&filesize, sizeof(int), NULL);
			
		}
		
	}
	void SendPosterID(){
		UINT AnimeID = 0;
		UINT PosterID;
		send(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		char query[256];
		_snprintf_c(query, 256, "SELECT PosterID FROM Poster WHERE Anime_AnimeID = %u", AnimeID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u poster search error!\nError: %s\n", ClientID, mysql_error(&mysql));
			PosterID = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried to get anime poster ID. This is no poster!\n", ClientID);
				PosterID = 0;
			}
			else{
				for (int i = 0; i < result; i++){
					row = mysql_fetch_row(result_mysql);
					sscanf_s(row[i], "%u", &PosterID);
				}
				printf("Client id: %u got anime poster ID\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		
		send(cl_socket, (char*)&PosterID, sizeof(UINT), NULL);
	}
	void DeletePoster(){
		UINT PosterID = 0;
		send(cl_socket, (char*)&PosterID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&PosterID, sizeof(UINT), NULL);
		char query[256];
		_snprintf_c(query, 256, "DELETE FROM Poster WHERE PosterID = %u", PosterID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u poster deletion error!\nError: %s\n", ClientID, mysql_error(&mysql));
		}
		else{
			printf("Client id: %u poster %u successfully deleted!\n", ClientID, PosterID);
		}
		send(cl_socket, (char*)&PosterID, sizeof(UINT), NULL);
	}
	void SendScreenshotsIDs(){
		UINT animeID = 0;
		int ScreenshotsIDs[64];
		char query[256];
		send(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		_snprintf_c(query, 256, "SELECT SkreenshotID FROM Skreenshot WHERE Anime_AnimeID = %u", animeID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anime all screenshots output error!\nError: %s\n", ClientID, mysql_error(&mysql));
			ScreenshotsIDs[0] = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried to get anime screenshots IDs. There are no screenshots!\n", ClientID);
				ScreenshotsIDs[0] = 0;
			}
			else{
				ScreenshotsIDs[0] = result;
				for (int i = 0; i < result; i++){
					row = mysql_fetch_row(result_mysql);
					sscanf_s(row[0], "%i", &ScreenshotsIDs[i + 1]);
				}
				printf("Client id: %u got anime screenshots IDs\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&ScreenshotsIDs[0], sizeof(int), NULL);
		if (ScreenshotsIDs[0] == 0) return;
		recv(cl_socket, (char*)&ScreenshotsIDs[0], sizeof(int), NULL);
		send(cl_socket, (char*)&ScreenshotsIDs[1], sizeof(int) * ScreenshotsIDs[0], NULL);
	}
	void UploadScreenshot(){
		UINT AnimeID = 0;
		send(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		send(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		int filesize;
		recv(cl_socket, (char*)&filesize, sizeof(int), NULL);
		char* posterData = (char*)malloc(filesize);
		send(cl_socket, (char*)&AnimeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)posterData, filesize, NULL);
		UINT posterID;
		char query[256];
		_snprintf_c(query, 256, "INSERT INTO Skreenshot (Anime_AnimeID) VALUES(%u)", AnimeID);
		int result = mysql_query(connection_mysql, query);		
		posterID = mysql_insert_id(connection_mysql);
		char path[128];
		_snprintf_c(path, 128, "Screenshots\\%u.png", posterID);
		FILE* file;
		fopen_s(&file, path, "wb");
		fwrite(posterData, filesize, 1, file);
		fclose(file);
		free(posterData);
		send(cl_socket, (char*)&posterID, sizeof(UINT), NULL);
	}
	void SendScreenshot(){
		UINT ScreenshotID = 0;
		send(cl_socket, (char*)&ScreenshotID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&ScreenshotID, sizeof(UINT), NULL);	
		char posterPath[128];
		_snprintf_c(posterPath, 128, "Screenshots\\%u.png", ScreenshotID);
		FILE* file;
		fopen_s(&file, posterPath, "rb"); 
		fseek(file, 0, SEEK_END);
		int filesize = ftell(file);
		rewind(file);
		char* ScreenshotData = (char*)malloc(filesize);
		fread_s(ScreenshotData, filesize, filesize, 1, file);
		fclose(file);
		send(cl_socket, (char*)&filesize, sizeof(int), NULL);
		recv(cl_socket, (char*)&ScreenshotID, sizeof(UINT), NULL);	
		send(cl_socket, (char*)ScreenshotData, filesize, NULL);
		free(ScreenshotData);
	}
	void DeleteScreenshot(){
		UINT ScreenshotID = 0;
		send(cl_socket, (char*)&ScreenshotID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&ScreenshotID, sizeof(UINT), NULL);
		char query[256];
		_snprintf_c(query, 256, "DELETE FROM Skreenshot WHERE SkreenshotID = %u", ScreenshotID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u screenshot deletion error!\nError: %s\n", ClientID, mysql_error(&mysql));
		}
		else{
			printf("Client id: %u screenshot %u successfully deleted!\n", ClientID, ScreenshotID);
		}
		send(cl_socket, (char*)&ScreenshotID, sizeof(UINT), NULL);
	}
	void SendAnimeScore(){
		UINT animeID = 0;
		send(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		float score = 0;
		int score_i = 0;
		char query[256];
		_snprintf_c(query, 256, "SELECT SUM(Score) FROM Rating WHERE Anime_AnimeID = %u", animeID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u score error!\nError: %s\n", ClientID, mysql_error(&mysql));
			score = 0.0f;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u no one voted!\n", ClientID);
				score = 0.0f;
			}
			else{
				row = mysql_fetch_row(result_mysql);
				sscanf_s(row[0], "%i", &score_i);
				score = score_i * 1.0f / result;
				printf("Client id: %u got score name: %f\n", ClientID, score);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&score, sizeof(float), NULL);
	}
	void vote(){
		UINT animeID = 0;
		send(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		send(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		float score;
		UINT score_u;
		send(cl_socket, (char*)&score_u, sizeof(UINT), NULL);
		if (score_u > 5) score_u = 5;
		char query[256];
		int result;
		if (ClientID_DB != CLIENT_INVALID_ID){
			_snprintf_c(query, 256, "SELECT * FROM Rating WHERE Anime_AnimeID = %u and user_userID = %u", animeID, ClientID_DB);
			result = mysql_query(connection_mysql, query);
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
			if (result == 0) _snprintf_c(query, 256, "INSERT INTO Rating (users_userID, Anime_AnimeID, Score) VALUES(%u, %u, %u)", ClientID_DB, animeID, score_u);
			else _snprintf_c(query, 256, "UPDATE Rating SET Score = %u WHERE users_userID = %u and Anime_AnimeID = %u", score_u, ClientID_DB, animeID);
			result = mysql_query(connection_mysql, query);
		}
		_snprintf_c(query, 256, "SELECT SUM(Score) FROM Rating WHERE Anime_AnimeID = %u", animeID);
		result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u score error!\nError: %s\n", ClientID, mysql_error(&mysql));
			score = 0.0f;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u no one voted!\n", ClientID);
				score = 0.0f;
			}
			else{
				row = mysql_fetch_row(result_mysql);
				sscanf_s(row[0], "%u", score_u);
				score = score_u * 1.0f / result;
				printf("Client id: %u got score name: %f\n", ClientID, score);
				mysql_free_result(result_mysql); 
				result_mysql = NULL;
			}
		}
		send(cl_socket, (char*)&score, sizeof(float), NULL);
	}
	void SendAniPage(){
		UINT animeID = 0;
		anime_page anipage;
		send(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		char query[256];
		_snprintf_c(query, 256, "SELECT Name FROM Anime WHERE AnimeID = %u", animeID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anipage sending error!\nError: %s\n", ClientID, mysql_error(&mysql));
			anipage.id = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u anipage not found!\n", ClientID);
				anipage.id = 0;
			}
			else{
				anipage.id = animeID;
				row = mysql_fetch_row(result_mysql);
				strcpy_s(anipage.name, 128, row[0]);
				mysql_free_result(result_mysql); 
				result_mysql = NULL;
				
				_snprintf_c(query, 256, "SELECT ReleaseYear FROM Anime WHERE AnimeID = %u", animeID);
				result = mysql_query(connection_mysql, query);
				result_mysql = mysql_store_result(connection_mysql);
				row = mysql_fetch_row(result_mysql);
				sscanf_s(row[0], "%i", &(anipage.release_year));
				mysql_free_result(result_mysql); 
				result_mysql = NULL;
				
				_snprintf_c(query, 256, "SELECT Description FROM Anime WHERE AnimeID = %u", animeID);
				result = mysql_query(connection_mysql, query);
				result_mysql = mysql_store_result(connection_mysql);
				row = mysql_fetch_row(result_mysql);
				strcpy_s(anipage.description, 4096, row[0]);
				mysql_free_result(result_mysql); 
				result_mysql = NULL;

				_snprintf_c(query, 256, "SELECT Episodes FROM Anime WHERE AnimeID = %u", animeID);
				result = mysql_query(connection_mysql, query);
				result_mysql = mysql_store_result(connection_mysql);
				row = mysql_fetch_row(result_mysql);
				sscanf_s(row[0], "%i", &(anipage.NumOfEpisodes));
				
				_snprintf_c(query, 256, "SELECT LastUpdTime FROM Anime WHERE AnimeID = %u", animeID);
				result = mysql_query(connection_mysql, query);
				result_mysql = mysql_store_result(connection_mysql);
				row = mysql_fetch_row(result_mysql);
				strcpy_s(anipage.LastUpdTime, 32, row[0]);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&anipage, sizeof(anime_page), NULL);
	}
	void BindAniPage(){
		anime_page anipage;
		UINT buf = 0;
		send(cl_socket, (char*)&buf, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&anipage, sizeof(anime_page), NULL);
		char query[8000];
		_snprintf_c(query, 8000, "INSERT INTO Anime (Name, ReleaseYear, Description, Episodes, LastUpdTime) VALUES('%s', %i, '%s', %i, NOW())", anipage.name, anipage.release_year, anipage.description, anipage.NumOfEpisodes);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anipage binding error!\nError: %s\n", ClientID, mysql_error(&mysql));
			anipage.id = 0;
		}
		else{
			anipage.id = mysql_insert_id(connection_mysql);
			printf("Client id: %u anipage %i successfully binded!\n", ClientID, anipage.id);
		}
		send(cl_socket, (char*)&(anipage.id), sizeof(UINT), NULL);
	}
	void UpdateAniPage(){
		anime_page anipage;
		UINT buf = 0;
		send(cl_socket, (char*)&buf, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&anipage, sizeof(anime_page), NULL);
		char query[8000];
		_snprintf_c(query, 8000, "UPDATE Anime SET Name='%s', ReleaseYear=%i, Description='%s', Episodes=%i, LastUpdTime=NOW() WHERE AnimeID = %i", anipage.name, anipage.release_year, anipage.description, anipage.NumOfEpisodes, anipage.id);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anipage update error!\nError: %s\n", ClientID, mysql_error(&mysql));
		}
		else{
			printf("Client id: %u anipage %i successfully updated!\n", ClientID, anipage.id);
		}
		send(cl_socket, (char*)&(anipage.id), sizeof(UINT), NULL);
	}
	void DeleteAnimePage(){
		UINT AniPageID = 0;
		send(cl_socket, (char*)&AniPageID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&AniPageID, sizeof(UINT), NULL);
		char query[256];
		_snprintf_c(query, 256, "DELETE FROM Anime WHERE AnimeID = %u", AniPageID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anipage deletion error!\nError: %s\n", ClientID, mysql_error(&mysql));
		}
		else{
			printf("Client id: %u anipage %u successfully deleted!\n", ClientID, AniPageID);
		}
		send(cl_socket, (char*)&AniPageID, sizeof(UINT), NULL);
	}
	
	void DeleteEpisode(){
		UINT EpisodeID = 0;
		send(cl_socket, (char*)&EpisodeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&EpisodeID, sizeof(anime_page), NULL);
		char query[256];
		_snprintf_c(query, 256, "DELETE FROM Episode WHERE EpisodeID = %u", EpisodeID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u episode deletion error!\nError: %s\n", ClientID, mysql_error(&mysql));
		}
		else{
			printf("Client id: %u episode %u successfully deleted!\n", ClientID, EpisodeID);
		}
		send(cl_socket, (char*)&EpisodeID, sizeof(UINT), NULL);
	}
	void SendEpisodesIDs(){
		UINT animeID = 0;
		int EpisodeIDs[1024];
		char query[256];
		send(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&animeID, sizeof(UINT), NULL);
		_snprintf_c(query, 256, "SELECT EpisodeID FROM Episode WHERE Anime_AnimeID = %u ORDER BY Number ASC", animeID);
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anime all episodes output error!\nError: %s\n", ClientID, mysql_error(&mysql));
			EpisodeIDs[0] = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried to get anime episodes IDs. There are no episodes!\n", ClientID);
				EpisodeIDs[0] = 0;
			}
			else{
				EpisodeIDs[0] = result;
				for (int i = 0; i < result; i++){
					row = mysql_fetch_row(result_mysql);
					sscanf_s(row[0], "%i", &EpisodeIDs[i + 1]);
				}
				printf("Client id: %u got anime episodes IDs\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&EpisodeIDs[0], sizeof(int), NULL);
		if (EpisodeIDs[0] == 0) return;
		recv(cl_socket, (char*)&EpisodeIDs[0], sizeof(int), NULL);
		send(cl_socket, (char*)&EpisodeIDs[1], sizeof(int) * EpisodeIDs[0], NULL);
	}
	void uploadEpisode(){
		EpisodeThread = boost::thread(&EpisodeRecvThreadFunction, this);
	}
	void downloadEpisode(){
		EpisodeThread = boost::thread(&EpisodeSendThreadFunction, this);
	}

	void SendMaxAnimeYear(){
		UINT MaxAniYear = 0;
		char query[256];
		_snprintf_c(query, 256, "SELECT MAX(ReleaseYear) FROM Anime");
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u max_ani_year error!\nError: %s\n", ClientID, mysql_error(&mysql));
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u There are no anime!\n", ClientID);
			}
			else{
				row = mysql_fetch_row(result_mysql);
				sscanf_s(row[0], "%u", &MaxAniYear);
				printf("Client id: %u got anime max year\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&MaxAniYear, sizeof(UINT), NULL);
	}
	void SendMinAnimeYear(){
		UINT MinAniYear = 0;
		char query[256];
		_snprintf_c(query, 256, "SELECT MIN(ReleaseYear) FROM Anime");
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u min_ani_year error!\nError: %s\n", ClientID, mysql_error(&mysql));
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u There are no anime!\n", ClientID);
			}
			else{
				row = mysql_fetch_row(result_mysql);
				sscanf_s(row[0], "%u", &MinAniYear);
				printf("Client id: %u got anime min year\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&MinAniYear, sizeof(UINT), NULL);
	}
	void stock_catalog(){
		int anipageIDs[2048];
		char query[256];
		_snprintf_c(query, 256, "SELECT AnimeID FROM Anime ORDER BY LastUpdTime DESC");
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anime IDs stock catalog output error!\nError: %s\n", ClientID, mysql_error(&mysql));
			anipageIDs[0] = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried to get anime IDs stock catalog. There are no anime!\n", ClientID);
				anipageIDs[0] = 0;
			}
			else{
				anipageIDs[0] = result;
				for (int i = 0; i < result; i++){
					row = mysql_fetch_row(result_mysql);
					sscanf_s(row[0], "%i", &anipageIDs[i + 1]);
				}
				printf("Client id: %u got anime IDs stock catalog\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&anipageIDs[0], sizeof(int), NULL);
		if (anipageIDs[0] == 0) return;
		recv(cl_socket, (char*)&anipageIDs[0], sizeof(int), NULL);
		send(cl_socket, (char*)&anipageIDs[1], sizeof(int) * anipageIDs[0], NULL);
	}
	void catalog_after_filter(){
		int anipageIDs[2048];
		UINT buf;
		filter Filter;
		send(cl_socket, (char*)&buf, sizeof(UINT), NULL);
		recv(cl_socket, (char*)&Filter, sizeof(filter), NULL);
		char query[2560];
		_snprintf_c(query, 2560, "SELECT DISTINCT anime.AnimeID FROM ");	
		char buffff[1000];
		if (Filter.Genre){
			_snprintf_c(buffff, 1000, "(anime LEFT JOIN anime_genre ON anime.AnimeID = anime_genre.Anime_AnimeID) ");
		}
		else{
			_snprintf_c(buffff, 1000, "anime ");
		}
		strcat(query, buffff);
		if (Filter.Studio){
			_snprintf_c(buffff, 1000, "LEFT JOIN anime_studio ON anime.AnimeID = anime_studio.Anime_AnimeID ");
			strcat(query, buffff);
		}
		_snprintf_c(buffff, 1000, "WHERE anime.ReleaseYear <= %i and anime.ReleaseYear >= %i and anime.Episodes <= %i and anime.Episodes >= %i ",
				Filter.MaxYear, Filter.MinYear, Filter.MaxEp, Filter.MinEp);
		strcat(query, buffff);
		if (Filter.Genre){
			_snprintf_c(buffff, 1000, "AND Genre_GenreID = %i ", Filter.Genre);
			strcat(query, buffff);
		}
		if (Filter.Studio){
			_snprintf_c(buffff, 1000, "AND Studio_StudioID = %i ", Filter.Studio);
			strcat(query, buffff);
		}
		if (Filter.SortType == 0){
			_snprintf_c(buffff, 1000, "ORDER BY anime.Name", Filter.Studio);
			strcat(query, buffff);
		}
		else {
			_snprintf_c(buffff, 1000, "ORDER BY anime.LastUpdTime", Filter.Studio);
			strcat(query, buffff);
		}
		int result = mysql_query(connection_mysql, query);
		if (result != 0){
			printf("Client id: %u anime IDs filtered catalog output error!\nError: %s\n", ClientID, mysql_error(&mysql));
			anipageIDs[0] = 0;
		}
		else{
			result_mysql = mysql_store_result(connection_mysql);
			result = mysql_num_rows(result_mysql);
			if (result == 0) {
				printf("Client id: %u tried to get anime IDs filtered catalog. There are no anime!\n", ClientID);
				anipageIDs[0] = 0;
			}
			else{
				anipageIDs[0] = result;
				for (int i = 0; i < result; i++){
					row = mysql_fetch_row(result_mysql);
					sscanf_s(row[0], "%i", &anipageIDs[i + 1]);
				}
				printf("Client id: %u got anime IDs filtered catalog\n", ClientID);
			}
			mysql_free_result(result_mysql); 
			result_mysql = NULL;
		}
		send(cl_socket, (char*)&anipageIDs[0], sizeof(int), NULL);
		if (anipageIDs[0] == 0) return;
		recv(cl_socket, (char*)&anipageIDs[0], sizeof(int), NULL);
		send(cl_socket, (char*)&anipageIDs[1], sizeof(int) * anipageIDs[0], NULL);
	}
	
public:
	CLIENT(UINT ClientNewID, SOCKET NewSocket, SERVER* pSrv){
		cl_socket = NewSocket;
		pServer = pSrv;
		ClientID = ClientNewID;
		ClientID_DB = CLIENT_INVALID_ID;
		ConnectionFailed = true;
		//если сможет подключиться к базе данных то пошлёт свой айди если нет от инвалидный и закроется
		if (!(connect_to_database())){
			cl_socket = CLIENT_INVALID_ID;
			send(cl_socket, (char*)&(ClientID), sizeof(UINT), NULL);
			AddClientToDisConQueue();
			return;
		}
		send(cl_socket, (char*)&ClientID, sizeof(UINT), NULL);
		printf("Client id: %u completely connected to server.\n", ClientID);
		ConnectionFailed = false;
		ClientThread = boost::thread(&CLIENT::ClientThreadFunction, this);
	}
	~CLIENT(){
		ClientThread.interrupt();
		shutdown(cl_socket, SD_BOTH);
		closesocket(cl_socket);
		shutdown_database_connection();
		printf("Client id: %u disconnected\n", ClientID);
	}
	bool IsConnectionFailed(){
		return ConnectionFailed;
	}
	void Reconnect(SOCKET NewSocket){
		//после удачного реконнекченья должен послать новый айди
		//ClientThread.interrupt();
		cl_socket = NewSocket;
		send(cl_socket, (char*)&ClientID, sizeof(UINT), NULL);
		printf("Client id: %u successfully reconnected to server.\n", ClientID);
		ConnectionFailed = false;
		//ClientThread = boost::thread(&CLIENT::ClientThreadFunction, this);
	}
};

#define MAX_CLIENT_NUMBER 2048

class STACK_UINT {
private:
	boost::mutex Mutex;
	UINT NumOfElements;
	UINT MaxNumOfElements;
	UINT* ElementsMassive;
public:
	STACK_UINT(){
		NumOfElements = 0;
		MaxNumOfElements = MAX_CLIENT_NUMBER;
		ElementsMassive = new UINT[MAX_CLIENT_NUMBER];	
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

#define CONNECTION_CODE 1
#define RECONNECTION_CODE 0

class SERVER{
private:
	SOCKET listen_socket;
	bool is_running;
	
	STACK_UINT DisconnectionsStack;
	STACK_UINT ClientsFreePointersStack;
	CLIENT* Clients[MAX_CLIENT_NUMBER];
	
	boost::thread ServerListenThread;
	boost::mutex ServerListenThreadMutex;
	
	boost::thread ServerDisconnectionThread;
	
	bool IsPausedServerListenThread;
	void PauseServerListenThread_OnOff(){
		if (IsPausedServerListenThread) ServerListenThreadMutex.unlock();
		else ServerListenThreadMutex.lock();
	}
	
	static void ServerListenThreadFunction(SERVER* pServer){
		while(!(pServer->ServerListenThread.interruption_requested())){
			pServer->ServerListenThreadMutex.lock();
			//подключение клиентов
			SOCKET new_socket = INVALID_SOCKET;
			new_socket = accept(pServer->listen_socket, NULL, NULL);
			if (new_socket != INVALID_SOCKET) {
				pServer->ConnectClient(new_socket, pServer);
			}
			pServer->ServerListenThreadMutex.unlock();
			Sleep(1);	
		}
	}
	static void ServerDisconnectionThreadFunction(SERVER* pServer){
		while(!(pServer->ServerDisconnectionThread.interruption_requested())){
			if(pServer->DisconnectionsStack.GetCurNumOfElements() > 0){
				UINT ClientID = pServer->DisconnectionsStack.ExtractElement();
				pServer->DisconnectClient(ClientID);
			}
			Sleep(1);
		}
	}
	
	void DisconnectClient(UINT Client_ID){
		if (Clients[Client_ID] == NULL) return;
		Clients[Client_ID]->~CLIENT();
		Clients[Client_ID] = NULL;
		ClientsFreePointersStack.AddElement(Client_ID);
	}
	void disconnect_all_clients(){
		if (!IsPausedServerListenThread) PauseServerListenThread_OnOff();
		for (int i = 0; i < MAX_CLIENT_NUMBER; i++){
			if (Clients[i]) {
				Clients[i]->~CLIENT();
				Clients[i] = NULL;
			}
		}
		DisconnectionsStack.clear();
		ClientsFreePointersStack.clear();
		for (int i = 0; i < MAX_CLIENT_NUMBER; i++){
			Clients[i] = NULL;
			ClientsFreePointersStack.AddElement(MAX_CLIENT_NUMBER - 1 - i);
		}
		PauseServerListenThread_OnOff();
	}
	void ConnectClient(SOCKET NewSocket, SERVER* pServer){
		//первым сообщение посылает клиент, он сообщает, о том 
		//подключается он в первый раз или реконнектится, 
		//	если реконнектится то сообщение должно было быть ещё со своим старым айди
		//	и сервак его проверяет, правда ли с таким клиентом был разрыв связи
		//	если всё правильно, то пересоединяет, если нет то шлёт лесом
		//если соедиенение или пересоединение прошло успешно то сервак создаёт объект для клиента,
		//	и тот если нормально запустится то отправит свой айди
		//в противном случае сервер посылает инвалидный айдишник
		UINT first_message[2];
		UINT client_id;
		recv(NewSocket, (char*)first_message, sizeof(UINT) * 2, NULL);
		if (first_message[0] == RECONNECTION_CODE){
			client_id = first_message[1];
			if (Clients[client_id]->IsConnectionFailed()){
				Clients[client_id]->Reconnect(NewSocket);
				//send(NewSocket, (char*)&client_id, sizeof(UINT), NULL);
			}
			else{
				client_id = CLIENT_INVALID_ID;
				send(NewSocket, (char*)&client_id, sizeof(UINT), NULL);
				closesocket(NewSocket); 
			}
		}
		else{
			if (ClientsFreePointersStack.GetCurNumOfElements() > 0){
				client_id = ClientsFreePointersStack.ExtractElement();
				Clients[client_id] = new CLIENT(client_id, NewSocket, pServer);
			}
			else{
				client_id = CLIENT_INVALID_ID;
				send(NewSocket, (char*)&client_id, sizeof(UINT), NULL);
				closesocket(NewSocket); 
			}
		}
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
		srv_sin.sin_port = htons(DEFAULT_PORT);
		
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
public:	
	SERVER(){
		_mkdir("Posters");
		_mkdir("Screenshots");
		_mkdir("Episodes");
		if (start_listen_socket()){
			IsPausedServerListenThread = false;
			for (int i = 0; i < MAX_CLIENT_NUMBER; i++){
				Clients[i] = NULL;
				ClientsFreePointersStack.AddElement(MAX_CLIENT_NUMBER - 1 - i);
			}
			ServerListenThread = boost::thread(&ServerListenThreadFunction, this);
			ServerDisconnectionThread = boost::thread(&ServerDisconnectionThreadFunction, this);
			is_running = true;
			printf("Server is running\n");
		}
		else{
			is_running = false;
			printf("Server running error!\n");
		}
	}
	~SERVER(){
		if(is_running){
			ServerListenThread.interrupt();
			ServerDisconnectionThread.interrupt();
			closesocket(listen_socket);
			WSACleanup();
			disconnect_all_clients();
			DisconnectionsStack.~STACK_UINT();
			ClientsFreePointersStack.~STACK_UINT();
			printf("Server is stopped\n");
			is_running = false;
		}
	}
	bool IsRunning(){
		return is_running;
	}
	bool AddClientToDisconnectionsQueue(UINT ClientID){
		return DisconnectionsStack.AddElement(ClientID);
	}
};

void CLIENT::AddClientToDisConQueue(){
	pServer->AddClientToDisconnectionsQueue(ClientID);
}

// нужно чтобы сервером можно было управлять через консоль
int main(){
	setlocale(LC_ALL, "russian");
	SetConsoleTitle(SERVER_CONSOLE_TITLE);
	SERVER server;
	//ввод с консоли комманд
	char query[512];
	bool to_exit = false;
//	printf("anipage size %u\n", (UINT)sizeof(anime_page));
	//test_class t_c;
	do {
		//получение строки-команды с консоли

		//лингвистический анализ строки
			//проверка, не exit ли это, если да, то to_exit = false
		
		//выбор метода и параметров в зависимости от команды
			
		//вывод результата
		
		Sleep(1);
	} while(!to_exit);	
	
	server.~SERVER();
	return 0;
}