#pragma once
#include "raylib.h"
#include <cmath>
#include <cstdio>

class AudioSystem {
private:
    Music menuMusic;
    bool initialized;
    
public:
    AudioSystem() : initialized(false) {}
    
    void Init() {
        InitAudioDevice();
        
        // Create a simple menu theme using procedural generation
        // Generate a simple melody using sine waves
        Wave wave = {0};
        wave.frameCount = 44100 * 4; // 4 seconds
        wave.sampleRate = 44100;
        wave.sampleSize = 16;
        wave.channels = 1;
        
        wave.data = (short*)malloc(wave.frameCount * wave.sampleSize / 8);
        
        // Generate simple melody notes (C-E-G-C)
        float frequencies[] = {261.63f, 329.63f, 392.00f, 261.63f}; // C4, E4, G4, C4
        float noteDuration = 0.5f; // 0.5 seconds per note
        int samplesPerNote = (int)(44100 * noteDuration);
        
        for (int note = 0; note < 4; note++) {
            int startSample = note * samplesPerNote;
            int endSample = (note + 1) * samplesPerNote;
            if (endSample > wave.frameCount) endSample = wave.frameCount;
            
            for (int i = startSample; i < endSample; i++) {
                float t = (float)(i - startSample) / 44100.0f;
                float envelope = 1.0f - (t / noteDuration) * 0.5f; // Simple fade
                float sample = sinf(2.0f * PI * frequencies[note] * t) * envelope * 0.3f;
                ((short*)wave.data)[i] = (short)(sample * 32767.0f);
            }
        }
        
        // Convert wave to music
        menuMusic = LoadMusicFromWave(wave);
        UnloadWave(wave);
        
        // Set music to loop
        menuMusic.looping = true;
        
        initialized = true;
        printf("Audio system initialized with menu theme\n");
    }
    
    void PlayMenuMusic() {
        if (initialized && !IsMusicPlaying(menuMusic)) {
            PlayMusicStream(menuMusic);
            printf("Playing menu music\n");
        }
    }
    
    void StopMenuMusic() {
        if (initialized && IsMusicPlaying(menuMusic)) {
            StopMusicStream(menuMusic);
            printf("Stopped menu music\n");
        }
    }
    
    void Update() {
        if (initialized) {
            UpdateMusicStream(menuMusic);
        }
    }
    
    void Cleanup() {
        if (initialized) {
            StopMenuMusic();
            UnloadMusicStream(menuMusic);
            CloseAudioDevice();
            initialized = false;
            printf("Audio system cleaned up\n");
        }
    }
};
