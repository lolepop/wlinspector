use std::{collections::HashMap, time::SystemTime};
use serde::{Deserialize, Serialize};

pub const DBUS_CONNECTION_NAME: &str = "lolepopie.wlinspector";

pub trait Method {
    const NAME: &str;
    type Signature;
}

pub mod methods {
    use crate::Method;
    pub const PATH: &str = "/main";

    pub struct WindowInfo;
    impl Method for WindowInfo {
        const NAME: &str = "WindowInfo";
        type Signature = (u32, String);
    }

    pub struct ListWindowInfo;
    impl Method for ListWindowInfo {
        const NAME: &str = "ListWindowInfo";
        type Signature = ();
    }
}

#[derive(Deserialize, Serialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Process {
    pub pid: u32,
    pub last_update: u64,
    pub windows: HashMap<u64, Window>
}

#[repr(C)]
#[derive(Deserialize, Serialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Window {
    pub app_id: String,
    pub title: String
}
impl ffi::VecFree for Window {}

impl Process {
    pub fn new(pid: u32) -> Self {
        Self { pid, last_update: Self::now(), windows: HashMap::new() }
    }

    pub fn update_windows(&mut self, windows: HashMap<u64, Window>) {
        self.last_update = Self::now();
        self.windows = windows;
    }

    fn now() -> u64 {
        SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_secs()
    }
}

pub mod ffi {
    pub trait VecFree {
        unsafe fn free(self) where Self: Sized {}
    }
}