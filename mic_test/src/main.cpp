#include <Arduino.h>
#include <driver/i2s.h>
#include "SD_MMC.h"

// I2S Pin Definitions
#define I2S_SCK 6  // Serial Clock (SCK)
#define I2S_WS  7  // Word Select (WS)
#define I2S_SD  9  // Serial Data (SD)

// SDMMC Pin Definitions (do not modify)
#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0  40

// Audio Configuration
#define SAMPLE_RATE 16000
#define BUFFER_SIZE 512 // Number of samples per batch
#define CUTOFF_FREQUENCY 20.0 // High-pass filter cutoff frequency (Hz)

// High-pass filter variables
float a0, a1, b1, prevX = 0.0, prevY = 0.0;

// Function to calculate high-pass filter coefficients
void calculateHighPassCoefficients() {
    float RC = 1.0 / (2 * M_PI * CUTOFF_FREQUENCY);
    float alpha = RC / (RC + (1.0 / SAMPLE_RATE));

    a0 = alpha;
    a1 = -alpha;
    b1 = 1.0 - alpha;
}

// High-pass filter function
float applyHighPassFilter(float input) {
    // Apply the high-pass filter equation
    float output = a0 * input + a1 * prevX - b1 * prevY;

    // Update delay buffers
    prevX = input;
    prevY = output;

    return output;
}

void setupI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = false, // Default
        .fixed_mclk = 0             // Default
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,       // Bit Clock
        .ws_io_num = I2S_WS,         // Word Select
        .data_out_num = I2S_PIN_NO_CHANGE, // Data Out (not used for RX)
        .data_in_num = I2S_SD        // Data In
    };

    // Install and start I2S driver
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

void setup() {
    Serial.begin(115200);

    // Initialize high-pass filter coefficients
    calculateHighPassCoefficients();

    // Initialize I2S
    setupI2S();
    Serial.println("I2S Initialized!");

    // Configure SD_MMC pins
    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

    // Initialize SD card
    if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) { // Set 5 retries for stability
        Serial.println("SD card initialization failed!");
        while (true);
    }

    // Check card type
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card detected!");
        while (true);
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    // Print card size
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void loop() {
    int16_t samples[BUFFER_SIZE];
    size_t bytesRead;

    // Read audio data from the microphone
    i2s_read(I2S_NUM_0, &samples, sizeof(samples), &bytesRead, portMAX_DELAY);

    // Apply the high-pass filter to each sample
    for (size_t i = 0; i < bytesRead / 2; i++) { // Divide by 2 because each sample is 2 bytes (16-bit)
        float input = (float)samples[i];
        float filtered = applyHighPassFilter(input);
        samples[i] = (int16_t)filtered;
    }

    // Open a file on the SD card
    File audioFile = SD_MMC.open("/audio3.raw", FILE_APPEND);
    if (!audioFile) {
        Serial.println("Failed to open file on SD card!");
        return;
    }

    // Write filtered audio data to the file
    audioFile.write((uint8_t *)samples, bytesRead);

    // Close the file to save the data
    audioFile.close();

    Serial.print("Saved ");
    Serial.print(bytesRead);
    Serial.println(" bytes to audio3.raw");

    delay(1); // Short delay to prevent overwhelming the SD card
}

