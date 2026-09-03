# SidBox Audio Format v1

All integer fields are little-endian.

## File Header

Offset | Size | Name | Value
--- | ---: | --- | ---
0 | 4 | magic | `SBA1`
4 | 2 | header_size | `64`
6 | 2 | version | `1`
8 | 4 | sample_rate | `44100`
12 | 2 | channels | `2`
14 | 2 | bits_per_sample | `16`
16 | 4 | block_frames | default `2048`
20 | 4 | block_count | number of blocks
24 | 8 | total_frames | total stereo frames
32 | 4 | table_offset | usually `64`
36 | 4 | data_offset | first payload byte
40 | 4 | flags | `0`
44 | 4 | header_crc32 | CRC32 of bytes `0..43`, with this field treated as zero
48 | 16 | reserved | zero

## Block Table Entry

One 16-byte entry per block.

Offset | Size | Name
--- | ---: | ---
0 | 4 | payload_offset, relative to `data_offset`
4 | 4 | payload_size
8 | 2 | frames
10 | 1 | method
11 | 1 | reserved
12 | 4 | decoded_crc32

Method `0` is raw signed 16-bit stereo PCM, interleaved L/R.

Method `1` is split-channel delta varint:

```text
left_initial_i16
left_delta_varints...
right_initial_i16
right_delta_varints...
```

For each channel, the decoder outputs `frames` samples. Deltas are applied from the previous decoded sample.

Signed deltas are encoded as zig-zag unsigned integers:

```c
zigzag = (delta << 1) ^ (delta >> 31)
```

Unsigned integers are encoded using standard 7-bit LEB128.
