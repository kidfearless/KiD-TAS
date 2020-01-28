#include <sourcemod>
#include <string_test>
#include <convar_class>
#include <thelpers/thelpers>
#include <kid_tas>

#include <sdkhooks>

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


bool _Enabled[MAXPLAYERS+1];
float _LastGain[MAXPLAYERS+1];
float _TimeScale[MAXPLAYERS+1] = {1.0, ...};
bool _AutoStrafe[MAXPLAYERS+1] = {true, ...};
bool _StrafeHack[MAXPLAYERS+1] = {true, ...};
bool _FastWalk[MAXPLAYERS+1] = {true, ...};
bool _AutoJump[MAXPLAYERS+1] = {true, ...};
int _Buttons[MAXPLAYERS+1];

methodmap Client < CBasePlayer
{
	public Client(int client)
	{
		return view_as<Client>(client);
	}

	public static Client Create(int client)
	{
		return new Client(client);
	}

	property int Index
	{
		public get()
		{
			return view_as<int>(this);
		}
	}

	property bool Enabled
	{
		public get()
		{
			return _Enabled[this.Index];
		}
		public set(bool newVal)
		{
			_Enabled[this.Index] = newVal;
		}
	}
	
	property int Buttons
	{
		public get()
		{
			return _Buttons[this.Index];
		}
		public set(int newVal)
		{
			_Buttons[this.Index] = newVal;
		}
	}

	property bool OnGround
	{
		public get()
		{
			return (!(this.Buttons & IN_JUMP) && (GetEntityFlags(this.Index) & FL_ONGROUND));
		}
	}

	property bool AutoJump
	{
		public get()
		{
			return _AutoJump[this.Index];
		}
		public set(bool newVal)
		{
			_AutoJump[this.Index] = newVal;
		}
	}

	property bool StrafeHack
	{
		public get()
		{
			return _StrafeHack[this.Index];
		}
		public set(bool newVal)
		{
			_StrafeHack[this.Index] = newVal;
		}
	}

	property bool AutoStrafe
	{
		public get()
		{
			return _AutoStrafe[this.Index];
		}
		public set(bool newVal)
		{
			_AutoStrafe[this.Index] = newVal;
		}
	}

	property bool FastWalk
	{
		public get()
		{
			return _FastWalk[this.Index];
		}
		public set(bool newVal)
		{
			_FastWalk[this.Index] = newVal;
		}
	}

	property float LastGain
	{
		public get()
		{
			return _LastGain[this.Index];
		}
		public set(float newVal)
		{
			_LastGain[this.Index] = newVal;
		}
	}

	property float TimeScale
	{
		public get()
		{
			return _TimeScale[this.Index];
		}
		public set(float newVal)
		{
			if(newVal < 0.0)
			{
				_TimeScale[this.Index] = 0.1;
			}
			else if(newVal > 1.0)
			{
				_TimeScale[this.Index] = 1.0;
			}
			else
			{
				_TimeScale[this.Index] = newVal;
			}

			string val;
			val.FromFloat(_TimeScale[this.Index]);

			host_timescale.ReplicateToClient(this.Index, val.StringValue);
		}
	}

	
	public void OpenMenu()
	{
		if(!this.Enabled)
		{
			return;
		}
		Menu menu = new Menu(MenuHandler_TAS);

		menu.SetTitle("TAS Menu\n");

		string buffer;

		menu.AddItem("cp", "Checkpoint Menu");

		buffer.Format("--Timescale\nCurrent Timescale: %.1f", this.TimeScale + 0.001);

		menu.AddItem("--", buffer.StringValue, (this.TimeScale == 0.0 ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT));


		menu.AddItem("++", "++Timescale", (this.TimeScale == 1.0 ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT));


		menu.AddItem("jmp", (this.AutoJump ? " [X] Auto-jump from start zone?" : "[ ] Auto-jump from start zone?"));


		menu.AddItem("as", (this.AutoStrafe ? "[X] Auto-Strafe" : "[ ] Auto-Strafe"));

		menu.AddItem("sh", (this.StrafeHack ? "[X] Strafe hack" : "[ ] Strafe Hack"));

		menu.Pagination = MENU_NO_PAGINATION;
		menu.ExitButton = true;
		menu.Display(this.Index, MENU_TIME_FOREVER);
	}

	public void ResetVariables()
	{
		this.TimeScale = 1.0;
		this.LastGain = 0.0;
		this.Enabled = false;
		this.StrafeHack = true;
		this.AutoStrafe = true;
		this.FastWalk = true;
	}

	public void ToggleTAS(int args)
	{
		this.Enabled = !this.Enabled;
		if(this.Enabled)
		{
			sv_cheats.ReplicateToClient(this.Index, "2");
			PrintToChat(this.Index, "For a better experience change the following convars:");
			PrintToChat(this.Index, "cl_clock_correction 0");
			PrintToChat(this.Index, "cl_clock_correction_force_server_tick 0");
			PrintToConsole(this.Index, "cl_clock_correction_force_server_tick 0;cl_clock_correction 0;");
			this.OpenMenu();
		}
		else
		{
			string convar;
			convar.FromConVar(g_cDefaultCheats);
			sv_cheats.ReplicateToClient(this.Index, convar.StringValue);
			host_timescale.ReplicateToClient(this.Index, "1");
		}
	}

	public void OnTick(int& buttons, float vel[3], float angles[3], int mouse[2])
	{
		this.Buttons = buttons;
		if(!this.IsAlive || !this.Enabled)
		{
			return;
		}

		// might investigate this again
		// SetEntProp(this.Index, Prop_Data, "m_nSimulationTick", GetGameTickCount());
	
		float vecvelocity[3];
		GetEntPropVector(this.Index, Prop_Data, "m_vecVelocity", vecvelocity);
		
		float perfAngleChange = RadToDeg(ArcTangent2(vecvelocity[1], vecvelocity[0]));
		
		float perfAngleDiff = NormalizeAngle(angles[1] - perfAngleChange);


		// if autostrafe and either not on the ground or on the ground and holding jump
		if(this.AutoStrafe && !this.OnGround)
		{
			vel[1] = 450.0;
	
			if (perfAngleDiff > 0.0)
			{
				vel[1] = -450.0;
			}	
		}
		
		// Check whether the player has tried to move their mouse more than the strafer
		float flAngleGain = RadToDeg(ArcTangent(vel[1] / vel[0]));
		
		// This check tells you when the mouse player movement is higher than the autostrafer one, and decide to put it or not
		if (!((this.LastGain < 0.0 && flAngleGain < 0.0) || (this.LastGain > 0.0 && flAngleGain > 0.0))) 
		{
			if(this.StrafeHack && !this.OnGround)
			{
				angles[1] -= perfAngleDiff;
			}
		}
		
		this.LastGain = flAngleGain;
	}

	public void OnPreThinkPost()
	{
		if(this.Enabled)
		{
			sv_clockcorrection_msecs.IntValue = 0;
			cl_clock_correction_force_server_tick.IntValue = -999;
			host_timescale.FloatValue = this.TimeScale;
		}
		else
		{
			host_timescale.RestoreDefault();
			cl_clock_correction_force_server_tick.RestoreDefault();
			sv_clockcorrection_msecs.RestoreDefault();
		}
	}

	public void OnPutInServer()
	{
		SDKHook(this.Index, SDKHook_PreThinkPost, OnPreThinkPost);
		this.ResetVariables();
	}
}

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	g_bLate = late;
	return APLRes_Success;
}

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

public void OnClientPutInServer(int client)
{
	Client.Create(client).OnPutInServer();
}

public Action Command_Tas(int client, int args)
{
	Client.Create(client).ToggleTAS(args);
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