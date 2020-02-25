#include <sourcemod>
#include <xutaxstrafe>
#include <shavit>

public void OnPluginStart()
{
	RegConsoleCmd("sm_xutaxtype", Command_StrafeType, "Set's the strafehack override type");
	RegConsoleCmd("sm_strafetype", Command_StrafeType, "Set's the strafehack override type");
}

public Action Command_StrafeType(int client, int args)
{
	if(args < 1)
	{
		ReplyToCommand(client, "sm_strafetype <0:Normal, 1:Surf, 2: Manual>");
		return Plugin_Handled;
	}

	char arg[3];
	GetCmdArg(1, arg, 3);

	switch(arg[0])
	{
		case '0':
		{
			SetXutaxType(client, 0);
		}
		case '1':
		{
			SetXutaxType(client, 1);
			
		}
		case '2':
		{
			SetXutaxType(client, 2);
		}
		default:
		{
			ReplyToCommand(client, "invalid type specified");
		}
	}

	return Plugin_Handled;
}

public void OnClientConnected(int client)
{
	SetXutaxStrafe(client, false);
}

public void Shavit_OnStyleChanged(int client, int oldstyle, int newstyle, int track, bool manual)
{
	char special[128];
	Shavit_GetStyleStrings(newstyle, sSpecialString, special, 128);
	if(StrContains(special, "xutax") != -1)
	{
		SetXutaxStrafe(client, true);
	}
	else if(StrContains(special, "TAS") != -1)
	{
		SetXutaxStrafe(client, true);
	}
	else
	{
		SetXutaxStrafe(client, false);
	}
}