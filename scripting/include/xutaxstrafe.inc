#if defined _xutax_strafe_included
 #endinput
#endif
#define _xutax_strafe_included


enum
{
	Type_Normal, // only w/s disables autostrafe
	Type_SurfOverride, // w/s always disables, a/d only over surf ramps
	Type_Override, // all keys disable
	Type_Size // size
};


/**
 * Sets the new value of the strafehack on the given client index.
 *
 * @param client		Client index of the player to set the strafehack on.
 * @param value			New value to set the strafehack to.
 *
 * @noreturn				
 */
native void SetXutaxStrafe(int client, bool value);

/**
 * Retrieves the current strafehack setting for the given client index.
 *
 * @param client		Client index of the player to get the strafehack value from.

 * @return				The current strafehack value.
 */
native bool GetXutaxStrafe(int client);

/**
 * Sets the strafehack type on the given client index.
 *
 * @param client		Client index of the player to set the strafehack on.
 * @param value			New type to set the strafehack to.
 *
 * @noreturn				
 */
native void SetXutaxType(int client, int value);

/**
 * Retrieves the current strafehack type for the given client index.
 *
 * @param client		Client index of the player to get the strafehack type from.

 * @return				The current strafehack type.
 */
native int GetXutaxType(int client);
