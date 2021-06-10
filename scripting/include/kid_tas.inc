#if defined _kid_tas_included
	#endinput
#endif
#define _kid_tas_included

#define PERFSPEEDHACK 227.6
#include <sourcemod>
#include <sdkhooks>
#include <shavit>
#include <dhooks>

#include <strings_struct>
#include <convar_class>
#include <thelpers/thelpers>
#include <xutaxstrafe>
/* #include <intmap-kid>

static stock IntMap s_MenuMap;


void OnPluginStarted()
{
	s_MenuMap = new IntMap();
} */

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
static ArrayList _Frames[MAXPLAYERS+1];
static int _UseCheckpoints[MAXPLAYERS+1] = {-1, ...};
// static stock IntMap _IntMapFrames[MAXPLAYERS+1];
static int _CurrentCheckpoint[MAXPLAYERS+1];
static int _TotalCheckpoints[MAXPLAYERS+1];

enum Client
{
	INVALID_CLIENT = -1,
	SERVER_CLIENT = 0,
	CLIENT_SIZE = MAXPLAYERS
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
		return GetEntProp( this.Index, propType, propName, size, element );
	}

	public int GetPropEnt( PropType propType, const char[] propName, int element = 0 )
	{
		return GetEntPropEnt( this.Index, propType, propName, element );
	}

	public void PrintToConsole(const char[] format, any ...)
	{
		string buffer;
		VFormat(buffer.Value, buffer.Size(), format, 3);
		PrintToConsole(this.Index, buffer.Value);
	}

	public void PrintToChat(const char[] format, any ...)
	{
		string buffer;
		VFormat(buffer.Value, buffer.Size(), format, 3);
		PrintToChat(this.Index, buffer.Value);
	}

	public void ReplyToCommand(const char[] format, any ...)
	{
		string buffer;
		VFormat(buffer.Value, buffer.Size(), format, 3);
		ReplyToCommand(this.Index, buffer.Value);
	}

	public void Teleport(const float origin[3] = NULL_VECTOR, const float angles[3] = NULL_VECTOR, const float velocity[3] = NULL_VECTOR)
	{
		TeleportEntity(this.Index, origin, angles, velocity);
	}

	public void GetVelocity(float vecvelocity[3])
	{
		GetEntPropVector(this.Index, Prop_Data, "m_vecVelocity", vecvelocity);
	}

	public void GetBaseVelocity(float vec[3])
	{
		GetEntPropVector(this.Index, Prop_Data, "m_vecBaseVelocity", vec);
	}

	public void GetTargetName(char[] buffer, int length = 64)
	{
		GetEntPropString(this.Index, Prop_Data, "m_iName", buffer, length);
	}

	public void SetTargetName(char[] buffer)
	{
		SetEntPropString(this.Index, Prop_Data, "m_iName", buffer);
	}

	public void GetClassName(char[] buffer, int length = 64)
	{
		GetEntityClassname(this.Index, buffer, length);
	}

	public void SetClassName(char[] buffer)
	{
		SetEntPropString(this.Index, Prop_Data, "m_iClassname", buffer);
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
			_Enabled[this.Index] = value;
			ToggleTAS(this);
		}
	}

	property bool Ducked
	{
		public get()
		{
			return !!GetEntProp(this.Index, Prop_Send, "m_bDucked");
		}
		public set(bool value)
		{
			SetEntProp(this.Index, Prop_Send, "m_bDucked", value);
		}
	}

	property bool Ducking
	{
		public get()
		{
			return !!GetEntProp(this.Index, Prop_Send, "m_bDucking");
		}
		public set(bool value)
		{
			SetEntProp(this.Index, Prop_Send, "m_bDucking", value);
		}
	}

	property float Stamina
	{
		public get()
		{
			return GetEntPropFloat(this.Index, Prop_Send, "m_flStamina");
		}
		public set(float value)
		{
			SetEntProp(this.Index, Prop_Send, "m_flStamina", value);
		}
	}

	property float DuckSpeed
	{
		public get()
		{
			return GetEntPropFloat(this.Index, Prop_Send, "m_flDuckSpeed");
		}
		public set(float value)
		{
			SetEntProp(this.Index, Prop_Send, "m_flDuckSpeed", value);
		}
	}

	property float DuckTime
	{
		public get()
		{
			if(Server.IsCSS)
			{
				GetEntPropFloat(this.Index, Prop_Send, "m_flDucktime");
			}
			else
			{
				GetEntPropFloat(this.Index, Prop_Send, "m_flDuckAmount");
			}
		}
		public set(float value)
		{
			if(Server.IsCSS)
			{
				SetEntPropFloat(this.Index, Prop_Send, "m_flDucktime", value);
			}
			else
			{
				SetEntPropFloat(this.Index, Prop_Send, "m_flDuckAmount", value);
			}
		}
	}

	property int GroundEntity
	{
		public get()
		{
			return GetEntPropEnt(this.Index, Prop_Send, "m_hGroundEntity");
		}
		public set(int value)
		{
			SetEntPropEnt(this.Index, Prop_Data, "m_hGroundEntity", value);
		}
	}

	property int Flags
	{
		public get()
		{
			return GetEntityFlags(this.Index);
		}
		public set(int value)
		{
			SetEntityFlags(this.Index, value);
		}
	}

	property ArrayList Frames
	{
		public get()
		{
			return _Frames[this.Index];
		}
		public set(ArrayList value)
		{
			_Frames[this.Index] = value;
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
			return GetXutaxStrafe(this.Index);
		}
		public set(bool value)
		{
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

	property float TimeScale
	{
		public get()
		{
			return _TimeScale[this.Index];
		}
		public set(float value)
		{
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

			SetXutaxType(this.Index, value);
			_XutaxType[this.Index] = value;
		}
	}

	property bool UseCheckpoints
	{
		public get()
		{
			return _UseCheckpoints[this.Index] == 1;
		}
		public set(bool value)
		{
			_UseCheckpoints[this.Index] = value;
		}
	}

	property float Power
	{
		public get()
		{
			return GetXutaxPower(this.Index);
		}
		public set(float value)
		{
			if(value >= 0.0 && value <= 2.0)
			{
				SetXutaxPower(this.Index, value);
			}
		}
	}

	property int CurrentCheckpoint
	{
		public get()
		{
			if(this.UseCheckpoints)
			{
				return Shavit_GetCurrentCheckpoint(this.Index);
			}
			else
			{
				return _CurrentCheckpoint[this.Index];
			}
		}
		public set(int value)
		{
			if(this.UseCheckpoints)
			{
				Shavit_SetCurrentCheckpoint(this.Index, value);
			}
			else
			{
				_CurrentCheckpoint[this.Index] = value;
			}
		}
	}

	property int TotalCheckpoints
	{
		public get()
		{
			if(this.UseCheckpoints)
			{
				return Shavit_GetTotalCheckpoints(this.Index);
			}
			else
			{
				return _TotalCheckpoints[this.Index];
			}
		}
		public set(int value)
		{
			if(!this.UseCheckpoints)
			{
				_TotalCheckpoints[this.Index] = value;
			}
		}
	}

	public void SaveCheckpoint()
	{
		if(this.UseCheckpoints)
		{
			this.CurrentCheckpoint = Shavit_SaveCheckpoint(this.Index);
		}
	}

	public void TeleportToCheckpoint()
	{
		if(this.UseCheckpoints)
		{
			float timescale = this.TimeScale;
			Shavit_TeleportToCheckpoint(this.Index, this.CurrentCheckpoint);
			this.TimeScale = timescale;
		}
	}

	public void OpenDecisionMenu()
	{
		Menu menu = new Menu(MenuHandler_TASDecision);

		string_64 display;

		// Title
		display.Format("%T\n ", "Pick Your Type", this.Index);
		menu.SetTitle(display.Value);

		// Rewind TAS
		display.Format("%T ", "Rewind TAS", this.Index);
		menu.AddItem("0", display.Value);

		// Checkpoint TAS
		display.Format("%T ", "Checkpoint TAS", this.Index);
		menu.AddItem("1", display.Value);

		menu.Pagination = MENU_NO_PAGINATION;
		menu.ExitButton = true;
		menu.Display(this.Index, MENU_TIME_FOREVER);
	}

	public void OpenCheckPointMenu()
	{
		Panel panel = new Panel();
		string_64 display;

		// title
		display.Format("%T\n ", "TAS Menu", this.Index);
		panel.SetTitle(display.Value);

		// save
		display.Format("%T", "MiscCheckpointSave", this.Index, this.TotalCheckpoints + 1);
		panel.DrawItem(display.Value);

		// teleport
		if(this.TotalCheckpoints > 0)
		{
			display.Format("%T", "MiscCheckpointTeleport", this.Index, this.CurrentCheckpoint);
			panel.DrawItem(display.Value);
		}
		else
		{
			display.Format("%T", "MiscCheckpointTeleport", this.Index, 1);
			panel.DrawItem(display.Value, ITEMDRAW_DISABLED);
		}

		// previous
		display.Format("%T", "MiscCheckpointPrevious", this.Index);
		panel.DrawItem(display.Value);
		// next
		display.Format("%T", "MiscCheckpointNext", this.Index);
		panel.DrawItem(display.Value);

		panel.DrawItem(" ", ITEMDRAW_RAWLINE);


		// --Timescale
		display.Format("--%T", "Timescale", this.Index);
		panel.DrawItem(display.Value);

		// Current Timescale
		display.Format("%T", "Current Timescale", this.Index, this.TimeScale + 0.001);
		panel.DrawItem(display.Value, ITEMDRAW_RAWLINE);

		// ++Timescale
		display.Format("++%T", "Timescale", this.Index);
		panel.DrawItem(display.Value);

		panel.DrawItem(" ", ITEMDRAW_RAWLINE);

		// Strafehack Settings
		display.Format("%T", "Strafehack Settings", this.Index);
		panel.DrawItem(display.Value);

		// Autojump
		display.Format("%T", "Autojump", this.Index, (this.AutoJump ? "X" : " "));
		panel.DrawItem(display.Value);

		panel.DrawItem(" ", ITEMDRAW_RAWLINE);

		// Exit
		display.Format("%T", "Exit", this.Index);
		panel.DrawItem(display.Value);

		panel.Send(this.Index, MenuHandler_TASCheckPoint, MENU_TIME_FOREVER);
		delete panel;
	}

	public void OpenRewindMenu()
	{

	}

	public void OpenMenu()
	{
		if(!this.Enabled)
		{
			return;
		}

		// temp setup to hide desiscion menu
		this.UseCheckpoints = true;
		switch(_UseCheckpoints[this.Index])
		{
			case 1:
			{
				this.OpenCheckPointMenu();
			}
			case 0:
			{
				this.OpenRewindMenu();
			}
			default:
			{
				this.OpenDecisionMenu();
			}
		}
	}

	public void OpenStrafeHackMenu()
	{
		Panel panel = new Panel();

		string_64 display;

		// Title
		display.Format("%T\n ", "Strafehack Settings", this.Index);
		panel.SetTitle(display.Value);

		// Strafe Hack
		display.Format("%T\n ", "Strafe hack", this.Index, this.StrafeHack ? "X" : " ");
		panel.DrawItem(display.Value);

		// --Power
		display.Format("--%T", "Power", this.Index);
		panel.DrawItem(display.Value);

		display.Format("%T", "Current Power", this.Index, (this.Power * 100.0) + 0.01);
		panel.DrawItem(display.Value, ITEMDRAW_RAWLINE);

		// ++Power
		display.Format("++%T", "Power", this.Index);
		panel.DrawItem(display.Value);

		// Type
		if(this.Type == XutaxType.Normal)
		{
			display.Format("%T", "Type Normal", this.Index);
		}
		else if(this.Type == XutaxType.Surf)
		{
			display.Format("%T", "Type Surf", this.Index);
		}
		else if(this.Type == XutaxType.Manual)
		{
			display.Format("%T", "Type Manual", this.Index);
		}

		panel.DrawItem(display.Value);

		// Back
		panel.CurrentKey = 7;
		display.Format("%T", "Back", this.Index);
		panel.DrawItem(display.Value);

		// Exit
		panel.CurrentKey = Server.IsCSGO ? 9 : 10;
		display.Format("%T", "Exit", this.Index);
		panel.DrawItem(display.Value);

		panel.Send(this.Index, MenuHandler_StrafehackSettings, MENU_TIME_FOREVER);
	}

	public void ResetVariables()
	{
		if(this.IsFakeClient)
		{
			return;
		}

		_TimeScale[this.Index] = 1.0;
		this.Enabled = false;
		this.FastWalk = true;
		this.Type = XutaxType.Surf;
		_UseCheckpoints[this.Index] = -1;
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
		// if(this.ForceJump)
		// {
		// 	if(this.OnGround)
		// 	{
		// 		buttons |= IN_JUMP;
		// 		this.ForceJump = false;
		// 	}
		// 	else
		// 	{
		// 		this.ForceJump = false;
		// 	}
		// }
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
	if(!client.IsValid())
	{
		return;
	}
	if(client.Enabled)
	{
		if(g_cSetCheats.BoolValue)
		{
			// Keeping this as it's TAS, should honestly have access to everything available
			Server.Cheats.ReplicateToClient(client.Index, "2");
		}
		_UseCheckpoints[client] = -1;
		RequestFrame(Frame_OpenMenu, client.Serial);
	}
	else
	{
		client.TimeScale = 1.0;
		if(g_cSetCheats.BoolValue)
		{
			string_8 convar;
			convar.FromInt(Server.GetDefaultCheats());
			Server.Cheats.ReplicateToClient(client.Index, convar.Value);
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

public int MenuHandler_TASCheckPoint(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			Client client = new Client(param1);
			if(!client.Enabled)
			{
				return 0;
			}

			EmitSoundToClient(client.Index, "buttons/button14.wav");
			switch(param2)
			{
				case 1: // SAVE CHECKPOINT
				{
					client.SaveCheckpoint();
				}
				case 2: // TELEPORT
				{
					client.TeleportToCheckpoint();
				}
				case 3: // PREV
				{
					--client.CurrentCheckpoint;
				}
				case 4: // NEXT
				{
					++client.CurrentCheckpoint;
				}
				case 5: // --TIMESCALE
				{
					client.TimeScale -= 0.1;
				}
				case 6: // ++TIMESCALE
				{
					client.TimeScale += 0.1;
				}
				case 7: // STRAFEHACK SETTINGS
				{
					client.OpenStrafeHackMenu();
					return 0;
				}
				case 8: // AUTOJUMP
				{
					client.AutoJump = !client.AutoJump;
				}
				case 9:
				{
					EmitSoundToClient(client.Index, "buttons/combine_button7.wav");
					return 0;
				}
			}


			client.OpenMenu();
		}

		case MenuAction_End:
		{
			delete menu;
		}
	}

	return 0;
}

public int MenuHandler_TASDecision(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			Client client = new Client(param1);
			if(!client.Enabled)
			{
				return 0;
			}

			_UseCheckpoints[client] = param2;

			client.OpenMenu();
		}

		case MenuAction_End:
		{
			delete menu;
		}
	}

	return 0;
}

public int MenuHandler_StrafehackSettings(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			Client client = new Client(param1);
			if(!client.Enabled)
			{
				return 0;
			}

			EmitSoundToClient(client.Index, "buttons/button14.wav");
			switch(param2)
			{
				case 1: // strafehack enabled
				{
					client.StrafeHack = !client.StrafeHack;
				}
				case 2: // --power
				{
					client.Power -= 0.1;
				}
				case 3: // ++power
				{
					client.Power += 0.1;
				}
				case 4: // switch type
				{
					++client.Type;
				}
				case 7:
				{
					client.OpenMenu();
					return 0;
				}
				default:
				{
					EmitSoundToClient(client.Index, "buttons/combine_button7.wav");
					return 0;
				}
			}

			client.OpenStrafeHackMenu();
		}

		case MenuAction_End:
		{
			delete menu;
		}
	}

	return 0;
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
	MoveType movetype;
	float gravity;
	float speed;
	float stamina;
	bool ducked;
	bool ducking;
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

		this.movetype = client.Movetype;
		this.gravity = client.Gravity;
		this.speed = client.LaggedMovementValue;

		this.stamina = client.Stamina;
		this.groundentity = client.GroundEntity;

		this.flags = client.Flags;

		this.ducked = client.Ducked;
		this.ducking = client.Ducking;

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
			// Shavit_PrintToChat(client.Index, "%T", "CommandAlive", client.Index, gS_ChatStrings.sVariable, gS_ChatStrings.sText);

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
		client.Ducked = this.ducked;
		client.Ducking = this.ducking;
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
	}
}

#define DEBUG 0
#if DEBUG == 1
stock void DebugPrint(const char[] format, any args ...)
{
	string buffer;
	VFormat(buffer.Value, buffer.Size(), format, 2);
	PrintToChatAll(buffer.Value);
}
#endif

