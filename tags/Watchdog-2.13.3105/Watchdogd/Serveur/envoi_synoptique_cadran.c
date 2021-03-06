/******************************************************************************************************************************/
/* Watchdogd/Serveur/envoi_synoptique_cadran.c     Envoi des cadrans aux clients                                              */
/* Projet WatchDog version 2.0       Gestion d'habitat                                          dim 22 mai 2005 17:35:28 CEST */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * envoi_synoptique_cadran.c
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
 #include <string.h>
 #include <unistd.h>

/**************************************************** Prototypes de fonctions *************************************************/
 #include "watchdogd.h"
 #include "Sous_serveur.h"
/******************************************************************************************************************************/
/* Proto_effacer_cadran_atelier: Retrait du cadran en parametre                                                               */
/* Entr?e: le client demandeur et le cadran en question                                                                       */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 void Proto_effacer_cadran_atelier ( struct CLIENT *client, struct CMD_TYPE_CADRAN *rezo_cadran )
  { gboolean retour;
    retour = Retirer_cadranDB( rezo_cadran );

    if (retour)
     { Envoi_client( client, TAG_ATELIER, SSTAG_SERVEUR_ATELIER_DEL_CADRAN_OK,
                     (gchar *)rezo_cadran, sizeof(struct CMD_TYPE_CADRAN) );
     }
    else
     { struct CMD_GTK_MESSAGE erreur;
       g_snprintf( erreur.message, sizeof(erreur.message),
                   "Unable to delete cadran %s", rezo_cadran->libelle);
       Envoi_client( client, TAG_GTK_MESSAGE, SSTAG_SERVEUR_ERREUR,
                     (gchar *)&erreur, sizeof(struct CMD_GTK_MESSAGE) );
     }
  }
/******************************************************************************************************************************/
/* Proto_ajouter_cadran_atelier: Ajout d'un cadran dans un synoptique                                                         */
/* Entr?e: le client demandeur et le cadran en question                                                                       */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 void Proto_ajouter_cadran_atelier ( struct CLIENT *client, struct CMD_TYPE_CADRAN *rezo_cadran )
  { struct CMD_TYPE_CADRAN *result;
    gint id;

    id = Ajouter_cadranDB ( rezo_cadran );
    if (id == -1)
     { struct CMD_GTK_MESSAGE erreur;
       g_snprintf( erreur.message, sizeof(erreur.message),
                   "Unable to add cadran %s", rezo_cadran->libelle );
       Envoi_client( client, TAG_GTK_MESSAGE, SSTAG_SERVEUR_ERREUR,
                     (gchar *)&erreur, sizeof(struct CMD_GTK_MESSAGE) );
     }
    else { result = Rechercher_cadranDB( id );
           if (!result) 
            { struct CMD_GTK_MESSAGE erreur;
              g_snprintf( erreur.message, sizeof(erreur.message),
                          "Unable to locate cadran %d", id );
              Envoi_client( client, TAG_GTK_MESSAGE, SSTAG_SERVEUR_ERREUR,
                            (gchar *)&erreur, sizeof(struct CMD_GTK_MESSAGE) );
            }
           else
            { Envoi_client( client, TAG_ATELIER, SSTAG_SERVEUR_ATELIER_ADD_CADRAN_OK,
                            (gchar *)result, sizeof(struct CMD_TYPE_CADRAN) );
              g_free(result);
            }
         }
  }
/******************************************************************************************************************************/
/* Proto_valider_editer_cadran_atelier: Le client desire editer un cadran                                                     */
/* Entr?e: le client demandeur et le cadran en question                                                                       */
/* Sortie: Niet                                                                                                               */
/******************************************************************************************************************************/
 void Proto_valider_editer_cadran_atelier ( struct CLIENT *client, struct CMD_TYPE_CADRAN *rezo_cadran )
  { gboolean retour;
    retour = Modifier_cadranDB ( rezo_cadran );
    if (retour==FALSE)
     { struct CMD_GTK_MESSAGE erreur;
       g_snprintf( erreur.message, sizeof(erreur.message),
                   "Unable to save cadran %s", rezo_cadran->libelle);
       Envoi_client( client, TAG_GTK_MESSAGE, SSTAG_SERVEUR_ERREUR,
                     (gchar *)&erreur, sizeof(struct CMD_GTK_MESSAGE) );
     }
  }
/******************************************************************************************************************************/
/* Chercher_bit_cadrans: Renvoie 0 si l'element en argument est dans la liste                                                */
/* Entr?e: L'element                                                                                                          */
/* Sortie: 0 si present, 1 sinon                                                                                              */
/******************************************************************************************************************************/
 static gint Chercher_bit_cadrans ( struct CADRAN *element, struct CADRAN *cherche )
  { if (element->bit_controle == cherche->bit_controle &&
        element->type == cherche->type)
         return 0;
    else return 1;
  }
/******************************************************************************************************************************/
/* Envoyer_cadran_tag: Envoi des cadran au client en parametre                                                                */
/* Entr?e: Le client, le tag reseau et sous-tag                                                                               */
/* Sortie: n?ant                                                                                                              */
/******************************************************************************************************************************/
 void Envoyer_cadran_tag ( struct CLIENT *client, gint tag, gint sstag, gint sstag_fin )
  { struct CMD_TYPE_CADRAN *cadran;
    struct CMD_ENREG nbr;
    struct DB *db;

    if ( ! Recuperer_cadranDB( &db, client->syn_to_send->id ) )
     { return; }                                                                                    /* Si pas de cadrans (??) */

    nbr.num = db->nbr_result;
    if (nbr.num)
     { g_snprintf( nbr.comment, sizeof(nbr.comment), "Loading %d cadrans", nbr.num );
       Envoi_client ( client, TAG_GTK_MESSAGE, SSTAG_SERVEUR_NBR_ENREG,
                      (gchar *)&nbr, sizeof(struct CMD_ENREG) );
     }

    while ( (cadran = Recuperer_cadranDB_suite( &db )) != NULL )                      /* Pour tous les cadrans de la database */
     { Info_new( Config.log, Cfg_ssrv.lib->Thread_debug, LOG_DEBUG, 
                "Envoyer_cadran_tag: cadran %d (%s) to client %s",
                 cadran->id, cadran->libelle, client->machine );

       Envoi_client ( client, tag, sstag,                                                        /* Envoi du cadran au client */
                      (gchar *)cadran, sizeof(struct CMD_TYPE_CADRAN) );

       if (tag == TAG_SUPERVISION)                                          /* Si mode supervision on envoit la valeur d'init */
        { struct CMD_ETAT_BIT_CADRAN *init_cadran;
          struct CADRAN *cadran_new;
		  cadran_new = (struct CADRAN *) g_try_malloc0 ( sizeof(struct CADRAN) );
		  if (!cadran_new)
           { Info_new( Config.log, Cfg_ssrv.lib->Thread_debug, LOG_ERR,
                      "Envoyer_cadran_tag: Memory Error for %d (%s)", cadran->id, cadran->libelle );
           }
          else
           { cadran_new->type         = cadran->type;
			 cadran_new->bit_controle = cadran->bit_controle;
			 
             init_cadran = Formater_cadran(cadran_new);                                   /* Formatage de la chaine associ?e */
             if (init_cadran)                                                            /* envoi la valeur d'init au client */
              { Envoi_client( client, TAG_SUPERVISION, SSTAG_SERVEUR_SUPERVISION_CHANGE_CADRAN,
                              (gchar *)init_cadran, sizeof(struct CMD_ETAT_BIT_CADRAN) );
                g_free(init_cadran);                                                                 /* On libere la m?moire */
              }
             else { Info_new( Config.log, Cfg_ssrv.lib->Thread_debug, LOG_ERR,
                             "Envoyer_cadran_tag: Formater_cadran failed for %d (%s)", cadran->id, cadran->libelle );
                  }

             if ( ! g_slist_find_custom(client->Liste_bit_cadrans, cadran_new, (GCompareFunc) Chercher_bit_cadrans) )
              { client->Liste_bit_cadrans = g_slist_prepend( client->Liste_bit_cadrans, cadran_new ); }
             else g_free( cadran_new );                          /* si deja dans la liste, plus besoin de cette zone m?moire */
	      }
        }
       g_free(cadran);
     }
    Envoi_client ( client, tag, sstag_fin, NULL, 0 );
  }
/*----------------------------------------------------------------------------------------------------------------------------*/
