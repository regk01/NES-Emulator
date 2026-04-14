from .. import nes as nes_core

def main():
    nes = nes_core.NES_CORE()
    nes.load_rom("roms/nestest.nes")
    nes.set_cpu_pc(0xC000)
    nes.run()

if __name__ == "__main__":
    main()
