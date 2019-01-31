#include "TestSipUserAgent.h"

int main( int argc, char * argv[] )
{
	CSipUserAgent clsUserAgent;
	CSipStackSetup clsSetup;

	GetLocalIp( clsSetup.m_strLocalIp );

	if( clsUserAgent.Start( clsSetup, NULL ) == false )
	{
		printf( "clsUserAgent.Start() error\n" );
		return 0;
	}

	CSipCallRtp clsRtp;
	CSipCallRoute clsRoute;
	std::string strCallId;

	clsRtp.m_iCodec = 0;
	clsRtp.m_strIp = clsSetup.m_strLocalIp;
	clsRtp.m_iPort = 6000;

	clsRoute.m_strDestIp = "192.168.0.1";
	clsRoute.m_iDestPort = 5060;

	if( clsUserAgent.StartCall( "1000", "1001", &clsRtp, &clsRoute, strCallId ) == false )
	{
		printf( "clsUserAgent.StartCall() error\n" );
		return 0;
	}

	INVITE_TRANSACTION_MAP clsICTMap;
	INVITE_TRANSACTION_MAP::iterator itTM;
	char szPacket[2048];

	memset( szPacket, 0, sizeof(szPacket) );

	clsUserAgent.m_clsSipStack.GetICTMap( clsICTMap );

	itTM = clsICTMap.begin();
	CSipMessage * pclsResponse = itTM->second->m_pclsRequest->CreateResponse( SIP_OK );
	pclsResponse->AddRecordRoute( "192.168.0.100", 5060 );
	pclsResponse->AddRecordRoute( "192.168.0.200", 5060 );
	pclsResponse->ToString( szPacket, sizeof(szPacket) );

	printf( "[%s]\n", szPacket );

	Socket hSocket = UdpSocket();

	UdpSend( hSocket, szPacket, strlen(szPacket), clsSetup.m_strLocalIp.c_str(), clsSetup.m_iLocalUdpPort );

	sleep(10);

	return 0;
}
