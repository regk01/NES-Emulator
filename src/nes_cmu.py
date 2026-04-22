import cppyy
from cppyy import gbl
from PIL import Image as PILImage
from cmu_graphics import *

class NES_CORE:
    def __init__(self, rom_path):
        # Initialize C++ Core
        cppyy.load_library("build/libnes_core.dylib")
        cppyy.add_include_path("include/")
        cppyy.include("core.h")

        self.core_ptr = gbl.NES_create()
        gbl.NES_load_rom(self.core_ptr, rom_path.encode('utf-8'))
        gbl.NES_init(self.core_ptr)

        # Buffer Info
        self.width, self.height = 256, 240
        self.isPaused = False
        self.input_state = 0

    def get_frame_as_pil(self):
        # Step the emulator
        if not self.isPaused:
            gbl.NES_step_frame(self.core_ptr)

        # Create PIL image from the raw pointer
        # 'RGBX' handles the 32-bit XRGB format by treating the 'X' as padding
        return PILImage.frombuffer(
            'RGBX',
            (self.width, self.height), 
            self.core_ptr.screen, 
            'raw', 
            'BGRX', 0, 1
        )

# --- CMU Graphics App Logic ---

def onAppStart(app):
    # Update this path to your actual ROM
    app.nes = NES_CORE("roms/smb.nes")
    
    # Scaling factor for the window
    app.scale = 3
    app.width = app.nes.width * app.scale
    app.height = app.nes.height * app.scale

    app.stepsPerSecond = 60
    app.display_image = None

def onStep(app):
    gbl.NES_controller_push_state(app.nes.core_ptr, app.nes.input_state)

    # 1. Capture the frame from the C++ core via PIL
    pil_img = app.nes.get_frame_as_pil()

    # 2. Convert PIL Image to CMUImage for rendering
    app.display_image = CMUImage(pil_img)

def redrawAll(app):
    if app.display_image:
        # Draw the frame scaled to the window size
        drawImage(app.display_image, 0, 0, width=app.width, height=app.height)
    else:
        drawLabel("Loading ROM...", app.width/2, app.height/2, size=20)

def handle_input(app, key, is_pressed):
    # Map CMU keys to NES bitmask
    mapping = {
        'w': 0x08, 's': 0x04, 'a': 0x02, 'd': 0x01,
        'tab': 0x20, 'enter': 0x10, 'space': 0x80, 'leftShift': 0x40,
        'up': 0x08, 'down': 0x04, 'left': 0x02, 'right': 0x01 # Add arrow keys too
    }

    clean_key = key.lower()
    if clean_key in mapping:
        bit = mapping[clean_key]
        if is_pressed:
            app.nes.input_state |= bit
        else:
            app.nes.input_state &= ~bit

def onKeyPress(app, key):
    if key == 'p':
        app.paused = not app.paused
        app.nes.isPaused = app.paused
    handle_input(app, key, True)

def onKeyRelease(app, key):
    handle_input(app, key, False)

runApp()