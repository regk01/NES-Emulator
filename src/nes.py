from pathlib import Path
import time
import cppyy
from cppyy import gbl
import numpy as np
import pygame
import pygame_widgets
from pygame_widgets.button import Button
from tkinter import Tk
from tkinter import filedialog
from . import logger

class NES_CORE:
    WIDTH = 256 # NES Window width
    HEIGHT = 240 # NES Window height
    STATUS_BAR_HEIGHT = 30 # accomodates for file button
    SIDE_BAR_WIDTH = 350
    PATTERN_TABLE_Y = 120
    PALETTE_Y = 20

    SCALE = 3
    XRGB_MASKS = (0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000) # convert XRGB for pygame
    BASE_ROM_DIR = Path(__file__).parent.parent / "roms"

    def __init__(self, width=WIDTH, height=HEIGHT):
        cppyy.load_library("build/libnes_core.dylib")
        cppyy.add_include_path("include/")
        cppyy.include("core.h")

        self.core_ptr = gbl.NES_create()
        gbl.set_logger_callback(logger.python_log_callback)
        self.tk_root = Tk()
        self.tk_root.withdraw()
        self.pattern_tables = np.frombuffer(
            self.core_ptr.ppu.pattern_table_ui, 
            dtype=np.uint32,
            count=2 * 128 * 128
        ).reshape((2, 128, 128))
        self.palettes = np.frombuffer(
            self.core_ptr.ppu.palette_ui, 
            dtype=np.uint32,
            count=8*4
        ).reshape((8, 4))

        pygame.init()
        pygame.key.stop_text_input()
        self.width, self.height = width*self.SCALE, height*self.SCALE
        self.main_win = pygame.display.set_mode((self.width+self.SIDE_BAR_WIDTH, self.height+self.STATUS_BAR_HEIGHT), pygame.SCALED, vsync=1)
        self.font = pygame.font.SysFont("monospace", 15)
        self.clock = pygame.time.Clock()

        self.screen_view = np.frombuffer(self.core_ptr.screen, dtype=np.uint32, count=256*240).reshape((240, 256)).T
        self.game_buffer = pygame.Surface((self.WIDTH, self.HEIGHT), 0, 32, self.XRGB_MASKS).convert()

        self.running = True
        self.testing = False
        self.isPaused = False

        self.file_open_button = Button(
            # Mandatory Parameters
            self.main_win,  # Surface to place button on
            0,  # X-coordinate of top left corner
            0,  # Y-coordinate of top left corner
            45,  # Width
            self.STATUS_BAR_HEIGHT,  # Height
            
            text = "Files",
            fontSize = 20,
            inactiveColour = (100, 100, 100),
            onClick = self.open_file_dialog
        )

    def open_file_dialog(self):
        was_paused = self.isPaused
        self.isPaused = True
        file_path = filedialog.askopenfilename(initialdir=self.BASE_ROM_DIR, title="Select ROM", filetypes=(("NES files", "*.nes"), ("All files", "*.*")))
        if file_path:
            gbl.NES_stop_audio(self.core_ptr)
            time.sleep(0.05)
            self.free_rom()
            self.load_rom(file_path)
            self.reset()
            gbl.NES_start_audio(self.core_ptr)

        pygame.event.clear()
        self.isPaused = was_paused

    def load_rom(self, file):
        gbl.NES_load_rom(self.core_ptr, file.encode('utf-8'))

    def free_rom(self):
        gbl.NES_free_rom(self.core_ptr)

    def init(self):
        gbl.NES_init(self.core_ptr)

    def reset(self):
        gbl.NES_reset(self.core_ptr)

    def set_cpu_pc(self, pc):
        gbl.NES_set_cpu_pc(self.core_ptr, pc)

    def enable_logging(self):
        gbl.NES_enable_logging(self.core_ptr)

    def set_cpu_ram(self, addr, value):
        gbl.NES_set_cpu_ram(self.core_ptr, addr, value)

    def run(self):
        while self.running:
            events = pygame.event.get()
            for event in events:
                if event.type == pygame.QUIT:
                    self.running = False
                elif event.type == pygame.KEYDOWN: # Command + Q to quit
                    if event.key == pygame.K_q and (event.mod & pygame.KMOD_META):
                        self.running = False
                elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                    self.handle_click(event.pos)

            self.onKeyPress()

            if not self.isPaused:
                gbl.NES_step_frame(self.core_ptr)


            pygame.surfarray.blit_array(self.game_buffer, self.screen_view)
            scaled_game = pygame.transform.scale(self.game_buffer, (self.width, self.height))
            self.main_win.blit(scaled_game, (0, self.STATUS_BAR_HEIGHT))
            self.draw_debug_ui()

            pygame_widgets.update(events)
            pygame.display.flip()
            self.clock.tick(60)

        pygame.quit()
        self.tk_root.destroy()
        gbl.NES_destroy(self.core_ptr)

    def handle_click(self, mouse_pos):
        for p_idx in range(8):
            row = p_idx // 4
            col = p_idx % 4
            base_x = self.width + 20 + (col * 70)
            base_y = self.PALETTE_Y + (row * 30)

            for c_idx in range(4):
                rect = pygame.Rect(base_x + (c_idx * 15), base_y, 14, 14)
                if rect.collidepoint(mouse_pos):
                    gbl.NES_set_current_ui_palette(self.core_ptr, p_idx)

    def draw_debug_ui(self):
        start_x = self.width
        sidebar_rect = pygame.Rect(start_x, 0, 350, self.height + self.STATUS_BAR_HEIGHT)
        pygame.draw.rect(self.main_win, (50, 50, 50), sidebar_rect)

        for p_idx in range(8):
            row = p_idx // 4
            col = p_idx % 4
            base_x = self.width + 40 + (col * 70)
            base_y = self.PALETTE_Y + (row * 30)

            for c_idx in range(4):
                color = self.palettes[p_idx][c_idx]
                rect = pygame.Rect(base_x + (c_idx * 15), base_y, 14, 14)
                pygame.draw.rect(self.main_win, int(color), rect)

        for i in range(2):
            SCALE = 2
            table_surf = pygame.Surface((128, 128), 0, 32, self.XRGB_MASKS).convert()
            pygame.surfarray.blit_array(table_surf, self.pattern_tables[i].T)
            x_pos = self.width + 50
            y_pos = self.PATTERN_TABLE_Y + (i * (128 * SCALE + 50))

            scaled_size = (128 * SCALE, 128 * SCALE)
            scaled_surf = pygame.transform.scale(table_surf, scaled_size)
            self.main_win.blit(scaled_surf, (x_pos, y_pos))

            for x in range(16):
                label = self.font.render(hex(x)[2:].upper(), True, (200, 200, 200))
                self.main_win.blit(label, (x_pos + (x * (128 * SCALE) / 16) + 2, y_pos - 15))

            for y in range(16):
                label = self.font.render(hex(y)[2:].upper() + "0", True, (200, 200, 200))
                self.main_win.blit(label, (x_pos - 22, y_pos + (y * (128 * SCALE) / 16) - 2))

    def onKeyPress(self):
        keys = pygame.key.get_pressed()
        if self.testing:
            if keys[pygame.K_0]:
                for _ in range(50):
                    gbl.NES_step(self.core_ptr)
            if keys[pygame.K_1]:
                gbl.NES_step_frame(self.core_ptr)

        if keys[pygame.K_p]:
            self.isPaused = not self.isPaused
        
        if keys[pygame.K_RIGHTBRACKET]: # just for marios
            self.set_cpu_ram(0x075F, 1)
            self.set_cpu_ram(0x0760, 1)
            self.set_cpu_ram(0x0770, 0)
            self.set_cpu_ram(0x0772, 0)
            self.set_cpu_ram(0x000E, 0)

        input = 0 # input for nes
        if keys[pygame.K_w]: # UP
            input |= 0x08
        if keys[pygame.K_s]: # DOWN
            input |= 0x04
        if keys[pygame.K_a]: # LEFT
            input |= 0x02
        if keys[pygame.K_d]: # RIGHT
            input |= 0x01
        if keys[pygame.K_TAB]: # SELECT
            input |= 0x20
        if keys[pygame.K_RETURN]: # START
            input |= 0x10
        if keys[pygame.K_SPACE]: # A
            input |= 0x80
        if keys[pygame.K_LSHIFT]: # B
            input |= 0x40

        gbl.NES_controller_push_state(self.core_ptr, input)
