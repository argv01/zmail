/*
 * Header file for the "zcomm.cpp" serial communications routines. 
 */

#ifndef _ZCOMM_H_
#define _ZCOMM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* global pointer to Comm Object */
extern void* pCommObj;

/* Function prototypes */

void* create_comm( void );
void configure_comm( void* pComm );
void connect_comm( void* pComm, LPSTR user, LPSTR password );
void open_comm( void* pComm, LPSTR host, LPSTR port );
void disconnect_comm( void* pComm );
int read_comm( void* pComm, LPSTR lpBuffer, int nLen );
int write_comm( void* pComm, LPSTR lpBuffer, int nLen );
void* get_comm( void );

#ifdef __cplusplus
}
#endif 

#endif	/* _ZCOMM_H_ */

