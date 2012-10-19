#include "SipParserDefine.h"
#include <stdio.h>
#include <string.h>

bool TestInt()
{
	uint64_t i64 = 12345678901234567890ULL;
	char	szBuf[255];

	snprintf( szBuf, sizeof(szBuf), "%llu", i64 );
	if( strcmp( szBuf, "12345678901234567890" ) )
	{
		printf( "snprintf uint64 is error\n" );
		return false;
	}
	
	return true;
}
