[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: ima_bswap16
[Debug] Collected MemberExpr accesses:
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: ima_bswap32
[Debug] Collected MemberExpr accesses:
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: ima_bswap64
[Debug] Collected MemberExpr accesses:
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: ima_btoh16
[Debug] Collected MemberExpr accesses:
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: ima_btoh32
[Debug] Collected MemberExpr accesses:
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: ima_btoh64
[Debug] Collected MemberExpr accesses:
[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: type
[Debug] Visiting MemberExpr: version
[Debug] Visiting MemberExpr: type
[Debug] Visiting MemberExpr: size
[Debug] Visiting MemberExpr: format_id
[Debug] Visiting MemberExpr: blocks
[Debug] Visiting MemberExpr: size
[Debug] Visiting MemberExpr: frame_count
[Debug] Visiting MemberExpr: frame_count
[Debug] Visiting MemberExpr: channel_count
[Debug] Visiting MemberExpr: channels_per_frame
[Debug] Visiting MemberExpr: u
[Debug] Parent is union: (anonymous)
[Debug] Visiting MemberExpr: sample_rate
[Debug] Visiting MemberExpr: u
[Debug] Parent is union: (anonymous)
[Debug] Visiting MemberExpr: u
[Debug] Parent is union: (anonymous)
[Debug] Visiting MemberExpr: sample_rate
[Debug] Visiting MemberExpr: f
[Debug] Parent is union: (anonymous)
[Debug] Done traversing Function Body for union accesses
[Debug] Function: ima_parse
[Debug] Collected MemberExpr accesses:
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/ex2.cpp:102:5)
[Debug] Collected MemberExpr accesses:
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/ex2.cpp ===
//#include <stddef.h>

typedef unsigned int ima_u32_t;
typedef unsigned long long ima_u64_t;
typedef double ima_f64_t;

typedef unsigned char ima_u8_t;
typedef unsigned short ima_u16_t;

struct ima_block {
    ima_u16_t preamble;
    ima_u8_t data[(32)];
};

struct ima_info {
    const struct ima_block *blocks;
    ima_u64_t size;
    ima_f64_t sample_rate;
    ima_u64_t frame_count;
    ima_u32_t channel_count;
};

typedef signed int ima_s32_t;
typedef signed long long ima_s64_t;

struct caf_header {
    ima_u32_t type;
    ima_u16_t version;
    ima_u16_t flags;
};

struct caf_chunk {
    ima_u32_t type;
    ima_s64_t size;
};

struct caf_audio_description {
    ima_f64_t sample_rate;
    ima_u32_t format_id;
    ima_u32_t format_flags;
    ima_u32_t bytes_per_packet;
    ima_u32_t frames_per_packet;
    ima_u32_t channels_per_frame;
    ima_u32_t bits_per_channel;
};

struct caf_packet_table {
    ima_s64_t packet_count;
    ima_s64_t frame_count;
    ima_s32_t priming_frames;
    ima_s32_t remainder_frames;
};

struct caf_data {
    ima_u32_t edit_count;
};

static ima_u16_t
ima_bswap16(ima_u16_t v) {
    return (v << 0x08 & 0xff00u) | (v >> 0x08 & 0x00ffu);
}

static ima_u32_t
ima_bswap32(ima_u32_t v) {
    return (v << 0x18 & 0xff000000ul) | (v << 0x08 & 0x00ff0000ul) |
           (v >> 0x08 & 0x0000ff00ul) | (v >> 0x18 & 0x000000fful);
}

static ima_u64_t
ima_bswap64(ima_u64_t v) {
    return (v << 0x38 & 0xff00000000000000ull) |
           (v << 0x28 & 0x00ff000000000000ull) |
           (v << 0x18 & 0x0000ff0000000000ull) |
           (v << 0x08 & 0x000000ff00000000ull) |
           (v >> 0x08 & 0x00000000ff000000ull) |
           (v >> 0x18 & 0x0000000000ff0000ull) |
           (v >> 0x28 & 0x000000000000ff00ull) |
           (v >> 0x38 & 0x00000000000000ffull);
}

static ima_u16_t ima_btoh16(ima_u16_t v) {
    return ima_bswap16(v);
}

static ima_u32_t ima_btoh32(ima_u32_t v) {
    return ima_bswap32(v);
}

static ima_u64_t ima_btoh64(ima_u64_t v) {
    return ima_bswap64(v);
}

int ima_parse(struct ima_info *info, const void *data) {
    const struct caf_header *header = (const struct caf_header *)data;
    const struct caf_chunk *chunk = (const struct caf_chunk *)&header[1];
    //const struct caf_audio_description *desc = NULL;
    //const struct caf_packet_table *pakt = NULL;
    //const struct ima_block *blocks = NULL;
    const struct caf_audio_description *desc = nullptr;
    const struct caf_packet_table *pakt = nullptr;
    const struct ima_block *blocks = nullptr;
    union {
        ima_f64_t f;
        ima_u64_t u;
    } conv64;
    ima_s64_t chunk_size;
    unsigned chunk_type;
    if (ima_btoh32(header->type) !=
        ((ima_u32_t)(ima_u8_t)('f') | ((ima_u32_t)(ima_u8_t)('f') << 8) |
         ((ima_u32_t)(ima_u8_t)('a') << 16) |
         ((ima_u32_t)(ima_u8_t)('c') << 24)))
        return -1;
    if (ima_btoh16(header->version) != 1)
        return -2;
    for (;;) {
        chunk_type = ima_btoh32(chunk->type);
        chunk_size = ima_btoh64(chunk->size);
        if (chunk_type ==
            ((ima_u32_t)(ima_u8_t)('c') | ((ima_u32_t)(ima_u8_t)('s') << 8) |
             ((ima_u32_t)(ima_u8_t)('e') << 16) |
             ((ima_u32_t)(ima_u8_t)('d') << 24)))
            desc = (const struct caf_audio_description *)&chunk[1];
        else if (chunk_type == ((ima_u32_t)(ima_u8_t)('t') |
                                ((ima_u32_t)(ima_u8_t)('k') << 8) |
                                ((ima_u32_t)(ima_u8_t)('a') << 16) |
                                ((ima_u32_t)(ima_u8_t)('p') << 24)))
            pakt = (const struct caf_packet_table *)&chunk[1];
        else if (chunk_type == ((ima_u32_t)(ima_u8_t)('a') |
                                ((ima_u32_t)(ima_u8_t)('t') << 8) |
                                ((ima_u32_t)(ima_u8_t)('a') << 16) |
                                ((ima_u32_t)(ima_u8_t)('d') << 24))) {
            blocks = (const struct ima_block *)&(
                (const struct caf_data *)&chunk[1])[1];
            break;
        }
        chunk = (const struct caf_chunk *)((const ima_u8_t *)&chunk[1] +
                                           chunk_size);
    }
    if (ima_btoh32(desc->format_id) !=
        ((ima_u32_t)(ima_u8_t)('4') | ((ima_u32_t)(ima_u8_t)('a') << 8) |
         ((ima_u32_t)(ima_u8_t)('m') << 16) |
         ((ima_u32_t)(ima_u8_t)('i') << 24)))
        return -3;
    info->blocks = blocks;
    info->size = chunk_size;
    info->frame_count = ima_btoh64(pakt->frame_count);
    info->channel_count = ima_btoh32(desc->channels_per_frame);
    conv64.u = desc->sample_rate;
    conv64.u = ima_btoh64(*(const ima_u64_t *)&conv64.u);
    info->sample_rate = conv64.f;
    return 0;
}
