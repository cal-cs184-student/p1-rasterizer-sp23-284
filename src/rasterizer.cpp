#include "rasterizer.h"

using namespace std;

namespace CGL {

  RasterizerImp::RasterizerImp(PixelSampleMethod psm, LevelSampleMethod lsm,
    size_t width, size_t height,
    unsigned int sample_rate) {
    this->psm = psm;
    this->lsm = lsm;
    this->width = width;
    this->height = height;
    this->sample_rate = sample_rate;

    sample_buffer.resize(width * height * sample_rate, Color::White);
  }

  // Used by rasterize_point and rasterize_line
  void RasterizerImp::fill_pixel(size_t x, size_t y, Color c) {
    // TODO: Task 2: You might need to this function to fix points and lines (such as the black rectangle border in test4.svg)
    // NOTE: You are not required to implement proper supersampling for points and lines
    // It is sufficient to use the same color for all supersamples of a pixel for points and lines (not triangles)
    int sqrt_sample_rate = sqrt(this->sample_rate);
    if (sqrt_sample_rate == 1) {
      sample_buffer[y * width + x] = c;
    }
    for (int i = 0; i < sqrt_sample_rate; i++) {
      for (int j = 0; j < sqrt_sample_rate; j++) {
        int coord = width * sqrt_sample_rate * (y * sqrt_sample_rate + j) + x * sqrt_sample_rate + i;
        sample_buffer[coord] = c;
      }
    }
  }

  // Rasterize a point: simple example to help you start familiarizing
  // yourself with the starter code.
  //
  void RasterizerImp::rasterize_point(float x, float y, Color color) {
    // fill in the nearest pixel
    int sx = (int)floor(x);
    int sy = (int)floor(y);

    // check bounds
    if (sx < 0 || sx >= width) return;
    if (sy < 0 || sy >= height) return;

    fill_pixel(sx, sy, color);
    return;
  }

  // Rasterize a line.
  void RasterizerImp::rasterize_line(float x0, float y0,
    float x1, float y1,
    Color color) {
    if (x0 > x1) {
      swap(x0, x1); swap(y0, y1);
    }

    float pt[] = { x0,y0 };
    float m = (y1 - y0) / (x1 - x0);
    float dpt[] = { 1,m };
    int steep = abs(m) > 1;
    if (steep) {
      dpt[0] = x1 == x0 ? 0 : 1 / abs(m);
      dpt[1] = x1 == x0 ? (y1 - y0) / abs(y1 - y0) : m / abs(m);
    }

    while (floor(pt[0]) <= floor(x1) && abs(pt[1] - y0) <= abs(y1 - y0)) {
      rasterize_point(pt[0], pt[1], color);
      pt[0] += dpt[0]; pt[1] += dpt[1];
    }
  }

  float RasterizerImp::line_equation(float x, float y,
    float x1, float y1,
    float x2, float y2) {
    return -1 * (x - x1) * (y2 - y1) + (y - y1) * (x2 - x1);
  }

  // Rasterize a triangle.
  void RasterizerImp::rasterize_triangle(float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    Color color) {
    // TODO: Task 1: Implement basic triangle rasterization here, no supersampling
    /*
    int xmin = min({x0, x1, x2}) - 1;
    int xmax = max({ x0, x1, x2 }) + 1;
    int ymin = min({ y0, y1, y2 }) - 1;
    int ymax = max({ y0, y1, y2 }) + 1;
    for (int y = ymin; y < ymax; y++) {
      for (int x = xmin; x < xmax; x++) {
        float l0 = line_equation(x + 0.5, y + 0.5, x0, y0, x1, y1);
        float l1 = line_equation(x + 0.5, y + 0.5, x1, y1, x2, y2);
        float l2 = line_equation(x + 0.5, y + 0.5, x2, y2, x0, y0);
        if ((l0 >= 0 && l1 >= 0 && l2 >= 0) || (l0 <= 0 && l1 <= 0 && l2 <= 0)) {
          rasterize_point(x + 0.5, y + 0.5, color);
        }
      }
    }
    */

    // TODO: Task 2: Update to implement super-sampled rasterization
    int sqrt_sample_rate = sqrt(this->sample_rate);
    x0 = x0 * sqrt_sample_rate;
    x1 = x1 * sqrt_sample_rate;
    x2 = x2 * sqrt_sample_rate;
    y0 = y0 * sqrt_sample_rate;
    y1 = y1 * sqrt_sample_rate;
    y2 = y2 * sqrt_sample_rate;
    int xmin = min({ x0, x1, x2 }) - 1;
    int xmax = max({ x0, x1, x2 }) + 1;
    int ymin = min({ y0, y1, y2 }) - 1;
    int ymax = max({ y0, y1, y2 }) + 1;
    for (int y = ymin; y < ymax; y++) {
      for (int x = xmin; x < xmax; x++) {
        int sx = (int)floor(x + 0.5);
        int sy = (int)floor(y + 0.5);
        // check bounds
        if (sx < 0 || sx >= width * sqrt_sample_rate) continue;
        if (sy < 0 || sy >= height * sqrt_sample_rate) continue;

        float l0 = line_equation(x + 0.5, y + 0.5, x0, y0, x1, y1);
        float l1 = line_equation(x + 0.5, y + 0.5, x1, y1, x2, y2);
        float l2 = line_equation(x + 0.5, y + 0.5, x2, y2, x0, y0);
        if ((l0 >= 0 && l1 >= 0 && l2 >= 0) || (l0 <= 0 && l1 <= 0 && l2 <= 0)) {
          //fill_pixel(sx, sy, color);
          sample_buffer[sy * width * sqrt_sample_rate + sx] = color;
        }
      }
    }
  }


  void RasterizerImp::rasterize_interpolated_color_triangle(float x0, float y0, Color c0,
    float x1, float y1, Color c1,
    float x2, float y2, Color c2)
  {
    // TODO: Task 4: Rasterize the triangle, calculating barycentric coordinates and using them to interpolate vertex colors across the triangle
    // Hint: You can reuse code from rasterize_triangle
    int sqrt_sample_rate = sqrt(this->sample_rate);
    x0 = x0 * sqrt_sample_rate;
    x1 = x1 * sqrt_sample_rate;
    x2 = x2 * sqrt_sample_rate;
    y0 = y0 * sqrt_sample_rate;
    y1 = y1 * sqrt_sample_rate;
    y2 = y2 * sqrt_sample_rate;
    int xmin = min({ x0, x1, x2 }) - 1;
    int xmax = max({ x0, x1, x2 }) + 1;
    int ymin = min({ y0, y1, y2 }) - 1;
    int ymax = max({ y0, y1, y2 }) + 1;
    for (int y = ymin; y < ymax; y++) {
      for (int x = xmin; x < xmax; x++) {
        int sx = (int)floor(x + 0.5);
        int sy = (int)floor(y + 0.5);
        // check bounds
        if (sx < 0 || sx >= width * sqrt_sample_rate) continue;
        if (sy < 0 || sy >= height * sqrt_sample_rate) continue;

        float l0 = line_equation(x + 0.5, y + 0.5, x0, y0, x1, y1);
        float l1 = line_equation(x + 0.5, y + 0.5, x1, y1, x2, y2);
        float l2 = line_equation(x + 0.5, y + 0.5, x2, y2, x0, y0);
        if ((l0 >= 0 && l1 >= 0 && l2 >= 0) || (l0 <= 0 && l1 <= 0 && l2 <= 0)) {
          //fill_pixel(sx, sy, color);
          float a = (-1 * (x + 0.5 - x1) * (y2 - y1) + (y + 0.5 - y1) * (x2 - x1)) / (-1 * (x0 - x1) * (y2 - y1) + (y0 - y1) * (x2 - x1));
          float b = (-1 * (x + 0.5 - x2) * (y0 - y2) + (y + 0.5 - y2) * (x0 - x2)) / (-1 * (x1 - x2) * (y0 - y2) + (y1 - y2) * (x0 - x2));
          float r = 1 - a - b;
          sample_buffer[sy * width * sqrt_sample_rate + sx] = a * c0 + b * c1 + r * c2;
        }
      }
    }


  }


  void RasterizerImp::rasterize_textured_triangle(float x0, float y0, float u0, float v0,
    float x1, float y1, float u1, float v1,
    float x2, float y2, float u2, float v2,
    Texture& tex)
  {
    // TODO: Task 5: Fill in the SampleParams struct and pass it to the tex.sample function.
    // TODO: Task 6: Set the correct barycentric differentials in the SampleParams struct.
    // Hint: You can reuse code from rasterize_triangle/rasterize_interpolated_color_triangle
    int sqrt_sample_rate = sqrt(this->sample_rate);
    x0 = x0 * sqrt_sample_rate;
    x1 = x1 * sqrt_sample_rate;
    x2 = x2 * sqrt_sample_rate;
    y0 = y0 * sqrt_sample_rate;
    y1 = y1 * sqrt_sample_rate;
    y2 = y2 * sqrt_sample_rate;
    int xmin = min({ x0, x1, x2 }) - 1;
    int xmax = max({ x0, x1, x2 }) + 1;
    int ymin = min({ y0, y1, y2 }) - 1;
    int ymax = max({ y0, y1, y2 }) + 1;
    for (int y = ymin; y < ymax; y++) {
      for (int x = xmin; x < xmax; x++) {
        int sx = (int)floor(x + 0.5);
        int sy = (int)floor(y + 0.5);
        // check bounds
        if (sx < 0 || sx >= width * sqrt_sample_rate) continue;
        if (sy < 0 || sy >= height * sqrt_sample_rate) continue;

        float l0 = line_equation(x + 0.5, y + 0.5, x0, y0, x1, y1);
        float l1 = line_equation(x + 0.5, y + 0.5, x1, y1, x2, y2);
        float l2 = line_equation(x + 0.5, y + 0.5, x2, y2, x0, y0);
        if ((l0 >= 0 && l1 >= 0 && l2 >= 0) || (l0 <= 0 && l1 <= 0 && l2 <= 0)) {
          float a = (-1 * (x + 0.5 - x1) * (y2 - y1) + (y + 0.5 - y1) * (x2 - x1)) / (-1 * (x0 - x1) * (y2 - y1) + (y0 - y1) * (x2 - x1));
          float b = (-1 * (x + 0.5 - x2) * (y0 - y2) + (y + 0.5 - y2) * (x0 - x2)) / (-1 * (x1 - x2) * (y0 - y2) + (y1 - y2) * (x0 - x2));
          float r = 1 - a - b;

          Vector2D uv((a * u0 + b * u1 + r * u2), (a * v0 + b * v1 + r * v2));
          SampleParams sp;
          sp.psm = this->psm;
          sp.lsm = this->lsm;
          sp.p_uv = uv;
          sample_buffer[sy * width * sqrt_sample_rate + sx] = tex.sample(sp);
        }
      }
    }



  }

  void RasterizerImp::set_sample_rate(unsigned int rate) {
    // TODO: Task 2: You may want to update this function for supersampling support

    this->sample_rate = rate;

    this->sample_buffer.resize(width * height * this->sample_rate, Color::White);
    clear_buffers();
  }


  void RasterizerImp::set_framebuffer_target(unsigned char* rgb_framebuffer,
    size_t width, size_t height)
  {
    // TODO: Task 2: You may want to update this function for supersampling support

    this->width = width;
    this->height = height;
    this->rgb_framebuffer_target = rgb_framebuffer;


    this->sample_buffer.resize(width * height * this->sample_rate, Color::White);
    clear_buffers();
  }


  void RasterizerImp::clear_buffers() {
    std::fill(rgb_framebuffer_target, rgb_framebuffer_target + 3 * width * height, 255);
    std::fill(sample_buffer.begin(), sample_buffer.end(), Color::White);
  }


  // This function is called at the end of rasterizing all elements of the
  // SVG file.  If you use a supersample buffer to rasterize SVG elements
  // for antialising, you could use this call to fill the target framebuffer
  // pixels from the supersample buffer data.
  //
  void RasterizerImp::resolve_to_framebuffer() {
    // TODO: Task 2: You will likely want to update this function for supersampling support

    // Task 1
    /*for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        Color col = sample_buffer[y * width + x];

        for (int k = 0; k < 3; ++k) {
          this->rgb_framebuffer_target[3 * (y * width + x) + k] = (&col.r)[k] * 255;
        }
      }
    }*/

    // Task 2
    int sqrt_sample_rate = sqrt(this->sample_rate);
    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        //Color col = sample_buffer[y * width + x];
        vector<float> c(3, 0.0);
        if (sqrt_sample_rate == 1) {
          c[0] = sample_buffer[y * width + x].r;
          c[1] = sample_buffer[y * width + x].g;
          c[2] = sample_buffer[y * width + x].b;
        }
        else {
          for (int i = 0; i < sqrt_sample_rate; i++) {
            for (int j = 0; j < sqrt_sample_rate; j++) {
              int coord = width * sqrt_sample_rate * (y * sqrt_sample_rate + j) + x * sqrt_sample_rate + i;
              //for (int k = 0; k < 3; ++k) {
              //  (&col.r)[k] += (&sample_buffer[coord].r)[k];
              //}
              c[0] += sample_buffer[coord].r;
              c[1] += sample_buffer[coord].g;
              c[2] += sample_buffer[coord].b;
            }
          }
        }
        for (int k = 0; k < 3; ++k) {
          //this->rgb_framebuffer_target[3 * (y * width + x) + k] = (&col.r)[k] / this->sample_rate * 255 ;
          this->rgb_framebuffer_target[3 * (y * width + x) + k] = c[k] / this->sample_rate * 255;
        }
      }
    }

  }

  Rasterizer::~Rasterizer() { }


}// CGL
