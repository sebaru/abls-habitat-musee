/******************************************************************************************************************************/
/* Watchdogd/Http/login.c       Gestion des request login pour le thread HTTP de watchdog                                     */
/* Projet WatchDog version 2.0       Gestion d'habitat                                                    05.09.2016 15:01:29 */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * login.c
 * This file is part of Watchdog
 *
 * Copyright (C) 2010 - Sebastien Lefevre
 *
 * Watchdog is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Watchdog is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Watchdog; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
 #include <openssl/rand.h>

/**************************************************** Prototypes de fonctions *************************************************/
 #include "watchdogd.h"
 #include "Http.h"

 static const char *PARAM_LOGIN[] =
  { "username", "password" };
 enum
  { PARAM_LOGIN_USERNAME,
    PARAM_LOGIN_PASSWORD,
    NBR_PARAM_LOGIN
  };

/******************************************************************************************************************************/
/* Groups_to_xml: Renvoi la liste des groupes de l'utilisateur, au format XML                                                 */
/* Entr??e : l'utilisateur                                                                                                     */
/* Sortie : xmlBuffer, ou NULL si erreur                                                                                      */
/******************************************************************************************************************************/
 xmlBufferPtr Groups_to_xml ( struct CMD_TYPE_UTILISATEUR *util )
  { xmlTextWriterPtr writer;
    xmlBufferPtr buf;
    gint cpt;

    buf = xmlBufferCreate();                                                                        /* Creation du buffer xml */
    if (buf == NULL)
     { Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_ERR,
                 "%s: XML Buffer creation failed", __func__ );
       return(NULL);
     }

    if ( (writer = xmlNewTextWriterMemory(buf, 0)) == NULL )                                         /* Creation du write XML */
     { Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_ERR,
                "%s: XML Writer creation failed", __func__ );
       xmlBufferFree(buf);
       return(NULL);
     }

    if ( xmlTextWriterStartDocument(writer, NULL, "UTF-8", "yes" ) < 0 )                              /* Creation du document */
     { Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_ERR,
                "%s: XML Start document failed", __func__ );
       xmlBufferFree(buf);
       return(NULL);
     }

    if (xmlTextWriterStartElement(writer, (const unsigned char *) "Groups") < 0)
     { Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_ERR,
                "%s: XML Failed to Start element Groups", __func__ );
       xmlBufferFree(buf);
       return(NULL);
     }

/*
 *     cpt=0;
    while ( util->gids[cpt] )
     { xmlTextWriterWriteFormatElement( writer, (const unsigned char *)"group", "%06d", util->gids[cpt] );
       cpt++;
     }
*/
    if (xmlTextWriterEndDocument(writer)<0)                                                                   /* End document */
     { Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_ERR,
                 "%s: Failed to end Document", __func__ );
       xmlBufferFree(buf);
       return(NULL);
     }

    xmlFreeTextWriter(writer);                                                                    /* Lib??ration du writer XML */
    return(buf);
  }
/******************************************************************************************************************************/
/* Http_get_session_id : Isole le SID d'une session d'une WSI                                                                 */
/* Entr??es: le WSI en question                                                                                                */
/* Sortie : sid ou "---" si erreur                                                                                            */
/******************************************************************************************************************************/
 gchar *Http_get_session_id ( struct HTTP_SESSION *session )
	 { if (session) return(session->sid_string);
    return("--- none ---");
  }
/******************************************************************************************************************************/
/* Http_get_session : V??rifie le num??ro de session en parametre                                                               */
/* Entr??es: le SID a tester                                                                                                   */
/* Sortie : la session, ou NULL si non trouv??e                                                                                */
/******************************************************************************************************************************/
 struct HTTP_SESSION *Http_get_session ( struct lws *wsi, gchar *remote_name, gchar *remote_ip )
  { struct HTTP_SESSION *session = NULL;
	   struct HTTP_PER_SESSION_DATA *pss;
    gchar buffer[4096], *sid = NULL;
    GSList *liste;

    pss = lws_wsi_user ( wsi );
    if ( lws_hdr_total_length( wsi, WSI_TOKEN_HTTP_COOKIE ) <= 0) return(NULL);
    if ( lws_hdr_copy( wsi, buffer, sizeof(buffer), WSI_TOKEN_HTTP_COOKIE ) != -1 )     /* R??cup??ration de la valeur du token */
     { gchar *cookies, *cookie, *savecookies;
       gchar *cookie_name, *cookie_value, *savecookie;
       cookies = buffer;
       while ( (cookie=strtok_r( cookies, ";", &savecookies)) != NULL )                                  /* D??coupage par ';' */
        { cookies=NULL;
          cookie_name=strtok_r( cookie, "=", &savecookie);                                               /* D??coupage par "=" */
          if (cookie_name)
           { cookie_value = strtok_r ( NULL, "=", &savecookie );
             if ( ! strcmp( cookie_name, "sid" ) )
              { sid = cookie_value;
                Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_DEBUG, "Http_get_session : Searching for sid %s", sid );
                pthread_mutex_lock( &Cfg_http.lib->synchro );                         /* Recherche dans la liste des sessions */
                liste = Cfg_http.Liste_sessions;
                while ( liste )
                 { session = (struct HTTP_SESSION *)liste->data;
                   if ( ! g_strcmp0 ( session->sid, sid ) )
                    { pss->session = session;
                      break;
                    }
                   liste = liste->next;
                 }
                pthread_mutex_unlock( &Cfg_http.lib->synchro );
                if (liste) return(session);
                return(NULL);
              }
             else Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_DEBUG,
                           "%s: Cookie found for %s(%s): %s=%s", __func__,
                            remote_name, remote_ip, (cookie_name ? cookie_name : "none"), (cookie_value ? cookie_value : "none") );
           }
        }       
     }
    return(NULL);                                                                          /* il n'y a pas de session trouv??e */
  }
/******************************************************************************************************************************/
/* Http Liberer_session : Libere la m??moire r??serv??e par la structure session                                                 */
/* Entr??es : la session ?? lib??rer                                                                                             */
/* Sortie : n??ant                                                                                                             */
/******************************************************************************************************************************/
 void Http_Liberer_session ( struct HTTP_SESSION *session )
  { pthread_mutex_lock( &Cfg_http.lib->synchro );                                     /* Recherche dans la liste des sessions */
    Cfg_http.Liste_sessions = g_slist_remove ( Cfg_http.Liste_sessions, session );
    pthread_mutex_unlock( &Cfg_http.lib->synchro );
    Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_INFO,
             "%s: (sid %s) session closed", __func__, Http_get_session_id (session) );
    if (session->util) g_free(session->util);
    g_free(session);                                                                     /* Lib??ration m??moire le cas ??ch??ant */
  }
/******************************************************************************************************************************/
/* Http Close_session : Termine une session sur demande terminal final                                                        */
/* Entr??es : la session ?? lib??rer                                                                                             */
/* Sortie : n??ant                                                                                                             */
/******************************************************************************************************************************/
 void Http_Close_session ( struct lws *wsi, struct HTTP_SESSION *session )
  { unsigned char header[256], *header_cur, *header_end;
    gint retour;
    if (session)
     { header_cur = header;
       header_end = header + sizeof(header);
       retour = lws_add_http_header_status( wsi, 200, &header_cur, header_end );
       retour = lws_finalize_http_header ( wsi, &header_cur, header_end );
       *header_cur='\0';                                                                            /* Caractere null d'arret */
       lws_write( wsi, header, header_cur - header, LWS_WRITE_HTTP_HEADERS );                      /* Send to client */
       Http_Liberer_session ( session );
     }
  }
/******************************************************************************************************************************/
/* Http_new_session : Cr??ation d'une nouvelle session pour la requete re??ue                                                   */
/* Entr??es : la session WSI                                                                                                   */
/* Sortie  : NULL si erreur                                                                                                   */
/******************************************************************************************************************************/
 static struct HTTP_SESSION *Http_new_session( struct lws *wsi, gchar *remote_name, gchar *remote_ip )
  { gchar cookie_bin[EVP_MAX_MD_SIZE], cookie_hex[2*EVP_MAX_MD_SIZE+1], cookie[256];
    struct HTTP_SESSION *session;
    gchar buffer[4096];
    gint cpt;

    session = (struct HTTP_SESSION *) g_try_malloc0 ( sizeof( struct HTTP_SESSION ) );
    if (!session)
     { Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_ERR,
                "%s: Memory Alloc ERROR session", __func__ );
       return(NULL);
     }
    session->last_top = Partage->top;
    session->is_ssl = lws_is_ssl ( wsi );

    if ( lws_hdr_copy( wsi, buffer, sizeof(buffer), WSI_TOKEN_HTTP_USER_AGENT ) != -1 ) /* R??cup??ration de la valeur du token */
     { g_snprintf( session->user_agent, sizeof(session->user_agent), "%s", buffer ); }

    g_snprintf( session->remote_name, sizeof(session->remote_name), "%s", remote_name );
    g_snprintf( session->remote_ip,   sizeof(session->remote_ip),   "%s", remote_ip   );

    RAND_bytes( (guchar *)cookie_bin, sizeof(cookie_bin) );                               /* R??cup??ration d'un nouveau COOKIE */
    for (cpt=0; cpt<sizeof(cookie_bin); cpt++)                                                 /* Mise en forme au format HEX */
     { g_snprintf( &session->sid[2*cpt], 3, "%02X", (guchar)cookie_bin[cpt] ); }

    Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_DEBUG,
             "%s: Creation session '%s' for %s/%s", __func__, session->sid, session->remote_name, session->remote_ip );

    pthread_mutex_lock( &Cfg_http.lib->synchro );                                         /* Ajout dans la liste des sessions */
    Cfg_http.Liste_sessions = g_slist_prepend( Cfg_http.Liste_sessions, session );
    pthread_mutex_unlock( &Cfg_http.lib->synchro );
    return(session);
  }
/******************************************************************************************************************************/
/* Http_Check_sessions : Fait tomber les sessions sur timeout                                                                 */
/* Entr??es: n??ant                                                                                                             */
/* Sortie : n??ant                                                                                                             */
/******************************************************************************************************************************/
 void Http_Check_sessions ( void )
  { struct HTTP_SESSION *session = NULL;
    GSList *liste;

search_again:
    pthread_mutex_lock( &Cfg_http.lib->synchro );                                            /* Ajout dans la liste a traiter */
    liste = Cfg_http.Liste_sessions;
    while ( liste )
     { session = (struct HTTP_SESSION *)liste->data;
       if (session->last_top && Partage->top - session->last_top >= 864000 )
        { Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_INFO,
                   "%s: closing timeout for SID %.12s", __func__, session->sid );
          break;
        }
       liste = liste->next;
     }
    pthread_mutex_unlock( &Cfg_http.lib->synchro );
    if (liste)
     { Http_Liberer_session(session);
       goto search_again;
     }
  }

/******************************************************************************************************************************/
/* Http_Traiter_request_login: Traite une requete de login                                                                    */
/* Entr??es: la connexion MHD                                                                                                  */
/* Sortie : n??ant                                                                                                             */
/******************************************************************************************************************************/
 gint Http_Traiter_request_login ( struct HTTP_SESSION *session, struct lws *wsi, gchar *remote_name, gchar *remote_ip )
  { struct HTTP_PER_SESSION_DATA *pss;

    Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_NOTICE,
             "Http_Traiter_request_login: HTTP request from %s(%s)",
              remote_name, remote_ip );

    if (session) Http_Liberer_session ( session );                                     /* si session existante, on la termine */
    pss = lws_wsi_user ( wsi );
    g_snprintf( pss->url, sizeof(pss->url), "/ws/login" );
    return(0);                                                        /* si pas de session, on continue de traiter la request */
  }
/******************************************************************************************************************************/
/* Http_Traiter_request_body_login: Traite une requete de login phase 2 (reception body)                                      */
/* Entr??es: la connexion wsi, et les data recue et leur taille                                                                */
/* Sortie : 1 si pb                                                                                                           */
/******************************************************************************************************************************/
 gint Http_Traiter_request_body_login ( struct lws *wsi, void *data, size_t taille )
  { struct HTTP_PER_SESSION_DATA *pss;
    pss = lws_wsi_user ( wsi );
    if (!pss->spa)
     {	pss->spa = lws_spa_create(wsi, PARAM_LOGIN, NBR_PARAM_LOGIN, 256, NULL, pss );
    			if (!pss->spa)	return(1);
     }
    return(lws_spa_process(pss->spa, data, taille));
  }
/******************************************************************************************************************************/
/* Http_Traiter_request_login: Traite une requete de login                                                                    */
/* Entr??es: la connexion MHD                                                                                                  */
/* Sortie : n??ant                                                                                                             */
/******************************************************************************************************************************/
 gint Http_Traiter_request_body_completion_login ( struct lws *wsi, gchar *remote_name, gchar *remote_ip )
  { gchar buffer[4096], username[80], password[80];
    unsigned char header[512], *header_cur, *header_end;
    struct HTTP_PER_SESSION_DATA *pss;
    struct CMD_TYPE_UTILISATEUR *util;
    struct HTTP_SESSION *session;
    gint retour, taille;

    Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_DEBUG,
             "%s: (sid %s) HTTP request from %s(%s)", __func__,
              Http_get_session_id(NULL), remote_name, remote_ip );

    pss = lws_wsi_user ( wsi );
    lws_spa_finalize(pss->spa);

    g_snprintf( username, sizeof(username), lws_spa_get_string ( pss->spa, PARAM_LOGIN_USERNAME ) );
    g_snprintf( password, sizeof(password), lws_spa_get_string ( pss->spa, PARAM_LOGIN_PASSWORD ) );

    header_cur = header;                                                             /* Pr??paration des headers de la r??ponse */
    header_end = header + sizeof(header);

    util = Rechercher_utilisateurDB_by_name( username );
    if (!util)
     { Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_WARNING,
                "%s: (sid %s) Username '%s' not found", __func__,
                 Http_get_session_id(NULL), username );
       retour = lws_add_http_header_status( wsi, 401, &header_cur, header_end );                              /* Unauthorized */
       retour = lws_finalize_http_header ( wsi, &header_cur, header_end );
       *header_cur='\0';                                                                            /* Caractere null d'arret */
       lws_write( wsi, header, header_cur - header, LWS_WRITE_HTTP_HEADERS );
     }
    else if ( Check_utilisateur_password( util, password ) == FALSE )
     { Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_WARNING,
                "%s: (sid %s) Wrong Password for user '%s'", __func__,
                 Http_get_session_id(NULL), username );
       retour = lws_add_http_header_status( wsi, 401, &header_cur, header_end );                              /* Unauthorized */
       retour = lws_finalize_http_header ( wsi, &header_cur, header_end );
       *header_cur='\0';                                                                            /* Caractere null d'arret */
       lws_write( wsi, header, header_cur - header, LWS_WRITE_HTTP_HEADERS );
     }
    else
     { xmlBufferPtr buf;
       buf = Groups_to_xml ( util );
       if (!buf)
        { g_free(util);
          retour = lws_add_http_header_status( wsi, 501, &header_cur, header_end );                           /* Server Error */
          retour = lws_finalize_http_header ( wsi, &header_cur, header_end );
          *header_cur='\0';                                                                         /* Caractere null d'arret */
          lws_write( wsi, header, header_cur - header, LWS_WRITE_HTTP_HEADERS );
        }
       else
        { session = Http_new_session ( wsi, remote_name, remote_ip );
          if (!session)
           { xmlBufferFree(buf);
             g_free(util);
             retour = lws_add_http_header_status( wsi, 501, &header_cur, header_end );                        /* Server Error */
             retour = lws_finalize_http_header ( wsi, &header_cur, header_end );
             *header_cur='\0';                                                                      /* Caractere null d'arret */
             lws_write( wsi, header, header_cur - header, LWS_WRITE_HTTP_HEADERS );
           }
          else                                                          /* Checking OK, create session and send habilitations */
           { const char *content_type = "application/xml";
             gchar cookie[512];

             session->util = util;                                                  /* Sauvegarde de la structure utilisateur */
             g_snprintf( session->sid_string, sizeof(session->sid_string), "%.12s:%s:%s:%s",
                         session->sid, session->remote_ip, session->remote_name, session->util->nom );

             g_snprintf ( cookie, sizeof(cookie), "sid=%s; Max-Age=%d; ", session->sid, 60*60*12 );
             retour = lws_add_http_header_status( wsi, 200, &header_cur, header_end );
             retour = lws_add_http_header_by_token ( wsi, WSI_TOKEN_HTTP_SET_COOKIE, (const unsigned char *)cookie, strlen(cookie),
                                                    &header_cur, header_end );
             retour = lws_add_http_header_by_token ( wsi, WSI_TOKEN_HTTP_CONTENT_TYPE, HTTP_CONTENT_XML, strlen(HTTP_CONTENT_XML),
                                                    &header_cur, header_end );
             retour = lws_add_http_header_content_length ( wsi, buf->use, &header_cur, header_end );
             retour = lws_finalize_http_header ( wsi, &header_cur, header_end );
             *header_cur='\0';                                                                      /* Caractere null d'arret */
             lws_write( wsi, header, header_cur - header, LWS_WRITE_HTTP_HEADERS );
             lws_write ( wsi, buf->content, buf->use, LWS_WRITE_HTTP);                                      /* Send to client */
             xmlBufferFree(buf);                                      /* Lib??ration du buffer dont nous n'avons plus besoin ! */
             Info_new( Config.log, Cfg_http.lib->Thread_debug, LOG_INFO,
                      "%s: (sid %s), New Session Cookie", __func__,
                       Http_get_session_id(session) );
          }
        }
     }
    lws_spa_destroy ( pss->spa	);
    return(1);
  }
/*----------------------------------------------------------------------------------------------------------------------------*/
