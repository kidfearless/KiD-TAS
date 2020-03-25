#pragma semicolon 1

#define DEBUG

#include <sourcemod>
#include <sdktools>
#include <cstrike>
#include <xutaxstrafe>
#include <kid_tas_api>
#include <convar_class>

#pragma newdecls required

float g_flAirSpeedCap = 30.0;
float g_flOldYawAngle[MAXPLAYERS + 1];
ConVar g_ConVar_sv_airaccelerate;
int g_iSurfaceFrictionOffset;
float g_fMaxMove = 400.0;
EngineVersion g_Game;
bool g_bEnabled[MAXPLAYERS + 1];
int g_iType[MAXPLAYERS + 1];
float g_fPower[MAXPLAYERS + 1] = {1.0, ...};
bool g_bTASEnabled;

Convar g_ConVar_AutoFind_Offset;


public Plugin myinfo =
{
	name = "Perfect autostrafe",
	author = "xutaxkamay",
	description = "",
	version = "1.1",
	url = "https://steamcommunity.com/id/xutaxkamay/"
};

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	CreateNative("SetXutaxStrafe", Native_SetAutostrafe);
	CreateNative("GetXutaxStrafe", Native_GetAutostrafe);
	CreateNative("SetXutaxType", Native_SetType);
	CreateNative("GetXutaxType", Native_GetType);
	CreateNative("SetXutaxPower", Native_SetPower);
	CreateNative("GetXutaxPower", Native_GetPower);

	RegPluginLibrary("xutax-strafe");
	return APLRes_Success;
}

public void OnLibraryAdded(const char[] name)
{
	if(StrEqual(name, "kid-tas"))
	{
		g_bTASEnabled = true;
	}
}

public void OnLibraryRemoved(const char[] name)
{
	if(StrEqual(name, "kid-tas"))
	{
		g_bTASEnabled = false;
	}
}

public void OnPluginStart()
{
	g_Game = GetEngineVersion();
	g_ConVar_sv_airaccelerate = FindConVar("sv_airaccelerate");

	GameData gamedata = new GameData("KiD-TAS.games");

	g_iSurfaceFrictionOffset = gamedata.GetOffset("m_surfaceFriction");
	delete gamedata;

	if(g_iSurfaceFrictionOffset <= 0)
	{
		LogError("[XUTAX] Invalid offset supplied, defaulting friction values");
	}
	if(g_Game == Engine_CSGO)
	{
		// g_cFrictionOffset = new Convar("xutax_friction_offset", "4680", "Surface friction offset");
		g_fMaxMove = 450.0;
		ConVar sv_air_max_wishspeed = FindConVar("sv_air_max_wishspeed");
		sv_air_max_wishspeed.AddChangeHook(OnWishSpeedChanged);
		g_flAirSpeedCap = sv_air_max_wishspeed.FloatValue;
	}
	else if(g_Game == Engine_CSS)
	{
		// saved for later use
	}
	else
	{
		SetFailState("This plugin is for CSGO/CSS only.");
	}

	g_ConVar_AutoFind_Offset = new Convar("xutax_find_offsets", "1", "Attempt to autofind offsets", _, true, 0.0, true, 1.0);

	Convar.AutoExecConfig();
}

// doesn't exist in css so we have to cache the value
public void OnWishSpeedChanged(ConVar convar, const char[] oldValue, const char[] newValue)
{
	g_flAirSpeedCap = StringToFloat(newValue);
}

public void OnClientConnected(int client)
{
	g_bEnabled[client] = false;
	g_iType[client] = Type_SurfOverride;
	g_fPower[client] = 1.0;
}

float AngleNormalize(float flAngle)
{
	if (flAngle > 180.0)
		flAngle -= 360.0;
	else if (flAngle < -180.0)
		flAngle += 360.0;

	return flAngle;
}

float Vec2DToYaw(float vec[2])
{
	float flYaw = 0.0;

	if (vec[0] != 0.0 || vec[1] != 0.0)
	{
		float vecNormalized[2];

		float flLength = SquareRoot(vec[0] * vec[0] + vec[1] * vec[1]);

		vecNormalized[0] = vec[0] / flLength;
		vecNormalized[1] = vec[1] / flLength;

		// Credits to Valve.
		flYaw = ArcTangent2(vecNormalized[1], vecNormalized[0]) * (180.0 / FLOAT_PI);

		flYaw = AngleNormalize(flYaw);
	}

	return flYaw;
}

/*
 * So our problem here is to find a wishdir that no matter the angles we choose, it should go to the direction we want.
 * So forward/right vector changing but not sidemove and forwardmove for the case where we modify our angles. (1)
 * But in our case we want sidemove and forwardmove values changing and not the forward/right vectors. (2)
 * So our unknown variables is fmove and smove to know the (2) case. But we know the (1) case so we can solve this into a linear equation.
 * To make it more simplier, we know the wishdir values and forward/right vectors, but we do not know the fowardmove and sidemove variables
 * and that's what we want to solve.
 * That's what is doing this function, but only in 2D since we can only move forward or side.
 * But, for noclip (3D) it's a different story that I will let you discover, same method, but 3 equations and 3 unknown variables (forwardmove, sidemove, upmove).
 */

void Solve2DMovementsVars(float vecWishDir[2], float vecForward[2], float vecRight[2], float &flForwardMove, float &flSideMove)
{
	// wishdir[0] = foward[0] * forwardmove + right[0] * sidemove;
	// wishdir[1] = foward[1] * forwardmove + right[1] * sidemove;

	// Let's translate this to letters.
	// v = a * b + c * d
	// w = e * b + f * d
	// v = wishdir[0]; w = wishdir[1]...

	// Now let's solve it with online solver https://quickmath.com/webMathematica3/quickmath/equations/solve/advanced.jsp
	// https://cdn.discordapp.com/attachments/609163806085742622/675477245178937385/c3ca4165c30b3b342e57b903a3ded367-3.png

	float v = vecWishDir[0];
	float w = vecWishDir[1];
	float a = vecForward[0];
	float c = vecRight[0];
	float e = vecForward[1];
	float f = vecRight[1];

	float flDivide = (c * e - a * f);
	if(flDivide == 0.0)
	{
		flForwardMove = g_fMaxMove;
		flSideMove = 0.0;
	}
	else
	{
		flForwardMove = (c * w - f * v) / flDivide;
		flSideMove = (e * v - a * w) / flDivide;
	}
}

float GetThetaAngleInAir(float flVelocity[2], float flAirAccelerate, float flMaxSpeed, float flSurfaceFriction, float flFrametime)
{
	// In order to solve this, we must check that accelspeed < 30
	// so it applies the correct strafing method.
	// So there is basically two cases:
	// if 30 - accelspeed <= 0 -> We use the perpendicular of velocity.
	// but if 30 - accelspeed > 0 the dot product must be equal to = 30 - accelspeed
	// in order to get the best gain.
	// First case is theta == 90
	// How to solve the second case?
	// here we go
	// d = velocity2DLength * cos(theta)
	// cos(theta) = d / velocity2D
	// theta = arcos(d / velocity2D)

	float flAccelSpeed = flAirAccelerate * flMaxSpeed * flSurfaceFriction * flFrametime;

	float flWantedDotProduct = g_flAirSpeedCap - flAccelSpeed;

	if (flWantedDotProduct > 0.0)
	{
		float flVelLength2D = SquareRoot(flVelocity[0] * flVelocity[0] + flVelocity[1] * flVelocity[1]);
		if(flVelLength2D == 0.0)
		{
			return 90.0;
		}
		float flCosTheta = flWantedDotProduct / flVelLength2D;

		if (flCosTheta > 1.0)
		{
			flCosTheta = 1.0;
		}
		else if(flCosTheta < -1.0)
		{
			flCosTheta = -1.0;
		}


		float flTheta = ArcCosine(flCosTheta) * (180.0 / FLOAT_PI);

		return flTheta;
	}
	else
	{
		return 90.0;
	}
}


// Same as above, but this time we calculate max delta angle
// so we can change between normal strafer and autostrafer depending on the player's viewangles difference.
/*float GetMaxDeltaInAir(float flVelocity[2], float flAirAccelerate, float flMaxSpeed, float flSurfaceFriction, float flFrametime)
{
	float flAccelSpeed = flAirAccelerate * flMaxSpeed * flSurfaceFriction * flFrametime;

	if (flAccelSpeed >= g_flAirSpeedCap)
	{
		flAccelSpeed = g_flAirSpeedCap;
	}

	float flVelLength2D = SquareRoot(flVelocity[0] * flVelocity[0] + flVelocity[1] * flVelocity[1]);

	float flMaxDelta = ArcTangent2(flAccelSpeed, flVelLength2D)  * (180 / FLOAT_PI);

	return flMaxDelta;
}*/

float SimulateAirAccelerate(float flVelocity[2], float flWishDir[2], float flAirAccelerate, float flMaxSpeed, float flSurfaceFriction, float flFrametime, float flVelocityOutput[2])
{
	float flWishSpeedCapped = flMaxSpeed;

	// Cap speed
	if( flWishSpeedCapped > g_flAirSpeedCap )
		flWishSpeedCapped = g_flAirSpeedCap;

	// Determine veer amount
	float flCurrentSpeed = flVelocity[0] * flWishDir[0] + flVelocity[1] * flWishDir[1];

	// See how much to add
	float flAddSpeed = flWishSpeedCapped - flCurrentSpeed;

	// If not adding any, done.
	if( flAddSpeed <= 0.0 )
	{
		return;
	}

	// Determine acceleration speed after acceleration
	float flAccelSpeed = flAirAccelerate * flMaxSpeed * flFrametime * flSurfaceFriction;

	// Cap it
	if( flAccelSpeed > flAddSpeed )
	{
		flAccelSpeed = flAddSpeed;
	}

	flVelocityOutput[0] = flVelocity[0] + flAccelSpeed * flWishDir[0];
	flVelocityOutput[1] = flVelocity[1] + flAccelSpeed * flWishDir[1];
}

// The idea is to get the maximum angle
float GetMaxDeltaInAir(float flVelocity[2], float flMaxSpeed, float flSurfaceFriction, bool bLeft)
{
	float flFrametime = GetTickInterval();
	float flAirAccelerate = g_ConVar_sv_airaccelerate.FloatValue;

	float flTheta = GetThetaAngleInAir(flVelocity, flAirAccelerate, flMaxSpeed, flSurfaceFriction, flFrametime);

	// Convert velocity 2D to angle.
	float flYawVelocity = Vec2DToYaw(flVelocity);

	// Get the best yaw direction on the right.
	float flBestYawRight = AngleNormalize(flYawVelocity + flTheta);

	// Get the best yaw direction on the left.
	float flBestYawLeft = AngleNormalize(flYawVelocity - flTheta);

	float flTemp[3], vecBestLeft3D[3], vecBestRight3D[3];

	flTemp[0] = 0.0;
	flTemp[1] = flBestYawLeft;
	flTemp[2] = 0.0;

	GetAngleVectors(flTemp, vecBestLeft3D, NULL_VECTOR, NULL_VECTOR);

	flTemp[0] = 0.0;
	flTemp[1] = flBestYawRight;
	flTemp[2] = 0.0;

	GetAngleVectors(flTemp, vecBestRight3D, NULL_VECTOR, NULL_VECTOR);

	float vecBestRight[2], vecBestLeft[2];

	vecBestRight[0] = vecBestRight3D[0];
	vecBestRight[1] = vecBestRight3D[1];

	vecBestLeft[0] = vecBestLeft3D[0];
	vecBestLeft[1] = vecBestLeft3D[1];

	float flCalcVelocityLeft[2], flCalcVelocityRight[2];

	// Simulate air accelerate function in order to get the new max gain possible on both side.
	SimulateAirAccelerate(flVelocity, vecBestLeft, flAirAccelerate, flMaxSpeed, flFrametime, flSurfaceFriction, flCalcVelocityLeft);
	SimulateAirAccelerate(flVelocity, vecBestRight, flAirAccelerate, flMaxSpeed, flFrametime, flSurfaceFriction, flCalcVelocityRight);

	float flNewBestYawLeft = Vec2DToYaw(flCalcVelocityLeft);
	float flNewBestYawRight = Vec2DToYaw(flCalcVelocityRight);

	// Then get the difference in order to find the maximum angle.
	if (bLeft)
	{
		return FloatAbs(AngleNormalize(flYawVelocity - flNewBestYawLeft));
	}
	else
	{
		return FloatAbs(AngleNormalize(flYawVelocity - flNewBestYawRight));
	}

	// Do an estimate otherwhise.
	// return FloatAbs(AngleNormalize(flNewBestYawLeft - flNewBestYawRight) / 2.0);
}

void GetIdealMovementsInAir(float flYawWantedDir, float flVelocity[2], float flMaxSpeed, float flSurfaceFriction, float &flForwardMove, float &flSideMove, bool bPreferRight = true)
{
	float flAirAccelerate = g_ConVar_sv_airaccelerate.FloatValue;
	float flFrametime = GetTickInterval();
	float flYawVelocity = Vec2DToYaw(flVelocity);

	// Get theta angle
	float flTheta = GetThetaAngleInAir(flVelocity, flAirAccelerate, flMaxSpeed, flSurfaceFriction, flFrametime);

	// Get the best yaw direction on the right.
	float flBestYawRight = AngleNormalize(flYawVelocity + flTheta);

	// Get the best yaw direction on the left.
	float flBestYawLeft = AngleNormalize(flYawVelocity - flTheta);

	float vecBestDirLeft[3], vecBestDirRight[3];
	float tempAngle[3];

	tempAngle[0] = 0.0;
	tempAngle[1] = flBestYawRight;
	tempAngle[2] = 0.0;

	GetAngleVectors(tempAngle, vecBestDirRight, NULL_VECTOR, NULL_VECTOR);

	tempAngle[0] = 0.0;
	tempAngle[1] = flBestYawLeft;
	tempAngle[2] = 0.0;

	GetAngleVectors(tempAngle, vecBestDirLeft, NULL_VECTOR, NULL_VECTOR);

	// Our wanted direction.
	float vecBestDir[2];

	// Let's follow the most the wanted direction now with max possible gain.
	float flDiffYaw = AngleNormalize(flYawWantedDir - flYawVelocity);

	if (flDiffYaw > 0.0)
	{
		vecBestDir[0] = vecBestDirRight[0];
		vecBestDir[1] = vecBestDirRight[1];
	}
	else if(flDiffYaw < 0.0)
	{
		vecBestDir[0] = vecBestDirLeft[0];
		vecBestDir[1] = vecBestDirLeft[1];
	}
	else
	{
		// Going straight.
		if (bPreferRight)
		{
			vecBestDir[0] = vecBestDirRight[0];
			vecBestDir[1] = vecBestDirRight[1];
		}
		else
		{
			vecBestDir[0] = vecBestDirLeft[0];
			vecBestDir[1] = vecBestDirLeft[1];
		}
	}

	float vecForwardWantedDir3D[3], vecRightWantedDir3D[3];
	float vecForwardWantedDir[2], vecRightWantedDir[2];

	tempAngle[0] = 0.0;
	tempAngle[1] = flYawWantedDir;
	tempAngle[2] = 0.0;

	// Convert our yaw wanted direction to vectors.
	GetAngleVectors(tempAngle, vecForwardWantedDir3D, vecRightWantedDir3D, NULL_VECTOR);

	vecForwardWantedDir[0] = vecForwardWantedDir3D[0];
	vecForwardWantedDir[1] = vecForwardWantedDir3D[1];

	vecRightWantedDir[0] = vecRightWantedDir3D[0];
	vecRightWantedDir[1] = vecRightWantedDir3D[1];

	// Solve the movement variables from our wanted direction and the best gain direction.
	Solve2DMovementsVars(vecBestDir, vecForwardWantedDir, vecRightWantedDir, flForwardMove, flSideMove);

	float flLengthMovements = SquareRoot(flForwardMove * flForwardMove + flSideMove * flSideMove);

	if(flLengthMovements != 0.0)
	{
		flForwardMove /= flLengthMovements;
		flSideMove /= flLengthMovements;
	}
}

public Action OnPlayerRunCmd(int client, int& buttons, int& impulse, float vel[3], float angles[3], int& weapon, int& subtype, int& cmdnum, int& tickcount, int& seed, int mouse[2])
{
	if(!g_bEnabled[client])
	{
		return Plugin_Continue;
	}

	if(!ShouldProcessFrame(client))
	{
		return Plugin_Continue;
	}

	bool bOnGround = !(buttons & IN_JUMP) && (GetEntPropEnt(client, Prop_Send, "m_hGroundEntity") != -1);

	if (IsPlayerAlive(client)
		&& !bOnGround
		&& !(GetEntityMoveType(client) & MOVETYPE_LADDER)
		&& (GetEntProp(client, Prop_Data, "m_nWaterLevel") <= 1))
	{
		if(!!(buttons & (IN_FORWARD | IN_BACK)))
		{
			return Plugin_Continue;
		}

		if(!!(buttons & (IN_MOVERIGHT | IN_MOVELEFT)))
		{
			if(g_iType[client] == Type_Override)
			{
				return Plugin_Continue;
			}
			else if(g_iType[client] == Type_SurfOverride && IsSurfing(client))
			{
				return Plugin_Continue;
			}
		}

		float flFowardMove, flSideMove;
		float flMaxSpeed = GetEntPropFloat(client, Prop_Data, "m_flMaxspeed");
		float flSurfaceFriction = 1.0;
		if(g_iSurfaceFrictionOffset > 0)
		{
			flSurfaceFriction = GetEntDataFloat(client, g_iSurfaceFrictionOffset);
			if(g_ConVar_AutoFind_Offset.BoolValue && !(flSurfaceFriction == 0.25 || flSurfaceFriction == 1.0))
			{
				FindNewFrictionOffset(client);
			}
		}


		float flVelocity[3], flVelocity2D[2];

		GetEntPropVector(client, Prop_Data, "m_vecVelocity", flVelocity);

		flVelocity2D[0] = flVelocity[0];
		flVelocity2D[1] = flVelocity[1];

		// PrintToChat(client, "%f", SquareRoot(flVelocity2D[0] * flVelocity2D[0] + flVelocity2D[1] * flVelocity2D[1]));

		GetIdealMovementsInAir(angles[1], flVelocity2D, flMaxSpeed, flSurfaceFriction, flFowardMove, flSideMove);

		float flAngleDifference = AngleNormalize(angles[1] - g_flOldYawAngle[client]);
		float flCurrentAngles = FloatAbs(flAngleDifference);


		// Right
		if (flAngleDifference < 0.0)
		{
			float flMaxDelta = GetMaxDeltaInAir(flVelocity2D, flMaxSpeed, flSurfaceFriction, true);
			vel[1] = g_fMaxMove;

			if (flCurrentAngles <= flMaxDelta * g_fPower[client])
			{
				vel[0] = flFowardMove * g_fMaxMove;
				vel[1] = flSideMove * g_fMaxMove;
			}
		}
		else if (flAngleDifference > 0.0)
		{
			float flMaxDelta = GetMaxDeltaInAir(flVelocity2D, flMaxSpeed, flSurfaceFriction, false);
			vel[1] = -g_fMaxMove;

			if (flCurrentAngles <= flMaxDelta * g_fPower[client])
			{
				vel[0] = flFowardMove * g_fMaxMove;
				vel[1] = flSideMove * g_fMaxMove;
			}
		}
		else
		{
			vel[0] = flFowardMove * g_fMaxMove;
			vel[1] = flSideMove * g_fMaxMove;
		}
	}

	g_flOldYawAngle[client] = angles[1];

	return Plugin_Continue;
}

void FindNewFrictionOffset(int client)
{
	for(int i = 1; i <= 128; ++i)
	{
		float friction = GetEntDataFloat(client, g_iSurfaceFrictionOffset + i);
		if(friction == 0.25 || friction == 1.0)
		{
			g_iSurfaceFrictionOffset += i;

			LogError("[XUTAX] Current offset is out of date. Please update to new offset: %i", g_iSurfaceFrictionOffset);
			break;
		}
	}
}

// natives
public any Native_SetAutostrafe(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	bool value = GetNativeCell(2);
	g_bEnabled[client] = value;
	return 0;
}

public any Native_GetAutostrafe(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	return g_bEnabled[client];
}

public any Native_SetType(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	int value = GetNativeCell(2);
	g_iType[client] = value;
	return 0;
}

public any Native_GetType(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	return g_iType[client];
}

public any Native_SetPower(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	float value = GetNativeCell(2);
	g_fPower[client] = value;
	return 0;
}

public any Native_GetPower(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	return g_fPower[client];
}

// stocks
// taken from shavit's oryx
stock bool IsSurfing(int client)
{
	float fPosition[3];
	GetClientAbsOrigin(client, fPosition);

	float fEnd[3];
	fEnd = fPosition;
	fEnd[2] -= 64.0;

	float fMins[3];
	GetEntPropVector(client, Prop_Send, "m_vecMins", fMins);

	float fMaxs[3];
	GetEntPropVector(client, Prop_Send, "m_vecMaxs", fMaxs);

	Handle hTR = TR_TraceHullFilterEx(fPosition, fEnd, fMins, fMaxs, MASK_PLAYERSOLID, TRFilter_NoPlayers, client);

	if(TR_DidHit(hTR))
	{
		float fNormal[3];
		TR_GetPlaneNormal(hTR, fNormal);

		delete hTR;

		// If the plane normal's Z axis is 0.7 or below (alternatively, -0.7 when upside-down) then it's a surf ramp.
		// https://mxr.alliedmods.net/hl2sdk-css/source/game/server/physics_main.cpp#1059

		return (-0.7 <= fNormal[2] <= 0.7);
	}

	delete hTR;

	return false;
}

public bool TRFilter_NoPlayers(int entity, int mask, any data)
{
	return (entity != view_as<int>(data) || (entity < 1 || entity > MaxClients));
}

stock bool ShouldProcessFrame(int client)
{
	if(g_bTASEnabled)
	{
		if(TAS_Enabled(client))
		{
			return TAS_ShouldProcessFrame(client);
		}
	}

	return true;
}


// reference code
/*
void CGameMovement::AirAccelerate( Vector& wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

	if (player->pl.deadflag)
		return;

	if (player->m_flWaterJumpTime)
		return;

	if (wishspd > 30)
		wishspd = 30;

	// I guess you remember how to do a dot product but this is how it works:
	// mv->m_vecVelocity.x * wishdir.x + mv->m_vecVelocity.y * wishdir.y
	// we also know that if you remember from school
    // velocityLength2D * wishdirLength2D * cos(theta) gives also the dot product.
	// wishdir is normalized, so we know that its length is 1
	// velocityLength2D * cos(theta)
	// You may heard why it needs the perpendicular of velocity
	// And thats where it says it all.
	// if cos(theta) == 0 then the dot product is 0
	// wich means that theta = 90.
	// But do we really want to be always perpendicular to the velocity?

	currentspeed = mv->m_vecVelocity.Dot(wishdir);

	// wishspd is capped to 30, but as you can see under
	addspeed = wishspd - currentspeed;

	if (addspeed <= 0)
		return;

	// accelspeed also regulates it, so it is not always 30 but accelspeed.
	accelspeed = accel * wishspeed * gpGlobals->frametime * player->m_surfaceFriction;

	// so if accelspeed is 15 for example
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i=0 ; i<3 ; i++)
	{
		// It will apply 15 * wishdir here.
		// Wich means if we go always on the perpendicular of velocity
		// We will deaccelerate when accelspeed < 30
		// Because the used dot product won't correspond to the gain accelspeed.
		mv->m_vecVelocity[i] += accelspeed * wishdir[i];
		mv->m_outWishVel[i] += accelspeed * wishdir[i];
	}
}
*/

/*
void CGameMovement::AirMove( void )
{
	int			i;
	Vector		wishvel;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	// Copy movement amounts
	fmove = mv->m_flForwardMove; // cmd->forwardmove
	smove = mv->m_flSideMove; // cmd->sidemove

	// Zero out z components of movement vectors to remove upwards velocity
	forward[2] = 0;
	right[2]   = 0;

	VectorNormalize(forward);
	VectorNormalize(right);

	for (i = 0; i < 2; i++) //;
		wishvel[i] = forward[i] * fmove + right[i] * smove; // As you can see here, the movement depends in the forwardmove and sidemove values;

	wishvel[2] = 0; // Z doesn't matter here

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir); // Basically it normalizes the vector in order to get the wishspeed desired from forwardmove/sidemove values
	// So if mv->m_flMaxSpeed is = 260 it is not necessary to set the values for sidemove or forwardmove to 450, but only 260 (since it's the maximum you can get).

	if ( wishspeed != 0 && (wishspeed > mv->m_flMaxSpeed))
	{
		VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}

	//
	AirAccelerate( wishdir, wishspeed, sv_airaccelerate.GetFloat() );

	// Add in any base velocity to the current velocity.
	VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	TryPlayerMove();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
}
*/
