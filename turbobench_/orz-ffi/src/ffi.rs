// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! C FFI bindings for the orz compressor.
//!
//! Link against the produced `liborz.a` / `liborz.so` (or `liborz.dylib` / `orz.dll`)
//! and include `orz.h`.
use std::{
    io::Cursor,
    os::raw::{c_int, c_void},
    ptr, slice,
};

use crate::{
    CountRead, CountWrite, LZCfg, ProgressLogger, SilentProgressLogger, decode, encode,
};

/// Error codes returned by the C API.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OrzError {
    Ok = 0,
    InvalidLevel = -1,
    NullPointer = -2,
    EncodeFailed = -3,
    DecodeFailed = -4,
    BufferTooSmall = -5,
    OutOfMemory = -6,
    InvalidInput = -7,
}

impl From<OrzError> for c_int {
    fn from(e: OrzError) -> c_int {
        e as c_int
    }
}

/// Compression configuration (same layout as internal `LZCfg`).
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct OrzCfg {
    pub match_depth: usize,
    pub lazy_match_depth1: usize,
    pub lazy_match_depth2: usize,
}

impl From<&OrzCfg> for LZCfg {
    fn from(c: &OrzCfg) -> LZCfg {
        LZCfg::new(c.match_depth, c.lazy_match_depth1, c.lazy_match_depth2)
    }
}

impl OrzCfg {
    /// Build configuration for the classic levels used by the CLI (0 / 1 / 2).
    pub fn from_level(level: u8) -> Option<Self> {
        match level {
            0 => Some(Self {
                match_depth: 5,
                lazy_match_depth1: 3,
                lazy_match_depth2: 2,
            }),
            1 => Some(Self {
                match_depth: 15,
                lazy_match_depth1: 9,
                lazy_match_depth2: 6,
            }),
            2 => Some(Self {
                match_depth: 45,
                lazy_match_depth1: 27,
                lazy_match_depth2: 18,
            }),
            _ => None,
        }
    }
}

/// Opaque buffer returned by the allocate-style API.
/// Caller must free with `orz_buffer_free`.
#[repr(C)]
pub struct OrzBuffer {
    pub data: *mut u8,
    pub len: usize,
    pub capacity: usize,
}

impl OrzBuffer {
    fn from_vec(mut v: Vec<u8>) -> Self {
        let data = v.as_mut_ptr();
        let len = v.len();
        let capacity = v.capacity();
        std::mem::forget(v);
        Self {
            data,
            len,
            capacity,
        }
    }

    unsafe fn into_vec(self) -> Vec<u8> {
        if self.data.is_null() {
            Vec::new()
        } else {
            Vec::from_raw_parts(self.data, self.len, self.capacity)
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

fn silent_logger() -> Box<dyn ProgressLogger> {
    Box::new(SilentProgressLogger)
}

fn do_encode(src: &[u8], cfg: &LZCfg) -> Result<Vec<u8>, OrzError> {
    let mut input = CountRead::new(Cursor::new(src));
    let mut output = CountWrite::new(Cursor::new(Vec::with_capacity(src.len() / 2 + 64)));
    let mut logger = silent_logger();
    encode(&mut input, &mut output, cfg, &mut logger).map_err(|_| OrzError::EncodeFailed)?;
    Ok(output.into_inner().into_inner())
}

fn do_decode(src: &[u8]) -> Result<Vec<u8>, OrzError> {
    let mut input = CountRead::new(Cursor::new(src));
    // Start with a reasonably sized buffer; the decoder will grow as needed via Write.
    let mut output = CountWrite::new(Cursor::new(Vec::with_capacity(src.len().saturating_mul(3) + 64)));
    let mut logger = silent_logger();
    decode(&mut input, &mut output, &mut logger).map_err(|_| OrzError::DecodeFailed)?;
    Ok(output.into_inner().into_inner())
}

// ---------------------------------------------------------------------------
// Public C API
// ---------------------------------------------------------------------------

/// Return a configuration for the given compression level (0, 1 or 2).
///
/// On invalid level the returned struct is zeroed and the function returns
/// `OrzError::InvalidLevel`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn orz_cfg_from_level(level: c_int, out_cfg: *mut OrzCfg) -> c_int {
    if out_cfg.is_null() {
        return OrzError::NullPointer.into();
    }
    match OrzCfg::from_level(level as u8) {
        Some(cfg) => {
            *out_cfg = cfg;
            OrzError::Ok.into()
        }
        None => {
            *out_cfg = OrzCfg {
                match_depth: 0,
                lazy_match_depth1: 0,
                lazy_match_depth2: 0,
            };
            OrzError::InvalidLevel.into()
        }
    }
}

/// Compress `src_len` bytes at `src` into a newly allocated buffer.
///
/// On success `*out` is filled and must later be released with `orz_buffer_free`.
/// `level` must be 0, 1 or 2 (same as the CLI).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn orz_encode(
    src: *const u8,
    src_len: usize,
    level: c_int,
    out: *mut OrzBuffer,
) -> c_int {
    if out.is_null() {
        return OrzError::NullPointer.into();
    }
    // Initialise out to a safe empty state in case of early return.
    *out = OrzBuffer {
        data: ptr::null_mut(),
        len: 0,
        capacity: 0,
    };

    if src.is_null() && src_len != 0 {
        return OrzError::NullPointer.into();
    }

    let cfg = match OrzCfg::from_level(level as u8) {
        Some(c) => LZCfg::from(&c),
        None => return OrzError::InvalidLevel.into(),
    };

    let src_slice = if src_len == 0 {
        &[][..]
    } else {
        slice::from_raw_parts(src, src_len)
    };

    match do_encode(src_slice, &cfg) {
        Ok(buf) => {
            *out = OrzBuffer::from_vec(buf);
            OrzError::Ok.into()
        }
        Err(e) => e.into(),
    }
}

/// Compress using an explicit configuration.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn orz_encode_with_cfg(
    src: *const u8,
    src_len: usize,
    cfg: *const OrzCfg,
    out: *mut OrzBuffer,
) -> c_int {
    if out.is_null() || cfg.is_null() {
        return OrzError::NullPointer.into();
    }
    *out = OrzBuffer {
        data: ptr::null_mut(),
        len: 0,
        capacity: 0,
    };

    if src.is_null() && src_len != 0 {
        return OrzError::NullPointer.into();
    }

    let src_slice = if src_len == 0 {
        &[][..]
    } else {
        slice::from_raw_parts(src, src_len)
    };

    let lz_cfg = LZCfg::from(&*cfg);

    match do_encode(src_slice, &lz_cfg) {
        Ok(buf) => {
            *out = OrzBuffer::from_vec(buf);
            OrzError::Ok.into()
        }
        Err(e) => e.into(),
    }
}

/// Decompress `src_len` bytes at `src` into a newly allocated buffer.
///
/// On success `*out` is filled and must later be released with `orz_buffer_free`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn orz_decode(
    src: *const u8,
    src_len: usize,
    out: *mut OrzBuffer,
) -> c_int {
    if out.is_null() {
        return OrzError::NullPointer.into();
    }
    *out = OrzBuffer {
        data: ptr::null_mut(),
        len: 0,
        capacity: 0,
    };

    if src.is_null() && src_len != 0 {
        return OrzError::NullPointer.into();
    }

    let src_slice = if src_len == 0 {
        &[][..]
    } else {
        slice::from_raw_parts(src, src_len)
    };

    match do_decode(src_slice) {
        Ok(buf) => {
            *out = OrzBuffer::from_vec(buf);
            OrzError::Ok.into()
        }
        Err(e) => e.into(),
    }
}

/// Compress into a caller-provided buffer.
///
/// `*dst_len` is both an input (capacity of `dst`) and an output (bytes written).
/// Returns `OrzError::BufferTooSmall` when the supplied buffer is insufficient;
/// in that case `*dst_len` is left unchanged.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn orz_encode_into(
    src: *const u8,
    src_len: usize,
    level: c_int,
    dst: *mut u8,
    dst_len: *mut usize,
) -> c_int {
    if dst_len.is_null() {
        return OrzError::NullPointer.into();
    }
    if src.is_null() && src_len != 0 {
        return OrzError::NullPointer.into();
    }
    if dst.is_null() && *dst_len != 0 {
        return OrzError::NullPointer.into();
    }

    let cfg = match OrzCfg::from_level(level as u8) {
        Some(c) => LZCfg::from(&c),
        None => return OrzError::InvalidLevel.into(),
    };

    let src_slice = if src_len == 0 {
        &[][..]
    } else {
        slice::from_raw_parts(src, src_len)
    };

    match do_encode(src_slice, &cfg) {
        Ok(buf) => {
            if buf.len() > *dst_len {
                return OrzError::BufferTooSmall.into();
            }
            if !buf.is_empty() {
                ptr::copy_nonoverlapping(buf.as_ptr(), dst, buf.len());
            }
            *dst_len = buf.len();
            OrzError::Ok.into()
        }
        Err(e) => e.into(),
    }
}

/// Decompress into a caller-provided buffer.
///
/// `*dst_len` is both an input (capacity of `dst`) and an output (bytes written).
/// Returns `OrzError::BufferTooSmall` when the supplied buffer is insufficient.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn orz_decode_into(
    src: *const u8,
    src_len: usize,
    dst: *mut u8,
    dst_len: *mut usize,
) -> c_int {
    if dst_len.is_null() {
        return OrzError::NullPointer.into();
    }
    if src.is_null() && src_len != 0 {
        return OrzError::NullPointer.into();
    }
    if dst.is_null() && *dst_len != 0 {
        return OrzError::NullPointer.into();
    }

    let src_slice = if src_len == 0 {
        &[][..]
    } else {
        slice::from_raw_parts(src, src_len)
    };

    match do_decode(src_slice) {
        Ok(buf) => {
            if buf.len() > *dst_len {
                return OrzError::BufferTooSmall.into();
            }
            if !buf.is_empty() {
                ptr::copy_nonoverlapping(buf.as_ptr(), dst, buf.len());
            }
            *dst_len = buf.len();
            OrzError::Ok.into()
        }
        Err(e) => e.into(),
    }
}

/// Free a buffer previously returned by `orz_encode` / `orz_decode`.
///
/// Passing a null or already-freed buffer is safe (no-op).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn orz_buffer_free(buf: *mut OrzBuffer) {
    if buf.is_null() {
        return;
    }
    let b = ptr::read(buf);
    // Drop the Vec, which frees the allocation.
    let _ = b.into_vec();
    // Leave the caller's struct in a safe empty state.
    *buf = OrzBuffer {
        data: ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
}

/// Convenience: free only the data pointer (when the caller copied the fields
/// out of an `OrzBuffer` and no longer has the struct).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn orz_free(ptr: *mut c_void, capacity: usize) {
    if ptr.is_null() {
        return;
    }
    // Reconstruct a Vec with the original capacity so the allocator receives
    // the correct layout. Length is irrelevant for deallocation.
    let _ = Vec::from_raw_parts(ptr as *mut u8, 0, capacity);
}
