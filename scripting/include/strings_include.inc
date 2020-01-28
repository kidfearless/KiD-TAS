void FromInt(int val)
{
	IntToString(val, this.StringValue, this.Size());
}

void FromFloat(float val)
{
	FloatToString(val, this.StringValue, this.Size());
}

void FromConVar(ConVar val)
{
	val.GetString(this.StringValue, this.Size());
}

void GetMenuInfo(Menu menu, int param)
{
	menu.GetItem(param, this.StringValue, this.Size());
}

void GetMenuDisplay(Menu menu, int param)
{
	menu.GetItem(param, "", 0, _, this.StringValue, this.Size());
}

int Length()
{
	return strlen(this.StringValue);
}

bool IsNullOrEmpty()
{
	if(IsNullString(this.StringValue))
	{
		return true;
	}
	if(this.StringValue[0] == 0)
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
	return BreakString(this.StringValue, arg, argLen);
}

int Compare(const char[] str2, int num, bool caseSensitive = true)
{
	return strncmp(this.StringValue, str2, num, caseSensitive);
}

int Contains(const char[] substr, bool caseSensitive = true)
{
	return StrContains(this.StringValue, substr, caseSensitive);
}

bool Equals(const char[] str2, bool caseSensitive = true)
{
	return (strcmp(this.StringValue, str2, caseSensitive) == 0);
}

float FloatValue()
{
	return StringToFloat(this.StringValue);
}

int FloatValueEx(float &result)
{
	return StringToFloatEx(this.StringValue, result);
}

int GetBytes()
{
	return GetCharBytes(this.StringValue);
}

int IntValue(int nBase = 10)
{
	return StringToInt(this.StringValue, nBase);
}

int IntValueEx(int &result, int nBase = 10)
{
	return StringToIntEx(this.StringValue, result, nBase);
}

int FindChar(const char c, bool reverse = false)
{
	int len = this.Length();
	
	if (!reverse)
	{
		for (int i = 0; i < len; ++i)
		{
			if (this.StringValue[i] == c)
			{
				return i;
			}
		}
	} 
	else
	{
		for (int i = len - 1; i >= 0; --i)
		{
			if (this.StringValue[i] == c)
			{
				return i;
			}
		}
	}

	return -1;
}

int Split(const char[] split, char[] part, int partLen)
{
	return SplitString(this.StringValue, split, part, partLen);
}

bool Strip()
{
	return StripQuotes(this.StringValue);
}

void ToUpper()
{
	for(int i = 0; i < this.Length(); ++i)
	{
		this.StringValue[i] = CharToUpper(this.StringValue[i]);
	}
}

void ToLower()
{
	for(int i = 0; i < this.Length(); ++i)
	{
		this.StringValue[i] = CharToLower(this.StringValue[i]);
	}
}

int Trim()
{
	return TrimString(this.StringValue);
}

int Copy(const char[] source)
{
	return strcopy(this.StringValue, this.Size(), source);
}

int Format(const char[] format, any ...)
{
	return VFormat(this.StringValue, this.Size(), format, 3);
}

int ConCat(const char[] appender)
{
	int len = this.Length();
	return Format(this.StringValue[len], this.Size() - len, "%s", appender);
}

int Insert(const char[] appender)
{
	return Format("%s %s", this.Size(), appender, this.StringValue);
}
int Replace(const char[] search, const char[] replace, bool caseSensitive = true)
{
	return ReplaceString(this.StringValue, this.Size(), search, replace, caseSensitive);
}

int ReplaceEx(const char[] search, const char[] replace, int searchLen = -1, int replaceLen = -1, bool caseSensitive = true)
{
	return ReplaceStringEx(this.StringValue, this.Size(), search, replace, searchLen, replaceLen, caseSensitive);
}

// clients.inc

bool GetClientName(int client)
{
	return GetClientName(client, this.StringValue, this.Size());
}

bool GetClientIP(int client, bool remport = true)
{
	return GetClientIP(client, this.StringValue, this.Size(), remport);
}

bool GetClientAuthId(int client, AuthIdType authType, bool validate=true)
{
	return GetClientAuthId(client, authType, this.StringValue, this.Size(), validate);
}

bool GetClientInfo(int client, const char[] key)
{
	return GetClientInfo(client, key, this.StringValue, this.Size());
}

void GetClientModel(int client)
{
	GetClientModel(client, this.StringValue, this.Size());
}

void GetClientWeapon(int client, char[] weapon, int maxlen)
{
	GetClientWeapon(client, this.StringValue, this.Size());
}

void GetCmdArg(int arg)
{
	GetCmdArg(arg, this.StringValue, this.Size());
}