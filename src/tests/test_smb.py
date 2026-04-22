from .. import nes as nes_core

def main():
    nes = nes_core.NES_CORE()
    nes.load_rom("roms/smb.nes")
    nes.init()
    nes.run()

if __name__ == "__main__":
    main()
