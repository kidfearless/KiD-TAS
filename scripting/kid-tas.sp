#if defined __tas_included
	#endinput
#endif
#define __tas_included

#include <sourcemod>
#include <sdkhooks>
#include <shavit>

#include <string_test>
#include <convar_class>
#include <thelpers/thelpers>

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
ConVar cl_clock_correction_force_server_tick;
ConVar sv_clockcorrection_msecs;
Convar g_cDefaultCheats;
bool g_bLate;

#include <kid_tas>

public void OnPluginStart()
{
	RegConsoleCmd("sm_tas", Command_Tas, "opens tas");
	RegConsoleCmd("sm_tasmenu", Command_TasMenu, "opens tas menu");
	RegConsoleCmd("sm_timescale", Command_TimeScale, "sets timescale");

	sv_cheats = FindConVar("sv_cheats");
	host_timescale = FindConVar("host_timescale");
	cl_clock_correction_force_server_tick = FindConVar("cl_clock_correction_force_server_tick");
	sv_clockcorrection_msecs = FindConVar("sv_clockcorrection_msecs");

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

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	g_bLate = late;
	return APLRes_Success;
}

public void OnClientPutInServer(int client)
{
	Client.Create(client).OnPutInServer();
}

public Action Command_Tas(int client, int args)
{
	Client cl = new Client(client);
	cl.Enabled = !cl.Enabled;
	return Plugin_Handled;
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

public Action OnPlayerRunCmd(int client, int& buttons, int& impulse, float vel[3], float angles[3], int& weapon, int& subtype, int& cmdnum, int& tickcount, int& seed, int mouse[2])
{
	Client.Create(client).OnTick(buttons, vel, angles, mouse);
	return Plugin_Continue;
}

public void OnPreThinkPost(int client)
{
	Client.Create(client).OnPreThinkPost();
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
				else if (info.Equals("as"))
				{
					client.AutoStrafe = !client.AutoStrafe;
				}
				else if(info.Equals("sh"))
				{
					client.StrafeHack = !client.StrafeHack;
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
