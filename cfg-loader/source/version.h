#ifndef _VERSION_H 
#define _VERSION_H 

#define STR(X) #X
#define QUOTE(X) STR(X)

#ifndef VERSION
#define VERSION 00xy
#endif

#define VERSION_STR QUOTE(VERSION)

#define CFG_VERSION_STR VERSION_STR

#endif 

