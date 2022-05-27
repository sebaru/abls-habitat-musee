<?php
defined('BASEPATH') OR exit('No direct script access allowed');

class Template {

    protected $CI;

    public function __construct()
     {	$this->CI =& get_instance(); }


    public function admin_render($content, $data = NULL)
     { if ( ! $content)
        { return NULL; }
        else
         { $this->template['header']          = $this->CI->load->view('admin/templates/header', $data, TRUE);
           $this->template['main_header']     = $this->CI->load->view('admin/templates/main_header', $data, TRUE);
           $this->template['main_sidebar']    = $this->CI->load->view('admin/templates/main_sidebar', $data, TRUE);
           $this->template['content']         = $this->CI->load->view($content, $data, TRUE);
           $this->template['footer']          = $this->CI->load->view('admin/templates/footer', $data, TRUE);
           return $this->CI->load->view('admin/templates/template', $this->template);
         }
	    }

}
