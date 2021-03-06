/**********************************************************************************************************/
/* Watchdog/db.c          Gestion des connexions ? la base de donn?es                                     */
/* Projet WatchDog version 2.0       Gestion d'habitat                      sam 18 avr 2009 00:44:37 CEST */
/* Auteur: LEFEVRE Sebastien                                                                              */
/**********************************************************************************************************/
/*
 * db.c
 * This file is part of Watchdog
 *
 * Copyright (C) 2010 - Sebastien LEFEVRE
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

 #include <glib.h>
 #include <string.h>

/******************************************** Chargement des prototypes ***********************************/
 #include "watchdogd.h"

/**********************************************************************************************************/
/* Normaliser_chaine: Normalise les chaines ( remplace ' par \', " par "" )                               */
/* Entr?es: un commentaire (gchar *)                                                                      */
/* Sortie: boolean false si probleme                                                                      */
/**********************************************************************************************************/
 gchar *Normaliser_chaine( gchar *pre_comment )
  { gchar *comment, *source, *cible;
    gunichar car;

    g_utf8_validate( pre_comment, -1, NULL );                                       /* Validate la chaine */
    comment = g_try_malloc0( (2*g_utf8_strlen(pre_comment, -1))*6 + 1 );  /* Au pire, ts les car sont doubl?s */
                                                                                  /* *6 pour gerer l'utf8 */
    if (!comment)
     { Info_new( Config.log, Config.log_db, LOG_WARNING, "Normaliser_chaine: memory error %s", pre_comment );
       return(NULL);
     }
    source = pre_comment;
    cible  = comment;
    
    while( (car = g_utf8_get_char( source )) )
     { if ( car == '\'' )                                                 /* D?doublage de la simple cote */
        { g_utf8_strncpy( cible, "\'", 1 ); cible = g_utf8_next_char( cible );
          g_utf8_strncpy( cible, "\'", 1 ); cible = g_utf8_next_char( cible );
        }
       else if (car =='\\')                                                    /* D?doublage du backspace */
        { g_utf8_strncpy( cible, "\\", 1 ); cible = g_utf8_next_char( cible );
          g_utf8_strncpy( cible, "\\", 1 ); cible = g_utf8_next_char( cible );
        }
       else
        { g_utf8_strncpy( cible, source, 1 ); cible = g_utf8_next_char( cible );
        }
       source = g_utf8_next_char(source);
     }
    return(comment);
  }
/**********************************************************************************************************/
/* Init_DB_SQL: essai de connexion ? la DataBase db                                                       */
/* Entr?e: toutes les infos necessaires a la connexion                                                    */
/* Sortie: une structure DB de r?f?rence                                                                  */
/**********************************************************************************************************/
 struct DB *Init_DB_SQL ( void )
  { static gint id = 1, taille;
    struct DB *db;
    my_bool reconnect;

    db = (struct DB *)g_try_malloc0( sizeof(struct DB) );
    if (!db)                                                          /* Si probleme d'allocation m?moire */
     { Info_new( Config.log, Config.log_db, LOG_ERR, "Init_DB_SQL: Memory error" );
       return(NULL);
     }

    db->mysql = mysql_init(NULL);
    if (!db->mysql)
     { Info_new( Config.log, Config.log_db, LOG_ERR, "Init_DB_SQL: Mysql_init failed (%s)",
                              (char *) mysql_error(db->mysql)  );
       g_free(db);
       return (NULL);
     }

    reconnect = 1;
    mysql_options( db->mysql, MYSQL_OPT_RECONNECT, &reconnect );
    if ( ! mysql_real_connect( db->mysql, Config.db_host, Config.db_username,
                               Config.db_password, Config.db_database, Config.db_port, NULL, 0 ) )
     { Info_new( Config.log, Config.log_db, LOG_ERR,
                 "Init_DB_SQL: mysql_real_connect failed (%s)",
                 (char *) mysql_error(db->mysql)  );
       mysql_close( db->mysql );
       g_free(db);
       return (NULL);
     }
    db->free = TRUE;
    pthread_mutex_lock ( &Partage->com_db.synchro );
    db->id = id++;
    Partage->com_db.Liste = g_slist_prepend ( Partage->com_db.Liste, db );
    taille = g_slist_length ( Partage->com_db.Liste );
    pthread_mutex_unlock ( &Partage->com_db.synchro );
    Info_new( Config.log, Config.log_db, LOG_DEBUG,
              "Init_DB_SQL: Database Connection OK with %s@%s:%d on %s (id=%05d). Nbr_requete_en_cours=%d",
               Config.db_username, Config.db_host, Config.db_port, Config.db_database, db->id, taille );
    return(db);
  }
/**********************************************************************************************************/
/* Libere_DB_SQL : Se deconnecte d'une base de donn?es en parametre                                       */
/* Entr?e: La DB                                                                                          */
/**********************************************************************************************************/
 void Libere_DB_SQL( struct DB **adr_db )
  { struct DB *db, *db_en_cours;
    gboolean found;
    GSList *liste;
    gint taille;
    if ( (!adr_db) || !(*adr_db) ) return;

    db = *adr_db;                                                   /* R?cup?ration de l'adresse de la db */
    found = FALSE;
    pthread_mutex_lock ( &Partage->com_db.synchro );
    liste = Partage->com_db.Liste;
    while ( liste )
     { db_en_cours = (struct DB *)liste->data;
       if (db_en_cours->id == db->id) found = TRUE;
       liste = g_slist_next( liste );
     }
    pthread_mutex_unlock ( &Partage->com_db.synchro );

    if (!found)
     { Info_new( Config.log, Config.log_db, LOG_CRIT,
                "Libere_DB_SQL: DB Free Request not in list ! ID=%05d, request=%s",
                 db->id, db->requete );
       return;
     }

    if (db->free==FALSE)
     { Info_new( Config.log, Config.log_db, LOG_WARNING,
                "Libere_DB_SQL: Reste un result a FREEer (id=%05d)!", db->id );
       Liberer_resultat_SQL ( db );
     }
    mysql_close( db->mysql );
    pthread_mutex_lock ( &Partage->com_db.synchro );
    Partage->com_db.Liste = g_slist_remove ( Partage->com_db.Liste, db );
    taille = g_slist_length ( Partage->com_db.Liste );
    pthread_mutex_unlock ( &Partage->com_db.synchro );
    Info_new( Config.log, Config.log_db, LOG_DEBUG,
             "Libere_DB_SQL: Deconnexion effective (id=%05d), Nbr_requete_en_cours=%d", db->id, taille );
    g_free( db );
    *adr_db = NULL;
    SEA ( NUM_EA_SYS_DBREQUEST_SIMULT, taille );                        /* Enregistrement pour historique */
  }
/**********************************************************************************************************/
/* Lancer_requete_SQL : lance une requete en parametre, sur la structure de ref?rence                     */
/* Entr?e: La DB, la requete                                                                              */
/* Sortie: TRUE si pas de souci                                                                           */
/**********************************************************************************************************/
 gboolean Lancer_requete_SQL ( struct DB *db, gchar *requete )
  { if (!db) return(FALSE);

    if (db->free==FALSE)
     { Info_new( Config.log, Config.log_db, LOG_WARNING,
                "Lancer_requete_SQL (id=%05d): Reste un result a FREEer!", db->id );
     }

    g_snprintf( db->requete, sizeof(db->requete), "%s", requete );                  /* Save for later use */
    Info_new( Config.log, Config.log_db, LOG_DEBUG,
             "Lancer_requete_SQL (id=%05d): NEW    (%s)", db->id, requete );
    if ( mysql_query ( db->mysql, requete ) )
     { Info_new( Config.log, Config.log_db, LOG_WARNING,
                "Lancer_requete_SQL (id=%05d): FAILED (%s)",
                 db->id, (char *)mysql_error(db->mysql) );
       return(FALSE);
     }

    if ( ! strncmp ( requete, "SELECT", 6 ) )
     { db->result = mysql_store_result ( db->mysql );
       db->free = FALSE;
       if ( ! db->result )
        { Info_new( Config.log, Config.log_db, LOG_WARNING,
                   "Lancer_requete_SQL (id=%05d): store_result failed (%s)",
                    db->id, (char *) mysql_error(db->mysql) );
          db->nbr_result = 0;
        }
       else 
        { /*Info( Config.log, DEBUG_DB, "Lancer_requete_SQL: store_result OK" );*/
          db->nbr_result = mysql_num_rows ( db->result );
        }
     }
    Info_new( Config.log, Config.log_db, LOG_DEBUG,
             "Lancer_requete_SQL (id=%05d): OK     (%s)", db->id, requete );
    return(TRUE);
  }
/**********************************************************************************************************/
/* Liberer_resultat_SQL: Libere la m?moire affect?e au resultat SQL                                       */
/* Entr?e: la DB                                                                                          */
/* Sortie: rien                                                                                           */
/**********************************************************************************************************/
 void Liberer_resultat_SQL ( struct DB *db )
  { if (db)
     { while( db->row ) Recuperer_ligne_SQL ( db );
       mysql_free_result( db->result );
       db->result = NULL;
       db->free = TRUE;
      /*Info( Config.log, DEBUG_DB, "Liberer_resultat_SQL: free OK" );*/
     }
  }
/**********************************************************************************************************/
/* Recuperer_ligne_SQL: Renvoie les lignes resultat, une par une                                          */
/* Entr?e: la DB                                                                                          */
/* Sortie: La ligne ou NULL si il n'y en en plus                                                          */
/**********************************************************************************************************/
 MYSQL_ROW Recuperer_ligne_SQL ( struct DB *db )
  { if (!db) return(NULL);
    db->row = mysql_fetch_row(db->result);
    return( db->row );
  }
/**********************************************************************************************************/
/* Recuperer_last_ID_SQL: Renvoie le dernier ID ins?r?                                                    */
/* Entr?e: la DB                                                                                          */
/* Sortie: Le dernier ID                                                                                  */
/**********************************************************************************************************/
 guint Recuperer_last_ID_SQL ( struct DB *db )
  { if (!db) return(0);
    return ( mysql_insert_id(db->mysql) );
  }
/**********************************************************************************************************/
/* Print_SQL_status : permet de logguer le statut SQL                                                     */
/* Entr?e: n?ant                                                                                          */
/* Sortie: n?ant                                                                                          */
/**********************************************************************************************************/
 void Print_SQL_status ( void )
  { GSList *liste;
    pthread_mutex_lock ( &Partage->com_db.synchro );
    Info_new( Config.log, Config.log_db, LOG_DEBUG,
             "Print_SQL_status: %03d running connexions",
              g_slist_length(Partage->com_db.Liste) );

    liste = Partage->com_db.Liste;
    while ( liste )
     { struct DB *db;
       db = (struct DB *)liste->data;
       Info_new( Config.log, Config.log_db, LOG_DEBUG,
              "Print_SQL_status: Connexion %03d requete %s",
               db->id, db->requete );
       liste = g_slist_next( liste );
     }
    pthread_mutex_unlock ( &Partage->com_db.synchro );
  }
/******************************************************************************************************************************/
/* Update_database_schema: V?rifie la connexion et le sch?ma de la base de donn?es                                            */
/* Entr?e: n?ant                                                                                                              */
/* Sortie: n?ant                                                                                                              */
/******************************************************************************************************************************/
 void Update_database_schema ( void )
  { gint database_version, server_major, server_minor, server_svnrev;
    gchar chaine[32], requete[4096];
    gchar *nom, *valeur;
    struct DB *db;

    if (Config.instance_is_master != TRUE)                                                  /* Do not update DB if not master */
     { Info_new( Config.log, Config.log_db, LOG_WARNING,
                "Update_database_schema: Instance is not master. Don't update schema." );
       return;
     }

    database_version = 0;                                                                                /* valeur par d?faut */
    if ( ! Recuperer_configDB( &db, "global" ) )                                            /* Connexion a la base de donn?es */
     { Info_new( Config.log, Config.log_db, LOG_WARNING,
                "Update_database_schema: Database connexion failed" );
       return;
     }

    while (Recuperer_configDB_suite( &db, &nom, &valeur ) )                           /* R?cup?ration d'une config dans la DB */
     { Info_new( Config.log, Config.log_db, LOG_INFO,                                                         /* Print Config */
                "Update_database_schema: found global param '%s' = %s in DB", nom, valeur );
       if ( ! g_ascii_strcasecmp ( nom, "database_version" ) )
        { database_version = atoi( valeur ); }
     }

    Info_new( Config.log, Config.log_db, LOG_NOTICE,
             "Update_database_schema: Actual Database_Version detected = %05d", database_version );

    db = Init_DB_SQL();       
    if (!db)
     { Info_new( Config.log, Config.log_db, LOG_ERR, "Update_database_schema: DB connexion failed" );
       return;
     }

    if (database_version < 2500)
     { g_snprintf( requete, sizeof(requete), "ALTER TABLE users DROP `imsg_bit_presence`" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete),
                  "ALTER TABLE users ADD `ssrv_bit_presence` INT NOT NULL DEFAULT '0'"
                 );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2510)
     { g_snprintf( requete, sizeof(requete),                                                                   /* Requete SQL */
                  "INSERT INTO `mnemos` (`id`, `type`, `num`, `num_plugin`, `acronyme`, `libelle`, `command_text`) VALUES"
                  "(23, 3,9999, 1, 'EVENT_NONE_TOR', 'Used for detected Event with no mapping yet.', ''),"
                  "(24, 5,9999, 1, 'EVENT_NONE_ANA', 'Used for detected Event with no mapping yet.', '')"
                 );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2532)
     { g_snprintf( requete, sizeof(requete), "RENAME TABLE eana TO mnemos_AnalogInput" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), 
                  "CREATE TABLE `mnemos_DigitalInput`"
                  "(`id_mnemo` int(11) NOT NULL, `furtif` int(1) NOT NULL, PRIMARY KEY (`id_mnemo`)"
                  ") ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci"
                 );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2541)
     { g_snprintf( requete, sizeof(requete), "RENAME TABLE dls_cpt_imp TO mnemos_CptImp" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2543)
     { g_snprintf( requete, sizeof(requete), "RENAME TABLE tempo TO mnemos_Tempo" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2571)
     { g_snprintf( requete, sizeof(requete), "ALTER TABLE `users` ADD `imsg_available` TINYINT NOT NULL AFTER `imsg_allow_cde`" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2573)
     { g_snprintf( requete, sizeof(requete), "INSERT INTO `mnemos` "
                   "(`id`, `type`, `num`, `num_plugin`, `acronyme`, `libelle`, `command_text`) VALUES "
                   "(25, 5, 122, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(26, 5, 121, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(27, 5, 120, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(28, 5, 119, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(29, 5, 118, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(30, 5, 117, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(31, 5, 116, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(32, 5, 115, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(33, 5, 114, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(34, 5, 113, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(35, 5, 112, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(36, 5, 111, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(37, 5, 110, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(38, 5, 109, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(39, 5, 108, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(40, 5, 107, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(41, 5, 106, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(42, 5, 105, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(43, 5, 104, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(44, 5, 103, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(45, 5, 102, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(46, 5, 101, 1, 'SYS_RESERVED', 'Reserved for internal use', ''),"
                   "(47, 5, 100, 1, 'SYS_RESERVED', 'Reserved for internal use', '');"
                 );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2581)
     { g_snprintf( requete, sizeof(requete), "ALTER TABLE `modbus_modules` CHANGE `min_e_tor` `map_E` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), "ALTER TABLE `modbus_modules` CHANGE `min_e_ana` `map_EA` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), "ALTER TABLE `modbus_modules` CHANGE `min_s_tor` `map_A` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), "ALTER TABLE `modbus_modules` CHANGE `min_s_ana` `map_AA` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2582)
     { g_snprintf( requete, sizeof(requete), "ALTER TABLE `rfxcom` CHANGE `e_min` `map_E` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), "ALTER TABLE `rfxcom` CHANGE `ea_min` `map_EA` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), "ALTER TABLE `rfxcom` CHANGE `a_min` `map_A` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2583)
     { g_snprintf( requete, sizeof(requete), "RENAME TABLE onduleurs TO ups" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), "ALTER TABLE `ups` CHANGE `e_min` `map_E` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), "ALTER TABLE `ups` CHANGE `ea_min` `map_EA` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), "ALTER TABLE `ups` CHANGE `a_min` `map_A` INT(11) NOT NULL" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2669)
     { g_snprintf( requete, sizeof(requete), "DROP TABLE rfxcom" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }

    if (database_version < 2696)
     { g_snprintf( requete, sizeof(requete), "ALTER TABLE `dls` ADD `compil_date` int(11) NOT NULL AFTER `actif`" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
       g_snprintf( requete, sizeof(requete), "ALTER TABLE `dls` ADD `compil_status` int(11) NOT NULL AFTER `compil_date`" );
       Lancer_requete_SQL ( db, requete );                                                     /* Execution de la requete SQL */
     }
     
    Libere_DB_SQL(&db);

    sscanf ( VERSION, "%d.%d.%d", &server_major, &server_minor, &server_svnrev );
     g_snprintf( chaine, sizeof(chaine), "%s", server_svnrev );
    if (Modifier_configDB ( "global", "database_version", chaine ))
     { Info_new( Config.log, Config.log_db, LOG_NOTICE,
                "Update_database_schema: updating Database_version to %d OK", server_svnrev );
     }
    else
     { Info_new( Config.log, Config.log_db, LOG_NOTICE,
                "Update_database_schema: updating Database_version to %d FAILED", server_svnrev );
     }

  }
/*--------------------------------------------------------------------------------------------------------*/
