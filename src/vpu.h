#pragma once

#include <memory>
#include "ram.h"
//#include <SFML/Graphics.hpp>
#include <stdlib.h>
#include <SDL.h>


struct vec_2d {
    unsigned int x;
    unsigned int y;
};

class VPU {
public:
    VPU(RAM *ram);
    void tick();
    void tear_down();
    void process_events();
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
private:
    // Clock cycle definitions
    // Not sure which is most critical - timing of overall screen refresh or lenth of vblank
    const unsigned int VBLANK_LENGTH = 0x11D0;
    const unsigned int SCREEN_REFRESH_INTERVAL = 0x011250;
    unsigned int refresh_timer_itx;
    
    const unsigned int SCREEN_WIDTH = 160; // 0xA0
    const unsigned int SCREEN_HEIGHT = 144; // 90
    const unsigned int MAX_LY = 0x99;
    const unsigned int MAX_LX = 0xff;
    
    // Line timings
    // Mode 0 is just the remaining time after mode 2 and 3 and before H_LENGTH
    const unsigned int MODE2_LENGTH = 0x4D; // 77
    const unsigned int MODE3_LENGTH = 0xA9; // 169
    const unsigned int H_LENGTH = 0x01c8; // 456
    unsigned int h_timer_itx;
    
    unsigned int current_pixel_x;
    
    // Each is 3FF bytes (1024 bytes), providing 32x32 tiles, meanig 1 byte per tile
    const uint16_t VRAM_BG_MAPS[2] = {(uint16_t)0x9800, (uint16_t)0x9c00};
    const uint16_t VRAM_TILE_DATA_TABLES[2] = {(uint16_t)0x8800, (uint16_t)0x8000};
    const unsigned int BACKGROUND_TILE_GRID_WIDTH = 32;
    const unsigned int BACKGROUND_TILE_GRID_HEIGHT = 32;
    const unsigned int TILE_WIDTH = 8;
    const unsigned int TILE_HEIGHT = 8;
    const unsigned int TILE_DATA_SIZE = 16;
    
    void reset_ly();
    void reset_lx();
    
    RAM *ram;
    
    uint8_t get_background_scroll_x();
    uint8_t get_background_scroll_y();
    uint8_t get_current_x();
    uint8_t get_current_y();
    
    // Control register bits
    uint8_t get_mode_flag_value();
    void update_mode_flag();
    uint8_t get_background_map();
    uint8_t get_background_data_type();
    uint8_t lcd_enabled();

    unsigned int get_tile_map_index_from_current_coord();
    uint16_t get_tile_data_address(uint8_t tile_number);
    uint8_t get_pixel_color();
    uint16_t get_tile_address();
    uint16_t get_current_tile_data_address();
    uint16_t get_current_map_address();
    vec_2d get_pixel_tile_position();
    
    void process_pixel();
    void wait_for_window();

};