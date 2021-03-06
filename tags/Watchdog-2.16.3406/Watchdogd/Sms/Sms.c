/******************************************************************************************************************************/
/* Watchdogd/sms.c        Gestion des SMS de Watchdog v2.0                                                                    */
/* Projet WatchDog version 2.0       Gestion d'habitat                                       ven. 02 avril 2010 20:37:40 CEST */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * Sms.c
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
 
 #include <sys/time.h>
 #include <sys/prctl.h>
 #include <string.h>
 #include <unistd.h>
 #include <gnokii.h>
 #include <curl/curl.h>

/**************************************************** Prototypes de fonctions *************************************************/
 #include "watchdogd.h"
 #include "Sms.h"

 static gboolean sending_is_disabled = FALSE;                      /* Variable permettant d'interdire l'envoi de sms si panic */

/******************************************************************************************************************************/
/* Sms_Lire_config : Lit la config Watchdog et rempli la structure m??moire                                                    */
/* Entr??e: le pointeur sur la LIBRAIRIE                                                                                       */
/* Sortie: N??ant                                                                                                              */
/******************************************************************************************************************************/
 gboolean Sms_Lire_config ( void )
  { gchar *nom, *valeur;
    struct DB *db;

    Cfg_sms.lib->Thread_debug = FALSE;                                                         /* Settings default parameters */
    Cfg_sms.enable            = FALSE; 
    g_snprintf( Cfg_sms.smsbox_apikey, sizeof(Cfg_sms.smsbox_apikey),
               "%s", DEFAUT_SMSBOX_APIKEY );

    if ( ! Recuperer_configDB( &db, NOM_THREAD ) )                                          /* Connexion a la base de donn??es */
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Database connexion failed. Using Default Parameters", __func__ );
       return(FALSE);
     }

    while (Recuperer_configDB_suite( &db, &nom, &valeur ) )                           /* R??cup??ration d'une config dans la DB */
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_INFO,                                             /* Print Config */
                "%s: '%s' = %s", __func__, nom, valeur );
            if ( ! g_ascii_strcasecmp ( nom, "smsbox_apikey" ) )
        { g_snprintf( Cfg_sms.smsbox_apikey, sizeof(Cfg_sms.smsbox_apikey), "%s", valeur ); }
       else if ( ! g_ascii_strcasecmp ( nom, "bit_comm" ) )
        { Cfg_sms.bit_comm = atoi ( valeur ); }
       else if ( ! g_ascii_strcasecmp ( nom, "enable" ) )
        { if ( ! g_ascii_strcasecmp( valeur, "true" ) ) Cfg_sms.enable = TRUE;  }
       else if ( ! g_ascii_strcasecmp ( nom, "debug" ) )
        { if ( ! g_ascii_strcasecmp( valeur, "true" ) ) Cfg_sms.lib->Thread_debug = TRUE;  }
       else
        { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_NOTICE,
                   "%s: Unknown Parameter '%s'(='%s') in Database", __func__, nom, valeur );
        }
     }
    return(TRUE);
  }
/******************************************************************************************************************************/
/* Recuperer_smsDB: r??cup??re la liste des utilisateurs et de leur num??ro de t??l??phone                                         */
/* Entr??e: une structure DB                                                                                                   */
/* Sortie: FALSE si pb                                                                                                        */
/******************************************************************************************************************************/
 gboolean Sms_Recuperer_smsDB ( struct DB *db )
  { gchar requete[512];

    g_snprintf( requete, sizeof(requete),                                                                      /* Requete SQL */
                "SELECT id,name,enable,comment,sms_enable,sms_phone,sms_allow_cde "
                " FROM %s as user ORDER BY user.name",
                NOM_TABLE_UTIL );

    return ( Lancer_requete_SQL ( db, requete ) );                                             /* Execution de la requete SQL */
  }
/******************************************************************************************************************************/
/* Recuperer_recipient_authorized: Recupere la liste des users habilit?? a recevoir des SMS                                    */
/* Entr??e: une structure DB                                                                                                   */
/* Sortie: FALSE si pb                                                                                                        */
/******************************************************************************************************************************/
 static gboolean Sms_Recuperer_recipient_authorized_smsDB ( struct DB *db )
  { gchar requete[512];

    g_snprintf( requete, sizeof(requete),                                                                      /* Requete SQL */
                "SELECT id,name,enable,comment,sms_enable,sms_phone,sms_allow_cde "
                " FROM %s as user WHERE enable=1 AND sms_enable=1 ORDER BY user.name",
                NOM_TABLE_UTIL );

    return ( Lancer_requete_SQL ( db, requete ) );                                             /* Execution de la requete SQL */
  }
/******************************************************************************************************************************/
/* Recuperer_liste_id_smsDB: Recup??ration de la liste des ids des smss                                                        */
/* Entr??e: une structure DB                                                                                                   */
/* Sortie: FALSE si pb                                                                                                        */
/******************************************************************************************************************************/
 struct SMSDB *Sms_Recuperer_smsDB_suite( struct DB *db )
  { struct SMSDB *sms;

    Recuperer_ligne_SQL(db);                                                               /* Chargement d'une ligne resultat */
    if ( ! db->row )
     { Liberer_resultat_SQL ( db );
       return(NULL);
     }

    sms = (struct SMSDB *)g_try_malloc0( sizeof(struct SMSDB) );
    if (!sms) Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_ERR, "%s: Erreur allocation m??moire", __func__ );
    else
     { g_snprintf( sms->user_sms_phone, sizeof(sms->user_sms_phone), "%s", db->row[5] );
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
/* Envoyer_sms: Envoi un sms                                                                                                  */
/* Entr??e: un texte au format UTF8 si possible                                                                                */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 void Envoyer_sms_smsbox_text ( gchar *texte )
  { struct CMD_TYPE_HISTO *histo;

    histo = (struct CMD_TYPE_HISTO *) g_try_malloc0( sizeof(struct CMD_TYPE_HISTO) );
    if (!histo) { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_ERR,
                           "%s: pas assez de m??moire pour copie", __func__ );
                  return;
                }

    g_snprintf(histo->msg.libelle_sms, sizeof(histo->msg.libelle_sms), "%s", texte );
    histo->id         = 0;
    histo->alive      = TRUE;
    histo->msg.num    = 0;
    histo->msg.enable = TRUE;
    histo->msg.sms    = MSG_SMS_SMSBOX_ONLY;

    pthread_mutex_lock( &Cfg_sms.lib->synchro );
    Cfg_sms.Liste_histos = g_slist_append ( Cfg_sms.Liste_histos, histo );
    pthread_mutex_unlock( &Cfg_sms.lib->synchro );
  }
/******************************************************************************************************************************/
/* Envoyer_sms: Envoi un sms                                                                                                  */
/* Entr??e: un texte au format UTF8 si possible                                                                                */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 void Envoyer_sms_gsm_text ( gchar *texte )
  { struct CMD_TYPE_HISTO *histo;

    histo = (struct CMD_TYPE_HISTO *) g_try_malloc0( sizeof(struct CMD_TYPE_HISTO) );
    if (!histo) { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_ERR,
                           "%s: pas assez de m??moire pour copie", __func__ );
                  return;
                }

    g_snprintf(histo->msg.libelle_sms, sizeof(histo->msg.libelle_sms), "%s", texte );
    histo->id         = 0;
    histo->alive      = TRUE;
    histo->msg.num    = 0;
    histo->msg.enable = TRUE;
    histo->msg.sms    = MSG_SMS_GSM_ONLY;

    pthread_mutex_lock( &Cfg_sms.lib->synchro );
    Cfg_sms.Liste_histos = g_slist_append ( Cfg_sms.Liste_histos, histo );
    pthread_mutex_unlock( &Cfg_sms.lib->synchro );
  }
/******************************************************************************************************************************/
/* Sms_Gerer_histo: Fonction d'abonn?? appell?? lorsqu'un message est disponible.                                               */
/* Entr??e: une structure CMD_TYPE_HISTO                                                                                       */
/* Sortie : N??ant                                                                                                             */
/******************************************************************************************************************************/
 static void Sms_Gerer_histo ( struct CMD_TYPE_HISTO *histo )
  { gsize bytes_written;
    gint taille;
    
    if ( ! histo->msg.sms ) { g_free(histo); return; }                                       /* Si flag = 0; on return direct */

    pthread_mutex_lock( &Cfg_sms.lib->synchro );                                             /* Ajout dans la liste a traiter */
    taille = g_slist_length( Cfg_sms.Liste_histos );
    pthread_mutex_unlock( &Cfg_sms.lib->synchro );

    if (taille > 150)
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: DROP message %d (length = %d > 150)", __func__, histo->msg.num, taille);
       g_free(histo);
       return;
     }
    else if (Cfg_sms.lib->Thread_run == FALSE)
     { Info_new( Config.log, Config.log_arch, LOG_INFO,
                "%s: Thread is down. Dropping msg %d", __func__, histo->msg.num );
       g_free(histo);
       return;
     }
/* conversion en Latin 1 ISO8859-1 20140626 -- Abandon 20171214 en passant par API 1.1 smsbox */
/*    new_libelle = g_convert ( histo->msg.libelle, -1, "ISO-8859-1", "UTF-8", NULL, &bytes_written, NULL);*/
    g_snprintf( histo->msg.libelle, sizeof(histo->msg.libelle), "%s", histo->msg.libelle );
    pthread_mutex_lock ( &Cfg_sms.lib->synchro );
    Cfg_sms.Liste_histos = g_slist_append ( Cfg_sms.Liste_histos, histo );                                /* Ajout a la liste */
    pthread_mutex_unlock ( &Cfg_sms.lib->synchro );
  }
/******************************************************************************************************************************/
/* Sms_is_recipient_authorized : Renvoi TRUE si le telephone en parametre peut set ou reset un bit interne                    */
/* Entr??e: le nom du destinataire                                                                                             */
/* Sortie : bool??en, TRUE/FALSE                                                                                               */
/******************************************************************************************************************************/
 static struct SMSDB *Sms_is_recipient_authorized ( gchar *tel )
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
                "SELECT id,name,enable,comment,sms_enable,sms_phone,sms_allow_cde "
                " FROM %s as user WHERE enable=1 AND sms_allow_cde=1 AND sms_phone LIKE '%s'"
                " ORDER BY user.name LIMIT 1",
                NOM_TABLE_UTIL, phone );
    g_free(phone);

    db = Init_DB_SQL();       
    if (!db)
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Database Connection Failed", __func__ );
       return(NULL);
     }

    if ( Lancer_requete_SQL ( db, requete ) == FALSE )                                         /* Execution de la requete SQL */
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Requete failed", __func__ );
       Libere_DB_SQL( &db );
       return(NULL);
     }
 
    sms = Sms_Recuperer_smsDB_suite( db );
    Libere_DB_SQL( &db );
    return(sms);
  }
/******************************************************************************************************************************/
/* Traiter_commande_sms: Fonction appel??e pour traiter la commande sms recu par le telephone                                  */
/* Entr??e: le message text ?? traiter                                                                                          */
/* Sortie : N??ant                                                                                                             */
/******************************************************************************************************************************/
 static void Traiter_commande_sms ( gchar *from, gchar *texte )
  { struct CMD_TYPE_MNEMO_BASE *mnemo;
    struct SMSDB *sms;
    gchar chaine[160];
    gint nbr;

    sms = Sms_is_recipient_authorized ( from );
    if ( sms == NULL )
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_NOTICE,
                "%s : unknown sender %s. Dropping message %s...", __func__, from, texte );
       return;
     }
     
    Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_NOTICE,
             "%s : Received %s from %s(%s). Processing...", __func__,
              texte, sms->user_name, sms->user_sms_phone );
    g_free(sms);
    g_snprintf(chaine, sizeof(chaine), "Processing: %s", texte );                           /* Envoi de l'acquit de reception */
    Envoyer_sms_gsm_text ( chaine );

    if ( ! strcasecmp( texte, "ping" ) )                                                               /* Interfacage de test */
     { Envoyer_sms_gsm_text ( "Pong !" );
       return;
     }

    if ( ! strcasecmp( texte, "smsoff" ) )                                                                      /* Smspanic ! */
     { sending_is_disabled = TRUE;
       Envoyer_sms_gsm_text ( "Sending SMS is off !" );
       Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_NOTICE, "%s: Sending SMS is DISABLED", __func__ );
       return;
     }

    if ( ! strcasecmp( texte, "smson" ) )                                                                       /* Smspanic ! */
     { Envoyer_sms_gsm_text ( "Sending SMS is on !" );
       Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_NOTICE, "%s: Sending SMS is ENABLED", __func__ );
       sending_is_disabled = FALSE;
       return;
     }

    g_snprintf(chaine, sizeof(chaine), "%s:%s:%s", Config.instance_id, NOM_THREAD, texte );             /* Recherche du mnemo */
    mnemo = Map_event_to_mnemo( chaine, &nbr );
    if (nbr==0)
     { g_snprintf(chaine, sizeof(chaine), "No event found for '%s'", texte );              /* Envoi de l'erreur si pas trouv?? */
       Envoyer_sms_gsm_text ( chaine );
       return;
     }
     
    if (nbr>1)
     { g_snprintf(chaine, sizeof(chaine), "Too many events found for '%s'", texte );             /* Envoi de l'erreur si trop */
       Envoyer_sms_gsm_text ( chaine );
       g_free(mnemo);
       return;
     }

    if (Config.instance_is_master==TRUE)                                                          /* si l'instance est Maitre */
     { switch( mnemo->type )
        { case MNEMO_MONOSTABLE:
               Info_new( Config.log, Config.log_msrv, LOG_NOTICE,
                         "%s: From %s -> Mise ?? un du bit M%03d", __func__, from, mnemo->num );
               Envoyer_commande_dls(mnemo->num);
               break;
          default:
          Info_new( Config.log, Config.log_msrv, LOG_ERR,
                    "%s: From %s -> Error, type of mnemo not handled", __func__, from );
        }
     }
    else /* Envoi au master vi thread HTTP */
     {
     }
    g_free(mnemo);
  }
/******************************************************************************************************************************/
/* Lire_sms_gsm: Lecture de tous les SMS du GSM                                                                               */
/* Entr??e: Rien                                                                                                               */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 static void Lire_sms_gsm ( void )
  { struct gn_statemachine *state;
    gn_sms_folder folder;
    gn_sms_folder_list folderlist;
    gn_error error;
    gn_data data;
    gn_sms sms;
    gint sms_index;

    if ((error=gn_lib_phoneprofile_load("", &state)) != GN_ERR_NONE)                                      /* Read config file */
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Read Phone profile NOK (%s)", __func__, gn_error_print(error) );
       if (Cfg_sms.bit_comm) SB ( Cfg_sms.bit_comm, 0 );
       return;
     }

    if ((error=gn_lib_phone_open(state)) != GN_ERR_NONE)
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Open Phone NOK (%s)", __func__, gn_error_print(error) );
       if (Cfg_sms.bit_comm) SB ( Cfg_sms.bit_comm, 0 );
       gn_lib_phone_close(state);
       gn_lib_phoneprofile_free(&state);
       gn_lib_library_free();
       return;
     }

    gn_data_clear(&data);

    folder.folder_id = 0;
    data.sms_folder_list = &folderlist;
    data.sms_folder = &folder;

    memset ( &sms, 0, sizeof(gn_sms) );
    sms.memory_type = gn_str2memory_type("ME");                           /* On recupere les SMS du Mobile equipment (pas SM) */
    data.sms = &sms;

    for (sms_index=1; ;sms_index++)
     { sms.number = sms_index;

       if ((error = gn_sms_get (&data, state)) == GN_ERR_NONE)                                          /* On recupere le SMS */
        { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_NOTICE,
                   "%s: Recu SMS %s de %s", __func__, (gchar *)sms.user_data[0].u.text, sms.remote.number );
          Traiter_commande_sms ( sms.remote.number, (gchar *)sms.user_data[0].u.text );
          gn_sms_delete (&data, state);                                                   /* On l'a trait??, on peut l'effacer */
        }
       else if (error == GN_ERR_INVALIDLOCATION) break;                           /* On regarde toutes les places de stockage */
       else  { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_DEBUG,
                        "%s: error %s from %s (sms_index=%d)", __func__,
                        gn_error_print(error), sms.remote.number, sms_index );
               break;
             }
     }

    gn_lib_phone_close(state);
    gn_lib_phoneprofile_free(&state);
    gn_lib_library_free();
    if (Cfg_sms.bit_comm) SB ( Cfg_sms.bit_comm, 1 );                                                     /* Communication OK */
  }
/******************************************************************************************************************************/
/* Envoi_sms_gsm: Envoi un sms par le gsm                                                                                     */
/* Entr??e: le message ?? envoyer sateur                                                                                        */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 static gboolean Envoi_sms_gsm ( struct CMD_TYPE_MESSAGE *msg, gchar *telephone )
  { struct gn_statemachine *state;
    gn_error error;
    gn_data data;
    gn_sms sms;

    if ((error=gn_lib_phoneprofile_load("", &state)) != GN_ERR_NONE)                                      /* Read config file */
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Read Phone profile NOK (%s)", __func__, gn_error_print(error) );
       return(FALSE);
     }

    if ((error=gn_lib_phone_open(state)) != GN_ERR_NONE)
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Open Phone NOK (%s)", __func__, gn_error_print(error) );
       return(FALSE);
     }

    gn_data_clear(&data);

    gn_sms_default_submit(&sms);                                                                 /* The memory is zeroed here */

    memset(&sms.remote.number, 0, sizeof(sms.remote.number));
    strncpy(sms.remote.number, telephone, sizeof(sms.remote.number) - 1);                                   /* Number a m'man */
    if (sms.remote.number[0] == '+') 
         { sms.remote.type = GN_GSM_NUMBER_International; }
    else { sms.remote.type = GN_GSM_NUMBER_Unknown; }

    if (!sms.smsc.number[0])                                                                          /* R??cup??ration du SMSC */
     { data.message_center = g_try_malloc0(sizeof(gn_sms_message_center));
       if (data.message_center)
       { data.message_center->id = 1;
          if (gn_sm_functions(GN_OP_GetSMSCenter, &data, state) == GN_ERR_NONE)
           { strcpy(sms.smsc.number, data.message_center->smsc.number);
             sms.smsc.type = data.message_center->smsc.type;
           }
          else
           { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING, "%s: Pb avec le SMSC", __func__ ); }
          g_free(data.message_center);
       }
      else { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_ERR, "%s: Memory Alloc Error", __func__ ); }
     }

    if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

    sms.user_data[0].length = g_snprintf( (gchar *)sms.user_data[0].u.text, sizeof (sms.user_data[0].u.text),
                                          "%s", msg->libelle_sms );
        
    sms.user_data[0].type = GN_SMS_DATA_Text;
    if (!gn_char_def_alphabet(sms.user_data[0].u.text))
     { sms.dcs.u.general.alphabet = GN_SMS_DCS_8bit; }
                                                                      /* 18/08/12 Test de passage en '8bit' au lieu de 'UCS2' */

    sms.user_data[1].type = GN_SMS_DATA_None;
/*	sms.delivery_report = true; */
    data.sms = &sms;                                                                                          /* Envoi du SMS */

    error = gn_sms_send(&data, state);
    if (error == GN_ERR_NONE)
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_INFO, "%s: Envoi SMS Ok to %s (%s)", __func__,
                 telephone, msg->libelle_sms ); }
    else
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Envoi SMS Nok to %s (%s)", __func__, telephone, gn_error_print(error)); }

    gn_lib_phone_close(state);
    gn_lib_phoneprofile_free(&state);
    gn_lib_library_free();
    sleep(5);                                                             /* Attente de 5 secondes pour ne pas saturer le GSM */
    if (error == GN_ERR_NONE) return(TRUE);
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
                  CURLFORM_COPYCONTENTS, Cfg_sms.smsbox_apikey,
                  CURLFORM_END); 
/*    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "login",
                  CURLFORM_COPYCONTENTS, Cfg_sms.smsbox_username,
                  CURLFORM_END); 
    curl_formadd( &formpost, &lastptr,
                  CURLFORM_COPYNAME,     "pass",
                  CURLFORM_COPYCONTENTS, Cfg_sms.smsbox_password,
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
        { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_INFO,
                   "%s: Envoi SMS '%s' to '%s'", __func__, msg->libelle_sms, telephone );
        }
       else
        { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                   "%s: Envoi SMS Nok - Pb cURL (%s)", __func__, erreur);
        }
       curl_easy_cleanup(curl);
     }
    else
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Envoi SMS Nok - Pb cURL Init", __func__ );
     }
    curl_formfree(formpost);
  }
/******************************************************************************************************************************/
/* Sms_send_to_all_authorized_recipients : Envoi ?? tous les portables autoris??s                                               */
/* Entr??e: le message                                                                                                         */
/* Sortie : n??ant                                                                                                             */
/******************************************************************************************************************************/
 static void Sms_send_to_all_authorized_recipients ( struct CMD_TYPE_MESSAGE *msg )
  { struct SMSDB *sms;
    struct DB *db;

    if (sending_is_disabled == TRUE) return;                           /* Si envoi d??sactiv??, on sort de suite de la fonction */

    db = Init_DB_SQL();       
    if (!db)
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Database Connection Failed", __func__ );
       return;
     }

/********************************************* Chargement des informations en bases *******************************************/
    if ( ! Sms_Recuperer_recipient_authorized_smsDB( db ) )
     { Libere_DB_SQL( &db );
       Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_WARNING,
                "%s: Recuperer_sms Failed", __func__ );
       return;
     }

    while ( (sms = Sms_Recuperer_smsDB_suite( db )) != NULL)
     { switch (msg->sms)
        { case MSG_SMS_YES:
               if ( Envoi_sms_gsm   ( msg, sms->user_sms_phone ) == FALSE )
                { Envoi_sms_smsbox( msg, sms->user_sms_phone ); }
               break;
          case MSG_SMS_GSM_ONLY:
               Envoi_sms_gsm   ( msg, sms->user_sms_phone );
               break;
          case MSG_SMS_SMSBOX_ONLY:
               Envoi_sms_smsbox( msg, sms->user_sms_phone );
               break;
        }
       sleep(5);
     }

    Libere_DB_SQL( &db );
  }
/******************************************************************************************************************************/
/* Envoyer_sms: Envoi un sms                                                                                                  */
/* Entr??e: un client et un utilisateur                                                                                        */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 void Run_thread ( struct LIBRAIRIE *lib )
  { struct CMD_TYPE_HISTO *histo;
    
    prctl(PR_SET_NAME, "W-SMS", 0, 0, 0 );
    memset( &Cfg_sms, 0, sizeof(Cfg_sms) );                                         /* Mise a zero de la structure de travail */
    Cfg_sms.lib = lib;                                             /* Sauvegarde de la structure pointant sur cette librairie */
    Cfg_sms.lib->TID = pthread_self();                                                      /* Sauvegarde du TID pour le pere */
    Sms_Lire_config ();                                                     /* Lecture de la configuration logiciel du thread */

    Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_NOTICE,
              "%s: Demarrage . . . TID = %p", __func__, pthread_self() );
    Cfg_sms.lib->Thread_run = TRUE;                                                                     /* Le thread tourne ! */

    g_snprintf( Cfg_sms.lib->admin_prompt, sizeof(Cfg_sms.lib->admin_prompt), NOM_THREAD );
    g_snprintf( Cfg_sms.lib->admin_help,   sizeof(Cfg_sms.lib->admin_help),   "Manage SMS system" );

    if (!Cfg_sms.enable)
     { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_NOTICE,
                "%s: Thread is not enabled in config. Shutting Down %p", __func__, pthread_self() );
       goto end;
     }

    Abonner_distribution_histo ( Sms_Gerer_histo );                                 /* Abonnement ?? la diffusion des messages */
    sending_is_disabled = FALSE;                                                     /* A l'init, l'envoi de SMS est autoris?? */
    while(Cfg_sms.lib->Thread_run == TRUE)                                                   /* On tourne tant que necessaire */
     { usleep(10000);
       sched_yield();

       if (Cfg_sms.lib->Thread_sigusr1)                                                       /* A-t'on recu un signal USR1 ? */
        { int nbr;

          Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_INFO, "%s: SIGUSR1", __func__ );
          pthread_mutex_lock( &Cfg_sms.lib->synchro );                             /* On recupere le nombre de sms en attente */
          nbr = g_slist_length(Cfg_sms.Liste_histos);
          pthread_mutex_unlock( &Cfg_sms.lib->synchro );
          Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_INFO, "%s: Nbr SMS a envoyer = %d", __func__, nbr );
          Cfg_sms.lib->Thread_sigusr1 = FALSE;
        }

       if (Cfg_sms.reload)                                       /* Prise en compte des reload conf depuis la console d'admin */
        { Cfg_sms.reload = FALSE;
          Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_INFO, "%s: Reloading conf", __func__ );
          Sms_Lire_config ();                                               /* Lecture de la configuration logiciel du thread */
        }          
/****************************************************** Lecture de SMS ********************************************************/
       Lire_sms_gsm();

/********************************************************* Envoi de SMS *******************************************************/
       sleep(5);
       if ( !Cfg_sms.Liste_histos )                                                         /* Attente de demande d'envoi SMS */
        { sched_yield();
          continue;
        }

       pthread_mutex_lock( &Cfg_sms.lib->synchro );
       histo = Cfg_sms.Liste_histos->data;
       Cfg_sms.Liste_histos = g_slist_remove ( Cfg_sms.Liste_histos, histo );
       Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_INFO,
                "%s: Reste %d a envoyer apres le msg %d", __func__,
                 g_slist_length(Cfg_sms.Liste_histos), histo->msg.num );
       pthread_mutex_unlock( &Cfg_sms.lib->synchro );
       if ( histo->alive == TRUE )                                                          /* On n'envoie que si MSGnum == 1 */
        { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_INFO,
                   "%s : Sending msg %d (%s)", __func__, histo->msg.num, histo->msg.libelle_sms );
      
/*************************************************** Envoi en mode GSM ********************************************************/
          if (Partage->top < TOP_MIN_ENVOI_SMS)
           { Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_INFO,
                      "%s: Envoi trop tot !! (%s)", __func__, histo->msg.libelle_sms ); }
          else 
           { Sms_send_to_all_authorized_recipients( &histo->msg ); }
        }
       g_free( histo );
     }
    Desabonner_distribution_histo ( Sms_Gerer_histo );                          /* Desabonnement de la diffusion des messages */

end:
    if (Cfg_sms.bit_comm) SB ( Cfg_sms.bit_comm, 0 );                                /* Communication NOK */
    Info_new( Config.log, Cfg_sms.lib->Thread_debug, LOG_NOTICE, "%s: Down . . . TID = %p", __func__, pthread_self() );
    Cfg_sms.lib->Thread_run = FALSE;
    Cfg_sms.lib->TID = 0;                                                     /* On indique au master que le thread est mort. */
    pthread_exit(GINT_TO_POINTER(0));
  }
/*----------------------------------------------------------------------------------------------------------------------------*/
