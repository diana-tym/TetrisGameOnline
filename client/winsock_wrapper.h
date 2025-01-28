#pragma once
#ifndef WINSOCK_WRAPPER_H
#define WINSOCK_WRAPPER_H

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOSOUND
#include <Windows.h>
#include <winsock2.h>  // Specific to your project
#include <ws2tcpip.h>
#endif

#endif  // WINSOCK_WRAPPER_H

