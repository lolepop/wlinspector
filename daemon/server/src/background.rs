use std::{sync::{Arc, RwLock}, thread::{self, sleep, JoinHandle}, time::Duration};

use crate::state::State;

pub (crate) fn prune_dead_processes(state: Arc<RwLock<State>>) {
    let mut system = sysinfo::System::new();
    debug!("dead processes pruning triggered");

    system.refresh_processes_specifics(sysinfo::ProcessesToUpdate::All, true, sysinfo::ProcessRefreshKind::nothing());
        
    let mut inner = state.write().unwrap();
    let invalid_procs = inner.processes.iter()
        .filter_map(|(k, v)|
            system.process((*k as usize).into())
                .is_some_and(|p| p.start_time() > v.last_update)
                .then(|| *k)
        ).collect::<Vec<_>>();

    for i in invalid_procs.into_iter() {
        debug!("removing dead process: {i}");
        inner.processes.remove(&i).expect("failed to remove from process hashmap");
    }
}

fn task_runner<F: FnMut() + Send + 'static>(mut f: F, interval: Duration) -> JoinHandle<()> {
    thread::spawn(move || {
        loop {
            sleep(interval);
            f();
        }
    })
}

pub (crate) fn start_background_tasks(state: Arc<RwLock<State>>) {
    let s = state.clone();
    task_runner(move || prune_dead_processes(s.clone()), Duration::from_secs(10 * 60));
}