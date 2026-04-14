import cppyy
from cppyy import gbl
import numpy as np
import pygame
from . import logger

class NES_CORE:
    WIDTH = 256
    HEIGHT = 240
    SCALE = 3
    XRGB_MASKS = (0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000) # convert XRGB for pygame

    def __init__(self, width=WIDTH, height=HEIGHT):
        cppyy.load_library("build/libnes_core.dylib")
        cppyy.add_include_path("include/")
        cppyy.include("core.h")

        self.core_ptr = gbl.NES_create()
        gbl.set_logger_callback(logger.python_log_callback)

        pygame.init()
        self.width, self.height = width, height
        self.main_win = pygame.display.set_mode((self.width, self.height), pygame.SCALED | pygame.RESIZABLE, vsync=1)
        self.clock = pygame.time.Clock()

        self.screen_view = np.frombuffer(self.core_ptr.screen, dtype=np.uint32, count=256*240).reshape((240, 256)).T
        self.game_buffer = pygame.Surface((self.width, self.height), 0, 32, self.XRGB_MASKS).convert()

        self.running = True
        self.testing = False
        self.isPaused = False

    
    def load_rom(self, file):
        gbl.NES_load_rom(self.core_ptr, file.encode('utf-8'))
        gbl.NES_init(self.core_ptr)
    
    def set_cpu_pc(self, pc):
        gbl.NES_set_cpu_pc(self.core_ptr, pc)

    def enable_logging(self):
        gbl.NES_enable_logging(self.core_ptr)
    
    def run(self):
        while self.running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False

            self.onKeyPress()

            if not self.isPaused:
                gbl.NES_step_frame(self.core_ptr)

            pygame.surfarray.blit_array(self.game_buffer, self.screen_view)
            scaled_game = pygame.transform.scale(self.game_buffer, (self.width, self.height))
            self.main_win.blit(scaled_game, (0, 0))

            pygame.display.flip()
            self.clock.tick(60)

        gbl.NES_destroy(self.core_ptr)

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
