/******************************************************************************************************************************/
/* Watchdogd/Archive/archiveDB.c       D?claration des fonctions pour la gestion des valeurs analogiques                      */
/* Projet WatchDog version 2.0       Gestion d'habitat                                         mer. 09 mai 2012 12:45:53 CEST */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * archiveDB.c
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
 
 #include <glib.h>
 #include <sys/time.h>
 #include <sys/prctl.h>
 
 #include "watchdogd.h"
/******************************************************************************************************************************/
/* Ajouter_archDB: Ajout d'une entree archive dans la Base de Donn?es                                                         */
/* Entr?e: un log et une database, un flag d'ajout/edition, et la structure arch                                              */
/* Sortie: false si probleme                                                                                                  */
/******************************************************************************************************************************/
 void Ajouter_archDB ( struct DB *db, struct ARCHDB *arch )
  { gchar requete[512], table[512];

    g_snprintf( requete, sizeof(requete),                                                                      /* Requete SQL */
                "INSERT INTO %s_%03d_%06d(date_time,valeur) VALUES "
                "(FROM_UNIXTIME(%d.%d),'%f')",
                NOM_TABLE_ARCH, arch->type, arch->num, arch->date_sec, arch->date_usec, arch->valeur );

    if (Lancer_requete_SQL ( db, requete )==FALSE)                                             /* Execution de la requete SQL */
     {                               /* Si erreur, c'est peut etre parce que la table n'existe pas, on tente donc de la cr?er */
       g_snprintf( table, sizeof(table),                                                                       /* Requete SQL */
                   "CREATE TABLE `%s_%03d_%06d`("
                   "`date_time` datetime(6) DEFAULT NULL,"
                   "`valeur` float NOT NULL DEFAULT '0',"
                   "KEY `index_date` (`date_time`)"
                   ") ENGINE=ARIA DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci"
                   "  PARTITION BY LINEAR KEY (date_time) PARTITIONS 52;",
                   NOM_TABLE_ARCH, arch->type, arch->num );
       Lancer_requete_SQL ( db, table );                                                       /* Execution de la requete SQL */
       Lancer_requete_SQL ( db, requete );                             /* Une fois la table cr??, on peut y stocker l'archive */
	 }

  }
/******************************************************************************************************************************/
/* Arch_Update_SQL_Partitions: Appel? une fois par jour pour faire des op?rations de menage dans les tables d'archivages      */
/* Entr?e: n?ant                                                                                                              */
/* Sortie: false si probleme                                                                                                  */
/******************************************************************************************************************************/
 void Arch_Update_SQL_Partitions_thread ( void )
  { gchar requete[512];
	   GSList *Liste_tables;
    struct DB *db;
    prctl(PR_SET_NAME, "W-ArchSQL", 0, 0, 0 );
    Liste_tables = NULL;

    db = Init_ArchDB_SQL();      
    if (!db)
     { Info_new( Config.log, Config.log_arch, LOG_ERR,
                "%s: Unable to open database %s", __func__, Partage->com_arch.archdb_database );
       return;
     }

    Info_new( Config.log, Config.log_arch, LOG_NOTICE,
             "%s: Starting Update SQL Partition on %s with days=%d", __func__,
              Partage->com_arch.archdb_database, Partage->com_arch.duree_retention );
    g_snprintf( requete, sizeof(requete),                                                                      /* Requete SQL */
                "SELECT table_name FROM information_schema.tables WHERE table_schema='%s' "
                "AND table_name like 'histo_bit_%%'", Partage->com_arch.archdb_database );
    if (Lancer_requete_SQL ( db, requete )==FALSE)                                             /* Execution de la requete SQL */
     { Libere_DB_SQL(&db);
	      Info_new( Config.log, Config.log_arch, LOG_ERR,
                "%s: Searching table names failed", __func__ );
       return;
     }

    while ( Recuperer_ligne_SQL(db) )                                                      /* Chargement d'une ligne resultat */
     { Liste_tables = g_slist_prepend ( Liste_tables, strdup(db->row[0]) ); }
    Libere_DB_SQL(&db);

    db = Init_ArchDB_SQL();      
    if (!db)
     { Info_new( Config.log, Config.log_arch, LOG_ERR,
                "%s: Unable to open database %s for deleting", __func__, Partage->com_arch.archdb_database );
       while (Liste_tables)
        { gchar *table;
	         table = Liste_tables->data;
	         Liste_tables = g_slist_remove ( Liste_tables, table );
   	      g_free(table);
        }
       return;
     }

    while (Liste_tables && Partage->com_arch.Thread_run == TRUE)
     { gchar *table;
       gint top;
	      table = Liste_tables->data;
	      Liste_tables = g_slist_remove ( Liste_tables, table );
       Info_new( Config.log, Config.log_arch, LOG_DEBUG,
                "%s: Starting Update SQL Partition table %s", __func__, table );
	      g_snprintf( requete, sizeof(requete),                                                                   /* Requete SQL */
                  "DELETE FROM %s WHERE date_time < NOW() - INTERVAL %d DAY", table, Partage->com_arch.duree_retention );
       top = Partage->top;
       if (Lancer_requete_SQL ( db, requete )==FALSE)                                          /* Execution de la requete SQL */
        { Info_new( Config.log, Config.log_arch, LOG_ERR,
                   "%s: Unable to delete from table '%s'", __func__, table );
        }
       Info_new( Config.log, Config.log_arch, LOG_NOTICE,
                "%s: Update SQL Partition table %s OK in %05.1fs", __func__, table, (Partage->top-top)/10.0 );
       g_free(table);
     }

    g_slist_foreach( Liste_tables, (GFunc)g_free, NULL );                            /* Vidage de la liste si arret prematur? */
    g_slist_free( Liste_tables );

    Libere_DB_SQL(&db);
    Info_new( Config.log, Config.log_arch, LOG_NOTICE,
             "%s: Update SQL Partition end", __func__ );
  }
/*----------------------------------------------------------------------------------------------------------------------------*/
