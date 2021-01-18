use std::mem;
use std::ffi::CStr;
use std::os::raw::c_char;
use libc::size_t;
use image::ImageResult;
use image::DynamicImage;
use image::GenericImageView;

#[no_mangle]
pub unsafe extern "C" fn open_image(
    cstr_path: *const c_char, 
    out_ptr: *mut *mut u8,
    width: *mut size_t,
    height: *mut size_t
) -> size_t {
    let path = CStr::from_ptr(cstr_path).to_string_lossy().into_owned();
    let image_result = image::io::Reader::open(path);
    
    match image_result {
        Ok(image) => {
            println!("file exist");
            let decoded_result: ImageResult<DynamicImage> = image.decode();
            match decoded_result {
                Ok(image_decoded) => {
                    println!("image decoded");
                    let rgba = image_decoded.to_rgba8();
                    let raw: &[u8] = &(*rgba.into_raw());
                    let mut as_vec: Vec<u8> = raw.to_vec();
                    let length = as_vec.len();
                    let ptr: *mut u8 = as_vec.as_mut_ptr();
                    *width = image_decoded.width() as usize;
                    *height = image_decoded.height() as usize;
                    *out_ptr = ptr;
                    mem::forget(as_vec);
                    return length;
                },
                Err(err) => {
                    println!("error decoding image {}", err);
                    return 0;
                }
            };
        },
        Err(err) => {
            println!("error opening image {}", err);
            return 0;
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn encode_image_as_png(
    cstr_path: *const c_char, 
    out_ptr: *mut *mut u8
) -> size_t {
    let path = CStr::from_ptr(cstr_path).to_string_lossy().into_owned();
    let image_result = image::io::Reader::open(path);
    
    match image_result {
        Ok(image) => {
            println!("file exist");
            let decoded_result: ImageResult<DynamicImage> = image.decode();
            match decoded_result {
                Ok(image_decoded) => {
                    let mut buffer = Vec::<u8>::new();
                    let png_encoder = image::codecs::png::PngEncoder::new(&mut buffer);
                    let rgba = image_decoded.to_rgba8();
                    let raw: &[u8] = &(*rgba.into_raw());
                    
                    let result = png_encoder.encode(
                        raw, 
                        image_decoded.width(), 
                        image_decoded.height(),
                        image::ColorType::Rgba8
                    );

                    match result {
                        Err(err) => {
                            println!("error encoding image {}", err);
                            return 0;
                        }
                        Ok(_) => {
                            let mut clone = buffer.clone();
                            let length = clone.len();
                            println!("image encoded successfully on {} bytes", length);
                            let ptr: *mut u8 = clone.as_mut_ptr();
                            *out_ptr = ptr;
                            mem::forget(clone);
                            return length;
                        }
                    }
                },
                Err(err) => {
                    println!("error decoding image {}", err);
                    return 0;
                }
            };
        },
        Err(err) => {
            println!("error opening image {}", err);
            return 0;
        }
    }
}