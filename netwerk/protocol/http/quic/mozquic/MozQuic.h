/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

/* This interface is straight C - the library implementation is not. */
/* Eventually this will form an ABI - but right now its a construction zone
   so buyer beware
*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  const char mozquic_alpn[] = "hq-03";
  const uint32_t mozquic_library_version = 1;

  enum {
    MOZQUIC_OK           = 0,
    MOZQUIC_ERR_GENERAL  = 1,
    MOZQUIC_ERR_INVALID  = 2,
    MOZQUIC_ERR_MEMORY   = 3,
    MOZQUIC_ERR_IO       = 4,
    MOZQUIC_ERR_CRYPTO   = 5,
    MOZQUIC_ERR_VERSION  = 6,
  };

  typedef void mozquic_connection_t;

  struct mozquic_config_t 
  {
    const char *originName;
    int originPort;
    int handleIO; // true if library should schedule read and write events
    void *closure;

    unsigned int greaseVersionNegotiation; // flag
    unsigned int preferMilestoneVersion; // flag
    unsigned int ignorePKI; // flag

    void (*logging_callback)(void *, char *); // todo va arg
    int  (*send_callback)(void *, unsigned char *, uint32_t len);
    int  (*recv_callback)(void *, unsigned char *, uint32_t len, uint32_t *outLen);
    int  (*error_callback)(void *, uint32_t err, char *);

    // TLS API
    int (*handshake_input)(void *, unsigned char *data, uint32_t len);
  };

  // this is a hack. it will be come a 'crypto config' and allow server key/cert and
  // some kind of client ca root
  int mozquic_nss_config(char *dir);

  int mozquic_new_connection(mozquic_connection_t **outSession, struct mozquic_config_t *inConfig);
  int mozquic_destroy_connection(mozquic_connection_t *inSession);
  int mozquic_start_connection(mozquic_connection_t *inSession); // client rename todo
  int mozquic_start_server(mozquic_connection_t *inSession, int (*handle_new_connection)(void *, mozquic_connection_t *newconn));

  ////////////////////////////////////////////////////
  // IO handlers
  // if library is handling IO this does not need to be called
  // otherwise call it to indicate IO should be handled
  int mozquic_IO(mozquic_connection_t *inSession);
  // todo need one to get the pollset

  /* socket typedef */
#ifdef WIN32
  typedef SOCKET mozquic_socket_t;
#else
  typedef int mozquic_socket_t;
#endif
  
  mozquic_socket_t mozquic_osfd(mozquic_connection_t *inSession);
  void mozquic_setosfd(mozquic_connection_t *inSession, mozquic_socket_t fd);

  // the mozquic application may either delegate TLS handling to the lib
  // or may imlement the TLS API : mozquic_handshake_input/output and then
  // mozquic_handshake_complete(ERRORCODE)
  void mozquic_handshake_output(mozquic_connection_t *session,
                                unsigned char *data, uint32_t data_len);
  void mozquic_handshake_complete(mozquic_connection_t *session, uint32_t err);

#ifdef __cplusplus
}
#endif

