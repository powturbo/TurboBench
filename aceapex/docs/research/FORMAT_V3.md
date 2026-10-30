# ACEPX3 — Streaming Container Format

## Design Goals
- Encode without loading full file into RAM
- Decode chunk-by-chunk (low RAM)
- No global boffs table
- Self-contained chunks (recoverable)
- Future: random access, parallel decode

## File Layout

    [FileHeader]
    [ChunkRecord 0]
    [ChunkRecord 1]
    ...
    [ChunkRecord N-1]

## FileHeader (32 bytes)

    magic:      8 bytes  "ACEPX3\0\0"
    version:    4 bytes  = 3
    flags:      4 bytes  = 0
    orig_size:  8 bytes  total uncompressed size
    num_chunks: 4 bytes
    chunk_size: 4 bytes  uncompressed chunk size

## ChunkRecord

    [ChunkHeader]  48 bytes
    [lit_data]     lit_size bytes
    [off_data]     off_size bytes
    [len_data]     len_size bytes
    [cmd_data]     cmd_size bytes

## ChunkHeader (48 bytes)

    magic:      4 bytes  0x434B4E48 ('CHNK')
    flags:      4 bytes  0=normal
    raw_size:   8 bytes  uncompressed size of this chunk
    lit_size:   8 bytes  compressed literals
    off_size:   8 bytes  compressed offsets
    len_size:   8 bytes  compressed lengths
    cmd_size:   8 bytes  compressed commands

## Decoder Algorithm

    open file
    read FileHeader
    for each chunk:
        read ChunkHeader
        read lit_data, off_data, len_data, cmd_data
        decompress chunk → output
        write output

## Key Properties
- No random access needed for sequential decode
- Each chunk independently decodable
- RAM usage = 2 * chunk_size (in + out)
