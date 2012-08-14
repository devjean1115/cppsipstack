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

#ifndef _SIP_USER_AGENT_CALLBACK_H_
#define _SIP_USER_AGENT_CALLBACK_H_

#ifndef ANDROID
#include "SipStackDefine.h"
#else
#include "../sipstack/SipStackDefine.h"
#endif

class CSipCallRtp
{
public:
	std::string	m_strIp;
	int					m_iPort;
	int					m_iCodec;
};

class ISipUserAgentCallBack
{
public:
	virtual ~ISipUserAgentCallBack(){};

	virtual void EventRegister( CSipServerInfo clsInfo, int iStatus ) = 0;
	virtual void EventIncomingCall( const char * pszCallId, const char * pszFrom, CSipCallRtp * pclsRtp ) = 0;
	virtual void EventCallRing( const char * pszCallId, int iSipStatus, CSipCallRtp * pclsRtp ) = 0;
	virtual void EventCallStart( const char * pszCallId, CSipCallRtp * pclsRtp ) = 0;
	virtual void EventCallEnd( const char * pszCallId, int iSipStatus ) = 0;
	virtual void EventReInvite( const char * pszCallId, CSipCallRtp * pclsRtp ) = 0;
};

#endif