#if defined __tas_included
	#endinput
#endif
#define __tas_included

#include <sourcemod>
#include <sdkhooks>
#include <shavit>
#include <dhooks>
#include <cstrike>

#include <string_test>
#include <convar_class>
#include <thelpers/thelpers>
#include <xutaxstrafe>

public Plugin myinfo = 
{
	name = "Shavit - TAS", 
	author = "KiD Fearless", 
	description = "TAS module for shavits timer.", 
	version = "2.1", 
	url = "https://github.com/kidfearless/"
};

stock StringMap g_smCheckPoints;

#include <methodmaps>
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

	Server.IsLate = late;
	return APLRes_Success;
}

public void OnPluginStart()
{
	// thanks xutaxkamay for pointing my head back to rngfix after i gave up on it.
	// dhooks
	LoadDHooks();

	// commands
	RegConsoleCmd("sm_tasmenu", Command_TasMenu, "opens tas menu");
	RegConsoleCmd("sm_timescale", Command_TimeScale, "sets timescale");
	RegConsoleCmd("+rewind", Command_Rewind, "turns on rewinding");
	RegConsoleCmd("-rewind", Command_Rewind, "sets off rewinding");
	RegConsoleCmd("+fastforward", Command_Forward, "turns on fast forwarding");
	RegConsoleCmd("-fastforward", Command_Forward, "turns off fast forwarding");

	// convars
	Server.Cheats = FindConVar("sv_cheats");
	Server.HostTimescale = FindConVar("host_timescale");

	Server.DefaultCheats = new Convar("kid_tas_cheats_default", "2", "Default sv_cheats value, used for servers that set cheats on connect.");

	Convar.AutoExecConfig();


	// frames
	for(Client client = 0; client <= MaxClients; ++client)
	{
		client.Frames = new ArrayList(sizeof(TASFrame));
	}

	// checkpoints
	g_smCheckPoints = new StringMap();

	// late loading stuff
	if(Server.IsLate)
	{
		for(Client client = 0; client <= MaxClients; ++client)
		{
			if(client.IsConnected && client.IsInGame && !client.IsFakeClient)
			{
				OnClientPutInServer(client);
			}
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

public void OnPluginEnd()
{
	for(Client client = 0; client <= MaxClients; ++client)
	{
		if(client.Enabled)
		{
			string_8 convar;
			convar.FromInt(Server.GetDefaultCheats());
			Server.Cheats.ReplicateToClient(client, convar.StringValue);
			Server.HostTimescale.ReplicateToClient(client, "1");
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
	Client client = new Client(index);
	SDKHook(client, SDKHook_PreThinkPost, OnPreThinkPost);
	SDKHook(client, SDKHook_PostThinkPost, OnPostThink);
	client.ResetVariables();
	client.Frames.Clear();
}

public void OnClientDisconnect(int index)
{
	// Client client = new Client(index);
	SetXutaxStrafe(index, false);
}

public Action Shavit_OnUserCmdPre(int index, int &buttons, int &impulse, float vel[3], float angles[3], TimerStatus status, int track, int style, stylesettings_t stylesettings, int mouse[2])
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

	Action returnValue = Plugin_Continue;

	if(client.AutoJump)
	{
		client.DoAutoJump(buttons);
		returnValue = Plugin_Changed;
	}

	if(client.InStartZone)
	{
		client.Frames.Clear();
	}
	else
	{		
		TASFrame frame;
		frame.Update(client);
		client.Frames.PushArray(frame);
	}
	

	return returnValue;
}

public void OnPlayerRunCmdPost(int index, int buttons, int impulse, const float vel[3], const float angles[3], int weapon, int subtype, int cmdnum, int tickcount, int seed, const int mouse[2])
{
	// create a Client from the player they are spectating
	// So we just check if the target we get back is valid and on TAS.
	Client client = new Client(index);

	// check for valid client index
	if(!client.IsValid(.checkIfAlive = false))
	{
		// client.PrintToConsole("invalid");
		return;
	}

	if(!client.IsAlive)
	{
		client.TimeScale = 1.0;
	}
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
	Shavit_GetStyleStrings(newstyle, sSpecialString, special.StringValue, special.Size());

	Client.Create(index).Enabled = special.Includes("TAS");
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
	Client client = DHookGetParam(hParams, 1);
	if(!client.Enabled || client.Method == Method.Client)
	{
		client.ProcessFrame = true;
		return MRES_Ignored;
	}

	if(client.NextFrameTime <= 0.0)
	{
		client.NextFrameTime += (1.0 - client.TimeScale);
		client.LastMoveType = client.Movetype;
		client.ProcessFrame = (client.NextFrameTime <= 0.0);

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
	Client client = DHookGetParam(hParams, 1);

	if(client.Enabled && client.Method != Method.Client)
	{
		client.Movetype = client.LastMoveType;
	}
}

//========================================================================================
/*                                                                                      *
 *                                       Commands                                       *
 *                                                                                      */
//========================================================================================

public Action Command_TasMenu(int index, int args)
{
	Client client = new Client(index);
	if(client.Enabled)
	{
		client.OpenTASMenu();
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

public Action Command_Rewind(int index, int args)
{
	Client client = new Client(index);
	if(client.Enabled)
	{
		client.Rewind = !client.Rewind;
	}
	else
	{
		client.ReplyToCommand("You must be on TAS first.");
	}

	return Plugin_Handled;
}


public Action Command_Forward(int index, int args)
{
	Client client = new Client(index);
	if(client.Enabled)
	{
		client.FastForward = !client.FastForward;
	}
	else
	{
		client.ReplyToCommand("You must be on TAS first.");
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