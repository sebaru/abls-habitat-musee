<?php
/*
 * Generated by CRUDigniter v3.2
 * www.crudigniter.com
 */

class Syn_model extends CI_Model
 { public function __construct()
    { parent::__construct(); }

/******************************************************************************************************************************/
    function get($id)
	    { $this->db->select("syn.id,syn.libelle,syn.page,syn.access_level,parent.id as pid,parent.page as ppage ");
       $this->db->from("syns as syn");
       $this->db->join("syns as parent", "syn.parent_id=parent.id", "INNER" );
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("syn.id=", $id );
       return $this->db->get()->row();
     }

/******************************************************************************************************************************/
    function get_all_syns_count()
     { $this->db->from('syns');
       return $this->db->count_all_results();
     }

/******************************************************************************************************************************/
    function motifs($id)
	    { $this->db->select("sm.*");/*, visu.libelle as libelle_visu");*/
       $this->db->from("syns_motifs as sm");
       $this->db->join("syns as syn", "sm.syn_id = syn.id", "INNER" );
       /*$this->db->join("mnemos_I as i", "sm.tech_id = i.tech_id AND sm.acronyme=i.acronyme", "INNER" );*/
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("syn.id=", $id );
       return $this->db->get()->result();
     }
/******************************************************************************************************************************/
    function get_motif($id)
	    { $this->db->select("sm.*");/*, visu.libelle as libelle_visu");*/
       $this->db->from("syns_motifs as sm");
       $this->db->join("syns as syn", "sm.syn_id = syn.id", "INNER" );
       /*$this->db->join("mnemos_I as i", "sm.tech_id = i.tech_id AND sm.acronyme=i.acronyme", "INNER" );*/
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("sm.id=", $id );
       return $this->db->get()->result();
     }
/******************************************************************************************************************************/
    function update_motif($id,$params)
     { $this->db->where('id',$id);
       return $this->db->update('syns_motifs',$params);
     }
/******************************************************************************************************************************/
    function delete_motif($id)
     { $this->db->where('access_level<=', $this->session->user_access_level);
       return $this->db->delete('syns_motifs',array('id'=>$id));
     }
/******************************************************************************************************************************/
    function add_motif($params)
     {  $this->db->insert('syns_motifs',$params);
       return $this->db->insert_id();
     }
/******************************************************************************************************************************/
    function comments($id)
	    { $this->db->select("sc.* ");
       $this->db->from("syns_comments as sc");
       $this->db->join("syns as syn", "sc.syn_id = syn.id", "INNER" );
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("syn.id=", $id );
       return $this->db->get()->result();
     }
/******************************************************************************************************************************/
    function get_comment($id)
	    { $this->db->select("sc.* ");
       $this->db->from("syns_comments as sc");
       $this->db->join("syns as syn", "sc.syn_id = syn.id", "INNER" );
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("sc.id=", $id );
       return $this->db->get()->result();
     }
/******************************************************************************************************************************/
    function update_comment($id,$params)
     { $this->db->where('id',$id);
       return $this->db->update('syns_comments',$params);
     }
/******************************************************************************************************************************/
    function delete_comment($id)
     { return $this->db->delete('syns_comments',array('id'=>$id));
     }
/******************************************************************************************************************************/
    function add_comment($params)
     { $this->db->insert('syns_comments',$params);
       return $this->db->insert_id();
     }
/******************************************************************************************************************************/
    function passerelles($id)
	    { $this->db->select("sp.*, cible.page, cible.libelle ");
       $this->db->from("syns_pass as sp");
       $this->db->join("syns as syn", "sp.syn_id = syn.id", "INNER" );
       $this->db->join("syns as cible", "sp.syn_cible_id = cible.id", "INNER" );
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("syn.id=", $id );
       return $this->db->get()->result();
     }
/******************************************************************************************************************************/
	   public function get_total()
	    { $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->from("syns as syn");
		     $query = $this->db->select("COUNT(*) as num")->get();
		     $result = $query->row();
		     if(isset($result)) return $result->num;
		     return 0;
	    }

/******************************************************************************************************************************/
    function get_all($start, $length)
     { $this->db->select("syn.id,syn.libelle,syn.page,syn.access_level,parent.id as pid,parent.page as ppage ");
       $this->db->from("syns as syn");
       $this->db->join("syns as parent", "syn.parent_id=parent.id", "INNER" );
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       if ($length != 0) { $this->db->limit($length,$start); }
       $result = $this->db->get();
       error_log ( 'get_all_syns '. $this->db->last_query() );
       return $result;
     }

/******************************************************************************************************************************/
    function create($params)
     { $this->db->insert('syns',$params);
       return $this->db->insert_id();
     }

/******************************************************************************************************************************/
    function update($id,$params)
     { $this->db->where('id',$id);
       return $this->db->update('syns',$params);
     }

/******************************************************************************************************************************/
    function delete($id)
     { $this->db->where('access_level<=', $this->session->user_access_level);
       return $this->db->delete('syns',array('id'=>$id));
     }
/******************************************************************************************************************************/
    function liens($id)
	    { $this->db->select("sl.*");/*, visu.libelle as libelle_visu");*/
       $this->db->from("syns_liens as sl");
       $this->db->join("syns as syn", "sl.syn_id = syn.id", "INNER" );
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("syn.id=", $id );
       return $this->db->get()->result();
     }
/******************************************************************************************************************************/
    function update_lien($id,$params)
     { $this->db->where('id',$id);
       return $this->db->update('syns_liens',$params);
     }
/******************************************************************************************************************************/
    function delete_lien($id)
     { return $this->db->delete('syns_liens',array('id'=>$id));
     }
/******************************************************************************************************************************/
    function add_lien($params)
     { $this->db->insert('syns_liens',$params);
       return $this->db->insert_id();
     }
/******************************************************************************************************************************/
    function get_lien($id)
	    { $this->db->select("sl.*");/*, visu.libelle as libelle_visu");*/
       $this->db->from("syns_liens as sl");
       $this->db->join("syns as syn", "sl.syn_id = syn.id", "INNER" );
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("sl.id=", $id );
       return $this->db->get()->result();
     }
/******************************************************************************************************************************/
    function rectangles($id)
	    { $this->db->select("sr.*");/*, visu.libelle as libelle_visu");*/
       $this->db->from("syns_rectangles as sr");
       $this->db->join("syns as syn", "sr.syn_id = syn.id", "INNER" );
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("syn.id=", $id );
       return $this->db->get()->result();
     }
/******************************************************************************************************************************/
    function update_rectangle($id,$params)
     { $this->db->where('id',$id);
       return $this->db->update('syns_rectangles',$params);
     }
/******************************************************************************************************************************/
    function delete_rectangle($id)
     { return $this->db->delete('syns_rectangles',array('id'=>$id));
     }
/******************************************************************************************************************************/
    function add_rectangle($params)
     { $this->db->insert('syns_rectangles',$params);
       return $this->db->insert_id();
     }
/******************************************************************************************************************************/
    function get_rectangle($id)
	    { $this->db->select("sr.*");/*, visu.libelle as libelle_visu");*/
       $this->db->from("syns_rectangles as sr");
       $this->db->join("syns as syn", "sr.syn_id = syn.id", "INNER" );
       $this->db->where("syn.access_level<=", $this->session->user_access_level );
       $this->db->where("sr.id=", $id );
       return $this->db->get()->result();
     }
  }