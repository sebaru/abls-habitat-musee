/******************************************************************************************************************************/
/* Watchdogd/sms.c        Gestion des SMS de Watchdog v2.0                                                                    */
/* Projet WatchDog version 3.0       Gestion d'habitat                                       ven. 02 avril 2010 20:37:40 CEST */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * Sms.c
 * This file is part of Watchdog
 *
 * Copyright (C) 2010-2019 - Sebastien Lefevre
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

 #include <sys/time.h>
 #include <sys/prctl.h>
 #include <string.h>
 #include <unistd.h>
 #include <gammu.h>
 #include <curl/curl.h>

/**************************************************** Prototypes de fonctions *************************************************/
 #include "watchdogd.h"
 #include "Sms.h"

 static gboolean sending_is_disabled = FALSE;                      /* Variable permettant d'interdire l'envoi de sms si panic */
 static GSM_Error sms_send_status;
 static GSM_StateMachine *s=NULL;
 static INI_Section *cfg=NULL;
/******************************************************************************************************************************/
/* Smsg_Lire_config : Lit la config Watchdog et rempli la structure m??moire                                                   */
/* Entr??e: le pointeur sur la LIBRAIRIE                                                                                       */
/* Sortie: N??ant                                                                                                              */
/******************************************************************************************************************************/
 gboolean Smsg_Lire_config ( void )
  { gchar *nom, *valeur;
    struct DB *db;

    Cfg_smsg.lib->Thread_debug = FALSE;                                                        /* Settings default parameters */
    Cfg_smsg.enable            = FALSE;
    g_snprintf( Cfg_smsg.smsbox_apikey, sizeof(Cfg_smsg.smsbox_apikey), "%s", DEFAUT_SMSBOX_APIKEY );

    if ( ! Recuperer_configDB( &db, NOM_THREAD ) )                                          /* Connexion a la base de donn??es */
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_WARNING,
                "%s: Database connexion failed. Using Default Parameters", __func__ );
       return(FALSE);
     }

    while (Recuperer_configDB_suite( &db, &nom, &valeur ) )                           /* R??cup??ration d'une config dans la DB */
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_INFO,                                            /* Print Config */
                "%s: '%s' = %s", __func__, nom, valeur );
            if ( ! g_ascii_strcasecmp ( nom, "smsbox_apikey" ) )
        { g_snprintf( Cfg_smsg.smsbox_apikey, sizeof(Cfg_smsg.smsbox_apikey), "%s", valeur ); }
       else if ( ! g_ascii_strcasecmp ( nom, "tech_id" ) )
        { g_snprintf( Cfg_smsg.tech_id, sizeof(Cfg_smsg.tech_id), "%s", valeur ); }
       else if ( ! g_ascii_strcasecmp ( nom, "enable" ) )
        { if ( ! g_ascii_strcasecmp( valeur, "true" ) ) Cfg_smsg.enable = TRUE;  }
       else if ( ! g_ascii_strcasecmp ( nom, "debug" ) )
        { if ( ! g_ascii_strcasecmp( valeur, "true" ) ) Cfg_smsg.lib->Thread_debug = TRUE;  }
       else
        { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE,
                   "%s: Unknown Parameter '%s'(='%s') in Database", __func__, nom, valeur );
        }
     }
    return(TRUE);
  }
/******************************************************************************************************************************/
/* Smsg_send_status_to_master: Envoie le bit de comm au master selon le status du GSM                                         */
/* Entr??e: le status du GSM                                                                                                   */
/* Sortie: n??ant                                                                                                              */
/******************************************************************************************************************************/
 static void Smsg_send_status_to_master ( gboolean status )
  { if (Config.instance_is_master==TRUE)                                                          /* si l'instance est Maitre */
     { Dls_data_set_bool ( Cfg_smsg.tech_id, "COMM", &Cfg_smsg.bit_comm, status ); }                      /* Communication OK */
    else /* Envoi au master via thread HTTP */
     { JsonBuilder *builder;
       gchar *result;
       gsize taille;
       builder = Json_create ();
       json_builder_begin_object ( builder );
       Json_add_string ( builder, "tech_id",  Cfg_smsg.tech_id );
       Json_add_string ( builder, "acronyme", "COMM" );
       Json_add_bool   ( builder, "etat", status );
       json_builder_end_object ( builder );
       result = Json_get_buf ( builder, &taille );
       Send_zmq_with_tag ( Cfg_smsg.zmq_to_master, NULL, NOM_THREAD, "*", "msrv", "SET_BOOL", result, taille );
       g_free(result);
     }
    Cfg_smsg.comm_status = status;
  }
/******************************************************************************************************************************/
/* Recuperer_smsDB: r??cup??re la liste des utilisateurs et de leur num??ro de t??l??phone                                         */
/* Entr??e: une structure DB                                                                                                   */
/* Sortie: FALSE si pb                                                                                                        */
/******************************************************************************************************************************/
 gboolean Smsg_Recuperer_smsDB ( struct DB *db )
  { gchar requete[512];

    g_snprintf( requete, sizeof(requete),                                                                      /* Requete SQL */
                "SELECT id,username,enable,comment,sms_enable,sms_phone,sms_allow_cde "
                " FROM users as user ORDER BY username" );

    return ( Lancer_requete_SQL ( db, requete ) );                                             /* Execution de la requete SQL */
  }
/******************************************************************************************************************************/
/* Recuperer_recipient_authorized: Recupere la liste des users habilit?? a recevoir des SMS                                    */
/* Entr??e: une structure DB                                                                                                   */
/* Sortie: FALSE si pb                                                                                                        */
/******************************************************************************************************************************/
 static gboolean Smsg_Recuperer_recipient_authorized_smsDB ( struct DB *db )
  { gchar requete[512];

    g_snprintf( requete, sizeof(requete),                                                                      /* Requete SQL */
                "SELECT id,username,enable,comment,sms_enable,sms_phone,sms_allow_cde "
                " FROM users as user WHERE enable=1 AND sms_enable=1 ORDER BY username" );

    return ( Lancer_requete_SQL ( db, requete ) );                                             /* Execution de la requete SQL */
  }
/******************************************************************************************************************************/
/* Recuperer_liste_id_smsDB: Recup??ration de la liste des ids des smss                                                        */
/* Entr??e: une structure DB                                                                                                   */
/* Sortie: FALSE si pb                                                                                                        */
/******************************************************************************************************************************/
 struct SMSDB *Smsg_Recuperer_smsDB_suite( struct DB *db )
  { struct SMSDB *sms;

    Recuperer_ligne_SQL(db);                                                               /* Chargement d'une ligne resultat */
    if ( ! db->row )
     { Liberer_resultat_SQL ( db );
       return(NULL);
     }

    sms = (struct SMSDB *)g_try_malloc0( sizeof(struct SMSDB) );
    if (!sms) Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR, "%s: Erreur allocation m??moire", __func__ );
    else
     { g_snprintf( sms->user_phone, sizeof(sms->user_phone), "%s", db->row[5] );
       g_snprintf( sms->user_name,      sizeof(sms->user_name),      "%s", db->row[1] );
       g_snprintf( sms->user_comment,   sizeof(sms->user_comment),   "%s", db->row[3] );
       sms->user_id            = atoi(db->row[0]);
       sms->user_enable        = atoi(db->row[2]);
       sms->user_sms_enable    = atoi(db->row[4]);
       sms->user_sms_allow_cde = atoi(db->row[6]);
     }
    return(sms);
  }
/******************************************************************************************************************************/
/* Smsg_is_recipient_authorized : Renvoi TRUE si le telephone en parametre peut set ou reset un bit interne                   */
/* Entr??e: le nom du destinataire                                                                                             */
/* Sortie : bool??en, TRUE/FALSE                                                                                               */
/******************************************************************************************************************************/
 static struct SMSDB *Smsg_is_recipient_authorized ( gchar *tel )
  { struct SMSDB *sms;
    gchar *phone, requete[512];
    struct DB *db;

    phone = Normaliser_chaine ( tel );
    if (!phone)
     { Info_new( Config.log, Config.log_msrv, LOG_WARNING,
                "%s: Normalisation phone impossible", __func__ );
       return(NULL);
     }

    g_snprintf( requete, sizeof(requete),                                                                      /* Requete SQL */
                "SELECT id,username,enable,comment,sms_enable,sms_phone,sms_allow_cde "
                " FROM users as user WHERE enable=1 AND sms_allow_cde=1 AND sms_phone LIKE '%s'"
                " ORDER BY username LIMIT 1", phone );
    g_free(phone);

    db = Init_DB_SQL();
    if (!db)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_WARNING,
                "%s: Database Connection Failed", __func__ );
       return(NULL);
     }

    if ( Lancer_requete_SQL ( db, requete ) == FALSE )                                         /* Execution de la requete SQL */
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_WARNING,
                "%s: Requete failed", __func__ );
       Libere_DB_SQL( &db );
       return(NULL);
     }

    sms = Smsg_Recuperer_smsDB_suite( db );
    Libere_DB_SQL( &db );
    return(sms);
  }
/******************************************************************************************************************************/
/* Smsg_Send_CB: Appel?? par le t??l??phone quand le SMS est parti                                                               */
/* Entr??e: le message ?? envoyer sateur                                                                                        */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 static void Smsg_Send_CB (GSM_StateMachine *sm, int status, int MessageReference, void * user_data)
  {	if (status==0) {	sms_send_status = ERR_NONE; }
    else sms_send_status = ERR_UNKNOWN;
  }
/******************************************************************************************************************************/
/* Envoi_sms_gsm: Envoi un sms par le gsm                                                                                     */
/* Entr??e: le message ?? envoyer sateur                                                                                        */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 static gboolean Envoi_sms_gsm ( struct CMD_TYPE_MESSAGE *msg, gchar *telephone )
  { GSM_SMSMessage sms;
    GSM_Error error;
    GSM_SMSC PhoneSMSC;
    GSM_StateMachine *s;
    INI_Section *cfg;
    gint wait;

    GSM_InitLocales(NULL);
   	memset(&sms, 0, sizeof(sms));                                                                       /* Pr??paration du SMS */
	  	EncodeUnicode( sms.Text, msg->libelle_sms, strlen(msg->libelle_sms));                              /* Encode message text */
    EncodeUnicode( sms.Number, telephone, strlen(telephone));

	   sms.PDU = SMS_Submit;                                                                        /* We want to submit message */
	   sms.UDH.Type = UDH_NoUDH;                                                                 /* No UDH, just a plain message */
	   sms.Coding = SMS_Coding_Default_No_Compression;                                        /* We used default coding for text */
   	sms.Class = 1;                                                                                /* Class 1 message (normal) */


	   if ( (s = GSM_AllocStateMachine()) == NULL )                                                   /* Allocates state machine */
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR, "%s: AllocStateMachine Error", __func__ );
       return(FALSE);
     }

	/*debug_info = GSM_GetDebug(s);
	GSM_SetDebugGlobal(FALSE, debug_info);
	GSM_SetDebugFileDescriptor(stderr, TRUE, debug_info);
	GSM_SetDebugLevel("textall", debug_info);*/

	   error = GSM_FindGammuRC(&cfg, NULL);
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: FindGammuRC Failed (%s)", __func__, GSM_ErrorString(error) );
       if (GSM_IsConnected(s))	GSM_TerminateConnection(s);
     }

   	error = GSM_ReadConfig(cfg, GSM_GetConfig(s, 0), 0);
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: ReadConfig Failed (%s)", __func__, GSM_ErrorString(error) );
       if (GSM_IsConnected(s))	GSM_TerminateConnection(s);
     }

   	INI_Free(cfg);
   	GSM_SetConfigNum(s, 1);

   	error = GSM_InitConnection(s, 3);
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: InitConnection Failed (%s)", __func__, GSM_ErrorString(error) );
       if (GSM_IsConnected(s))	GSM_TerminateConnection(s);
     }

    GSM_SetSendSMSStatusCallback(s, Smsg_Send_CB, NULL);

	   PhoneSMSC.Location = 1;                                                                   	/* We need to know SMSC number */
   	error = GSM_GetSMSC(s, &PhoneSMSC);
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: GetSMSC Failed (%s)", __func__, GSM_ErrorString(error) );
       if (GSM_IsConnected(s))	GSM_TerminateConnection(s);
     }

	   CopyUnicodeString(sms.SMSC.Number, PhoneSMSC.Number);                                       /* Set SMSC number in message */

   	sms_send_status = ERR_TIMEOUT;
	   error = GSM_SendSMS(s, &sms); 	                                                                           /* Send message */
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: SendSMS Failed (%s)", __func__, GSM_ErrorString(error) );
       if (GSM_IsConnected(s))	GSM_TerminateConnection(s);
     }


    wait = Partage->top; 	                                                                          /* Wait for network reply */
	   while ( (Partage->top < wait+150) && sms_send_status == ERR_TIMEOUT )
     {	GSM_ReadDevice(s, TRUE); }

    if (sms_send_status == ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_INFO,
                "%s: Envoi SMS Ok to %s (%s)", __func__, telephone, msg->libelle_sms );
     }
    else
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_WARNING,
                "%s: Envoi SMS Nok to %s (error '%s')", __func__, telephone, GSM_ErrorString(error) ); }

   	error = GSM_TerminateConnection(s); 	                                                             /* Terminate connection */
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: TerminateConnection Failed (%s)", __func__, GSM_ErrorString(error) );
       if (GSM_IsConnected(s))	GSM_TerminateConnection(s);
     }


	   GSM_FreeStateMachine(s);                                                                          	/* Free up used memory */
    if (sms_send_status == ERR_NONE) { Cfg_smsg.nbr_sms++; return(TRUE); }
    else return(FALSE);
  }
/******************************************************************************************************************************/
/* Envoi_sms_smsbox: Envoi un sms par SMSBOX                                                                                  */
/* Entr??e: le message ?? envoyer sateur                                                                                        */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 static void Envoi_sms_smsbox ( struct CMD_TYPE_MESSAGE *msg, gchar *telephone )
  { gchar erreur[CURL_ERROR_SIZE+1];
    struct curl_httppost *formpost;
    struct curl_httppost *lastptr;
    CURLcode res;
    CURL *curl;

    formpost = lastptr = NULL;
    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "apikey",
                  CURLFORM_COPYCONTENTS, Cfg_smsg.smsbox_apikey,
                  CURLFORM_END);
/*    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "login",
                  CURLFORM_COPYCONTENTS, Cfg_smsg.smsbox_username,
                  CURLFORM_END);
    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "pass",
                  CURLFORM_COPYCONTENTS, Cfg_smsg.smsbox_password,
                  CURLFORM_END); */
    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "msg",
                  CURLFORM_COPYCONTENTS, msg->libelle_sms,
                  CURLFORM_END);
    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "charset",
                  CURLFORM_COPYCONTENTS, "utf-8",
                  CURLFORM_END);
    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "dest",
                  CURLFORM_COPYCONTENTS, telephone,
                  CURLFORM_END);
    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "strategy",
                  CURLFORM_COPYCONTENTS, "2",
                  CURLFORM_END);
    curl_formadd( &formpost, &lastptr,                              /* Pas de SMS les 2 premi??res minutes de vie du processus */
                  CURLFORM_COPYNAME,     "origine",                                 /* 'debugvar' pour lancer en mode semonce */
                  CURLFORM_COPYCONTENTS, VERSION,
/*                     CURLFORM_COPYCONTENTS, "debugvar",*/
                  CURLFORM_END);

    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "mode",
                  CURLFORM_COPYCONTENTS, "Standard",
                  CURLFORM_END);

    curl = curl_easy_init();
    if (curl)
     { curl_easy_setopt(curl, CURLOPT_URL, "https://api.smsbox.fr/1.1/api.php" );
       curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
       curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, erreur );
       curl_easy_setopt(curl, CURLOPT_VERBOSE, 1 );
       res = curl_easy_perform(curl);
       if (!res)
        { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_INFO,
                   "%s: Envoi SMS '%s' to '%s'", __func__, msg->libelle_sms, telephone );
          Cfg_smsg.nbr_sms++;
        }
       else
        { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_WARNING,
                   "%s: Envoi SMS Nok - Pb cURL (%s)", __func__, erreur);
        }
       curl_easy_cleanup(curl);
     }
    else
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_WARNING,
                "%s: Envoi SMS Nok - Pb cURL Init", __func__ );
     }
    curl_formfree(formpost);
  }
/******************************************************************************************************************************/
/* Smsg_send_to_all_authorized_recipients : Envoi ?? tous les portables autoris??s                                              */
/* Entr??e: le message                                                                                                         */
/* Sortie : n??ant                                                                                                             */
/******************************************************************************************************************************/
 void Smsg_send_to_all_authorized_recipients ( struct CMD_TYPE_MESSAGE *msg )
  { struct SMSDB *sms;
    struct DB *db;

    if (sending_is_disabled == TRUE)                                   /* Si envoi d??sactiv??, on sort de suite de la fonction */
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE, "%s: Sending is disabled. Dropping message", __func__ );
       return;
     }

    db = Init_DB_SQL();
    if (!db)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_WARNING, "%s: Database Connection Failed", __func__ );
       return;
     }

/********************************************* Chargement des informations en bases *******************************************/
    if ( ! Smsg_Recuperer_recipient_authorized_smsDB( db ) )
     { Libere_DB_SQL( &db );
       Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_WARNING, "%s: Recuperer_sms Failed", __func__ );
       return;
     }

    while ( (sms = Smsg_Recuperer_smsDB_suite( db )) != NULL)
     { switch (msg->sms)
        { case MSG_SMS_YES:
               if ( Envoi_sms_gsm ( msg, sms->user_phone ) == FALSE )
                { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                            "%s: Error sending with GSM. Falling back to SMSBOX", __func__ );
                  Envoi_sms_smsbox( msg, sms->user_phone );
                }
               break;
          case MSG_SMS_GSM_ONLY:
               Envoi_sms_gsm   ( msg, sms->user_phone );
               break;
          case MSG_SMS_SMSBOX_ONLY:
               Envoi_sms_smsbox( msg, sms->user_phone );
               break;
        }
       /*sleep(5);*/
     }

    Libere_DB_SQL( &db );
  }
/******************************************************************************************************************************/
/* Envoyer_sms: Envoi un sms                                                                                                  */
/* Entr??e: un texte au format UTF8 si possible                                                                                */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 void Envoyer_smsg_smsbox_text ( gchar *texte )
  { struct CMD_TYPE_MESSAGE msg;

    g_snprintf(msg.libelle_sms, sizeof(msg.libelle_sms), "%s", texte );
    msg.num    = 0;
    msg.enable = TRUE;
    msg.sms    = MSG_SMS_SMSBOX_ONLY;

    Smsg_send_to_all_authorized_recipients( &msg );
  }
/******************************************************************************************************************************/
/* Envoyer_sms: Envoi un sms                                                                                                  */
/* Entr??e: un texte au format UTF8 si possible                                                                                */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 void Envoyer_smsg_gsm_text ( gchar *texte )
  { struct CMD_TYPE_MESSAGE msg;

    g_snprintf(msg.libelle_sms, sizeof(msg.libelle_sms), "%s", texte );
    msg.num    = 0;
    msg.enable = TRUE;
    msg.sms    = MSG_SMS_GSM_ONLY;

    Smsg_send_to_all_authorized_recipients( &msg );
  }
/******************************************************************************************************************************/
/* Traiter_commande_sms: Fonction appel??e pour traiter la commande sms recu par le telephone                                  */
/* Entr??e: le message text ?? traiter                                                                                          */
/* Sortie : N??ant                                                                                                             */
/******************************************************************************************************************************/
 static void Traiter_commande_sms ( gchar *from, gchar *texte )
  { gboolean found = FALSE;
    struct SMSDB *sms;
    gchar chaine[160];
    struct DB *db;

    sms = Smsg_is_recipient_authorized ( from );
    if ( sms == NULL )
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE,
                "%s : unknown sender %s. Dropping message %s...", __func__, from, texte );
       return;
     }

    if ( ! strcasecmp( texte, "ping" ) )                                                               /* Interfacage de test */
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE, "%s: Ping Received from '%s'. Sending Pong", __func__, from );
       Envoyer_smsg_gsm_text ( "Pong !" );
       return;
     }

    if ( ! strcasecmp( texte, "smsoff" ) )                                                                      /* Smspanic ! */
     { sending_is_disabled = TRUE;
       Envoyer_smsg_gsm_text ( "Sending SMS is off !" );
       Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE, "%s: Sending SMS is DISABLED by '%s'", __func__, from );
       return;
     }

    if ( ! strcasecmp( texte, "smson" ) )                                                                       /* Smspanic ! */
     { Envoyer_smsg_gsm_text ( "Sending SMS is on !" );
       Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE, "%s: Sending SMS is ENABLED by '%s'", __func__, from );
       sending_is_disabled = FALSE;
       return;
     }

    if ( ! Recuperer_mnemos_DI_by_text ( &db, NOM_THREAD, texte ) )
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR, "%s: Error searching Database for '%s'", __func__, texte ); }
    else while ( Recuperer_mnemos_DI_suite( &db ) )
     { gchar *tech_id = db->row[0], *acro = db->row[1], *libelle = db->row[3], *src_text = db->row[2];
       Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE, "%s: Dyn Match found '%s' '%s:%s' - %s", __func__,
                 src_text, tech_id, acro, libelle );

       found=TRUE;
       if (Config.instance_is_master==TRUE)                                                       /* si l'instance est Maitre */
        { Envoyer_commande_dls_data ( tech_id, acro ); }
       else /* Envoi au master via thread HTTP */
        { JsonBuilder *builder;
          gchar *result;
          gsize taille;
          builder = Json_create ();
          json_builder_begin_object ( builder );
          Json_add_string ( builder, "tech_id", tech_id );
          Json_add_string ( builder, "acronyme", acro );
          Json_add_bool   ( builder, "etat", TRUE );
          json_builder_end_object ( builder );
          result = Json_get_buf ( builder, &taille );
          Send_zmq_with_tag ( Cfg_smsg.zmq_to_master, NULL, NOM_THREAD, "*", "msrv", "SET_CDE", result, taille );
          g_free(result);
        }
     }

    if (found)
         { g_snprintf(chaine, sizeof(chaine), "'%s' done.", texte ); }
    else { g_snprintf(chaine, sizeof(chaine), "'%s' not found in database.", texte ); }    /* Envoi de l'erreur si pas trouv?? */
    Envoyer_smsg_gsm_text ( chaine );
  }
/******************************************************************************************************************************/
/* Smsg_disconnect: Se deconnecte du telephone ou de la clef 3G                                                               */
/* Entr??e: Rien                                                                                                               */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 static void Smsg_disconnect ( void )
  { GSM_Error error;
   	error = GSM_TerminateConnection(s); 	                                                             /* Terminate connection */
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: TerminateConnection Failed (%s)", __func__, GSM_ErrorString(error) );
     }
    GSM_FreeStateMachine(s);                                                                          	/* Free up used memory */
    s = NULL;
    Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_DEBUG, "%s: Disconnected", __func__ );
  }
/******************************************************************************************************************************/
/* smsg_connect: Ouvre une connexion vers le t??l??phone ou la clef 3G                                                          */
/* Entr??e: Rien                                                                                                               */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 static gboolean Smsg_connect ( void )
  { GSM_Error error;

    GSM_InitLocales(NULL);
    if ( (s = GSM_AllocStateMachine()) == NULL )                                                   /* Allocates state machine */
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR, "%s: AllocStateMachine Error", __func__ );
       return(FALSE);
     }

	   error = GSM_FindGammuRC(&cfg, NULL);
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: FindGammuRC Failed (%s)", __func__, GSM_ErrorString(error) );
       if (GSM_IsConnected(s))	GSM_TerminateConnection(s);
   	   Smsg_disconnect();
       sleep(2);
       return(FALSE);
     }

   	error = GSM_ReadConfig(cfg, GSM_GetConfig(s, 0), 0);
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: ReadConfig Failed (%s)", __func__, GSM_ErrorString(error) );
       if (GSM_IsConnected(s))	GSM_TerminateConnection(s);
   	   Smsg_disconnect();
       sleep(2);
       return(FALSE);
     }

   	INI_Free(cfg);
   	GSM_SetConfigNum(s, 1);

   	error = GSM_InitConnection(s, 1);
	   if (error != ERR_NONE)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                "%s: InitConnection Failed (%s)", __func__, GSM_ErrorString(error) );
       if (GSM_IsConnected(s))	GSM_TerminateConnection(s);
   	   Smsg_disconnect();
       sleep(2);
       return(FALSE);
     }
    Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_DEBUG, "%s: Connection OK", __func__ );
    return(TRUE);
  }
/******************************************************************************************************************************/
/* Lire_sms_gsm: Lecture de tous les SMS du GSM                                                                               */
/* Entr??e: Rien                                                                                                               */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 static void Lire_sms_gsm ( void )
  { gchar from[80], texte[180];
    GSM_MultiSMSMessage sms;
    gboolean found = FALSE;
    GSM_Error error;

   	memset(&sms, 0, sizeof(sms));                                                                       /* Pr??paration du SMS */

    if (Smsg_connect()==FALSE) return;
/* Read all messages */
   	error = ERR_NONE;
	   sms.Number = 0;
	   sms.SMS[0].Location = 0;
	   sms.SMS[0].Folder = 0;
    error = GSM_GetNextSMS(s, &sms, TRUE);
    if (error == ERR_NONE)
     { gint i;
       for (i = 0; i < sms.Number; i++)
			     { g_snprintf( from, sizeof(from), "%s", DecodeUnicodeConsole(sms.SMS[i].Number) );
          g_snprintf( texte, sizeof(texte), "%s", DecodeUnicodeConsole(sms.SMS[i].Text) );
          sms.SMS[0].Folder = 0;/* https://github.com/gammu/gammu/blob/ed2fec4a382e7ac4b5dfc92f5b10811f76f4817e/gammu/message.c */
          error = GSM_DeleteSMS( s, &sms.SMS[i] );
          if (error != ERR_NONE)
           { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                      "%s: Delete '%s' from '%s' Location %d/%d Folder %d Failed ('%s')!", __func__,
                       texte, from, i, sms.SMS[i].Location, sms.SMS[i].Folder, GSM_ErrorString(error) );
           }
          else
           { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_DEBUG,
                      "%s: Delete '%s' from '%s' Location %d/%d Folder %d OK !", __func__,
                       texte, from, i, sms.SMS[i].Location, sms.SMS[i].Folder );
           }
          if (sms.SMS[i].State == SMS_UnRead)                                /* Pour tout nouveau message, nous le processons */
           { found = TRUE; break; }
         }
	     }
     else if (error != ERR_EMPTY)
      { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_ERR,
                  "%s: No sms received ('%s')!", __func__, GSM_ErrorString(error) );
      }
    Smsg_disconnect();                                                                                	/* Free up used memory */
    if (found) Traiter_commande_sms ( from, texte );

    if ( (error == ERR_NONE) || (error == ERR_EMPTY) )
         { Smsg_send_status_to_master( TRUE  ); }
    else { Smsg_send_status_to_master( FALSE ); }
  }
/******************************************************************************************************************************/
/* Envoyer_sms: Envoi un sms                                                                                                  */
/* Entr??e: un client et un utilisateur                                                                                        */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 void Run_thread ( struct LIBRAIRIE *lib )
  { struct CMD_TYPE_HISTO *histo, histo_buf;
    struct ZMQUEUE *zmq_msg;
    struct ZMQUEUE *zmq_from_bus;

    prctl(PR_SET_NAME, "W-SMSG", 0, 0, 0 );
    memset( &Cfg_smsg, 0, sizeof(Cfg_smsg) );                                        /* Mise a zero de la structure de travail */
    Cfg_smsg.lib = lib;                                             /* Sauvegarde de la structure pointant sur cette librairie */
    Cfg_smsg.lib->TID = pthread_self();                                                      /* Sauvegarde du TID pour le pere */
    Smsg_Lire_config ();                                                     /* Lecture de la configuration logiciel du thread */

    Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE,
              "%s: Demarrage %s . . . TID = %p (thread %s)", __func__, VERSION, pthread_self(), NOM_THREAD );
    Cfg_smsg.lib->Thread_run = TRUE;                                                                     /* Le thread tourne ! */

    g_snprintf( Cfg_smsg.lib->admin_prompt, sizeof(Cfg_smsg.lib->admin_prompt), NOM_THREAD );
    g_snprintf( Cfg_smsg.lib->admin_help,   sizeof(Cfg_smsg.lib->admin_help),   "Manage SMS system (libgammu)" );

    if (!Cfg_smsg.enable)
     { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE,
                "%s: Thread is not enabled in config. Shutting Down %p", __func__, pthread_self() );
       goto end;
     }

    zmq_msg                = Connect_zmq ( ZMQ_SUB, "listen-to-msgs", "inproc", ZMQUEUE_LIVE_MSGS, 0 );
    zmq_from_bus           = Connect_zmq ( ZMQ_SUB, "listen-to-bus",  "inproc", ZMQUEUE_LOCAL_BUS, 0 );
    Cfg_smsg.zmq_to_master = Connect_zmq ( ZMQ_PUB, "pub-to-master",  "inproc", ZMQUEUE_LOCAL_MASTER, 0 );

    Envoyer_smsg_gsm_text ( "SMS System is running" );
    sending_is_disabled = FALSE;                                                     /* A l'init, l'envoi de SMS est autoris?? */
    while(Cfg_smsg.lib->Thread_run == TRUE)                                                  /* On tourne tant que necessaire */
     { struct ZMQ_TARGET *event;
       gchar buffer[256];
       void *payload;

       if (Cfg_smsg.lib->Thread_reload)                                                      /* A-t'on recu un signal USR1 ? */
        { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_INFO, "%s: Thread Reload !", __func__ );
          Smsg_Lire_config();
          Cfg_smsg.lib->Thread_reload = FALSE;
        }


/****************************************************** Lecture de SMS ********************************************************/
       Lire_sms_gsm();

/********************************************************* Envoi de SMS *******************************************************/
       if (Recv_zmq_with_tag ( zmq_from_bus, NOM_THREAD, &buffer, sizeof(buffer), &event, &payload ) > 0) /* Reception d'un paquet master ? */
        { if ( !strcmp( event->tag, "send_smsbox" ) )   { Envoyer_smsg_smsbox_text ( payload ); }
          else if ( !strcmp( event->tag, "send_sms" ) ) { Envoyer_smsg_gsm_text ( payload ); }
        }

       while ( Cfg_smsg.lib->Thread_run == TRUE &&
               Recv_zmq ( zmq_msg, &histo_buf, sizeof(struct CMD_TYPE_HISTO) ) == sizeof(struct CMD_TYPE_HISTO) )
        { histo = &histo_buf;

          if ( histo && histo->alive == TRUE && histo->msg.sms != MSG_SMS_NONE)             /* On n'envoie que si MSGnum == 1 */
           { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE,
                      "%s : Sending msg %d (%s)", __func__, histo->msg.num, histo->msg.libelle_sms );

/*************************************************** Envoi en mode GSM ********************************************************/
             if (Partage->top < TOP_MIN_ENVOI_SMS)
              { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_INFO,
                         "%s: Envoi trop tot !! (%s)", __func__, histo->msg.libelle_sms );
                sleep(1);
              }
             else
              { Smsg_send_to_all_authorized_recipients( &histo->msg ); }
           }
          else
           { Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_DEBUG,
                       "%s : msg %d not sent (alive=%d, msg.sms = %d) (%s)", __func__,
                       histo->msg.num, histo->alive, histo->msg.sms, histo->msg.libelle_sms );
           }
        }
     }
    Smsg_send_status_to_master( FALSE );
    Close_zmq ( zmq_msg );
    Close_zmq ( zmq_from_bus );
    Close_zmq ( Cfg_smsg.zmq_to_master );

end:
    Info_new( Config.log, Cfg_smsg.lib->Thread_debug, LOG_NOTICE, "%s: Down . . . TID = %p", __func__, pthread_self() );
    Cfg_smsg.lib->Thread_run = FALSE;
    Cfg_smsg.lib->TID = 0;                                                    /* On indique au master que le thread est mort. */
    pthread_exit(GINT_TO_POINTER(0));
  }
/*----------------------------------------------------------------------------------------------------------------------------*/
