#include "NetworkRuntimeQS.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#endif

namespace qs {

NetworkRuntimeQS::NetworkRuntimeQS() {
#ifdef _WIN32
    WSADATA data;
    ok_ = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    ok_ = true;
#endif
}

NetworkRuntimeQS::~NetworkRuntimeQS() {
#ifdef _WIN32
    if (ok_) WSACleanup();
#endif
}

}  // namespace qs
