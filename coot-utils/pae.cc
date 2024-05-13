
#include <iostream>
#include <fstream>
#include "utils/coot-utils.hh"
#include "pae.hh"
#include <cairo.h>
#include "json.hpp"
using json = nlohmann::json;

std::string
pae_t::file_to_string(const std::string &file_name) const {

   std::string s;
   std::string line;
   std::ifstream f(file_name.c_str());
   if (!f) {
      std::cout << "Failed to open " << file_name << std::endl;
   } else {
      while (std::getline(f, line)) {
         s += line;
         s += "\n";
      }
   }
   return s;
}


pae_t::pae_t(const std::string &file_name, int n_pixels_in) {

   auto file_to_string = [] (const std::string &file_name) {
      std::fstream f(file_name);
      std::string s;
      f.seekg(0, std::ios::end);
      s.reserve(f.tellg());
      f.seekg(0, std::ios::beg);
      s.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
      return s;
   };

   n_pixels = n_pixels_in;
   if (coot::file_exists(file_name)) {
      std::string s = file_to_string(file_name);
      try {
         json j = json::parse(s);
         json item_1 = j[0];
         json ls = item_1["predicted_aligned_error"];
         std::vector<std::vector<int> > pae_vecs;
         for (json::iterator it_1=ls.begin(); it_1!=ls.end(); ++it_1) {
            json &item_2 = *it_1;
            // item_2 is a list with a number for each residue (self is 0)
            std::vector<int> arr = item_2;
            pae_vecs.push_back(arr);
         }
         bool is_square = true;
         unsigned int l = pae_vecs.size();
         for (auto &a : pae_vecs)
            if (a.size() != l) is_square = false;
         if (is_square) {
            image = make_image(pae_vecs);
         }

      }
      catch (const nlohmann::detail::type_error &e) {
         std::cout << "ERROR:: " << e.what() << std::endl;
      }
   }
}

std::string
pae_t::make_image(const std::vector<std::vector<int> > &pae_vecs) const {

   auto text_png_as_string_png_writer = [] (void *closure, const unsigned char *data, unsigned int length) {
      std::string *s_ptr = static_cast<std::string *>(closure);
      *s_ptr += std::string(reinterpret_cast<const char *>(data), length);
      return CAIRO_STATUS_SUCCESS;
   };

   int n_pixels_for_pae_image = n_pixels - 100;

   std::string s;
   unsigned char *image_data = new unsigned char[n_pixels_for_pae_image*n_pixels_for_pae_image*4];
   // pre-colour with dark purple
   for (unsigned int i=0; i<n_pixels_for_pae_image; i++) {
      for (unsigned int j=0; j<n_pixels_for_pae_image; j++) {
         image_data[4*(i*n_pixels_for_pae_image+j)]   = 70;
         image_data[4*(i*n_pixels_for_pae_image+j)+1] = 0;
         image_data[4*(i*n_pixels_for_pae_image+j)+2] = 70;
         image_data[4*(i*n_pixels_for_pae_image+j)+3] = 255;
      }
   }

   unsigned int n_residues = pae_vecs.size();

   float max_value = 32.0;
   for (unsigned int i=0; i<n_pixels_for_pae_image; i++) {
      for (unsigned int j=0; j<n_pixels_for_pae_image; j++) {
         int idx = 4*(i*n_pixels_for_pae_image+j);
         float f_x = static_cast<float>(i)/static_cast<float>(n_pixels_for_pae_image);
         float f_y = static_cast<float>(j)/static_cast<float>(n_pixels_for_pae_image);
         int i_res_index = static_cast<int>(f_x * static_cast<float>(n_residues));
         int j_res_index = static_cast<int>(f_y * static_cast<float>(n_residues));
         float f = static_cast<float>(pae_vecs[i_res_index][j_res_index])/max_value;
         unsigned char rb_col = static_cast<unsigned char>(255.0 * f);
         unsigned char  g_col = static_cast<unsigned char>(170.0 * f) + 85;
         if (rb_col < 0)   rb_col = 0;
         if (rb_col > 255) rb_col = 255;
         if (g_col < 0)    g_col = 0;
         if (g_col > 255)  g_col = 255;
         image_data[idx]   = rb_col;
         image_data[idx+1] = g_col;
         image_data[idx+2] = rb_col;
      }
   }

   cairo_t *cr;
   cairo_surface_t *pae_img_surface = cairo_image_surface_create_for_data(image_data, CAIRO_FORMAT_RGB24,
                                                                   n_pixels_for_pae_image, n_pixels_for_pae_image,
                                                                   n_pixels_for_pae_image*4);
   cairo_surface_t *base_surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, n_pixels, n_pixels);

   cr = cairo_create(base_surface);
   cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
   cairo_paint(cr);

   if (cairo_surface_status(base_surface) == CAIRO_STATUS_SUCCESS) {
      // std::cout << "### cairo_surface_status() success " << std::endl;
      cairo_set_source_surface(cr, pae_img_surface, 80, 20);
      cairo_paint(cr);

      // draw a box around the pae plot
      cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
      cairo_rectangle(cr, 80, 20, n_pixels_for_pae_image, n_pixels_for_pae_image); // x, y, width, height
      cairo_stroke(cr);

      // "scored residue" label
      cairo_set_font_size(cr, 20);
      cairo_move_to(cr, 260, 580);
      cairo_show_text(cr, "Scored Residue");

      // "aligned residue" label
      cairo_move_to(cr, 30, 320);
      cairo_save(cr);
      cairo_rotate(cr, - M_PI / 2.0); //
      cairo_show_text(cr, "Aligned Residue");
      cairo_restore(cr);

      // tick labels - x axis
      int tick_res_no = 0;
      while (tick_res_no < n_residues) {
         int x = 80 + tick_res_no;
         cairo_move_to(cr, x, 550);
         std::string text = std::to_string(tick_res_no);
         cairo_show_text(cr, text.c_str());
         tick_res_no += 100;
      }

      // tick labels - y axis
      tick_res_no = 0;
      while (tick_res_no < n_residues) {
         int y = 30 + tick_res_no;
         cairo_move_to(cr, 40, y);
         std::string text = std::to_string(tick_res_no);
         cairo_show_text(cr, text.c_str());
         tick_res_no += 100;
      }

      // output
      cairo_surface_write_to_png_stream(base_surface, text_png_as_string_png_writer,
                                        reinterpret_cast<void *>(&s));

   } else {
      std::cout << "########### cairo_surface_status() fail " << std::endl;
   }
   return s;

}