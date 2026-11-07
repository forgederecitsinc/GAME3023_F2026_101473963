import numpy as np
import wave
import struct
import math
import os

def create_livewire_bgm(output_path, sample_rate=44100, bpm=124.0, bars=32):
    print(f"Synthesizing LiveWire Substation Theme BGM...")
    print(f"BPM: {bpm}, Bars: {bars}, Sample Rate: {sample_rate} Hz")
    
    # Calculate timing
    beat_duration = 60.0 / bpm
    bar_duration = beat_duration * 4.0
    total_duration = bar_duration * bars
    total_samples = int(total_duration * sample_rate)
    t = np.linspace(0, total_duration, total_samples, endpoint=False)
    
    # Initialize stereo mix buffer
    left = np.zeros(total_samples, dtype=np.float32)
    right = np.zeros(total_samples, dtype=np.float32)
    
    # helper: convert note name to frequency
    notes = {
        'C2': 65.41, 'D2': 73.42, 'Eb2': 77.78, 'E2': 82.41, 'F2': 87.31, 'G2': 98.00, 'A2': 110.00, 'Bb2': 116.54,
        'C3': 130.81, 'D3': 146.83, 'E3': 164.81, 'F3': 174.61, 'G3': 196.00, 'A3': 220.00, 'Bb3': 233.08, 'B3': 246.94,
        'C4': 261.63, 'D4': 293.66, 'E4': 329.63, 'F4': 349.23, 'G4': 392.00, 'A4': 440.00, 'Bb4': 466.16, 'C5': 523.25,
        'D5': 587.33, 'E5': 659.25, 'F5': 698.46, 'A5': 880.00
    }
    
    # =========================================================================
    # 1. DRUMS (Kick, Snare, Hi-Hats, Industrial Percussion)
    # =========================================================================
    print("Generating Industrial Cyberpunk Drums...")
    
    # Pre-render a punchy 808/909 industrial kick drum (~0.35s)
    kick_len = int(sample_rate * 0.35)
    t_k = np.linspace(0, 0.35, kick_len, endpoint=False)
    # Pitch drop 165Hz down to 42Hz
    f_k = 42.0 + 123.0 * np.exp(-t_k * 28.0)
    phase_k = 2.0 * np.pi * np.cumsum(f_k) / sample_rate
    # Body + sub click
    env_k = np.exp(-t_k * 9.5)
    kick_sample = np.sin(phase_k) * env_k
    # Add transient punch click
    click = np.sin(2.0 * np.pi * 800.0 * t_k) * np.exp(-t_k * 90.0) * 0.35
    kick_sample = np.clip(kick_sample + click, -1.0, 1.0)
    # Slight saturation for industrial grit
    kick_sample = np.tanh(kick_sample * 1.4) * 0.85
    
    # Pre-render Snare / Clap with noise burst (~0.25s)
    snare_len = int(sample_rate * 0.25)
    t_sn = np.linspace(0, 0.25, snare_len, endpoint=False)
    sn_tone = np.sin(2.0 * np.pi * (185.0 * np.exp(-t_sn * 20.0) + 120.0) * t_sn) * np.exp(-t_sn * 16.0) * 0.45
    np.random.seed(42)
    sn_noise = (np.random.uniform(-1.0, 1.0, snare_len)) * np.exp(-t_sn * 12.0) * 0.55
    snare_sample = np.tanh((sn_tone + sn_noise) * 1.3) * 0.70
    
    # Pre-render Closed Hi-Hat (~0.06s)
    hh_len = int(sample_rate * 0.06)
    t_hh = np.linspace(0, 0.06, hh_len, endpoint=False)
    hh_noise = np.random.uniform(-1.0, 1.0, hh_len)
    # simple highpass by subtracting smoothed noise
    hh_noise = hh_noise - np.convolve(hh_noise, np.ones(5)/5, mode='same')
    hh_sample = hh_noise * np.exp(-t_hh * 55.0) * 0.22
    
    # Pre-render Open Hi-Hat (~0.22s)
    oh_len = int(sample_rate * 0.22)
    t_oh = np.linspace(0, 0.22, oh_len, endpoint=False)
    oh_noise = np.random.uniform(-1.0, 1.0, oh_len)
    oh_noise = oh_noise - np.convolve(oh_noise, np.ones(4)/4, mode='same')
    oh_sample = oh_noise * np.exp(-t_oh * 14.0) * 0.25

    # Sidechain envelope buffer for pumping the bass/pads
    sidechain = np.ones(total_samples, dtype=np.float32)
    
    # Place drums across 32 bars
    for bar in range(bars):
        bar_start = bar * bar_duration
        is_breakdown = (bar >= 24 and bar < 28) # bar 25-28 breakdown
        
        for beat in range(4):
            beat_time = bar_start + beat * beat_duration
            idx = int(beat_time * sample_rate)
            
            # Kick on beats 1, 2, 3, 4 (except during breakdown bar 25-26)
            should_kick = not (is_breakdown and bar < 26)
            if should_kick:
                end_idx = min(idx + kick_len, total_samples)
                actual_k_len = end_idx - idx
                left[idx:end_idx] += kick_sample[:actual_k_len] * 0.95
                right[idx:end_idx] += kick_sample[:actual_k_len] * 0.95
                
                # Apply sidechain dip
                sc_len = min(int(sample_rate * 0.28), total_samples - idx)
                t_sc = np.linspace(0, 1, sc_len)
                sc_curve = 0.15 + 0.85 * (t_sc ** 2)
                sidechain[idx:idx+sc_len] = np.minimum(sidechain[idx:idx+sc_len], sc_curve)
                
            # Extra syncopated kick before beat 4 in energetic sections
            if (bar in [7, 15, 23, 31]) and beat == 3:
                off_idx = idx + int(beat_duration * 0.5 * sample_rate)
                end_idx = min(off_idx + kick_len, total_samples)
                if end_idx > off_idx:
                    k_len = end_idx - off_idx
                    left[off_idx:end_idx] += kick_sample[:k_len] * 0.8
                    right[off_idx:end_idx] += kick_sample[:k_len] * 0.8
            
            # Snare on beats 2 and 4 (skip during breakdown bar 25)
            if (beat == 1 or beat == 3) and not (bar == 24 or bar == 25):
                end_idx = min(idx + snare_len, total_samples)
                actual_sn_len = end_idx - idx
                left[idx:end_idx] += snare_sample[:actual_sn_len] * 0.85
                right[idx:end_idx] += snare_sample[:actual_sn_len] * 0.85
                
            # 16th-note Hi-Hats
            if not is_breakdown or bar >= 26:
                for sixteenth in range(4):
                    hh_time = beat_time + sixteenth * (beat_duration / 4.0)
                    hh_idx = int(hh_time * sample_rate)
                    if hh_idx + hh_len < total_samples:
                        # Upbeat open hi-hat on 'and' (sixteenth == 2)
                        if sixteenth == 2 and (bar >= 8):
                            left[hh_idx:hh_idx+oh_len] += oh_sample * 0.7
                            right[hh_idx:hh_idx+oh_len] += oh_sample * 0.7
                        else:
                            # Closed hat with accented velocities
                            vel = 0.9 if sixteenth == 0 else (0.55 if sixteenth % 2 == 1 else 0.75)
                            left[hh_idx:hh_idx+hh_len] += hh_sample * vel
                            right[hh_idx:hh_idx+hh_len] += hh_sample * (vel * 0.95)

    # =========================================================================
    # 2. HIGH-VOLTAGE ROLLING BASSLINE (Industrial Saw + Sub)
    # =========================================================================
    print("Generating High-Voltage Rolling Synth Bassline...")
    bass_left = np.zeros(total_samples, dtype=np.float32)
    bass_right = np.zeros(total_samples, dtype=np.float32)
    
    # 8-bar chord progression pattern (repeated 4 times = 32 bars)
    # Dm (bars 1-2) -> F (bar 3) -> G (bar 4) -> Dm (bars 5-6) -> Bb (bar 7) -> A (bar 8)
    progression = ['D2', 'D2', 'F2', 'G2', 'D2', 'D2', 'Bb2', 'A2']
    
    sixteenth_dur = beat_duration / 4.0
    sixteenth_samples = int(sixteenth_dur * sample_rate)
    
    for bar in range(bars):
        root_note = progression[bar % 8]
        root_freq = notes[root_note]
        
        # 16 sixteenth-notes per bar
        for s in range(16):
            # Bass rhythm: driving 16th pattern
            # In breakdown (bars 24-27), play sustained notes
            note_time = bar * bar_duration + s * sixteenth_dur
            idx = int(note_time * sample_rate)
            if idx + sixteenth_samples >= total_samples:
                break
                
            # Pattern variation: octave bounce on off-beats
            freq = root_freq
            if s in [2, 6, 10, 14]:
                freq = root_freq * 2.0 # octave jump
            elif s in [15]:
                freq = root_freq * 1.5 # fifth bounce
                
            n_samples = int(sixteenth_dur * sample_rate * 0.92)
            t_note = np.linspace(0, n_samples / sample_rate, n_samples, endpoint=False)
            
            # Sawtooth oscillator with rich harmonics
            saw = 2.0 * (t_note * freq - np.floor(0.5 + t_note * freq))
            # Sub-bass sine layer
            sub = np.sin(2.0 * np.pi * (freq * 0.5) * t_note) * 0.8
            # Filter envelope (snappy low-pass sweep)
            env = np.exp(-t_note * 24.0) * 0.8 + 0.2 * np.exp(-t_note * 6.0)
            
            bass_tone = (saw * 0.65 + sub * 0.75) * env
            # Industrial drive
            bass_tone = np.tanh(bass_tone * 1.8) * 0.55
            
            end_idx = min(idx + n_samples, total_samples)
            actual_len = end_idx - idx
            bass_left[idx:end_idx] += bass_tone[:actual_len]
            bass_right[idx:end_idx] += bass_tone[:actual_len]
            
    # Apply sidechain ducking to bass
    bass_left *= sidechain
    bass_right *= sidechain
    left += bass_left
    right += bass_right

    # =========================================================================
    # 3. ELECTRIFIED ARPEGGIATOR (Laser Matrix Theme)
    # =========================================================================
    print("Generating Electrified Laser Arpeggio...")
    arp_left = np.zeros(total_samples, dtype=np.float32)
    arp_right = np.zeros(total_samples, dtype=np.float32)
    
    # Arpeggio patterns matching the chords
    arp_patterns = {
        'D2': ['D4', 'F4', 'A4', 'D5', 'A4', 'F4', 'D4', 'A4', 'D4', 'F4', 'A4', 'E5', 'D5', 'A4', 'F4', 'E4'],
        'F2': ['F4', 'A4', 'C5', 'F5', 'C5', 'A4', 'F4', 'C5', 'F4', 'A4', 'C5', 'G5', 'F5', 'C5', 'A4', 'G4'],
        'G2': ['G4', 'Bb4', 'D5', 'G5', 'D5', 'Bb4', 'G4', 'D5', 'G4', 'Bb4', 'D5', 'F5', 'D5', 'Bb4', 'A4', 'G4'],
        'Bb2': ['Bb4', 'D5', 'F5', 'Bb5', 'F5', 'D5', 'Bb4', 'F5', 'Bb4', 'D5', 'F5', 'C5', 'Bb4', 'A4', 'G4', 'F4'],
        'A2': ['A4', 'C5', 'E5', 'A5', 'E5', 'C5', 'A4', 'E5', 'A4', 'C5', 'E5', 'F5', 'E5', 'C5', 'Bb4', 'A4']
    }
    
    # Arpeggio plays throughout most of the song (except first 4 bars intro)
    for bar in range(bars):
        if bar < 4:
            continue # Intro silence for arp
            
        root_chord = progression[bar % 8]
        pattern = arp_patterns.get(root_chord, arp_patterns['D2'])
        
        # Pan sweeps across bars for wide stereo space
        pan = 0.5 + 0.35 * np.sin(bar * 0.8)
        
        for s in range(16):
            note_name = pattern[s]
            freq = notes.get(note_name, 440.0)
            
            note_time = bar * bar_duration + s * sixteenth_dur
            idx = int(note_time * sample_rate)
            if idx >= total_samples:
                break
                
            n_samples = int(sixteenth_dur * sample_rate * 0.85)
            t_note = np.linspace(0, n_samples / sample_rate, n_samples, endpoint=False)
            
            # Bright square/saw hybrid for laser-like pluck
            sqr = np.sign(np.sin(2.0 * np.pi * freq * t_note)) * 0.4
            saw = (2.0 * (t_note * freq - np.floor(0.5 + t_note * freq))) * 0.6
            env = np.exp(-t_note * 32.0) # snappy pluck
            
            arp_sample = (sqr + saw) * env * 0.28
            
            end_idx = min(idx + n_samples, total_samples)
            actual_len = end_idx - idx
            arp_left[idx:end_idx] += arp_sample[:actual_len] * pan
            arp_right[idx:end_idx] += arp_sample[:actual_len] * (1.0 - pan)
            
            # Simple stereo delay echo (1.5 sixteenth notes later)
            delay_samples = int(sixteenth_dur * 1.5 * sample_rate)
            del_idx = idx + delay_samples
            if del_idx + actual_len < total_samples:
                arp_left[del_idx:del_idx+actual_len] += arp_sample[:actual_len] * (1.0 - pan) * 0.45
                arp_right[del_idx:del_idx+actual_len] += arp_sample[:actual_len] * pan * 0.45

    left += arp_left
    right += arp_right

    # =========================================================================
    # 4. SUBSTATION INDUSTRIAL ATMOSPHERE & POWER PADS
    # =========================================================================
    print("Generating High-Voltage Substation Industrial Drone & Chords...")
    pad_left = np.zeros(total_samples, dtype=np.float32)
    pad_right = np.zeros(total_samples, dtype=np.float32)
    
    # 60Hz transformer electrical hum with harmonics
    hum_60 = np.sin(2.0 * np.pi * 60.0 * t) * 0.05
    hum_120 = np.sin(2.0 * np.pi * 120.0 * t) * 0.03
    hum_180 = np.sin(2.0 * np.pi * 180.0 * t) * 0.02
    electric_hum = (hum_60 + hum_120 + hum_180)
    
    pad_left += electric_hum
    pad_right += electric_hum
    
    # Warm dark chords (Detuned Supersaw Pad)
    chord_freqs = {
        'D2': [notes['D3'], notes['A3'], notes['D4'], notes['F4']],
        'F2': [notes['F3'], notes['C4'], notes['F4'], notes['A4']],
        'G2': [notes['G3'], notes['D4'], notes['G4'], notes['Bb4']],
        'Bb2': [notes['Bb3'], notes['F4'], notes['Bb4'], notes['D5']],
        'A2': [notes['A3'], notes['E4'], notes['A4'], notes['C5']]
    }
    
    for bar in range(bars):
        chord_root = progression[bar % 8]
        voices = chord_freqs.get(chord_root, chord_freqs['D2'])
        bar_start_idx = int(bar * bar_duration * sample_rate)
        bar_samples = int(bar_duration * sample_rate)
        end_idx = min(bar_start_idx + bar_samples, total_samples)
        actual_len = end_idx - bar_start_idx
        
        t_bar = np.linspace(0, actual_len / sample_rate, actual_len, endpoint=False)
        chord_buf = np.zeros(actual_len, dtype=np.float32)
        
        # Smooth attack and release envelope
        attack_len = int(sample_rate * 0.4)
        release_len = int(sample_rate * 0.4)
        env = np.ones(actual_len, dtype=np.float32)
        if actual_len > attack_len:
            env[:attack_len] = np.linspace(0, 1, attack_len)
        if actual_len > release_len:
            env[-release_len:] = np.linspace(1, 0, release_len)
            
        for f in voices:
            # Multi-oscillator detuning for rich chorus
            osc1 = np.sin(2.0 * np.pi * (f * 0.998) * t_bar)
            osc2 = np.sin(2.0 * np.pi * (f * 1.002) * t_bar)
            osc3 = np.sin(2.0 * np.pi * f * t_bar)
            chord_buf += (osc1 + osc2 + osc3) * 0.08
            
        chord_buf *= env * 0.45
        pad_left[bar_start_idx:end_idx] += chord_buf
        pad_right[bar_start_idx:end_idx] += chord_buf
        
    pad_left *= sidechain
    pad_right *= sidechain
    left += pad_left
    right += pad_right

    # =========================================================================
    # 5. CYBERPUNK LEAD SYNTH MELODY (Enters Bar 16-24 & 28-32)
    # =========================================================================
    print("Generating Heroic Arena Lead Melody...")
    lead_left = np.zeros(total_samples, dtype=np.float32)
    lead_right = np.zeros(total_samples, dtype=np.float32)
    
    # 8-bar melody phrases
    melody = [
        # Bar 1: D4 -> A4 -> F4
        (0.0, 1.5, 'D4'), (1.5, 0.5, 'E4'), (2.0, 1.0, 'F4'), (3.0, 1.0, 'A4'),
        # Bar 2: G4 -> F4 -> E4
        (4.0, 2.0, 'G4'), (6.0, 1.0, 'F4'), (7.0, 1.0, 'E4'),
        # Bar 3: F4 -> C5 -> A4
        (8.0, 1.5, 'F4'), (9.5, 0.5, 'G4'), (10.0, 1.0, 'A4'), (11.0, 1.0, 'C5'),
        # Bar 4: D5 -> C5 -> A4
        (12.0, 2.0, 'D5'), (14.0, 1.0, 'C5'), (15.0, 1.0, 'A4'),
        # Bar 5: D5 -> F5 -> E5 -> D5
        (16.0, 1.5, 'D5'), (17.5, 0.5, 'E5'), (18.0, 1.0, 'F5'), (19.0, 1.0, 'E5'),
        # Bar 6: D5 -> C5 -> Bb4
        (20.0, 2.0, 'D5'), (22.0, 1.0, 'C5'), (23.0, 1.0, 'Bb4'),
        # Bar 7: Bb4 -> D5 -> C5
        (24.0, 1.5, 'Bb4'), (25.5, 0.5, 'C5'), (26.0, 1.0, 'D5'), (27.0, 1.0, 'C5'),
        # Bar 8: A4 resolution
        (28.0, 3.5, 'A4')
    ]
    
    # Render melody in Bars 16-24 and 28-32
    lead_bars = [16, 28]
    for start_bar in lead_bars:
        for beat_offset, duration_beats, note_name in melody:
            note_time = (start_bar * 4.0 + beat_offset) * beat_duration
            note_dur = duration_beats * beat_duration * 0.90
            idx = int(note_time * sample_rate)
            if idx >= total_samples:
                continue
                
            freq = notes.get(note_name, 440.0)
            n_samples = int(note_dur * sample_rate)
            t_note = np.linspace(0, note_dur, n_samples, endpoint=False)
            
            # Portamento vibrato
            vib = np.sin(2.0 * np.pi * 5.5 * t_note) * 0.015 * np.clip(t_note * 2.0, 0, 1)
            freq_v = freq * (1.0 + vib)
            
            phase = 2.0 * np.pi * np.cumsum(freq_v) / sample_rate
            # Classic synth lead waveform: mix of pulse and saw
            pulse = np.sign(np.sin(phase)) * 0.4
            saw = (2.0 * ((phase / (2.0 * np.pi)) % 1.0) - 1.0) * 0.6
            
            # ADSR envelope
            attack = int(sample_rate * 0.04)
            decay = int(sample_rate * 0.1)
            release = int(sample_rate * 0.15)
            sustain_level = 0.75
            
            env = np.ones(n_samples, dtype=np.float32) * sustain_level
            if n_samples > attack:
                env[:attack] = np.linspace(0, 1.0, attack)
            if n_samples > attack + decay:
                env[attack:attack+decay] = np.linspace(1.0, sustain_level, decay)
            if n_samples > release:
                env[-release:] = np.linspace(sustain_level, 0, release)
                
            lead_tone = (pulse + saw) * env * 0.35
            # Tube saturation
            lead_tone = np.tanh(lead_tone * 1.5) * 0.65
            
            end_idx = min(idx + n_samples, total_samples)
            actual_len = end_idx - idx
            lead_left[idx:end_idx] += lead_tone[:actual_len] * 0.6
            lead_right[idx:end_idx] += lead_tone[:actual_len] * 0.6
            
            # Ping-pong delay on lead
            delay1 = int(beat_duration * 0.75 * sample_rate)
            if idx + delay1 + actual_len < total_samples:
                lead_left[idx+delay1:idx+delay1+actual_len] += lead_tone[:actual_len] * 0.35
            delay2 = int(beat_duration * 1.5 * sample_rate)
            if idx + delay2 + actual_len < total_samples:
                lead_right[idx+delay2:idx+delay2+actual_len] += lead_tone[:actual_len] * 0.25

    left += lead_left
    right += lead_right

    # =========================================================================
    # 6. MASTERING & SEAMLESS LOOP NORMALIZATION
    # =========================================================================
    print("Mastering & Normalizing Audio...")
    # Soft limiter / saturation
    left = np.tanh(left * 1.15)
    right = np.tanh(right * 1.15)
    
    # Peak normalization to -0.5 dB (~0.94)
    max_peak = max(np.max(np.abs(left)), np.max(np.abs(right)))
    if max_peak > 0:
        gain = 0.94 / max_peak
        left *= gain
        right *= gain
        
    # Ensure seamless crossfade at loop boundary (first 100 samples and last 100 samples)
    fade_len = 256
    fade_in = np.linspace(0, 1, fade_len)
    fade_out = np.linspace(1, 0, fade_len)
    # Smooth boundary to guarantee zero click on loop
    left[:fade_len] = left[:fade_len] * fade_in + left[-fade_len:] * fade_out * 0.5
    right[:fade_len] = right[:fade_len] * fade_in + right[-fade_len:] * fade_out * 0.5
    left[-fade_len:] = left[-fade_len:] * fade_out + left[:fade_len] * fade_in * 0.5
    right[-fade_len:] = right[-fade_len:] * fade_out + right[:fade_len] * fade_in * 0.5

    # Convert to 16-bit PCM
    left_16 = np.clip(left * 32767, -32768, 32767).astype(np.int16)
    right_16 = np.clip(right * 32767, -32768, 32767).astype(np.int16)
    
    # Interleave stereo channels
    stereo_pcm = np.empty((total_samples * 2,), dtype=np.int16)
    stereo_pcm[0::2] = left_16
    stereo_pcm[1::2] = right_16
    
    # Write WAV file
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with wave.open(output_path, 'wb') as wav_file:
        wav_file.setnchannels(2)
        wav_file.setsampwidth(2) # 16-bit
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(stereo_pcm.tobytes())
        
    print(f"BGM successfully written to: {output_path}")
    print(f"Total size: {os.path.getsize(output_path) / (1024*1024):.2f} MB ({total_duration:.1f} seconds)")

if __name__ == '__main__':
    # Save into Content/Sound
    output_content = 'GAME3023_F26_Roshan/Content/Sound/LiveWire_Substation_Theme.wav'
    create_livewire_bgm(output_content)
    
    # Also save into Sound folder at project root as requested
    output_root = 'GAME3023_F26_Roshan/Sound/LiveWire_Substation_Theme.wav'
    create_livewire_bgm(output_root)
