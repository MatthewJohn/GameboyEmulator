#include "vpu.h"

// Used for printing hex
// #include  <iomanip>

#include <iostream>
// #include <string.h>
#include <memory>

#define DEBUG 1

VPU::VPU(RAM *ram, sf::RenderWindow *window) {
    this->ram = ram;
    this->window = window;
    this->next_screen();
    const sf::Color black = sf::Color((uint32_t)0xffff);
    this->sf_image = sf::Image();
    this->sf_image.create((unsigned int)this->SCREEN_WIDTH, (unsigned int)this->SCREEN_HEIGHT, black);
    this->sf_texture = sf::Texture();
    this->sf_texture.loadFromImage(this->sf_image);
    this->sf_sprite = sf::Sprite(this->sf_texture, sf::IntRect(0, 0, this->SCREEN_WIDTH, this->SCREEN_HEIGHT));
    this->sf_sprite.setTexture(this->sf_texture);
    this->sf_sprite.setPosition(0, 0);
    this->window->draw(this->sf_sprite);
    this->window->display();
}

void VPU::next_screen() {
    std::cout << "NEXT SCREEN" << std::endl;
    this->h_timer_itx = 0;
    this->refresh_timer_itx = 0;
    // Reset current line
    this->ram->set(this->LCDC_LY_ADDR, 0x00);
    this->current_pixel_x = 0;
}

void VPU::next_line() {
    this->h_timer_itx = 0;
    // Increment current line
    if (this->get_current_y() == this->SCREEN_HEIGHT)
        this->next_screen();
    else
        this->ram->inc(this->LCDC_LY_ADDR);
}

void VPU::tick() {
    // Update timers and skip to next screen,
    // if required.
    // Update screen timer
    this->refresh_timer_itx ++;
    if (this->refresh_timer_itx >= this->SCREEN_REFRESH_INTERVAL)
        this->next_screen();
    // Update line timer
    this->h_timer_itx ++;

    if (this->h_timer_itx >= this->H_LENGTH)
        this->next_line();

    if (this->get_current_x() < this->SCREEN_WIDTH)
        this->process_pixel();
    //else
    //    this->next_line();     // Temporarily go to next line after completing row

}

uint8_t VPU::get_background_scroll_y() {
    return this->ram->get_val(this->LCDC_SCY);
}
uint8_t VPU::get_background_scroll_x() {
    return this->ram->get_val(this->LCDC_SCX);
}

unsigned char VPU::get_register_bit(uint16_t address, uint8_t bit_shift) {
    uint8_t val = this->ram->get_val(address);
    return (val & (1U  << bit_shift)) >> bit_shift;
}

uint8_t VPU::get_background_map_type() {
    return this->get_register_bit(this->LCDC_CONTROL_ADDR, 0x04);
}
uint8_t VPU::get_background_data_type() {
    return this->get_register_bit(this->LCDC_CONTROL_ADDR, 0x04);
}

vec_2d VPU::get_pixel_tile_position() {
    vec_2d pos;
    pos.x = ((int)this->get_background_scroll_x() + (int)this->get_current_x()) % this->TILE_WIDTH;
    pos.y = ((int)this->get_background_scroll_y() + (int)this->get_current_y()) % this->TILE_HEIGHT;
    return pos;
}

uint8_t VPU::get_pixel_color() {
    
    vec_2d pos = this->get_pixel_tile_position();
    // Get byte index, by determining where pixel is in the byte (which contains half a line of colours)
    int byte_index = ((int)pos.x % (this->TILE_WIDTH / 2)) * 2;
    uint8_t colour_byte = this->ram->get_val(
        this->get_tile_address() +
        (uint16_t)(pos.y * this->TILE_HEIGHT) +
        (uint16_t)(pos.x >= (this->TILE_WIDTH / 2) ? 1 : 0)
    );
    if (DEBUG)
        std::cout << std::hex << "Colour Byte index: " << byte_index <<
            " tile width: " << (int)this->TILE_WIDTH <<
            " tile x pos: " << (int)pos.x <<
            " colour byte: " << (int)colour_byte <<
            " colour nibble: " << (int)((colour_byte << byte_index) & (0x04)) << std::endl;
    return (colour_byte << byte_index) & (0x04);
}

void VPU::process_pixel() {
    //
    if ((int)this->get_pixel_color() != 0)
        this->sf_image.setPixel((int)this->get_current_x(), (int)this->get_current_y(), sf::Color(0xff, 0xff, 0xff, 0x00));
    this->current_pixel_x ++;
    //this->sf_texture.loadFromImage(this->sf_image);
    //this->sf_sprite.setTexture(this->sf_texture);
    //this->window->draw(this->sf_sprite);
    //this->window->display();
    
}

// Return the on-screen X coornidate of the pixel being drawn 
uint8_t VPU::get_current_x() {
    return this->current_pixel_x;
}
// Return the on-screen X coornidate of the pixel being drawn 
uint8_t VPU::get_current_y() {
    return this->ram->get_val(this->LCDC_LY_ADDR);
}


uint16_t VPU::get_tile_address() {
    return (this->VRAM_TILE_DATA_TABLES[this->get_background_data_type()] + this->ram->get_val(
        this->VRAM_BG_MAPS[this->get_background_map_type()] +
        (uint16_t)(
            ((int)(this->get_background_scroll_y() + this->get_current_y()) * this->BACKGROUND_TILE_GRID_WIDTH) +
            (int)(this->get_background_scroll_x() + this->get_current_x())
        )
    ));
}

//void VPU::get_pixel_color(uint8_t x, uint8_t y) {
//    
//}