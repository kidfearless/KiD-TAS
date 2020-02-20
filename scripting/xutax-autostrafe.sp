#include <sourcemod>
#include <xutaxstrafe>
#include <shavit>

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