#if defined __tas_included
	#endinput
#endif
#define __tas_included

#include <sourcemod>
#include <sdkhooks>
#include <shavit>
#include <dhooks>

#include <string_test>
#include <convar_class>
#include <thelpers/thelpers>
#include <xutaxstrafe>

public Plugin myinfo = 
{
	name = "Shavit - TAS", 
	author = "KiD Fearless", 
	description = "TAS module for shavits timer.", 
	version = "2.0", 
	url = "https://github.com/kidfearless/"
};

ConVar sv_cheats;
ConVar host_timescale;
Convar g_cDefaultCheats;
bool g_bLate;

methodmap ServerMap
{
	property ConVar Cheats
	{
		public get()
		{
			return sv_cheats;
		}
		public set(ConVar value)
		{
			sv_cheats = value;
		}
	}
	property ConVar HostTimescale
	{
		public get()
		{
			return host_timescale;
		}
		public set(ConVar value)
		{
			host_timescale = value;
		}
	}
	property bool IsLate
	{
		public get()
		{
			return g_bLate;
		}
		public set(bool value)
		{
			g_bLate = value;
		}
	}
	property bool IsCSGO
	{
		public get()
		{
			return GetEngineVersion() == Engine_CSGO;
		}
	}
	property bool IsCSS
	{
		public get()
		{
			return GetEngineVersion() == Engine_CSS;
		}
	}
	public int GetDefaultCheats()
	{
		return g_cDefaultCheats.IntValue;
	}
}

ServerMap Server;

#include <kid_tas>

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{

	CreateNative("TAS_ShouldProcessFrame", Native_ShouldProcess);

	Server.IsLate = late;
	return APLRes_Success;
}

public void OnPluginStart()
{
	// thanks xutaxkamay for pointing my head back to rngfix after i gave up on it.
	LoadDHooks();

	RegConsoleCmd("sm_tasmenu", Command_TasMenu, "opens tas menu");
	RegConsoleCmd("sm_timescale", Command_TimeScale, "sets timescale");

	Server.Cheats = FindConVar("sv_cheats");
	Server.HostTimescale = FindConVar("host_timescale");

	g_cDefaultCheats = new Convar("kid_tas_cheats_default", "2", "Default sv_cheats value, used for servers that set cheats on connect.");

	Convar.AutoExecConfig();

	if(g_bLate)
	{
		for(int i = 1; i <= MaxClients; ++i)
		{
			Client client = new Client(i);
			if(client.IsConnected && client.IsInGame && !client.IsFakeClient)
			{
				client.OnPutInServer();
			}
		}
	}
}

public void OnPluginEnd()
{
	for(int i = 1; i <= MaxClients; ++i)
	{
		Client client = new Client(i);
		if(client.Enabled)
		{
			string_8 convar;
			convar.FromInt(Server.GetDefaultCheats());
			Server.Cheats.ReplicateToClient(client.Index, convar.StringValue);
			Server.HostTimescale.ReplicateToClient(client.Index, "1");
		}
	}
}

void LoadDHooks()
{
	// totally not ripped from rngfix :)
	Handle gamedataConf = LoadGameConfigFile("KiD-TAS.games");

	if(gamedataConf == null)
	{
		SetFailState("Failed to load KiD-TAS gamedata");
	}

	// CreateInterface
	// Thanks SlidyBat and ici
	StartPrepSDKCall(SDKCall_Static);
	if(!PrepSDKCall_SetFromConf(gamedataConf, SDKConf_Signature, "CreateInterface"))
	{
		SetFailState("Failed to get CreateInterface");
	}
	PrepSDKCall_AddParameter(SDKType_String, SDKPass_Pointer);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Pointer, VDECODE_FLAG_ALLOWNULL);
	PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_Plain);
	Handle CreateInterface = EndPrepSDKCall();

	if(CreateInterface == null)
	{
		SetFailState("Unable to prepare SDKCall for CreateInterface");
	}

	char interfaceName[64];

	// ProcessMovement
	if(!GameConfGetKeyValue(gamedataConf, "IGameMovement", interfaceName, sizeof(interfaceName)))
	{
		SetFailState("Failed to get IGameMovement interface name");
	}

	Address IGameMovement = SDKCall(CreateInterface, interfaceName, 0);
	
	if(!IGameMovement)
	{
		SetFailState("Failed to get IGameMovement pointer");
	}

	int offset = GameConfGetOffset(gamedataConf, "ProcessMovement");
	if(offset == -1)
	{
		SetFailState("Failed to get ProcessMovement offset");
	}

	Handle processMovement = DHookCreate(offset, HookType_Raw, ReturnType_Void, ThisPointer_Ignore, DHook_ProcessMovementPre);
	DHookAddParam(processMovement, HookParamType_CBaseEntity);
	DHookAddParam(processMovement, HookParamType_ObjectPtr);
	DHookRaw(processMovement, false, IGameMovement);

	Handle processMovementPost = DHookCreate(offset, HookType_Raw, ReturnType_Void, ThisPointer_Ignore, DHook_ProcessMovementPost);
	DHookAddParam(processMovementPost, HookParamType_CBaseEntity);
	DHookAddParam(processMovementPost, HookParamType_ObjectPtr);
	DHookRaw(processMovementPost, true, IGameMovement);

	

	delete CreateInterface;
	delete gamedataConf;
}

public void OnClientPutInServer(int client)
{
	Client.Create(client).OnPutInServer();
}

public void OnClientDisconnect(int client)
{
	Client.Create(client).OnDisconnect();
}

public Action Command_TasMenu(int client, int args)
{
	Client cl = new Client(client);
	if(cl.Enabled)
	{
		cl.OpenMenu();
	}
	else
	{
		ReplyToCommand(client, "You must be in TAS first");
	}
	return Plugin_Handled;
}

public Action Command_TimeScale(int client, int args)
{
	string arg;
	arg.GetCmdArg(1);

	float scale = arg.FloatValue();

	Client.Create(client).TimeScale = scale;

	return Plugin_Handled;
}

public Action Shavit_OnUserCmdPre(int client, int &buttons, int &impulse, float vel[3], float angles[3], TimerStatus status, int track, int style, stylesettings_t stylesettings, int mouse[2])
{
	return Client.Create(client).OnTick(buttons, vel, angles, mouse);
}

public void OnPlayerRunCmdPost(int client, int buttons, int impulse, const float vel[3], const float angles[3], int weapon, int subtype, int cmdnum, int tickcount, int seed, const int mouse[2])
{
	Client.Create(client).OnTickPost(client, buttons, impulse, vel, angles, weapon, subtype, cmdnum, tickcount, seed, mouse);
}

public void OnPreThinkPost(int client)
{
	Client.Create(client).OnPreThinkPost();
}

public void OnPostThink(int client)
{
	Client.Create(client).OnPostThink();
}

public MRESReturn DHook_ProcessMovementPre(Handle hParams)
{
	int client = DHookGetParam(hParams, 1);
	return Client.Create(client).OnProcessMovement();
}

public MRESReturn DHook_ProcessMovementPost(Handle hParams)
{
	int client = DHookGetParam(hParams, 1);
	return Client.Create(client).OnProcessMovementPost();
}

public int MenuHandler_TAS(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			Client client = new Client(param1);
			string info;
			info.GetMenuInfo(menu, param2);

			if(!client.Enabled)
			{
				return 0;
			}

			if (info.Equals("cp"))
			{
				FakeClientCommand(client.Index, "sm_cpmenu");
			}
			else
			{
				if (info.Equals("++"))
				{
					client.TimeScale += 0.1;
				}
				else if (info.Equals("--"))
				{
					client.TimeScale -= 0.1;
				}
				else if (info.Equals("jump"))
				{
					client.AutoJump = !client.AutoJump;
				}
				else if(info.Equals("sh"))
				{
					client.StrafeHack = !client.StrafeHack;
				}
				else if(info.Equals("met"))
				{
					++client.Method;
					if(Server.IsCSGO && client.Method == Method.Client)
					{
						++client.Method;
					}
				}


				client.OpenMenu();
			}
		}

		case MenuAction_End:
		{
			delete menu;
		}
	}

	return 0;
}

public void Shavit_OnLeaveZone(int client, int type, int track, int id, int entity, int data)
{
	if(type == Zone_Start)
	{
		Client.Create(client).OnLeaveStartZone();
	}
}

public void Shavit_OnStyleChanged(int client, int oldstyle, int newstyle, int track, bool manual)
{
	string_128 special;
	Shavit_GetStyleStrings(newstyle, sSpecialString, special.StringValue, special.Size());

	Client.Create(client).Enabled = special.Includes("TAS");
}

// Stocks
public float NormalizeAngle(float angle)
{
	float temp = angle;
	
	while (temp <= -180.0)
	{
		temp += 360.0;
	}
	
	while (temp > 180.0)
	{
		temp -= 360.0;
	}
	
	return temp;
}

public any Native_ShouldProcess(Handle time, int numParams)
{
	return Client.Create(GetNativeCell(1)).ProcessFrame;
}