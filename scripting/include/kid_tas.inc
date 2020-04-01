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
static bool _FastWalk[MAXPLAYERS+1] = {true, ...};
static bool _AutoJump[MAXPLAYERS+1] = {true, ...};
static bool _ForceJump[MAXPLAYERS+1];
static bool _FastWalkDirection[MAXPLAYERS+1];
static int _Buttons[MAXPLAYERS+1];
static stock int _Method[MAXPLAYERS+1];
static float _NextFrame[MAXPLAYERS+1];
static bool _UseThisFrame[MAXPLAYERS+1];
static MoveType _LastMoveType[MAXPLAYERS+1];
static int _XutaxType[MAXPLAYERS+1];

enum Client
{
	INVALID_CLIENT = -1,
	SERVER_CLIENT = 0,
	CLIENT_SIZE = MAXPLAYERS
}


stock Client operator=(int value)
{
	return view_as<Client>(value);
}

stock int operator=(Client value)
{
	return view_as<int>(value);
}

stock Client operator++(Client client)
{
	return view_as<Client>(view_as<int>(client)++);
}

stock Client operator--(Client client)
{
	return view_as<Client>(view_as<int>(client)--);
}

stock bool operator<(Client client, int value)
{
	return view_as<int>(client) < value;
}

stock bool operator<=(Client client, int value)
{
	return view_as<int>(client) <= value;
}

stock bool operator==(Client client, int value)
{
	return view_as<int>(client) == value;
}

stock bool operator>=(Client client, int value)
{
	return view_as<int>(client) >= value;
}

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

		property bool Valid
	{
		public get()
		{
			return (this > SERVER_CLIENT && this < CLIENT_SIZE);
		}
	}

	public bool IsValid(bool checkIfAlive = false, bool checkIfBot = true)
	{
		// checks for valid index, if they are connected and in game, with options for checking bots and alive
		return this.Valid && this.IsConnected && this.IsInGame && (!checkIfBot || (!this.IsFakeClient && !this.IsSourceTV)) && (!checkIfAlive || this.IsAlive);
	}

	public int GetProp( PropType propType, const char[] propName, int size = 4, int element = 0 )
	{
		return GetEntProp( this, propType, propName, size, element );
	}

	public int GetPropEnt( PropType propType, const char[] propName, int element = 0 )
	{
		return GetEntPropEnt( this, propType, propName, element );
	}

	public void PrintToConsole(const char[] format, any ...)
	{
		string buffer;
		VFormat(buffer.StringValue, buffer.Size(), format, 3);
		PrintToConsole(this, buffer.StringValue);
	}

	public void ReplyToCommand(const char[] format, any ...)
	{
		string buffer;
		VFormat(buffer.StringValue, buffer.Size(), format, 3);
		ReplyToCommand(this, buffer.StringValue);
	}

	public void Teleport(const float origin[3] = NULL_VECTOR, const float angles[3] = NULL_VECTOR, const float velocity[3] = NULL_VECTOR)
	{
		TeleportEntity(this, origin, angles, velocity);
	}

	property MoveType Movetype
	{
		public get()
		{
			return GetEntityMoveType(this);
		}
		public set(MoveType type)
		{
			SetEntityMoveType(this, type);
		}
	}

	property MoveType LastMoveType
	{
		public get()
		{
			return _LastMoveType[this];
		}
		public set(MoveType type)
		{
			_LastMoveType[this] = type;
		}
	}

	property int Style
	{
		public get()
		{
			return Shavit_GetBhopStyle(this);
		}
	}

	property int Track
	{
		public get()
		{
			return Shavit_GetClientTrack(this);
		}
	}

	property bool Enabled
	{
		public get()
		{
			return _Enabled[this];
		}
		public set(bool value)
		{
			_Enabled[this] = value;
			ToggleTAS(this);
		}
	}

	property bool ForceJump
	{
		public get()
		{
			return _ForceJump[this];
		}
		public set(bool value)
		{
			_ForceJump[this] = value;
		}
	}

	property int Buttons
	{
		public get()
		{
			return _Buttons[this];
		}
		public set(int value)
		{
			_Buttons[this] = value;
		}
	}

	property float NextFrameTime
	{
		public get()
		{
			return _NextFrame[this];
		}
		public set(float value)
		{
			_NextFrame[this] = value;
		}
	}

	property float Speed
	{
		public get()
		{
			float vecvelocity[3];
			GetEntPropVector(this, Prop_Data, "m_vecVelocity", vecvelocity);
			vecvelocity[2] = 0.0;
			return GetVectorLength(vecvelocity);
		}
	}

	public void GetVelocity(float vecvelocity[3])
	{
		GetEntPropVector(this, Prop_Data, "m_vecVelocity", vecvelocity);
	}

	property int GroundEntity
	{
		public get()
		{
			this.GetPropEnt(Prop_Send, "m_hGroundEntity");
		}
	}

	property bool OnGround
	{
		public get()
		{
			return (!(this.Buttons & IN_JUMP) && (this.GroundEntity != -1));
		}
	}

	property bool AutoJump
	{
		public get()
		{
			return _AutoJump[this];
		}
		public set(bool value)
		{
			_AutoJump[this] = value;
		}
	}

	property bool StrafeHack
	{
		public get()
		{
			return GetXutaxStrafe(this);
		}
		public set(bool value)
		{
			SetXutaxStrafe(this, value);
		}
	}

	property bool ProcessFrame
	{
		public get()
		{
			return _UseThisFrame[this];
		}
		public set(bool value)
		{
			_UseThisFrame[this] = value;
		}
	}

	property bool FastWalk
	{
		public get()
		{
			return _FastWalk[this];
		}
		public set(bool value)
		{
			_FastWalk[this] = value;
		}
	}

	property float LaggedMovementValue
	{
		public get()
		{
			return GetEntPropFloat(this, Prop_Data, "m_flLaggedMovementValue");
		}
		public set(float value)
		{
			SetEntPropFloat(this, Prop_Data, "m_flLaggedMovementValue", value);
		}
	}

	property float TimeScale
	{
		public get()
		{
			return _TimeScale[this];
		}
		public set(float value)
		{
			if(value < 0.1)
			{
				_TimeScale[this] = 0.1;
			}
			else if(value > 1.0)
			{
				_TimeScale[this] = 1.0;
			}
			else
			{
				_TimeScale[this] = value;
			}

			Shavit_SetClientTimescale(this, _TimeScale[this]);
			this.LaggedMovementValue = 1.0;
		}
	}

	property int Type
	{
		public get()
		{
			return _XutaxType[this];
		}
		public set(int value)
		{
			value %= XutaxType.Size;

			if(value != _XutaxType[this])
			{
				SetXutaxType(this, value);
				_XutaxType[this] = value;
			}
		}
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
		menu.Display(this, MENU_TIME_FOREVER);
	}

	public void ResetVariables()
	{
		if(this.IsFakeClient)
		{
			return;
		}

		_TimeScale[this] = 1.0;
		this.Enabled = false;
		this.FastWalk = true;
		this.Type = XutaxType.Surf;
	}

	public void DoFastWalk(float vel[3])
	{
		bool direction = _FastWalkDirection[this];

		if(this.Buttons & IN_FORWARD && !(this.Buttons & (IN_MOVELEFT | IN_MOVERIGHT)))
		{
			vel[1] = direction ? PERFSPEEDHACK : -PERFSPEEDHACK;
		}

		_FastWalkDirection[this] = !direction;
	}

	public void DoAutoJump(int& buttons)
	{
		if(this.ForceJump)
		{
			if(this.OnGround)
			{
				buttons |= IN_JUMP;
				this.ForceJump = false;
			}
			else
			{
				this.ForceJump = false;
			}
		}
	}

	public Client GetHUDTarget()
	{
		if(!this.IsSpectator)
		{
			return INVALID_CLIENT;
		}

		int mode = this.GetProp(Prop_Send, "m_iObserverMode");

		if(mode >= 3 && mode <= 5)
		{
			Client result = new Client(view_as<int>(this.GetPropEnt(Prop_Send, "m_hObserverTarget")));

			if(result.IsValid())
			{
				return result;
			}
		}
		
		return INVALID_CLIENT;
	}
}

stock void ToggleTAS(Client client)
{
	if(client.Enabled)
	{
		if(g_cSetCheats.BoolValue)
		{
			// Keeping this as it's TAS, should honestly have access to everything available
			Server.Cheats.ReplicateToClient(client, "2");
		}
		
		RequestFrame(Frame_OpenMenu, client.Serial);
	}
	else
	{
		client.TimeScale = 1.0;
		if(g_cSetCheats.BoolValue)
		{
			string_8 convar;
			convar.FromInt(Server.GetDefaultCheats());
			Server.Cheats.ReplicateToClient(client, convar.StringValue);
		}
	}
}

public void Frame_OpenMenu(int serial)
{
	Client client = new Client(GetClientFromSerial(serial));
	if(client.Valid && client.Enabled)
	{
		client.OpenMenu();
	}
}

#define DEBUG 0
#if DEBUG == 1
stock void DebugPrint(const char[] format, any args ...)
{
	string buffer;
	VFormat(buffer.StringValue, buffer.Size(), format, 2);
	PrintToChatAll(buffer.StringValue);
}
#endif

