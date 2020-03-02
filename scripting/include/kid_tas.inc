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
static ArrayList _Frames[MAXPLAYERS+1];
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

enum Client
{
	INVALID_CLIENT = -1,
	SERVER_CLIENT = 0,
	CLIENT_SIZE = MAXPLAYERS
}

// implicit casting
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
	public Client(int client = 0)
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

	property bool InStartZone
	{
		public get()
		{
			return Shavit_InsideZone(this, Zone_Start, -1);
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
			if(_Enabled[this] != value)
			{
				_Enabled[this] = value;
				ToggleTAS(this);
			}
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

	property bool Ducked
	{
		public get()
		{
			return !!GetEntProp(this, Prop_Send, "m_bDucked");
		}
		public set(bool value)
		{
			SetEntProp(this, Prop_Send, "m_bDucked", value);
		}
	}

	property bool Ducking
	{
		public get()
		{
			return !!GetEntProp(this, Prop_Send, "m_bDucking");
		}
		public set(bool value)
		{
			SetEntProp(this, Prop_Send, "m_bDucking", value);
		}
	}

	property float Stamina
	{
		public get()
		{
			return GetEntPropFloat(this, Prop_Send, "m_flStamina");
		}
		public set(float value)
		{
			SetEntProp(this, Prop_Send, "m_flStamina", value);
		}
	}

	property float DuckSpeed
	{
		public get()
		{
			return GetEntPropFloat(this, Prop_Send, "m_flDuckSpeed");
		}
		public set(float value)
		{
			SetEntProp(this, Prop_Send, "m_flDuckSpeed", value);
		}
	}

	property float DuckTime
	{
		public get()
		{
			if(Server.IsCSS)
			{
				GetEntPropFloat(this, Prop_Send, "m_flDucktime");
			}
			else
			{
				GetEntPropFloat(this, Prop_Send, "m_flDuckAmount");
			}
		}
		public set(float value)
		{
			if(Server.IsCSS)
			{
				SetEntPropFloat(this, Prop_Send, "m_flDucktime", value);
			}
			else
			{
				SetEntPropFloat(this, Prop_Send, "m_flDuckAmount", value);
			}
		}
	}

	property int GroundEntity
	{
		public get()
		{
			return GetEntPropEnt(this, Prop_Send, "m_hGroundEntity");
		}
		public set(int value)
		{
			SetEntPropEnt(this, Prop_Data, "m_hGroundEntity", value);
		}
	}

	property int Flags
	{
		public get()
		{
			return GetEntityFlags(this);
		}
		public set(int value)
		{
			SetEntityFlags(this, value);
		}
	}

	property ArrayList Frames
	{
		public get()
		{
			return _Frames[this];
		}
		public set(ArrayList value)
		{
			_Frames[this] = value;
		}
	}

	public void GetVelocity(float vecvelocity[3])
	{
		GetEntPropVector(this, Prop_Data, "m_vecVelocity", vecvelocity);
	}

	public void GetBaseVelocity(float vec[3])
	{
		GetEntPropVector(this, Prop_Data, "m_vecBaseVelocity", vec);
	}

	public void GetTargetName(char[] buffer, int length = 64)
	{
		GetEntPropString(this, Prop_Data, "m_iName", buffer, length);
	}

	public void SetTargetName(char[] buffer)
	{
		SetEntPropString(this, Prop_Data, "m_iName", buffer);
	}

	public void GetClassName(char[] buffer, int length = 64)
	{
		GetEntityClassname(this, buffer, length);
	}
	public void SetClassName(char[] buffer)
	{
		SetEntPropString(this, Prop_Data, "m_iClassname", buffer);
	}

	property bool OnGround
	{
		public get()
		{
			return (!(this.Buttons & IN_JUMP) && (GetEntityFlags(this) & FL_ONGROUND));
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
			return _StrafeHack[this];
		}
		public set(bool value)
		{
			_StrafeHack[this] = value;
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

	property int Method
	{
		public get()
		{
			return _Method[this];
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

			if(value != _Method[this])
			{
				if(value == Method.Server)
				{
					Server.HostTimescale.ReplicateToClient(this, "1");
				}
				else
				{
					Shavit_SetClientTimescale(this, _TimeScale[this]);
					this.LaggedMovementValue = 1.0;
					if(value == Method.Client || value == Method.ServerClient)
					{
						string sValue;
						sValue.FromFloat(_TimeScale[this]);
						Server.HostTimescale.ReplicateToClient(this, sValue.StringValue);
					}
				}
				_Method[this] = value;
			}
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
			if(this.Method == Method.Client || this.Method == Method.ServerClient)
			{
				string sValue;
				sValue.FromFloat(_TimeScale[this]);
				Server.HostTimescale.ReplicateToClient(this, sValue.StringValue);
			}
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
		menu.Display(this, MENU_TIME_FOREVER);
	}

	public void ResetVariables()
	{
		this.TimeScale = 1.0;
		this.Enabled = false;
		_StrafeHack[this] = true;
		this.FastWalk = true;
		this.Method = Method.ServerClient;
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
		client.TimeScale = 1.0;
		string_8 convar;
		convar.FromInt(Server.GetDefaultCheats());
		Server.Cheats.ReplicateToClient(client.Index, convar.StringValue);
		Server.HostTimescale.ReplicateToClient(client.Index, "1");
	}
}

enum struct TASFrame
{
	int index; // current index of that frame to put them in

	float time;
	int jumps;
	int strafes;
	int totalmeasures;
	int gains;

	float origin[3];
	float angles[3];
	float velocity[3];
	float basevelocity[3];
	// MoveType movetype;
	char movetype;
	float gravity;
	float speed;
	float stamina;
	// bool ducked;
	// bool ducking;
	char ducked;
	char ducking;
	float ducktime; // m_flDuckAmount in csgo
	float duckspeed; // m_flDuckSpeed in csgo; doesn't exist in css
	int flags;
	char targetname[64];
	// int targetname;
	char classname[64];
	// int classname;
	int groundentity;

	void Update(Client client)
	{
		client.GetAbsOrigin(this.origin);
		client.GetEyeAngles(this.angles);
		client.GetVelocity(this.velocity);
		client.GetBaseVelocity(this.basevelocity);
		client.GetTargetName(this.targetname);
		client.GetClassName(this.classname);

		this.movetype = view_as<char>(client.Movetype);
		this.gravity = client.Gravity;
		this.speed = client.LaggedMovementValue;

		this.stamina = client.Stamina;
		this.groundentity = client.GroundEntity;
		
		this.flags = client.Flags;

		this.ducked = view_as<char>(client.Ducked);
		this.ducking = view_as<char>(client.Ducking);
		
		this.ducktime = client.DuckTime;
		if(Server.IsCSGO)
		{
			this.duckspeed = client.DuckSpeed;
		}
		

		timer_snapshot_t snapshot;

		Shavit_SaveSnapshot(client.Index, snapshot);

		this.time = snapshot.fCurrentTime;
		this.jumps = snapshot.iJumps;
		this.strafes = snapshot.iStrafes;
		this.totalmeasures = snapshot.iTotalMeasures;
		this.gains = snapshot.iGoodGains;
	}

	void Set(Client client)
	{
		if(IsNullVector(this.origin))
		{
			return;
		}

		if(!client.IsAlive)
		{
			// Shavit_PrintToChat(client.Index, "%T", "CommandAlive", client, gS_ChatStrings.sVariable, gS_ChatStrings.sText);

			return;
		}

		if(Shavit_InsideZone(client.Index, Zone_Start, -1))
		{
			Shavit_StopTimer(client.Index);
		}

		MoveType mt = this.movetype;

		if(mt == MOVETYPE_LADDER || mt == MOVETYPE_WALK)
		{
			client.Movetype = mt;
		}

		client.Flags = this.flags;
		client.LaggedMovementValue = this.speed;
		client.GroundEntity = this.groundentity;
		client.Stamina = this.stamina;
		client.Ducked = !!this.ducked;
		client.Ducking = !!this.ducking;
		client.DuckTime = this.ducktime;

		if(Server.IsCSGO)
		{
			client.DuckSpeed = this.duckspeed;
		}

		timer_snapshot_t snapshot;
		snapshot.bTimerEnabled = true;
		snapshot.fCurrentTime = this.time;
		snapshot.bClientPaused = false;
		snapshot.iJumps = this.jumps;
		snapshot.bsStyle = client.Style;
		snapshot.iTotalMeasures = this.totalmeasures;
		snapshot.iGoodGains = this.gains;
		snapshot.iSHSWCombination = 0;
		snapshot.iTimerTrack = client.Track;
		snapshot.iMeasuredJumps = snapshot.iJumps;
		snapshot.iPerfectJumps = snapshot.iJumps;

		Shavit_LoadSnapshot(client.Index, snapshot);
		Shavit_ResumeTimer(client.Index);

		float vel[3];
		AddVectors(vel, this.basevelocity, this.velocity);

		client.SetTargetName(this.targetname);
		client.SetClassName(this.classname);
		
		client.Teleport(this.origin, this.angles, vel);

		client.Gravity = this.gravity;
		
		// Shavit_PrintToChat(client, "%T", "MiscCheckpointsTeleported", client, index, gS_ChatStrings.sVariable, gS_ChatStrings.sText);
		
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