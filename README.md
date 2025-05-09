# Panel Kontrol DSP STM32F411 untuk Sistem Audio Crossover Aktif

## Deskripsi Proyek

Proyek ini mengimplementasikan sistem kontrol digital untuk audio crossover aktif berbasis mikrokontroler STM32F411. Panel kontrol DSP ini dirancang untuk memberikan tingkat kontrol yang tinggi terhadap parameter audio seperti frekuensi crossover, kompresi, limiter, delay, dan fase melalui antarmuka pengguna yang praktis. Dengan sistem ini, pengguna dapat menyesuaikan suara secara presisi untuk berbagai kebutuhan audio, mulai dari home audio hingga aplikasi profesional.

## Fitur Utama

### 🎚️ Pengaturan Crossover
- Sistem crossover 4-band: Sub, Low, Mid, dan High
- Pengaturan frekuensi crossover untuk setiap band
- Dukungan untuk berbagai jenis filter (Butterworth, Linkwitz-Riley)
- Pengaturan gain per band

### 🔊 Kompresor
- Pengaturan parameter kompresi: Threshold, Ratio, Attack, dan Release
- Kontrol dinamika untuk mencegah distorsi pada level suara tinggi
- Pengaturan makeup gain untuk mempertahankan level output

### 🚫 Limiter
- Pengaturan Threshold untuk mencegah clipping
- Kontrol Release untuk adaptasi sinyal yang natural
- Perlindungan sistem audio dari overload

### ⏱️ Delay & Fase
- Pengaturan delay per band dalam milidetik (ms)
- Koreksi fase untuk sinkronisasi antar speaker
- Penyelarasan waktu untuk mengoptimalkan pola suara

### 🎵 Preset Musik
- Preset untuk berbagai genre musik:
  - Default (Flat Response)
  - Rock
  - Jazz
  - Dangdut
  - Pop
- Penyimpanan dan pemanggilan preset pengguna

### 🧾 Antarmuka Pengguna
- LCD 16x2 untuk menampilkan informasi sistem
- Kontrol navigasi dengan rotary encoder
- Tombol-tombol untuk akses cepat ke fungsi utama

## Persyaratan Hardware

- Mikrokontroler STM32F411 (contoh: STM32F411CEU6 "Black Pill")
- 1 buah PCM1808: ADC Audio
- 2 buah PCM5102A: DAC Audio
- LCD Display 16x2 (kompatibel dengan HD44780)
- Rotary Encoder dengan push button
- Tombol-tombol tambahan untuk kontrol
- Memori EEPROM/Flash eksternal (opsional, untuk menyimpan preset)
- Komponen audio pasif (resistor, kapasitor, op-amp)
- Catu daya 3.3V atau 5V yang stabil

## Instalasi dan Penggunaan

### Persiapan Development Environment
1. Install STM32CubeIDE
2. Clone repository ini
3. Buka proyek dengan STM32CubeIDE

### Membangun Proyek
1. Pastikan semua dependensi terinstal
2. Gunakan STM32CubeIDE untuk membangun proyek:
   - Klik kanan pada proyek
   - Pilih "Build Project"

### Mengupload Firmware ke STM32F411
1. Hubungkan board STM32F411 ke komputer dengan programmer ST-Link
2. Di STM32CubeIDE:
   - Klik kanan pada proyek
   - Pilih "Run As" → "STM32 C/C++ Application"
   - Atau untuk mode debugging, pilih "Debug As" → "STM32 C/C++ Application"

### Metode Upload Alternatif
1. **Menggunakan ST-Link Utility**:
   - Buka ST-Link Utility
   - Pilih file .bin dari folder Debug
   - Atur alamat awal ke 0x08000000
   - Klik "Program & Verify"

2. **Menggunakan Mode DFU** (jika board mendukung):
   - Set pin BOOT0 ke high untuk boot dari system memory
   - Reset board
   - Gunakan DfuSe atau dfu-util untuk upload firmware

## Penggunaan Panel Kontrol

### Menu Utama
Saat dinyalakan, sistem akan menampilkan menu utama. Gunakan rotary encoder untuk navigasi dan tekan untuk memilih opsi.

### Pengaturan Crossover
1. Pilih "Crossover" dari menu utama
2. Pilih band yang ingin diatur (Sub, Low, Mid, High)
3. Atur frekuensi crossover dengan rotary encoder
4. Tekan untuk konfirmasi atau tekan tombol kembali untuk batal

### Pengaturan Kompressor
1. Pilih "Compressor" dari menu utama
2. Pilih parameter yang ingin diatur
3. Atur nilai dengan rotary encoder
4. Tekan untuk konfirmasi

### Pengaturan Limiter
1. Pilih "Limiter" dari menu utama
2. Pilih parameter yang ingin diatur
3. Atur nilai dengan rotary encoder
4. Tekan untuk konfirmasi

### Pengaturan Delay & Fase
1. Pilih "Delay/Phase" dari menu utama
2. Pilih band yang ingin diatur
3. Pilih delay atau fase
4. Atur nilai dengan rotary encoder
5. Tekan untuk konfirmasi

### Mengelola Preset
1. Pilih "Presets" dari menu utama
2. Pilih salah satu opsi:
   - "Load Preset": Memuat preset yang tersimpan
   - "Save Preset": Menyimpan pengaturan saat ini sebagai preset baru
   - "Delete Preset": Menghapus preset yang tersimpan

## Struktur Proyek

```
AudioCrossover/
├── .project                   # STM32CubeIDE project file
├── .cproject                  # STM32CubeIDE C project settings
├── .settings/                 # IDE settings
├── STM32F411CEUX_FLASH.ld    # Linker script
├── AudioCrossover.ioc         # STM32CubeMX configuration file
│
├── Core/
│   ├── Inc/
│   │   ├── main.h            # Main header file
│   │   ├── gpio.h            # GPIO configuration
│   │   ├── dma.h             # DMA configuration for audio streams
│   │   ├── i2c.h             # I2C for LCD and peripherals
│   │   ├── i2s.h             # I2S for audio codec interface
│   │   ├── spi.h             # SPI for external EEPROM/Flash
│   │   ├── tim.h             # Timers for system timing
│   │   ├── usart.h           # UART for debug output
│   │   └── stm32f4xx_it.h    # Interrupt handlers
│   │
│   └── Src/
│       ├── main.c            # Main program
│       ├── gpio.c            # GPIO initialization
│       ├── dma.c             # DMA configuration
│       ├── i2c.c             # I2C peripheral setup
│       ├── i2s.c             # I2S configuration for audio
│       ├── spi.c             # SPI configuration
│       ├── tim.c             # Timer setup
│       ├── usart.c           # UART configuration
│       ├── stm32f4xx_it.c    # Interrupt service routines
│       ├── stm32f4xx_hal_msp.c  # HAL MSP initialization
│       └── system_stm32f4xx.c   # System initialization
│
├── Drivers/
│   ├── CMSIS/
│   │   ├── Device/
│   │   │   └── ST/
│   │   │       └── STM32F4xx/
│   │   │           └── [CMSIS Device Files]
│   │   └── Include/
│   │       └── [CMSIS Core Files]
│   │
│   └── STM32F4xx_HAL_Driver/
│       ├── Inc/
│       │   └── [HAL Driver Headers]
│       └── Src/
│           └── [HAL Driver Sources]
│
├── Middlewares/
│   ├── Third_Party/
│   │   └── ARM_CMSIS/
│   │       └── DSP/
│   │           └── [CMSIS-DSP Library Files]
│   └── ST/
│       └── [Any ST middleware]
│
├── Audio/
│   ├── Inc/
│   │   ├── audio_driver.h        # Audio I/O driver interface
│   │   ├── audio_processing.h    # DSP processing chain
│   │   ├── crossover.h           # Multi-band crossover implementation
│   │   ├── compressor.h          # Dynamic range compression
│   │   ├── limiter.h             # Limiter implementation
│   │   ├── delay.h               # Delay processing
│   │   └── audio_preset.h        # Preset data structures
│   │
│   └── Src/
│       ├── audio_driver.c
│       ├── audio_processing.c
│       ├── crossover.c
│       ├── compressor.c
│       ├── limiter.c
│       ├── delay.c
│       └── audio_preset.c
│
├── Interface/
│   ├── Inc/
│   │   ├── lcd_driver.h          # LCD 16x2 driver
│   │   ├── rotary_encoder.h      # Rotary encoder interface
│   │   ├── button_handler.h      # Button interface
│   │   ├── menu_system.h         # Menu navigation and display
│   │   └── ui_manager.h          # User interface coordinator
│   │
│   └── Src/
│       ├── lcd_driver.c
│       ├── rotary_encoder.c
│       ├── button_handler.c
│       ├── menu_system.c
│       └── ui_manager.c
│
├── Storage/
│   ├── Inc/
│   │   ├── flash_storage.h       # Internal flash storage
│   │   ├── eeprom_driver.h       # External EEPROM interface (if used)
│   │   └── preset_manager.h      # Preset save/load management
│   │
│   └── Src/
│       ├── flash_storage.c
│       ├── eeprom_driver.c
│       └── preset_manager.c
│
├── Presets/
│   ├── Inc/
│   │   └── factory_presets.h     # Built-in preset declarations
│   │
│   └── Src/
│       └── factory_presets.c     # Built-in preset implementations
│
└── Debug/
    └── [Build output files]
```
## Pengembangan Lebih Lanjut

Beberapa area yang dapat dikembangkan lebih lanjut:

1. **Penambahan Konektivitas**: Implementasi Bluetooth atau WiFi untuk kontrol nirkabel
2. **Ekspansi Preset**: Penambahan preset untuk genre musik lain
3. **Pengembangan UI**: Dukungan untuk LCD grafis atau OLED
4. **Multizone**: Dukungan untuk beberapa zona audio independen
5. **Equalisasi Parametrik**: Penambahan modul EQ parametrik per band

## Troubleshooting

### Masalah Umum

1. **Tidak Ada Suara**:
   - Periksa koneksi ADC dan DAC
   - Periksa apakah semua band tidak dalam status mute
   - Pastikan gain tidak terlalu rendah

2. **Suara Terdistorsi**:
   - Periksa pengaturan threshold limiter
   - Turunkan gain pada band yang bermasalah
   - Periksa apakah sinyal input terlalu besar

3. **LCD Tidak Menampilkan Informasi**:
   - Periksa koneksi I2C/paralel ke LCD
   - Periksa tegangan kontras LCD
   - Reset mikrocontroller

4. **Rotary Encoder Tidak Responsif**:
   - Periksa koneksi ke rotary encoder
   - Periksa apakah debouncing diimplementasikan dengan benar
   - Restart sistem

### Batas Kemampuan

STM32F411 memiliki keterbatasan sumber daya:
- Memori RAM terbatas: 128KB
- Flash terbatas: 512KB
- Kecepatan prosesor bisa menjadi pembatas untuk DSP kompleks

## Kontribusi

Kami sangat menghargai kontribusi untuk pengembangan proyek ini. Silakan mengikuti langkah-langkah berikut:

1. Fork repositori ini
2. Buat branch fitur baru (`git checkout -b fitur-baru`)
3. Commit perubahan Anda (`git commit -m 'Menambahkan fitur baru'`)
4. Push ke branch (`git push origin fitur-baru`)
5. Buat Pull Request

## Lisensi

Proyek ini dilisensikan di bawah lisensi MIT. Lihat file LICENSE untuk informasi lebih lanjut.

## Kontak

Untuk pertanyaan atau dukungan, silakan hubungi:
- Email: [email@example.com]
- Website: [www.example.com]

---

Dikembangkan dengan ❤️ untuk komunitas audio Indonesia