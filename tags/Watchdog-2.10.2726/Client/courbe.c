/**********************************************************************************************************/
/* Client/courbe.c        Affichage des courbes synoptiques de supervision                                */
/* Projet WatchDog version 2.0       Gestion d'habitat                       sam 14 jan 2006 13:46:08 CET */
/* Auteur: LEFEVRE Sebastien                                                                              */
/**********************************************************************************************************/
/*
 * courbe.c
 * This file is part of Watchdog
 *
 * Copyright (C) 2007 - S�bastien Lefevre
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
 #include <sys/time.h>
 #include <gtkdatabox.h>
 #include <gtkdatabox_points.h>
 #include <gtkdatabox_lines.h>
 #include <gtkdatabox_grid.h>
 #include <gtkdatabox_markers.h>
 
 #include "Reseaux.h"
 #include "Config_cli.h"

 extern GList *Liste_pages;                                   /* Liste des pages ouvertes sur le notebook */  
 extern GtkWidget *Notebook;                                         /* Le Notebook de controle du client */
 extern GtkWidget *F_client;                                                     /* Widget Fenetre Client */
 extern struct CONFIG_CLI Config_cli;                          /* Configuration generale cliente watchdog */

/********************************* D�finitions des prototypes programme ***********************************/
 #include "protocli.h"

 static GtkWidget *F_source = NULL;
 static GtkWidget *Liste_source = NULL;
 enum
  {  COLONNE_ID,
     COLONNE_TYPE,
     COLONNE_TYPE_EA,
     COLONNE_OBJET,
     COLONNE_NUM,
     COLONNE_MIN,
     COLONNE_MAX,
     COLONNE_UNITE_STRING,
     COLONNE_LIBELLE,
     NBR_COLONNE
  };
 static GdkColor COULEUR_COURBE[NBR_MAX_COURBES]=
  { { 0x0, 0xFFFF, 0x0,    0x0    },
    { 0x0, 0x0,    0xFFFF, 0x0    },
    { 0x0, 0x0,    0x0,    0xFFFF },
    { 0x0, 0xFFFF, 0xFFFF, 0x0    },
    { 0x0, 0x0,    0xFFFF, 0xFFFF },
    { 0x0, 0xFFFF, 0x0,    0xFFFF },
  };

/**********************************************************************************************************/
/* Valeur_to_description_marker: Formate une chaine de caractere pour inform� de l'etat d'une entr�e      */
/* Entr�e: la courbe et la position x � formater                                                          */
/* sortie: la chaine de caractere (non freeable, non re-entrant)                                          */
/**********************************************************************************************************/
 static gchar *Valeur_to_description_marker ( struct COURBE *courbe, guint index_posx )
  { static gchar description[80];
    float valeur;
    
    switch(courbe->mnemo.mnemo_base.type)
           { case MNEMO_SORTIE:
             case MNEMO_ENTREE:
                  g_snprintf( description, sizeof(description),
                              "%s%04d=%d",
                              Type_bit_interne_court(courbe->mnemo.mnemo_base.type),
                              courbe->mnemo.mnemo_base.num,
                              ( (gint)courbe->Y[index_posx] % ENTREAXE_Y_TOR == 0 ? 0 : 1) );
                  break;
             case MNEMO_ENTREE_ANA:
                  valeur = courbe->Y[index_posx];
                  g_snprintf( description, sizeof(description),
                              "EA%04d=%8.2f %s", courbe->mnemo.mnemo_base.num, valeur, courbe->mnemo.mnemo_ai.unite );
                  break;
             default: g_snprintf( description, sizeof(description), "type unknown" );
           }
    return(description);
  }
/**********************************************************************************************************/
/* CB_zoom_databox: Appell� quand l'utilisateur fait un zoom sur le databox                               */
/* Entr�e: la page infos en question                                                                      */
/* sortie: TRUE                                                                                           */
/**********************************************************************************************************/
 gboolean CB_zoom_databox ( struct TYPE_INFO_COURBE *infos, GdkEvent *event, gpointer data )
  { gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON(infos->Check_rescale), FALSE );
    return(TRUE);
  }
/**********************************************************************************************************/
/* CB_deplacement_databox: Fonction appel�e qd on appuie sur un des boutons de l'interface                */
/* Entr�e: la page infos en question                                                                      */
/* sortie: TRUE                                                                                           */
/**********************************************************************************************************/
 gboolean CB_deplacement_databox ( struct TYPE_INFO_COURBE *infos, GdkEvent *event, gpointer data )
  { gfloat left, right, top, bottom;
    gchar date[128], *date_create;
    struct tm *temps;
    time_t time_select;
    guint cpt, posx_select;

    gtk_databox_get_visible_limits (GTK_DATABOX(infos->Databox), &left, &right, &top, &bottom);

    posx_select = gtk_databox_pixel_to_value_x (GTK_DATABOX(infos->Databox), (gint16) event->motion.x );

    for (cpt=0; cpt<NBR_MAX_COURBES; cpt++)                         /* Affichage des descriptions courbes */
     { if (infos->Courbes[cpt].actif)
        { gint index_posx;
          for(index_posx = 0; index_posx < infos->Courbes[cpt].taille_donnees; index_posx++)
           { if ( posx_select < infos->Courbes[cpt].X[index_posx] ) break;
           }
          if (index_posx>=infos->Courbes[cpt].taille_donnees) index_posx = infos->Courbes[cpt].taille_donnees-1;
          infos->Courbes[cpt].marker_select_x = infos->Courbes[cpt].X[index_posx];
          infos->Courbes[cpt].marker_select_y = infos->Courbes[cpt].Y[index_posx];
          gtk_databox_markers_set_label ( GTK_DATABOX_MARKERS(infos->Courbes[cpt].marker_select), 0, GTK_DATABOX_MARKERS_TEXT_S,
                                         Valeur_to_description_marker(&infos->Courbes[cpt], index_posx), TRUE );
          gtk_databox_graph_set_hide ( infos->Courbes[cpt].marker_select, FALSE );
        }
     }

    time_select = posx_select + COURBE_ORIGINE_TEMPS;
    temps = localtime( (time_t *)&time_select );
    strftime( date, sizeof(date), "%F %T", temps );
    date_create = g_locale_to_utf8( date, -1, NULL, NULL, NULL );

    gtk_entry_set_text( GTK_ENTRY(infos->Entry_date_select), date_create );
    g_free( date_create );
    gtk_widget_queue_draw (infos->Databox);
    return(FALSE);
  }
/**********************************************************************************************************/
/* CB_ajouter_editer_source: Fonction appel�e qd on appuie sur un des boutons de l'interface              */
/* Entr�e: la reponse de l'utilisateur et un flag precisant l'edition/ajout                               */
/* sortie: TRUE                                                                                           */
/**********************************************************************************************************/
 gboolean CB_sortir_databox ( struct TYPE_INFO_COURBE *infos, GdkEvent *event, gpointer data )
  { gint cpt;

    for (cpt=0; cpt<NBR_MAX_COURBES; cpt++)                         /* Affichage des descriptions courbes */
     { if (infos->Courbes[cpt].actif)
        { gtk_databox_graph_set_hide (infos->Courbes[cpt].marker_select, TRUE);
        }
     }
    gtk_widget_queue_draw (infos->Databox);
    return(FALSE);
  }
/**********************************************************************************************************/
/* CB_ajouter_editer_source: Fonction appel�e qd on appuie sur un des boutons de l'interface              */
/* Entr�e: la reponse de l'utilisateur et un flag precisant l'edition/ajout                               */
/* sortie: TRUE                                                                                           */
/**********************************************************************************************************/
 static gboolean CB_ajouter_retirer_courbe ( GtkDialog *dialog, gint reponse,
                                             struct TYPE_INFO_COURBE *infos )
  { GtkTreeSelection *selection;
    struct COURBE *new_courbe;
    struct CMD_TYPE_COURBE rezo_courbe;
    gchar *libelle, *unite;
    GtkTreeModel *store;
    GtkTreeIter iter;
    GList *lignes;
    guint nbr;

    if (reponse == GTK_RESPONSE_ACCEPT)
     {
       if( infos->Courbes[infos->slot_id].actif )                          /* Enleve la pr�c�dente courbe */
        { infos->Courbes[infos->slot_id].actif = FALSE;
          gtk_entry_set_text( GTK_ENTRY(infos->Entry[infos->slot_id]), "" );
          gtk_databox_graph_remove ( GTK_DATABOX(infos->Databox), infos->Courbes[infos->slot_id].index );
          gtk_databox_graph_remove ( GTK_DATABOX(infos->Databox), infos->Courbes[infos->slot_id].marker_select );
          gtk_databox_graph_remove ( GTK_DATABOX(infos->Databox), infos->Courbes[infos->slot_id].marker_last );
          gtk_widget_queue_draw (infos->Databox);
        }

       selection = gtk_tree_view_get_selection( GTK_TREE_VIEW(Liste_source) );
       store     = gtk_tree_view_get_model    ( GTK_TREE_VIEW(Liste_source) );

       nbr = gtk_tree_selection_count_selected_rows( selection );
       if (!nbr) return(FALSE);                                              /* Si rien n'est selectionn� */

       lignes = gtk_tree_selection_get_selected_rows ( selection, NULL );

       gtk_tree_model_get_iter( store, &iter, lignes->data );          /* Recuperation ligne selectionn�e */
       gtk_tree_model_get( store, &iter, COLONNE_ID, &rezo_courbe.num, -1 );               /* Recup du id */
       gtk_tree_model_get( store, &iter, COLONNE_TYPE, &rezo_courbe.type, -1 );          /* Recup du type */
       gtk_tree_model_get( store, &iter, COLONNE_LIBELLE, &libelle, -1 );
       memcpy( &rezo_courbe.libelle, libelle, sizeof(rezo_courbe.libelle) );
       g_free( libelle );
       rezo_courbe.slot_id = infos->slot_id;

                                                          /* Placement de la nouvelle courbe sur l'id gui */
       new_courbe = &infos->Courbes[infos->slot_id];
       new_courbe->actif = TRUE;                /* R�cup�ration des donn�es EANA dans la structure COURBE */
       new_courbe->mnemo.mnemo_base.type  = rezo_courbe.type;    /* R�cup�ration des donn�es EANA dans la structure COURBE */
       switch( new_courbe->mnemo.mnemo_base.type )
        { case MNEMO_ENTREE_ANA:
               new_courbe->mnemo.mnemo_base.num = rezo_courbe.num;
               gtk_tree_model_get( store, &iter, COLONNE_TYPE_EA, &new_courbe->mnemo.mnemo_ai.type, -1 );
               gtk_tree_model_get( store, &iter, COLONNE_MIN, &new_courbe->mnemo.mnemo_ai.min, -1 );
               gtk_tree_model_get( store, &iter, COLONNE_MAX, &new_courbe->mnemo.mnemo_ai.max, -1 );
               gtk_tree_model_get( store, &iter, COLONNE_UNITE_STRING, &unite, -1 );
               gtk_tree_model_get( store, &iter, COLONNE_LIBELLE, &libelle, -1 );
               g_snprintf( new_courbe->mnemo.mnemo_base.libelle, sizeof(new_courbe->mnemo.mnemo_base.libelle), "%s", libelle );
               g_snprintf( new_courbe->mnemo.mnemo_ai.unite,   sizeof(new_courbe->mnemo.mnemo_ai.unite),   "%s", unite );
               g_free(libelle);
               g_free(unite);
               break;
          case MNEMO_SORTIE:
          case MNEMO_ENTREE:
               new_courbe->mnemo.mnemo_base.id = rezo_courbe.num;
               new_courbe->mnemo.mnemo_base.num = rezo_courbe.num;
               gtk_tree_model_get( store, &iter, COLONNE_LIBELLE, &libelle, -1 );
               g_snprintf( new_courbe->mnemo.mnemo_base.libelle, sizeof(new_courbe->mnemo.mnemo_base.libelle), "%s", libelle );
               g_free(libelle);
               break;
        }
                                                          /* Placement de la nouvelle courbe sur l'id gui */
       gtk_tree_selection_unselect_iter( selection, &iter );
       g_list_foreach (lignes, (GFunc) gtk_tree_path_free, NULL);
       g_list_free (lignes);                                                        /* Liberation m�moire */

       Envoi_serveur( TAG_COURBE, SSTAG_CLIENT_ADD_COURBE,
                      (gchar *)&rezo_courbe, sizeof(struct CMD_TYPE_COURBE) );
     }
    else if (reponse == GTK_RESPONSE_REJECT)                            /* On retire la courbe de la visu */
     { rezo_courbe.slot_id = infos->slot_id;

       Envoi_serveur( TAG_COURBE, SSTAG_CLIENT_DEL_COURBE,
                      (gchar *)&rezo_courbe, sizeof(struct CMD_TYPE_COURBE) );

       new_courbe = &infos->Courbes[infos->slot_id];
       new_courbe->actif = FALSE;               /* R�cup�ration des donn�es EANA dans la structure COURBE */
       new_courbe->mnemo.mnemo_base.type  = 0;                   /* R�cup�ration des donn�es EANA dans la structure COURBE */
       gtk_databox_graph_remove ( GTK_DATABOX(infos->Databox), new_courbe->index );
       gtk_databox_graph_remove ( GTK_DATABOX(infos->Databox), new_courbe->marker_select );
       gtk_databox_graph_remove ( GTK_DATABOX(infos->Databox), new_courbe->marker_last );
       gtk_entry_set_text( GTK_ENTRY(infos->Entry[infos->slot_id]), " -- no info --" );
     }
    gtk_widget_destroy(F_source);
    F_source = NULL;
    return(TRUE);
  }
/**********************************************************************************************************/
/* Menu_ajouter_courbe: Demande au serveur de superviser une nouvelle courbe                              */
/* Entr�e: rien                                                                                           */
/* Sortie: Niet                                                                                           */
/**********************************************************************************************************/
 static void Menu_changer_courbe ( struct TYPE_INFO_COURBE *infos, guint num )
  { GtkWidget *frame, *hboite, *scroll;
    GtkTreeSelection *selection;
    GtkTreeViewColumn *colonne;
    GtkCellRenderer *renderer;
    GtkListStore *store;

    if (F_source) return;
    F_source = gtk_dialog_new_with_buttons( _("View curves"),
                                               GTK_WINDOW(F_client),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                               GTK_STOCK_REMOVE, GTK_RESPONSE_REJECT,
                                               GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT,
                                               NULL );
    g_signal_connect( F_source, "response",
                      G_CALLBACK(CB_ajouter_retirer_courbe), infos );

    gtk_widget_set_size_request (F_source, 800, 600);

    frame = gtk_frame_new("Curves/EntreeANA");                       /* Cr�ation de l'interface graphique */
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.5, 0.5 );
    gtk_container_set_border_width( GTK_CONTAINER(frame), 6 );
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG(F_source)->vbox ), frame, TRUE, TRUE, 0 );

    hboite = gtk_hbox_new( FALSE, 6 );
    gtk_container_set_border_width( GTK_CONTAINER(hboite), 6 );
    gtk_container_add( GTK_CONTAINER(frame), hboite );

    scroll = gtk_scrolled_window_new( NULL, NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
    gtk_box_pack_start( GTK_BOX(hboite), scroll, TRUE, TRUE, 0 );

    store = gtk_list_store_new ( NBR_COLONNE, G_TYPE_UINT,                                    /* Id (num) */
                                              G_TYPE_UINT,                                        /* Type */
                                              G_TYPE_UINT,                                     /* Type EA */
                                              G_TYPE_STRING,                                     /* Objet */
                                              G_TYPE_STRING,                 /* Num (id en string "EAxxx" */
                                              G_TYPE_FLOAT,                                       /* min */
                                              G_TYPE_FLOAT,                                       /* max */
                                              G_TYPE_STRING,                              /* Unite_string */
                                              G_TYPE_STRING                                    /* libelle */
                               );

    Liste_source = gtk_tree_view_new_with_model ( GTK_TREE_MODEL(store) );/* Creation de la vue */
    selection = gtk_tree_view_get_selection( GTK_TREE_VIEW(Liste_source) );
    gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
    gtk_container_add( GTK_CONTAINER(scroll), Liste_source );

    renderer = gtk_cell_renderer_text_new();                                 /* Colonne de l'id du source */
    colonne = gtk_tree_view_column_new_with_attributes ( _("Numero"), renderer,
                                                         "text", COLONNE_NUM,
                                                         NULL);
    gtk_tree_view_column_set_sort_column_id(colonne, COLONNE_NUM);                    /* On peut la trier */
    gtk_tree_view_append_column ( GTK_TREE_VIEW (Liste_source), colonne );

    renderer = gtk_cell_renderer_text_new();                                 /* Colonne de l'id du source */
    colonne = gtk_tree_view_column_new_with_attributes ( _("Objet"), renderer,
                                                         "text", COLONNE_OBJET,
                                                         NULL);
    gtk_tree_view_column_set_sort_column_id(colonne, COLONNE_OBJET);                  /* On peut la trier */
    gtk_tree_view_append_column ( GTK_TREE_VIEW (Liste_source), colonne );

    renderer = gtk_cell_renderer_text_new();                              /* Colonne du libelle de source */
    colonne = gtk_tree_view_column_new_with_attributes ( _("Min"), renderer,
                                                         "text", COLONNE_MIN,
                                                         NULL);
    gtk_tree_view_column_set_sort_column_id(colonne, COLONNE_MIN);                    /* On peut la trier */
    gtk_tree_view_append_column ( GTK_TREE_VIEW (Liste_source), colonne );

    renderer = gtk_cell_renderer_text_new();                                    /* Colonne du commentaire */
    colonne = gtk_tree_view_column_new_with_attributes ( _("Max"), renderer,
                                                         "text", COLONNE_MAX,
                                                         NULL);
    gtk_tree_view_column_set_sort_column_id (colonne, COLONNE_MAX);
    gtk_tree_view_append_column ( GTK_TREE_VIEW (Liste_source), colonne );

    renderer = gtk_cell_renderer_text_new();                                    /* Colonne du commentaire */
    colonne = gtk_tree_view_column_new_with_attributes ( _("Unit"), renderer,
                                                         "text", COLONNE_UNITE_STRING,
                                                         NULL);
    gtk_tree_view_column_set_sort_column_id (colonne, COLONNE_UNITE_STRING);
    gtk_tree_view_append_column ( GTK_TREE_VIEW (Liste_source), colonne );

    renderer = gtk_cell_renderer_text_new();                           /* Colonne du libelle de source */
    colonne = gtk_tree_view_column_new_with_attributes ( _("Description"), renderer,
                                                         "text", COLONNE_LIBELLE,
                                                         NULL);
    gtk_tree_view_column_set_sort_column_id(colonne, COLONNE_LIBELLE);                /* On peut la trier */
    gtk_tree_view_append_column ( GTK_TREE_VIEW (Liste_source), colonne );

    gtk_tree_view_set_rules_hint( GTK_TREE_VIEW(Liste_source), TRUE );              /* Pour faire beau */

    g_object_unref (G_OBJECT (store));                        /* nous n'avons plus besoin de notre modele */
    gtk_widget_show_all(F_source);                                /* Affichage de l'interface compl�te */

    infos->slot_id = num;                      /* Place de la prochaine courbe dans l'interface graphique */
    Envoi_serveur( TAG_COURBE, SSTAG_CLIENT_WANT_PAGE_SOURCE_FOR_COURBE, NULL, 0 );

  }
/**********************************************************************************************************/
/* Menu_changer_courbe1: Demande au serveur de changer la vue de la courbe 1                              */
/* Entr�e: rien                                                                                           */
/* Sortie: Niet                                                                                           */
/**********************************************************************************************************/
 static void Menu_changer_courbe1 ( struct TYPE_INFO_COURBE *infos )
  { Menu_changer_courbe( infos, 0 ); }
 static void Menu_changer_courbe2 ( struct TYPE_INFO_COURBE *infos )
  { Menu_changer_courbe( infos, 1 ); }
 static void Menu_changer_courbe3 ( struct TYPE_INFO_COURBE *infos )
  { Menu_changer_courbe( infos, 2 ); }
 static void Menu_changer_courbe4 ( struct TYPE_INFO_COURBE *infos )
  { Menu_changer_courbe( infos, 3 ); }
 static void Menu_changer_courbe5 ( struct TYPE_INFO_COURBE *infos )
  { Menu_changer_courbe( infos, 4 ); }
 static void Menu_changer_courbe6 ( struct TYPE_INFO_COURBE *infos )
  { Menu_changer_courbe( infos, 5 ); }
/**********************************************************************************************************/
/* Detruire_page_supervision: L'utilisateur veut fermer la page de supervision                            */
/* Entr�e: la page en question                                                                            */
/* Sortie: rien                                                                                           */
/**********************************************************************************************************/
 void Detruire_page_courbe( struct PAGE_NOTEBOOK *page )
  { struct TYPE_INFO_COURBE *infos;
    infos = (struct TYPE_INFO_COURBE *)page->infos;
    gtk_databox_graph_remove_all ( GTK_DATABOX(infos->Databox) );
    gtk_widget_destroy(infos->Databox);
  }

/**********************************************************************************************************/
/* Creer_page_courbe: Creation d'une page du notebook pour les courbes watchdog                           */
/* Entr�e: rien                                                                                           */
/* Sortie: rien                                                                                           */
/**********************************************************************************************************/
 void Creer_page_courbe ( gchar *libelle )
  { GdkColor grille = { 0x0, 0x7FFF, 0x7FFF, 0x7FFF };
    GtkWidget *bouton, *boite, *hboite, *vboite, *table, *table2, *separateur;
    struct TYPE_INFO_COURBE *infos;
    struct PAGE_NOTEBOOK *page;

    GdkColor fond   = { 0x0, 0x0, 0x0, 0x0 };

    page = (struct PAGE_NOTEBOOK *)g_try_malloc0( sizeof(struct PAGE_NOTEBOOK) );
    if (!page) return;
    
    page->infos = (struct TYPE_INFO_COURBE *)g_try_malloc0( sizeof(struct TYPE_INFO_COURBE) );
    infos = (struct TYPE_INFO_COURBE *)page->infos;
    if (!page->infos) { g_free(page); return; }

    page->type   = TYPE_PAGE_COURBE;
    Liste_pages  = g_list_append( Liste_pages, page );
    hboite = gtk_hbox_new( FALSE, 6 );
    page->child = hboite;
    gtk_container_set_border_width( GTK_CONTAINER(hboite), 6 );
 
    vboite = gtk_vbox_new( FALSE, 6 );
    gtk_box_pack_start( GTK_BOX(hboite), vboite, TRUE, TRUE, 0 );

    table = gtk_table_new ( 4, 6, FALSE );
    gtk_table_set_row_spacings( GTK_TABLE(table), 5 );
    gtk_table_set_col_spacings( GTK_TABLE(table), 5 );
    gtk_box_pack_start( GTK_BOX(vboite), table, FALSE, FALSE, 0 );

    bouton = gtk_button_new_from_stock( GTK_STOCK_EDIT );
    g_signal_connect_swapped( G_OBJECT(bouton), "clicked",
                              G_CALLBACK(Menu_changer_courbe1), infos );
    gtk_table_attach( GTK_TABLE(table), bouton, 1, 2, 0, 1, 0, 0, 0, 0 );
    infos->Entry[0] = gtk_entry_new();
    gtk_widget_modify_base (infos->Entry[0], GTK_STATE_NORMAL, &COULEUR_COURBE[0]);
    gtk_editable_set_editable( GTK_EDITABLE(infos->Entry[0]), FALSE );
    gtk_table_attach_defaults( GTK_TABLE(table), infos->Entry[0], 2, 3, 0, 1 );

    bouton = gtk_button_new_from_stock( GTK_STOCK_EDIT );
    g_signal_connect_swapped( G_OBJECT(bouton), "clicked",
                              G_CALLBACK(Menu_changer_courbe2), infos );
    gtk_table_attach( GTK_TABLE(table), bouton, 1, 2, 1, 2, 0, 0, 0, 0 );
    infos->Entry[1] = gtk_entry_new();
    gtk_widget_modify_base (infos->Entry[1], GTK_STATE_NORMAL, &COULEUR_COURBE[1]);
    gtk_editable_set_editable( GTK_EDITABLE(infos->Entry[1]), FALSE );
    gtk_table_attach_defaults( GTK_TABLE(table), infos->Entry[1], 2, 3, 1, 2 );

    bouton = gtk_button_new_from_stock( GTK_STOCK_EDIT );
    g_signal_connect_swapped( G_OBJECT(bouton), "clicked",
                              G_CALLBACK(Menu_changer_courbe3), infos );
    gtk_table_attach( GTK_TABLE(table), bouton, 1, 2, 2, 3, 0, 0, 0, 0 );
    infos->Entry[2] = gtk_entry_new();
    gtk_widget_modify_base (infos->Entry[2], GTK_STATE_NORMAL, &COULEUR_COURBE[2]);
    gtk_editable_set_editable( GTK_EDITABLE(infos->Entry[2]), FALSE );
    gtk_table_attach_defaults( GTK_TABLE(table), infos->Entry[2], 2, 3, 2, 3 );

    bouton = gtk_button_new_from_stock( GTK_STOCK_EDIT );
    g_signal_connect_swapped( G_OBJECT(bouton), "clicked",
                              G_CALLBACK(Menu_changer_courbe4), infos );
    gtk_table_attach( GTK_TABLE(table), bouton, 4, 5, 0, 1, 0, 0, 0, 0 );
    infos->Entry[3] = gtk_entry_new();
    gtk_widget_modify_base (infos->Entry[3], GTK_STATE_NORMAL, &COULEUR_COURBE[3]);
    gtk_editable_set_editable( GTK_EDITABLE(infos->Entry[3]), FALSE );
    gtk_table_attach_defaults( GTK_TABLE(table), infos->Entry[3], 5, 6, 0, 1 );

    bouton = gtk_button_new_from_stock( GTK_STOCK_EDIT );
    g_signal_connect_swapped( G_OBJECT(bouton), "clicked",
                              G_CALLBACK(Menu_changer_courbe5), infos );
    gtk_table_attach( GTK_TABLE(table), bouton, 4, 5, 1, 2, 0, 0, 0, 0 );
    infos->Entry[4] = gtk_entry_new();
    gtk_widget_modify_base (infos->Entry[4], GTK_STATE_NORMAL, &COULEUR_COURBE[4]);
    gtk_editable_set_editable( GTK_EDITABLE(infos->Entry[4]), FALSE );
    gtk_table_attach_defaults( GTK_TABLE(table), infos->Entry[4], 5, 6, 1, 2 );

    bouton = gtk_button_new_from_stock( GTK_STOCK_EDIT );
    g_signal_connect_swapped( G_OBJECT(bouton), "clicked",
                              G_CALLBACK(Menu_changer_courbe6), infos );
    gtk_table_attach( GTK_TABLE(table), bouton, 4, 5, 2, 3, 0, 0, 0, 0 );
    infos->Entry[5] = gtk_entry_new();
    gtk_widget_modify_base (infos->Entry[5], GTK_STATE_NORMAL, &COULEUR_COURBE[5]);
    gtk_editable_set_editable( GTK_EDITABLE(infos->Entry[5]), FALSE );
    gtk_table_attach_defaults( GTK_TABLE(table), infos->Entry[5], 5, 6, 2, 3 );

/****************************************** L'entry pour la date select ***********************************/
    infos->Entry_date_select = gtk_entry_new();
    gtk_editable_set_editable( GTK_EDITABLE(infos->Entry_date_select), FALSE );
    gtk_box_pack_start( GTK_BOX(vboite), infos->Entry_date_select, FALSE, FALSE, 0 );

/****************************************** La databox ****************************************************/
    gtk_databox_create_box_with_scrollbars_and_rulers ( &infos->Databox, &table2, TRUE, TRUE, FALSE, FALSE );
    gtk_box_pack_start( GTK_BOX(vboite), table2, TRUE, TRUE, 0 );
    g_signal_connect_swapped( G_OBJECT(infos->Databox), "zoomed",
                              G_CALLBACK(CB_zoom_databox), infos );

    gtk_databox_set_scale_type_x ( GTK_DATABOX (infos->Databox), GTK_DATABOX_SCALE_LINEAR );
    gtk_databox_set_scale_type_y ( GTK_DATABOX (infos->Databox), GTK_DATABOX_SCALE_LINEAR );
    gtk_widget_modify_bg (infos->Databox, GTK_STATE_NORMAL, &fond);

    g_signal_connect_swapped( G_OBJECT(infos->Databox), "motion_notify_event",
                              G_CALLBACK(CB_deplacement_databox), infos );
    gtk_widget_add_events( GTK_WIDGET(infos->Databox), GDK_LEAVE_NOTIFY_MASK );
    g_signal_connect_swapped( G_OBJECT(infos->Databox), "leave_notify_event",
                              G_CALLBACK(CB_sortir_databox), infos );

    infos->index_grille = gtk_databox_grid_new ( 3, 10, &grille, 1 );
    gtk_databox_graph_add (GTK_DATABOX (infos->Databox), infos->index_grille);

/**************************************** Boutons de controle *********************************************/
    boite = gtk_vbox_new( FALSE, 6 );
    gtk_box_pack_start( GTK_BOX(hboite), boite, FALSE, FALSE, 0 );

    bouton = gtk_button_new_from_stock( GTK_STOCK_CLOSE );
    gtk_box_pack_start( GTK_BOX(boite), bouton, FALSE, FALSE, 0 );
    g_signal_connect_swapped( G_OBJECT(bouton), "clicked",
                              G_CALLBACK(Detruire_page), page );

    separateur = gtk_hseparator_new();
    gtk_box_pack_start( GTK_BOX(boite), separateur, FALSE, FALSE, 0 );

    infos->Check_rescale = gtk_check_button_new_with_label( _("Temps reel") );
    gtk_box_pack_start( GTK_BOX(boite), infos->Check_rescale, FALSE, FALSE, 0 );
    gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON(infos->Check_rescale), TRUE );

    gtk_widget_show_all( page->child );
    gtk_notebook_append_page( GTK_NOTEBOOK(Notebook), page->child, gtk_label_new ( libelle ) );
  }
/**********************************************************************************************************/
/* Rafraichir_visu_source: Rafraichissement d'un source la liste � l'�cran                                */
/* Entr�e: une reference sur le source                                                                    */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 static void Rafraichir_visu_source( GtkTreeIter *iter, struct CMD_TYPE_MNEMO_FULL *source )
  { struct PAGE_NOTEBOOK *page;
    GtkTreeModel *store;
    gchar chaine[20], groupe_page[512], *unite;
    gdouble min, max;

    page = Page_actuelle();
    if (page->type != TYPE_PAGE_COURBE) return;                                            /* Bon type ?? */

    store = gtk_tree_view_get_model( GTK_TREE_VIEW(Liste_source) );              /* Acquisition du modele */

    g_snprintf( chaine, sizeof(chaine), "%s%04d", Type_bit_interne_court(source->mnemo_base.type), source->mnemo_base.num );
    g_snprintf( groupe_page, sizeof(groupe_page), "%s/%s/%s",
                source->mnemo_base.groupe, source->mnemo_base.page, source->mnemo_base.plugin_dls );

    if (source->mnemo_base.type == MNEMO_ENTREE_ANA)
     { min = source->mnemo_ai.min;
       max = source->mnemo_ai.max;
       unite = source->mnemo_ai.unite;
     }
    else
     { min = 0.0; max = 1.0; unite="On/Off"; }

    gtk_list_store_set ( GTK_LIST_STORE(store), iter,
                         COLONNE_ID, source->mnemo_base.num,
                         COLONNE_TYPE, source->mnemo_base.type,
                         COLONNE_TYPE_EA, source->mnemo_ai.type,
                         COLONNE_OBJET, groupe_page,
                         COLONNE_NUM, chaine,
                         COLONNE_MIN, min,
                         COLONNE_MAX, max,
                         COLONNE_UNITE_STRING, unite,
                         COLONNE_LIBELLE, source->mnemo_base.libelle,
                         -1
                       );
  }
/**********************************************************************************************************/
/* Afficher_un_source: Ajoute un source dans la liste des sources                                         */
/* Entr�e: une reference sur le source                                                                    */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 void Proto_afficher_une_source_for_courbe( struct CMD_TYPE_MNEMO_FULL *source )
  { GtkListStore *store;
    GtkTreeIter iter;

    store = GTK_LIST_STORE(gtk_tree_view_get_model( GTK_TREE_VIEW(Liste_source) ));
    gtk_list_store_append ( store, &iter );                                      /* Acquisition iterateur */
    Rafraichir_visu_source ( &iter, source );
  }
/**********************************************************************************************************/
/* Append_courbe: Ajoute une valeur � une courbe                                                          */
/* Entr�e: une courbe, et une structure append_courbe                                                     */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 gboolean Append_courbe ( struct TYPE_INFO_COURBE *infos, struct COURBE *courbe, struct CMD_APPEND_COURBE *append_courbe )
  { gfloat *new_X, *new_Y;

    append_courbe->date -= COURBE_ORIGINE_TEMPS; 

    if (!courbe->X || !courbe->Y) return(FALSE);
    if (courbe->X[0] < append_courbe->date - COURBE_NBR_HEURE_ARCHIVE*3600)/* Si premier enreg trop vieux */
     { memmove( courbe->X, courbe->X+1, (courbe->taille_donnees-1)*sizeof(gfloat));
       memmove( courbe->Y, courbe->Y+1, (courbe->taille_donnees-1)*sizeof(gfloat));
     }
    else                                     /* Sinon, on garde le premier enreg et on agrandit le tampon */
     { if (courbe->taille_donnees > 600000)
        { printf(" Depassement du nombre d'enregistrement \n");
          return(FALSE);
        }

       courbe->taille_donnees++;                                            /* Agrandissement du tableau */

       new_X = g_try_realloc ( courbe->X, courbe->taille_donnees * sizeof(gfloat) );
       new_Y = g_try_realloc ( courbe->Y, courbe->taille_donnees * sizeof(gfloat) );

       if (new_X && new_Y)
        { courbe->X = new_X;
          courbe->Y = new_Y;
                                                                         /* Suppression de l'ancien graph */
          
          if (courbe->index) gtk_databox_graph_remove ( GTK_DATABOX(infos->Databox), courbe->index );
          courbe->index = gtk_databox_lines_new ( courbe->taille_donnees,
                                                  courbe->X, courbe->Y,
                                                  &COULEUR_COURBE[append_courbe->slot_id], 1);
          gtk_databox_graph_add (GTK_DATABOX (infos->Databox), courbe->index);
        } else printf("Append_courbe : realloc failed\n");
     }
                  
    switch(courbe->mnemo.mnemo_base.type)
     { case MNEMO_ENTREE_ANA:
                { courbe->X[courbe->taille_donnees-1] = append_courbe->date*1.0;
                  courbe->Y[courbe->taille_donnees-1] = append_courbe->val_ech;
                  printf("2 - append courbe : X=%f, Y=%f\n", courbe->X[courbe->taille_donnees-1], courbe->Y[courbe->taille_donnees-1] );
                }
               break;
       case MNEMO_SORTIE:
       case MNEMO_ENTREE:
                { courbe->X[courbe->taille_donnees-1] = append_courbe->date*1.0;
                  courbe->Y[courbe->taille_donnees-1] = 1.0*(append_courbe->slot_id*ENTREAXE_Y_TOR +
                                                            (append_courbe->val_ech ? HAUTEUR_Y_TOR : 0));
                  printf("3 - append courbe : X=%f, Y=%f\n", courbe->X[courbe->taille_donnees-1], courbe->Y[courbe->taille_donnees-1] );
                }
               break;
       default: printf("type inconnu %d\n", courbe->mnemo.mnemo_base.type);
                return(FALSE);
     }
    if (courbe->marker_last)
     { courbe->marker_last_x = courbe->X[courbe->taille_donnees-1];
       courbe->marker_last_y = courbe->Y[courbe->taille_donnees-1];
       gtk_databox_markers_set_label ( GTK_DATABOX_MARKERS(courbe->marker_last), 0, GTK_DATABOX_MARKERS_TEXT_E,
                                       Valeur_to_description_marker(courbe, courbe->taille_donnees-1), TRUE );
     }
    return(TRUE);
  }
/**********************************************************************************************************/
/* Proto_append_courbe : Le serveur envoie des informations pour compl�ter une courbe � l'�cran           */
/* Entr�e: une reference sur la courbe                                                                    */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 void Proto_append_courbe( struct CMD_APPEND_COURBE *append_courbe )
  { struct TYPE_INFO_COURBE *infos;
    struct PAGE_NOTEBOOK *page;
    struct COURBE *courbe;

    page = Chercher_page_notebook( TYPE_PAGE_COURBE, 0, FALSE );
    if (!page) return;
    infos = (struct TYPE_INFO_COURBE *)page->infos;           /* R�cup�ration des meta donn�es de la page */

    courbe = &infos->Courbes[append_courbe->slot_id];
    if ( ! (courbe && courbe->actif) ) return;

    if ( ! Append_courbe(infos, courbe, append_courbe) )
     { struct CMD_TYPE_COURBE rezo_courbe;
       rezo_courbe.type    = append_courbe->type;
       rezo_courbe.slot_id = append_courbe->slot_id;/* On demande au serveur de ne plus nous envoyer les infos */
       rezo_courbe.num      = 0;
       Envoi_serveur( TAG_COURBE, SSTAG_CLIENT_DEL_COURBE,
                      (gchar *)&rezo_courbe, sizeof(struct CMD_TYPE_COURBE) );
     }
    else
     { if (gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(infos->Check_rescale) ) == TRUE )
        { gfloat left, right, top, bottom;
          gtk_databox_auto_rescale( GTK_DATABOX(infos->Databox), 0.1 );
          gtk_databox_get_visible_limits (GTK_DATABOX(infos->Databox), &left, &right, &top, &bottom);

          gtk_databox_set_total_limits (GTK_DATABOX(infos->Databox),  left,  right+250, top + 0.1*MAX_RESOLUTION, -0.1*MAX_RESOLUTION );
          gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(infos->Check_rescale), TRUE );
        }
       gtk_widget_queue_draw (infos->Databox);                                  /* Mise � jour du Databox */
     }
  }
/**********************************************************************************************************/
/* Proto_ajouter_courbe: Appeler lorsque le client recoit la reponse d'ajout de courbe par le serveur     */
/* Entr�e: une reference sur le source                                                                    */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 void Ajouter_courbe( struct CMD_TYPE_COURBE *courbe, struct TYPE_INFO_COURBE *infos, gboolean marker_last )
  { struct COURBE *new_courbe;
    gchar description[256];

    /* La nouvelle courbe va dans l'id courbe->slot_id */
    new_courbe                 = &infos->Courbes[courbe->slot_id];
    new_courbe->index          = NULL;
    new_courbe->taille_donnees = 0;
    new_courbe->X              = NULL;
    new_courbe->Y              = NULL;

    new_courbe->marker_select = gtk_databox_markers_new ( 1, &new_courbe->marker_select_x, &new_courbe->marker_select_y,
                                                          &COULEUR_COURBE[courbe->slot_id], 10,
                                                          GTK_DATABOX_MARKERS_TRIANGLE
                                                        );
    gtk_databox_graph_add (GTK_DATABOX (infos->Databox), new_courbe->marker_select);
    gtk_databox_graph_set_hide ( new_courbe->marker_select, TRUE );

    if (marker_last)
     { new_courbe->marker_last = gtk_databox_markers_new ( 1, &new_courbe->marker_last_x, &new_courbe->marker_last_y,
                                                           &COULEUR_COURBE[courbe->slot_id], 10,
                                                           GTK_DATABOX_MARKERS_TRIANGLE
                                                         );
       gtk_databox_graph_add (GTK_DATABOX (infos->Databox), new_courbe->marker_last);
     }
    else new_courbe->marker_last = NULL;

    gtk_widget_queue_draw (infos->Databox);

    switch(new_courbe->mnemo.mnemo_base.type)
     { case MNEMO_SORTIE:
       case MNEMO_ENTREE:
            g_snprintf( description, sizeof(description), "%s%04d  - %s",
                        Type_bit_interne_court(new_courbe->mnemo.mnemo_base.type),
                        new_courbe->mnemo.mnemo_base.num,
                        new_courbe->mnemo.mnemo_base.libelle );
            break;
       case MNEMO_ENTREE_ANA:
            g_snprintf( description, sizeof(description), "%s%04d - %s (%8.2f/%8.2f)",
                        Type_bit_interne_court(new_courbe->mnemo.mnemo_base.type),
                        new_courbe->mnemo.mnemo_ai.num,
                        new_courbe->mnemo.mnemo_base.libelle,
                        new_courbe->mnemo.mnemo_ai.min, new_courbe->mnemo.mnemo_ai.max );
            break;
       default: g_snprintf( description, sizeof(description), " -- type unknown -- " );
     }
    gtk_entry_set_text( GTK_ENTRY(infos->Entry[courbe->slot_id]), description );
  }
/**********************************************************************************************************/
/* Proto_ajouter_courbe: Appeler lorsque le client recoit la reponse d'ajout de courbe par le serveur     */
/* Entr�e: une reference sur le source                                                                    */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 void Proto_ajouter_courbe( struct CMD_TYPE_COURBE *courbe )
  { struct PAGE_NOTEBOOK *page;
    struct TYPE_INFO_COURBE *infos;

    page = Chercher_page_notebook( TYPE_PAGE_COURBE, 0, TRUE );               /* R�cup�ration page courbe */
    if (!page) return;
    infos = page->infos;
    if (!infos) return;

    Ajouter_courbe ( courbe, infos, TRUE );
  }
/**********************************************************************************************************/
/* Proto_start_courbe: Appeler lorsque le client recoit un premier bloc de valeur a afficher              */
/* Entr�e: une reference sur la courbe                                                                    */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 void Afficher_courbe( struct CMD_START_COURBE *start_courbe, struct TYPE_INFO_COURBE *infos )
  { struct COURBE *courbe;
    gfloat *new_X, *new_Y;
    gint cpt, i;

    courbe = &infos->Courbes[start_courbe->slot_id];

    printf(" Recu %d enreg pour le slot %d old enreg = %d\n", start_courbe->taille_donnees, start_courbe->slot_id, courbe->taille_donnees );
    if (!start_courbe->taille_donnees)
     { printf(" Recu aucun enregistrement, on sort \n");
       return;
     }

    if (courbe->taille_donnees > 600000)
     { printf(" Depassement du nombre d'enregistrement \n");
       return;
     }

    cpt = courbe->taille_donnees;                 /* Sauvegarde pour garnissage dans la boucle ci dessous */
    courbe->taille_donnees += start_courbe->taille_donnees;                  /* Agrandissement du tableau */

    new_X = g_try_realloc ( courbe->X, courbe->taille_donnees * sizeof(gfloat) );
    new_Y = g_try_realloc ( courbe->Y, courbe->taille_donnees * sizeof(gfloat) );

    if (new_X && new_Y)
     { courbe->X = new_X;
       courbe->Y = new_Y;

       for ( i=0; cpt < courbe->taille_donnees; cpt++, i++ )
        { courbe->X[cpt] = start_courbe->valeurs[i].date - COURBE_ORIGINE_TEMPS;
          courbe->Y[cpt] = start_courbe->valeurs[i].val_ech;
        }
                                                                         /* Suppression de l'ancien graph */
          
       if (courbe->index) gtk_databox_graph_remove ( GTK_DATABOX(infos->Databox), courbe->index );
       courbe->index = gtk_databox_lines_new ( courbe->taille_donnees,
                                               courbe->X, courbe->Y,
                                               &COULEUR_COURBE[start_courbe->slot_id], 1);
       gtk_databox_graph_add (GTK_DATABOX (infos->Databox), courbe->index);
     } else printf("Afficher_courbe : realloc failed\n");

    courbe->marker_select_x = courbe->X[0];
    courbe->marker_select_y = courbe->Y[0];
/*    gtk_databox_auto_rescale( GTK_DATABOX(infos->Databox), 0.1 );
      gtk_widget_queue_draw (infos->Databox);
*/
  }
/**********************************************************************************************************/
/* Proto_start_courbe: Appeler lorsque le client recoit un premier bloc de valeur a afficher              */
/* Entr�e: une reference sur la courbe                                                                    */
/* Sortie: N�ant                                                                                          */
/**********************************************************************************************************/
 void Proto_start_courbe( struct CMD_START_COURBE *start_courbe )
  { struct PAGE_NOTEBOOK *page;
    struct TYPE_INFO_COURBE *infos;

    page = Chercher_page_notebook( TYPE_PAGE_COURBE, 0, TRUE );               /* R�cup�ration page courbe */
    if (!page) return;
    infos = page->infos;
    if (!infos) return;

    Afficher_courbe ( start_courbe, infos );
  }
/*--------------------------------------------------------------------------------------------------------*/
