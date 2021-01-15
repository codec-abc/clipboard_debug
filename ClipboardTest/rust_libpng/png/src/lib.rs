use std::mem;
use std::ffi::CStr;
use std::os::raw::c_char;
use libc::size_t;
use image::ImageResult;
use image::DynamicImage;
use image::GenericImageView;
use std::io::Write;
use std::rc::Rc;
use std::cell::RefCell;

struct VecEncoder {
    inner_vec: Rc<RefCell<Vec::<u8>>>
}


impl VecEncoder {
    fn new(inner_vec: Rc<RefCell<Vec::<u8>>>) -> VecEncoder {
        VecEncoder {
            inner_vec
        }
    }
}


impl Write for VecEncoder {
    fn write(&mut self, content_to_write: &[u8]) -> std::result::Result<usize, std::io::Error> { 
        let mut borrow = self.inner_vec.borrow_mut();
        borrow.write(content_to_write)
    }

    fn flush(&mut self) -> std::result::Result<(), std::io::Error> { 
        let mut borrow = self.inner_vec.borrow_mut();
        borrow.flush()
    }
 
}

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
                    let buffer = Rc::new(RefCell::new(Vec::<u8>::new()));
                    let vec_encoder = VecEncoder::new(buffer.clone());
                    let png_encoder = image::codecs::png::PngEncoder::new(vec_encoder);
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
                            let borrow = buffer.borrow();
                            let mut clone = borrow.clone();
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