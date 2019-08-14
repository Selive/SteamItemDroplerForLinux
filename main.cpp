#include "stdafx.h"

CSteamAPILoader g_steamAPILoader;

int main(int argc, char* argv[])
{
	printf("SteamItemDroplerForLinux by SeLive\n");

	char steamAccountName[33];
	char steamAccountPassword[33];
	char steamGuardCode[33];
	AppId_t appID;
	SteamItemDef_t dropListDefinition;

	if (argc == 6) {
		strcpy(steamAccountName, argv[1]);
		strcpy(steamAccountPassword, argv[2]);

		sscanf(argv[3], "%d", &appID);
		sscanf(argv[4], "%d", &dropListDefinition);

		strcpy(steamGuardCode, argv[5]);

		memset(argv[1], 0, strlen(argv[1]));
		memset(argv[2], 0, strlen(argv[2]));
		memset(argv[5], 0, strlen(argv[5]));

		printf("Enter your Steam account name: %s\n", steamAccountName);
		printf("Enter your Steam account password: \n");
		printf("Enter the AppID: %d\n", appID);
		printf("Enter the drop list definition: %d\n", dropListDefinition);
		printf("Enter the 2fa code: %s\n", steamGuardCode);
	}
	else {
		printf("Not found all arguments!!!");
		return 0;
	}

	CreateInterfaceFn steam3Factory = g_steamAPILoader.GetSteam3Factory();
	if (!steam3Factory) {
		printf("GetSteam3Factory failed\n");
		return 0;
	}

	IClientEngine* clientEngine = (IClientEngine*)steam3Factory(CLIENTENGINE_INTERFACE_VERSION, NULL);
	if (!clientEngine) {
		printf("clientEngine is null\n");
		return 0;
	}

	ISteamClient017* steamClient = (ISteamClient017*)steam3Factory(STEAMCLIENT_INTERFACE_VERSION_017, NULL);
	if (!steamClient) {
		printf("steamClient is null\n");
		return 0;
	}

	HSteamPipe hSteamPipe;
	HSteamUser hSteamUser = clientEngine->CreateLocalUser(&hSteamPipe, k_EAccountTypeIndividual);
	if (!hSteamPipe || !hSteamUser) {
		printf("CreateLocalUser failed (1)\n");
		return 0;
	}

	IClientBilling* clientBilling = clientEngine->GetIClientBilling(hSteamUser, hSteamPipe, CLIENTBILLING_INTERFACE_VERSION);
	if (!clientBilling) {
		printf("clientBilling is null\n");
		return 0;
	}

	IClientFriends* clientFriends = clientEngine->GetIClientFriends(hSteamUser, hSteamPipe, CLIENTFRIENDS_INTERFACE_VERSION);
	if (!clientFriends) {
		printf("clientFriends is null\n");
		return 0;
	}

	IClientUser* clientUser = clientEngine->GetIClientUser(hSteamUser, hSteamPipe, CLIENTUSER_INTERFACE_VERSION);
	if (!clientUser) {
		printf("clientUser is null\n");
		return 0;
	}

	IClientUtils* clientUtils = clientEngine->GetIClientUtils(hSteamPipe, CLIENTUTILS_INTERFACE_VERSION);
	if (!clientUtils) {
		printf("clientUtils is null\n");
		return 0;
	}

	ISteamGameCoordinator001* steamGameCoordinator = (ISteamGameCoordinator001*)steamClient->GetISteamGenericInterface(hSteamUser, hSteamPipe, STEAMGAMECOORDINATOR_INTERFACE_VERSION_001);
	if (!steamGameCoordinator) {
		printf("steamGameCoordinator is null\n");
		return 0;
	}

	ISteamInventory001* steamInventory = (ISteamInventory001*)steamClient->GetISteamInventory(hSteamUser, hSteamPipe, "STEAMINVENTORY_INTERFACE_V001");
	if (!steamInventory) {
		printf("steamInventory is null\n");
		return 0;
	}

	ISteamUser017* steamUser = (ISteamUser017*)steamClient->GetISteamUser(hSteamUser, hSteamPipe, STEAMUSER_INTERFACE_VERSION_017);
	if (!steamUser) {
		printf("steamUser is null\n");
		return 0;
	}

	printf("Start Steam\n");
	clientUser->SetTwoFactorCode(steamGuardCode);
	clientUser->LogOnWithPassword(false, steamAccountName, steamAccountPassword);

	bool bPlayingGame = false;
	bool bPlayingOnServer = false; 
	while (true)
	{
		CallbackMsg_t callbackMsg;
		while (Steam_BGetCallback(hSteamPipe, &callbackMsg))
		{
			printf("User callback: %d\n", callbackMsg.m_iCallback );
			switch (callbackMsg.m_iCallback)
			{
				case SteamServersConnected_t::k_iCallback: {
					printf( "SteamServersConnected_t\n");
					clientFriends->SetPersonaState(k_EPersonaStateOnline);
					
					clientUtils->SetAppIDForCurrentPipe(appID, true);
					bPlayingGame = true;
					
					/*if (clientUser->BIsSubscribedApp(appID)) {
						clientUtils->SetAppIDForCurrentPipe(appID, true);
						bPlayingGame = true;
					}
					else {
						printf("You are not subscribed to this app. Trying to add a free license...\n");

						SteamAPICall_t hRequestFreeLicenseForApps = clientBilling->RequestFreeLicenseForApp(appID);
						bool bFailed;
						while (!clientUtils->IsAPICallCompleted(hRequestFreeLicenseForApps, &bFailed))
							sleep(1000);

						RequestFreeLicenseResponse_t requestFreeLicenseResponse;
						if (!clientUtils->GetAPICallResult(hRequestFreeLicenseForApps, &requestFreeLicenseResponse, sizeof(RequestFreeLicenseResponse_t), RequestFreeLicenseResponse_t::k_iCallback, &bFailed)) {
							printf("GetAPICallResult failed\n");
							goto funcEnd;
						}
						if (requestFreeLicenseResponse.m_EResult == k_EResultOK /* && requestFreeLicenseResponse.m_nGrantedAppIds == 1) {
							printf("Added a free license\n");
							clientUtils->SetAppIDForCurrentPipe(appID, true);
							bPlayingGame = true;
						}
						else {
							printf("Failed to add a free license. You do not own this game\n");
							goto funcEnd;
						}
					}*/

					printf("Item drop idling is now in progress\n");
					break;
				}
				case SteamServerConnectFailure_t::k_iCallback:
				{
					SteamServerConnectFailure_t* steamServerConnectFailure = (SteamServerConnectFailure_t*)callbackMsg.m_pubParam;
					switch (steamServerConnectFailure->m_eResult)
					{
					case k_EResultInvalidLoginAuthCode:
						printf("Invalid Steam Guard code\n");
					case k_EResultAccountLogonDenied:
					{

						printf("Enter the Steam Guard code: ");
						scanf("%32s", steamGuardCode);
						getchar();
						clientUser->Set2ndFactorAuthCode(steamGuardCode, false);
						clientUser->LogOnWithPassword(false, steamAccountName, steamAccountPassword);
						break;
					}
					case k_EResultTwoFactorCodeMismatch:
						printf("Invalid Steam Mobile Authenticator code\n");
					case k_EResultAccountLogonDeniedNeedTwoFactorCode:
					{
						printf( "Enter the Steam Mobile Authenticator code: %s\n", steamGuardCode);
						clientUser->SetTwoFactorCode(steamGuardCode);
						clientUser->LogOnWithPassword(false, steamAccountName, steamAccountPassword);
						break;
					}
					default:
						printf("Login failed (%d)\n", steamServerConnectFailure->m_eResult);
						break;
					}

					bPlayingGame = false;
					bPlayingOnServer = false;
					break;
				}
				case SteamServersDisconnected_t::k_iCallback:
				{
					SteamServersDisconnected_t* steamServersDisconnected = (SteamServersDisconnected_t*)callbackMsg.m_pubParam;
					printf("Disconnected from steam servers (%d)\n", steamServersDisconnected->m_eResult);

					bPlayingGame = false;
					bPlayingOnServer = false;
					break;
				}
			}

			Steam_FreeLastCallback(hSteamPipe);
			sleep(5);
			if(callbackMsg.m_iCallback == 819){
				break;
			}
		}

		if (bPlayingGame) {
			if (appID == 440) {
				static bool bHelloMsgSent = false;
				static bool bGameServerInited = false;

				// do game coordinator stuff
				if (!bHelloMsgSent) {
					// k_EMsgGCClientHello
					unsigned char response[] = { 0xA6, 0x0F, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x08, 0x98, 0xE1, 0xC0, 0x01 };
					steamGameCoordinator->SendMessage(0x80000FA6, response, sizeof(response));
					printf("Sent hello msg to game coordinator\n");

					bHelloMsgSent = true;
				}

				uint32 msgSize;
				while (steamGameCoordinator->IsMessageAvailable(&msgSize)) {
					uint32 msgType;
					unsigned char* msg = new unsigned char[msgSize];
					if (steamGameCoordinator->RetrieveMessage(&msgType, msg, msgSize, &msgSize) == k_EGCResultOK) {
						printf("Retrieved message of type 0x%X from game coordinator\n", msgType);
						if (msgType == 0x80000FA4) { // k_EMsgGCClientWelcome
							printf("Got welcome msg from game coordinator\n");
						}
						else if (msgType == 0x8000001B) { // k_ESOMsg_CacheSubscriptionCheck
							// k_ESOMsg_CacheSubscriptionRefresh
							unsigned char response[] = { 0x1C, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
							*(CSteamID*)& response[9] = steamUser->GetSteamID();
							steamGameCoordinator->SendMessage(0x8000001C, response, sizeof(response));
							printf("Sent response to game coordinator\n");
						}
					}
					else {
						printf("Failed to retrieve message from game coordinator\n");
					}
					delete[] msg;
				}

				// do game server stuff
				static HSteamPipe hSteamGameServerPipe;
				static HSteamUser hSteamGameServerUser;
				static ISteamGameServer012* steamGameServer;
				if (!bGameServerInited) {
					// called by SteamGameServer_Init. needed for games that require us to be connected to a server
					steamClient->SetLocalIPBinding(0, 26901);
					hSteamGameServerUser = steamClient->CreateLocalUser(&hSteamGameServerPipe, k_EAccountTypeGameServer);
					if (!hSteamGameServerPipe || !hSteamGameServerUser) {
						printf("CreateLocalUser failed (2)\n");
						return 0;
					}

					steamGameServer = (ISteamGameServer012*)steamClient->GetISteamGameServer(hSteamGameServerUser, hSteamGameServerPipe, STEAMGAMESERVER_INTERFACE_VERSION_012);
					if (!steamGameServer) {
						printf("steamGameServer is null\n");
						return 0;
					}

					steamGameServer->InitGameServer(0, 27015, MASTERSERVERUPDATERPORT_USEGAMESOCKETSHARE, k_unServerFlagSecure, 440, "3158168");
					steamGameServer->SetProduct("tf");
					steamGameServer->SetGameDescription("Team Fortress");
					steamGameServer->SetModDir("tf");
					steamGameServer->SetDedicatedServer(false);
					steamGameServer->LogOnAnonymous();
					steamGameServer->SetMaxPlayerCount(1);
					steamGameServer->SetBotPlayerCount(0);
					steamGameServer->SetPasswordProtected(true);
					steamGameServer->SetRegion("-1");
					steamGameServer->SetServerName("Team Fortress 2");
					steamGameServer->SetMapName("ctf_2fort");
					steamGameServer->SetGameData("tf_mm_trusted:0,tf_mm_servermode:0,lobby:0,steamblocking:0");
					steamGameServer->SetKeyValue("tf_gamemode_ctf", "1");
					steamGameServer->SetKeyValue("sv_tags", "ctf");
					steamGameServer->SetGameTags("ctf");
					//steamGameServer->EnableHeartbeats( true );

					bGameServerInited = true;
				}

				if (!bPlayingOnServer) {
					static HAuthTicket hAuthTicket = 0;
					if (hAuthTicket) {
						steamUser->CancelAuthTicket(hAuthTicket);
						steamGameServer->EndAuthSession(steamUser->GetSteamID());
						hAuthTicket = 0;
					}

					unsigned char ticket[1024];
					uint32 ticketSize;
					hAuthTicket = steamUser->GetAuthSessionTicket(ticket, sizeof(ticket), &ticketSize);
					if (hAuthTicket != k_HAuthTicketInvalid) {
						EBeginAuthSessionResult beginAuthSessionResult = steamGameServer->BeginAuthSession(ticket, ticketSize, steamUser->GetSteamID());
						if (beginAuthSessionResult == k_EBeginAuthSessionResultOK)
							bPlayingOnServer = true;
						else
							printf("BeginAuthSession failed (%d)\n", beginAuthSessionResult);
					}
					else {
						printf("GetAuthSessionTicket failed\n");
					}
				}

				// process steam game server callbacks
				while (Steam_BGetCallback(hSteamGameServerPipe, &callbackMsg))
				{
					switch (callbackMsg.m_iCallback)
					{
					case ValidateAuthTicketResponse_t::k_iCallback:
					{
						ValidateAuthTicketResponse_t* validateAuthTicketResponse = (ValidateAuthTicketResponse_t*)callbackMsg.m_pubParam;
						if (validateAuthTicketResponse->m_eAuthSessionResponse == k_EAuthSessionResponseOK) {
							printf("BeginAuthSession callback ok\n");
							//steamGameServer->BUpdateUserData( validateAuthTicketResponse->m_SteamID, "Player", 0 );
						}
						else {
							printf("BeginAuthSession callback failed (%d)\n", validateAuthTicketResponse->m_eAuthSessionResponse);
							bPlayingOnServer = false;
						}
						break;
					}
					/*default:
						printf( "Game server callback: %d\n", callbackMsg.m_iCallback );
						break;*/
					}

					Steam_FreeLastCallback(hSteamGameServerPipe);
				}
			}
			else {
				printf("Give me item!!!\n");
				steamInventory->SendItemDropHeartbeat();

				SteamInventoryResult_t steamInventoryResult;
				steamInventory->TriggerItemDrop(&steamInventoryResult, dropListDefinition);
				steamInventory->DestroyResult(steamInventoryResult);
			}
		}

		sleep(10);
	}
}

S_API bool STEAM_CALL Steam_BGetCallback(HSteamPipe hSteamPipe, CallbackMsg_t* pCallbackMsg)
{
	static bool (STEAM_CALL * _Steam_BGetCallback)(HSteamPipe, CallbackMsg_t*) = NULL;
	if (!_Steam_BGetCallback)
	{
		void* hSteamClient = dlopen("./Open Steamworks/Resources/Libs/steamclient.so", RTLD_LAZY);
		if (hSteamClient)
		{
			_Steam_BGetCallback = (bool (STEAM_CALL*)(HSteamPipe, CallbackMsg_t*))dlsym(hSteamClient, "Steam_BGetCallback");
			dlclose(hSteamClient);
		}
	}

	return _Steam_BGetCallback(hSteamPipe, pCallbackMsg);
}

S_API void STEAM_CALL Steam_FreeLastCallback(HSteamPipe hSteamPipe)
{
	static void (STEAM_CALL * _Steam_FreeLastCallback)(HSteamPipe) = NULL;
	if (!_Steam_FreeLastCallback)
	{
		void* hSteamClient = dlopen("./Open Steamworks/Resources/Libs/steamclient.so", RTLD_LAZY);
		if (hSteamClient)
		{
			_Steam_FreeLastCallback = (void (STEAM_CALL*)(HSteamPipe)) dlsym(hSteamClient, "Steam_FreeLastCallback");
			dlclose(hSteamClient);
		}
	}
	return _Steam_FreeLastCallback(hSteamPipe);
}