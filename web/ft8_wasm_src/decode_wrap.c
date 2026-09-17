// decode_wrap.c
// MIT-licensed wrapper around kgoba/ft8_lib exposing FT8 and FT4 decode
// as two independent, runtime-selectable entry points from one WASM build.
// Loosely modeled on ft8_lib's own demo/decode_ft8.c (also MIT).

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include <ft8/decode.h>
#include <ft8/encode.h>
#include <ft8/message.h>

#include <common/common.h>
#include <common/wave.h>
#include <common/monitor.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

const int kMax_decoded_messages = 50;
#define kMax_candidates_cap 400
const int kFreq_osr = 2;
const int kTime_osr = 2;
const int kSampleRate = 12000;

#define CALLSIGN_HASHTABLE_SIZE 256
#define RESULT_BUF_SIZE 4096

typedef struct
{
    char callsign[12];
    uint32_t hash;
} hash_entry_t;

// One independent hashtable + monitor per protocol so FT8/FT4 state never mixes.
typedef struct
{
    monitor_t mon;
    bool mon_ready;
    hash_entry_t table[CALLSIGN_HASHTABLE_SIZE];
    int table_size;
    ftx_protocol_t protocol;
} decoder_ctx_t;

static decoder_ctx_t ctx_ft8;
static decoder_ctx_t ctx_ft4;

static void hashtable_init(decoder_ctx_t* ctx)
{
    ctx->table_size = 0;
    memset(ctx->table, 0, sizeof(ctx->table));
}

static void hashtable_cleanup(decoder_ctx_t* ctx, uint8_t max_age)
{
    for (int idx_hash = 0; idx_hash < CALLSIGN_HASHTABLE_SIZE; ++idx_hash)
    {
        if (ctx->table[idx_hash].callsign[0] != '\0')
        {
            uint8_t age = (uint8_t)(ctx->table[idx_hash].hash >> 24);
            if (age > max_age)
            {
                ctx->table[idx_hash].callsign[0] = '\0';
                ctx->table[idx_hash].hash = 0;
                ctx->table_size--;
            }
            else
            {
                ctx->table[idx_hash].hash = (((uint32_t)age + 1u) << 24) | (ctx->table[idx_hash].hash & 0x3FFFFFu);
            }
        }
    }
}

static void hashtable_add(decoder_ctx_t* ctx, const char* callsign, uint32_t hash)
{
    uint16_t hash10 = (hash >> 12) & 0x3FFu;
    int idx_hash = (hash10 * 23) % CALLSIGN_HASHTABLE_SIZE;
    while (ctx->table[idx_hash].callsign[0] != '\0')
    {
        if (((ctx->table[idx_hash].hash & 0x3FFFFFu) == hash) && (0 == strcmp(ctx->table[idx_hash].callsign, callsign)))
        {
            ctx->table[idx_hash].hash &= 0x3FFFFFu;
            return;
        }
        idx_hash = (idx_hash + 1) % CALLSIGN_HASHTABLE_SIZE;
    }
    ctx->table_size++;
    strncpy(ctx->table[idx_hash].callsign, callsign, 11);
    ctx->table[idx_hash].callsign[11] = '\0';
    ctx->table[idx_hash].hash = hash;
}

static bool hashtable_lookup(ftx_callsign_hash_type_t hash_type, uint32_t hash, char* callsign, void* user_data)
{
    decoder_ctx_t* ctx = (decoder_ctx_t*)user_data;
    uint8_t hash_shift = (hash_type == FTX_CALLSIGN_HASH_10_BITS) ? 12 : (hash_type == FTX_CALLSIGN_HASH_12_BITS ? 10 : 0);
    uint16_t hash10 = (hash >> (12 - hash_shift)) & 0x3FFu;
    int idx_hash = (hash10 * 23) % CALLSIGN_HASHTABLE_SIZE;
    while (ctx->table[idx_hash].callsign[0] != '\0')
    {
        if (((ctx->table[idx_hash].hash & 0x3FFFFFu) >> hash_shift) == hash)
        {
            strcpy(callsign, ctx->table[idx_hash].callsign);
            return true;
        }
        idx_hash = (idx_hash + 1) % CALLSIGN_HASHTABLE_SIZE;
    }
    callsign[0] = '\0';
    return false;
}

static bool hashtable_lookup_thunk_ft8(ftx_callsign_hash_type_t t, uint32_t h, char* c) { return hashtable_lookup(t, h, c, &ctx_ft8); }
static bool hashtable_lookup_thunk_ft4(ftx_callsign_hash_type_t t, uint32_t h, char* c) { return hashtable_lookup(t, h, c, &ctx_ft4); }
static void hashtable_add_thunk_ft8(const char* c, uint32_t h) { hashtable_add(&ctx_ft8, c, h); }
static void hashtable_add_thunk_ft4(const char* c, uint32_t h) { hashtable_add(&ctx_ft4, c, h); }

static ftx_callsign_hash_interface_t hash_if_ft8 = { .lookup_hash = hashtable_lookup_thunk_ft8, .save_hash = hashtable_add_thunk_ft8 };
static ftx_callsign_hash_interface_t hash_if_ft4 = { .lookup_hash = hashtable_lookup_thunk_ft4, .save_hash = hashtable_add_thunk_ft4 };

static void run_decode(decoder_ctx_t* ctx, ftx_callsign_hash_interface_t* hash_if, char* results,
                        int min_score, int ldpc_iterations, int max_candidates)
{
    if (max_candidates > kMax_candidates_cap)
        max_candidates = kMax_candidates_cap;
    if (max_candidates < 1)
        max_candidates = 1;

    const ftx_waterfall_t* wf = &ctx->mon.wf;
    ftx_candidate_t candidate_list[kMax_candidates_cap];
    int num_candidates = ftx_find_candidates(wf, max_candidates, candidate_list, min_score);

    int num_decoded = 0;
    ftx_message_t decoded[kMax_decoded_messages];
    ftx_message_t* decoded_hashtable[kMax_decoded_messages];
    for (int i = 0; i < kMax_decoded_messages; ++i)
        decoded_hashtable[i] = NULL;

    results[0] = '\0';
    size_t results_used = 0;

    for (int idx = 0; idx < num_candidates; ++idx)
    {
        const ftx_candidate_t* cand = &candidate_list[idx];
        float freq_hz = (ctx->mon.min_bin + cand->freq_offset + (float)cand->freq_sub / wf->freq_osr) / ctx->mon.symbol_period;
        float time_sec = (cand->time_offset + (float)cand->time_sub / wf->time_osr) * ctx->mon.symbol_period;

        ftx_message_t message;
        ftx_decode_status_t status;
        if (!ftx_decode_candidate(wf, cand, ldpc_iterations, &message, &status))
        {
            continue;
        }

        int idx_hash = message.hash % kMax_decoded_messages;
        bool found_empty_slot = false;
        bool found_duplicate = false;
        do
        {
            if (decoded_hashtable[idx_hash] == NULL)
            {
                found_empty_slot = true;
            }
            else if ((decoded_hashtable[idx_hash]->hash == message.hash) && (0 == memcmp(decoded_hashtable[idx_hash]->payload, message.payload, sizeof(message.payload))))
            {
                found_duplicate = true;
            }
            else
            {
                idx_hash = (idx_hash + 1) % kMax_decoded_messages;
            }
        } while (!found_empty_slot && !found_duplicate);

        if (found_empty_slot)
        {
            memcpy(&decoded[idx_hash], &message, sizeof(message));
            decoded_hashtable[idx_hash] = &decoded[idx_hash];
            ++num_decoded;

            char text[FTX_MAX_MESSAGE_LENGTH];
            ftx_message_offsets_t offsets;
            ftx_message_rc_t unpack_status = ftx_message_decode(&message, hash_if, text, &offsets);
            if (unpack_status != FTX_MESSAGE_RC_OK)
            {
                snprintf(text, sizeof(text), "Error [%d] while unpacking!", (int)unpack_status);
            }

            // Same rough approximation used by ft8_lib's own demo (score * 0.5).
            // Not a calibrated SNR-in-dB value -- treat as a relative signal score.
            float score = cand->score * 0.5f;

            char line[FTX_MAX_MESSAGE_LENGTH + 48];
            int n = snprintf(line, sizeof(line), "%.1f,%.2f,%.1f,%s\n", score, time_sec, freq_hz, text);
            if (n > 0 && results_used + (size_t)n < RESULT_BUF_SIZE - 1)
            {
                memcpy(results + results_used, line, (size_t)n);
                results_used += (size_t)n;
                results[results_used] = '\0';
            }
        }
    }
    hashtable_cleanup(ctx, 10);
}

static void ctx_init(decoder_ctx_t* ctx, ftx_protocol_t protocol)
{
    monitor_config_t mon_cfg = {
        .f_min = 200,
        .f_max = 3000,
        .sample_rate = kSampleRate,
        .time_osr = kTime_osr,
        .freq_osr = kFreq_osr,
        .protocol = protocol
    };
    hashtable_init(ctx);
    monitor_init(&ctx->mon, &mon_cfg);
    ctx->protocol = protocol;
    ctx->mon_ready = true;
}

// ---- Exported API ----
// init_decode_ft8 / init_decode_ft4: call once (idempotent) before decoding.
// exec_decode_ft8 / exec_decode_ft4: signal = 12kHz mono float samples,
//   num_samples = length of `signal`. min_score/ldpc_iterations/max_candidates
//   are the "depth" knobs (lower min_score + more iterations + more candidates
//   = slower but more sensitive). results must point to a caller-owned buffer
//   of at least RESULT_BUF_SIZE bytes; on return it holds zero or more lines
//   "score,time_sec,freq_hz,text\n".

EMSCRIPTEN_KEEPALIVE
void init_decode_ft8(void)
{
    if (!ctx_ft8.mon_ready)
        ctx_init(&ctx_ft8, FTX_PROTOCOL_FT8);
}

EMSCRIPTEN_KEEPALIVE
void init_decode_ft4(void)
{
    if (!ctx_ft4.mon_ready)
        ctx_init(&ctx_ft4, FTX_PROTOCOL_FT4);
}

static void exec_decode_common(decoder_ctx_t* ctx, ftx_callsign_hash_interface_t* hash_if, float* signal, int num_samples,
                                int min_score, int ldpc_iterations, int max_candidates, char* results)
{
    monitor_reset(&ctx->mon);
    for (int frame_pos = 0; frame_pos + ctx->mon.block_size <= num_samples; frame_pos += ctx->mon.block_size)
    {
        monitor_process(&ctx->mon, signal + frame_pos);
    }
    run_decode(ctx, hash_if, results, min_score, ldpc_iterations, max_candidates);
}

EMSCRIPTEN_KEEPALIVE
void exec_decode_ft8(float* signal, int num_samples, int min_score, int ldpc_iterations, int max_candidates, char* results)
{
    if (!ctx_ft8.mon_ready)
        ctx_init(&ctx_ft8, FTX_PROTOCOL_FT8);
    exec_decode_common(&ctx_ft8, &hash_if_ft8, signal, num_samples, min_score, ldpc_iterations, max_candidates, results);
}

EMSCRIPTEN_KEEPALIVE
void exec_decode_ft4(float* signal, int num_samples, int min_score, int ldpc_iterations, int max_candidates, char* results)
{
    if (!ctx_ft4.mon_ready)
        ctx_init(&ctx_ft4, FTX_PROTOCOL_FT4);
    exec_decode_common(&ctx_ft4, &hash_if_ft4, signal, num_samples, min_score, ldpc_iterations, max_candidates, results);
}
