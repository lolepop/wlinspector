mod background;
mod state;

use std::sync::{Arc, RwLock};

use anyhow::Result;
use background::{prune_dead_processes, start_background_tasks};
use dbus::blocking::Connection;
use dbus_crossroads::Crossroads;
use state::State;
use wlinspector_common::{methods, Method, Process, DBUS_CONNECTION_NAME};

#[macro_use]
extern crate log;

fn start(state: Arc<RwLock<State>>) -> Result<()> {
    let conn = Connection::new_session()?;
    conn.request_name(DBUS_CONNECTION_NAME, false, true, false)?;

    let mut cr = Crossroads::new();
    let token = cr.register(DBUS_CONNECTION_NAME, |b| {
        let s = state.clone();
        b.method(
            methods::WindowInfo::NAME,
            ("origin_pid", "window_info"),
            (),
            move |_, _, (origin_pid, window_info): <methods::WindowInfo as Method>::Signature| {
                debug!("WindowInfo called: {origin_pid}, {window_info}");
                match serde_json::from_str(&window_info) {
                    Ok(windows) => {
                        let mut inner = s.write().unwrap();
                        if let Some(proc) = inner.processes.get_mut(&origin_pid) {
                            proc.update_windows(windows);
                        } else {
                            let mut p = Process::new(origin_pid);
                            p.update_windows(windows);
                            inner.processes.insert(origin_pid, p);
                        }
                    }
                    Err(e) => warn!("WindowInfo invalid data received from sender ({origin_pid}), {e}: {window_info}"),
                }
                Ok(())
            },
        );

        let s = state.clone();
        b.method(
            methods::ListWindowInfo::NAME,
            (),
            ("window_info",),
            move |_, _, (): <methods::ListWindowInfo as Method>::Signature| {
                debug!("ListWindowInfo called");
                prune_dead_processes(s.clone());
                let inner = s.read().unwrap();
                let processes: Vec<_> = inner.processes.values().cloned().collect();
                Ok((serde_json::to_string(&processes).unwrap(),))
            },
        );
    });
    cr.insert(methods::PATH, &[token], ());
    cr.serve(&conn)?;
    Ok(())
}

fn main() -> Result<()> {
    // pretty_env_logger::init();
    env_logger::init();
    let state = Arc::new(RwLock::new(State::new()));
    start_background_tasks(state.clone());
    start(state)?;
    Ok(())
}
