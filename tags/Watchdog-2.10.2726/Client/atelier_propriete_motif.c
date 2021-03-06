/**********************************************************************************************************/
/* Client/atelier_propriete_motif.c         gestion des selections synoptique                             */
/* Projet WatchDog version 2.0       Gestion d'habitat                      sam 04 avr 2009 13:43:20 CEST */
/* Auteur: LEFEVRE Sebastien                                                                              */
/**********************************************************************************************************/
/*
 * atelier_propriete_motif.c
 * This file is part of Watchdog
 *
 * Copyright (C) 2008 - Sébastien Lefevre
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
 
 #include <gnome.h>
 #include <string.h>
 #include <stdlib.h>

 #include "Reseaux.h"
 #include "trame.h"

 static gchar *TYPE_GESTION_MOTIF[]=                      /* Type de gestion d'un motif sur un synoptique */
  { "Inerte (Mur)",
    "Fond d'ecran",
    "Actif (Lampe)",
    "Repos/Actif (Porte/Volet)",
    "Repos/Anime(0-n) (Moteur)",
    "Repos/Anime(1-n) (Moteur)",
    "Repos/Anime(2-n) (Moteur)",
    "Repos/Anime/Repos (Rideau)",
    "Inerte/Repos/Actif (Bouton)",
    NULL
  };

 static gchar *TYPE_DIALOG_CDE[]=                   /* Type de boite de dialogue lors d'un clic sur motif */
  { "Pas d'action",
    "immediate",
    "programme",
    NULL
  };

 #define RESOLUTION_TIMER        500
 #define TIMER_OFF     0
 #define TIMER_ON      1
    
 #define ROUGE1     0
 #define VERT1    255
 #define BLEU1      0

 extern GtkWidget *F_client;                                                     /* Widget Fenetre Client */
/********************************* Définitions des prototypes programme ***********************************/
 #include "protocli.h"

 extern GdkBitmap *Rmask, *Bmask, *Vmask, *Omask, *Jmask;                         /* Des pitites boules ! */
 extern GdkPixmap *Rouge, *Bleue, *Verte, *Orange, *Jaune;
 extern GtkWidget *F_trame;                       /* C'est bien le widget referencant la trame synoptique */

 static GtkWidget *F_propriete;                                      /* Pour acceder la fenetre graphique */
 static GtkWidget *Combo_gestion;                                            /* Type de gestion du motif */
 static GtkWidget *Option_dialog_cde;                      /* Type de boite de dialogue clic gauche motif */
 static GtkWidget *Combo_groupe;                                        /* groupe d'appartenance du motif */
 static GtkWidget *Spin_rafraich;                    /* Frequence de refraichissement d'un motif cyclique */
 static GtkWidget *Couleur_inactive;                                       /* Parametres visuels du motif */
 static GtkWidget *Entry_libelle;                                  /* Libelle du motif en cours d'edition */
 static GtkWidget *Spin_bit_clic;                                         /* Numero du bit de clic gauche */
 static GtkWidget *Entry_bit_clic;                                           /* Mnemonique du bit de clic */
 static GtkWidget *Spin_bit_clic2;                                        /* Numero du bit de clic gauche */
 static GtkWidget *Entry_bit_clic2;                                          /* Mnemonique du bit de clic */
 static GtkWidget *Spin_bit_ctrl;                                      /* Bit de controle (Ixxx) du motif */
 static GtkWidget *Entry_bit_ctrl;                                       /* Mnemonique du bit de controle */
 static struct TRAME *Trame_preview0;                             /* Previsualisation du motif par défaut */
 static struct TRAME *Trame_preview1;                                  /* Previsualisation du motif actif */
 static struct TRAME_ITEM_MOTIF *Trame_motif;                              /* Motif en cours de selection */
 static struct TRAME_ITEM_MOTIF *Trame_motif_p0;                           /* Motif en cours de selection */
 static struct TRAME_ITEM_MOTIF *Trame_motif_p1;                           /* Motif en cours de selection */
 static struct CMD_TYPE_MOTIF Motif_preview0;
 static struct CMD_TYPE_MOTIF Motif_preview1;
 static gint Tag_timer, ok_timer;                             /* Gestion des motifs cycliques/indicateurs */
 static GList *Liste_index_groupe; /* Pour correspondance index de l'option menu/Id du groupe en question */
/**********************************************************************************************************/
/* Type_gestion_motif: Renvoie le type correspondant au numero passé en argument                          */
/* Entrée: le numero du type                                                                              */
/* Sortie: le type                                                                                        */
/**********************************************************************************************************/
 static gchar *Type_gestion_motif ( gint num )
  { if (num>sizeof(TYPE_GESTION_MOTIF)/sizeof(TYPE_GESTION_MOTIF[0])-1)
     return("Type_gestion_motif: Erreur interne");
    return( TYPE_GESTION_MOTIF[num] );
  }
/**********************************************************************************************************/
/* Type_dialog_cde: Renvoie le type correspondant au numero passé en argument                             */
/* Entrée: le numero du type                                                                              */
/* Sortie: le type                                                                                        */
/**********************************************************************************************************/
 static gchar *Type_dialog_cde ( gint num )
  { if (num>sizeof(TYPE_DIALOG_CDE)/sizeof(TYPE_DIALOG_CDE[0])-1)
     return("Type_dialog_cde: Erreur interne");
    return( TYPE_DIALOG_CDE[num] );
  }
/**********************************************************************************************************/
/* Fonction TIMER  appélée toutes les 10 millisecondes    pour rafraichissment visuel                     */
/**********************************************************************************************************/
 static gint Timer_preview ( gpointer data )
  { static gint top=0;

    if (!ok_timer) return(TRUE);

    if (Trame_motif->motif->type_gestion == TYPE_DYNAMIQUE ||
        Trame_motif->motif->type_gestion == TYPE_BOUTON )
     { Trame_choisir_frame( Trame_motif_p1, Trame_motif_p1->num_image+1,
                                            ROUGE1, VERT1, BLEU1 );                    /* frame numero ++ */
     }
    else if (Trame_motif->motif->type_gestion == TYPE_CYCLIQUE_0N ||
             Trame_motif->motif->type_gestion == TYPE_PROGRESSIF
            )
     { if (top >= Trame_motif->motif->rafraich)
        { Trame_choisir_frame( Trame_motif_p1, Trame_motif_p1->num_image+1,
                                               ROUGE1, VERT1, BLEU1 );                 /* frame numero ++ */
          top = 0;                                                            /* Raz pour prochaine frame */
        }
     }
    else if (Trame_motif->motif->type_gestion == TYPE_CYCLIQUE_1N)
     { if (top >= Trame_motif->motif->rafraich)
        { if (Trame_motif_p1->num_image == Trame_motif_p1->nbr_images-1)
           { Trame_choisir_frame( Trame_motif_p1, 1,
                                  ROUGE1, VERT1, BLEU1 );                              /* frame numero ++ */
           }
          else
           { Trame_choisir_frame( Trame_motif_p1, Trame_motif_p1->num_image+1,
                                  ROUGE1, VERT1, BLEU1 );                              /* frame numero ++ */
           }
          top = 0;                                                            /* Raz pour prochaine frame */
        }
     }
    else if (Trame_motif->motif->type_gestion == TYPE_CYCLIQUE_2N)
     { if (top >= Trame_motif->motif->rafraich)
        { if (Trame_motif_p1->num_image == Trame_motif_p1->nbr_images-1)
           { Trame_choisir_frame( Trame_motif_p1, 2,
                                  ROUGE1, VERT1, BLEU1 );                              /* frame numero ++ */
           }
          else
           { Trame_choisir_frame( Trame_motif_p1, Trame_motif_p1->num_image+1,
                                  ROUGE1, VERT1, BLEU1 );                              /* frame numero ++ */
           }
          top = 0;                                                            /* Raz pour prochaine frame */
        }
     }
    top += RESOLUTION_TIMER;                                           /* sinon, la prochaine sera +10 ms */
    return(TRUE);
  }
/**********************************************************************************************************/
/* Rafraichir_sensibilite: met a jour la sensibilite des widgets de la fenetre propriete                  */
/* Entrée: void                                                                                           */
/* Sortie: void                                                                                           */
/**********************************************************************************************************/
 void Rafraichir_sensibilite ( void )
  { gtk_widget_set_sensitive( Spin_rafraich, FALSE );
    gtk_widget_set_sensitive( Spin_bit_ctrl, TRUE );
    gtk_widget_set_sensitive( Entry_bit_ctrl, TRUE );
    printf("Rafraichir_sensibilite1\n" );

    switch( Trame_motif->motif->type_gestion )
     { case TYPE_FOND:
       case TYPE_INERTE  :   gtk_widget_set_sensitive( Spin_bit_ctrl, FALSE );  /* Pas de bit de controle */
                             gtk_widget_set_sensitive( Entry_bit_ctrl, FALSE );
                             /*gtk_spin_button_set_value( GTK_SPIN_BUTTON(Spin_bit_ctrl), 0 );*/
                             Trame_choisir_frame( Trame_motif_p1, 0,
                                                                  Trame_motif->motif->rouge0,
                                                                  Trame_motif->motif->vert0,
                                                                  Trame_motif->motif->bleu0 ); /* frame 0 */
                             ok_timer = TIMER_OFF;
                             break;
       case TYPE_STATIQUE:   Trame_choisir_frame( Trame_motif_p1, 0, ROUGE1, VERT1, BLEU1 );   /* frame 0 */
                             ok_timer = TIMER_OFF;
                             break;
       case TYPE_BOUTON    :
       case TYPE_PROGRESSIF:
       case TYPE_DYNAMIQUE : Trame_choisir_frame( Trame_motif_p1, 1, ROUGE1, VERT1, BLEU1 );   /* frame 1 */
                             ok_timer = TIMER_ON;
                             break;
       case TYPE_CYCLIQUE_0N:
       case TYPE_CYCLIQUE_1N:
       case TYPE_CYCLIQUE_2N:gtk_widget_set_sensitive( Spin_rafraich, TRUE );
                             ok_timer = TIMER_ON;
                             printf("type cyclique: ok_timer = %d\n", ok_timer );
                             break;
       default: printf("Rafraichir_sensibilite: type_gestion inconnu\n" );
     }

    switch( Trame_motif->motif->type_dialog )
     { case ACTION_SANS      : gtk_widget_set_sensitive( Spin_bit_clic, FALSE );
                               gtk_widget_set_sensitive( Entry_bit_clic, FALSE );
                               gtk_widget_set_sensitive( Spin_bit_clic2, FALSE );
                               gtk_widget_set_sensitive( Entry_bit_clic2, FALSE );
                               gtk_widget_set_sensitive( Combo_groupe, FALSE );
                               break;
       case ACTION_IMMEDIATE : gtk_widget_set_sensitive( Spin_bit_clic, TRUE );
                               gtk_widget_set_sensitive( Entry_bit_clic, TRUE );
                               gtk_widget_set_sensitive( Spin_bit_clic2, FALSE );
                               gtk_widget_set_sensitive( Entry_bit_clic2, FALSE );
                               gtk_widget_set_sensitive( Combo_groupe, TRUE );
                               break;
       case ACTION_PROGRAMME : gtk_widget_set_sensitive( Spin_bit_clic, TRUE );
                               gtk_widget_set_sensitive( Entry_bit_clic, TRUE );
                               gtk_widget_set_sensitive( Spin_bit_clic2, TRUE );
                               gtk_widget_set_sensitive( Entry_bit_clic2, TRUE );
                               gtk_widget_set_sensitive( Combo_groupe, FALSE );
                               break;
     }
  }
/**********************************************************************************************************/
/* Changer_gestion_motif: Change le type de gestion de l'icone                                            */
/* Entrée: void                                                                                           */
/* Sortie: la base de données est mise ŕ jour                                                             */
/**********************************************************************************************************/
 static void Changer_gestion_motif ( void )
  { Trame_motif->motif->type_gestion = gtk_combo_box_get_active( GTK_COMBO_BOX(Combo_gestion) );
    printf("Gestion = %s\n", Type_gestion_motif( Trame_motif->motif->type_gestion) );
    Rafraichir_sensibilite();                           /* Pour mettre a jour les sensibility des widgets */
  }
/**********************************************************************************************************/
/* Changer_dialog_cde: Change le type de dialogue clic gauche motif                                       */
/* Entrée: rien                                                                                           */
/* Sortie: la base de données est mise ŕ jour                                                             */
/**********************************************************************************************************/
 static void Changer_dialog_cde ( void )
  { Trame_motif->motif->type_dialog = gtk_option_menu_get_history( GTK_OPTION_MENU(Option_dialog_cde) );
    printf("dialog = %s\n", Type_dialog_cde( Trame_motif->motif->type_gestion ) );
    Rafraichir_sensibilite();                           /* Pour mettre a jour les sensibility des widgets */
  }
/**********************************************************************************************************/
/* Changer_groupe: Change le groupe du motif                                                              */
/* Entrée: rien                                                                                           */
/* Sortie: la base de données est mise ŕ jour                                                             */
/**********************************************************************************************************/
 static void Changer_groupe ( void )
  { gint index_groupe;

    index_groupe = gtk_combo_box_get_active (GTK_COMBO_BOX (Combo_groupe) );
    Trame_motif->motif->gid = GPOINTER_TO_INT((g_list_nth( Liste_index_groupe, index_groupe ))->data);
printf("Changer groupe = %d\n", Trame_motif->motif->gid );
  }
/**********************************************************************************************************/
/* Changer_rafraich: Changement du taux de rafraichissement du motif                                      */
/* Entrée: widget, data                                                                                   */
/* Sortie: la base de données est mise ŕ jour                                                             */
/**********************************************************************************************************/
 static void Changer_rafraich( GtkWidget *widget, gpointer data )
  { Trame_motif->motif->rafraich = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(Spin_rafraich));
  }
/**********************************************************************************************************/
/* Changer_libelle_motif: Change le libellé du motif en fonction de la saisie utilisateur                 */
/* Entrée: widget/data                                                                                    */
/* Sortie: Rien du tout                                                                                   */
/**********************************************************************************************************/
 static void Changer_libelle_motif ( GtkWidget *widget, gpointer data )
  { g_snprintf( Trame_motif->motif->libelle, sizeof(Trame_motif->motif->libelle),
                "%s", gtk_entry_get_text( GTK_ENTRY(Entry_libelle) ) );
    printf("Changer_libelle_motif: new=%s\n", Trame_motif->motif->libelle );
  }
/**********************************************************************************************************/
/* Changer_couleur: Changement de la couleur du motif                                                     */
/* Entrée: widget, data = 0 pour un chgmt via propriete DLS, 1 pour chgmt via Couleur par Def             */
/* Sortie: la base de données est mise ŕ jour                                                             */
/**********************************************************************************************************/
 static void Changer_couleur( GtkWidget *widget, gpointer data )
  { guint8 r, v, b;
printf("Changer_couleur %p\n", data);

    if (widget == Couleur_inactive)                                     /* Changement de couleur inactive */
     { gnome_color_picker_get_i8( GNOME_COLOR_PICKER(Couleur_inactive), &r, &v, &b, NULL );
       Trame_motif->motif->rouge0 = r;
       Trame_motif->motif->vert0  = v;
       Trame_motif->motif->bleu0  = b;
       Trame_peindre_motif( Trame_motif, r, v, b );
       Trame_peindre_motif( Trame_motif_p0, r, v, b );
     }
    else                                                                     /* Rafraichissement direct ? */
     { gdouble coul[8];
       gtk_color_selection_get_color( GTK_COLOR_SELECTION(data), &coul[0] );
       Trame_motif->motif->rouge0 = (guchar)(coul[0]*255.0);
       Trame_motif->motif->vert0  = (guchar)(coul[1]*255.0);
       Trame_motif->motif->bleu0  = (guchar)(coul[2]*255.0);
       Trame_peindre_motif( Trame_motif, Trame_motif->motif->rouge0,
                                         Trame_motif->motif->vert0,
                                         Trame_motif->motif->bleu0 );
     }
  }
/**********************************************************************************************************/
/* Changer_couleur_motif_directe: Changement de la couleur du motif en direct live                        */
/* Entrée: widget, data =0 pour inactive, 1 pour active                                                   */
/* Sortie: la base de données est mise ŕ jour                                                             */
/**********************************************************************************************************/
 void Changer_couleur_motif_directe( struct TRAME_ITEM_MOTIF *trame_motif )
  { GtkWidget *fen, *choix;
    gdouble coul[8];

    Trame_motif = trame_motif;              /* On sauvegarde la reference pour peindre le motif plus tard */
    fen = gtk_dialog_new_with_buttons( _("Edit the default color"),
                                       GTK_WINDOW(F_client),
                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_STOCK_OK, GTK_RESPONSE_OK,
                                       NULL);
    g_signal_connect_swapped( fen, "response",
                              G_CALLBACK(gtk_widget_destroy), fen );

    choix = gtk_color_selection_new();          /* Creation de la zone de saisie de la couleur par defaut */
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG(fen)->vbox ), choix, TRUE, TRUE, 0 );

    coul[0] = trame_motif->motif->rouge0/255.0;
    coul[1] = trame_motif->motif->vert0/255.0;
    coul[2] = trame_motif->motif->bleu0/255.0;
    coul[3] = 0.0; coul[4] = 0.0;
    coul[5] = 0.0; coul[6] = 0.0;
    coul[7] = 0.0;

    gtk_color_selection_set_color( GTK_COLOR_SELECTION(choix), &coul[0] );
    g_signal_connect( G_OBJECT( choix ), "color_changed",
                      G_CALLBACK( Changer_couleur ), choix );
    gtk_widget_show_all( fen );
  }
/**********************************************************************************************************/
/* Rafraichir_propriete: Rafraichit les données de la fenetre d'edition des proprietes du motif           */
/* Entrée: Le trame_motif souhaité                                                                        */
/* Sortie: niet                                                                                           */
/**********************************************************************************************************/
 static void Rafraichir_propriete ( struct TRAME_ITEM_MOTIF *trame_motif )
  { struct CMD_TYPE_MOTIF *motif;
    GList *liste;
    gint i,cpt;

    Trame_motif = trame_motif;                  /* Sauvegarde pour les futurs changements d'environnement */
 
    if (!F_propriete) return;
    motif = Trame_motif->motif;

    gnome_color_picker_set_i8 ( GNOME_COLOR_PICKER(Couleur_inactive),
                                motif->rouge0, motif->vert0, motif->bleu0, 0 );

    memcpy( &Motif_preview0, trame_motif->motif, sizeof(struct CMD_TYPE_MOTIF) );/* Recopie et ajustement */
    memcpy( &Motif_preview1, trame_motif->motif, sizeof(struct CMD_TYPE_MOTIF) );/* Recopie et ajustement */
    Reduire_en_vignette( &Motif_preview0 );
    Reduire_en_vignette( &Motif_preview1 );
    Motif_preview0.position_x = Motif_preview1.position_x = TAILLE_ICONE_X/2;
    Motif_preview0.position_y = Motif_preview1.position_y = TAILLE_ICONE_Y/2;

    if (Trame_motif_p0)
     { goo_canvas_item_remove( Trame_motif_p0->item_groupe );
       Trame_preview0->trame_items = g_list_remove( Trame_preview0->trame_items, Trame_motif_p0 );
       g_object_unref( Trame_motif_p0->pixbuf );
       g_free(Trame_motif_p0);
     }    
    if (Trame_motif_p1)
     { goo_canvas_item_remove( Trame_motif_p1->item_groupe );
       Trame_preview1->trame_items = g_list_remove( Trame_preview1->trame_items, Trame_motif_p1 );
       g_object_unref( Trame_motif_p1->pixbuf );
       g_free(Trame_motif_p1);
     }    
    Trame_motif_p0 = Trame_ajout_motif( TRUE, Trame_preview0, &Motif_preview0 );   /* Affichage ŕ l'ecran */
    Trame_motif_p1 = Trame_ajout_motif( TRUE, Trame_preview1, &Motif_preview1 );

    g_signal_handlers_block_by_func( G_OBJECT( GTK_ENTRY(Entry_libelle) ),
                                     G_CALLBACK( Changer_libelle_motif ), NULL );
    gtk_entry_set_text( GTK_ENTRY(Entry_libelle), motif->libelle );
    g_signal_handlers_unblock_by_func( G_OBJECT( GTK_ENTRY(Entry_libelle) ),
                                       G_CALLBACK( Changer_libelle_motif ), NULL );

    printf("Rafraichir_proprietes1:  ctrl=%d clic=%d\n", motif->bit_controle, motif->bit_clic );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(Spin_bit_ctrl), motif->bit_controle );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(Spin_bit_clic), motif->bit_clic );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(Spin_bit_clic2), motif->bit_clic2 );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(Spin_rafraich), motif->rafraich );
    printf("Rafraichir_proprietes2:  ctrl=%d clic=%d\n", motif->bit_controle, motif->bit_clic );

    g_signal_handlers_block_by_func( G_OBJECT( GTK_COMBO_BOX(Combo_gestion) ),
                                     G_CALLBACK( Changer_gestion_motif ), NULL );
    for (i=0; i<NBR_TYPE_GESTION_MOTIF; i++) gtk_combo_box_remove_text( GTK_COMBO_BOX(Combo_gestion), 0 );
    printf("Nombre de images motif! %d....\n", trame_motif->nbr_images );
    gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_gestion), Type_gestion_motif( TYPE_INERTE   ) );
    gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_gestion), Type_gestion_motif( TYPE_FOND     ) );
    gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_gestion), Type_gestion_motif( TYPE_STATIQUE ) );
    printf("Rafraichir_proprietes3:  ctrl=%d clic=%d\n", motif->bit_controle, motif->bit_clic );

    if (trame_motif->nbr_images >= 2)                                    /* Sensibilite des boutons radio */
     { gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_gestion), Type_gestion_motif( TYPE_DYNAMIQUE   ) );
       gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_gestion), Type_gestion_motif( TYPE_CYCLIQUE_0N ) );
     }
    if (trame_motif->nbr_images >= 3)
     { gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_gestion), Type_gestion_motif( TYPE_CYCLIQUE_1N ) );
       gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_gestion), Type_gestion_motif( TYPE_CYCLIQUE_2N ) );
       gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_gestion), Type_gestion_motif( TYPE_PROGRESSIF  ) );
       gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_gestion), Type_gestion_motif( TYPE_BOUTON      ) );
     }
    gtk_widget_show_all(Combo_gestion);
    g_signal_handlers_unblock_by_func( G_OBJECT( GTK_COMBO_BOX(Combo_gestion) ),
                                       G_CALLBACK( Changer_gestion_motif ), NULL );

    gtk_combo_box_set_active( GTK_COMBO_BOX(Combo_gestion), motif->type_gestion );

    gtk_option_menu_set_history( GTK_OPTION_MENU(Option_dialog_cde), motif->type_dialog );
    printf("Rafraichir_proprietes8:  ctrl=%d clic=%d\n", motif->bit_controle, motif->bit_clic );

    cpt = 0;
    liste = Liste_index_groupe; 
    while (liste)
     { if ( liste->data == GINT_TO_POINTER(trame_motif->motif->gid) ) break;
       cpt++;
       liste = liste->next;
     }
    if (liste)
     { gtk_combo_box_set_active (GTK_COMBO_BOX (Combo_groupe), cpt );
       printf("Set history %d\n", cpt );
     }

    Rafraichir_sensibilite();  /* test 18/01/2006 */
    printf("rafraichir_propriete Oktimer = %d\n", ok_timer );
  }
/**********************************************************************************************************/
/* Proto_afficher_un_groupe_existant: ajoute un groupe dans la liste des groupes existants                */
/* Entrée: rien                                                                                           */
/* sortie: kedal                                                                                          */
/**********************************************************************************************************/
 void Proto_afficher_un_groupe_pour_propriete_synoptique ( struct CMD_TYPE_GROUPE *groupe )
  {
printf("Print groupe pour propriete synoptique \n");
    gtk_combo_box_append_text( GTK_COMBO_BOX(Combo_groupe), groupe->nom );
    Liste_index_groupe = g_list_append( Liste_index_groupe, GINT_TO_POINTER(groupe->id) );
printf("Print groupe pour propriete synoptique %s %d\n", groupe->nom, groupe->id);
  }
/**********************************************************************************************************/
/* Afficher_mnemo: Changement du mnemonique et affichage                                                  */
/* Entre: widget, data.                                                                                   */
/* Sortie: void                                                                                           */
/**********************************************************************************************************/
 void Proto_afficher_mnemo_atelier ( int tag, struct CMD_TYPE_MNEMO_BASE *mnemo )
  { gchar chaine[NBR_CARAC_LIBELLE_MNEMONIQUE_UTF8+10];
    snprintf( chaine, sizeof(chaine), "%s%04d  %s",
              Type_bit_interne_court(mnemo->type), mnemo->num, mnemo->libelle );             /* Formatage */
    switch (tag)
     { case SSTAG_SERVEUR_TYPE_NUM_MNEMO_CLIC:
            gtk_entry_set_text( GTK_ENTRY(Entry_bit_clic), chaine );
            break;
       case SSTAG_SERVEUR_TYPE_NUM_MNEMO_CLIC2:
            gtk_entry_set_text( GTK_ENTRY(Entry_bit_clic2), chaine );
            break;
       case SSTAG_SERVEUR_TYPE_NUM_MNEMO_CTRL:
            gtk_entry_set_text( GTK_ENTRY(Entry_bit_ctrl), chaine );
            break;
     }
  }
/**********************************************************************************************************/
/* Afficher_mnemo: Changement du mnemonique et affichage                                                  */
/* Entre: widget, data.                                                                                   */
/* Sortie: void                                                                                           */
/**********************************************************************************************************/
 static void Afficher_mnemo_clic ( void )
  { struct CMD_TYPE_NUM_MNEMONIQUE mnemo;

    mnemo.type = MNEMO_MONOSTABLE;
    mnemo.num = gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(Spin_bit_clic) );
    Trame_motif->motif->bit_clic = mnemo.num;

    Envoi_serveur( TAG_ATELIER, SSTAG_CLIENT_TYPE_NUM_MNEMO_CLIC,
                   (gchar *)&mnemo, sizeof( struct CMD_TYPE_NUM_MNEMONIQUE ) );
  }
/**********************************************************************************************************/
/* Afficher_mnemo: Changement du mnemonique et affichage                                                  */
/* Entre: widget, data.                                                                                   */
/* Sortie: void                                                                                           */
/**********************************************************************************************************/
 static void Afficher_mnemo_clic2 ( void )
  { struct CMD_TYPE_NUM_MNEMONIQUE mnemo;

    mnemo.type = MNEMO_MONOSTABLE;
    mnemo.num = gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(Spin_bit_clic2) );
    Trame_motif->motif->bit_clic2 = mnemo.num;

    Envoi_serveur( TAG_ATELIER, SSTAG_CLIENT_TYPE_NUM_MNEMO_CLIC2,
                   (gchar *)&mnemo, sizeof( struct CMD_TYPE_NUM_MNEMONIQUE ) );
  }
/**********************************************************************************************************/
/* Afficher_mnemo: Changement du mnemonique et affichage                                                  */
/* Entre: widget, data.                                                                                   */
/* Sortie: void                                                                                           */
/**********************************************************************************************************/
 static void Afficher_mnemo_ctrl ( void )
  { struct CMD_TYPE_NUM_MNEMONIQUE mnemo;
    mnemo.type = MNEMO_MOTIF;
    mnemo.num = gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(Spin_bit_ctrl) );
    Trame_motif->motif->bit_controle = mnemo.num;
    
    Envoi_serveur( TAG_ATELIER, SSTAG_CLIENT_TYPE_NUM_MNEMO_CTRL,
                   (gchar *)&mnemo, sizeof( struct CMD_TYPE_NUM_MNEMONIQUE ) );
  }
/**********************************************************************************************************/
/* Editer_propriete_TOR: Mise ŕ jour des parametres de la fenetre edition motif et du motif proprement dit*/
/* Entrée: une structure referencant le motif ŕ editer                                                    */
/* Sortie: niet                                                                                           */
/**********************************************************************************************************/
 void Editer_propriete_TOR ( struct TRAME_ITEM_MOTIF *trame_motif )
  { Rafraichir_propriete( trame_motif );             /* On rafraichit les données visuelles de la fenetre */
    gtk_widget_show( F_propriete );
    ok_timer = TIMER_ON;                                                                /* Arret du timer */
  }
/**********************************************************************************************************/
/* CB_editier_propriete_TOR: Fonction appelée qd on appuie sur un des boutons de l'interface              */
/* Entrée: la reponse de l'utilisateur et un flag precisant l'edition/ajout                               */
/* sortie: TRUE                                                                                           */
/**********************************************************************************************************/
 static gboolean CB_editer_propriete_TOR ( GtkDialog *dialog, gint reponse,
                                           gboolean edition )
  { switch(reponse)
     { case GTK_RESPONSE_OK:
       case GTK_RESPONSE_CANCEL: break;
     }
    ok_timer = TIMER_OFF;                                                               /* Arret du timer */ 
    gtk_widget_hide( F_propriete );
    return(TRUE);
  }
/**********************************************************************************************************/
/* Detruire_fenetre_propriete_TOR: Destruction de la fenetre de parametres DLS                            */
/* Entrée: rien                                                                                           */
/* Sortie: toute trace de la fenetre est eliminée                                                         */
/**********************************************************************************************************/
 void Detruire_fenetre_propriete_TOR ( void )
  {
    ok_timer = TIMER_OFF;
    gtk_timeout_remove( Tag_timer );                                  /* On le vire des fonctions actives */
    if (Trame_preview0) Trame_detruire_trame( Trame_preview0 );
    if (Trame_preview1) Trame_detruire_trame( Trame_preview1 );
    gtk_widget_destroy( F_propriete );
    g_list_free( Liste_index_groupe );
    Trame_motif_p0 = NULL;
    Trame_motif_p1 = NULL;
    F_propriete = NULL;
  }
/**********************************************************************************************************/
/* Creer_fenetre_propriete_TOR: Creation de la fenetre d'edition des proprietes TOR                       */
/* Entrée: niet                                                                                           */
/* Sortie: niet                                                                                           */
/**********************************************************************************************************/
 void Creer_fenetre_propriete_TOR ( struct TYPE_INFO_ATELIER *infos )
  { GtkWidget *texte, *table, *bouton, *separator;
    GtkWidget *boite, *Frame, *hboite, *menu;
    GtkObject *adj;
    gint cpt;

    F_propriete = gtk_dialog_new_with_buttons( _("Edit a item"),
                                               GTK_WINDOW(F_client),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_STOCK_OK, GTK_RESPONSE_OK,
                                               NULL);
    g_signal_connect( F_propriete, "response",
                      G_CALLBACK(CB_editer_propriete_TOR), FALSE );
    g_signal_connect( F_propriete, "delete-event",
                      G_CALLBACK(CB_editer_propriete_TOR), FALSE );

/*********************************** Frame de representation du motif actif *******************************/
    Frame = gtk_frame_new( _("Properties") );
    gtk_frame_set_label_align( GTK_FRAME(Frame), 0.5, 0.5 );
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG(F_propriete)->vbox ), Frame, TRUE, TRUE, 0 );

    boite = gtk_vbox_new( FALSE, 6 );
    gtk_container_add( GTK_CONTAINER(Frame), boite );
    gtk_container_set_border_width( GTK_CONTAINER(boite), 6 );

    Trame_preview0 = Trame_creer_trame( TAILLE_ICONE_X, TAILLE_ICONE_Y, "darkgray", 0 );
    gtk_widget_set_usize( Trame_preview0->trame_widget, TAILLE_ICONE_X, TAILLE_ICONE_Y );
    Trame_preview1 = Trame_creer_trame( TAILLE_ICONE_X, TAILLE_ICONE_Y, "darkgray", 0 );
    gtk_widget_set_usize( Trame_preview1->trame_widget, TAILLE_ICONE_X, TAILLE_ICONE_Y );
printf("Creer_fenetre_propriete_TOR: trame_p0=%p, trame_p1=%p\n", Trame_preview0, Trame_preview1 );

    hboite = gtk_hbox_new( FALSE, 6 );
    gtk_box_pack_start( GTK_BOX(boite), hboite, FALSE, FALSE, 0 );

    bouton = gtk_button_new();
    gtk_container_add( GTK_CONTAINER(bouton), Trame_preview0->trame_widget );
    gtk_box_pack_start( GTK_BOX(hboite), bouton, TRUE, FALSE, 0 );

    bouton = gtk_button_new();
    gtk_container_add( GTK_CONTAINER(bouton), Trame_preview1->trame_widget );
    gtk_box_pack_start( GTK_BOX(hboite), bouton, TRUE, FALSE, 0 );

    separator = gtk_hseparator_new();
    gtk_box_pack_start( GTK_BOX(boite), separator, FALSE, FALSE, 0 );

/********************************************** Proprietes DLS ********************************************/
    table = gtk_table_new(9, 4, TRUE);
    gtk_table_set_row_spacings( GTK_TABLE(table), 5 );
    gtk_table_set_col_spacings( GTK_TABLE(table), 5 );
    gtk_box_pack_start( GTK_BOX(boite), table, TRUE, TRUE, 0 );
    
    texte = gtk_label_new( _("Description") );
    gtk_table_attach_defaults( GTK_TABLE(table), texte, 0, 1, 0, 1 );

    Entry_libelle = gtk_entry_new();
    gtk_entry_set_max_length( GTK_ENTRY(Entry_libelle), NBR_CARAC_LIBELLE_MOTIF );
    gtk_table_attach_defaults( GTK_TABLE(table), Entry_libelle, 1, 4, 0, 1 );
    g_signal_connect( G_OBJECT(Entry_libelle), "changed",
                      G_CALLBACK(Changer_libelle_motif), NULL );

    texte = gtk_label_new( _("Type of response") );
    gtk_table_attach_defaults( GTK_TABLE(table), texte, 0, 2, 1, 2 );

    Combo_gestion = gtk_combo_box_new_text();
    gtk_table_attach_defaults( GTK_TABLE(table), Combo_gestion, 2, 4, 1, 2 );
    g_signal_connect( G_OBJECT( GTK_OPTION_MENU(Combo_gestion) ), "changed",
                      G_CALLBACK( Changer_gestion_motif ), NULL );

    texte = gtk_label_new( _("Control bit (I)") );
    gtk_table_attach_defaults( GTK_TABLE(table), texte, 0, 1, 2, 3 );

    Spin_bit_ctrl = gtk_spin_button_new_with_range( 0, NBR_BIT_DLS, 1 );
    g_signal_connect( G_OBJECT(Spin_bit_ctrl), "changed",
                      G_CALLBACK(Afficher_mnemo_ctrl), NULL );
    gtk_table_attach_defaults( GTK_TABLE(table), Spin_bit_ctrl, 1, 2, 2, 3 );

    Entry_bit_ctrl = gtk_entry_new();
    gtk_entry_set_editable( GTK_ENTRY(Entry_bit_ctrl), FALSE );
    gtk_table_attach_defaults( GTK_TABLE(table), Entry_bit_ctrl, 2, 4, 2, 3 );

    texte = gtk_label_new( _("Type of dialog box") );
    gtk_table_attach_defaults( GTK_TABLE(table), texte, 0, 2, 3, 4 );

    Option_dialog_cde = gtk_option_menu_new();
    gtk_table_attach_defaults( GTK_TABLE(table), Option_dialog_cde, 2, 4, 3, 4 );
    menu = gtk_menu_new();
    for (cpt=0; cpt<ACTION_NBR_ACTION; cpt++)
     { gtk_menu_shell_append( GTK_MENU_SHELL(menu),
                              gtk_menu_item_new_with_label( Type_dialog_cde( cpt ) ) );
     }
    gtk_option_menu_set_menu( GTK_OPTION_MENU(Option_dialog_cde), menu );
    g_signal_connect( G_OBJECT( GTK_OPTION_MENU(Option_dialog_cde) ), "changed",
                      G_CALLBACK( Changer_dialog_cde ), NULL );

    texte = gtk_label_new( _("First Action bit (M)") );
    gtk_table_attach_defaults( GTK_TABLE(table), texte, 0, 1, 4, 5 );

    Spin_bit_clic = gtk_spin_button_new_with_range( 0, NBR_BIT_DLS, 1 );
    g_signal_connect( G_OBJECT(Spin_bit_clic), "changed",
                      G_CALLBACK(Afficher_mnemo_clic), NULL );
    gtk_table_attach_defaults( GTK_TABLE(table), Spin_bit_clic, 1, 2, 4, 5 );

    Entry_bit_clic = gtk_entry_new();
    gtk_entry_set_editable( GTK_ENTRY(Entry_bit_clic), FALSE );
    gtk_table_attach_defaults( GTK_TABLE(table), Entry_bit_clic, 2, 4, 4, 5 );

    texte = gtk_label_new( _("(dont-use)Act.bit2 (M)") );
    gtk_table_attach_defaults( GTK_TABLE(table), texte, 0, 1, 5, 6 );

    Spin_bit_clic2 = gtk_spin_button_new_with_range( 0, NBR_BIT_DLS, 1 );
    g_signal_connect( G_OBJECT(Spin_bit_clic2), "changed",
                      G_CALLBACK(Afficher_mnemo_clic2), NULL );
    gtk_table_attach_defaults( GTK_TABLE(table), Spin_bit_clic2, 1, 2, 5, 6 );

    Entry_bit_clic2 = gtk_entry_new();
    gtk_entry_set_editable( GTK_ENTRY(Entry_bit_clic2), FALSE );
    gtk_table_attach_defaults( GTK_TABLE(table), Entry_bit_clic2, 2, 4, 5, 6 );

    texte = gtk_label_new( _("Delai between frame") );
    gtk_table_attach_defaults( GTK_TABLE(table), texte, 0, 2, 6, 7 );

    adj = gtk_adjustment_new( RESOLUTION_TIMER, RESOLUTION_TIMER, 10*RESOLUTION_TIMER,
                              RESOLUTION_TIMER, 10*RESOLUTION_TIMER, 0 );
    Spin_rafraich = gtk_spin_button_new( (GtkAdjustment *)adj, 0.5, 0.5);
    gtk_signal_connect( GTK_OBJECT(Spin_rafraich), "changed", GTK_SIGNAL_FUNC(Changer_rafraich), NULL );
    gtk_table_attach_defaults( GTK_TABLE(table), Spin_rafraich, 2, 4, 6, 7 );

    texte = gtk_label_new( _("Default color") );
    gtk_table_attach_defaults( GTK_TABLE(table), texte, 0, 2, 7, 8 );

    Couleur_inactive = gnome_color_picker_new();
    gtk_table_attach_defaults( GTK_TABLE(table), Couleur_inactive, 2, 4, 7, 8 );
    g_signal_connect( G_OBJECT(Couleur_inactive), "color_set",
                      G_CALLBACK(Changer_couleur), NULL );

    texte = gtk_label_new( _("Control group") );                                  /* Combo du type d'acces */
    gtk_table_attach_defaults( GTK_TABLE(table), texte, 0, 2, 8, 9 );
    Combo_groupe = gtk_combo_box_new_text();
    g_signal_connect( G_OBJECT(Combo_groupe), "changed",
                      G_CALLBACK(Changer_groupe), NULL );
    gtk_table_attach_defaults( GTK_TABLE(table), Combo_groupe, 2, 4, 8, 9 );
    Liste_index_groupe = NULL;

    ok_timer = TIMER_OFF;                                                       /* Timer = OFF par défaut */
    Tag_timer = gtk_timeout_add( RESOLUTION_TIMER, Timer_preview, NULL );      /* Enregistrement du timer */
    gtk_widget_show_all( Frame );                     /* On voit tout sauf la fenetre de plus haut niveau */
  }
/*--------------------------------------------------------------------------------------------------------*/
