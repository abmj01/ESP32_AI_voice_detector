import numpy as np
import matplotlib.pyplot as plt
import soundfile as sf

# Load audio file
audio, fs = sf.read('audio3_16_bit_high_pass_filtered.wav')  # Replace with your audio file

# Perform FFT
N = len(audio)
freq = np.fft.rfftfreq(N, d=1/fs)  # Frequency vector
fft_magnitude = np.abs(np.fft.rfft(audio))  # FFT magnitude

# Plot frequency spectrum
plt.plot(freq, fft_magnitude)
plt.xlabel('Frequency (Hz)')
plt.ylabel('Magnitude')
plt.title('Frequency Spectrum')
plt.xlim(0, 1000)  # Focus on 0-1000 Hz range
plt.grid()
plt.show()
