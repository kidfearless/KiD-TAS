#if defined _kid_tas_included
	#endinput
#endif
#define _kid_tas_included

#define PERFSPEEDHACK 227.6
#include <sourcemod>
#include <sdkhooks>
#include <shavit>
#include <dhooks>

#include <string_test>
#include <convar_class>
#include <thelpers/thelpers>
#include <xutaxstrafe>
/*

TODO: TAS will be considered done when all of these things have been completed
1.	Calculated max prestrafe gains
2.	Rewinds
4.	Menu contains it's own cp menu
5.	Run saving (with or without checkpoints included)
*/

// private members
static bool _Enabled[MAXPLAYERS+1];
// static float _LastGain[MAXPLAYERS+1];
static float _TimeScale[MAXPLAYERS+1] = {1.0, ...};
static bool _StrafeHack[MAXPLAYERS+1] = {true, ...};
static bool _FastWalk[MAXPLAYERS+1] = {true, ...};
static bool _AutoJump[MAXPLAYERS+1] = {true, ...};
static bool _ForceJump[MAXPLAYERS+1];
static bool _FastWalkDirection[MAXPLAYERS+1];
static int _Buttons[MAXPLAYERS+1];
static int _Method[MAXPLAYERS+1];
static float _NextFrame[MAXPLAYERS+1];
static bool _UseThisFrame[MAXPLAYERS+1];
static MoveType _LastMoveType[MAXPLAYERS+1];
static int _XutaxType[MAXPLAYERS+1];

methodmap MethodMap
{
	property int Client
	{
		public get()
		{
			return 0;
		}
	}
	property int None
	{
		public get()
		{
			return -1;
		}
	}
	property int Server
	{
		public get()
		{
			return 1;
		}
	}
	property int ServerClient
	{
		public get()
		{
			return 2;
		}
	}
	property int Size
	{
		public get()
		{
			return 3;
		}
	}
}
MethodMap Method;

methodmap Client < CBasePlayer
{
	// constructors
	public Client(int client)
	{
		return view_as<Client>(client);
	}

	public static Client Create(int client)
	{
		return new Client(client);
	}

	// public members
	property int Index
	{
		public get()
		{
			return view_as<int>(this);
		}
	}

	property MoveType Movetype
	{
		public get()
		{
			return GetEntityMoveType(this.Index);
		}
		public set(MoveType type)
		{
			SetEntityMoveType(this.Index, type);
		}
	}

	property MoveType LastMoveType
	{
		public get()
		{
			return _LastMoveType[this.Index];
		}
		public set(MoveType type)
		{
			_LastMoveType[this.Index] = type;
		}
	}

	property int Style
	{
		public get()
		{
			return Shavit_GetBhopStyle(this.Index);
		}
	}

	property int Track
	{
		public get()
		{
			return Shavit_GetClientTrack(this.Index);
		}
	}

	property bool Enabled
	{
		public get()
		{
			return _Enabled[this.Index];
		}
		public set(bool value)
		{
			if(_Enabled[this.Index] != value)
			{
				_Enabled[this.Index] = value;
				ToggleTAS(this);
			}
		}
	}

	property bool ForceJump
	{
		public get()
		{
			return _ForceJump[this.Index];
		}
		public set(bool value)
		{
			_ForceJump[this.Index] = value;
		}
	}

	property int Buttons
	{
		public get()
		{
			return _Buttons[this.Index];
		}
		public set(int value)
		{
			_Buttons[this.Index] = value;
		}
	}

	property float NextFrameTime
	{
		public get()
		{
			return _NextFrame[this.Index];
		}
		public set(float value)
		{
			_NextFrame[this.Index] = value;
		}
	}

	property float Speed
	{
		public get()
		{
			float vecvelocity[3];
			GetEntPropVector(this.Index, Prop_Data, "m_vecVelocity", vecvelocity);
			vecvelocity[2] = 0.0;
			return GetVectorLength(vecvelocity);
		}
	}

	public void GetVelocity(float vecvelocity[3])
	{
		GetEntPropVector(this.Index, Prop_Data, "m_vecVelocity", vecvelocity);
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
		public set(bool value)
		{
			_AutoJump[this.Index] = value;
		}
	}

	property bool StrafeHack
	{
		public get()
		{
			return _StrafeHack[this.Index];
		}
		public set(bool value)
		{
			_StrafeHack[this.Index] = value;
			SetXutaxStrafe(this.Index, value);
		}
	}

	property bool ProcessFrame
	{
		public get()
		{
			return _UseThisFrame[this.Index];
		}
		public set(bool value)
		{
			_UseThisFrame[this.Index] = value;
		}
	}

	property bool FastWalk
	{
		public get()
		{
			return _FastWalk[this.Index];
		}
		public set(bool value)
		{
			_FastWalk[this.Index] = value;
		}
	}

	property float LaggedMovementValue
	{
		public get()
		{
			return GetEntPropFloat(this.Index, Prop_Data, "m_flLaggedMovementValue");
		}
		public set(float value)
		{
			SetEntPropFloat(this.Index, Prop_Data, "m_flLaggedMovementValue", value);
		}
	}

	property int Method
	{
		public get()
		{
			return _Method[this.Index];
		}
		public set(int value)
		{
			value %= Method.Size;
			if(Server.IsCSGO && value == Method.Client
			|| Server.IsCSS && value == Method.ServerClient)
			{
				++value;
				value %= Method.Size;
			}

			if(value != _Method[this.Index])
			{
				if(value == Method.Server)
				{
					Server.HostTimescale.ReplicateToClient(this.Index, "1");
				}
				else
				{
					Shavit_SetClientTimescale(this.Index, _TimeScale[this.Index]);
					this.LaggedMovementValue = 1.0;
					if(value == Method.Client || value == Method.ServerClient)
					{
						string sValue;
						sValue.FromFloat(_TimeScale[this.Index]);
						Server.HostTimescale.ReplicateToClient(this.Index, sValue.StringValue);
					}
				}
				_Method[this.Index] = value;
			}
		}
	}

	property float TimeScale
	{
		public get()
		{
			return _TimeScale[this.Index];
		}
		public set(float value)
		{
			if(!this.Enabled)
			{
				return;
			}

			if(value < 0.1)
			{
				_TimeScale[this.Index] = 0.1;
			}
			else if(value > 1.0)
			{
				_TimeScale[this.Index] = 1.0;
			}
			else
			{
				_TimeScale[this.Index] = value;
			}


			Shavit_SetClientTimescale(this.Index, _TimeScale[this.Index]);
			this.LaggedMovementValue = 1.0;
			if(this.Method == Method.Client || this.Method == Method.ServerClient)
			{
				string sValue;
				sValue.FromFloat(_TimeScale[this.Index]);
				Server.HostTimescale.ReplicateToClient(this.Index, sValue.StringValue);
			}
		}
	}

	property int Type
	{
		public get()
		{
			return _XutaxType[this.Index];
		}
		public set(int value)
		{
			value %= XutaxType.Size;

			if(value != _XutaxType[this.Index])
			{
				SetXutaxType(this.Index, value);
				_XutaxType[this.Index] = value;
			}
		}
	}

	public void PrintToConsole(const char[] format, any ...)
	{
		string buffer;
		VFormat(buffer.StringValue, buffer.Size(), format, 3);
		PrintToConsole(this.Index, buffer.StringValue);
	}

	public void ReplyToCommand(const char[] format, any ...)
	{
		string buffer;
		VFormat(buffer.StringValue, buffer.Size(), format, 3);
		ReplyToCommand(this.Index, buffer.StringValue);
	}

	public void OpenMenu()
	{
		if(!this.Enabled)
		{
			// DebugPrint("OpenMenu Not Enabled");

			return;
		}

		Menu menu = new Menu(MenuHandler_TAS);

		menu.SetTitle("TAS Menu\n");

		string buffer;

		menu.AddItem("cp", "Checkpoint Menu");

		buffer.Format("--Timescale\nCurrent Timescale: %.1f", this.TimeScale + 0.001);

		menu.AddItem("--", buffer.StringValue, ITEMDRAW_DEFAULT);


		menu.AddItem("++", "++Timescale",   ITEMDRAW_DEFAULT);


		menu.AddItem("jmp", (this.AutoJump ? "[X] Auto-jump from start zone?" : "[ ] Auto-jump from start zone?"));

		menu.AddItem("sh", (this.StrafeHack ? "[X] Strafe hack" : "[ ] Strafe Hack"));
		
		if(this.StrafeHack)
		{
			if(this.Method == Method.Client)
			{
				menu.AddItem("met", "Method: Client");
			}
			else if(this.Method == Method.Server)
			{
				menu.AddItem("met", "Method: Server");
			}
			else if(this.Method == Method.ServerClient)
			{
				menu.AddItem("met", "Method: Server + Client");
			}

			if(this.Type == XutaxType.Normal)
			{
				menu.AddItem("ty", "Type: Normal");
			}
			else if(this.Type == XutaxType.Surf)
			{
				menu.AddItem("ty", "Type: Surf");
			}
			else if(this.Type == XutaxType.Manual)
			{
				menu.AddItem("ty", "Type: Manual");
			}
		}
		menu.Pagination = MENU_NO_PAGINATION;
		menu.ExitButton = true;
		menu.Display(this.Index, MENU_TIME_FOREVER);
	}

	public void ResetVariables()
	{
		this.TimeScale = 1.0;
		this.Enabled = false;
		_StrafeHack[this.Index] = true;
		this.FastWalk = true;
		this.Method = Method.ServerClient;
		this.Type = XutaxType.Surf;
	}

	public void DoFastWalk(float vel[3])
	{
		bool direction = _FastWalkDirection[this.Index];

		if(this.Buttons & IN_FORWARD && !(this.Buttons & (IN_MOVELEFT | IN_MOVERIGHT)))
		{
			vel[1] = direction ? PERFSPEEDHACK : -PERFSPEEDHACK;
		}

		_FastWalkDirection[this.Index] = !direction;
	}

	public void DoAutoJump(int& buttons)
	{
		if(this.ForceJump)
		{
			if(this.OnGround)
			{
				buttons |= IN_JUMP;
			}
			else
			{
				this.ForceJump = false;
			}
		}
	}
}

void ToggleTAS(Client client)
{
	if(client.Enabled)
	{
		// Keeping this as it's TAS, should honestly have access to everything available
		Server.Cheats.ReplicateToClient(client.Index, "2");
		client.OpenMenu();
	}
	else
	{
		string_8 convar;
		convar.FromInt(Server.GetDefaultCheats());
		Server.Cheats.ReplicateToClient(client.Index, convar.StringValue);
		Server.HostTimescale.ReplicateToClient(client.Index, "1");
	}
}

#define DEBUG 0
#if DEBUG == 1
void DebugPrint(const char[] format, any args ...)
{
	string buffer;
	VFormat(buffer.StringValue, buffer.Size(), format, 2);
	PrintToChatAll(buffer.StringValue);
}
#endif

native bool TAS_ShouldProcessFrame(int client);

public SharedPlugin __pl_kid_tas =
{
	name = "kid-tas",
#if defined REQUIRE_PLUGIN
	required = 0
#else
	required = 0
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_kid_tas_SetNTVOptional()
{
	MarkNativeAsOptional("TAS_ShouldProcessFrame");
}
#endif