/* audio_compat.c
 * Comprehensive audio compatibility system
 * Replaces DOS Sound Blaster code with modern SDL audio implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "compat.h"
#include "OLDSOUND.H"
#include "ENV.H"

#ifdef USE_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#endif

// Audio constants
#define MAX_SOUNDS 16
#define AUDIO_FREQUENCY 22050
#define AUDIO_FORMAT AUDIO_S16LSB
#define AUDIO_CHANNELS 2
#define AUDIO_SAMPLES 1024

// Extended sound structure for SDL compatibility
typedef struct {
    INT8 *samples;      // Raw sample data (from original SOUND)
    DWORD length;       // Length in samples (from original SOUND)
    #ifdef USE_SDL
    Mix_Chunk *chunk;   // SDL_mixer chunk
    #endif
    char filename[256]; // Original filename for debugging
    int loaded;         // Whether sound is loaded
} SOUND_EXT;

// Audio system state
static int audio_initialized = 0;
static int audio_available = 0;
static SOUND_EXT sounds[MAX_SOUNDS];
static int master_volume = 128;  // 0-128

// Audio device information
static struct {
    int frequency;
    int channels;
    int samples;
    int format;
} audio_device;

// Function prototypes
static int load_wav_file(const char *filename, SOUND_EXT *sound);
static void free_sound(SOUND_EXT *sound);

/*
 * Internal audio functions
 */

#ifdef USE_SDL
// SDL audio callback (if needed for raw audio)
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    // Fill with silence by default
    memset(stream, 0, len);
    (void)userdata;
}
#endif

// Initialize audio system
static int init_audio_system(void) {
    if (audio_initialized) {
        return audio_available ? 0 : -1;
    }

    #ifdef USE_SDL
    // Initialize SDL audio subsystem
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL audio init failed: %s\n", SDL_GetError());
        audio_available = 0;
        audio_initialized = 1;
        return -1;
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(AUDIO_FREQUENCY, AUDIO_FORMAT, AUDIO_CHANNELS, AUDIO_SAMPLES) < 0) {
        fprintf(stderr, "SDL_mixer init failed: %s\n", Mix_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        audio_available = 0;
        audio_initialized = 1;
        return -1;
    }

    // Get actual audio device specs
    int frequency;
    Uint16 format;
    int channels;
    Mix_QuerySpec(&frequency, &format, &channels);
    
    audio_device.frequency = frequency;
    audio_device.format = format;
    audio_device.channels = channels;
    audio_device.samples = AUDIO_SAMPLES;

    // Set mixing channels
    Mix_AllocateChannels(16);
    Mix_Volume(-1, MIX_MAX_VOLUME / 2);  // 50% volume by default

    printf("Audio initialized: %d Hz, %d channels, format 0x%x\n", 
           frequency, channels, format);

    #endif

    // Initialize sound array
    memset(sounds, 0, sizeof(sounds));
    
    audio_available = 1;
    audio_initialized = 1;
    return 0;
}

// Cleanup audio system
static void cleanup_audio_system(void) {
    if (!audio_initialized) return;

    // Free all loaded sounds
    for (int i = 0; i < MAX_SOUNDS; i++) {
        free_sound(&sounds[i]);
    }

    #ifdef USE_SDL
    if (audio_available) {
        Mix_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    #endif

    audio_initialized = 0;
    audio_available = 0;
}

// Load WAV file into sound structure
static int load_wav_file(const char *filename, SOUND_EXT *sound) {
    if (!sound || !filename) return -1;

    // Free existing sound data
    free_sound(sound);

    #ifdef USE_SDL
    if (!audio_available) {
        printf("Audio not available, skipping load of %s\n", filename);
        return -1;
    }

    // Try to load with SDL_mixer
    sound->chunk = Mix_LoadWAV(filename);
    if (!sound->chunk) {
        fprintf(stderr, "Failed to load %s: %s\n", filename, Mix_GetError());
        return -1;
    }

    // Store metadata
    strncpy(sound->filename, filename, sizeof(sound->filename) - 1);
    sound->filename[sizeof(sound->filename) - 1] = '\0';
    sound->length = sound->chunk->alen;
    sound->samples = (int8_t*)sound->chunk->abuf;
    sound->loaded = 1;

    printf("Loaded sound: %s (%lu bytes)\n", filename, (unsigned long)sound->length);
    return 0;

    #else
    // Stub implementation
    printf("STUB: Would load sound %s\n", filename);
    strncpy(sound->filename, filename, sizeof(sound->filename) - 1);
    sound->filename[sizeof(sound->filename) - 1] = '\0';
    sound->samples = (int8_t*)malloc(1024);  // Dummy data
    sound->length = 1024;
    sound->loaded = 1;
    if (sound->samples) {
        memset(sound->samples, 0, 1024);
    }
    return sound->samples ? 0 : -1;
    #endif
}

// Free sound data
static void free_sound(SOUND_EXT *sound) {
    if (!sound || !sound->loaded) return;

    #ifdef USE_SDL
    if (sound->chunk) {
        Mix_FreeChunk(sound->chunk);
        sound->chunk = NULL;
    }
    #else
    if (sound->samples) {
        free(sound->samples);
    }
    #endif

    sound->samples = NULL;
    sound->length = 0;
    sound->loaded = 0;
    sound->filename[0] = '\0';
}

/*
 * Public API functions - DOS Sound Blaster compatibility
 */

// Initialize digital audio (replaces dig_Init)
INT8 dig_Init(env_BLASTER *blaster) {
    (void)blaster;  // Ignore blaster parameter in modern implementation
    return (INT8)(init_audio_system() == 0 ? 1 : 0);
}

// Cleanup digital audio (replaces dig_Kill)
void dig_Kill(void) {
    cleanup_audio_system();
}

// Play a sound (replaces dig_Play)
void dig_Play(SOUND *sound) {
    if (!sound || !sound->samples || sound->length == 0) {
        return;
    }

    // Find corresponding extended sound for SDL playback
    #ifdef USE_SDL
    if (audio_available) {
        // Find the extended sound that matches this basic sound
        for (int i = 0; i < MAX_SOUNDS; i++) {
            if (sounds[i].loaded && sounds[i].samples == sound->samples) {
                if (sounds[i].chunk) {
                    int channel = Mix_PlayChannel(-1, sounds[i].chunk, 0);
                    if (channel == -1) {
                        fprintf(stderr, "Failed to play sound: %s\n", Mix_GetError());
                    }
                }
                return;
            }
        }
    }
    #else
    printf("STUB: Playing sound (samples=%p, length=%lu)\n", sound->samples, sound->length);
    #endif
}

// Sound Blaster hardware functions (stubs)
INT8 sb_Reset(WORD baseport) {
    (void)baseport;
    return audio_available ? 1 : 0;
}

void sb_DacSpkrOn(WORD baseport) {
    (void)baseport;
    if (audio_available) {
        master_volume = 128;
        #ifdef USE_SDL
        Mix_Volume(-1, MIX_MAX_VOLUME / 2);
        #endif
    }
}

void sb_DacSpkrOff(WORD baseport) {
    (void)baseport;
    if (audio_available) {
        master_volume = 0;
        #ifdef USE_SDL
        Mix_Volume(-1, 0);
        #endif
    }
}

void sb_Speed(WORD baseport, WORD speed, BYTE value) {
    (void)baseport;
    (void)speed;
    (void)value;
    // Sample rate changes not supported in this implementation
}

void sb_Play(WORD baseport, WORD buffsize) {
    (void)baseport;
    (void)buffsize;
    // Continuous playback not needed for this game
}

/*
 * High-level game audio functions
 */

// Sound effect IDs (matching original game)
enum {
    SND_CRACK = 0,
    SND_GROAN = 1,
    SND_MAGIC = 2,
    SND_PLATEOFF = 3,
    SND_PLATEON = 4,
    SND_SCRAPE = 5,
    SND_WATER = 6,
    SND_LAVA = 7,
    SND_SPLINTER = 8,
    SND_GLASS = 9
};

// Sound file mapping
static const char* sound_files[MAX_SOUNDS] = {
    "crack.wav",     // SND_CRACK
    "groan.wav",     // SND_GROAN
    "magic.wav",     // SND_MAGIC
    "plateoff.wav",  // SND_PLATEOFF
    "plateon.wav",   // SND_PLATEON
    "scrape.wav",    // SND_SCRAPE
    "water.wav",     // SND_WATER
    "lava.wav",      // SND_LAVA
    "splinter.wav",  // SND_SPLINTER
    "glass.wav",     // SND_GLASS
    NULL, NULL, NULL, NULL, NULL, NULL  // Unused slots
};

// Load a wave file (compatible with original load_Wave function)
int load_Wave(const char *filename, SOUND *sound) {
    // Convert SOUND to SOUND_EXT for internal use
    SOUND_EXT *ext_sound = NULL;
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (!sounds[i].loaded) {
            ext_sound = &sounds[i];
            break;
        }
    }
    if (!ext_sound) return -1;
    
    int result = load_wav_file(filename, ext_sound);
    if (result == 0) {
        // Copy back to original SOUND structure
        sound->samples = ext_sound->samples;
        sound->length = ext_sound->length;
    }
    return result;
}

// Play sound by ID (replaces PlaySound function)
void PlaySound(int sound_id) {
    if (sound_id < 0 || sound_id >= MAX_SOUNDS) {
        return;
    }

    if (!sounds[sound_id].loaded) {
        // Try to load sound on demand
        if (sound_files[sound_id]) {
            if (load_wav_file(sound_files[sound_id], &sounds[sound_id]) != 0) {
                printf("Warning: Could not load sound %d (%s)\n", 
                       sound_id, sound_files[sound_id]);
                return;
            }
        } else {
            return;  // No sound file defined
        }
    }

    // Create temporary SOUND structure for dig_Play
    SOUND temp_sound;
    temp_sound.samples = sounds[sound_id].samples;
    temp_sound.length = sounds[sound_id].length;
    dig_Play(&temp_sound);
}

// Initialize game audio system
int audio_init_game(void) {
    // Initialize audio system
    if (dig_Init(NULL) == 0) {
        printf("Audio initialization failed, continuing without sound\n");
        return -1;
    }

    // Pre-load common sounds
    for (int i = 0; i < 10; i++) {  // Load first 10 sounds
        if (sound_files[i]) {
            load_wav_file(sound_files[i], &sounds[i]);
        }
    }

    return 0;
}

// Cleanup game audio
void audio_cleanup_game(void) {
    dig_Kill();
}

/*
 * Audio control functions
 */

// Set master volume (0-128)
void set_master_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 128) volume = 128;
    
    master_volume = volume;
    
    #ifdef USE_SDL
    if (audio_available) {
        Mix_Volume(-1, (volume * MIX_MAX_VOLUME) / 128);
    }
    #endif
}

// Get master volume
int get_master_volume(void) {
    return master_volume;
}

// Check if audio is available
int is_audio_available(void) {
    return audio_available;
}

// Pause all audio
void pause_audio(void) {
    #ifdef USE_SDL
    if (audio_available) {
        Mix_Pause(-1);
    }
    #endif
}

// Resume all audio
void resume_audio(void) {
    #ifdef USE_SDL
    if (audio_available) {
        Mix_Resume(-1);
    }
    #endif
}

// Stop all playing sounds
void stop_all_sounds(void) {
    #ifdef USE_SDL
    if (audio_available) {
        Mix_HaltChannel(-1);
    }
    #endif
}

/*
 * Debug and utility functions
 */

// Print audio system status
void print_audio_status(void) {
    printf("=== Audio System Status ===\n");
    printf("Initialized: %s\n", audio_initialized ? "Yes" : "No");
    printf("Available: %s\n", audio_available ? "Yes" : "No");
    
    if (audio_available) {
        printf("Frequency: %d Hz\n", audio_device.frequency);
        printf("Channels: %d\n", audio_device.channels);
        printf("Format: 0x%x\n", audio_device.format);
        printf("Buffer size: %d samples\n", audio_device.samples);
        printf("Master volume: %d/128\n", master_volume);
        
        printf("Loaded sounds:\n");
        for (int i = 0; i < MAX_SOUNDS; i++) {
            if (sounds[i].loaded) {
                printf("  %d: %s (%lu bytes)\n", i, sounds[i].filename, (unsigned long)sounds[i].length);
            }
        }
    }
    printf("========================\n");
}

// Get sound info
int get_sound_info(int sound_id, char *filename, int *length) {
    if (sound_id < 0 || sound_id >= MAX_SOUNDS) return -1;
    if (!sounds[sound_id].loaded) return -1;
    
    if (filename) {
        strcpy(filename, sounds[sound_id].filename);
    }
    if (length) {
        *length = sounds[sound_id].length;
    }
    
    return 0;
}

// Test audio system
int test_audio_system(void) {
    printf("Testing audio system...\n");
    
    if (!audio_available) {
        printf("Audio not available for testing\n");
        return -1;
    }
    
    // Try to play a test sound
    if (sounds[SND_MAGIC].loaded) {
        printf("Playing test sound (magic)...\n");
        PlaySound(SND_MAGIC);
        return 0;
    }
    
    printf("No test sound available\n");
    return -1;
}

/*
 * Compatibility aliases and wrapper functions
 */

// Global blaster structure for compatibility  
env_BLASTER blaster = {0x220, 5, 1};  // Default SB values

// blasteravailable variable for compatibility
int blasteravailable = 0;

// Initialize blaster compatibility
void init_blaster_compat(void) {
    blasteravailable = (dig_Init(&blaster) != 0) ? 1 : 0;
}

// Cleanup blaster compatibility
void cleanup_blaster_compat(void) {
    dig_Kill();
    blasteravailable = 0;
}

/*
 * Entry points for different build configurations
 */

// Initialize audio compatibility system
int init_audio_compat(void) {
    int result = audio_init_game();
    init_blaster_compat();
    return result;
}

// Cleanup audio compatibility system
void cleanup_audio_compat(void) {
    cleanup_blaster_compat();
    audio_cleanup_game();
}