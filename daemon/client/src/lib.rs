#![allow(warnings)]

pub mod ffi;

pub mod connection_string {
    pub use wlinspector_common::*;
}

pub mod handlers {
    use dbus::blocking::Connection;
    use wlinspector_common::{methods, Method, Process};
    use anyhow::Result;

    pub fn list_window_info() -> Result<Vec<Process>> {
        let conn = Connection::new_session()?;
        let proxy = conn.with_proxy(wlinspector_common::DBUS_CONNECTION_NAME, methods::PATH, std::time::Duration::from_millis(5000));
        let (info,): (String,) = proxy.method_call(wlinspector_common::DBUS_CONNECTION_NAME, methods::ListWindowInfo::NAME, ())?;
        let info: Vec<Process> = serde_json::from_str(&info)?;
        Ok(info)
    }
}
