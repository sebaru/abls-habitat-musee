/******************************************************************************************************************************/
/* Watchdogd/process.c        Gestion des process                                                                             */
/* Projet WatchDog version 3.0       Gestion d'habitat                                          sam 11 avr 2009 12:21:45 CEST */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * process.c
 * This file is part of Watchdog
 *
 * Copyright (C) 2010-2020 - Sebastien LEFEVRE
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
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <dirent.h>
 #include <string.h>
 #include <stdio.h>

 #include <sys/wait.h>
 #include <fcntl.h>
 #include <errno.h>
 #include <dlfcn.h>

/**************************************************** Prototypes de fonctions *************************************************/
 #include "Reseaux.h"
 #include "watchdogd.h"

/******************************************************************************************************************************/
/* Start_librairie: Demarre le thread en paremetre                                                                            */
/* Entr????e: La structure associ????e au thread                                                                                    */
/* Sortie: FALSE si erreur                                                                                                    */
/******************************************************************************************************************************/
 gboolean Start_librairie ( struct LIBRAIRIE *lib )
  { pthread_attr_t attr;
    pthread_t tid;
    if (!lib) return(FALSE);
    if (lib->Thread_run == TRUE)
     { Info_new( Config.log, Config.log_msrv, LOG_INFO, "%s: thread %s already seems to be running", __func__, lib->nom_fichier );
       return(FALSE);
     }

    if ( pthread_attr_init(&attr) )                                                 /* Initialisation des attributs du thread */
     { Info_new( Config.log, Config.log_msrv, LOG_ERR, "%s: pthread_attr_init failed (%s)", __func__, lib->nom_fichier );
       return(FALSE);
     }

    if ( pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) )                       /* On le laisse joinable au boot */
     { Info_new( Config.log, Config.log_msrv, LOG_ERR, "%s: pthread_setdetachstate failed (%s)", __func__, lib->nom_fichier );
       return(FALSE);
     }

    if ( pthread_create( &tid, &attr, (void *)lib->Run_thread, lib ) )
     { Info_new( Config.log, Config.log_msrv, LOG_ERR, "%s: pthread_create failed (%s)", __func__, lib->nom_fichier );
       return(FALSE);
     }
    pthread_attr_destroy(&attr);                                                                        /* Lib????ration m????moire */
    Info_new( Config.log, Config.log_msrv, LOG_NOTICE, "%s: Starting thread %s OK (TID=%p)", __func__, lib->nom_fichier, tid );
    return(TRUE);
  }
/******************************************************************************************************************************/
/* Stop_librairie: Arrete le thread en paremetre                                                                              */
/* Entr????e: La structure associ????e au thread                                                                                    */
/* Sortie: FALSE si erreur                                                                                                    */
/******************************************************************************************************************************/
 gboolean Stop_librairie ( struct LIBRAIRIE *lib )
  { if (!lib) return(FALSE);
    if ( lib->TID != 0 )
     { Info_new( Config.log, Config.log_msrv, LOG_INFO, "%s: thread %s, stopping in progress", __func__, lib->nom_fichier );
       lib->Thread_run = FALSE;                                                          /* On demande au thread de s'arreter */
       while( lib->TID != 0 ) sched_yield();                                                            /* Attente fin thread */
       sleep(1);
     }
    Info_new( Config.log, Config.log_msrv, LOG_NOTICE, "%s: thread %s stopped", __func__, lib->nom_fichier );
    return(TRUE);
  }
/******************************************************************************************************************************/
/* Charger_librarie_par_fichier: Ouverture d'une librairie par son nom                                                        */
/* Entr????e: Le nom de fichier correspondant                                                                                    */
/* Sortie: Rien                                                                                                               */
/******************************************************************************************************************************/
 struct LIBRAIRIE *Charger_librairie_par_prompt ( gchar *nom_prompt )
  { pthread_mutexattr_t attr;                                                          /* Initialisation des mutex de synchro */
    struct LIBRAIRIE *lib;
    gchar nom_absolu[128];
    GSList *liste;

    liste = Partage->com_msrv.Librairies;
    while (liste)
     { struct LIBRAIRIE *lib;
       lib = (struct LIBRAIRIE *)liste->data;
       if ( ! strcmp( lib->admin_prompt, nom_prompt ) )
        { Info_new( Config.log, Config.log_msrv, LOG_INFO, "%s: Librairie %s already loaded", __func__, nom_prompt );
          return(NULL);
        }
       liste=liste->next;
     }

    lib = (struct LIBRAIRIE *) g_try_malloc0( sizeof ( struct LIBRAIRIE ) );
    if (!lib) { Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: MemoryAlloc failed", __func__ );
                return(NULL);
              }

    g_snprintf( nom_absolu, sizeof(nom_absolu), "%s/libwatchdog-server-%s.so", Config.librairie_dir, nom_prompt );

    lib->dl_handle = dlopen( nom_absolu, RTLD_GLOBAL | RTLD_NOW );
    if (!lib->dl_handle)
     { Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: Candidat %s failed (%s)", __func__, nom_absolu, dlerror() );
       g_free(lib);
       return(NULL);
     }

    lib->Run_thread = dlsym( lib->dl_handle, "Run_thread" );                                      /* Recherche de la fonction */
    if (!lib->Run_thread)
     { Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: Candidat %s rejected (Run_thread not found)", __func__, nom_absolu );
       dlclose( lib->dl_handle );
       g_free(lib);
       return(NULL);
     }

    lib->Admin_json    = dlsym( lib->dl_handle, "Admin_json" );                                   /* Recherche de la fonction */

    g_snprintf( lib->nom_fichier, sizeof(lib->nom_fichier), "%s", nom_absolu );
    Info_new( Config.log, Config.log_msrv, LOG_INFO, "%s: %s loaded", __func__, nom_absolu );

    pthread_mutexattr_init( &attr );                                                          /* Creation du mutex de synchro */
    pthread_mutexattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
    pthread_mutex_init( &lib->synchro, &attr );

    Partage->com_msrv.Librairies = g_slist_prepend( Partage->com_msrv.Librairies, lib );
    return(lib);
  }
/******************************************************************************************************************************/
/* Decharger_librairie_par_nom: Decharge la librairie dont le nom est en param??tre                                            */
/* Entr??e: Le nom de la librairie                                                                                             */
/* Sortie: Rien                                                                                                               */
/******************************************************************************************************************************/
 gboolean Decharger_librairie_par_prompt ( gchar *prompt )
  { struct LIBRAIRIE *lib;
    GSList *liste;

    liste = Partage->com_msrv.Librairies;
    while(liste)                                                                            /* Liberation m????moire des modules */
     { lib = (struct LIBRAIRIE *)liste->data;                                         /* R????cup????ration des donn????es de la liste */

       if ( ! strcmp ( lib->admin_prompt, prompt ) )
        { Info_new( Config.log, Config.log_msrv, LOG_INFO, "%s: trying to unload %s", __func__, lib->nom_fichier );

          Stop_librairie(lib);
          pthread_mutex_destroy( &lib->synchro );
          dlclose( lib->dl_handle );
          Partage->com_msrv.Librairies = g_slist_remove( Partage->com_msrv.Librairies, lib );
                                                                             /* Destruction de l'entete associ???? dans la GList */
          Info_new( Config.log, Config.log_msrv, LOG_NOTICE, "%s: library %s unloaded", __func__, lib->nom_fichier );
          g_free( lib );
          return(TRUE);
        }
       liste = liste->next;
     }
   Info_new( Config.log, Config.log_msrv, LOG_ERR, "%s: library %s not found", __func__, prompt );
   return(FALSE);
  }
/******************************************************************************************************************************/
/* Decharger_librairies: Decharge toutes les librairies                                                                       */
/* Entr????e: Rien                                                                                                               */
/* Sortie: Rien                                                                                                               */
/******************************************************************************************************************************/
 void Decharger_librairies ( void )
  { struct LIBRAIRIE *lib;
    GSList *liste;

    liste = Partage->com_msrv.Librairies;                 /* Envoie une commande d'arret pour toutes les librairies d'un coup */
    while(liste)
     { lib = (struct LIBRAIRIE *)liste->data;
       lib->Thread_run = FALSE;                                                          /* On demande au thread de s'arreter */
       liste = liste->next;
     }

    while(Partage->com_msrv.Librairies)                                                     /* Liberation m????moire des modules */
     { lib = (struct LIBRAIRIE *)Partage->com_msrv.Librairies->data;
       Decharger_librairie_par_prompt (lib->admin_prompt);
     }
  }
/******************************************************************************************************************************/
/* Charger_librairies: Ouverture de toutes les librairies possibles pour Watchdog                                             */
/* Entr????e: Rien                                                                                                               */
/* Sortie: Rien                                                                                                               */
/******************************************************************************************************************************/
 void Charger_librairies ( void )
  { struct dirent *fichier;
    DIR *repertoire;

    repertoire = opendir ( Config.librairie_dir );                                  /* Ouverture du r????pertoire des librairies */
    if (!repertoire)
     { Info_new( Config.log, Config.log_msrv, LOG_ERR, "%s: Directory %s Unknown", __func__, Config.librairie_dir );
       return;
     }
    else
     { Info_new( Config.log, Config.log_msrv, LOG_NOTICE, "%s: Loading Directory %s in progress", __func__, Config.librairie_dir );
     }

    while( (fichier = readdir( repertoire )) )                                      /* Pour chacun des fichiers du r????pertoire */
     { gchar prompt[64];
       if (!strncmp( fichier->d_name, "libwatchdog-server-", 19 ))                     /* Chargement unitaire d'une librairie */
        { if ( ! strncmp( fichier->d_name + strlen(fichier->d_name) - 3, ".so", 4 ) )
           { struct LIBRAIRIE *lib;
             g_snprintf( prompt, strlen(fichier->d_name)-21, "%s", fichier->d_name + 19 );
             lib = Charger_librairie_par_prompt( prompt );
             if (lib) { Start_librairie( lib ); }
           }
        }
     }
    closedir( repertoire );                                                 /* Fermeture du r????pertoire a la fin du traitement */

    Info_new( Config.log, Config.log_msrv, LOG_INFO, "%s: %d Library loaded", __func__, g_slist_length( Partage->com_msrv.Librairies ) );
  }
/******************************************************************************************************************************/
/* Demarrer_dls: Thread un process DLS                                                                                        */
/* Entr????e: rien                                                                                                               */
/* Sortie: false si probleme                                                                                                  */
/******************************************************************************************************************************/
 gboolean Demarrer_dls ( void )
  { Info_new( Config.log, Config.log_msrv, LOG_DEBUG, "%s: Demande de demarrage %d", __func__, getpid() );
    if (Partage->com_dls.Thread_run == TRUE)
     { Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: An instance is already running %d",__func__, Partage->com_dls.TID );
       return(FALSE);
     }
    memset( &Partage->com_dls, 0, sizeof(Partage->com_dls) );                       /* Initialisation des variables du thread */
    if ( pthread_create( &Partage->com_dls.TID, NULL, (void *)Run_dls, NULL ) )
     { Info_new( Config.log, Config.log_msrv, LOG_ERR, "%s: pthread_create failed", __func__ );
       return(FALSE);
     }
    Info_new( Config.log, Config.log_msrv, LOG_NOTICE, "%s: thread dls (%p) seems to be running", __func__, Partage->com_dls.TID );
    return(TRUE);
  }
/******************************************************************************************************************************/
/* Demarrer_arch: Thread un process arch                                                                                      */
/* Entr????e: rien                                                                                                               */
/* Sortie: false si probleme                                                                                                  */
/******************************************************************************************************************************/
 gboolean Demarrer_arch ( void )
  { Info_new( Config.log, Config.log_msrv, LOG_DEBUG, "%s: Demande de demarrage %d", __func__, getpid() );
    if (Partage->com_arch.Thread_run == TRUE)
     { Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: An instance is already running", __func__, Partage->com_arch.TID );
       return(FALSE);
     }
    if (pthread_create( &Partage->com_arch.TID, NULL, (void *)Run_arch, NULL ))
     { Info_new( Config.log, Config.log_msrv, LOG_ERR, "%s: pthread_create failed", __func__ );
       return(FALSE);
     }
    Info_new( Config.log, Config.log_msrv, LOG_NOTICE, "%s: thread arch (%p) seems to be running", __func__, Partage->com_arch.TID );
    return(TRUE);
  }
/******************************************************************************************************************************/
/* Stopper_fils: arret de tous les fils Watchdog                                                                              */
/* Entr??/Sortie: n??ant                                                                                                        */
/******************************************************************************************************************************/
 void Stopper_fils ( void )
  { Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: Debut stopper_fils", __func__ );

    Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: Waiting for DLS (%p) to finish", __func__, Partage->com_dls.TID );
    Partage->com_dls.Thread_run = FALSE;
    while ( Partage->com_dls.TID != 0 ) sched_yield();                                                     /* Attente fin DLS */
    Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: ok, DLS is down", __func__ );

    Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: Waiting for ARCH (%p) to finish", __func__, Partage->com_arch.TID );
    Partage->com_arch.Thread_run = FALSE;
    while ( Partage->com_arch.TID != 0 ) sched_yield();                                                    /* Attente fin DLS */
    Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: ok, ARCH is down", __func__ );

    Info_new( Config.log, Config.log_msrv, LOG_WARNING, "%s: Fin stopper_fils", __func__ );
  }
/*----------------------------------------------------------------------------------------------------------------------------*/
