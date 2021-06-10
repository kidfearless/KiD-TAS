#if defined __tas_included
	#endinput
#endif
#define __tas_included

/*
	Due to issues with host_timescale on csgo and css it will no longer be an option.
	CSGO:	Voice comms become distorted
	CSS:	Extra frames are processed in the replay files(possibly csgo too)
	BOTH: 	Is dependent on the client being honest and not changing the timescale themselves.

	To use the host_timescale method the server must have sv_maxusrcmdprocessticks 0.
	This will disable the speed hack prevention on the server and allow clients to
	Send less than the servers tickrate in frames.
*/

#include <sourcemod>
#include <sdkhooks>
#include <shavit>
#include <dhooks>
#include <cstrike>

#include <strings_struct>
#include <convar_class>
#include <thelpers/thelpers>
#include <xutaxstrafe>
#include <kid_tas_api>

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
Convar g_cSetCheats;
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

methodmap TypeMap
{
	property int Normal
	{
		public get()
		{
			return Type_Normal;
		}
	}
	property int Surf
	{
		public get()
		{
			return Type_SurfOverride;
		}
	}
	property int Manual
	{
		public get()
		{
			return Type_Override;
		}
	}
	property int Size
	{
		public get()
		{
			return Type_Size;
		}
	}
}

TypeMap XutaxType;
ServerMap Server;

#include <kid_tas>

//========================================================================================
/*                                                                                      *
 *                                        Startup                                       *
 *                                                                                      */
//========================================================================================

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	CreateNative("TAS_ShouldProcessFrame", Native_ShouldProcess);
	CreateNative("TAS_GetCurrentTimescale", Native_GetCurrentTimescale);
	CreateNative("TAS_Enabled", Native_Enabled);

	RegPluginLibrary("kid-tas");

	Server.IsLate = late;
	return APLRes_Success;
}

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("shavit-common.phrases");
	LoadTranslations("shavit-misc.phrases");
	LoadTranslations("kid-tas.phrases");
	// thanks xutaxkamay for pointing my head back to rngfix after i gave up on it.
	// dhooks
	LoadDHooks();

	// commands
	RegConsoleCmd("sm_tasmenu", Command_TasMenu, "opens tas menu");
	RegConsoleCmd("sm_timescale", Command_TimeScale, "sets timescale");

	// convars
	Server.Cheats = FindConVar("sv_cheats");
	Server.HostTimescale = FindConVar("host_timescale");

	g_cDefaultCheats = new Convar("kid_tas_cheats_default", "0", "Default sv_cheats value, used for servers that set cheats on connect.");
	g_cSetCheats = new Convar("kid_tas_cheats_enabled", "0", "Should we set sv_cheats on clients in TAS");

	Convar.AutoExecConfig();

	// late loading stuff
	if(g_bLate)
	{
		for(int i = 1; i <= MaxClients; ++i)
		{
			Client client = new Client(i);
			if(client.IsConnected && client.IsInGame && !client.IsFakeClient)
			{
				OnClientPutInServer(i);
			}
		}
	}

	// OnPluginStarted();
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

public void OnPluginEnd()
{
	if(g_cSetCheats.BoolValue)
	{
		for(int i = 1; i <= MaxClients; ++i)
		{
			Client client = new Client(i);
			if(client.Enabled && client.IsConnected)
			{
				string_8 convar;
				convar.FromInt(Server.GetDefaultCheats());
				Server.Cheats.ReplicateToClient(client.Index, convar.Value);
			}
		}
	}
}

//========================================================================================
/*                                                                                      *
 *                                    Global Forwards                                   *
 *                                                                                      */
//========================================================================================

public void OnClientPutInServer(int index)
{
	// Client client = new Client(index);
	SDKHook(index, SDKHook_PreThinkPost, OnPreThinkPost);
	SDKHook(index, SDKHook_PostThinkPost, OnPostThink);
}

public void OnClientDisconnect(int index)
{
	Client client = new Client(index);
	client.ResetVariables();
}

public Action Shavit_OnUserCmdPre(int index, int &buttons, int &impulse, float vel[3], float angles[3], TimerStatus status, int track, int style)
{
	Client client = new Client(index);
	client.Buttons = buttons;

	if(!client.IsAlive || !client.Enabled)
	{
		return Plugin_Continue;
	}

	if(!client.ProcessFrame)
	{
		return Plugin_Continue;
	}

	if(client.AutoJump)
	{
		client.DoAutoJump(buttons);
		return Plugin_Changed;
	}

	return Plugin_Continue;
}

public void Shavit_OnTimeIncrement(int index, timer_snapshot_t snapshot, float &time)
{
}

public void OnPlayerRunCmdPost(int index, int buttons, int impulse, const float vel[3], const float angles[3], int weapon, int subtype, int cmdnum, int tickcount, int seed, const int mouse[2])
{
	// Client client = new Client(index);
}

public void Shavit_OnLeaveZone(int index, int type, int track, int id, int entity, int data)
{
	if(type == Zone_Start)
	{
		Client client = new Client(index);
		if(client.OnGround)
		{
			client.ForceJump = true;
		}
	}
}

public void Shavit_OnStyleChanged(int index, int oldstyle, int newstyle, int track, bool manual)
{
	string_128 special;
	Shavit_GetStyleStrings(newstyle, sSpecialString, special.Value, special.Size());

	Client.Create(index).Enabled = special.Includes("TAS");
}

public Action Shavit_OnCheckPointMenuMade(int index, bool segmented)
{
	Client client = new Client(index);
	if(client.Enabled)
	{
		return Plugin_Handled;
	}
	return Plugin_Continue;
}

//========================================================================================
/*                                                                                      *
 *                                   Private Forwards                                   *
 *                                                                                      */
//========================================================================================

public void OnPreThinkPost(int index)
{
	// Client client = new Client(index);
}

public void OnPostThink(int index)
{
	// Client client = new Client(index);
}

//========================================================================================
/*                                                                                      *
 *                                Process Movement DHooks                               *
 *                                                                                      */
//========================================================================================

public MRESReturn DHook_ProcessMovementPre(Handle hParams)
{
	int index = DHookGetParam(hParams, 1);
	Client client = new Client(index);
	if(!client.Enabled)
	{
		client.ProcessFrame = true;
		return MRES_Ignored;
	}

	if(client.NextFrameTime <= 0.0)
	{
		client.NextFrameTime += (1.0 - client.TimeScale);
		client.LastMoveType = client.Movetype;
		client.ProcessFrame = (client.NextFrameTime <= 0.0);
		client.LaggedMovementValue = 1.0;
		return MRES_Ignored;
	}
	else
	{
		client.NextFrameTime -= client.TimeScale;
		client.Movetype = MOVETYPE_NONE;
		client.ProcessFrame = (client.NextFrameTime <= 0.0);

		return MRES_Ignored;
	}
}

public MRESReturn DHook_ProcessMovementPost(Handle hParams)
{
	int index = DHookGetParam(hParams, 1);
	Client client = new Client(index);

	if(client.Enabled)
	{
		client.Movetype = client.LastMoveType;
		client.LaggedMovementValue = client.TimeScale;
	}
}

//========================================================================================
/*                                                                                      *
 *                                Handlers and Callbacks                                *
 *                                                                                      */
//========================================================================================

public Action Command_TasMenu(int index, int args)
{
	Client client = new Client(index);
	if(client.Enabled)
	{
		client.OpenMenu();
	}
	else
	{
		client.ReplyToCommand("You must be in TAS first");
	}

	return Plugin_Handled;
}

public Action Command_TimeScale(int index, int args)
{
	string arg;
	arg.GetCmdArg(1);

	float scale = arg.FloatValue();

	Client client = new Client(index);
	if(!client.Enabled)
	{
		client.ReplyToCommand("You must be on TAS first.");
	}
	else
	{
		client.TimeScale = scale;
	}

	return Plugin_Handled;
}

//========================================================================================
/*                                                                                      *
 *                                        Natives                                       *
 *                                                                                      */
//========================================================================================

public any Native_ShouldProcess(Handle time, int numParams)
{
	return Client.Create(GetNativeCell(1)).ProcessFrame;
}

public any Native_GetCurrentTimescale(Handle time, int numParams)
{
	return Client.Create(GetNativeCell(1)).TimeScale;
}

public any Native_Enabled(Handle time, int numParams)
{
	return Client.Create(GetNativeCell(1)).Enabled;
}

//========================================================================================
/*                                                                                      *
 *                                        Stocks                                        *
 *                                                                                      */
//========================================================================================

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
