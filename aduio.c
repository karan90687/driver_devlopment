
// full gmf pipeline for playback and record 
// #include "av_render.h"
// #include "av_render_default.h"
#include "codec_board.h"
#include "codec_init.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_enc.h"
#include "esp_audio_enc_default.h"
#include "esp_audio_enc_reg.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "peer.h"
#include "peer_connection.c"
#include "peer_signaling.h"
#include "sdkconfig.h"
#include "videosdk.c"
#include "videosdk.h"
#include <math.h>
#include <string.h>

// opus to pcm
#include "esp_audio_dec.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_dec_reg.h"

// GMF Includes
#include "esp_gmf_aec.h"
#include "esp_gmf_audio_helper.h"
#include "esp_gmf_bit_cvt.h"
#include "esp_gmf_ch_cvt.h"
#include "esp_gmf_io_codec_dev.h"
#include "esp_gmf_rate_cvt.h"
#include "freertos/ringbuf.h"
#include "gmf_loader_setup_defaults.h"
// headers for the opus to pcm
#include "esp_audio_dec.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_dec_reg.h"

#define CUSTOM_HEADER_SIZE 16
#if CONFIG_ESP32S3_XIAO
typedef struct {
  audio_render_handle_t audio_render;
  av_render_handle_t player;
} player_system_t;
#endif

#if CONFIG_ESP32S3_XIAO
static player_system_t player_sys = {0};
#endif
int opus_sample_rate = 16000;
int pcma_pcmu_sample_rate = 8000;
int channel = 1;
int bits_per_sample = 16;
#if defined(CONFIG_ESP32S3_XIAO)
#define I2S_CLK_GPIO 42
#define I2S_DATA_GPIO 41
#endif

#if defined(CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD)
#define I2S_CLK_GPIO 4 // BCLK
#define I2S_WS_GPIO 6  // WS / LRCK
#define I2S_DOUT_GPIO 5
#endif

#define TAG "AUDIO"

extern PeerConnection *g_pc_publish;
extern PeerConnectionState eState;
static esp_codec_dev_handle_t record_handle = NULL;
static esp_audio_enc_handle_t enc_handle = NULL;
static esp_audio_enc_in_frame_t aenc_in_frame = {0};
static esp_audio_enc_out_frame_t aenc_out_frame = {0};
esp_opus_enc_config_t opus_enc_cfg = ESP_OPUS_ENC_CONFIG_DEFAULT();

esp_g711_enc_config_t g711_cfg;
esp_audio_enc_config_t enc_cfg;
i2s_chan_handle_t rx_handle = NULL;
esp_codec_dev_sample_info_t fs;

// struct for storing no header opus frames
typedef struct {
  const uint8_t *data;
  size_t len;
} out_frame_opus_t;

// global or file-scope variable
out_frame_opus_t opus_out_frame = {0};

#if CONFIG_ESP32S3_XIAO
av_render_audio_info_t audio_info;
av_render_audio_frame_info_t aud_info;
#endif

static uint8_t *read_buf = NULL;
static uint8_t *write_buf = NULL;

// GMF & AEC Variables
#if CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD
static esp_gmf_pool_handle_t pool = NULL;
static esp_gmf_pipeline_handle_t aec_pipe = NULL;
static RingbufHandle_t aec_out_rb = NULL; // Ring buffer for clean audio
static esp_gmf_task_handle_t gmf_task = NULL;
static size_t encoder_frame_bytes = 0; // Encoder frame size in bytes
static uint8_t *aec_accum_buf = NULL;  // Accumulator for AEC output
static size_t aec_accum_pos = 0;       // Current pos in accumulator

// GMF Playback Pipeline Variables
static esp_gmf_pipeline_handle_t playback_pipe = NULL;
static esp_gmf_task_handle_t playback_task = NULL;
static RingbufHandle_t playback_rb =
    NULL; // Ring buffer for Opus frames (sent to GMF decoder)
#endif

// Helper to write GMF output to RingBuffer
static esp_gmf_err_io_t
aec_output_callback(void *handle, esp_gmf_payload_t *load, int block_ticks) {
  if (load == NULL || load->valid_size == 0) {
    return ESP_GMF_IO_FAIL;
  }

  // Write clean PCM (16k, 16bit, Mono) to Ring Buffer
  if (xRingbufferSend(aec_out_rb, load->buf, load->valid_size, block_ticks) !=
      pdTRUE) {
    ESP_LOGW(TAG, "RingBuffer full, dropping AEC frame");
    // Optional: Return OK to keep pipeline running, or FAIL to signal
    // backpressure
    return ESP_GMF_IO_OK;
  }
  return ESP_GMF_IO_OK;
}

static esp_gmf_err_io_t no_op_acquire(void *handle, esp_gmf_payload_t *load,
                                      int wanted_size, int block_ticks) {
  return ESP_GMF_IO_OK;
}

#if CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD
// Playback Pipeline Ring Buffer Helpers
static esp_gmf_err_io_t playback_rb_acquire(void *handle,
                                            esp_gmf_payload_t *load,
                                            int wanted_size, int block_ticks) {
  RingbufHandle_t rb = (RingbufHandle_t)handle;
  size_t item_size = 0;

  // Receive encoded Opus audio from ring buffer
  void *data = xRingbufferReceive(rb, &item_size, block_ticks);
  if (data == NULL) {
    return ESP_GMF_IO_TIMEOUT;
  }

  load->buf = data;
  load->valid_size = item_size;

  return ESP_GMF_IO_OK;
}

static esp_gmf_err_io_t
playback_rb_release(void *handle, esp_gmf_payload_t *load, int block_ticks) {
  RingbufHandle_t rb = (RingbufHandle_t)handle;

  if (load->buf) {
    vRingbufferReturnItem(rb, load->buf);
    load->buf = NULL;
  }

  return ESP_GMF_IO_OK;
}
#endif

esp_err_t audio_codec_init(audio_codec_t cfg) {
#if CONFIG_ESP32S3_XIAO
  i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

  if (cfg == AUDIO_CODEC_OPUS) {
    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(opus_sample_rate),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                   I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                .clk = I2S_CLK_GPIO,
                .din = I2S_DATA_GPIO,
                .invert_flags =
                    {
                        .clk_inv = false,
                    },
            },
    };
    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
  } else {
    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(pcma_pcmu_sample_rate),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                   I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                .clk = I2S_CLK_GPIO,
                .din = I2S_DATA_GPIO,
                .invert_flags =
                    {
                        .clk_inv = false,
                    },
            },
    };
    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
  }
#endif

  esp_audio_enc_register_default();

  int read_size = 0, out_size = 0;
  if (cfg == AUDIO_CODEC_OPUS) {
    opus_enc_cfg.sample_rate = opus_sample_rate;
    opus_enc_cfg.channel = channel;
    opus_enc_cfg.bits_per_sample = bits_per_sample;
    opus_enc_cfg.frame_duration = ESP_OPUS_ENC_FRAME_DURATION_20_MS;
    opus_enc_cfg.application_mode = ESP_OPUS_ENC_APPLICATION_AUDIO;

    enc_cfg.type = ESP_AUDIO_TYPE_OPUS;
    enc_cfg.cfg = &opus_enc_cfg;
    enc_cfg.cfg_sz = sizeof(opus_enc_cfg);
  } else if (cfg == AUDIO_CODEC_PCMU) {
    g711_cfg.sample_rate = pcma_pcmu_sample_rate;
    g711_cfg.channel = channel;
    g711_cfg.bits_per_sample = bits_per_sample;
    g711_cfg.frame_duration = 20;
    enc_cfg.type = ESP_AUDIO_TYPE_G711U;
    enc_cfg.cfg = &g711_cfg;
    enc_cfg.cfg_sz = sizeof(g711_cfg);
  } else {
    g711_cfg.sample_rate = pcma_pcmu_sample_rate;
    g711_cfg.channel = channel;
    g711_cfg.bits_per_sample = bits_per_sample;
    g711_cfg.frame_duration = 20;
    enc_cfg.type = ESP_AUDIO_TYPE_G711A;
    enc_cfg.cfg = &g711_cfg;
    enc_cfg.cfg_sz = sizeof(g711_cfg);
  }

#if CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD
  record_handle = get_record_handle();
  if (!record_handle) {
    ESP_LOGE(TAG, "Failed to get record handle");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Record handle initialized: %p", record_handle);

  // Open codec with 4-channel config for AEC (RMNM format)
  // ES7210 is configured as 32-bit stereo which gives us 16-bit 4-channel
  // GMF Pipeline will handle rate conversion if needed, but we capture at
  // native high rate for quality
  if (cfg == AUDIO_CODEC_OPUS) {
    fs.sample_rate = 48000; // Capture at 48k for AEC (will downsample in GMF)
    fs.channel = 2;         // Stereo for TDM (32-bit stereo = 16-bit 4-channel)
    fs.bits_per_sample = 32; // 32-bit stereo mode

    // Playback configured in GMF pipeline
  } else {
    // For G711 we might still want AEC path, so stick to high quality capture
    fs.sample_rate = 48000;
    fs.channel = 2;
    fs.bits_per_sample = 32;
  }
  esp_codec_dev_open(record_handle, &fs);
  ESP_LOGI(TAG, "Codec opened: rate=%lu ch=%d bits=%d", fs.sample_rate,
           fs.channel, fs.bits_per_sample);
#endif

  esp_audio_err_t enc_err = esp_audio_enc_open(&enc_cfg, &enc_handle);
  if (enc_err != ESP_AUDIO_ERR_OK || !enc_handle) {
    ESP_LOGE(TAG, "Failed to open audio encoder: %d", enc_err);
    return ESP_FAIL;
  }

  // Get buffer sizes from encoder
  esp_audio_enc_get_frame_size(enc_handle, &read_size, &out_size);

  read_buf = malloc(read_size);
  write_buf = malloc(out_size);
  if (!read_buf || !write_buf) {
    ESP_LOGE(TAG, "Failed to allocate encoder buffers");
    return ESP_FAIL;
  }

  aenc_in_frame.buffer = read_buf;
  aenc_in_frame.len = read_size;
  aenc_out_frame.buffer = write_buf;
  aenc_out_frame.len = out_size;

  // Initialize GMF AEC Pipeline
#if CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD
  encoder_frame_bytes = read_size;

  if (aec_accum_buf)
    free(aec_accum_buf);
  aec_accum_buf = malloc(encoder_frame_bytes);
  aec_accum_pos = 0;
  if (!aec_accum_buf) {
    ESP_LOGE(TAG, "Failed to allocate accumulation buffer");
    return ESP_FAIL;
  }

  // 1. Initialize Ring Buffer
  aec_out_rb = xRingbufferCreate(16 * 1024, RINGBUF_TYPE_BYTEBUF);
  if (!aec_out_rb) {
    ESP_LOGE(TAG, "Failed to create Ring Buffer");
    return ESP_FAIL;
  }

  // 2. Initialize GMF Pool & Loader
  esp_gmf_pool_init(&pool);
  gmf_loader_setup_io_default(pool);
  gmf_loader_setup_ai_audio_default(pool);
  gmf_loader_setup_audio_effects_default(pool);
  gmf_loader_setup_audio_codec_default(
      pool); // ← Add decoder/encoder registration

  // 3. Create Pipeline
  const char *elements[] = {"aud_rate_cvt", "ai_aec"};
  esp_gmf_pool_new_pipeline(pool, "io_codec_dev", elements, 2, NULL, &aec_pipe);

  if (aec_pipe == NULL) {
    ESP_LOGE(TAG, "Failed to create GMF pipeline");
    return ESP_FAIL;
  }

  // 4. Connect Input (Record Handle)
  esp_gmf_io_codec_dev_set_dev(ESP_GMF_PIPELINE_GET_IN_INSTANCE(aec_pipe),
                               record_handle);

  // 5. Connect Output (RingBuffer Callback)
  esp_gmf_port_handle_t out_port = NEW_ESP_GMF_PORT_OUT_BYTE(
      no_op_acquire, aec_output_callback, NULL, NULL, 1024, portMAX_DELAY);
  esp_gmf_element_register_out_port(aec_pipe->last_el, out_port);

  // 6. Configure Elements
  // Rate Converter: 48k -> 16k
  esp_gmf_obj_handle_t rate_cvt = NULL;
  esp_gmf_pipeline_get_el_by_name(aec_pipe, "aud_rate_cvt", &rate_cvt);
  esp_gmf_rate_cvt_set_dest_rate(rate_cvt, 16000);

  // AEC Input Format
  esp_gmf_info_sound_t info = {
      .sample_rates = 48000,
      .channels = 4, // TDM 4 channel
      .bits = 16,    // 16-bit
  };
  esp_gmf_pipeline_report_info(aec_pipe, ESP_GMF_INFO_SOUND, &info,
                               sizeof(info));

  // 7. Start GMF Task
  esp_gmf_task_cfg_t gmf_cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
  gmf_cfg.thread.core = 1;
  gmf_cfg.thread.stack = 5 * 1024;
  gmf_cfg.thread.prio = 10;
  esp_gmf_task_init(&gmf_cfg, &gmf_task);
  esp_gmf_pipeline_bind_task(aec_pipe, gmf_task);
  esp_gmf_pipeline_loading_jobs(aec_pipe);
  esp_gmf_pipeline_run(aec_pipe);

  ESP_LOGI(TAG, "GMF AEC Pipeline Started for Korvo-2");

#endif

  return ESP_OK;
}

// esp_err_t audio_av_render_init(audio_codec_t codec) {
//   esp_audio_dec_register_default();
//   i2s_render_cfg_t i2s_cfg = {
//       // videodemo .fixed_clock = true,
//       .play_handle =
//           get_playback_handle(), // You must ensure this returns valid handle
//   };

//   player_sys.audio_render = av_render_alloc_i2s_render(&i2s_cfg);
//   if (!player_sys.audio_render) {
//     ESP_LOGE(TAG, "Failed to allocate av_render handle");
//     return ESP_FAIL;
//   }

//   esp_codec_dev_set_out_vol(i2s_cfg.play_handle, 100);
//   av_render_cfg_t render_cfg = {
//       .audio_render = player_sys.audio_render,
//       .audio_raw_fifo_size = 8 * 4096,
//       .audio_render_fifo_size = 100 * 1024,
//       .allow_drop_data = false,
//   };

//   player_sys.player = av_render_open(&render_cfg);
//   if (codec == AUDIO_CODEC_OPUS) {
//     audio_info.codec = AV_RENDER_AUDIO_CODEC_OPUS;
//     audio_info.sample_rate = opus_sample_rate;
//     audio_info.channel = channel;
//     audio_info.bits_per_sample = bits_per_sample;

//     aud_info.sample_rate = opus_sample_rate;
//     aud_info.channel = 1;
//     aud_info.bits_per_sample = bits_per_sample;
//   } else if (codec == AUDIO_CODEC_PCMA) {
//     audio_info.codec = AV_RENDER_AUDIO_CODEC_PCMA;
//     audio_info.sample_rate = pcma_pcmu_sample_rate;
//     audio_info.channel = channel;
//     audio_info.bits_per_sample = bits_per_sample;

//     aud_info.sample_rate = pcma_pcmu_sample_rate;
//     aud_info.channel = channel;
//     aud_info.bits_per_sample = bits_per_sample;
//   } else {
//     audio_info.codec = AV_RENDER_AUDIO_CODEC_PCMU;
//     audio_info.sample_rate = pcma_pcmu_sample_rate;
//     audio_info.channel = channel;
//     audio_info.bits_per_sample = bits_per_sample;

//     aud_info.sample_rate = pcma_pcmu_sample_rate;
//     aud_info.channel = channel;
//     aud_info.bits_per_sample = bits_per_sample;
//   }

//   av_render_set_fixed_frame_info(player_sys.player, &aud_info);

//   int ret = av_render_add_audio_stream(player_sys.player, &audio_info);
//   if (ret != 0) {
//     ESP_LOGE(TAG, "Failed to add audio stream to av_render (%d)", ret);
//     return ESP_FAIL;
//   }

//   // ESP_LOGI(TAG, "AV Render initialized and stream added");
//   return ESP_OK;
// }

#if CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD
esp_err_t audio_gmf_playback_init(audio_codec_t codec) {
  // 0. Initialize GMF pool if not already done (for subscribe-only mode)
  if (!pool) {
    ESP_LOGI(TAG, "Initializing GMF pool for playback");
    esp_gmf_pool_init(&pool);
    gmf_loader_setup_io_default(pool);
    gmf_loader_setup_ai_audio_default(pool);
    gmf_loader_setup_audio_effects_default(pool);
    gmf_loader_setup_audio_codec_default(pool);
  }

  // 1. Create playback ring buffer for incoming network audio
  playback_rb = xRingbufferCreate(32 * 1024, RINGBUF_TYPE_BYTEBUF);
  if (!playback_rb) {
    ESP_LOGE(TAG, "Failed to create playback ring buffer");
    return ESP_FAIL;
  }

  // 2. Create GMF playback pipeline
  // Pipeline: RingBuffer → Decoder → Rate Convert → Channel Convert → Bit
  // Convert → Codec Device (ES8311)
  // Pipeline: RingBuffer(Opus) → Decoder → Rate/Ch/Bit Convert → ES8311
  const char *elements[] = {"aud_dec", "aud_rate_cvt", "aud_ch_cvt",
                            "aud_bit_cvt"};
  esp_gmf_pool_new_pipeline(pool, NULL, elements, 4, "io_codec_dev",
                            &playback_pipe);

  if (playback_pipe == NULL) {
    ESP_LOGE(TAG, "Failed to create GMF playback pipeline");
    vRingbufferDelete(playback_rb);
    playback_rb = NULL;
    return ESP_FAIL;
  }

  // 3. Get decoder element and connect input (ring buffer with Opus frames)
  esp_gmf_obj_handle_t dec_el = NULL;
  esp_gmf_pipeline_get_el_by_name(playback_pipe, "aud_dec", &dec_el);
  if (!dec_el) {
    ESP_LOGE(TAG, "Failed to get decoder element");
    return ESP_FAIL;
  }

  esp_gmf_port_handle_t in_port =
      NEW_ESP_GMF_PORT_IN_BYTE(playback_rb_acquire, playback_rb_release, NULL,
                               playback_rb, 1024, portMAX_DELAY);
  esp_gmf_element_register_in_port(dec_el, in_port);

  // 4. Connect output to ES8311 DAC
  esp_gmf_io_codec_dev_set_dev(ESP_GMF_PIPELINE_GET_OUT_INSTANCE(playback_pipe),
                               get_playback_handle());

  // 5. Configure GMF Opus decoder
  esp_gmf_info_sound_t opus_info = {
      .format_id = ESP_AUDIO_TYPE_OPUS, // Opus decoder
      .sample_rates = 16000,            // Incoming Opus is 16kHz
      .channels = 1,                    // Mono
      .bits = 16,
  };
  esp_gmf_pipeline_report_info(playback_pipe, ESP_GMF_INFO_SOUND, &opus_info,
                               sizeof(opus_info));

  // 6. Configure rate converter (upsample to 48kHz for DAC)
  esp_gmf_obj_handle_t rate_cvt = NULL;
  esp_gmf_pipeline_get_el_by_name(playback_pipe, "aud_rate_cvt", &rate_cvt);
  esp_gmf_rate_cvt_set_dest_rate(rate_cvt, 48000); // ES8311 runs at 48kHz

  // 7. Explicitly configure channel converter (mono → stereo for ES8311)
  esp_gmf_obj_handle_t ch_cvt = NULL;
  esp_gmf_pipeline_get_el_by_name(playback_pipe, "aud_ch_cvt", &ch_cvt);
  esp_gmf_ch_cvt_set_dest_channel(ch_cvt, 2); // ES8311 needs stereo

  // 8. Explicitly configure bit converter (16-bit → 32-bit for ES8311)
  esp_gmf_obj_handle_t bit_cvt = NULL;
  esp_gmf_pipeline_get_el_by_name(playback_pipe, "aud_bit_cvt", &bit_cvt);
  esp_gmf_bit_cvt_set_dest_bits(bit_cvt, 32); // ES8311 uses 32-bit I2S

  // 9. Start playback pipeline on Core 0 (AEC is on Core 1)
  esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
  cfg.thread.core = 0; // Different core from AEC
  cfg.thread.stack = 10 * 1024;  // Increased for Opus decoding (needs ~1920 bytes output buffer)
  cfg.thread.prio = 10;
  esp_gmf_task_init(&cfg, &playback_task);

  esp_gmf_pipeline_bind_task(playback_pipe, playback_task);
  esp_gmf_pipeline_loading_jobs(playback_pipe);
  esp_gmf_pipeline_run(playback_pipe);

  ESP_LOGI(TAG, "GMF Playback Pipeline Started on Core 0");
  return ESP_OK;
}
#endif

void audio_deinit(void) {
#if CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD
  // Cleanup playback pipeline
  if (playback_task) {
    esp_gmf_task_deinit(playback_task);
    playback_task = NULL;
  }
  if (playback_pipe) {
    esp_gmf_pipeline_stop(playback_pipe);
    esp_gmf_pipeline_destroy(playback_pipe);
    playback_pipe = NULL;
  }
  if (playback_rb) {
    vRingbufferDelete(playback_rb);
    playback_rb = NULL;
  }

  // Cleanup AEC pipeline
  if (gmf_task) {
    esp_gmf_task_deinit(gmf_task);
    gmf_task = NULL;
  }
  if (aec_pipe) {
    esp_gmf_pipeline_stop(aec_pipe);
    esp_gmf_pipeline_destroy(aec_pipe);
    aec_pipe = NULL;
  }
  if (pool) {
    gmf_loader_teardown_all_defaults(pool);
    esp_gmf_pool_deinit(pool);
    pool = NULL;
  }
  if (aec_out_rb) {
    vRingbufferDelete(aec_out_rb);
    aec_out_rb = NULL;
  }
  if (aec_accum_buf) {
    free(aec_accum_buf);
    aec_accum_buf = NULL;
    aec_accum_pos = 0;
  }
#endif

  if (enc_handle) {
    esp_audio_enc_close(enc_handle);
    enc_handle = NULL;
  }
  if (read_buf) {
    free(read_buf);
    read_buf = NULL;
  }
  if (write_buf) {
    free(write_buf);
    write_buf = NULL;
  }
}

void audio_task(void *arg) {
#if CONFIG_ESP32S3_XIAO
  int32_t audio_get_mono_samples(uint8_t *buf, size_t size) {
    size_t bytes_read = 0;
    esp_err_t err =
        i2s_channel_read(rx_handle, (char *)buf, size, &bytes_read, 100);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "i2s read error: %d", err);
    }
    return bytes_read;
  }
#endif

  int ret;
  float bytes = 0;
  size_t item_size;

  for (;;) {
    if (eState == PEER_CONNECTION_COMPLETED) {
      taskYIELD();

#if CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD
      // Read from Ring Buffer (populated by GMF AEC)
      // Wait for up to 20ms for data
      char *pcm_data =
          (char *)xRingbufferReceive(aec_out_rb, &item_size, pdMS_TO_TICKS(20));

      if (pcm_data != NULL) {
        size_t bytes_to_copy = item_size;
        size_t copied = 0;

        // Process the chunk by filling the accumulator
        while (copied < bytes_to_copy) {
          size_t space_left = encoder_frame_bytes - aec_accum_pos;
          size_t chunk = (bytes_to_copy - copied < space_left)
                             ? (bytes_to_copy - copied)
                             : space_left;

          memcpy(aec_accum_buf + aec_accum_pos, pcm_data + copied, chunk);
          aec_accum_pos += chunk;
          copied += chunk;

          // If accumulator is full, encode one frame
          if (aec_accum_pos == encoder_frame_bytes) {
            memcpy(aenc_in_frame.buffer, aec_accum_buf, encoder_frame_bytes);
            aenc_in_frame.len = encoder_frame_bytes; // Ensure exact size
            aec_accum_pos = 0;                       // Reset accumulator

            esp_audio_err_t enc_ret = esp_audio_enc_process(
                enc_handle, &aenc_in_frame, &aenc_out_frame);

            if (enc_ret == ESP_AUDIO_ERR_OK) {
              int send_ret = peer_connection_send_audio(
                  g_pc_publish, aenc_out_frame.buffer,
                  aenc_out_frame.encoded_bytes);
              if (send_ret < 0) {
                ESP_LOGE(TAG, "FAILED TO SEND AUDIO");
              } else {
                ESP_LOGD(TAG, "Audio Sent (GMF AEC)");
              }
              bytes += aenc_out_frame.encoded_bytes;
            } else {
              ESP_LOGE(TAG, "AUDIO ENCODE FAILED");
            }
          }
        }

        // Return item to Ring Buffer
        vRingbufferReturnItem(aec_out_rb, (void *)pcm_data);
      }

#elif CONFIG_ESP32S3_XIAO
      // For XIAO: No AEC, read mono
      ret = audio_get_mono_samples(aenc_in_frame.buffer, aenc_in_frame.len);
      if (ret == aenc_in_frame.len) {
        taskYIELD();
        esp_audio_err_t enc_ret =
            esp_audio_enc_process(enc_handle, &aenc_in_frame, &aenc_out_frame);
        if (enc_ret == ESP_AUDIO_ERR_OK) {
          int send_ret =
              peer_connection_send_audio(g_pc_publish, aenc_out_frame.buffer,
                                         aenc_out_frame.encoded_bytes);
          if (send_ret < 0) {
            ESP_LOGE(TAG, "FAILED TO SEND AUDIO");
          } else {
            ESP_LOGI(TAG, "Audio Sent\n");
          }
          bytes += aenc_out_frame.encoded_bytes;
        } else {
          ESP_LOGE(TAG, "AUDIO ENCODE FAILED");
        }
      } else {
        ESP_LOGE(TAG, "FAILED TO READ AUDIO");
      }
#endif

      vTaskDelay(pdMS_TO_TICKS(5));
    } else {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

bool remove_custom_header(const uint8_t *input_data, size_t input_len,
                          out_frame_opus_t *out_frame) {
  if (input_len < CUSTOM_HEADER_SIZE) {
    ESP_LOGW(TAG, "Payload too small for header removal: %d bytes",
             (int)input_len);
    return false;
  }

  if (input_data[0] != 0xBE || input_data[1] != 0xDE) {
    ESP_LOGE(TAG, "Unexpected magic bytes: %02X %02X", input_data[0],
             input_data[1]);
    // optionally return false here if strict
  }
  ESP_LOGI(TAG, "no header data length: %d", input_len);
  out_frame->data = input_data + CUSTOM_HEADER_SIZE;
  out_frame->len = input_len - CUSTOM_HEADER_SIZE;

  return true;
}
void audio_receive_and_render(uint8_t *encoded_data, size_t encoded_len,
                              void *userdata) {
  ESP_LOGD(TAG, "audio_receive_and_render: encoded_len=%d", (int)encoded_len);

  const uint8_t *final_audio_data = NULL;
  size_t final_audio_len = 0;
  int ret = ESP_AUDIO_ERR_OK;

  if (!encoded_data || encoded_len == 0) {
    ESP_LOGW(TAG, "Invalid encoded audio input");
    return;
  }

  if (!remove_custom_header(encoded_data, encoded_len, &opus_out_frame)) {
    ESP_LOGE(TAG, "Failed to remove custom header");
    return;
  }

  ESP_LOGD(TAG, "Header removed: opus_len=%d", (int)opus_out_frame.len);

#if CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD

  // Send raw Opus frames directly to GMF pipeline (GMF handles decoding)
  if (playback_rb && opus_out_frame.len > 0) {
    if (xRingbufferSend(playback_rb, opus_out_frame.data, opus_out_frame.len,
                        pdMS_TO_TICKS(10)) != pdTRUE) {
      ESP_LOGW(TAG, "Playback RB full, dropped %d bytes Opus",
               (int)opus_out_frame.len);
    } else {
      ESP_LOGD(TAG, "Sent %d bytes Opus to GMF playback pipeline",
               (int)opus_out_frame.len);
    }
  }

#else // Other boards (XIAO)

  if (!final_audio_data || final_audio_len == 0) {
    ESP_LOGW(TAG, "No PCM data to render");
    return;
  }

  av_render_audio_data_t audio_data = {
      .pts = 0,
      .data = (uint8_t *)final_audio_data,
      .size = final_audio_len,
  };

  ret = av_render_add_audio_data(player_sys.player, &audio_data);
  if (ret != 0) {
    ESP_LOGE(TAG, "Audio render failed (ret=%d)", ret);
  } else {
    ESP_LOGD(TAG, "Rendered %d bytes of audio", (int)final_audio_len);
  }

#endif
}
