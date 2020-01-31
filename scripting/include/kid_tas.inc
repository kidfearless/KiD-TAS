#if defined _kid_tas_included
	#endinput
#endif
#define _kid_tas_included

#define PERFSPEEDHACK 227.6

// private members
static bool _Enabled[MAXPLAYERS+1];
static float _LastGain[MAXPLAYERS+1];
static float _TimeScale[MAXPLAYERS+1] = {1.0, ...};
static bool _AutoStrafe[MAXPLAYERS+1] = {true, ...};
static bool _StrafeHack[MAXPLAYERS+1] = {true, ...};
static bool _FastWalk[MAXPLAYERS+1] = {true, ...};
static bool _AutoJump[MAXPLAYERS+1] = {true, ...};
static bool _ForceJump[MAXPLAYERS+1];
static bool _FastWalkDirection[MAXPLAYERS+1];
static int _Buttons[MAXPLAYERS+1];
static float _NextFrame[MAXPLAYERS+1];

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
		public set(bool newVal)
		{
			if(_Enabled[this.Index] != newVal)
			{
				_Enabled[this.Index] = newVal;
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
		public set(bool newVal)
		{
			_ForceJump[this.Index] = newVal;
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

	property float NextFrameTime
	{
		public get()
		{
			return _NextFrame[this.Index];
		}
		public set(float newVal)
		{
			_NextFrame[this.Index] = newVal;
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

	property float TimeScale
	{
		public get()
		{
			return _TimeScale[this.Index];
		}
		public set(float newVal)
		{
			if(!this.Enabled)
			{
				return;
			}
			
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
			
			
			Shavit_SetClientTimescale(this.Index, _TimeScale[this.Index]);			
		}
	}

	public void PrintToConsole(const char[] format, any ...)
	{
		string buffer;
		VFormat(buffer.StringValue, buffer.Size(), format, 3);
		PrintToConsole(this.Index, buffer.StringValue);
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


		menu.AddItem("as", (this.AutoStrafe ? "[X] Auto-Strafe" : "[ ] Auto-Strafe"));

		menu.AddItem("sh", (this.StrafeHack ? "[X] Strafe hack" : "[ ] Strafe Hack"));

		menu.Pagination = MENU_NO_PAGINATION;
		menu.ExitButton = true;
		menu.Display(this.Index, MENU_TIME_FOREVER);
	}

	public void ResetVariables()
	{
		this.TimeScale = 1.0;
		_LastGain[this.Index] = 0.0;
		this.Enabled = false;
		this.StrafeHack = true;
		this.AutoStrafe = true;
		this.FastWalk = true;
	}

	public void DoAutoStrafe(float vel[3], float perfAngleDiff)
	{
		// if autostrafe and either not on the ground or on the ground and holding jump
		if(this.AutoStrafe && !this.OnGround)
		{
			vel[1] = 450.0;
	
			if (perfAngleDiff > 0.0)
			{
				vel[1] = -450.0;
			}	
		}
	}

	public void DoStrafeHack(float vel[3], float angles[3], float perfAngleDiff)
	{
		if(this.OnGround)
		{
			return;
		}

		// this.PrintToConsole("perf %f", perfAngleDiff);
		//if (!((lastGain < 0.0 && flAngleGain < 0.0) || (lastGain > 0.0 && flAngleGain > 0.0))) 
		{
			angles[1] -= perfAngleDiff;
		}
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
		if(this.ForceJump && this.OnGround)
		{
			buttons |= IN_JUMP;
			this.ForceJump = false;
		}
	}

	public MRESReturn OnProcessMovement()
	{
		if(!this.Enabled)
		{
			return MRES_Ignored;
		}
		if(this.NextFrameTime <= 0.0)
		{
			this.NextFrameTime += (1.0 - this.TimeScale);
			return MRES_Ignored;
		}
		else
		{
			this.NextFrameTime -= this.TimeScale;
			return MRES_Supercede;
		}
	}

	public void OnPreThinkPost()
	{
		if(this.Enabled)
		{
		}
		else
		{
		}
	}

	public Action OnTick(int& buttons, float vel[3], float angles[3], int mouse[2])
	{
		this.Buttons = buttons;

		if(!this.IsAlive || !this.Enabled)
		{
			return Plugin_Continue;
		}

		float flVelocity[3];
		this.GetVelocity(flVelocity);
	
		float perfAngleChange = RadToDeg(ArcTangent2(flVelocity[1], flVelocity[0]));
		float perfAngleDiff = NormalizeAngle(angles[1] - perfAngleChange);

		

		if(this.AutoStrafe)
		{
			this.DoAutoStrafe(vel, perfAngleDiff);
		}
		if(this.StrafeHack)
		{
			this.DoStrafeHack(vel, angles, perfAngleDiff);
		}
		if(this.FastWalk)
		{
			this.DoFastWalk(vel);
		}
		if(this.AutoJump)
		{
			this.DoAutoJump(buttons);
		}
		
		return Plugin_Continue;
	}

	public void OnPostThink()
	{
	}

	public void OnPutInServer()
	{
		SDKHook(this.Index, SDKHook_PreThinkPost, OnPreThinkPost);
		SDKHook(this.Index, SDKHook_PostThinkPost, OnPostThink);
		this.ResetVariables();
	}

	public void OnLeaveStartZone()
	{
		this.ForceJump = true;
	}
}

void ToggleTAS(Client client)
{
	if(client.Enabled)
	{
		// Keeping this as it's TAS, should honestly have access to everything available
		sv_cheats.ReplicateToClient(client.Index, "2");
		client.OpenMenu();
	}
	else
	{
		string convar;
		convar.FromConVar(g_cDefaultCheats);
		sv_cheats.ReplicateToClient(client.Index, convar.StringValue);
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