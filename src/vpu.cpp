#include "vpu.h"

// Used for printing hex
// #include  <iomanip>
#include "helper.h"
#include <iostream>
#include <unistd.h>
// #include <string.h>
#include <memory>

#define DEBUG 1

VPU::VPU(RAM *ram) {
    Helper::init();
    this->ram = ram;
    SDL_Init(SDL_INIT_VIDEO);
    this->window = SDL_CreateWindow("Gameboy Emu",
        0, 0, this->SCREEN_WIDTH, this->SCREEN_HEIGHT, 0);
    this->renderer = SDL_CreateRenderer(this->window, -1, 0);
    SDL_CreateWindowAndRenderer(this->SCREEN_HEIGHT, this->SCREEN_WIDTH, 0, &this->window, &this->renderer);


    this->h_timer_itx = 0;
    this->refresh_timer_itx = 0;
    // Reset current line
    this->ram->v_set(this->ram->LCDC_LY_ADDR, 0x00);
    this->current_pixel_x = 0;

	//Create Window

    // Clear with white
    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 0);
    SDL_RenderClear(this->renderer);
    SDL_RenderPresent(this->renderer);
    SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 0);
    // Set all pixels to black
    for (unsigned int x = 0; x < this->SCREEN_WIDTH; x++)
        for (unsigned int y = 0; y < this->SCREEN_HEIGHT; y++)
                SDL_RenderDrawPoint(this->renderer, (int)y, (int)x);
                

    SDL_SetRenderDrawColor(this->renderer, 0, 255, 0, 0);
    // Set all pixels to black
    for (unsigned int x = 60; x < 80; x++)
        for (unsigned int y = 50; y < 90; y++)
                SDL_RenderDrawPoint(this->renderer, (int)x, (int)y);
    SDL_RenderPresent(this->renderer);
}

void VPU::process_events() {
    SDL_RenderPresent(this->renderer);
}

// VPU ticks happen at ~ 4213440Hz - 4.213KHz
// CPU ticks will be limited to 1MHz
void VPU::update_mode_flag() {
    uint8_t mode = this->get_mode_flag_value();
    this->ram->set_ram_bit(this->ram->LCDC_STATUS_ADDR, 0x0, (mode & 0x1));
    this->ram->set_ram_bit(this->ram->LCDC_STATUS_ADDR, 0x1, (mode & 0x2));
}
uint8_t VPU::get_mode_flag_value() {
    if (! this->lcd_enabled())
        return 0x1;
    if (this->get_current_y() > this->SCREEN_HEIGHT)
        return 0x1;
    if (this->get_current_x() > this->SCREEN_WIDTH)
        return 0x0;
    return 0x3;
}

void VPU::reset_ly() {
    this->process_events();
    if (DEBUG)
        std::cout << "NEXT SCREEN" << std::endl;

    // Reset current line
    this->ram->v_set(this->ram->LCDC_LY_ADDR, 0x00);
}

void VPU::reset_lx() {
    this->process_events();
    this->current_pixel_x = 0;
    uint8_t current_y = this->ram->v_inc(this->ram->LCDC_LY_ADDR);
    if (current_y == this->ram->get_val(this->ram->LCDC_LYC_ADDR))
        this->ram->set_ram_bit(this->ram->LCDC_STATUS_ADDR, 2, 1U);
    else
        this->ram->set_ram_bit(this->ram->LCDC_STATUS_ADDR, 2, 0U);
}

void VPU::tick() {
    
    // Check X position
    this->current_pixel_x ++;
    uint8_t current_y = this->get_current_y();
    if (this->get_current_x() >= this->MAX_LX)
        this->reset_lx();    

    // Check if LY counter is the max value
    // and reset
    if (current_y >= this->MAX_LY)
    {
        this->reset_ly();
        current_y = 0;
    }
    
    // Update timers and skip to next screen,
    // if required.
    this->update_mode_flag();

    // Check if LCD is enabled
    if (! this->lcd_enabled())
        return;

    // If not at end of screen, in either x or y, process pixel
    if (current_y < this->SCREEN_HEIGHT && this->get_current_x() < this->SCREEN_WIDTH)
        this->process_pixel();
}

uint8_t VPU::get_background_scroll_y() {
    return this->ram->get_val(this->ram->LCDC_SCY);
}
uint8_t VPU::get_background_scroll_x() {
    return this->ram->get_val(this->ram->LCDC_SCX);
}

uint8_t VPU::lcd_enabled() {
    //return 1;
    return this->ram->get_ram_bit(this->ram->LCDC_CONTROL_ADDR, 0x07);
}

uint8_t VPU::get_background_map() {
    return this->ram->get_ram_bit(this->ram->LCDC_CONTROL_ADDR, 0x03);
}
uint8_t VPU::get_background_data_type() {
    return this->ram->get_ram_bit(this->ram->LCDC_CONTROL_ADDR, 0x04);
}

vec_2d VPU::get_pixel_tile_position() {
    vec_2d pos;
    pos.x = ((unsigned int)this->get_background_scroll_x() + (unsigned int)this->get_current_x()) % this->TILE_WIDTH;
    pos.y = ((unsigned int)this->get_background_scroll_y() + (unsigned int)this->get_current_y()) % this->TILE_HEIGHT;
    return pos;
}

uint8_t VPU::get_pixel_color() {
    
    vec_2d pos = this->get_pixel_tile_position();
    // Get byte index, by determining where pixel is in the byte (which contains half a line of colours)
    uint16_t byte_addr = (uint16_t)(
        (unsigned long)this->get_current_tile_data_address() +
         (((unsigned int)this->get_background_scroll_y() + (unsigned int)this->get_current_y()) % this->TILE_HEIGHT * 2) + // Y position x 2, since there's two bytes per row (4 pixels per byte)
         (((unsigned int)this->get_background_scroll_x() + (unsigned int)this->get_current_x()) % this->TILE_WIDTH / 4)
    );
    int byte_index = (pos.x % (this->TILE_WIDTH / 2)) * 2;
    //std::cout << std::hex << pos.x << std::endl;
//    if (this->get_current_tile_data_address() != 0x8800)
//        std::cout << std::hex << (unsigned int)this->get_current_tile_data_address() << std::endl;
    uint8_t colour_byte = this->ram->get_val(byte_addr);
    if (DEBUG && (unsigned int)this->get_current_tile_data_address() != 0x8000) {
        std::cout << std::hex << "Colour Byte index: " << byte_index <<
            " tile width: " << this->TILE_WIDTH <<
            " tile x pos: " << pos.x <<
            " colour byte: " << (unsigned int)colour_byte <<
            " colour nibble: " << (unsigned int)((colour_byte >> byte_index) & (0x04)) <<
            " Tile data location " << this->get_tile_data_address(this->ram->get_val(this->get_current_map_address())) <<
            " Memory location: " << 
        (uint16_t)
        ((unsigned int)this->get_current_tile_data_address() +
         (pos.y * 2) + // Y position x 2, since there's two bytes per row (4 pixels per byte)
         (pos.x / 4)) << std::endl;
         std::cout << (unsigned int)this->get_tile_map_index_from_current_coord() << std::endl;
         std::cout << (unsigned long)this->VRAM_BG_MAPS[this->get_background_map()] << std::endl;
         std::cout << (unsigned long)this->get_tile_map_index_from_current_coord() << std::endl;
         std::cout << std::hex << (unsigned int)this->get_current_map_address() << std::endl;
         std::cout << (unsigned long)this->ram->get_val(this->get_current_map_address()) << std::endl;
         std::cout << (unsigned int)this->ram->get_val(this->get_current_tile_data_address()) << std::endl;
         //std::cin.get();
    }
    return (colour_byte >> byte_index) & uint16_t(0x04);
}

void VPU::tear_down() {
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);
    SDL_Quit();
}

void VPU::process_pixel() {
    //
    uint8_t color = this->get_pixel_color();
    switch((unsigned int)color) {
        case 0x00:
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            break;
        case 0x01:
            SDL_SetRenderDrawColor(renderer, 86, 86, 86, 0);
            break;
        case 0x02:
            SDL_SetRenderDrawColor(renderer, 172, 172, 172, 0);
            break;
        case 0x03:
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
            break;
        default:
            // Random colour
            SDL_SetRenderDrawColor(renderer, 255, 0, 128, 0);
            std::cout << std::hex << "Unknown color: " << (unsigned int)color << std::endl;
    };
    SDL_RenderDrawPoint(this->renderer, (int)this->get_current_x(), (int)this->get_current_y());
    //this->process_events();
    if (DEBUG && color != 0x0)
        std::cout << std::hex << "Setting Pixel color: " << (unsigned int)this->get_current_x() << " " << (unsigned int)this->get_current_y() << " " << (int)color << std::endl;
}

// Return the on-screen X coornidate of the pixel being drawn 
uint8_t VPU::get_current_x() {
    return this->current_pixel_x;
}
// Return the on-screen X coornidate of the pixel being drawn 
uint8_t VPU::get_current_y() {
    return this->ram->get_val(this->ram->LCDC_LY_ADDR);
}

uint8_t convert_int8_uint8(uint8_t in_val) {
    union {
        uint8_t uint8[2];
        int8_t int8[2];
    } data;
    data.int8[0] = (int8_t)in_val;
    return data.uint8[0];
}

unsigned int VPU::get_tile_map_index_from_current_coord() {
    //if (this->get_current_x() == 0)
    //    std::cout << (unsigned int)this->VRAM_BG_MAPS[this->get_background_map()] << std::endl;
    unsigned int bgs_y = (unsigned int)this->get_background_scroll_y();
    unsigned int bgs_x = (unsigned int)this->get_background_scroll_x();
    unsigned int y = (unsigned int)this->get_current_y();
    unsigned int x = (unsigned int)this->get_current_x();
    return (unsigned int)((((bgs_y + y) / this->TILE_HEIGHT) * this->BACKGROUND_TILE_GRID_WIDTH) +
                          ((bgs_x + x) / this->TILE_WIDTH));
}

uint16_t VPU::get_tile_data_address(uint8_t tile_number) {
    uint8_t offset;
    unsigned int mode = (unsigned int)this->get_background_data_type();
    if (tile_number)
        std::cout << (int)tile_number << std::endl;
    if (mode) {
        offset = (uint8_t)((tile_number & 0x80) ? (tile_number - 128) : tile_number);
        if (offset)
            std::cout << "Signed: " << (int)offset << std::endl;
    }
    else
        offset = tile_number;
    
    //if ( (unsigned int)this->VRAM_TILE_DATA_TABLES[mode]) != 0x8800) {
//    if (tile_number)
//        std::cout << std::hex << (unsigned int)this->VRAM_TILE_DATA_TABLES[mode] << std::endl;
    return (uint16_t)(((uint16_t)offset << 4) + this->VRAM_TILE_DATA_TABLES[mode]);
}

uint16_t VPU::get_current_map_address() {
    return (uint16_t)((unsigned int)this->VRAM_BG_MAPS[this->get_background_map()] +
                      (unsigned int)this->get_tile_map_index_from_current_coord());
}

uint16_t VPU::get_current_tile_data_address() {
    return this->get_tile_data_address(
        this->ram->get_val(this->get_current_map_address()));
}
