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
                    //*len = length;
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