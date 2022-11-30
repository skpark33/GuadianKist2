#include "tcpGuestSocketAcceptor.h"


tcpGuestSocketAcceptor::tcpGuestSocketAcceptor() {
	
}

tcpGuestSocketAcceptor::~tcpGuestSocketAcceptor() {
}

int
tcpGuestSocketAcceptor::make_svc_handler(tcpGuestSocketSession*& pSvcHandler)
{
    return inherited::make_svc_handler(pSvcHandler);
}

int
tcpGuestSocketAcceptor::activate_svc_handler(tcpGuestSocketSession* pSvcHandler)
{

	return inherited::activate_svc_handler(pSvcHandler);
}