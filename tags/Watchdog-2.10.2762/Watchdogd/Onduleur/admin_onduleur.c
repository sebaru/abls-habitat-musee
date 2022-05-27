/**********************************************************************************************************/
/* Watchdogd/Admin/admin_ups.c        Gestion des connexions Admin ONDULEUR au serveur watchdog      */
/* Projet WatchDog version 2.0       Gestion d'habitat                     mer. 11 nov. 2009 11:28:29 CET */
/* Auteur: LEFEVRE Sebastien                                                                              */
/**********************************************************************************************************/
/*
 * admin_ups.c
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
 
 #include "watchdogd.h"
 #include "Onduleur.h"
/**********************************************************************************************************/
/* Admin_ups_reload: Demande le rechargement des conf ONDULEUR                                            */
/* Entr�e: le connexion                                                                                   */
/* Sortie: rien                                                                                           */
/**********************************************************************************************************/
 static void Admin_ups_reload ( struct CONNEXION *connexion )
  { if (Cfg_ups.lib->Thread_run == FALSE)
     { Admin_write ( connexion, " Thread ONDULEUR is not running\n" );
       return;
     }
    
    Cfg_ups.reload = TRUE;
    Admin_write ( connexion, " ONDULEUR Reloading in progress\n" );
    while (Cfg_ups.reload) sched_yield();
    Admin_write ( connexion, " ONDULEUR Reloading done\n" );
  }
/**********************************************************************************************************/
/* Admin_ups_print : Affiche en details les infos d'un onduleur en parametre                              */
/* Entr�e: La connexion connexion ADMIN et l'onduleur                                                     */
/* Sortie: Rien, tout est envoy� dans le pipe Admin                                                       */
/**********************************************************************************************************/
 static void Admin_ups_print ( struct CONNEXION *connexion, struct MODULE_UPS *module )
  { gchar chaine[1024];
    g_snprintf( chaine, sizeof(chaine),
                " UPS[%02d] ------> %s@%s %s\n"
                "  | - enable = %d, started = %d (bit B%04d=%d)\n"
                "  | - map_EA = EA%03d, map_E = E%03d, map_A = A%03d\n"
                "  | - username = %s, password = %s,\n"
                "  | - date_next_connexion = in %03ds\n"
                "  -\n",
                module->ups.id, module->ups.ups, module->ups.host, module->libelle,
                module->ups.enable, module->started, module->ups.bit_comm, B(module->ups.bit_comm),
                module->ups.map_EA, module->ups.map_E, module->ups.map_A,
                module->ups.username, module->ups.password,
          (int)(module->date_next_connexion > Partage->top ? (module->date_next_connexion - Partage->top)/10 : -1)
              );
    Admin_write ( connexion, chaine );
  }
/**********************************************************************************************************/
/* Admin_ups_list : Affichage de toutes les infos op�rationnelles de tous les onduleurs                   */
/* Entr�e: La connexion connexion ADMIN                                                                   */
/* Sortie: Rien, tout est envoy� dans le pipe Admin                                                       */
/**********************************************************************************************************/
 static void Admin_ups_list ( struct CONNEXION *connexion )
  { GSList *liste_modules;
       
    pthread_mutex_lock ( &Cfg_ups.lib->synchro );
    liste_modules = Cfg_ups.Modules_UPS;
    while ( liste_modules )
     { struct MODULE_UPS *module;
       module = (struct MODULE_UPS *)liste_modules->data;
       Admin_ups_print ( connexion, module );
       liste_modules = liste_modules->next;
     }
    pthread_mutex_unlock ( &Cfg_ups.lib->synchro );
  }
/**********************************************************************************************************/
/* Admin_ups_show : Affichage des infos op�rationnelles li�es � l'onduleur en parametre                   */
/* Entr�e: La connexion connexion ADMIN et le numero de l'onduleur                                        */
/* Sortie: Rien, tout est envoy� dans le pipe Admin                                                       */
/**********************************************************************************************************/
 static void Admin_ups_show ( struct CONNEXION *connexion, gint num )
  { GSList *liste_modules;
       
    pthread_mutex_lock ( &Cfg_ups.lib->synchro );
    liste_modules = Cfg_ups.Modules_UPS;
    while ( liste_modules )
     { struct MODULE_UPS *module;
       module = (struct MODULE_UPS *)liste_modules->data;
       if (module->ups.id == num) { Admin_ups_print ( connexion, module ); break; }
       liste_modules = liste_modules->next;
     }
    pthread_mutex_unlock ( &Cfg_ups.lib->synchro );
  }
/**********************************************************************************************************/
/* Admin_ups_start: Demande le demarrage d'un onduleur en parametre                                       */
/* Entr�e: La connexion et le num�ro d'onduleur                                                           */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 static void Admin_ups_start ( struct CONNEXION *connexion, gint id )
  { gchar chaine[128];

    g_snprintf( chaine, sizeof(chaine), " -- Demarrage d'un module UPS\n" );
    Admin_write ( connexion, chaine );

    Cfg_ups.admin_start = id;

    g_snprintf( chaine, sizeof(chaine), " Module UPS %d started\n", id );
    Admin_write ( connexion, chaine );
  }
/**********************************************************************************************************/
/* Admin_ups_stop: Demande l'arret d'un onduleur en parametre                                             */
/* Entr�e: La connexion et le num�ro d'onduleur                                                           */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 static void Admin_ups_stop ( struct CONNEXION *connexion, gint id )
  { gchar chaine[128];

    g_snprintf( chaine, sizeof(chaine), " -- Arret d'un module UPS\n" );
    Admin_write ( connexion, chaine );

    Cfg_ups.admin_stop = id;

    g_snprintf( chaine, sizeof(chaine), " Module UPS %d stopped\n", id );
    Admin_write ( connexion, chaine );
  }
/**********************************************************************************************************/
/* Admin_ups_set: Change un parametre dans la DB ups                                                      */
/* Entr�e: La connexion et la ligne de commande (champ valeur)                                            */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 static void Admin_ups_set ( struct CONNEXION *connexion, gchar *ligne )
  { gchar id_char[16], param[32], valeur_char[64], chaine[128];
    struct MODULE_UPS *module = NULL;
    GSList *liste_modules;
    guint id, valeur;
    gint retour;

    if ( ! strcmp ( ligne, "list" ) )
     { Admin_write ( connexion, " | Parameter can be:\n" );
       Admin_write ( connexion, " | - enable, host, ups, username, password, bit_comm,\n" );
       Admin_write ( connexion, " | - map_E, map_EA, map_A\n" );
       Admin_write ( connexion, " -\n" );
       return;
     }

    sscanf ( ligne, "%s %s %[^\n]", id_char, param, valeur_char );
    id     = atoi ( id_char     );
    valeur = atoi ( valeur_char );

    pthread_mutex_lock( &Cfg_ups.lib->synchro );                        /* Recherche du module en m�moire */
    liste_modules = Cfg_ups.Modules_UPS;
    while ( liste_modules )
     { module = (struct MODULE_UPS *)liste_modules->data;
       if (module->ups.id == id) break;
       liste_modules = liste_modules->next;                                  /* Passage au module suivant */
     }
    pthread_mutex_unlock( &Cfg_ups.lib->synchro );

    if (!module)                                                                         /* Si non trouv� */
     { Admin_write( connexion, " Module not found\n");
       return;
     }

    if ( ! strcmp( param, "enable" ) )
     { module->ups.enable = (valeur ? TRUE : FALSE); }
    else if ( ! strcmp( param, "bit_comm" ) )
     { module->ups.bit_comm = valeur; }
    else if ( ! strcmp( param, "map_E" ) )
     { module->ups.map_E = valeur; }
    else if ( ! strcmp( param, "map_EA" ) )
     { module->ups.map_EA = valeur; }
    else if ( ! strcmp( param, "map_A" ) )
     { module->ups.map_A = valeur; }
    else if ( ! strcmp( param, "host" ) )
     { g_snprintf( module->ups.host, sizeof(module->ups.host), "%s", valeur_char ); }
    else if ( ! strcmp( param, "ups" ) )
     { g_snprintf( module->ups.ups, sizeof(module->ups.ups), "%s", valeur_char ); }
    else if ( ! strcmp( param, "username" ) )
     { g_snprintf( module->ups.username, sizeof(module->ups.username), "%s", valeur_char ); }
    else if ( ! strcmp( param, "password" ) )
     { g_snprintf( module->ups.password, sizeof(module->ups.password), "%s", valeur_char ); }
    else
     { g_snprintf( chaine, sizeof(chaine),
                 " Parameter %s not known for UPS id %s ('ups set list' can help)\n", param, id_char );
       Admin_write ( connexion, chaine );
       return;
     }

    retour = Modifier_upsDB ( &module->ups );
    if (retour)
     { Admin_write ( connexion, " ERROR : UPS module NOT set\n" ); }
    else
     { Admin_write ( connexion, " UPS module parameter set\n" ); }
  }
/**********************************************************************************************************/
/* Admin_command : Fonction principale de traitement des commandes du thread                              */
/* Entr�e: La connexion et la ligne de commande a parser                                                  */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 void Admin_command ( struct CONNEXION *connexion, gchar *ligne )
  { gchar commande[128], chaine[128];

    sscanf ( ligne, "%s", commande );                                /* D�coupage de la ligne de commande */

    if ( ! strcmp ( commande, "start" ) )
     { int num;
       sscanf ( ligne, "%s %d", commande, &num );                    /* D�coupage de la ligne de commande */
       Admin_ups_start ( connexion, num );
     }
    else if ( ! strcmp ( commande, "stop" ) )
     { int num;
       sscanf ( ligne, "%s %d", commande, &num );                    /* D�coupage de la ligne de commande */
       Admin_ups_stop ( connexion, num );
     }
    else if ( ! strcmp ( commande, "show" ) )
     { int num;
       sscanf ( ligne, "%s %d", commande, &num );                    /* D�coupage de la ligne de commande */
       Admin_ups_show ( connexion, num );
     }
    else if ( ! strcmp ( commande, "list" ) )
     { Admin_ups_list ( connexion );
     }
    else if ( ! strcmp ( commande, "reload" ) )
     { Admin_ups_reload(connexion);
     }
    else if ( ! strcmp ( commande, "add" ) )
     { struct UPSDB ups;
       gint retour;
       sscanf ( ligne, "%s %[^\n]", commande, ups.ups );
       ups.enable = TRUE;
       retour = Ajouter_upsDB ( &ups );
       if (retour == -1)
        { Admin_write ( connexion, "Error, UPS not added\n" ); }
       else
        { gchar chaine[80];
          g_snprintf( chaine, sizeof(chaine), " UPS %s added. New ID=%d\n", ups.ups, retour );
          Admin_write ( connexion, chaine );
          Cfg_ups.reload = TRUE;                    /* Rechargement des modules UPS en m�moire de travail */
        }
     }
    else if ( ! strcmp ( commande, "set" ) )
     { Admin_ups_set ( connexion, ligne+4 );
       Cfg_ups.reload = TRUE;                       /* Rechargement des modules UPS en m�moire de travail */
     }
    else if ( ! strcmp ( commande, "del" ) )
     { struct UPSDB ups;
       gboolean retour;
       sscanf ( ligne, "%s %d", commande, &ups.id );                 /* D�coupage de la ligne de commande */
       retour = Retirer_upsDB ( &ups );
       if (retour == FALSE)
        { Admin_write ( connexion, "Error, UPS not erased\n" ); }
       else
        { gchar chaine[80];
          g_snprintf( chaine, sizeof(chaine), " UPS %d erased\n", ups.id );
          Admin_write ( connexion, chaine );
          Cfg_ups.reload = TRUE;                     /* Rechargement des modules RS en m�moire de travail */
        }
     }
    else if ( ! strcmp ( commande, "dbcfg" ) ) /* Appelle de la fonction d�di�e � la gestion des parametres DB */
     { if (Admin_dbcfg_thread ( connexion, NOM_THREAD, ligne+6 ) == TRUE)   /* Si changement de parametre */
        { gboolean retour;
          retour = Ups_Lire_config();
          g_snprintf( chaine, sizeof(chaine), " Reloading Thread Parameters from Database -> %s\n",
                      (retour ? "Success" : "Failed") );
          Admin_write ( connexion, chaine );
        }
     }
    else if ( ! strcmp ( commande, "help" ) )
     { Admin_write ( connexion, "  -- Watchdog ADMIN -- Help du mode 'UPS'\n" );
       Admin_write ( connexion, "  dbcfg ...             - Get/Set Database Parameters\n" );
       Admin_write ( connexion, "  add $ups              - Ajoute un UPS\n" );
       Admin_write ( connexion, "  set $id $champ $val   - Set $val to $champ for module $id\n" );
       Admin_write ( connexion, "  set list              - List parameter that can be set\n" );
       Admin_write ( connexion, "  del $id               - Delete UPS id\n" );
       Admin_write ( connexion, "  start $id             - Start UPS id\n" );
       Admin_write ( connexion, "  stop $id              - Stop UPS id\n" );
       Admin_write ( connexion, "  show $id              - Show UPS id\n" );
       Admin_write ( connexion, "  list                  - Liste les modules ONDULEUR\n" );
       Admin_write ( connexion, "  reload                - Recharge la configuration\n" );
     }
    else
     { gchar chaine[128];
       g_snprintf( chaine, sizeof(chaine), " Unknown NUT command : %s\n", ligne );
       Admin_write ( connexion, chaine );
     }
  }
/*--------------------------------------------------------------------------------------------------------*/
