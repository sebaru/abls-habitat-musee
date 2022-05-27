/******************************************************************************************************************************/
/* Watchdogd/Include/Module_dls.h -> Déclaration des prototypes de fonctions                                                  */
/* Projet WatchDog version 2.0       Gestion d'habitat                                          jeu 31 jui 2003 11:49:36 CEST */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * Module_dls.h
 * This file is part of Watchdog
 *
 * Copyright (C) 2010-2020 - Sebastien Lefevre
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

 #ifndef _MODULE_DLS_H_
 #define _MODULE_DLS_H_
 #include <glib.h>

 struct DLS_TO_PLUGIN                                                 /* structure dechange de données entre DLS et le plugin */
  { gboolean resetted;                                  /* 1 si les bits internes "start" du plugins doivent etre positionnés */
    gboolean debug;                                                 /* TRUE si le plugin doit logguer ses changements de bits */
    gint     num_ligne;                                                         /* N° de ligne du plugin en cours d'execution */
    gpointer bit_comm;
    gpointer bit_msg_comm_ok;
    gpointer bit_msg_comm_hs;
    gboolean bit_defaut;
    gboolean bit_defaut_fixe;
    gboolean bit_alarme;
    gboolean bit_alarme_fixe;
    gboolean bit_veille;
    gboolean bit_alerte;
    gboolean bit_alerte_fixe;
    gboolean bit_derangement;
    gboolean bit_derangement_fixe;
    gboolean bit_danger;
    gboolean bit_danger_fixe;
    gboolean bit_acquit;
    gboolean bit_activite_ok;
    gboolean bit_secupers_ok;
    gboolean bit_alerte_fugitive;
  };

 extern gboolean Dls_get_top_alerte ( void );
 extern gboolean Dls_get_top_alerte_fugitive ( void );
 extern gboolean Dls_data_get_BI        ( gchar *tech_id, gchar *acronyme, gpointer *bool_p );
 extern gboolean Dls_data_get_MONO      ( gchar *tech_id, gchar *acronyme, gpointer *bool_p );
 extern gboolean Dls_data_get_bool_up   ( gchar *tech_id, gchar *acronyme, gpointer *bool_p );
 extern gboolean Dls_data_get_bool_down ( gchar *tech_id, gchar *acronyme, gpointer *bool_p );
 extern void     Dls_data_set_BI        ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *bool_p, gboolean valeur );
 extern void     Dls_data_set_MONO      ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *bool_p, gboolean valeur );
 extern gboolean Dls_data_get_DI        ( gchar *tech_id, gchar *acronyme, gpointer *di_p );
 extern gboolean Dls_data_get_DI_up     ( gchar *tech_id, gchar *acronyme, gpointer *di_p );
 extern gboolean Dls_data_get_DI_down   ( gchar *tech_id, gchar *acronyme, gpointer *di_p );
 extern void     Dls_data_set_DO        ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *dout_p, gboolean valeur );
 extern void     Dls_data_set_MSG       ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *msg_p, gboolean update, gboolean etat );
 extern void     Dls_data_set_tempo     ( gchar *tech_id, gchar *acronyme, gpointer *tempo_p, gboolean etat,
                                          gint delai_on, gint min_on, gint max_on, gint delai_off, gint random);
 extern gfloat   Dls_data_get_AO        ( gchar *tech_id, gchar *acronyme, gpointer *ao_p );
 extern void     Dls_data_set_AO        ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *ao_p, gfloat val_avant_ech );
 extern gboolean Dls_data_get_tempo     ( gchar *tech_id, gchar *acronyme, gpointer *tempo_p );
 extern gboolean Dls_data_get_WATCHDOG ( gchar *tech_id, gchar *acronyme, gpointer *wtd_p );
 extern void     Dls_data_set_WATCHDOG ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *wtd_p, gint consigne );
 extern void Dls_data_set_bus ( gchar *tech_id, gchar *acronyme, gpointer *bus_p, gboolean etat,
                                gchar *host, gchar *thread, gchar *tag, gchar *param1);
 extern gfloat   Dls_data_get_AI        ( gchar *tech_id, gchar *acronyme, gpointer *ai_p );
 extern gboolean Dls_data_get_AI_inrange ( gchar *tech_id, gchar *acronyme, gpointer *ai_p );
 extern void Dls_data_set_CI ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *cpt_imp_p, gboolean etat, gint reset, gint ratio );
 extern gint Dls_data_get_CI ( gchar *tech_id, gchar *acronyme, gpointer *cpt_imp_p );
 extern void Dls_data_set_CH ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *cpt_h_p, gboolean etat, gint reset );
 extern gint Dls_data_get_CH ( gchar *tech_id, gchar *acronyme, gpointer *cpt_h_p );
 extern void Dls_data_set_R ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *r_p, gfloat valeur );
 extern gfloat Dls_data_get_R ( gchar *tech_id, gchar *acronyme, gpointer *r_p );
 extern gchar *Dls_dyn_string ( gchar *format, gint type_bit, gchar *tech_id, gchar *acronyme, gpointer *dlsdata_p );
 extern void Dls_data_set_VISUEL ( struct DLS_TO_PLUGIN *vars, gchar *tech_id, gchar *acronyme, gpointer *visuel_p, gint mode,
                                   gchar *color, gboolean cligno );

 extern int Heure( int heure, int minute );                                                        /* Tester l'heure actuelle */
 extern int Heure_avant( int heure, int minute );
 extern int Heure_apres( int heure, int minute );
 extern int Jour_semaine( int jour );                                     /* Sommes nous le jour de la semaine en parametre ? */
 #endif
/*----------------------------------------------------------------------------------------------------------------------------*/
