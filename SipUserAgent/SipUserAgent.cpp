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

#include "SipUserAgent.h"
#include "SipRegisterThread.h"
#include "SipUtility.h"
#include "SdpMessage.h"
#include <time.h>

CSipStack	gclsSipStack;

#include "SipUserAgentRegister.hpp"
#include "SipUserAgentInvite.hpp"
#include "SipUserAgentBye.hpp"
#include "SipUserAgentCancel.hpp"

/**
 * @ingroup SipUserAgent
 * @brief 생성자
 */
CSipUserAgent::CSipUserAgent() : m_pclsCallBack(NULL)
{
}

/**
 * @ingroup SipUserAgent
 * @brief 소멸자
 */
CSipUserAgent::~CSipUserAgent()
{
}

/**
 * @ingroup SipUserAgent
 * @brief SIP 로그인 정보를 추가한다.
 * @param clsInfo SIP 로그인 정보 저장 객체
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::AddRegisterInfo( CSipServerInfo & clsInfo )
{
	if( clsInfo.m_strIp.empty() ) return false;
	if( clsInfo.m_strUserId.empty() ) return false;

	if( clsInfo.m_strDomain.empty() )
	{
		clsInfo.m_strDomain = clsInfo.m_strIp;
	}

	m_clsRegisterMutex.acquire();
	m_clsRegisterList.push_back( clsInfo );
	m_clsRegisterMutex.release();

	return true;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP stack 을 시작하고 SIP 로그인 쓰레드를 시작한다.
 * @param clsSetup	SIP stack 설정 객체
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::Start( CSipStackSetup & clsSetup )
{
	gclsSipStack.AddCallBack( this );

	if( gclsSipStack.Start( clsSetup ) == false ) return false;

	StartSipRegisterThread( this );

	return true;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP stack 을 종료하고 SIP 로그인 쓰레드를 종료한다.
 * @returns true 를 리턴한다.
 */
bool CSipUserAgent::Stop( )
{
	// QQQ: SIP 로그아웃하여야 한다.

	gclsSipStack.Stop();

	return true;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP 통화 요청 메시지를 전송한다.
 * @param pszFrom		발신자 아이디
 * @param pszTo			수신자 아이디
 * @param pclsRtp		local RTP 정보 저장 객체
 * @param pclsRoute SIP 메시지 목적지 주소 저장 객체
 * @param strCallId 생성된 INVITE 메시지의 Call-ID 가 저장될 변수
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::StartCall( const char * pszFrom, const char * pszTo, CSipCallRtp * pclsRtp, CSipCallRoute * pclsRoute, std::string & strCallId )
{
	if( pszFrom == NULL || pszTo == NULL ) return false;
	if( pclsRtp == NULL || pclsRoute == NULL ) return false;

	CSipDialog	clsDialog;

	clsDialog.m_strFromId = pszFrom;
	clsDialog.m_strToId = pszTo;

	clsDialog.m_strLocalRtpIp = pclsRtp->m_strIp;
	clsDialog.m_iLocalRtpPort = pclsRtp->m_iPort;
	clsDialog.m_iCodec = pclsRtp->m_iCodec;

	clsDialog.m_strContactIp = pclsRoute->m_strDestIp;
	clsDialog.m_iContactPort = pclsRoute->m_iDestPort;

	if( SendInvite( clsDialog ) == false ) return false;
	strCallId = clsDialog.m_strCallId;

	return true;
}

/**
 * @ingroup SipUserAgent
 * @brief 통화를 종료한다. 
 *				통화 요청을 보내고 연결되지 않으면 통화 취소 메시지를 전송한다.
 *				통화 연결되었으면 통화 종료 메시지를 전송한다.
 *				통화 수락인 경우 통화 거절 응답 메시지를 전송한다.
 * @param pszCallId SIP Call-ID
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::StopCall( const char * pszCallId )
{
	SIP_DIALOG_MAP::iterator		itMap;
	bool	bRes = false;
	CSipMessage * pclsMessage = NULL;

	m_clsMutex.acquire();
	itMap = m_clsMap.find( pszCallId );
	if( itMap != m_clsMap.end() )
	{
		if( itMap->second.m_sttStartTime.tv_sec != 0 )
		{
			if( itMap->second.m_sttEndTime.tv_sec == 0 )
			{
				pclsMessage = itMap->second.CreateBye();
				gettimeofday( &itMap->second.m_sttEndTime, NULL );
			}
		}
		else
		{
			if( itMap->second.m_pclsInvite )
			{
				pclsMessage = itMap->second.m_pclsInvite->CreateResponse( SIP_DECLINE );
				gettimeofday( &itMap->second.m_sttEndTime, NULL );
				m_clsMap.erase( itMap );
			}
			else if( itMap->second.m_sttCancelTime.tv_sec == 0 )
			{
				pclsMessage = itMap->second.CreateCancel();
				gettimeofday( &itMap->second.m_sttCancelTime, NULL );
			}
		}
		bRes = true;
	}
	m_clsMutex.release();

	if( pclsMessage )
	{
		gclsSipStack.SendSipMessage( pclsMessage );
	}

	return bRes;
}

/**
 * @ingroup SipUserAgent
 * @brief 수신된 통화를 수락한다.
 * @param pszCallId SIP Call-ID
 * @param pclsRtp		local RTP 정보 저장 객체
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::AcceptCall( const char * pszCallId, CSipCallRtp * pclsRtp )
{
	SIP_DIALOG_MAP::iterator		itMap;
	bool	bRes = false;
	CSipMessage * pclsMessage = NULL;

	if( pclsRtp == NULL ) return false;

	m_clsMutex.acquire();
	itMap = m_clsMap.find( pszCallId );
	if( itMap != m_clsMap.end() )
	{
		if( itMap->second.m_sttStartTime.tv_sec == 0 )
		{
			itMap->second.m_strLocalRtpIp = pclsRtp->m_strIp;
			itMap->second.m_iLocalRtpPort = pclsRtp->m_iPort;
			itMap->second.m_iCodec = pclsRtp->m_iCodec;

			pclsMessage = itMap->second.m_pclsInvite->CreateResponse( SIP_OK );
			gettimeofday( &itMap->second.m_sttStartTime, NULL );

			itMap->second.AddSdp( pclsMessage );

			delete itMap->second.m_pclsInvite;
			itMap->second.m_pclsInvite = NULL;

			bRes = true;
		}
	}
	m_clsMutex.release();

	if( pclsMessage )
	{
		gclsSipStack.SendSipMessage( pclsMessage );
	}

	return bRes;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP 통화 요청 INVITE 메시지를 검색한다.
 * @param pszCallId SIP Call-ID
 * @returns 성공하면 SIP 통화 요청 INVITE 메시지를 리턴하고 실패하면 NULL 를 리턴한다.
 */
CSipMessage * CSipUserAgent::DeleteIncomingCall( const char * pszCallId )
{
	SIP_DIALOG_MAP::iterator		itMap;
	CSipMessage * pclsMessage = NULL;

	m_clsMutex.acquire();
	itMap = m_clsMap.find( pszCallId );
	if( itMap != m_clsMap.end() )
	{
		if( itMap->second.m_sttStartTime.tv_sec == 0 )
		{
			if( itMap->second.m_pclsInvite )
			{
				pclsMessage = itMap->second.m_pclsInvite;
				itMap->second.m_pclsInvite = NULL;
				m_clsMap.erase( itMap );
			}
		}
	}
	m_clsMutex.release();

	return pclsMessage;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP 통화 요청에 대한 Ring / Session Progress 응답 메시지를 전송한다.
 * @param pszCallId		SIP Call-ID
 * @param iSipStatus	SIP 응답 코드
 * @param pclsRtp			local RTP 정보 저장 객체
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::RingCall( const char * pszCallId, int iSipStatus, CSipCallRtp * pclsRtp )
{
	SIP_DIALOG_MAP::iterator		itMap;
	CSipMessage * pclsMessage = NULL;
	bool	bRes = false;

	m_clsMutex.acquire();
	itMap = m_clsMap.find( pszCallId );
	if( itMap != m_clsMap.end() )
	{
		if( itMap->second.m_sttStartTime.tv_sec == 0 )
		{
			if( itMap->second.m_pclsInvite )
			{
				pclsMessage = itMap->second.m_pclsInvite->CreateResponse( iSipStatus );

				if( pclsRtp )
				{
					itMap->second.m_strLocalRtpIp = pclsRtp->m_strIp;
					itMap->second.m_iLocalRtpPort = pclsRtp->m_iPort;
					itMap->second.m_iCodec = pclsRtp->m_iCodec;
					itMap->second.AddSdp( pclsMessage );
				}

				bRes = true;
			}
		}
	}
	m_clsMutex.release();

	if( pclsMessage )
	{
		gclsSipStack.SendSipMessage( pclsMessage );
	}

	return bRes;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP 요청 메시지 수신 callback method
 * @param iThreadId		SIP stack 의 UDP 쓰레드 아이디
 * @param pclsMessage 수신된 SIP 요청 메시지
 * @returns SIP 요청 메시지를 처리한 경우 true 를 리턴하고 그렇지 않으면 false 를 리턴한다.
 */
bool CSipUserAgent::RecvRequest( int iThreadId, CSipMessage * pclsMessage )
{
	if( pclsMessage->IsMethod( "INVITE" ) )
	{
		return RecvInviteRequest( iThreadId, pclsMessage );
	}
	else if( pclsMessage->IsMethod( "BYE" ) )
	{
		return RecvByeRequest( iThreadId, pclsMessage );
	}
	else if( pclsMessage->IsMethod( "CANCEL" ) )
	{
		return RecvCancelRequest( iThreadId, pclsMessage );
	}

	return false;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP 응답 메시지 수신 callback method
 * @param iThreadId		SIP stack 의 UDP 쓰레드 아이디
 * @param pclsMessage 수신된 SIP 응답 메시지
 * @returns SIP 응답 메시지를 처리한 경우 true 를 리턴하고 그렇지 않으면 false 를 리턴한다.
 */
bool CSipUserAgent::RecvResponse( int iThreadId, CSipMessage * pclsMessage )
{
	if( pclsMessage->IsMethod( "REGISTER" ) )
	{
		return RecvRegisterResponse( iThreadId, pclsMessage );
	}
	else if( pclsMessage->IsMethod( "INVITE" ) )
	{
		return RecvInviteResponse( iThreadId, pclsMessage );
	}

	return false;
}

/**
 * @ingroup SipUserAgent
 * @brief CSipDialog 에서 SIP INVITE 메시지를 생성하여 전송한다.
 * @param clsDialog 통화 정보 저장 객체
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::SendInvite( CSipDialog & clsDialog )
{
	if( clsDialog.m_strFromId.empty() || clsDialog.m_strToId.empty() ) return false;
	
	SIP_DIALOG_MAP::iterator			itMap;
	char	szTag[SIP_TAG_MAX_SIZE], szBranch[SIP_BRANCH_MAX_SIZE], szCallIdName[SIP_CALL_ID_NAME_MAX_SIZE];
	bool	bInsert = false;
	CSipMessage * pclsMessage = NULL;

	SipMakeTag( szTag, sizeof(szTag) );
	SipMakeBranch( szBranch, sizeof(szBranch) );

	clsDialog.m_strFromTag = szTag;
	clsDialog.m_strViaBranch = szBranch;

	gettimeofday( &clsDialog.m_sttInviteTime, NULL );

	while( 1 )
	{
		SipMakeCallIdName( szCallIdName, sizeof(szCallIdName) );

		clsDialog.m_strCallId = szCallIdName;
		clsDialog.m_strCallId.append( "@" );
		clsDialog.m_strCallId.append( gclsSipStack.m_clsSetup.m_strLocalIp );

		m_clsMutex.acquire();
		itMap = m_clsMap.find( clsDialog.m_strCallId );
		if( itMap == m_clsMap.end() )
		{
			pclsMessage = clsDialog.CreateInvite();
			if( pclsMessage )
			{
				m_clsMap.insert( SIP_DIALOG_MAP::value_type( clsDialog.m_strCallId, clsDialog ) );
				bInsert = true;
			}
		}
		m_clsMutex.release();

		if( bInsert ) break;
	}

	if( gclsSipStack.SendSipMessage( pclsMessage ) == false )
	{
		delete pclsMessage;
		Delete( clsDialog.m_strCallId.c_str() );
		return false;
	}

	return true;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP Dialog 를 삭제한다.
 * @param pszCallId SIP Call-ID
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::Delete( const char * pszCallId )
{
	SIP_DIALOG_MAP::iterator			itMap;

	m_clsMutex.acquire();
	itMap = m_clsMap.find( pszCallId );
	if( itMap != m_clsMap.end() )
	{
		m_clsMap.erase( itMap );
	}
	m_clsMutex.release();

	return true;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP INVITE 응답 메시지에 포함된 정보를 CSipDialog 에 저장한다.
 * @param pclsMessage SIP INVITE 응답 메시지
 * @param pclsRtp			remote RTP 정보 저장 객체
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::SetInviteResponse( CSipMessage * pclsMessage, CSipCallRtp * pclsRtp )
{
	std::string	strCallId;

	if( pclsMessage->GetCallId( strCallId ) == false ) return false;

	SIP_DIALOG_MAP::iterator		itMap;
	bool	bFound = false;
	CSipMessage *pclsAck = NULL, *pclsInvite = NULL;

	m_clsMutex.acquire();
	itMap = m_clsMap.find( strCallId );
	if( itMap != m_clsMap.end() )
	{
		pclsMessage->m_clsTo.SelectParam( "tag", itMap->second.m_strToTag );

		if( pclsRtp )
		{
			itMap->second.m_strRemoteRtpIp = pclsRtp->m_strIp;
			itMap->second.m_iRemoteRtpPort = pclsRtp->m_iPort;
			itMap->second.m_iCodec = pclsRtp->m_iCodec;
		}

		if( pclsMessage->m_iStatusCode >= 200 )
		{
			pclsMessage->m_clsTo.SelectParam( "tag", itMap->second.m_strToTag );
			pclsAck = itMap->second.CreateAck();

			if( pclsMessage->m_iStatusCode >= 200 && pclsMessage->m_iStatusCode < 300 )
			{
				if( itMap->second.m_sttStartTime.tv_sec == 0 )
				{
					gettimeofday( &itMap->second.m_sttStartTime, NULL );
				}

				SIP_FROM_LIST::iterator	itContact = pclsMessage->m_clsContactList.begin();
				if( itContact != pclsMessage->m_clsContactList.end() )
				{
					char	szUri[255];

					itContact->m_clsUri.ToString( szUri, sizeof(szUri) );
					itMap->second.m_strContactUri = szUri;
				}
			}
			else if( pclsMessage->m_iStatusCode == SIP_UNAUTHORIZED || pclsMessage->m_iStatusCode == SIP_PROXY_AUTHENTICATION_REQUIRED )
			{
				itMap->second.m_strToTag.clear();

				pclsInvite = itMap->second.CreateInvite();
				if( pclsInvite )
				{
					SIP_SERVER_INFO_LIST::iterator itSL;
					const char * pszUserId = pclsMessage->m_clsFrom.m_clsUri.m_strUser.c_str();

					m_clsRegisterMutex.acquire();
					for( itSL = m_clsRegisterList.begin(); itSL != m_clsRegisterList.end(); ++itSL )
					{
						if( !strcmp( itSL->m_strUserId.c_str(), pszUserId ) )
						{
							itSL->AddAuth( pclsInvite, pclsMessage );
							break;
						}
					}
					m_clsRegisterMutex.release();
				}
			}
			else
			{
				gettimeofday( &itMap->second.m_sttEndTime, NULL );
				m_clsMap.erase( itMap );
			}
		}

		bFound = true;
	}
	m_clsMutex.release();

	if( pclsAck )
	{
		gclsSipStack.SendSipMessage( pclsAck );
	}

	if( pclsInvite )
	{
		gclsSipStack.SendSipMessage( pclsInvite );
	}

	return bFound;
}

/**
 * @ingroup SipUserAgent
 * @brief SIP 메시지에서 RTP 정보를 가져온다.
 * @param pclsMessage SIP 메시지
 * @param clsRtp			RTP 정보 저장 변수
 * @returns 성공하면 true 를 리턴하고 실패하면 false 를 리턴한다.
 */
bool CSipUserAgent::GetSipCallRtp( CSipMessage * pclsMessage, CSipCallRtp & clsRtp )
{
	if( pclsMessage->m_clsContentType.IsEqual( "application", "sdp" ) && pclsMessage->m_strBody.empty() == false )
	{
		CSdpMessage clsSdp;

		if( clsSdp.Parse( pclsMessage->m_strBody.c_str(), pclsMessage->m_strBody.length() ) == -1 ) return false;

		clsRtp.m_strIp = clsSdp.m_clsConnection.m_strAddr;

		SDP_MEDIA_LIST::iterator itMedia = clsSdp.m_clsMediaList.begin();
		if( itMedia == clsSdp.m_clsMediaList.end() ) return false;

		clsRtp.m_iPort = itMedia->m_iPort;

		SDP_FMT_LIST::iterator itFmt;
		
		for( itFmt = itMedia->m_clsFmtList.begin(); itFmt != itMedia->m_clsFmtList.end(); ++itFmt )
		{
			clsRtp.m_iCodec = atoi( itFmt->c_str() );
			if( clsRtp.m_iCodec == 0 )
			{
				break;
			}
		}
		
		return true;
	}

	return false;
}
