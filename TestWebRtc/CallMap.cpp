/* 
 * Copyright (C) 2012 Yee Young Han <websearch@naver.com> (http://blog.naver.com/websearch)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "CallMap.h"

CCallMap gclsCallMap;

CCallMap::CCallMap()
{
}

CCallMap::~CCallMap()
{
}

bool CCallMap::Insert( const char * pszCallId, const char * pszUserId )
{
	CALL_MAP::iterator itMap;
	bool bRes = false;

	m_clsMutex.acquire();
	itMap = m_clsMap.find( pszCallId );
	if( itMap == m_clsMap.end() )
	{
		CCallInfo clsInfo;

		clsInfo.m_strUserId = pszUserId;

		m_clsMap.insert( CALL_MAP::value_type( pszCallId, clsInfo ) );
		bRes = true;
	}
	m_clsMutex.release();

	return bRes;
}

bool CCallMap::Select( const char * pszCallId, CCallInfo & clsCallInfo )
{
	CALL_MAP::iterator itMap;
	bool bRes = false;

	m_clsMutex.acquire();
	itMap = m_clsMap.find( pszCallId );
	if( itMap != m_clsMap.end() )
	{
		clsCallInfo = itMap->second;
		bRes = true;
	}
	m_clsMutex.release();

	return bRes;
}

bool CCallMap::Delete( const char * pszCallId )
{
	CALL_MAP::iterator itMap;
	bool bRes = false;

	m_clsMutex.acquire();
	itMap = m_clsMap.find( pszCallId );
	if( itMap != m_clsMap.end() )
	{
		m_clsMap.erase( itMap );
		bRes = true;
	}
	m_clsMutex.release();

	return bRes;
}

bool CCallMap::DeleteUserId( const char * pszUserId, SIP_CALL_ID_LIST & clsCallIdList )
{
	CALL_MAP::iterator itMap, itNext;
	bool bRes = false;

	clsCallIdList.clear();

	m_clsMutex.acquire();
	for( itMap = m_clsMap.begin(); itMap != m_clsMap.end(); ++itMap )
	{
LOOP_START:
		if( !strcmp( pszUserId, itMap->second.m_strUserId.c_str() ) )
		{
			clsCallIdList.push_back( itMap->first );

			itNext = itMap;
			++itNext;

			m_clsMap.erase( itMap );

			if( itNext == m_clsMap.end() ) break;
			itMap = itNext;
			goto LOOP_START;
		}
	}
	m_clsMutex.release();

	if( clsCallIdList.size() > 0 )
	{
		bRes = true;
	}

	return bRes;
}
