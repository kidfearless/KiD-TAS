#if defined _xutax_strafe_included
 #endinput
#endif
#define _xutax_strafe_included


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
