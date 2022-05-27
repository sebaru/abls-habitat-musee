/**********************************************************************************************************/
/* Include/Reseaux_supervision.h:   Sous_tag de supervision pour watchdog 2.0 par lefevre Sebastien       */
/* Projet WatchDog version 2.0       Gestion d'habitat                       mar 21 fév 2006 13:46:48 CET */
/* Auteur: LEFEVRE Sebastien                                                                              */
/**********************************************************************************************************/
/*
 * Reseaux_supervision.h
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

#ifndef _RESEAUX_SUPERVISION_H_
 #define _RESEAUX_SUPERVISION_H_

 struct CMD_ETAT_BIT_CTRL
  { guint   num;
    guchar  etat;
    guchar  rouge;
    guchar  vert;
    guchar  bleu;
    guchar  cligno;
  };
 struct CMD_ETAT_BIT_CLIC
  { guint   num;
  };
 struct CMD_WANT_SCENARIO_MOTIF
  { guint   bit_clic;
    guint   bit_clic2;
  };
 struct CMD_ETAT_BIT_CAPTEUR
  { guint   bit_controle;
    guint   type;
    gchar   libelle[25];
  };

 enum 
  { SSTAG_CLIENT_WANT_PAGE_SUPERVISION,              /* Le client desire la page de supervision graphique */
    SSTAG_SERVEUR_AFFICHE_PAGE_SUP,
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_MOTIF,          /* Le serveur envoi des motifs page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_MOTIF_FIN,      /* Le serveur envoi des motifs page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_COMMENT,      /* Le serveur envoi des comments page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_COMMENT_FIN,  /* Le serveur envoi des comments page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_PASS,      /* Le serveur envoi des passerelles page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_PASS_FIN,  /* Le serveur envoi des passerelles page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_PALETTE,      /* Le serveur envoi des palettes page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_PALETTE_FIN,  /* Le serveur envoi des palettes page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_CAPTEUR,      /* Le serveur envoi des capteurs page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_CAPTEUR_FIN,  /* Le serveur envoi des capteurs page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_CAMERA_SUP,     /* Le serveur envoi des camera page supervision */
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_CAMERA_SUP_FIN, /* Le serveur envoi des camera page supervision */

    SSTAG_SERVEUR_SUPERVISION_CHANGE_MOTIF,        /* Un motif à changé d'etat, nous l'envoyons au client */
    SSTAG_CLIENT_CHANGE_MOTIF_UNKNOWN,      /* Reponse si le numero Ixxx n'est pas utilisé dans le client */
    SSTAG_SERVEUR_SUPERVISION_CHANGE_CAPTEUR,    /* Un capteur à changé d'etat, nous l'envoyons au client */
    SSTAG_CLIENT_CHANGE_CAPTEUR_UNKNOWN,    /* Reponse si le numero Ixxx n'est pas utilisé dans le client */
    SSTAG_CLIENT_ACTION_M,                                        /* Le client envoie un bit M au serveur */
    SSTAG_CLIENT_SUP_WANT_SCENARIO,                     /* Récupération des scenarios associés à un motif */
    SSTAG_CLIENT_SUP_ADD_SCENARIO,
    SSTAG_CLIENT_SUP_DEL_SCENARIO,
    SSTAG_CLIENT_SUP_EDIT_SCENARIO,
    SSTAG_CLIENT_SUP_VALIDE_EDIT_SCENARIO,
    SSTAG_SERVEUR_SUPERVISION_ADD_SCENARIO_OK,
    SSTAG_SERVEUR_SUPERVISION_DEL_SCENARIO_OK,
    SSTAG_SERVEUR_SUPERVISION_EDIT_SCENARIO_OK,
    SSTAG_SERVEUR_SUPERVISION_VALIDE_EDIT_SCENARIO_OK,
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_SCENARIO,
    SSTAG_SERVEUR_ADDPROGRESS_SUPERVISION_SCENARIO_FIN,

  };

#endif
/*--------------------------------------------------------------------------------------------------------*/

