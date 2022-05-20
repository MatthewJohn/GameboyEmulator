// Copyright (C) Dock Studios Ltd, Inc - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential
// Written by Matt Comben <matthew@dockstudios.co.uk>, May 2019

#include "vpu.h"

// Used for printing hex
// #include  <iomanip>
#include "helper.h"
#include <iostream>
#include <unistd.h>
// #include <string.h>
#include <memory>

#define DEBUG 0

VPU::VPU(RAM *ram) {
    Helper::init();
    this->ram = ram;
    SDL_Init(SDL_INIT_VIDEO);
    this->window = SDL_CreateWindow("Gameboy Emu",
        0, 0, this->SCREEN_WIDTH, this->SCREEN_HEIGHT, 0);
    this->renderer = SDL_CreateRenderer(this->window, -1, 0);
    SDL_CreateWindowAndRenderer(this->SCREEN_HEIGHT, this->SCREEN_WIDTH, 0, &this->window, &this->renderer);


    this->mode_timer_itx = 0;

    // Reset current line
    this->ram->v_set(LCDC_LY_ADDR, 0x00);
    this->current_lx = 0;

    // Reset control address value
    this->ram->v_set(LCDC_CONTROL_ADDR, 0x91);

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

void VPU::capture_screenshot(char* file_path)
{
    int w, h;
    SDL_GetRendererOutputSize(this->renderer, &w, &h);
    SDL_Surface *sshot = SDL_CreateRGBSurface(0, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    SDL_RenderReadPixels(this->renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
    SDL_SaveBMP(sshot, file_path);
    SDL_FreeSurface(sshot);
}

void VPU::redraw()
{
    // Redraw SDL
    SDL_RenderPresent(this->renderer);
}

VpuEventType VPU::process_events() {   
    // Handle SDL events
    SDL_Event event;
    while(SDL_PollEvent(&event) != 0) {

        switch(event.type) {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    return VpuEventType::EXIT;
                }
                break;

            case SDL_QUIT:
                return VpuEventType::EXIT;
            
            // Check for keyboard events
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        return VpuEventType::EXIT;
                        
                    // Handle entering terminal mode
                    case SDLK_t:
                        return VpuEventType::TERMINAL;
                }
        }
    }
    return VpuEventType::NONE;
}

// VPU ticks happen at ~ 4213440Hz - 4.213KHz
// CPU ticks will be limited to 1MHz
void VPU::update_mode_flag()
{
    uint8_t mode;
    // Check for v-blank - since ly starts at 0 and screen height starts at
    // 1, check greater than or equal to
    if (this->get_ly() >= this->SCREEN_HEIGHT)
    {
        this->current_mode = this->MODE::MODE1;
        
        // Set mode timer from start of blank (x * y since vblank start)
        this->mode_timer_itx = ((this->get_ly() - this->SCREEN_HEIGHT) * SCREEN_WIDTH) + this->get_lx();

        // Set bit 0+1 to 1
        mode = 0x01;
    }
    // Check for OAM transfer
    else if (this->get_lx() < this->MODE2_LENGTH)
    {
        this->current_mode = this->MODE::MODE2;
        this->mode_timer_itx = this->get_lx();
        // Set bit 0+1 to 2
        mode = 0x02;
    }
    // Check for LCD transfer
    else if (this->get_lx() < (this->MODE2_LENGTH + this->MODE3_LENGTH))
    {
        this->current_mode = this->MODE::MODE3;
        this->mode_timer_itx = this->get_lx() - this->MODE2_LENGTH;
        // Set bits 0+1 to 3
        mode = 0x03;
    }
    // Default to H-blank
    else
    {
        this->current_mode = this->MODE::MODE0;
        this->mode_timer_itx = this->get_lx() - (this->MODE2_LENGTH + this->MODE3_LENGTH);
        // Set bits 0+1 to 0
        mode = 0x00;
    }
    
    if (this->get_ly() == this->ram->get_val(LCDC_LYC_ADDR))
    {
        // Set bit 2
        mode |= 0x04;
    }

    uint8_t current_mode = this->ram->get_val(LCDC_STATUS_ADDR);
    
    // Blank out bits 0-2
    current_mode &= 0x07;
    
    // Combine new mode flags with remainder of original flag
    current_mode |= mode;
    
    this->ram->v_set(LCDC_STATUS_ADDR, current_mode);
}

void VPU::increment_lx_ly()
{
    if (this->current_lx == this->MAX_LX)
    {
        this->current_lx = 0;
        // If LX 'overflowed', increment
        // LY
        uint8_t new_ly = this->get_ly();
        if (new_ly == this->MAX_LY)
        {
            new_ly = 0;
            if (DEBUG)
                std::cout << "NEXT SCREEN CYCLE" << std::endl;
        }
        else
        {
            new_ly ++;
        }
        this->ram->v_set(LCDC_LY_ADDR, new_ly);
    }
    else
    {
        this->current_lx ++;
    }
}

void VPU::trigger_stat_interrupt()
{
    this->ram->set_ram_bit(INTERRUPT_IF_REGISTER_ADDRESS, 1, 1U);
}



VpuEventType VPU::tick()
{
    VpuEventType return_val = VpuEventType::NONE;

    // Increment lx
    this->increment_lx_ly();

    this->update_mode_flag();

    // Check current mode
    if (this->current_mode == this->MODE::MODE2)
    {
        // Check for interrupts
        if (this->mode_timer_itx == 0)
        {
            // Since this is the first mode for a line draw line,
            // check LYC=LY coincide interrupt
            if (this->get_ly() == this->ram->get_val(LCDC_LYC_ADDR) &&
                this->ram->get_ram_bit(LCDC_STATUS_ADDR, 6) == 1)
            {
                this->trigger_stat_interrupt();
            }

            // Check if STAT interrupt should be set on first tick
            if (this->ram->get_ram_bit(LCDC_STATUS_ADDR, 5) == 1)
            {
                this->trigger_stat_interrupt();
            }
        }

        // Do nothing
        // In future, deal with OAM 
    }
    else if (this->current_mode == this->MODE::MODE3)
    {
        // Check if LCD is enabled
        if (! this->lcd_enabled())
            return return_val;

        // Draw pixels
        if (this->mode_timer_itx < this->SCREEN_WIDTH)
        {
            this->current_draw_pixel = this->mode_timer_itx;
            this->process_pixel();
        }
    }
    else if (this->current_mode == this->MODE::MODE0)
    {
        // Check if STAT interrupt should be set on first tick
        if (this->mode_timer_itx == 0 &&
            this->ram->get_ram_bit(LCDC_STATUS_ADDR, 3) == 1)
        {
            this->trigger_stat_interrupt();
        }
        // Do nothing in h-blank
    }
    else if (this->current_mode == this->MODE::MODE1)
    {
        // If timer is at 0, check for interrupts
        if (this->mode_timer_itx == 0)
        {
            // Redraw at beginning of h-blank
            this->redraw();
 
            // Handle SDL2 events
            return_val = this->process_events();

            // Trigger v-blank interrupt
            this->ram->set_ram_bit(INTERRUPT_IF_REGISTER_ADDRESS, 0, 1U);

            // Check if STAT interrupt should be set on first tick
            if (this->ram->get_ram_bit(LCDC_STATUS_ADDR, 4) == 1)
            {
                this->trigger_stat_interrupt();
            }
        }
    }
    
    return return_val;
}

uint8_t VPU::get_background_scroll_y() {
    return this->ram->get_val(LCDC_SCY);
}
uint8_t VPU::get_background_scroll_x() {
    return this->ram->get_val(LCDC_SCX);
}

uint8_t VPU::lcd_enabled() {
    //return 1;
    return this->ram->get_ram_bit(LCDC_CONTROL_ADDR, 0x07);
}

uint8_t VPU::get_background_map() {
    return this->ram->get_ram_bit(LCDC_CONTROL_ADDR, 0x03);
}
uint8_t VPU::get_background_data_type() {
    return this->ram->get_ram_bit(LCDC_CONTROL_ADDR, 0x04);
}

vec_2d VPU::get_pixel_tile_position() {
    vec_2d pos;
    pos.x = ((unsigned int)((unsigned int)this->get_background_scroll_x() + (unsigned int)this->current_draw_pixel) % (this->TILE_WIDTH * this->BACKGROUND_TILE_GRID_WIDTH)) % this->TILE_WIDTH;
    pos.y = (((unsigned int)this->get_background_scroll_y() + (unsigned int)this->get_ly())  %(this->TILE_HEIGHT * this->BACKGROUND_TILE_GRID_HEIGHT)) % this->TILE_HEIGHT;
    return pos;
}

uint8_t VPU::get_pixel_color() {

    vec_2d pos = this->get_pixel_tile_position();
    // Get byte index, by determining where pixel is in the byte (which contains half a line of colours)
    uint16_t byte_addr = (uint16_t)(
        (unsigned long)this->get_current_tile_data_address() +
         (pos.x / 4) + (pos.y * 2)
    );
    // Unset last bit of address to get lower byte
    byte_addr &= (uint16_t)0xfffe;

    // Get bit index of bytes, by getting the reverse order
    unsigned int byte_index = (7 - pos.x);
    // Obtain upper and lower bytes
    uint8_t byte1 = this->ram->get_val(byte_addr);
    uint8_t byte2 = this->ram->get_val(byte_addr | (uint16_t)0x0001);

    // Get nth (byte_index) bit from both bytes and use one as lsb and second as msb
    uint8_t colour_byte = ((uint8_t)((byte2 >> byte_index) & (uint8_t)(0x01)) |
                           (uint8_t)((byte1 >> byte_index) & (uint8_t)(0x01) << 1));

    if (DEBUG && (unsigned int)this->get_current_tile_data_address() == 0x83a0) {
        std::cout << std::hex << "byte1: " << (unsigned int)byte1 << " byte2: " << (unsigned int)byte2 << std::endl;
        std::cout << std::hex << "Colour Byte index: " << byte_index <<
            " tile width: " << this->TILE_WIDTH <<
            " tile y pos: " << pos.y << std::endl <<
            " tile x pos: " << pos.x << std::endl <<
            " bit1: " << (unsigned int)((byte1 >> byte_index) & (uint8_t)(0x01)) <<
            " bit2: " << (unsigned int)((byte2 >> byte_index) & (uint8_t)(0x01) << 1) <<
            "  x: " << (unsigned int)this->current_draw_pixel << ", y: " << (unsigned int)this->get_ly() << std::endl <<
            " colour byte: " << (unsigned int)colour_byte <<
            " tile index: " << byte_index <<
            " Tile data location " << this->get_tile_data_address(this->ram->get_val(this->get_current_map_address())) <<
            " Memory location: " << (unsigned int)byte_addr << std::endl;
         std::cout << (unsigned int)this->get_tile_map_index_from_current_coord() << std::endl;
         std::cout << (unsigned long)this->VRAM_BG_MAPS[this->get_background_map()] << std::endl;
         std::cout << (unsigned long)this->get_tile_map_index_from_current_coord() << std::endl;
         std::cout << std::hex << (unsigned int)this->get_current_map_address() << std::endl;
         std::cout << (unsigned long)this->ram->get_val(this->get_current_map_address()) << std::endl;
         std::cout << (unsigned int)this->ram->get_val(this->get_current_tile_data_address()) << std::endl;
         //std::cin.get();
    }
    return colour_byte;
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
        case 0:
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 0);
            break;
        case 1:
            SDL_SetRenderDrawColor(renderer, 172, 172, 172, 0);
            break;
        case 3:
            SDL_SetRenderDrawColor(renderer, 86, 86, 86, 0);
            break;
        case 2:
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            break;
        default:
            // Random colour
            SDL_SetRenderDrawColor(renderer, 255, 0, 128, 0);
            //std::cout << std::hex << "Unknown color: " << (unsigned int)color << std::endl;
    };
    SDL_RenderDrawPoint(this->renderer, (int)this->current_draw_pixel, (int)this->get_ly());
    //this->process_events();
    //if (DEBUG && color != 0x0)
    //    std::cout << std::hex << "Setting Pixel color: " << (unsigned int)this->current_draw_pixel << " " << (unsigned int)this->get_ly() << " " << (int)color << std::endl;
}

// Return the on-screen X coornidate of the pixel being drawn
uint8_t VPU::get_lx() {
    return this->current_lx;
}
// Return the on-screen X coornidate of the pixel being drawn
uint8_t VPU::get_ly() {
    return this->ram->get_val(LCDC_LY_ADDR);
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
    //if (this->current_draw_pixel() == 0)
    //    std::cout << (unsigned int)this->VRAM_BG_MAPS[this->get_background_map()] << std::endl;
    unsigned int bgs_y = (unsigned int)this->get_background_scroll_y();
    unsigned int bgs_x = (unsigned int)this->get_background_scroll_x();
    unsigned int y = (unsigned int)this->get_ly();
    unsigned int x = (unsigned int)this->current_draw_pixel;
    return (unsigned int)(((((bgs_y + y) % (this->TILE_HEIGHT * this->BACKGROUND_TILE_GRID_HEIGHT)) / this->TILE_HEIGHT) * this->BACKGROUND_TILE_GRID_WIDTH) +
                          (((bgs_x + x) % (this->TILE_WIDTH * this->BACKGROUND_TILE_GRID_WIDTH)) / this->TILE_WIDTH));
}

uint16_t VPU::get_tile_data_address(uint8_t tile_number) {
    uint8_t offset;
    unsigned int mode = (unsigned int)this->get_background_data_type();
    if (mode) {
        offset = (uint8_t)((tile_number & 0x80) ? (tile_number - 128) : tile_number);
    }
    else
        offset = tile_number;

    //if ( (unsigned int)this->VRAM_TILE_DATA_TABLES[mode]) != 0x8800) {
//    if (tile_number)
//        std::cout << std::hex << (unsigned int)this->VRAM_TILE_DATA_TABLES[mode] << std::endl;
    return (uint16_t)(((uint16_t)offset * 16) + this->VRAM_TILE_DATA_TABLES[mode]);
}

uint16_t VPU::get_current_map_address() {
    return (uint16_t)((unsigned int)this->VRAM_BG_MAPS[this->get_background_map()] +
                      (unsigned int)this->get_tile_map_index_from_current_coord());
}

uint16_t VPU::get_current_tile_data_address() {
    return this->get_tile_data_address(
        this->ram->get_val(this->get_current_map_address()));
}
