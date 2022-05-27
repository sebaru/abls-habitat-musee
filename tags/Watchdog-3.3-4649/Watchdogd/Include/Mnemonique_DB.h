/******************************************************************************************************************************/
/* Watchdogd/Include/Mnemonique.h        Déclaration structure internes des mnemoniques watchdog                              */
/* Projet WatchDog version 2.0       Gestion d'habitat                                           mer 21 jan 2004 18:45:59 CET */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * Mnemonique_DB.h
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

#ifndef _MNEMONIQUE_H_
 #define _MNEMONIQUE_H_

 #include <glib.h>
 #include "Reseaux.h"
 #include "Db.h"

 #define NOM_TABLE_MNEMO          "mnemos"
 #define NOM_TABLE_MNEMO_AI       "mnemos_AnalogInput"
 #define NOM_TABLE_MNEMO_CPTH     "mnemos_CptHoraire"
 #define NOM_TABLE_MNEMO_CPTIMP   "mnemos_CptImp"
 #define NOM_TABLE_MNEMO_REGISTRE "mnemos_Registre"

/***************************************************** Définitions des prototypes *********************************************/
 extern struct CMD_TYPE_MNEMO_BASE *Rechercher_mnemo_baseDB ( guint id );
 extern gboolean Recuperer_mnemo_baseDB_with_conditions ( struct DB **db_retour, gchar *conditions, gint start, gint length );

 extern struct CMD_TYPE_MNEMO_BASE *Recuperer_mnemo_baseDB_suite( struct DB **db );
 extern gint Ajouter_mnemo_fullDB ( struct CMD_TYPE_MNEMO_FULL *mnemo );
 extern gboolean Retirer_mnemo_baseDB ( struct CMD_TYPE_MNEMO_BASE *mnemo );
 extern struct CMD_TYPE_MNEMO_BASE *Rechercher_mnemo_baseDB_by_acronyme ( gchar *tech_id, gchar *acronyme );
 extern struct CMD_TYPE_MNEMO_BASE *Rechercher_mnemo_baseDB_type_num ( struct CMD_TYPE_NUM_MNEMONIQUE *critere );
 extern struct CMD_TYPE_MNEMO_FULL *Rechercher_mnemo_fullDB ( guint id );
 extern struct CMD_TYPE_MNEMO_FULL *Rechercher_mnemo_fullDB_by_acronyme ( gchar*tech_id, gchar *acronyme );
 extern gboolean Modifier_mnemo_fullDB ( struct CMD_TYPE_MNEMO_FULL *mnemo_full );

 extern void Charger_analogInput ( void );                                                                 /* Dans Mnemo_AI.c */
 extern gboolean Mnemo_auto_create_AI ( gchar *tech_id, gchar *acronyme, gchar *libelle_src, gchar *unite_src );
 extern struct DB *Rechercher_AI ( gchar *tech_id, gchar *acronyme );
 extern void Charger_confDB_AI ( void );
 extern gboolean Recuperer_mnemos_AI_by_text ( struct DB **db_retour, gchar *thread, gchar *text );
 extern gboolean Recuperer_mnemos_AI_by_map_question_vocale ( struct DB **db_retour, gchar *map_snips );
 extern gboolean Recuperer_mnemos_AI_suite( struct DB **db_orig );
 extern void Updater_confDB_AI( void );
 extern void Dls_AI_to_json ( JsonBuilder *builder, struct DLS_AI *bit );

 extern gboolean Mnemo_auto_create_HORLOGE ( gchar *tech_id, gchar *acronyme, gchar *libelle_src ); /* Dans Mnemos_Horloges.c */
 extern void Activer_horlogeDB ( void );

 extern void Updater_confDB_CI ( void );                                                                   /* Dans Mnemo_CI.c */
 extern void Charger_conf_CI ( struct DLS_CI *cpt_imp );
 extern gboolean Mnemo_auto_create_CI ( gchar *tech_id, gchar *acronyme, gchar *libelle_src );
 extern struct DB *Rechercher_CI ( gchar *tech_id, gchar *acronyme );
 extern void Dls_CI_to_json ( JsonBuilder *builder, struct DLS_CI *bit );

 extern void Updater_confDB_CH ( void );                                                                   /* Dans Mnemo_CH.c */
 extern void Charger_conf_CH ( struct DLS_CH *cpt_h );
 extern gboolean Mnemo_auto_create_CH ( gchar *tech_id, gchar *acronyme, gchar *libelle_src );
 extern struct DB *Rechercher_CH ( gchar *tech_id, gchar *acronyme );
 extern void Dls_CH_to_json ( JsonBuilder *builder, struct DLS_CH *bit );

 extern gboolean Mnemo_auto_create_TEMPO ( gchar *tech_id, gchar *acronyme, gchar *libelle_src );       /* Dans Mnemo_tempo.c */
 extern struct DB *Rechercher_Tempo ( gchar *tech_id, gchar *acronyme );
 extern void Dls_TEMPO_to_json ( JsonBuilder *builder, struct DLS_TEMPO *bit );

 extern void Charger_confDB_Registre ( void );                                                       /* Dans Mnemo_registre.c */
 extern void Updater_confDB_Registre ( void );
 extern gboolean Mnemo_auto_create_REGISTRE ( gchar *tech_id, gchar *acronyme, gchar *libelle_src, gchar *unite_src );
 extern gboolean Recuperer_mnemos_R_by_map_question_vocale ( struct DB **db_retour, gchar *map_snips );
 extern gboolean Recuperer_mnemos_R_suite( struct DB **db_orig );
 extern void Dls_REGISTRE_to_json ( JsonBuilder *builder, struct DLS_REGISTRE *bit );

 extern gboolean Mnemo_auto_create_DI ( gchar *tech_id, gchar *acronyme, gchar *libelle );                /* Dans mnemos_DI.c */
 extern struct DB *Rechercher_DI ( gchar *tech_id, gchar *acronyme );
 extern gboolean Recuperer_mnemos_DI_by_text ( struct DB **db_retour, gchar *thread, gchar *text );
 extern gboolean Recuperer_mnemos_DI_suite( struct DB **db_orig );
 extern void Dls_DI_to_json ( JsonBuilder *builder, struct DLS_DI *bit );

 extern gboolean Mnemo_auto_create_DO ( gchar *tech_id, gchar *acronyme, gchar *libelle );                /* Dans mnemos_DO.c */
 extern struct DB *Rechercher_DO ( gchar *tech_id, gchar *acronyme );
 extern gboolean Recuperer_mnemos_DO_by_tag ( struct DB **db_retour, gchar *thread, gchar *tag );
 extern gboolean Recuperer_mnemos_DO_suite( struct DB **db_orig );
 extern void Dls_DO_to_json ( JsonBuilder *builder, struct DLS_DO *bit );

 extern struct DB *Rechercher_BOOL ( gchar *tech_id, gchar *acronyme );                                 /* Dans mnemos_BOOL.c */
 extern gboolean Mnemo_auto_create_BOOL ( gint type, gchar *tech_id, gchar *acronyme, gchar *libelle_src );
 extern void Charger_confDB_BOOL ( void );
 extern void Updater_confDB_BOOL ( void );
 extern void Dls_BOOL_to_json ( JsonBuilder *builder, struct DLS_BOOL *bit );

 extern gboolean Mnemo_auto_create_AO ( gchar *tech_id, gchar *acronyme, gchar *libelle );                /* Dans mnemos_AO.c */
 extern struct DB *Rechercher_AO ( gchar *tech_id, gchar *acronyme );
 extern gboolean Recuperer_mnemos_AO_by_text ( struct DB **db_retour, gchar *thread, gchar *text );
 extern gboolean Recuperer_mnemos_AO_suite( struct DB **db_orig );
 extern void Updater_confDB_AO ( void );
 extern void Dls_AO_to_json ( JsonBuilder *builder, struct DLS_AO *bit );

#endif
/*----------------------------------------------------------------------------------------------------------------------------*/
