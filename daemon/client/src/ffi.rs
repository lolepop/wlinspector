use wlinspector_common::ffi::VecFree;
use wlinspector_common::{Process, Window};

#[repr(C)]
pub struct CProcess {
    pub pid: u32,
    pub last_update: u64,
    pub windows: VecArr<Window>
}

impl From<Process> for CProcess {
    fn from(mut p: Process) -> Self {
        unsafe {
            let windows = p.windows.drain().map(|(_, v)| v).collect();
            CProcess {
                pid: p.pid,
                last_update: p.last_update,
                windows: VecArr::from_vec(windows),
            }
        }
    }
}

impl VecFree for CProcess {
    unsafe fn free(mut self) {
        self.windows.free_ref();
    }
}

#[repr(C)]
pub struct VecArr<T: VecFree> {
    arr: *mut T,
    length: usize,
    cap: usize
}

impl<T: VecFree> VecArr<T> {
    pub unsafe fn from_vec(mut v: Vec<T>) -> Self {
        let out = Self {
            arr: v.as_mut_ptr(),
            length: v.len(),
            cap: v.capacity(),
        };
        std::mem::forget(v);
        out
    }

    unsafe fn free_ref(self: &mut Self) {
        let mut v = Vec::from_raw_parts(self.arr, self.length, self.cap);
        for i in v.into_iter() {
            i.free();
        }
    }

    unsafe fn free(this: *mut Self) {
        let this = this.as_mut().expect("null pointer passed to free");
        this.free_ref();
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn wli_list_window_info() -> VecArr<CProcess> {
    let res = crate::handlers::list_window_info()
        .unwrap() // TODO: error handling, should not panic if connection fails
        .into_iter()
        .map(|p| p.into())
        .collect();
    VecArr::from_vec(res)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn wli_free_window_info(info: *mut VecArr<CProcess>) {
    VecArr::free(info);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn vecarr_smoke() {
        unsafe {
            let mut info = wli_list_window_info();
            let s = std::slice::from_raw_parts(info.arr, info.length);
            let w = s.iter()
                .flat_map(|process| unsafe {
                    std::slice::from_raw_parts(process.windows.arr, process.windows.length)
                })
                .collect::<Vec<_>>();
            println!("{w:?}");
            wli_free_window_info(&mut info as *mut _);
        }
    }
}