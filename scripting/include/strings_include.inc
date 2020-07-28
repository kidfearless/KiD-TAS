char Value[STRING_SIZE];

int Size() {return STRING_SIZE;}

void FromInt(int val)
{
	IntToString(val, this.Value, STRING_SIZE);
}

void FromFloat(float val)
{
	FloatToString(val, this.Value, STRING_SIZE);
}

void FromConVar(ConVar val)
{
	val.GetString(this.Value, STRING_SIZE);
}

void GetMenuInfo(Menu menu, int param)
{
	menu.GetItem(param, this.Value, STRING_SIZE);
}

void GetMenuDisplay(Menu menu, int param)
{
	menu.GetItem(param, "", 0, _, this.Value, STRING_SIZE);
}

int Length()
{
	return strlen(this.Value);
}

bool IsNullOrEmpty()
{
	if(IsNullString(this.Value))
	{
		return true;
	}
	if(this.Value[0] == 0)
	{
		return true;
	}
	return this.Equals(NULL_STRING);
}

bool BoolValue()
{
	return !!this.IntValue();
}

int Break(char[] arg, int argLen)
{
	return BreakString(this.Value, arg, argLen);
}

int Compare(const char[] str2, int num, bool caseSensitive = true)
{
	return strncmp(this.Value, str2, num, caseSensitive);
}

int Contains(const char[] substr, bool caseSensitive = true)
{
	return StrContains(this.Value, substr, caseSensitive);
}

bool Includes(const char[] substr, bool caseSensitive = true)
{
	return StrContains(this.Value, substr, caseSensitive) != -1;
}

bool Equals(const char[] str2, bool caseSensitive = true)
{
	return (strcmp(this.Value, str2, caseSensitive) == 0);
}

float FloatValue()
{
	return StringToFloat(this.Value);
}

int FloatValueEx(float &result)
{
	return StringToFloatEx(this.Value, result);
}

int GetBytes()
{
	return GetCharBytes(this.Value);
}

int IntValue(int nBase = 10)
{
	return StringToInt(this.Value, nBase);
}

int IntValueEx(int &result, int nBase = 10)
{
	return StringToIntEx(this.Value, result, nBase);
}

int FindChar(const char c, bool reverse = false)
{
	int len = this.Length();
	
	if (!reverse)
	{
		for (int i = 0; i < len; ++i)
		{
			if (this.Value[i] == c)
			{
				return i;
			}
		}
	} 
	else
	{
		for (int i = len - 1; i >= 0; --i)
		{
			if (this.Value[i] == c)
			{
				return i;
			}
		}
	}

	return -1;
}

int Split(const char[] split, char[] part, int partLen)
{
	return SplitString(this.Value, split, part, partLen);
}

bool Strip()
{
	return StripQuotes(this.Value);
}

void ToUpper()
{
	for(int i = 0; i < this.Length(); ++i)
	{
		this.Value[i] = CharToUpper(this.Value[i]);
	}
}

void ToLower()
{
	for(int i = 0; i < this.Length(); ++i)
	{
		this.Value[i] = CharToLower(this.Value[i]);
	}
}

int Trim()
{
	return TrimString(this.Value);
}

int Copy(const char[] source)
{
	return strcopy(this.Value, STRING_SIZE, source);
}

int Format(const char[] format, any ...)
{
	return VFormat(this.Value, STRING_SIZE, format, 3);
}

int ConCat(const char[] appender)
{
	int len = this.Length();
	return Format(this.Value[len], STRING_SIZE - len, "%s", appender);
}

int Insert(const char[] appender)
{
	return Format("%s %s", STRING_SIZE, appender, this.Value);
}
int Replace(const char[] search, const char[] replace, bool caseSensitive = true)
{
	return ReplaceString(this.Value, STRING_SIZE, search, replace, caseSensitive);
}

int ReplaceEx(const char[] search, const char[] replace, int searchLen = -1, int replaceLen = -1, bool caseSensitive = true)
{
	return ReplaceStringEx(this.Value, STRING_SIZE, search, replace, searchLen, replaceLen, caseSensitive);
}

// clients.inc

bool GetClientName(int client)
{
	return GetClientName(client, this.Value, STRING_SIZE);
}

bool GetClientIP(int client, bool remport = true)
{
	return GetClientIP(client, this.Value, STRING_SIZE, remport);
}

bool GetClientAuthId(int client, AuthIdType authType, bool validate=true)
{
	return GetClientAuthId(client, authType, this.Value, STRING_SIZE, validate);
}

bool GetClientInfo(int client, const char[] key)
{
	return GetClientInfo(client, key, this.Value, STRING_SIZE);
}

void GetClientModel(int client)
{
	GetClientModel(client, this.Value, STRING_SIZE);
}

void GetClientWeapon(int client, char[] weapon, int maxlen)
{
	GetClientWeapon(client, this.Value, STRING_SIZE);
}

void GetCmdArg(int arg)
{
	GetCmdArg(arg, this.Value, STRING_SIZE);
}