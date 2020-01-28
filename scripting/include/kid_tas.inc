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
static bool _ShouldJump[MAXPLAYERS+1];
static bool _FastWalkDirection[MAXPLAYERS+1];
static int _Buttons[MAXPLAYERS+1];

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
				ToggleTAS(this, newVal);
			}
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
		// Check whether the player has tried to move their mouse more than the strafer
		float flAngleGain = RadToDeg(ArcTangent(vel[1] / vel[0]));
		float lastGain = _LastGain[this.Index];
		// This check tells you when the mouse player movement is higher than the autostrafer one, and decide to put it or not
		if (!((lastGain < 0.0 && flAngleGain < 0.0) || (lastGain > 0.0 && flAngleGain > 0.0))) 
		{
			if(this.StrafeHack && !this.OnGround)
			{
				angles[1] -= perfAngleDiff;
			}
		}
		
		_LastGain[this.Index] = flAngleGain;
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
		if(_ShouldJump[this.Index] && this.OnGround)
		{
			buttons |= IN_JUMP;
			_ShouldJump[this.Index] = false;
		}
	}

	public void OnTick(int& buttons, float vel[3], float angles[3], int mouse[2])
	{
		this.Buttons = buttons;
		if(!this.IsAlive || !this.Enabled)
		{
			return;
		}
		
		float vecvelocity[3];
		GetEntPropVector(this.Index, Prop_Data, "m_vecVelocity", vecvelocity);
		float perfAngleChange = RadToDeg(ArcTangent2(vecvelocity[1], vecvelocity[0]));
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

		// might investigate this again
		// SetEntProp(this.Index, Prop_Data, "m_nSimulationTick", GetGameTickCount());
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

	public void OnLeaveStartZone()
	{
		_ShouldJump[this.Index] = true;
	}
}

void ToggleTAS(Client client, bool newVal)
{
	client.Enabled = newVal;
	if(client.Enabled)
	{
		sv_cheats.ReplicateToClient(client.Index, "2");
		PrintToChat(client.Index, "For a better experience change the following convars:");
		PrintToChat(client.Index, "cl_clock_correction 0");
		PrintToChat(client.Index, "cl_clock_correction_force_server_tick 0");
		PrintToConsole(client.Index, "cl_clock_correction_force_server_tick 0;cl_clock_correction 0;");
		client.OpenMenu();
	}
	else
	{
		string convar;
		convar.FromConVar(g_cDefaultCheats);
		sv_cheats.ReplicateToClient(client.Index, convar.StringValue);
		host_timescale.ReplicateToClient(client.Index, "1");
	}
}