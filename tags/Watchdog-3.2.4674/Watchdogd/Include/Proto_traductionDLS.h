/******************************************************************************************************************************/
/* Watchdogd/Include/Proto_traductionDLS.h     Interpretation du langage DLS                                                  */
/* Projet WatchDog version 2.0       Gestion d'habitat                                           ven 23 nov 2007 20:33:19 CET */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * Proto_traductionDLS.h
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

 #ifndef _PROTO_TRADDLS_H_
 #define _PROTO_TRADDLS_H_

 #include <glib.h>
 #include "Erreur.h"

 enum { TRAD_DLS_OK,                                                                   /* Retour de la fonction Traduire DLS. */
        TRAD_DLS_WARNING,
        TRAD_DLS_ERROR,
        TRAD_DLS_ERROR_FILE
      };

 struct ACTION
  { gchar *alors;                                                          /* Chaine pointant sur le nom du tableau (B/M/E..) */
    gchar *sinon;
  };


 struct OPTION
  { gint type;
    union { int entier;
            gchar *chaine;
          };
  };

 struct COMPARATEUR
  { gint type;
    union { gint val;
            gfloat valf;
          };
  };

 enum
  { ALIAS_TYPE_STATIC,
    ALIAS_TYPE_DYNAMIC
  };

 struct ALIAS
  { gint type;                                                                                 /* Alias static ou dynamique ? */
    gchar *tech_id;
    gchar *acronyme;
    gint type_bit;                                                                           /* Type de tableau (E/A/B/M....) */
    gint num;                                                                                  /* Numero du bit interne cibl?? */
    int barre;                                                                               /* Represente la negation ou pas */
    GList *options;
    gint used;
    gboolean external;                 /* TRUE si le bit interne est externe au module (ie pas d??clar?? en tant qu'alias local */
  };

/****************************************************** Prototypes ************************************************************/
 extern gint Traduire_DLS( gint id );                                                                        /* Dans Interp.c */
 extern void Trad_dls_set_debug ( gboolean actif );
 extern char *New_chaine( int longueur );
 extern void Emettre( char *chaine );
 extern void Emettre_erreur_new( gchar *format, ... );
 extern void Emettre_init_alias( void );
 extern struct COMPARATEUR *New_comparateur( void );
 extern gchar *New_condition_entree( int barre, struct ALIAS *alias, GList *options );
 extern gchar *New_condition_entree_ana( int barre, struct ALIAS *alias, GList *options, struct COMPARATEUR *comparateur );
 extern gchar *New_condition_sortie_ana( int barre, struct ALIAS *alias, GList *options, struct COMPARATEUR *comparateur );
 extern gchar *New_condition_bi( int barre, struct ALIAS *alias, GList *options );
 extern gchar *New_condition_mono( int barre, struct ALIAS *alias, GList *options );
 extern gchar *New_condition_tempo( int barre, struct ALIAS *alias, GList *options );
 extern gchar *New_condition_horloge( int barre, struct ALIAS *alias, GList *options );
 extern gchar *New_condition_vars( int barre, gchar *nom );
 extern struct ACTION *New_action( void );
 extern struct ACTION *New_action_msg( struct ALIAS *alias );
 extern struct ACTION *New_action_sortie( struct ALIAS *alias, int barre, GList *options );
 extern struct ACTION *New_action_digital_output( struct ALIAS *alias, GList *options );
 extern struct ACTION *New_action_vars_mono( gchar *nom );
 extern struct ACTION *New_action_bus( struct ALIAS *alias, GList *options );
 extern struct ACTION *New_action_mono( struct ALIAS *alias );
 extern struct ACTION *New_action_icone( struct ALIAS *alias, GList *options );
 extern struct ACTION *New_action_tempo( struct ALIAS *alias, GList *options );
 extern struct ACTION *New_action_bi( struct ALIAS *alias, gint barre );
 extern struct ACTION *New_action_cpt_h( struct ALIAS *alias, GList *options );
 extern struct ACTION *New_action_cpt_imp( struct ALIAS *alias, GList *options );
 extern gboolean New_alias( gint type, gchar *tech_id, gchar *acronyme, gint bit, gint num, gint barre, GList *options );
 extern struct ALIAS *Set_new_external_alias( gchar *tech_id, gchar *acronyme );
 extern struct ALIAS *Get_alias_par_acronyme( gchar *tech_id, gchar *acronyme );
 extern struct OPTION *New_option( void );
 /*extern int Get_option_entier( GList *liste_options, gint type );*/
 extern void Liberer_options ( GList *options );
 extern int DlsScanner_error ( char *s );
/* Fonctions mise a disposition par Flex et Bison */
 extern int  DlsScanner_lex (void);
 extern void DlsScanner_restart (FILE * input_file);
 extern int  DlsScanner_get_lineno (void );
 extern void DlsScanner_set_lineno (int _line_number);
 #endif
/*----------------------------------------------------------------------------------------------------------------------------*/
