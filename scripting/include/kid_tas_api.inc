#if defined _kid_tas_api_included
	#endinput
#endif
#define _kid_tas_api_included

/**
 * @brief Returns whether or not the current usercmd will be processed.
 * 
 * @param client 			Client index
 * @return bool 			
 */
native bool TAS_ShouldProcessFrame(int client);
/**
 * @brief Returns the client's current timescale regardless of method used. Will return 1.0 if not on TAS
 * 
 * @param client 			Client index
 * @return float 
 */
native float TAS_GetCurrentTimescale(int client);
/**
 * @brief Returns whether or not the client is on TAS
 * 
 * @param client 			Client index
 * @return bool
 */
native bool TAS_Enabled(int client);

public SharedPlugin __pl_kid_tas =
{
	name = "kid-tas",
	required = 0
};

public void __pl_kid_tas_SetNTVOptional()
{
	MarkNativeAsOptional("TAS_ShouldProcessFrame");
	MarkNativeAsOptional("TAS_GetCurrentTimescale");
	MarkNativeAsOptional("TAS_Enabled");
}